#!/bin/sh

outfile="$1"
outvar=$2
shift 2

shlice=$(cat $@ | sha1sum | head -c 16)

echo "#pragma once" > "$outfile"
echo "#include <stdint.h>" >> "$outfile"
echo "const uint64_t $outvar = 0x$shlice;" >> "$outfile"
