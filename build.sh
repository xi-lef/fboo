#!/usr/bin/env sh
set -eu

build_dir='.build'

if [ ! -d $build_dir ] || [ "$#" -ne 0 ]; then
    rm -rf $build_dir
    cmake -S . -B $build_dir
fi

cmake --build $build_dir -j "$(nproc)"

ln -fs $build_dir/apps/fboo
