/**
 *  Copyright 2010-2016 Comcast Cable Communications Management, LLC
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#ifndef _MOCK_SATCK_H_
#define _MOCK_SATCK_H_

#include <ccsp_base_api.h>
#include <rbus/rbus.h>
#define UNUSED(x) (void )(x)
/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/

void set_global_components(componentStruct_t **components);
componentStruct_t ** get_global_components();
void set_global_values(parameterValStruct_t **values);
parameterValStruct_t ** get_global_values();
void set_global_component_size(int size);
int get_global_component_size();
void set_global_parameters_count(int count);
int get_global_parameters_count();
void set_global_attributes(parameterAttributeStruct_t **attributes);
parameterAttributeStruct_t ** get_global_attributes();
void clearTraceContext();
rbusError_t getTraceContext(char* traceContext[]);
rbusError_t setTraceContext(char* traceContext[]);

#endif

