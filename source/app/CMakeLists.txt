#   Copyright 2016 Comcast Cable Communications Management, LLC
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
add_definitions(-std=gnu99)
set(SOURCES main.c libpd.c)
set(HEADERS libpd.h)
link_directories(${CMAKE_CURRENT_BINARY_DIR}/../broadband)
add_executable(webpa ${SOURCES} ${HEADERS})
set(COMMON_LIBS -lwrp-c -lpthread -lwdmp-c -lmsgpackc -ltrower-base64 -lm -lcimplog -lnanomsg -lcjson -lrt -llibparodus)
target_link_libraries (webpa ${CMAKE_THREAD_LIBS_INIT} ${COMMON_LIBS} libbroadband.so)

install (TARGETS webpa DESTINATION bin)
