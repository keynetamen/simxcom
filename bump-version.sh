#!/usr/bin/env bash

if [ $# != 1 ]; then
    echo "Usage: $0 VERSION-NUMBER"
else
    sed -Ei "/project\(/s/VERSION ([0-9]\.*)+/VERSION $1/" CMakeLists.txt

    echo "Files modified successfully, version bumped to $1"
fi