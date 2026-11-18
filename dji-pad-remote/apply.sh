#!/usr/bin/env bash
# Builds the pad-remote project folder: fetches DJI's official demo at the
# tested commit, applies dji-demo.patch and copies in the pad/ sources.
#
#   ./apply.sh                 -> creates ./dji-pad-remote-build
#   ./apply.sh ~/esp/my-remote -> creates it somewhere else
set -euo pipefail

HERE="$(cd "$(dirname "$0")" && pwd)"
DEST="${1:-dji-pad-remote-build}"
COMMIT="$(cat "$HERE/.dji-demo-commit")"

if [ ! -d "$DEST/.git" ]; then
    git clone https://github.com/dji-sdk/Osmo-GPS-Controller-Demo.git "$DEST"
fi
cd "$DEST"

if git apply --reverse --check "$HERE/dji-demo.patch" 2>/dev/null; then
    echo "Patch already applied."
else
    git checkout -q "$COMMIT"
    git apply "$HERE/dji-demo.patch"
    echo "Patch applied to DJI demo @ ${COMMIT:0:7}."
fi

rm -rf pad
cp -r "$HERE/pad" pad
echo "Ready: $(pwd)"
echo "Next:  cd \"$(pwd)\" && idf.py build flash monitor"
