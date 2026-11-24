#!/usr/bin/env python3
"""
Runner for the IFJ2026 integration tests (designed to be invoked directly
via Makefile: `make test`).

Every test lives in testing/tests as <name>.ifj plus:
  <name>.rc      expected result, one line:
                   c:N   compiler must exit with N (N may be "a|b" for alternatives),
                         the interpreter is not run
                   r:N   compiler must exit 0, interpreter must exit with N
  <name>.stdout  expected interpreter stdout, byte-exact (checked when r:0)
  <name>.in      optional interpreter stdin

Each test runs ./compiler, checks the expected compiler return code
(for semantic/syntax/lex errors) or executes the generated IFJcode26
in ./ic26int-linux-x86_64 and verifies the return code and stdout.
Failed tests print the expected and actual results and a diff of stdout.

Usage:
    python3 run_simple_tests.py
    python3 run_simple_tests.py --verbose
    python3 run_simple_tests.py --only read_int,unit_values
    python3 run_simple_tests.py --valgrind
    python3 run_simple_tests.py --save-logs testing
"""

import argparse
import difflib
import os
import re
import shutil
import subprocess
import sys
import tempfile
from dataclasses import dataclass
from typing import List, Optional, Tuple

GREEN = "\033[32m"
RED = "\033[31m"
YELLOW = "\033[33m"
CYAN = "\033[36m"
BOLD = "\033[1m"
RESET = "\033[0m"


@dataclass
class TestCase:
    name: str
    code: bytes
    # accepted compiler return codes
    expected_rc: Tuple[int, ...] = (0,)
    # interpreter return code; None = compile only, do not run the interpreter
    expected_int_rc: Optional[int] = 0
    # byte-exact expected stdout; None = not checked
    expected_stdout: Optional[bytes] = None
    stdin_data: bytes = b""
    # set when the .rc file is malformed
    error: str = ""


@dataclass
class TestResult:
    ok: bool
    detail: str
    ifjcode: str = ""
    stderr: str = ""
    vg_issue: str = ""
    comp_rc: Optional[int] = None
    int_rc: Optional[int] = None
    stdout: Optional[bytes] = None


def load_tests(directory: str) -> List[TestCase]:
    """Load <name>.ifj tests with their .rc/.stdout/.in files from a directory."""
    tests: List[TestCase] = []
    for fname in sorted(os.listdir(directory)):
        if not fname.endswith(".ifj"):
            continue
        name = fname[: -len(".ifj")]
        base = os.path.join(directory, name)

        def read(ext: str) -> Optional[bytes]:
            path = base + ext
            if not os.path.isfile(path):
                return None
            with open(path, "rb") as f:
                return f.read()

        test = TestCase(name=name, code=read(".ifj") or b"")
        rc_line = (read(".rc") or b"").decode("utf-8").strip()
        kind, _, value = rc_line.partition(":")
        try:
            codes = tuple(int(x) for x in value.split("|"))
        except ValueError:
            codes = ()

        if kind == "c" and codes:
            test.expected_rc = codes
            test.expected_int_rc = None
        elif kind == "r" and len(codes) == 1:
            test.expected_int_rc = codes[0]
            if codes[0] == 0:
                test.expected_stdout = read(".stdout") or b""
            test.stdin_data = read(".in") or b""
        else:
            test.error = f"bad .rc file {rc_line!r} (want c:N or r:N)"
        tests.append(test)
    return tests


def check_valgrind_log(log_text: str) -> Tuple[bool, str]:
    err_match = re.search(r"ERROR SUMMARY:\s+(\d+)\s+errors", log_text)
    errors = int(err_match.group(1)) if err_match else -1
    def_lost = re.search(r"definitely lost:\s+([\d,]+)\s+bytes", log_text)
    ind_lost = re.search(r"indirectly lost:\s+([\d,]+)\s+bytes", log_text)
    def_bytes = int(def_lost.group(1).replace(",", "")) if def_lost else 0
    ind_bytes = int(ind_lost.group(1).replace(",", "")) if ind_lost else 0
    if errors == 0 and def_bytes == 0 and ind_bytes == 0:
        return True, ""
    return False, f"valgrind errors={errors}, definitely_lost={def_bytes}B, indirectly_lost={ind_bytes}B"


