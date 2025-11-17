#!/bin/bash
set -e

# Use local directory relative to where script is run (should be build/ directory)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
export RBUS_ROOT="$(pwd)/rbus_deps"
export RBUS_INSTALL_DIR=${RBUS_ROOT}/install
export RBUS_BRANCH=main

mkdir -p $RBUS_ROOT
cd $RBUS_ROOT

# Build and install cJSON first (rbus dependency)
echo "Building cJSON..."
git clone https://github.com/DaveGamble/cJSON.git
cmake -S cJSON -B build/cjson \
  -DCMAKE_INSTALL_PREFIX=${RBUS_INSTALL_DIR}/usr \
  -DENABLE_CJSON_TEST=OFF \
  -DBUILD_SHARED_AND_STATIC_LIBS=ON
make -C build/cjson && make -C build/cjson install

# Build and install msgpack (rbus dependency)
echo "Building msgpack..."
git clone https://github.com/msgpack/msgpack-c.git
cd msgpack-c
git checkout b6803a5fecbe321458faafd6a079dac466614ff9  # 3.1.0
cd ..
cmake -S msgpack-c -B build/msgpack \
  -DCMAKE_INSTALL_PREFIX=${RBUS_INSTALL_DIR}/usr \
  -DMSGPACK_ENABLE_CXX=OFF \
  -DMSGPACK_BUILD_EXAMPLES=OFF \
  -DMSGPACK_BUILD_TESTS=OFF
make -C build/msgpack && make -C build/msgpack install

# Now build rbus
echo "Building rbus..."
git clone https://github.com/rdkcentral/rbus
cmake -S rbus -B build/rbus \
  -DCMAKE_INSTALL_PREFIX=${RBUS_INSTALL_DIR}/usr \
  -DCMAKE_PREFIX_PATH=${RBUS_INSTALL_DIR}/usr \
  -DBUILD_FOR_DESKTOP=ON \
  -DCMAKE_BUILD_TYPE=Debug
make -C build/rbus && make -C build/rbus install

# Set up environment and start rtrouted
export PATH=${RBUS_INSTALL_DIR}/usr/bin:${PATH}
export LD_LIBRARY_PATH=${RBUS_INSTALL_DIR}/usr/lib:${LD_LIBRARY_PATH}

echo "Starting rtrouted..."
nohup rtrouted -f -l DEBUG > /tmp/rtrouted_log.txt 2>&1 &
sleep 2

# Copy libraries to temp location
mkdir -p ${RBUS_INSTALL_DIR}/usr/lib/rbus_temp_lib
cp ${RBUS_INSTALL_DIR}/usr/lib/librbuscore.so* ${RBUS_INSTALL_DIR}/usr/lib/rbus_temp_lib/ 2>/dev/null || true
cp ${RBUS_INSTALL_DIR}/usr/lib/librtMessage.so* ${RBUS_INSTALL_DIR}/usr/lib/rbus_temp_lib/ 2>/dev/null || true
cp ${RBUS_INSTALL_DIR}/usr/lib/libcjson.so* ${RBUS_INSTALL_DIR}/usr/lib/rbus_temp_lib/ 2>/dev/null || true

echo "rtrouted setup complete"
