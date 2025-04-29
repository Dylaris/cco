#!/bin/bash

fasm co.asm co

if [ -f co ]; then
    chmod +x co
    ./co
else
    echo "ERROR: 'co' file was not generated."
fi