def as_text(data: bytes) -> str:
    return data.decode("utf-8", errors="replace")


def visible(data: bytes) -> str:
    """Show stdout on one line with escapes (\\n, \\t, \\r, trailing spaces visible)."""
    return repr(as_text(data))


def describe_expected(test: TestCase) -> str:
    rc = "|".join(map(str, test.expected_rc))
    if test.expected_int_rc is None:
        return f"compiler rc={rc}"
    text = f"compiler rc={rc}, interpreter rc={test.expected_int_rc}"
    if test.expected_stdout is not None:
        text += f", stdout={visible(test.expected_stdout)}"
    return text


def describe_actual(result: TestResult) -> str:
    if result.comp_rc is None:
        return "-"
    text = f"compiler rc={result.comp_rc}"
    if result.int_rc is not None:
        text += f", interpreter rc={result.int_rc}"
    if result.stdout is not None:
        text += f", stdout={visible(result.stdout)}"
    return text


def stdout_diff(expected: bytes, actual: bytes) -> List[str]:
    """Unified line diff of expected vs actual stdout, each line shown with escapes."""
    exp_lines = [repr(line)[1:-1] for line in as_text(expected).splitlines(keepends=True)]
    act_lines = [repr(line)[1:-1] for line in as_text(actual).splitlines(keepends=True)]
    return list(
        difflib.unified_diff(exp_lines, act_lines, "expected", "actual", n=2, lineterm="")
    )


def colorize_diff(line: str) -> str:
    if line.startswith(("---", "+++")):
        return f"{BOLD}{line}{RESET}"
    if line.startswith("-"):
        return f"{RED}{line}{RESET}"
    if line.startswith("+"):
        return f"{GREEN}{line}{RESET}"
    if line.startswith("@@"):
        return f"{CYAN}{line}{RESET}"
    return line


