#!/bin/sh

PREFIX=/usr/local/cross-tools
TARGET=i386-mingw32msvc
export PATH="$PREFIX/bin:$PREFIX/$TARGET/bin:$PATH"
export CROSS_BUILD=1
export DEST="./win32-builds"
exec make -fMakefile.mingw $*
