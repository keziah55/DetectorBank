#!/bin/bash

INCLUDE_DIR=/usr/include
LN_TARGET_DIR="$1"/include
TARGET_DIR="$LN_TARGET_DIR"/rapidxml

rm -rf $LN_TARGET_DIR

if [ -d "$INCLUDE_DIR/rapidxml" ]; then
    mkdir -p "$LN_TARGET_DIR"
    ln -s "$INCLUDE_DIR/rapidxml/" $LN_TARGET_DIR
elif [ -f "$INCLUDE_DIR/rapidxml.h" ] || [ -f "$INCLUDE_DIR/rapidxml.hpp" ]; then
    rm -rf "$TARGET_DIR"
    mkdir -p "$TARGET_DIR"
    find $INCLUDE_DIR/rapidxml*.* | while read rapid_xml_path; do
        basename="$(basename $rapid_xml_path)"
        fname="${basename%.*}.hpp"
        ln -s "$rapid_xml_path" "$TARGET_DIR/$fname"
    done
else
  exit 1
fi
