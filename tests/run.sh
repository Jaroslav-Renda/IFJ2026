#!/bin/sh
#/**
#* @file    run.sh
#* @author  Vojtech Urich (xurich00)
#* @brief   Builds the codegen fixture driver and runs every fixture through
#*          codegen and the interpreter
#* @date    2026-11-18
#*
#* @details The root-directory Makefile does not compile tests/, so this script
#*          compiles just the sources the fixtures need. For every fixture the
#*          generated IFJcode26 is captured into build/<name>.ic26 and handed
#*          to the bundled interpreter (ic26int-linux-x86_64); when
#*          tests/<name>.in / tests/<name>.out exist they are used as stdin
#*          and as the expected output. codegen is work in progress, so every
#*          step runs under `timeout` and a hang is reported, not waited on.
#*
#*          Usage: tests/run.sh [fixture ...]   (default: all fixtures)
#*
#*          AI-assisted source/default-list/result-summary additions by Kimi.
#*/
set -u

here=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
root=$(dirname -- "$here")
outDir="$here/build"
CC=${CC:-gcc}
CFLAGS="-pedantic -Wall -Wextra -g -O0 -std=c23"
CODEGEN_TIMEOUT=${CODEGEN_TIMEOUT:-15}
INTERP_TIMEOUT=${INTERP_TIMEOUT:-15}

mkdir -p "$outDir"

$CC $CFLAGS -o "$outDir/codegen_fixture_test" \
	"$root/ast.c" \
	"$root/codegen.c" \
	"$root/dynamicString.c" \
	"$root/error.c" \
	"$root/semantAnalys.c" \
	"$root/symtable.c" \
	"$root/symtableStack.c" \
	"$here/fixture_common.c" \
	"$here/factorial_fixture.c" \
	"$here/strings_fixture.c" \
	"$here/arith_fixture.c" \
	"$here/control_fixture.c" \
	"$here/numeric_fixture.c" \
	"$here/functions_fixture.c" \
	"$here/literals_fixture.c" \
	"$here/scopes_fixture.c" \
	"$here/looplocals_fixture.c" \
	"$here/widening_fixture.c" \
	"$here/readers_fixture.c" \
	"$here/substring_fixture.c" \
	"$here/ordbounds_fixture.c" \
	"$here/codegen_fixture_test.c" || exit 1

int="$here/ic26int-linux-x86_64"
# fresh checkouts may drop the executable bit on the bundled binary
[ -x "$int" ] || chmod +x "$int"

# maps a driver failure/timeout to a short phase tag
phaseTag() {
	case "$1" in
	124) echo "TIMEOUT" ;;
	0) echo "ok" ;;
	*) echo "FAIL($1)" ;;
	esac
}

runOne() {
	name=$1
	codeFile="$outDir/$name.ic26"
	gotFile="$outDir/$name.got"
	status=0

	printf '=== fixture: %s ===\n' "$name"

	# --- phase 1: semant + codegen ---------------------------------------
	# stdbuf keeps partial output flowing even when codegen hangs.
	codegenStatus=0
	timeout "$CODEGEN_TIMEOUT" stdbuf -o0 "$outDir/codegen_fixture_test" "$name" \
		>"$codeFile" 2>"$outDir/$name.codegen.log" || codegenStatus=$?
	tag=$(phaseTag "$codegenStatus")
	printf 'codegen: %-8s -> %s\n' "$tag" "$codeFile"
	[ "$codegenStatus" -eq 0 ] || status=1
	if [ "$codegenStatus" -eq 124 ]; then
		printf 'codegen hung; skipping the interpreter for this fixture\n'
		return 1
	fi

	# --- phase 2: run the interpreter ------------------------------------
	stdinFile="$here/$name.in"
	expectedFile="$here/$name.out"
	runStatus=0
	if [ -f "$stdinFile" ]; then
		timeout "$INTERP_TIMEOUT" "$int" "$codeFile" <"$stdinFile" \
			>"$gotFile" 2>&1 || runStatus=$?
	else
		timeout "$INTERP_TIMEOUT" "$int" "$codeFile" \
			>"$gotFile" 2>&1 || runStatus=$?
	fi
	tag=$(phaseTag "$runStatus")
	printf 'interp : %-8s -> %s\n' "$tag" "$gotFile"
	[ "$runStatus" -eq 0 ] || status=1
	if [ "$runStatus" -ne 0 ]; then
		sed 's/^/  | /' "$gotFile" | head -10
		return 1
	fi

	# --- phase 3: compare against the expected output --------------------
	if [ -f "$expectedFile" ]; then
		if diff -u "$expectedFile" "$gotFile" >"$outDir/$name.diff" 2>&1; then
			printf 'diff   : ok\n'
		else
			printf 'diff   : FAIL (see %s)\n' "$outDir/$name.diff"
			head -20 "$outDir/$name.diff"
			status=1
		fi
	else
		printf 'diff   : no %s, skipped\n' "$expectedFile"
	fi

	return "$status"
}

overall=0
if [ $# -eq 0 ]; then
	# Run every registered fixture. AI-added fixture names by Kimi; prompt:
	# "make new and more tests like how they are in the tests folder, for now only for the codegen".
	set -- factorial strings arith control numeric functions literals scopes looplocals widening \
		readers substring ordbounds
fi

# AI-assisted result summary by Kimi; prompts: "can you also make a result at
# the end of run.sh showing passed: and failed:"; "but that doesnt show the
# numbers anywhere"; "add also a separator for the summary from the tests".
passed=""
failed=""
passedCount=0
failedCount=0
for fixture in "$@"; do
	if runOne "$fixture"; then
		passed="$passed $fixture"
		passedCount=$((passedCount + 1))
	else
		failed="$failed $fixture"
		failedCount=$((failedCount + 1))
		overall=1
	fi
done

printf '\n========================================\n'
printf 'passed: %d%s\n' "$passedCount" "$passed"
printf 'failed: %d%s\n' "$failedCount" "$failed"
printf '=== summary: %s ===\n' \
	"$([ "$overall" -eq 0 ] && echo ALL OK || echo FAILURES PRESENT)"
exit "$overall"
