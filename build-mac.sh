#!/usr/bin/env bash
set -euo pipefail

root_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
build_dir="${root_dir}/build"
qt_dir="${QT6_DIR:-/opt/homebrew/opt/qt/lib/cmake/Qt6}"
seven_zip_dir="${SEVENZIP_BIN_DIR:-/opt/homebrew/lib}"

export CC=/usr/bin/clang
export CXX=/usr/bin/clang++

mkdir -p "${build_dir}"
cd "${build_dir}"

cmake .. \
	-DCMAKE_BUILD_TYPE=Release \
	-DQt6_DIR="${qt_dir}" \
	-D7zip_BIN_DIR="${seven_zip_dir}" \
	-G Ninja

cmake --build . --parallel

app_path="${build_dir}/bin/FLibrary.app"
if [[ -d "${app_path}" ]]; then
	open -R "${app_path}"
else
	open "${build_dir}/bin"
fi
