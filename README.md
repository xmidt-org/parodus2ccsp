[![Build Status](https://travis-ci.com/xmidt-org/parodus2ccsp.svg?branch=master)](https://travis-ci.com/xmidt-org/parodus2ccsp)
[![codecov.io](http://codecov.io/github/xmidt-org/parodus2ccsp/coverage.svg?branch=master)](http://codecov.io/github/xmidt-org/parodus2ccsp?branch=master)
[![Coverity](https://img.shields.io/coverity/scan/16783.svg)](https://scan.coverity.com/projects/comcast-parodus2ccsp)
[![Apache V2 License](http://img.shields.io/badge/license-Apache%20V2-blue.svg)](https://github.com/xmidt-org/parodus2ccsp/blob/master/LICENSE)
[![Quality Gate Status](https://sonarcloud.io/api/project_badges/measure?project=xmidt-org_parodus2ccsp&metric=alert_status)](https://sonarcloud.io/dashboard?id=xmidt-org_parodus2ccsp)

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
