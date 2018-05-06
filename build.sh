#!/bin/bash
cd "$(dirname "$0")"
gcc -Wall -Wextra -Woverflow -std=c99 -g -Os stringobj.c kvtree.c main.c -o TF2ItemSchemaParser
#clang-3.5 -std=c11 -g -O3 stringobj.c list.c dict.c main.c -o compiler
#don't forget -g for debugging

#valgrind --leak-check=full --show-leak-kinds=all ./SonaParser testparse.c
#gcc -std=c11 -O3 -S -masm=intel symbol.c
