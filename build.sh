#!/usr/bin/env sh
set -euo pipefail

build_dir='.build'

if [ ! -d $build_dir ] || [ "$#" -ne 0 ]; then
    rm -rf $build_dir
    cmake -S . -B $build_dir
fi

if which bear 2> /dev/null 1>&2; then
    prefix='bear --'
fi

${prefix:-} cmake --build $build_dir -j $(nproc)

ln -fs $build_dir/apps/fboo
