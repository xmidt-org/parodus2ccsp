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

set(PROJ_PARODUS2CCSP parodus2ccsp)

file(GLOB HEADERS adapter.h)
set(SOURCES adapter.c)

add_library(${PROJ_PARODUS2CCSP} STATIC ${HEADERS} ${SOURCES})
add_library(${PROJ_PARODUS2CCSP}.shared SHARED ${HEADERS} ${SOURCES})
set_target_properties(${PROJ_PARODUS2CCSP}.shared PROPERTIES OUTPUT_NAME ${PROJ_PARODUS2CCSP})

install (TARGETS ${PROJ_PARODUS2CCSP} DESTINATION lib${LIB_SUFFIX})
install (TARGETS ${PROJ_PARODUS2CCSP}.shared DESTINATION lib${LIB_SUFFIX})
install (FILES adapter.h DESTINATION include/${PROJ_PARODUS2CCSP})
