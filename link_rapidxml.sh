#!/bin/bash

INCLUDE_DIR=/usr/include
TARGET_DIR=include/rapidxml

hfiles=($INCLUDE_DIR/rapidxml*.*)

if [ -d "$INCLUDE_DIR/rapidxml" ]; then
    ln -s "$INCLUDE_DIR/rapidxml" $TARGET_DIR
elif [ -e "${hfiles[0]}" ]; then
    rm -rf "$TARGET_DIR"
    mkdir -p "$TARGET_DIR"
    find $INCLUDE_DIR/rapidxml/rapidxml*.* | while read path; do
        basename="$(basename $path)"
        fname="${basename%.*}.hpp"
        ln -s "$path" "$TARGET_DIR/$fname"
    done
else
  exit 1
fi
