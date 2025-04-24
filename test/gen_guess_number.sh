#!/usr/bin/env bash
set -euo pipefail

cat<<EOF
x := $RANDOM
m := 100
x := x % m
n := x - 1

while n <> x do
    n := read
    if n < x then
        print -1
    end
    if n > x then
        print 1
    end
    if n = x then
        print 0
    end
end
EOF
