#!/bin/bash

# Build the file to embed
gcc -O2 payload.c -o payload
strip payload

# Build the packer and embed the payload
gcc packer.c -o emb && cat payload >> emb && ./emb
