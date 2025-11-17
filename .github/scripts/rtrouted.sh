#!/bin/bash
set -e

export RBUS_ROOT=${HOME}/rbus
export RBUS_INSTALL_DIR=${RBUS_ROOT}/install
export RBUS_BRANCH=main

mkdir -p $RBUS_INSTALL_DIR
cd $RBUS_ROOT

# Build and install cJSON first (rbus dependency)
echo "Building cJSON..."
git clone https://github.com/DaveGamble/cJSON.git
cmake -HcJSON -Bbuild/cjson \
  -DCMAKE_INSTALL_PREFIX=${RBUS_INSTALL_DIR}/usr \
  -DENABLE_CJSON_TEST=OFF \
  -DBUILD_SHARED_AND_STATIC_LIBS=ON
make -C build/cjson && make -C build/cjson install

# Now build rbus
echo "Building rbus..."
git clone https://github.com/rdkcentral/rbus
cmake -Hrbus -Bbuild/rbus \
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
