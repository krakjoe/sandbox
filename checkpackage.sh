#!/bin/sh

for i in src/*.c src/*.h $(find tests -type f)
do
  k=$(basename $i)
  grep -q $k package.xml || echo missing $k
done

