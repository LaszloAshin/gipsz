#!/bin/sh

echo "Source is:"
echo " `cat src/*.{c,h} | wc -l` lines (including empty ones)"
echo " `cat src/*.{c,h} | wc -c` bytes"
