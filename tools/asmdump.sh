#!/usr/bin/env bash
# asmdump.sh - compile a C file to assembly and strip everything that is not
# an instruction, so what is left is the code the CPU actually runs.
#
#   tools/asmdump.sh 01-a-plus-b/add.c
#   tools/asmdump.sh -O 0 -a x86_64 01-a-plus-b/add.c
#   tools/asmdump.sh -f add_called 01-a-plus-b/add.c
#   tools/asmdump.sh -d 02-reading-arm64/basics.c    # -O0 beside -O2
#   tools/asmdump.sh -r 03-control-flow/control.c    # keep directives and data
#
set -euo pipefail

TOOLS=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)

ARCH=arm64
OPT=2
FUNC=""
DIFF=0
RAW=0

usage() {
    sed -n '2,10p' "$0" | sed 's/^# \{0,1\}//'
    exit "${1:-0}"
}

while getopts ":a:O:f:drh" o; do
    case "$o" in
        a) ARCH=$OPTARG ;;
        O) OPT=$OPTARG ;;
        f) FUNC=$OPTARG ;;
        d) DIFF=1 ;;
        r) RAW=1 ;;
        h) usage 0 ;;
        *) usage 1 ;;
    esac
done
shift $((OPTIND - 1))

[ $# -eq 1 ] || usage 1
SRC=$1
[ -f "$SRC" ] || { echo "asmdump: no such file: $SRC" >&2; exit 1; }

# Compile to assembly. Compiler errors are deliberately NOT suppressed: an
# empty dump caused by a failed build looks identical to a dump of a function
# with no code, and the silent version wasted an afternoon once already.
compile() {
    clang -arch "$ARCH" "-O$1" -S \
          -fno-asynchronous-unwind-tables \
          -fno-unwind-tables \
          -fomit-frame-pointer \
          -I "$TOOLS" \
          -x c "$SRC" -o -
}

# One awk pass rather than a grep chain, because grep exits 1 when it filters
# everything out and `set -o pipefail` would turn that into a crash.
#
# Every assembler directive begins with a dot and describes layout or metadata
# rather than work, so dropping /^\s*\./ removes them all. Labels and
# instructions survive. -r keeps everything, which matters when the interesting
# part IS the data, such as a switch statement's jump table.
clean() {
    awk -v raw="$RAW" -v want="$FUNC" '
        function is_global_label(s) { return s ~ /^_[A-Za-z_][A-Za-z0-9_$.]*:$/ }

        {
            # bare drives every decision. clang appends trailing comments to
            # label lines ("_add_called:   ; @add_called"), so a label has to
            # have its comment removed before it can be compared by name.
            bare = $0
            sub(/[[:space:]]*(;|##|\/\/).*$/, "", bare)
            sub(/[[:space:]]+$/, "", bare)

            # -f mode ends at clang own "-- End function" marker. That is the
            # reliable boundary: stopping at the next label would trail the
            # constant pool (LCPI...) x86 emits after the body, and stopping at
            # `ret` would truncate any function that has two of them.
            #
            # This has to run before the blank-line skip below, because the
            # marker line is nothing but a comment and `bare` is empty by now.
            if (want != "" && inside && $0 ~ /-- End function/) { inside = 0; next }

            line = raw ? $0 : bare
            if (line == "") next
            if (!raw && bare ~ /^[[:space:]]*\./) next

            # x86 parks floating-point constants after the function as LCPI
            # labels whose bodies are .long/.quad directives. Those directives
            # are already stripped above, so keeping the labels would print
            # headings with nothing under them. -r shows the data.
            if (!raw && bare ~ /^LCPI[0-9]/) next

            if (want == "") { print line; next }

            if (is_global_label(bare)) {
                inside = (bare == "_" want ":")
                if (inside) { print line; found = 1 }
                next
            }
            if (inside) print line
        }
        END {
            if (want != "" && !found) {
                printf("asmdump: no function named %s in this file\n", want) > "/dev/stderr"
                exit 3
            }
        }
    '
}

emit() { compile "$1" | clean; }

if [ "$DIFF" -eq 1 ]; then
    echo "### $SRC  [$ARCH]  -O0 (left)  vs  -O2 (right)${FUNC:+  function: $FUNC}"
    echo
    diff -y --width=110 <(emit 0) <(emit 2) || true
else
    echo "### $SRC  [$ARCH -O$OPT]${FUNC:+  function: $FUNC}"
    echo
    emit "$OPT"
fi
