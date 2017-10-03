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
#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <malloc.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include <ccsp_base_api.h>
#include "mock_stack.h"
#include "../source/include/webpa_adapter.h"

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
#define UNUSED(x) (void )(x)

/*----------------------------------------------------------------------------*/
/*                               Data Structures                              */
/*----------------------------------------------------------------------------*/
typedef int DmErr_t;

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
int bus_handle;
componentStruct_t **compList;
int compSize;
parameterValStruct_t **valueList;
parameterAttributeStruct_t **attrList;
int totalCount;
int rowId;
char *faultParam;

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/

void set_global_components(componentStruct_t **components)
{
    compList = components;
}

componentStruct_t ** get_global_components(void)
{
    return (componentStruct_t **) mock();
}

void set_global_component_size(int size)
{
    compSize = size;
}

int get_global_component_size(void)
{
    return (int) mock();
}

void set_global_parameters_count(int count)
{
    totalCount = count;
}

int get_global_parameters_count(void)
{
    return (int) mock();
}

char *get_global_faultParam(void)
{
    return (char *) mock();
}

void set_global_values(parameterValStruct_t **values)
{
    valueList = values;
}

parameterValStruct_t ** get_global_values(void)
{
    return (parameterValStruct_t **) mock();
}

void set_global_attributes(parameterAttributeStruct_t **attributes)
{
    attrList = attributes;
}

parameterAttributeStruct_t ** get_global_attributes()
{
    return (parameterAttributeStruct_t **) mock();
}

/*----------------------------------------------------------------------------*/
/*                                   Mocks                                    */
/*----------------------------------------------------------------------------*/

int CcspBaseIf_discComponentSupportingNamespace (void* bus_handle, const char* dst_component_id, const char *name_space, const char *subsystem_prefix, componentStruct_t ***components, int *size)
{
    UNUSED(bus_handle); UNUSED(dst_component_id); UNUSED(name_space); UNUSED(subsystem_prefix);
    *components = get_global_components();
    *size = get_global_component_size();
    function_called();
    return (int) mock();
}

void free_componentStruct_t (void* bus_handle, int size, componentStruct_t **val)
{
    UNUSED(bus_handle);
    int i;
    for(i = 0; i< size; i++)
    {
        WAL_FREE(val[i]->componentName);
        WAL_FREE(val[i]->dbusPath);
        WAL_FREE(val[i]);
    }
    WAL_FREE(val);
    function_called();
}

int CcspBaseIf_getParameterValues(void* bus_handle, const char* dst_component_id, char* dbus_path, char * parameterNames[], int size, int *val_size, parameterValStruct_t ***val)
{
    UNUSED(bus_handle); UNUSED(dst_component_id); UNUSED(dbus_path); UNUSED(parameterNames);
    check_expected(size);
    *val = get_global_values();
    *val_size = get_global_parameters_count();
    function_called();
    return (int) mock();
}

void free_parameterValStruct_t (void* bus_handle, int size,parameterValStruct_t **val)
{
    UNUSED(bus_handle); UNUSED(size); UNUSED(val);
    function_called();
}

int CcspBaseIf_setParameterValues(void* bus_handle, const char* dst_component_id, char* dbus_path, int sessionId, unsigned int writeID, parameterValStruct_t *val, int size, dbus_bool commit, char ** invalidParameterName)
{
    UNUSED(bus_handle); UNUSED(dst_component_id); UNUSED(dbus_path); UNUSED(sessionId); UNUSED(writeID); UNUSED(size); UNUSED(val); UNUSED(commit);
    check_expected(size);
    *invalidParameterName = get_global_faultParam();
    function_called();
    return (int) mock();
}

int CcspBaseIf_setParameterAttributes(void* bus_handle, const char* dst_component_id, char* dbus_path, int sessionId, parameterAttributeStruct_t *val, int size)
{
    UNUSED(bus_handle); UNUSED(dst_component_id); UNUSED(dbus_path); UNUSED(sessionId); UNUSED(size); UNUSED(val);
    function_called();
    check_expected(size);
    return (int) mock();
}

