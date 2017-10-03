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

#ifndef _MOCK_STACK_H_
#define _MOCK_STACK_H_

#include <ccsp_base_api.h>

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
#if defined(CcspTraceInfo)
#undef CcspTraceInfo  
#define CcspTraceInfo(...)     \
{                              \
    (printf __VA_ARGS__);      \
    fflush(stdout);            \
}
#endif //CcspTraceInfo

#if defined(CcspTraceWarning)
#undef CcspTraceWarning
#define CcspTraceWarning(...)  \
{                              \
    (printf __VA_ARGS__);      \
    fflush(stdout);            \
}
#endif //CcspTraceWarning

#if defined(CcspTraceError)
#undef CcspTraceError
#define CcspTraceError(...)    \
{                              \
    (printf __VA_ARGS__);      \
    fflush(stdout);            \
}
#endif //CcspTraceError

/*----------------------------------------------------------------------------*/
/*                               Data Structures                              */
/*----------------------------------------------------------------------------*/
typedef void* ANSC_HANDLE;

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

#endif

