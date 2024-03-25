# Mongoose Embedded Web Server Client for Audio Visualization

Source Code: https://github.com/cesanta/mongoose
Main Website: https://mongoose.ws/
Mongoose Documentation: https://mongoose.ws/documentation/
Full Mongoose Build Tools Documentation: https://mongoose.ws/documentation/tutorials/tools/

## Required Build Tools
- Git, https://git-scm.com/ - for downloading source code and version control
- GCC / MinGW compiler - for compiling desktop / server programs
- GNU make, http://www.gnu.org/software/make/ - for build automation

If you are going to build for embedded systems, extra tools are required:

- For ARM embedded systems ARM GCC, https://launchpad.net/gcc-arm-embedded

## Ubuntu Linux Setup
Execute the following:

`sudo apt -y update`
`sudo apt -y install build-essential make gcc-arm-none-eabi stlink-tools git cmake gcc-riscv64-unknown-elf`

## Build Mongoose

Within `prj-audiojak/mongoose_server` call `make` to build and start up a webserver accessible via `http://localhost:8000`