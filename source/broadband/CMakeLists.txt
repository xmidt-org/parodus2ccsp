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

set(PROJ_BROADBAND broadband)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_ANSC_LINUX ")
set(SOURCES ssp_messagebus_interface.c ssp_main.c ssp_action.c cosa_webpa_dml.c cosa_webpa_internal.c cosa_webpa_apis.c plugin_main.c webpa_adapter.c webpa_internal.c webpa_table.c webpa_parameter.c webpa_attribute.c webpa_notification.c)

set (COMMON_FLAGS -ldbus-1 -lccsp_common -lwdmp-c -lcjson -lpthread -lrt -lcimplog -lm)
add_library(${PROJ_BROADBAND} STATIC ${SOURCES})
add_library(${PROJ_BROADBAND}.shared SHARED ${HEADERS} ${SOURCES})
set_target_properties(${PROJ_BROADBAND}.shared PROPERTIES OUTPUT_NAME ${PROJ_BROADBAND})
target_link_libraries(${PROJ_BROADBAND}.shared ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(${PROJ_BROADBAND}.shared ${COMMON_FLAGS})
install (TARGETS ${PROJ_BROADBAND} DESTINATION ${INSTALL_DIR})
install (TARGETS ${PROJ_BROADBAND}.shared DESTINATION ${INSTALL_DIR})
