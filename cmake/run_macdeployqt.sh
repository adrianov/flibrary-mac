#!/usr/bin/env bash
set -uo pipefail
"$@" 2>&1 | grep -v -E 'Cannot resolve rpath|using QList\(' || true
exit "${PIPESTATUS[0]}"
