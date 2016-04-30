#!/bin/sh

aclocal --install -I m4 &&
  autoreconf -fi &&
  autoheader &&
  automake --add-missing --copy
