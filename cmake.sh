#!/usr/bin/env bash

rm -rf cmake-release-symbol cmake-release cmake-debug

mkdir cmake-release-symbol
mkdir cmake-release
mkdir cmake-debug

cd cmake-release-symbol
cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo ..

cd ../cmake-release
cmake -DCMAKE_BUILD_TYPE=Release ..

cd ../cmake-debug
cmake -DCMAKE_BUILD_TYPE=Debug ..