def run_single_test(
    test: TestCase,
    compiler_bin: str,
    interp_bin: str,
    use_valgrind: bool,
    timeout: float,
) -> TestResult:
    if test.error:
        return TestResult(False, test.error)

    vg_log_path: Optional[str] = None
    vg_issue = ""

    if use_valgrind:
        fd, vg_log_path = tempfile.mkstemp(prefix="ifj26_vg_", suffix=".log")
        os.close(fd)
        comp_cmd = [
            "valgrind",
            "--leak-check=full",
            "--show-leak-kinds=definite,indirect",
            "--track-origins=yes",
            f"--log-file={vg_log_path}",
            compiler_bin,
        ]
    else:
        comp_cmd = [compiler_bin]

    # Step 1: Compile
    try:
        comp_proc = subprocess.run(comp_cmd, input=test.code, capture_output=True, timeout=timeout)
    except subprocess.TimeoutExpired:
        if vg_log_path and os.path.exists(vg_log_path):
            os.remove(vg_log_path)
        return TestResult(False, "COMPILE TIMEOUT")

    vg_ok = True
    if use_valgrind and vg_log_path:
        try:
            with open(vg_log_path, "r", encoding="utf-8", errors="replace") as f:
                log_text = f.read()
            vg_ok, vg_issue = check_valgrind_log(log_text)
        finally:
            if os.path.exists(vg_log_path):
                os.remove(vg_log_path)

    ifjcode_raw = comp_proc.stdout
    result = TestResult(
        ok=False,
        detail="",
        ifjcode=as_text(ifjcode_raw),
        stderr=as_text(comp_proc.stderr),
        vg_issue=vg_issue,
        comp_rc=comp_proc.returncode,
    )
    comp_rc = comp_proc.returncode

    if not vg_ok:
        result.detail = "valgrind error"
        return result

    if comp_rc not in test.expected_rc:
        extra = f" [SIGNAL {-comp_rc}]" if comp_rc < 0 else ""
        expected = "|".join(map(str, test.expected_rc))
        result.detail = f"compiler rc={comp_rc}{extra} (expected {expected})"
        return result

    # For compile-only tests (expected compile-time errors), DO NOT run the interpreter.
    if comp_rc != 0 or test.expected_int_rc is None:
        result.ok = True
        result.detail = f"rc={comp_rc}"
        return result

    # Step 2: Execute generated IFJcode26 on interpreter
    fd, code_path = tempfile.mkstemp(prefix=f"ifj26_{test.name}_", suffix=".ifjcode")
    os.close(fd)
    try:
        with open(code_path, "wb") as f:
            f.write(ifjcode_raw)
        try:
            int_proc = subprocess.run(
                [interp_bin, code_path],
                input=test.stdin_data,
                capture_output=True,
                timeout=timeout,
            )
        except subprocess.TimeoutExpired:
            result.detail = f"INTERPRETER TIMEOUT after {timeout}s"
            return result
    finally:
        if os.path.exists(code_path):
            os.remove(code_path)

    result.int_rc = int_proc.returncode
    result.stdout = int_proc.stdout
    result.stderr = as_text(int_proc.stderr)

    if int_proc.returncode != test.expected_int_rc:
        result.detail = f"interpreter rc={int_proc.returncode} (expected {test.expected_int_rc})"
        return result

    if test.expected_stdout is not None and int_proc.stdout != test.expected_stdout:
        result.detail = "stdout mismatch"
        return result

    result.ok = True
    if test.expected_int_rc != 0:
        result.detail = f"interpreter rc={test.expected_int_rc}"
    else:
        result.detail = visible(int_proc.stdout)
    return result


