#!/usr/bin/env bash
set -euo pipefail
tmp="$(mktemp)"
trap 'rm -f "$tmp"' EXIT
if ! "$@" >"$tmp" 2>&1; then
	cat "$tmp"
	exit 1
fi
grep -v -E 'Cannot resolve rpath|using QList\(' "$tmp" || true
