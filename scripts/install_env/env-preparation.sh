#!/bin/bash

source scripts/install_env/PKG_EXPORT.sh

echo $PKG_UPDATE

$PKG_UPDATE

$PKG_INSTALL gcc make git clang cmake clang-tools