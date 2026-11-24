#!/usr/bin/env python3
"""
Runner for the simple IFJ2026 integration tests stored in SCALACODE
(designed to be invoked directly via Makefile: `make test`).

Parses numbered test blocks (`1:`, `2:`, ..., `9:`) from SCALACODE,
runs ./compiler on each block, checks expected compiler return codes
(for semantic/syntax/lex errors) or executes the generated IFJcode26
in ./ic26int-linux-x86_64 and verifies stdout.

Usage:
    python3 run_simple_tests.py
    python3 run_simple_tests.py --source testing/SCALACODE --verbose
    python3 run_simple_tests.py --valgrind
    python3 run_simple_tests.py --save-logs testing
"""

import argparse
import math
import os
import re
import shutil
import subprocess
import sys
import tempfile
from dataclasses import dataclass
from typing import Dict, List, Optional, Tuple, Union

GREEN = "\033[32m"
RED = "\033[31m"
YELLOW = "\033[33m"
CYAN = "\033[36m"
BOLD = "\033[1m"
RESET = "\033[0m"


@dataclass
class ExpectedSpec:
    name: str
    expected_rc: int = 0
    expected_stdout: Optional[Union[str, Tuple[str, ...]]] = None
    stdin_data: str = ""


# Expected results matching testing/RESULTS.txt (tests 1-9).
# Edit or add entries here whenever you append new numbered tests to SCALACODE.
EXPECTED: Dict[int, ExpectedSpec] = {
    1: ExpectedSpec(
        name="Aritmetika a unarni minus (Int + Double)",
        expected_rc=0,
        expected_stdout="16 -0x1.ep+2",
    ),
    2: ExpectedSpec(
        name="Test 2 (120)",
        expected_rc=0,
        expected_stdout="120",
    ),
    3: ExpectedSpec(
        name="Test 3 (15 100)",
        expected_rc=0,
        expected_stdout=("15 100", "15, 100"),
    ),
    4: ExpectedSpec(
        name="Retezce a vestavene funkce",
        expected_rc=0,
        expected_stdout="IFJ2026 7 IFJ 73 A",
    ),
    5: ExpectedSpec(
        name="Vstup ze stdin (a - b)",
        expected_rc=0,
        expected_stdout="3",
        stdin_data="10\n7\n",
    ),
    6: ExpectedSpec(
        name="Komplexni vypocet",
        expected_rc=0,
        expected_stdout="6 21 125 777 16 673",
    ),
    7: ExpectedSpec(
        name="Semanticka chyba 3 (nedeklarovana promenna)",
        expected_rc=3,
    ),
    8: ExpectedSpec(
        name="Semanticka chyba 6 (vzajemna rekurze bez typu)",
        expected_rc=6,
    ),
    9: ExpectedSpec(
        name="Semanticka chyba 7 (zmena val)",
        expected_rc=7,
    ),
}


def find_source_file(explicit_path: str) -> Optional[str]:
    if explicit_path:
        return explicit_path if os.path.isfile(explicit_path) else None
    candidates = [
        "testing/SCALACODE",
        "testing/SCALACODE.txt",
        "SCALACODE",
        "SCALACODE.txt",
    ]
    for path in candidates:
        if os.path.isfile(path):
            return path
    return None


def parse_scalacode(path: str) -> List[Tuple[int, str]]:
    """Parse numbered test blocks (`1:`, `2:`, ...) from a SCALACODE file."""
    with open(path, "r", encoding="utf-8") as f:
        lines = f.readlines()

    tests: List[Tuple[int, str]] = []
    current_id: Optional[int] = None
    current_lines: List[str] = []

    header_re = re.compile(r"^\s*(\d+)\s*:\s*$")

    for line in lines:
        if current_id is None and line.strip().upper() in ("", "SCALACODE", "SCALACODE:"):
            continue
        m = header_re.match(line)
        if m:
            if current_id is not None:
                code = "".join(current_lines).strip("\n") + "\n"
                tests.append((current_id, code))
            current_id = int(m.group(1))
            current_lines = []
        elif current_id is not None:
            current_lines.append(line)

    if current_id is not None:
        code = "".join(current_lines).strip("\n") + "\n"
        tests.append((current_id, code))

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


