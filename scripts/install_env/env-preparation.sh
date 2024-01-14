#!/bin/bash

source scripts/install_env/PKG_EXPORT.sh

echo $PKG_UPDATE

sudo $PKG_UPDATE

sudo $PKG_INSTALL gcc make git clang cmake clang-tools libcunit1 libcunit1-dev