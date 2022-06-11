# opv-cxx-demod

Opulent Voice Modulator and Demodulator in C++ (GPL)

# THE REST OF THIS README HAS YET TO BE UPDATED FROM M17 to OPV

# m17-cxx-demod
M17 Modulator & Demodulator in C++ (GPL)

## m17-cxx-demod
This program reads a 48k samples per second 16-bit, little-endian, single
channel, M17 4-FSK baseband stream from STDIN and writes a demodulated/decoded
8k SPS 16-bit, single channel audio stream to STDOUT.

Some diagnostic information is written to STDERR while the demodulator is
running.

## m17-cxx-mod
This program reads in an 8k sample per second, 16-bit, 1 channel raw audio
stream from STDIN and writes out an M17 4-FSK baseband stream at 48k SPS,
16-bit, 1 channel to STDOUT.

## Build

### Prerequisites

This code requires the codec2-devel, boost-devel and gtest-devel packages be installed.

It also requires a modern C++17 compiler (GCC 8 minimum).

### Build Steps

    mkdir build
    cd build
    cmake ..
    make
    make test
    sudo make install

## Build Steps for local building under Anaconda for Windows

### Prequisites
- Microsoft Visual Studio 2019
- Miniconda (or Anaconda) x64 for Windows

### From a clean Conda environment

    conda config --add channels conda-forge
    conda create -n M17 vs2019_win-64 cmake ninja pkg-config boost-cpp gtest gmock gtest libcodec2
    conda activate M17

### And then from the top level of the m17-cxx-demod repo, execute win_build.bat

## Running

This program was designed to be used with RTL-SDR, specifically rtl-fm.

    rtl_fm -E offset -f 144.91M -s 48k | m17-demod -l | play -b 16 -r 8000 -c1 -t s16 -

You should run this in a terminal window that is 132 characters wide. It
will output diagnostic information on a single line in the window.

## Testing the Modulator

    sox ~/m17-demodulator/brain.wav -t raw - |  ./m17-mod -S WX9O | ./m17-demod -l -d | play -q -b 16 -r 8000 -c1 -t s16 -

The input audio stream must be 1 channel, 16-bit, 8000 samples per second.

Use `-S <callsign>` to set your source (callsign).

Use `-b` to output a bitstream rather than baseband.

Use `-h` to see the full help.  Many of the options do not yet work.

The output of the modulator is 48ksps, 16-bit, 1 channel raw audio.

To output a bitstream file:

    sox ~/m17-demodulator/brain.wav -t raw - | ./m17-mod -S WX9O -b > m17.bin

This bitstream file can be fed into [m17-gnuradio](https://github.com/mobilinkd/m17-gnuradio) to
transmit M17 using a PlutoSDR (or any SDR with an appropriate GNU Radio sink), or loaded into
a vector signal generator such as an ESG-D Series signal generator.

I have recently been testing the modulator and demodulator with long-form programs. One
that works well is the [AR Newsline](https://www.arnewsline.org/) program.  This is available
as a downloadable MP3 file.  To convert that to a bitstream, run the following:

    ffmpeg -i ~/Downloads/Report2287.mp3 -ar 8000 -ac 1 -f s16le -acodec pcm_s16le - | ./apps/m17-mod -b -S NS9RC > Report2287.bin

This can then be used with the the `M17_Impaired.grc` GNU Radio flow graph.

### Command Line Options

There are two command line options for the demodulator that determine the diagnostic output.

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

## Demodulator Diagnostic

The demodulator diagnostics are calibrated for the following command:

    rtl_fm -F 9 -f 144.91M -s 18k | sox -t s16 -r 18k -c1 - -t raw - gain 9 rate -v -s 48k | ./apps/m17-demod -d -l | play -b 16 -r 8000 -c1 -t s16 -

Specifically, the initial rate (18k samples per second) and the conversion rate and gain (gain of 9,
output rate 48k samples per second) are important for deviation and frequency offset.

The demodulator produces diagnost output which looks like:

    SRC: BROADCAST, DEST: MBLKDTNC3, TYPE: 0002, NONCE: 0000000000000000000000000000, CRC: bb9b
    dcd: 1, evm:    13.27%, deviation:   0.9857, freq offset:  0.03534, locked:   true, clock:        1, sample: 0, 0, 0, cost: 9

The first line shows the received link information.  The second line contains the following diagnostics.

 - **DCD** -- data carrier detect -- uses a DFT to detect baseband energy.  Very good at detecting whether data may be there.
 - **EVM** -- error vector magnitude -- measure the Std Dev of the offset from ideal for each symbol.
 - **Deviation** -- normalized to 1.0 to mean 2400Hz deviation for the input from the following command:
    `rtl_fm -F 9 -f 144.91M -s 18k | sox -t s16 -r 18k -c1 - -t raw - gain 9 rate -v -s 48k` -- the rates and gain are the important part.
 - **Frequency Offset** -- the estimated frequency offset from 0 based on the DC level of each symbol.  The magnutude has
    not been measure but should be around 1.0 == 2kHz using the same normalized input as for deviation.  Anything > 0.1
    or less than -0.1 requires review/calibration of the TX and RX frequencies.
 - **Locked** -- sync word detected. 
 - **Clock** -- estimated difference between TX and RX clock.  Will not work if > 500ppm.  Normalized to 1.0 -- meaning the clocks are equal.
 - **Sample** -- estimated sample point based on 2 clock recovery methods.  Should agree to within 1 always.  There are
    10 samples per symbol.  Should never jump by more than 1 per frame.  The third number is the "winner" based on
    certain heuristics.
 - **Cost** -- the normalized Viterbi cost estimate for decoding the frame.  < 5 great, < 15 good, < 30 OK, < 50 bad, > 80 you're hosed.

## BER Testing

When transmitting a BER test, the diagnostics line will show additional information.

    dcd: 1, evm:    2.626%, deviation:   0.9318, freq offset:   -0.154, locked:   true, clock:        1, sample: 2, 2, 2, cost: 2, BER: 0.000000 (53190 bits)

The BER rate and number of bits received are also displayed.

## Thanks

Thanks to the M17 team to for the great work on the spec.
