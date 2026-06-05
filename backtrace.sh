#!/bin/bash
# Extract backtrace from apport crash or core file
# Usage: ./backtrace.sh [core_file]

CRASH_FILE=$(ls -t /var/crash/*FLibrary* 2>/dev/null | head -1)
BINARY=/home/crexus/flibrary-mac/build/bin/FLibrary

if [ -n "$1" ]; then
    CORE="$1"
elif [ -n "$CRASH_FILE" ]; then
    echo "Found apport crash: $CRASH_FILE"
    echo "Unpacking..."
    TMPDIR=$(mktemp -d)
    apport-unpack "$CRASH_FILE" "$TMPDIR"
    CORE="$TMPDIR/CoreDump"
    echo "Core dump at: $CORE"
else
    echo "No crash files found in /var/crash/"
    exit 1
fi

echo "=== Backtrace ==="
gdb -batch \
    -ex "set confirm off" \
    -ex "thread apply all bt full" \
    -ex "info threads" \
    -ex "quit" \
    "$BINARY" "$CORE" 2>&1
