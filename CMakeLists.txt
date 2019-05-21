#   Copyright 2010-2016 Comcast Cable Communications Management, LLC
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.

cmake_minimum_required(VERSION 2.8.7)

project(parodus2ccsp)
include(ExternalProject)

add_definitions(-std=c99)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_GNU_SOURCE ")
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_GNU_SOURCE -Werror -Wall")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_GNU_SOURCE -Werror -Wall")

set(INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}/_install)
set(PREFIX_DIR ${CMAKE_CURRENT_BINARY_DIR}/_prefix)
set(INCLUDE_DIR ${INSTALL_DIR}/include)
set(LIBRARY_DIR ${INSTALL_DIR}/lib)
set(LIBRARY_DIR64 ${INSTALL_DIR}/lib64)
set(BIN_DIR ${INSTALL_DIR}/bin)
set(SHARE_DIR ${INSTALL_DIR}/share)
set(DOC_DIR ${INSTALL_DIR}/doc)
set(COMMON_LIBRARY_DIR ${INSTALL_DIR}/lib/${CMAKE_LIBRARY_ARCHITECTURE})

option(BUILD_RASPBERRYPI "BUILD_RASPBERRYPI" OFF)
option(BUILD_EMULATOR "BUILD_EMULATOR" OFF)

include_directories(${INCLUDE_DIR}
                    ${INCLUDE_DIR}/cjson
                    ${INCLUDE_DIR}/msgpack
                    ${INCLUDE_DIR}/trower-base64
                    ${INCLUDE_DIR}/wrp-c
                    ${INCLUDE_DIR}/wdmp-c
                    ${INCLUDE_DIR}/libparodus
                    ${INCLUDE_DIR}/cimplog
                    ${LIBRARY_DIR}/dbus-1.0/include
                    ${INCLUDE_DIR}/dbus-1.0
                    ${INCLUDE_DIR}/ccsp
                    ${INCLUDE_DIR}/ccsp/linux
		   )

include_directories(
                     source/include
                     source/broadband/include
		   )
if (FEATURE_SUPPORT_WEBCONFIG)
include_directories(
		     ${INCLUDE_DIR}/curl
                     source/WebConfig
                    )
endif (FEATURE_SUPPORT_WEBCONFIG)
include_directories (SYSTEM /usr/include)

# pthread external dependency
#-------------------------------------------------------------------------------
find_package (Threads REQUIRED)

