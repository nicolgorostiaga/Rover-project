#!/bin/sh

rm -r latex
doxygen
cd latex
make
cp refman.pdf ../TX2Nodes.pdf
cd ..
rm -r latex
