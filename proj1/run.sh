#!/bin/bash

# Simple script to test program
./assembler $1.as $1.mc;
./simulator $1.mc > output;
less output;
