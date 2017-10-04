#!/bin/sh

swig -small -noproxy -O -I.. -o php/send2LMAT.c    -outdir php    -php7    send2LMAT.i
swig -small -noproxy -O -I.. -o perl/send2LMAT.c   -outdir perl   -perl   send2LMAT.i
swig -small -noproxy -O -I.. -o python/send2LMAT.c -outdir python -python -builtin send2LMAT.i