def tokens_equivalent(actual: str, expected: str) -> bool:
    """Exact match, with fallback comparing hex/decimal float tokens numerically."""
    a_clean = actual.strip()
    e_clean = expected.strip()
    if a_clean == e_clean:
        return True

    a_toks = a_clean.split()
    e_toks = e_clean.split()
    if len(a_toks) != len(e_toks):
        return False

    for at, et in zip(a_toks, e_toks):
        if at == et:
            continue
        try:
            av = float.fromhex(at) if "0x" in at.lower() else float(at)
            ev = float.fromhex(et) if "0x" in et.lower() else float(et)
            if not math.isclose(av, ev, rel_tol=1e-9, abs_tol=1e-12):
                return False
        except ValueError:
            return False
    return True


def stdout_matches(actual: str, expected: Optional[Union[str, Tuple[str, ...]]]) -> bool:
    if expected is None:
        return True
    if isinstance(expected, tuple):
        return any(tokens_equivalent(actual, opt) for opt in expected)
    return tokens_equivalent(actual, expected)


def run_single_test(
    test_id: int,
    code: str,
    spec: ExpectedSpec,
    compiler_bin: str,
    interp_bin: str,
    use_valgrind: bool,
    timeout: float,
):
    vg_log_path: Optional[str] = None
    vg_issue = ""

    if use_valgrind:
        fd, vg_log_path = tempfile.mkstemp(prefix="ifj26_simple_vg_", suffix=".log")
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
        comp_proc = subprocess.run(
            comp_cmd, input=code, text=True, capture_output=True, timeout=timeout
        )
    except subprocess.TimeoutExpired:
        if vg_log_path and os.path.exists(vg_log_path):
            os.remove(vg_log_path)
        return False, "COMPILE TIMEOUT", "", "", "", -999

    vg_ok = True
    if use_valgrind and vg_log_path:
        try:
            with open(vg_log_path, "r", encoding="utf-8", errors="replace") as f:
                log_text = f.read()
            vg_ok, vg_issue = check_valgrind_log(log_text)
        finally:
            if os.path.exists(vg_log_path):
                os.remove(vg_log_path)

    comp_rc = comp_proc.returncode
    ifjcode = comp_proc.stdout

    if not vg_ok:
        return False, "valgrind error", ifjcode, comp_proc.stderr, vg_issue, comp_rc

    if comp_rc != spec.expected_rc:
        extra = f" [SIGNAL {-comp_rc}]" if comp_rc < 0 else ""
        return (
            False,
            f"compiler rc={comp_rc}{extra} (expected {spec.expected_rc})",
            ifjcode,
            comp_proc.stderr,
            "",
            comp_rc,
        )

    # For expected compile-time error tests (RC != 0), DO NOT run the interpreter.
    if spec.expected_rc != 0:
        return True, f"rc={comp_rc}", f"semERR:{comp_rc}", comp_proc.stderr, "", comp_rc

    # Step 2: Execute generated IFJcode26 on interpreter
    fd, code_path = tempfile.mkstemp(prefix=f"ifj26_t{test_id}_", suffix=".ifjcode")
    os.close(fd)
    try:
        with open(code_path, "w", encoding="utf-8") as f:
            f.write(ifjcode)
        try:
            int_proc = subprocess.run(
                [interp_bin, code_path],
                input=spec.stdin_data,
                text=True,
                capture_output=True,
                timeout=timeout,
            )
        except subprocess.TimeoutExpired:
            return False, f"INTERPRETER TIMEOUT after {timeout}s", ifjcode, "", "", comp_rc
    finally:
        if os.path.exists(code_path):
            os.remove(code_path)

    if int_proc.returncode != 0:
        return (
            False,
            f"interpreter rc={int_proc.returncode} (expected 0)",
            ifjcode,
            int_proc.stderr,
            "",
            comp_rc,
        )

    actual_out = int_proc.stdout
    if not stdout_matches(actual_out, spec.expected_stdout):
        exp_display = (
            " / ".join(repr(x) for x in spec.expected_stdout)
            if isinstance(spec.expected_stdout, tuple)
            else repr(spec.expected_stdout)
        )
        return (
            False,
            f"stdout mismatch: expected={exp_display}, got={actual_out!r}",
            ifjcode,
            int_proc.stderr,
            "",
            comp_rc,
        )

    return True, actual_out.strip(), ifjcode, int_proc.stderr, "", comp_rc