# Get git commit hash
#-------------------------------------------------------------------------------
execute_process(
  COMMAND git describe --tags --always
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE GIT_COMMIT_TAG
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

add_definitions("-DWEBPA_GIT_VERSION=\"${GIT_COMMIT_TAG}\"")

if (BUILD_RASPBERRYPI)
add_definitions(-DPLATFORM_RASPBERRYPI)
endif (BUILD_RASPBERRYPI)

if (BUILD_EMULATOR)
add_definitions(-DRDKB_EMU)
endif (BUILD_EMULATOR)

if (FEATURE_SUPPORT_WEBCONFIG)
add_definitions(-DFEATURE_SUPPORT_WEBCONFIG)
endif (FEATURE_SUPPORT_WEBCONFIG)

if (NOT BUILD_YOCTO)
# cJSON external dependency
#-------------------------------------------------------------------------------
ExternalProject_Add(cJSON
    PREFIX ${PREFIX_DIR}/cJSON
    GIT_REPOSITORY https://github.com/DaveGamble/cJSON.git
    GIT_TAG "aafb64a1c549b7b927e339df6d35b1d5059dc235"
    CMAKE_ARGS += -DCMAKE_INSTALL_PREFIX=${INSTALL_DIR}
)
add_library(libcJSON STATIC IMPORTED)
add_dependencies(libcJSON cJSON)


# cimplog external dependency
#-------------------------------------------------------------------------------
ExternalProject_Add(cimplog
    PREFIX ${CMAKE_CURRENT_BINARY_DIR}/_prefix/cimplog
    GIT_REPOSITORY https://github.com/Comcast/cimplog.git
    GIT_TAG "8a5fb3c2f182241d17f5342bea5b7688c28cd1fd"
    CMAKE_ARGS += -DCMAKE_INSTALL_PREFIX=${INSTALL_DIR}
)
add_library(libcimplog STATIC SHARED IMPORTED)
add_dependencies(libcimplog cimplog)

# base64 external dependency
#-------------------------------------------------------------------------------
ExternalProject_Add(trower-base64
    PREFIX ${CMAKE_CURRENT_BINARY_DIR}/_prefix/trower-base64
    GIT_REPOSITORY https://github.com/Comcast/trower-base64.git
    GIT_TAG "fbb9440ae2bc1118866baefcea7ff814f16613dd"
    CMAKE_ARGS += -DCMAKE_INSTALL_PREFIX=${INSTALL_DIR} -DBUILD_TESTING=OFF
)
add_library(libtrower-base64 STATIC SHARED IMPORTED)
add_dependencies(libtrower-base64 trower-base64)

# wdmp-c external dependency
#-------------------------------------------------------------------------------
ExternalProject_Add(wdmp-c
    PREFIX ${CMAKE_CURRENT_BINARY_DIR}/_prefix/wdmp-c
    GIT_REPOSITORY https://github.com/Comcast/wdmp-c.git
    GIT_TAG "7de6c7c43f8d773713aad6f0c5b4e5a8d2809b1a"
    CMAKE_ARGS += -DCMAKE_INSTALL_PREFIX=${INSTALL_DIR}
                  -DBUILD_TESTING=OFF
)
add_library(libwdmp-c STATIC SHARED IMPORTED)
add_dependencies(libwdmp-c wdmp-c)

if (FEATURE_SUPPORT_WEBCONFIG)
# curl external dependency
#-------------------------------------------------------------------------------
ExternalProject_Add(curl
    PREFIX ${CMAKE_CURRENT_BINARY_DIR}/_prefix/curl
    GIT_REPOSITORY https://github.com/curl/curl.git
    GIT_TAG "curl-7_63_0"
    CMAKE_ARGS += -DCMAKE_INSTALL_PREFIX=${INSTALL_DIR} -DBUILD_TESTING=OFF
)
add_library(libcurl STATIC SHARED IMPORTED)
add_dependencies(libcurl curl)
endif (FEATURE_SUPPORT_WEBCONFIG)

# nanoMsg external dependency
#-------------------------------------------------------------------------------
ExternalProject_Add(nanomsg
    PREFIX ${CMAKE_CURRENT_BINARY_DIR}/_prefix/nanomsg
    GIT_REPOSITORY https://github.com/nanomsg/nanomsg.git
    GIT_TAG "1.1.4"
    CMAKE_ARGS += -DCMAKE_INSTALL_PREFIX=${INSTALL_DIR}
)
add_library(libnanomsg STATIC SHARED IMPORTED)
add_dependencies(libnanomsg nanomsg)

# msgpack-c external dependency
#-------------------------------------------------------------------------------
ExternalProject_Add(msgpack
    PREFIX ${CMAKE_CURRENT_BINARY_DIR}/_prefix/msgpack
    GIT_REPOSITORY https://github.com/msgpack/msgpack-c.git
    GIT_TAG "c6e6dbc608366090c12b142b3832604e6aa12f54"
    CMAKE_ARGS += -DCMAKE_INSTALL_PREFIX=${INSTALL_DIR}
                  -DMSGPACK_ENABLE_CXX=OFF
                  -DMSGPACK_BUILD_EXAMPLES=OFF
)
add_library(libmsgpack STATIC SHARED IMPORTED)
add_dependencies(libmsgpack msgpack)

# wrp-c external dependency
#-------------------------------------------------------------------------------
ExternalProject_Add(wrp-c
	DEPENDS trower-base64 msgpack cimplog
    PREFIX ${CMAKE_CURRENT_BINARY_DIR}/_prefix/wrp-c
    GIT_REPOSITORY https://github.com/Comcast/wrp-c.git
    GIT_TAG "5df89e383d7228150fbfbf127182d86379070930"
    CMAKE_ARGS += -DCMAKE_INSTALL_PREFIX=${INSTALL_DIR}
                  -DMSGPACK_ENABLE_CXX=OFF
                  -DMSGPACK_BUILD_EXAMPLES=OFF
                  -DBUILD_TESTING=OFF
                  -DMAIN_PROJ_BUILD=ON
                  -DMAIN_PROJ_LIB_PATH=${LIBRARY_DIR}
                  -DMAIN_PROJ_INCLUDE_PATH=${INCLUDE_DIR}
)
add_library(libwrp-c STATIC SHARED IMPORTED)
add_dependencies(libwrp-c wrp-c)

# libparodus external dependency
#-------------------------------------------------------------------------------
ExternalProject_Add(libparodus
    DEPENDS trower-base64 msgpack nanomsg wrp-c
    PREFIX ${CMAKE_CURRENT_BINARY_DIR}/_prefix/libparodus
    GIT_REPOSITORY https://github.com/Comcast/libparodus.git
    GIT_TAG "fcef57c79dcf9d2576aff481f1a61fe71ae93813"
    CMAKE_ARGS += -DCMAKE_INSTALL_PREFIX=${INSTALL_DIR}
                  -DMAIN_PROJ_BUILD=ON
                  -DMAIN_PROJ_LIB_PATH=${LIBRARY_DIR}
                  -DMAIN_PROJ_LIB64_PATH=${LIBRARY_DIR64}
                  -DMAIN_PROJ_COMMON_PATH=${COMMON_LIBRARY_DIR}
                  -DMAIN_PROJ_INCLUDE_PATH=${INCLUDE_DIR}
)
add_library(liblibparodus STATIC SHARED IMPORTED)
add_dependencies(liblibparodus libparodus)

# dbus external dependency
#-------------------------------------------------------------------------------
set(PATCHES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/patches)
ExternalProject_Add(dbus
    PREFIX ${PREFIX_DIR}/dbus
    GIT_REPOSITORY https://github.com/Distrotech/dbus.git
    GIT_TAG "f4606cf30763b5ae432c192cb589effc74336378"
    PATCH_COMMAND patch -p1 < ${PATCHES_DIR}/dbus.patch
    CONFIGURE_COMMAND COMMAND <SOURCE_DIR>/autogen.sh --enable-verbose-mode=no
                                                      --quiet
                                                      --disable-tests
						      --disable-Werror
                                                      --enable-asserts=no
                                                      --disable-xml-docs
                                                      --disable-doxygen-docs
                                                      --prefix=${PREFIX}
                                                      --includedir=${INCLUDE_DIR}
                                                      --libdir=${LIBRARY_DIR}
                                                      --bindir=${BIN_DIR}
                                                      --docdir=${DOC_DIR}
                                                      ${CUSTOM_HOST}
    BUILD_IN_SOURCE 1
)

add_library(libdbus STATIC SHARED IMPORTED)
add_dependencies(libdbus dbus)

# CcspCommonLibrary external dependency
#-------------------------------------------------------------------------------
set(CCSP_HEADERS "-I${INCLUDE_DIR}/dbus-1.0 -I${LIBRARY_DIR}/dbus-1.0/include")
ExternalProject_Add(CcspCommonLibrary
    DEPENDS dbus
    PREFIX ${PREFIX_DIR}/CcspCommonLibrary
    GIT_REPOSITORY https://github.com/rdkcmf/rdkb-CcspCommonLibrary.git
    GIT_TAG "a3117b5c45532a24507557886910cdaa9d0c411c"
    CONFIGURE_COMMAND COMMAND <SOURCE_DIR>/autogen.sh --quiet CPPFLAGS=${CCSP_HEADERS}
                                                      --prefix=${INSTALL_DIR}
                                                      --includedir=${INCLUDE_DIR}
                                                      --libdir=${LIBRARY_DIR}
                                                      --bindir=${BIN_DIR}
                                                      --docdir=${DOC_DIR}
                                                      ${CUSTOM_HOST}
    LOG_BUILD 1
    LOG_INSTALL 1
    BUILD_IN_SOURCE 1
)

ExternalProject_Add_Step(
  CcspCommonLibrary CopyHeaders
  COMMAND ${CMAKE_COMMAND} -E make_directory ${INCLUDE_DIR}/ccsp
  COMMAND sudo ${CMAKE_COMMAND} -E copy_directory <SOURCE_DIR>/source/debug_api/include ${INCLUDE_DIR}/ccsp
  COMMAND sudo ${CMAKE_COMMAND} -E copy_directory <SOURCE_DIR>/source/util_api/ansc/include ${INCLUDE_DIR}/ccsp
  COMMAND sudo ${CMAKE_COMMAND} -E copy_directory <SOURCE_DIR>/source/util_api/asn.1/include ${INCLUDE_DIR}/ccsp
  COMMAND sudo ${CMAKE_COMMAND} -E copy_directory <SOURCE_DIR>/source/util_api/http/include ${INCLUDE_DIR}/ccsp
  COMMAND sudo ${CMAKE_COMMAND} -E copy_directory <SOURCE_DIR>/source/util_api/stun/include ${INCLUDE_DIR}/ccsp
  COMMAND sudo ${CMAKE_COMMAND} -E copy_directory <SOURCE_DIR>/source/util_api/tls/include ${INCLUDE_DIR}/ccsp
  COMMAND sudo ${CMAKE_COMMAND} -E copy_directory <SOURCE_DIR>/source/util_api/web/include ${INCLUDE_DIR}/ccsp
  COMMAND sudo ${CMAKE_COMMAND} -E copy_directory <SOURCE_DIR>/source/cosa/include ${INCLUDE_DIR}/ccsp
  COMMAND sudo ${CMAKE_COMMAND} -E copy_directory <SOURCE_DIR>/source/cosa/package/slap/include ${INCLUDE_DIR}/ccsp
  COMMAND sudo ${CMAKE_COMMAND} -E copy_directory <SOURCE_DIR>/source/cosa/package/system/include ${INCLUDE_DIR}/ccsp
  COMMAND sudo ${CMAKE_COMMAND} -E copy_directory <SOURCE_DIR>/source/ccsp/include ${INCLUDE_DIR}/ccsp
  COMMAND sudo ${CMAKE_COMMAND} -E copy_directory <SOURCE_DIR>/source/ccsp/custom ${INCLUDE_DIR}/ccsp
  COMMAND sudo ${CMAKE_COMMAND} -E copy_directory <SOURCE_DIR>/source/ccsp/components/include ${INCLUDE_DIR}/ccsp
  COMMAND sudo ${CMAKE_COMMAND} -E copy_directory <SOURCE_DIR>/source/ccsp/components/common/MessageBusHelper/include ${INCLUDE_DIR}/ccsp
  COMMAND sudo ${CMAKE_COMMAND} -E copy <SOURCE_DIR>/source/util_api/slap/components/SlapVarConverter/slap_vco_global.h ${INCLUDE_DIR}/ccsp
  COMMAND sudo ${CMAKE_COMMAND} -E copy <SOURCE_DIR>/source/ccsp/components/common/PoamIrepFolder/poam_irepfo_exported_api.h ${INCLUDE_DIR}/ccsp
  COMMAND sudo ${CMAKE_COMMAND} -E copy <SOURCE_DIR>/source/ccsp/components/common/PoamIrepFolder/poam_irepfo_global.h ${INCLUDE_DIR}/ccsp
  COMMAND sudo ${CMAKE_COMMAND} -E copy <SOURCE_DIR>/source/ccsp/components/common/PoamIrepFolder/poam_irepfo_interface.h ${INCLUDE_DIR}/ccsp
  COMMAND sudo ${CMAKE_COMMAND} -E copy <SOURCE_DIR>/source/ccsp/components/common/PoamIrepFolder/poam_irepfo_internal_api.h ${INCLUDE_DIR}/ccsp
  DEPENDEES install
)

add_library(libccsp_common SHARED IMPORTED)
add_dependencies(libccsp_common CcspCommonLibrary)

if (BUILD_TESTING)
# cmocka external dependency
#-------------------------------------------------------------------------------
ExternalProject_Add(cmocka
    PREFIX ${CMAKE_CURRENT_BINARY_DIR}/_prefix/cmocka
    GIT_REPOSITORY https://github.com/elventear/cmocka.git
    GIT_TAG "b71a3060699bc1a5b00e958be353772f42545ac2"
    CMAKE_ARGS += -DCMAKE_INSTALL_PREFIX=${INSTALL_DIR}
)
add_library(libcmocka STATIC SHARED IMPORTED)
add_dependencies(libcmocka cmocka)

endif (BUILD_TESTING)
endif () # NOT BUILD_YOCTO

link_directories ( ${LIBRARY_DIR} ${COMMON_LIBRARY_DIR} ${LIBRARY_DIR64} )

enable_testing()

add_subdirectory(source)

if (BUILD_TESTING)
add_subdirectory(tests)
endif (BUILD_TESTING)