int CcspBaseIf_getParameterAttributes(void* bus_handle, const char* dst_component_id, char* dbus_path, char * parameterNames[], int size, int *val_size, parameterAttributeStruct_t ***val)
{
    UNUSED(bus_handle); UNUSED(dst_component_id); UNUSED(dbus_path); UNUSED(parameterNames);
    check_expected(size);
    *val_size = totalCount;
    *val = get_global_attributes();
    function_called();
    return (int) mock();
}

void free_parameterAttributeStruct_t(void* bus_handle, int size, parameterAttributeStruct_t **val)
{
    UNUSED(bus_handle); UNUSED(size); UNUSED(val);
    function_called();
}

int CcspBaseIf_AddTblRow(void* bus_handle, const char* dst_component_id, char* dbus_path, int sessionId, char * objectName, int * instanceNumber)
{
    UNUSED(bus_handle); UNUSED(dst_component_id); UNUSED(dbus_path); UNUSED(objectName); UNUSED(sessionId);
    *instanceNumber = rowId;
    function_called();
    return (int) mock();
}

int CcspBaseIf_DeleteTblRow(void* bus_handle, const char* dst_component_id, char* dbus_path, int sessionId, char * objectName)
{
    UNUSED(bus_handle); UNUSED(dst_component_id); UNUSED(dbus_path); UNUSED(objectName); UNUSED(sessionId);
    function_called();
    return (int) mock();
}

void free_parameterInfoStruct_t (void* bus_handle, int size, parameterInfoStruct_t **val)
{
    UNUSED(bus_handle); UNUSED(size); UNUSED(val);
    function_called();
}

void CcspBaseIf_SetCallback2(void* bus_handle, char *name, void*  func, void * user_data)
{
    UNUSED(bus_handle); UNUSED(name); UNUSED(func); UNUSED(user_data);
    function_called();
}

int CcspBaseIf_isSystemReady (void* bus_handle, const char* dst_component_id, dbus_bool *val)
{
    UNUSED(bus_handle); UNUSED(dst_component_id); UNUSED(val);
    function_called();
    return (int) mock();
}

int  CcspBaseIf_Register_Event(void* bus_handle, const char* sender, const char* event_name)
{
    UNUSED(bus_handle); UNUSED(sender); UNUSED(event_name);
    function_called();
    return (int) mock();
}

int CcspBaseIf_unregisterComponent (void* bus_handle, const char* dst_component_id, const char *component_name)
{
    UNUSED(bus_handle); UNUSED(dst_component_id); UNUSED(component_name);
    function_called();
    return (int) mock();
}

void CcspBaseIf_SetCallback(void* bus_handle, CCSP_Base_Func_CB*  func)
{
    UNUSED(bus_handle); UNUSED(func);
    function_called();
}

DBusHandlerResult CcspBaseIf_base_path_message_func (DBusConnection *conn, DBusMessage *message, DBusMessage *reply, const char *interface, const char *method,
                                                     CCSP_MESSAGE_BUS_INFO *bus_info)
{
    UNUSED(conn); UNUSED(message); UNUSED(reply); UNUSED(interface); UNUSED(method); UNUSED(bus_info);
    function_called();
    return (DBusHandlerResult) mock();
}

void CCSP_Msg_SleepInMilliSeconds(int milliSecond)
{
    sleep(milliSecond/1000);
}

int CcspCcMbi_GetParameterValues(unsigned int writeID, char *parameterNames[], int size, int *val_size, parameterValStruct_t ***val, void * user_data)
{
    UNUSED(writeID); UNUSED(parameterNames); UNUSED(size); UNUSED(val_size); UNUSED(val); UNUSED(user_data);
    function_called();
    return (int) mock();
}

int CcspCcMbi_SetParameterValues(int sessionId, unsigned int writeID, parameterValStruct_t *val, int size, dbus_bool commit, char ** invalidParameterName, void * user_data)
{
    UNUSED(sessionId); UNUSED(writeID); UNUSED(val); UNUSED(size); UNUSED(commit); UNUSED(invalidParameterName); UNUSED(user_data);
    function_called();
    return (int) mock();
}

