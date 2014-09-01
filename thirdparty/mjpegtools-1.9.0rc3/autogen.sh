#!/bin/sh
# Run this to generate all the initial makefiles, etc.
export ACLOCAL="aclocal -I missing_M4"
autoreconf -f -i
