# m17-cxx-demod
M17 Demodulator in C++ (GPL)

This program reads a 48K SPS 16-bit, little-endian, single channel, M17  4-FSK
baseband input stream from STDIN and writes a demodulated/decoded 8K SPS
16-bit, single channel audio stream to STDOUT.

Some diagnostic information is written to STDERR while the demodulator is
running.

## Build

### Prerequisites

This code requires the codec2-devel and gtest-devel packages be installed.

It also requires a modern C++17 compiler (GCC 8 minimum).

### Build Steps

    mkdir build
    cd build
    cmake ..
    make
    sudo make install

## Running

This program was designed to be used with RTL-SDR, specifically rtl-fm.

    rtl_fm -E offset -f 144.91M -s 48k | m17-demod -l | play -b 16 -r 8000 -c1 -t s16 -

You should run this in a terminal window that is 132 characters wide. It
will output diagnostic information on a single line in the window.

### Command Line Options

There are two command line options that determine the diagnostic output.

    -d causes demodulator diagnosts to be streamed to the terminal on STDERR.
    -l causes the link setup frame information (either from the first frame or LICH) to be displayed.

Note that the oscillators on the PlutoSDR and on most RTL-SDR dongles are
rather inaccurate.  You will need to have both tuned to the same frequency,
correcting for clock inaccuracies on one or both devices.

Also note that I need to use `-E offset` to decode the data well, even though
I see this in the `rtl_fm` output:

    WARNING: Failed to set offset tuning.

This was tested using the [m17-gnuradio](https://github.com/mobilinkd/m17-gnuradio)
GNU Radio block feeding an Analog Devices 
[ADALM Pluto SDR](https://www.analog.com/en/design-center/evaluation-hardware-and-software/evaluation-boards-kits/adalm-pluto.html),
modulating the m17.bin file from the
[m17-demodulator](https://github.com/mobilinkd/m17-demodulator) repo.

## Notes

As of now, this is using the older versions of the sync word and LICH
encoding.  It is out of date with the current M17 spec.

## Thanks

Thanks to the M17 team to for the great work on the spec.
