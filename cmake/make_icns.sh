#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 2 ]]; then
	echo "usage: make_icns.sh <input.ico> <output.icns>" >&2
	exit 2
fi

ico="$1"
out="$2"
work="$(mktemp -d "${TMPDIR:-/tmp}/flibrary-icon.XXXXXX")"
iconset="${work}/AppIcon.iconset"
base="${work}/base.png"

cleanup() {
	rm -rf "${work}"
}
trap cleanup EXIT

mkdir "${iconset}"
sips -s format png "${ico}" --out "${base}" >/dev/null

make_icon() {
	sips -z "$1" "$1" "${base}" --out "${iconset}/$2" >/dev/null
}

make_icon 16 icon_16x16.png
make_icon 32 icon_16x16@2x.png
make_icon 32 icon_32x32.png
make_icon 64 icon_32x32@2x.png
make_icon 128 icon_128x128.png
make_icon 256 icon_128x128@2x.png
make_icon 256 icon_256x256.png
make_icon 512 icon_256x256@2x.png
make_icon 512 icon_512x512.png
make_icon 1024 icon_512x512@2x.png

iconutil -c icns "${iconset}" -o "${out}"
