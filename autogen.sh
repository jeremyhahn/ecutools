#!/bin/sh

aclocal --install -I m4 &&
  autoconf &&
  autoheader &&
  automake --add-missing --copy
