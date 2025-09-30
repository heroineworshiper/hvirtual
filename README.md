Running the binary:

Run ./cinelerra.sh from this directory.

There is a build of ffmpeg for use with the command line output.  This
should be put in your system path if it's desired.




Building the source:

Was last tested on Ubuntu 24 x86_64.

For GPU acceleration, it needs the proprietary Nvidia X11 drivers & CUDA
libraries.

Install the dependencies:

```
apt install build-essential yasm cmake autoconf pkg-config libtool \
libz-dev texinfo libpng-dev libxv-dev libasound2-dev libbz2-dev liblzma-dev \
libxft-dev libxfixes-dev \
libglx-dev libgl-dev libxi-dev libsdl2-dev \
gettext
```

Edit CUDA_DIR in configure to set your CUDA location.



* Run `./bootstrap.sh`.  
* Run `make`. 
* Run `make install` to put it in the bin/ directory. 
* Run `./cinelerra.sh` in the bin/ directory.





