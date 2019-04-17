#!/bin/bash
cd "$(dirname "$0")"
gcc -Wall -Wextra -std=c99 -s -O2 stringobj.c kvtree.c main.c -o TF2ItemSchemaParser