def main() -> int:
    parser = argparse.ArgumentParser(description="Run IFJ2026 integration tests.")
    parser.add_argument("--bin", default="./compiler", help="Path to compiler binary")
    parser.add_argument(
        "--int",
        dest="interp",
        default="./ic26int-linux-x86_64",
        help="Path to IFJcode26 interpreter",
    )
    parser.add_argument(
        "--tests-dir",
        default="testing/tests",
        help="Directory with <name>.ifj/.rc/.stdout/.in tests",
    )
    parser.add_argument("--valgrind", action="store_true", help="Run compiler under Valgrind")
    parser.add_argument("--verbose", action="store_true", help="Show source/stderr on failure")
    parser.add_argument(
        "--only",
        default="",
        help="Comma-separated test names to run (e.g. --only read_int,unit_values)",
    )
    parser.add_argument(
        "--save-logs",
        default="",
        help="Optional directory to write fresh IFJCODE.txt, RESULTS.txt and DIFF.txt",
    )
    parser.add_argument("--timeout", type=float, default=5.0, help="Timeout per step (s)")
    args = parser.parse_args()

    if not os.path.isfile(args.bin) or not os.access(args.bin, os.X_OK):
        print(f"{RED}Error:{RESET} Compiler binary '{args.bin}' not found or not executable.")
        return 1
    if not os.path.isfile(args.interp) or not os.access(args.interp, os.X_OK):
        print(f"{RED}Error:{RESET} Interpreter '{args.interp}' not found or not executable.")
        return 1
    if args.valgrind and shutil.which("valgrind") is None:
        print(f"{RED}Error:{RESET} valgrind not found in PATH.")
        return 1
    if not os.path.isdir(args.tests_dir):
        print(f"{RED}Error:{RESET} Test directory '{args.tests_dir}' not found.")
        return 1

    tests = load_tests(args.tests_dir)
    if not tests:
        print(f"{RED}Error:{RESET} No <name>.ifj tests found in '{args.tests_dir}'.")
        return 1

    if args.only:
        wanted = {x.strip() for x in args.only.split(",") if x.strip()}
        tests = [t for t in tests if t.name in wanted]
        if not tests:
            print(f"{YELLOW}No tests matched --only {args.only}.{RESET}")
            return 1

    mode = "VALGRIND + INTERPRETER" if args.valgrind else "INTERPRETER"
    print(f"{BOLD}{'=' * 70}{RESET}")
    print(f"{BOLD}  IFJ2026 TESTS ({mode}) | Source: {args.tests_dir}{RESET}")
    print(f"{BOLD}{'=' * 70}{RESET}")

    passed = 0
    failed_names: List[str] = []
    ifjcode_log: List[str] = ["IFJCODE:\n"]
    results_log: List[str] = ["RESULTS:\n"]
    diff_log: List[str] = ["DIFF (failed tests only):\n"]

    for test in tests:
        res = run_single_test(test, args.bin, args.interp, args.valgrind, args.timeout)
        expected = describe_expected(test)
        actual = describe_actual(res)
        diff: List[str] = []
        if (
            not res.ok
            and test.expected_stdout is not None
            and res.stdout is not None
            and res.stdout != test.expected_stdout
        ):
            diff = stdout_diff(test.expected_stdout, res.stdout)

        status = "PASS" if res.ok else "FAIL"
        ifjcode_log.append(f"\n{test.name}:\n{res.ifjcode.rstrip()}\n")
        results_log.append(
            f"{status} {test.name}\n    expected: {expected}\n    actual:   {actual}\n"
        )

        if res.ok:
            passed += 1
            print(f"{GREEN}[PASS]{RESET} {test.name} -> {CYAN}{res.detail}{RESET}")
            continue

        failed_names.append(test.name)
        print(f"{RED}[FAIL]{RESET} {test.name} -> {res.detail}")
        print(f"       {BOLD}expected:{RESET} {expected}")
        print(f"       {BOLD}actual:{RESET}   {actual}")
        for line in diff:
            print(f"       {colorize_diff(line)}")
        stderr = res.stderr.strip().replace("\n", "\n               ")
        if stderr:
            print(f"       {YELLOW}stderr:{RESET} {stderr}")
        if res.vg_issue:
            print(f"       {RED}valgrind:{RESET} {res.vg_issue}")
        if args.verbose:
            print(f"       {BOLD}--- source ({test.name}) ---{RESET}")
            for line_no, line in enumerate(as_text(test.code).splitlines(), 1):
                print(f"       {line_no:2d} | {line}")

        diff_log.append(
            f"\n{test.name}: {res.detail}\n    expected: {expected}\n    actual:   {actual}\n"
        )
        diff_log.extend(f"    {line}\n" for line in diff)
        if stderr:
            diff_log.append(f"    stderr: {stderr}\n")

    if args.save_logs:
        os.makedirs(args.save_logs, exist_ok=True)
        for fname, lines in (
            ("IFJCODE.txt", ifjcode_log),
            ("RESULTS.txt", results_log),
            ("DIFF.txt", diff_log),
        ):
            with open(os.path.join(args.save_logs, fname), "w", encoding="utf-8") as f:
                f.writelines(lines)
        print(
            f"\n{CYAN}Saved logs to {args.save_logs}/IFJCODE.txt, "
            f"{args.save_logs}/RESULTS.txt and {args.save_logs}/DIFF.txt{RESET}"
        )

    failed = len(failed_names)
    total = passed + failed
    color = GREEN if failed == 0 else RED
    print(f"\n{BOLD}{'=' * 70}{RESET}")
    print(
        f"{BOLD}Summary:{RESET} Total: {total} | "
        f"{GREEN}Passed: {passed}{RESET} | {color}Failed: {failed}{RESET}"
    )
    if failed_names:
        print(f"{RED}Failed:{RESET} {' '.join(failed_names)}")
    print(f"{BOLD}{'=' * 70}{RESET}")
    return 0 if failed == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
