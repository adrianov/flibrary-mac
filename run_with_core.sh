#!/bin/bash
# Run FLibrary with core dump support for backtrace on crash
# Usage: ./run_with_core.sh
# After crash, backtrace with: gdb build/bin/FLibrary /tmp/core.FLibrary.<pid>

ulimit -c unlimited
export LD_LIBRARY_PATH=/home/crexus/Qt/6.11.0/gcc_64/lib:/home/crexus/flibrary-mac/build/lib
export PATH=/home/crexus/Qt/6.11.0/gcc_64/bin:$PATH

echo "Core dumps enabled. Core files will be in /var/crash/ (apport) or check:"
echo "  ls -la /var/crash/*FLibrary*"
echo ""
echo "To extract backtrace from apport crash file:"
echo "  apport-unpack /var/crash/_path_to_FLibrary.crash /tmp/flibrary_crash"
echo "  gdb build/bin/FLibrary /tmp/flibrary_crash/CoreDump"
echo ""
echo "Starting FLibrary..."
exec /home/crexus/flibrary-mac/build/bin/FLibrary "$@"