int CcspCcMbi_SetCommit(int sessionId, unsigned int writeID, dbus_bool commit, void * user_data)
{
    UNUSED(sessionId); UNUSED(writeID); UNUSED(commit); UNUSED(user_data);
    function_called();
    return (int) mock();
}

int CcspCcMbi_SetParameterAttributes(int sessionId, parameterAttributeStruct_t *val, int size, void *user_data)
{
    UNUSED(sessionId); UNUSED(val); UNUSED(size); UNUSED(user_data);
    function_called();
    return (int) mock();
}

int CcspCcMbi_GetParameterAttributes(char* parameterNames[], int size, int* val_size, parameterAttributeStruct_t ***val, void * user_data)
{
    UNUSED(parameterNames); UNUSED(size); UNUSED(val_size); UNUSED(val); UNUSED(user_data);
    function_called();
    return (int) mock();
}

int CcspCcMbi_AddTblRow(int sessionId, char *objectName, int *instanceNumber, void *user_data)
{
    UNUSED(sessionId); UNUSED(objectName); UNUSED(instanceNumber); UNUSED(user_data);
    function_called();
    return (int) mock();
}

int CcspCcMbi_DeleteTblRow(int sessionId, char *objectName, void *user_data)
{
    UNUSED(sessionId); UNUSED(objectName); UNUSED(user_data);
    function_called();
    return (int) mock();
}

int CcspCcMbi_GetParameterNames(char *parameterName, dbus_bool nextLevel, int *size, parameterInfoStruct_t ***val, void *user_data)
{
    UNUSED(parameterName); UNUSED(nextLevel); UNUSED(size); UNUSED(val); UNUSED(user_data);
    function_called();
    return (int) mock();
}

void CcspCcMbi_CurrentSessionIdSignal( int priority, int sessionID, void *user_data)
{
    UNUSED(priority); UNUSED(sessionID); UNUSED(user_data);
    function_called();
}

int CCSP_Message_Bus_Register_Path2(void* bus_handle, const char *path, DBusObjectPathMessageFunction funcptr, void *user_data)
{
    UNUSED(bus_handle); UNUSED(path); UNUSED(funcptr); UNUSED(user_data);
    function_called();
    return (int) mock();
}

DmErr_t Cdm_Init(const void *busHdl, const char *subSys, const char *cnfFil, const char *crId, const char *compId)
{
    UNUSED(busHdl); UNUSED(subSys); UNUSED(cnfFil); UNUSED(crId); UNUSED(compId);
    function_called();
    return (DmErr_t) mock();
}

DmErr_t Cdm_Term(void)
{
    function_called();
    return (DmErr_t) mock();
}

const char *Cdm_StrError(DmErr_t err)
{
    UNUSED(err);
    function_called();
    return (char *) mock();
}

void AnscPrintComponentMemoryTable(char *pComponentName)
{
    UNUSED(pComponentName);
    function_called();
}

void AnscTraceMemoryTable(void)
{
    function_called();
}

void AnscZeroMemory(void *pMemory, unsigned long ulMemorySize)
{
    UNUSED(pMemory); UNUSED(ulMemorySize);
    function_called();
}

long AnscGetComponentMemorySize(char *pComponentName)
{
    UNUSED(pComponentName);
    function_called();
    return (long) mock();
}

void *Ansc_AllocateMemory_Callback(unsigned long ulMemorySize)
{
    UNUSED(ulMemorySize);
    function_called();
    return (void *) mock();
}

void Ansc_FreeMemory_Callback(void *pMemory)
{
    UNUSED(pMemory);
    function_called();
}

char *AnscCloneString(char *s)
{ 
    return strdup(s); 
}

ANSC_HANDLE DslhCreateCpeController(ANSC_HANDLE hContainerContext, ANSC_HANDLE hOwnerContext, ANSC_HANDLE hAnscReserved)
{
    UNUSED(hContainerContext); UNUSED(hOwnerContext); UNUSED(hAnscReserved);
    function_called();
    return (ANSC_HANDLE) mock();
}

void* MsgHelper_CreateCcdMbiIf(void *dbusHandle, char *pPrefix)
{
    UNUSED(dbusHandle); UNUSED(pPrefix);
    function_called();
    return (void*) mock();
}
