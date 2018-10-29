#!/bin/bash

# Build the file to embed
gcc -O2 hello.c -o hello
strip hello

# Build the payload and embed 'hello'
gcc main.c -o emb && cat hello >> emb && ./emb
