#!/bin/bash

CURRENTPATH=$(pwd)
DESTDIR=$1

gcc -fPIC -o lallocator.o -c "$CURRENTPATH"/src/allocator.c
gcc -shared -o lallocator.so lallocator.o

if [ -z "$DESTDIR" ]; then
    DESTDIR=/usr
    sudo mv lallocator.so "$DESTDIR"/lib/
    sudo cp "$CURRENTPATH"/src/allocator.h "$DESTDIR"/include/
else
    mkdir -p "$DESTDIR"/build/lib
    mkdir -p "$DESTDIR"/build/include
    mv "$CURRENTPATH"/lallocator.so "$DESTDIR"/build/lib/
    cp "$CURRENTPATH"/src/allocator.h "$DESTDIR"/build/include/
fi

rm -f "$CURRENTPATH"/lallocator.o
