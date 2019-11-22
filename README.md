# fits2img
Creates conforming Helioviewer JP2 files from FITS files.

This software is derived from the code used at [PROBA-2 Science Operations Center](http://proba2.oma.be) to create JP2 files for the Helioviewer project from SWAP FITS files. It is based on the [OpenJPEG](http://www.openjpeg.org) library.

## Build

```
cmake ~/git/fits2img/ -DCMAKE_INSTALL_PREFIX=~/fits2img
make install
```
