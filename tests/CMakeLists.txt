#   Copyright 2016 Comcast Cable Communications Management, LLCD
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

set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -W -g -fprofile-arcs -ftest-coverage -O0")
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DTEST -D_ANSC_LINUX")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -W  -g -fprofile-arcs -ftest-coverage -O0")
set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fprofile-arcs -ftest-coverage -O0")
set (WEBPA_COMMON_LIBS gcov -lcunit -lcimplog -lwrp-c -lpthread -lmsgpackc -lnanomsg -Wl,--no-as-needed -lcjson -ltrower-base64 -lssl -lcrypto -lrt -luuid -lm -lcmocka)
set (WEBPA_COMMON_SOURCES ../source/broadband/webpa_adapter.c ../source/broadband/webpa_internal.c ../source/broadband/webpa_parameter.c ../source/broadband/webpa_attribute.c ../source/broadband/webpa_notification.c)
set (WEBPA_TABLE_SOURCES ../source/broadband/webpa_replace.c ../source/broadband/webpa_table.c)
link_directories ( ${LIBRARY_DIR} )

#-------------------------------------------------------------------------------
#   test_libpd
#-------------------------------------------------------------------------------
add_test(NAME test_libpd COMMAND ${MEMORY_CHECK} ./test_libpd)
add_executable(test_libpd test_libpd.c ../source/app/libpd.c)
target_link_libraries (test_libpd -lwrp-c ${WEBPA_COMMON_LIBS} -llibparodus)
target_link_libraries (test_libpd gcov -Wl,--no-as-needed )

#-------------------------------------------------------------------------------
#   test_webpa_rbus
#-------------------------------------------------------------------------------
add_test(NAME test_webpa_rbus COMMAND ${MEMORY_CHECK} ./test_webpa_rbus)
add_executable(test_webpa_rbus test_webpa_rbus.c ../source/broadband/webpa_rbus.c)
target_link_libraries (test_webpa_rbus ${WEBPA_COMMON_LIBS} -llibparodus -lrbus)
target_link_libraries (test_webpa_rbus gcov -Wl,--no-as-needed )

# #-------------------------------------------------------------------------------
# #   test_webpa_adapter
# #-------------------------------------------------------------------------------
# add_test(NAME test_webpa_adapter COMMAND ${MEMORY_CHECK} ./test_webpa_adapter)
# add_executable(test_webpa_adapter test_webpa_adapter.c ../source/broadband/webpa_adapter.c)
# target_link_libraries (test_webpa_adapter -lwrp-c -ldbus-1 -lccsp_common -lwdmp-c -lcjson ${WEBPA_COMMON_LIBS})

# #-------------------------------------------------------------------------------
# #   test_webpa_get
# #-------------------------------------------------------------------------------
# add_test(NAME test_webpa_get COMMAND ${MEMORY_CHECK} ./test_webpa_get)
# add_executable(test_webpa_get test_webpa_get.c mock_stack.c mock_cache.c ../source/app/libpd.c ${WEBPA_COMMON_SOURCES} ${WEBPA_TABLE_SOURCES})
# target_link_libraries (test_webpa_get -lwrp-c -ldbus-1 -lccsp_common -lwdmp-c -lcjson ${WEBPA_COMMON_LIBS} -llibparodus)

# #-------------------------------------------------------------------------------
# #   test_webpa_set
# #-------------------------------------------------------------------------------
# add_test(NAME test_webpa_set COMMAND ${MEMORY_CHECK} ./test_webpa_set)
# add_executable(test_webpa_set test_webpa_set.c mock_stack.c mock_cache.c ../source/app/libpd.c ${WEBPA_COMMON_SOURCES} ${WEBPA_TABLE_SOURCES})
# target_link_libraries (test_webpa_set -lwrp-c -ldbus-1 -lccsp_common -lwdmp-c -lcjson ${WEBPA_COMMON_LIBS} -llibparodus)

 #-------------------------------------------------------------------------------
 #   test_webpa_notification
 #-------------------------------------------------------------------------------
 add_test(NAME test_webpa_notification COMMAND ${MEMORY_CHECK} ./test_webpa_notification)
 add_executable(test_webpa_notification test_webpa_notification.c mock_stack.c mock_cache.c ../source/app/libpd.c ${WEBPA_COMMON_SOURCES} ${WEBPA_TABLE_SOURCES})
 target_link_libraries (test_webpa_notification -lwrp-c -ldbus-1 -lccsp_common -lwdmp-c -lcjson ${WEBPA_COMMON_LIBS} -llibparodus)

# #-------------------------------------------------------------------------------
# #   test_webpa_get_attributes
# #-------------------------------------------------------------------------------
# add_test(NAME test_webpa_get_attr COMMAND ${MEMORY_CHECK} ./test_webpa_get_attr)
# add_executable(test_webpa_get_attr test_webpa_get_attr.c mock_stack.c mock_cache.c ../source/app/libpd.c ${WEBPA_COMMON_SOURCES} ${WEBPA_TABLE_SOURCES})
# target_link_libraries (test_webpa_get_attr -lwrp-c -ldbus-1 -lccsp_common -lwdmp-c -lcjson ${WEBPA_COMMON_LIBS} -llibparodus)

# #-------------------------------------------------------------------------------
# #   test_webpa_set_attributes
# #-------------------------------------------------------------------------------
# add_test(NAME test_webpa_set_attr COMMAND ${MEMORY_CHECK} ./test_webpa_set_attr)
# add_executable(test_webpa_set_attr test_webpa_set_attr.c mock_stack.c mock_cache.c ../source/app/libpd.c ${WEBPA_COMMON_SOURCES} ${WEBPA_TABLE_SOURCES})
# target_link_libraries (test_webpa_set_attr -lwrp-c -ldbus-1 -lccsp_common -lwdmp-c -lcjson ${WEBPA_COMMON_LIBS} -llibparodus)

# #-------------------------------------------------------------------------------
# #   test_webpa_replace
# #-------------------------------------------------------------------------------
# add_test(NAME test_webpa_replace COMMAND ${MEMORY_CHECK} ./test_webpa_replace)
# add_executable(test_webpa_replace test_webpa_replace.c mock_stack.c mock_cache.c ../source/app/libpd.c ../source/broadband/webpa_replace.c ${WEBPA_COMMON_SOURCES})
# target_link_libraries (test_webpa_replace -lwrp-c -ldbus-1 -lccsp_common -lwdmp-c -lcjson ${WEBPA_COMMON_LIBS} -llibparodus)

# Code coverage

add_custom_target(coverage
COMMAND lcov -q --capture --directory 
${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/test_libpd.dir/__/src --output-file test_libpd.info

COMMAND lcov
-a test_libpd.info

COMMAND genhtml coverage.info
WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
