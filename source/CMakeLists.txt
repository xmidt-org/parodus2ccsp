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

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_ANSC_LINUX ")
set(WEBCONFIG_PATCH "${PATCHES_DIR}/Web_config_XML.patch")
set(SOURCES broadband/ssp_messagebus_interface.c broadband/ssp_main.c broadband/ssp_action.c broadband/cosa_webpa_dml.c broadband/cosa_webpa_internal.c broadband/cosa_webpa_apis.c broadband/plugin_main.c broadband/plugin_main_apis.c broadband/webpa_adapter.c broadband/webpa_internal.c broadband/webpa_table.c broadband/webpa_replace.c broadband/webpa_parameter.c broadband/webpa_attribute.c broadband/webpa_notification.c app/main.c app/libpd.c app/privilege.c)

if (BUILD_YOCTO)
set(SOURCES ${SOURCES} broadband/dm_pack_datamodel.c)
endif()

if (FEATURE_SUPPORT_WEBCONFIG)
set(SOURCES ${SOURCES} WebConfig/cosa_apis_util.c WebConfig/cosa_webconfig_dml.c WebConfig/cosa_webconfig_apis.c WebConfig/webconfig_internal.c WebConfig/webconfig_log.c WebConfig/cosa_webconfig_internal.c)
 execute_process(
    COMMAND patch -p1 --forward --ignore-whitespace
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/source/arch"
    INPUT_FILE "${WEBCONFIG_PATCH}"
    OUTPUT_VARIABLE OUTPUT
    RESULT_VARIABLE RESULT
)
if (RESULT EQUAL 0)
      message(STATUS "Patch applied: ${WEBCONFIG_PATCH}")
else()
    message(STATUS "Failed to apply patch: ${WEBCONFIG_PATCH}")
endif()
endif()
set(HEADERS app/libpd.h)
add_executable(webpa ${SOURCES} ${HEADERS})
set(COMMON_LIBS -ldbus-1 -lccsp_common -lwrp-c -lpthread -lwdmp-c -lmsgpackc -ltrower-base64 -lm -lnanomsg -lcjson -lrt -luuid -llibparodus -lcimplog)
if (BUILD_YOCTO)
set(COMMON_LIBS "${COMMON_LIBS} -llog4c -lrdkloggers")
endif()
if (FEATURE_SUPPORT_WEBCONFIG)
set(COMMON_LIBS "${COMMON_LIBS} -lcurl")
endif()
target_link_libraries (webpa ${CMAKE_THREAD_LIBS_INIT} ${COMMON_LIBS})
install (TARGETS webpa DESTINATION bin)
