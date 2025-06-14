#!/usr/bin/env bash
set -euf -o pipefail
ROOT_DIR="$(dirname "$(readlink -f "$0")")"
source "$ROOT_DIR/scripts/init.sh"

GenericPackage "$BUILD_LINUX_DIR_NAME" "$PACKAGE_DIR_NAME"
