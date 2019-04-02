#!/usr/bin/env bash

rm -rf cmake-build-*

mkdir -p cmake-build-release
pushd cmake-build-release
cmake -DCMAKE_BUILD_TYPE=Release ..
popd

mkdir -p cmake-build-release-symbol
pushd cmake-build-release-symbol
cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo ..
popd

mkdir -p cmake-build-debug
pushd cmake-build-debug
cmake -DCMAKE_BUILD_TYPE=Debug ..
popd