def main() -> int:
    parser = argparse.ArgumentParser(description="Run simple IFJ2026 tests from SCALACODE.")
    parser.add_argument("--bin", default="./compiler", help="Path to compiler binary")
    parser.add_argument(
        "--int",
        dest="interp",
        default="./ic26int-linux-x86_64",
        help="Path to IFJcode26 interpreter",
    )
    parser.add_argument(
        "--source",
        default="",
        help="Path to SCALACODE file (auto-detects testing/SCALACODE by default)",
    )
    parser.add_argument("--valgrind", action="store_true", help="Run compiler under Valgrind")
    parser.add_argument("--verbose", action="store_true", help="Show source/stderr on failure")
    parser.add_argument(
        "--only",
        default="",
        help="Comma-separated test numbers to run (e.g. --only 1,4,7)",
    )
    parser.add_argument(
        "--save-logs",
        default="",
        help="Optional directory to write fresh IFJCODE.txt and RESULTS.txt",
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

    source_path = find_source_file(args.source)
    if source_path is None:
        print(
            f"{RED}Error:{RESET} SCALACODE file not found. "
            "Pass it explicitly with --source <path> (e.g. --source testing/SCALACODE)."
        )
        return 1

    parsed_tests = parse_scalacode(source_path)
    if not parsed_tests:
        print(f"{RED}Error:{RESET} No numbered test blocks (`1:`, `2:`, ...) found in '{source_path}'.")
        return 1

    if args.only:
        wanted = {int(x.strip()) for x in args.only.split(",") if x.strip()}
        parsed_tests = [(tid, code) for tid, code in parsed_tests if tid in wanted]
        if not parsed_tests:
            print(f"{YELLOW}No tests matched --only {args.only}.{RESET}")
            return 1

    mode = "VALGRIND + INTERPRETER" if args.valgrind else "INTERPRETER"
    print(f"{BOLD}{'=' * 70}{RESET}")
    print(f"{BOLD}  IFJ2026 SIMPLE TESTS ({mode}) | Source: {source_path}{RESET}")
    print(f"{BOLD}{'=' * 70}{RESET}")

    passed = 0
    failed = 0
    ifjcode_log: List[str] = ["IFJCODE:\n"]
    results_log: List[str] = ["RESULTS:\n"]

    for tid, code in parsed_tests:
        spec = EXPECTED.get(tid, ExpectedSpec(name=f"Test {tid}", expected_rc=0))
        ok, detail, ifjcode, stderr, vg_issue, comp_rc = run_single_test(
            tid, code, spec, args.bin, args.interp, args.valgrind, args.timeout
        )

        ifjcode_log.append(f"\n{tid}:\n{ifjcode.rstrip()}\n")
        if comp_rc != 0:
            results_log.append(f"{tid}:err:{comp_rc}\n")
        else:
            results_log.append(f"{tid}:{detail}\n")

        if ok:
            passed += 1
            print(f"{GREEN}[PASS]{RESET} Test {tid}: {spec.name} -> {CYAN}{detail}{RESET}")
        else:
            failed += 1
            print(f"{RED}[FAIL]{RESET} Test {tid}: {spec.name} -> {detail}")
            if stderr.strip():
                print(f"       {YELLOW}stderr:{RESET} {stderr.strip()}")
            if vg_issue:
                print(f"       {RED}valgrind:{RESET} {vg_issue}")
            if args.verbose:
                print(f"       {BOLD}--- source (Test {tid}) ---{RESET}")
                for line_no, line in enumerate(code.splitlines(), 1):
                    print(f"       {line_no:2d} | {line}")

    if args.save_logs:
        os.makedirs(args.save_logs, exist_ok=True)
        with open(os.path.join(args.save_logs, "IFJCODE.txt"), "w", encoding="utf-8") as f:
            f.writelines(ifjcode_log)
        with open(os.path.join(args.save_logs, "RESULTS.txt"), "w", encoding="utf-8") as f:
            f.writelines(results_log)
        print(f"\n{CYAN}Saved logs to {args.save_logs}/IFJCODE.txt and {args.save_logs}/RESULTS.txt{RESET}")

    total = passed + failed
    color = GREEN if failed == 0 else RED
    print(f"\n{BOLD}{'=' * 70}{RESET}")
    print(
        f"{BOLD}Summary:{RESET} Total: {total} | "
        f"{GREEN}Passed: {passed}{RESET} | {color}Failed: {failed}{RESET}"
    )
    print(f"{BOLD}{'=' * 70}{RESET}")
    return 0 if failed == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
