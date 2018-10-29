#!/bin/bash

# Build the file to embed
g++ -O2 hello.cpp -o hello
strip hello

# Build the payload and embed 'hello'
g++ main.cpp -o emb && cat hello >> emb && ./emb
