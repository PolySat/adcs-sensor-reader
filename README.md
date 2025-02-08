# ADCS Sensor Reader

For PolySat missions without attidute control, we use this process to mock out the ADCS status command.
This interfaces with the rest of the system to provide gyroscope, accelerometer, and magnetometer readings
from satellite side pannels and Intrepid board.

This is a simple process that makes it ideal for learning how to build process that interface with
the PolySat ecosystem.

### Building

It's not possible to build this project without installing the PolySat internal libraries libproc, libpolydrivers, and libsatpkt. We hope to open source these libraries over time, but in the meantime, please get in touch!

After installing the libraries, the process can be built with `make` on linux.

If you have the PolySat flight software built, you can also build this process for the Intrepid board
using `make TARGET=arm`.

### Usage

You can run this process by running the executable, `./adcs-sensor-reader`.

After starting the process, you can call it with the adcs util program.
Give `./adcs-sesor-reader-util -S` and `./adcs-sensor-reader-util -T` a try!
The command `./adcs-sesor-reader-util -dl` will print a datalogger sensor config file.

If the process is running on an different computer, like an Intrepid board, you can supply the process IP with the `-h` flag.

For example, `./adcs-util -S -h 10.42.42.42` calls a payload process running at `10.42.42.42`.
