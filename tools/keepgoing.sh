#!/bin/sh
# Compiler launcher used while porting (see CMakeLists.txt, BISMUTH_KEEP_GOING).
# A compile error is printed by the compiler as usual, but the build carries on with the
# next file. One CI run then lists the errors of every file instead of only the first few.
# The link step still fails afterwards, so a build with errors never produces a mod.
"$@"
status=$?
if [ $status -ne 0 ]; then
    echo "BISMUTH_KEEP_GOING: compile failed (exit $status): $*" | cut -c1-200
fi
exit 0
