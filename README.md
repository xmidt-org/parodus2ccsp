[![Build Status](https://travis-ci.org/Comcast/parodus2ccsp.svg?branch=master)](https://travis-ci.org/Comcast/parodus2ccsp)
[![codecov.io](http://codecov.io/github/Comcast/parodus2ccsp/coverage.svg?branch=master)](http://codecov.io/github/Comcast/parodus2ccsp?branch=master)
[![Coverity](https://img.shields.io/coverity/scan/xxx.svg)](https://scan.coverity.com/projects/comcast-parodusxxx)
[![Apache V2 License](http://img.shields.io/badge/license-Apache%20V2-blue.svg)](https://github.com/Comcast/parodus2ccsp/blob/master/LICENSE)

# parodus2ccsp

Webpa client to communicate with parodus in RDK environment.

# Building and Testing Instructions

```
Pre-Requisite:
--------------
- cmake >= 2.8.7
- openssl >= 1.0.2i and < 1.1.0
- expat

Configuration & Build:
----------------------
mkdir build
cd build
cmake .. -D<option>
sudo make

Test:
-----
By default tests will be disabled. Enable tests by configuring BUILD_TESTING to true and re-build.

cmake .. -DBUILD_TESTING:BOOL=true
sudo make
make test
```
