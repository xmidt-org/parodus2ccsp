/**
 *  Copyright 2010-2023 Comcast Cable Communications Management, LLC
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

#include <rbus/rbus.h>
#include <string.h>
#include "../source/include/webpa_adapter.h"
#include "../source/broadband/include/webpa_internal.h"
#include "../source/broadband/include/webpa_notification.h"
#include <cimplog/cimplog.h>
#include <wdmp-c.h>
#include <cJSON.h>
#include "ccsp_dm_api.h"
//#include "mock_stack.h"
#include <libparodus.h>
#include <ccsp_base_api.h>
#include <CUnit/Basic.h>

extern char deviceMAC[32];
#define MAX_PARAMETER_LEN			512
#define UNUSED(x) (void )(x)
#define WEBPA_NOTIFY_EVENT_MAX_LENGTH                   256
ANSC_HANDLE bus_handle;
extern int numLoops;
extern pthread_cond_t con;

void clearTraceContext()
{

}

rbusError_t getTraceContext(char* traceContext[])
{
    UNUSED(traceContext);
}

rbusError_t setTraceContext(char* traceContext[])
{
    UNUSED(traceContext);
}


int getWebpaParameterValues(char **parameterNames, int paramCount, int *val_size, parameterValStruct_t ***val)
{
    UNUSED(parameterNames); UNUSED(paramCount); UNUSED(val_size); UNUSED(val); 
}

int setWebpaParameterValues(parameterValStruct_t *val, int paramCount, char **faultParam )
{
    UNUSED(faultParam); UNUSED(paramCount); UNUSED(val);
}

int CcspBaseIf_discComponentSupportingNamespace (void* bus_handle, const char* dst_component_id, const char *name_space, const char *subsystem_prefix, componentStruct_t ***components, int *size)
{
    UNUSED(bus_handle);
    UNUSED(dst_component_id);
    UNUSED(name_space);
    UNUSED(subsystem_prefix);
     int count = 2, i = 0;
    char *compNames[] = {"com.ccsp.pam","com.ccsp.webpa"};
    char *dbusPaths[] = {"/com/ccsp/pam","/com/ccsp/webpa"};
    componentStruct_t **list = (componentStruct_t **) malloc(sizeof(componentStruct_t *)*count);
    for(i=0; i<count; i++)
    {
        list[i] = (componentStruct_t *) malloc(sizeof(componentStruct_t));
        list[i]->componentName = strdup(compNames[i]);
        list[i]->dbusPath = strdup(dbusPaths[i]);
    }

    *components = list;
    *size = 1;
    
    return 100;  
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
}

int CcspBaseIf_getParameterValues(
    void* bus_handle,
    const char* dst_component_id,
    char* dbus_path,
    char* parameterNames[],
    int size,
    int* val_size,
    parameterValStruct_t*** val
) {
    UNUSED(bus_handle);
    UNUSED(dst_component_id);
    UNUSED(dbus_path);
    UNUSED(parameterNames);
    UNUSED(size);

    int totalCount = 1;
    *val = (parameterValStruct_t**)malloc(sizeof(parameterValStruct_t*) * totalCount);

    for (int i = 0; i < totalCount; i++) {
        (*val)[i] = (parameterValStruct_t*)malloc(sizeof(parameterValStruct_t));
        (*val)[i]->parameterName = (char*)malloc(sizeof(char) * MAX_PARAMETER_LEN);
        strncpy((*val)[i]->parameterName, "Device.DeviceInfo.X_RDKCENTRAL-COM_AkerEnable", MAX_PARAMETER_LEN);
        (*val)[i]->parameterValue = (char*)malloc(sizeof(char) * MAX_PARAMETER_LEN);
        strncpy((*val)[i]->parameterValue, "true", MAX_PARAMETER_LEN);
        (*val)[i]->type = ccsp_boolean;
    }

    // Set the size of the result
    *val_size = totalCount;

    return 100;
}

int libparodus_send (libpd_instance_t instance, wrp_msg_t *msg)
{
    UNUSED(instance);
    UNUSED(msg);
    return 0;
}

int  CcspBaseIf_Register_Event(void* bus_handle, const char* sender, const char* event_name)
{
    UNUSED(bus_handle); UNUSED(sender); UNUSED(event_name);
    return 100;
}

void CcspBaseIf_SetCallback2(void* bus_handle, char *name, void*  func, void * user_data)
{
    UNUSED(bus_handle); UNUSED(name); UNUSED(func); UNUSED(user_data);
    return 100;
}

int CcspBaseIf_setParameterAttributes(void* bus_handle, const char* dst_component_id, char* dbus_path, int sessionId, parameterAttributeStruct_t *val, int size)
{
    UNUSED(bus_handle); UNUSED(dst_component_id); UNUSED(dbus_path); UNUSED(sessionId); UNUSED(size); UNUSED(val);
    return 100;
}

//TODO: This functions stops in the middle, need to look into it
void test_initNotifyTask()
{
    numLoops = 1;
    int i=1;
    strcpy(deviceMAC, "14cfe2142112");
    getDeviceMac();
    initNotifyTask(0);
}

void test_validate_conn_client_notify_data()
{
    char *param = "dummy";
    char *interface = "abcdef";
    char *mac_id = "12345678901234567";
    char *status = "success";
    char *name = "hostname";

    int result = validate_conn_client_notify_data(param, interface, mac_id, status, name);
    CU_ASSERT_EQUAL(result,WDMP_SUCCESS);
}

void test_validate_conn_client_param_name_failure()
{
    char notify_param_name[WEBPA_NOTIFY_EVENT_MAX_LENGTH + 1];
    memset(notify_param_name, 'A', WEBPA_NOTIFY_EVENT_MAX_LENGTH);
    char *interface = "abcdef";
    char *mac_id = "12345678901234567";
    char *status = "success";
    char *name = "hostname";
    
    //notify_param_name validation fail
    int result = validate_conn_client_notify_data(notify_param_name, interface, mac_id, status, name);
    CU_ASSERT_EQUAL(result,WDMP_FAILURE);
}

void test_validate_conn_client_interface_failure()
{
    char *param = "dummy";
    char interface[17];
    memset(interface, 'A',17);
    char *mac_id = "12345678901234567";
    char *status = "success";
    char *name = "hostname";
    
    //interface validation fail
    int result = validate_conn_client_notify_data(param, interface, mac_id, status, name);
    CU_ASSERT_EQUAL(result,WDMP_FAILURE);
}

void test_validate_conn_client_mac_failure()
{
    char *param = "dummy";
    char *interface = "abcdef"; 
    char *mac_id = "123456789012345";
    char *status = "success";
    char *name = "hostname";
    
    //mac validation fail
    int result = validate_conn_client_notify_data(param, interface, mac_id, status, name);
    CU_ASSERT_EQUAL(result,WDMP_FAILURE);
} 

void test_validate_conn_client_status_failure()
{
    char *param = "dummy";
    char *interface = "abcdef"; 
    char *mac_id = "12345678901234567";
    char status[33];
    memset(status, 'A',33);
    char *name = "hostname";
    
    //status validation fail
    int result = validate_conn_client_notify_data(param, interface, mac_id, status, name);
    CU_ASSERT_EQUAL(result,WDMP_FAILURE);
} 

void test_validate_conn_client_hostname_failure()
{
    char *param = "dummy";
    char *interface = "abcdef"; 
    char *mac_id = "12345678901234567";
    char *status = "success";
    char name[WEBPA_NOTIFY_EVENT_MAX_LENGTH+1];
    memset(name,'A',WEBPA_NOTIFY_EVENT_MAX_LENGTH+1);
    
    //hostname validation fail
    int result = validate_conn_client_notify_data(param, interface, mac_id, status, name);
    CU_ASSERT_EQUAL(result,WDMP_FAILURE);
} 

void test_validate_webpa_notification_data_success()
{
    char *name = "dummmyvalue";
    char *write = "dummywrite";
    int result = validate_webpa_notification_data(name,write);
    CU_ASSERT_EQUAL(result,WDMP_SUCCESS);
}

void test_validate_webpa_notification_notify_failure()
{
    char name[WEBPA_NOTIFY_EVENT_MAX_LENGTH+1];
    memset(name,'A',WEBPA_NOTIFY_EVENT_MAX_LENGTH+1);
    char *write = "dummywrite";
    int result = validate_webpa_notification_data(name,write);
    CU_ASSERT_EQUAL(result,WDMP_FAILURE);
}

void test_validate_webpa_notification_write_id_failure()
{
    char *name = "dummmyvalue";
    char write[17];
    memset(write,'A',17);
    int result = validate_webpa_notification_data(name,write);
    CU_ASSERT_EQUAL(result,WDMP_FAILURE);
}

void add_suites( CU_pSuite *suite )
{
    *suite = CU_add_suite( "tests", NULL, NULL );
    CU_add_test( *suite, "test initNotifyTask", test_initNotifyTask);
    CU_add_test( *suite, "test validate_conn_client_notify_data", test_validate_conn_client_notify_data);
    CU_add_test( *suite, "test validate_conn_client_param_name_failure", test_validate_conn_client_param_name_failure);
    CU_add_test( *suite, "test validate_conn_client_interface_failure", test_validate_conn_client_interface_failure);
    CU_add_test( *suite, "test validate_conn_client_mac_failure", test_validate_conn_client_mac_failure);
    CU_add_test( *suite, "test validate_conn_client_status_failure", test_validate_conn_client_status_failure);
    CU_add_test( *suite, "test validate_conn_client_hostname_failure", test_validate_conn_client_hostname_failure);
    CU_add_test( *suite, "test validate_webpa_notification_data_success", test_validate_webpa_notification_data_success);
    CU_add_test( *suite, "test validate_webpa_notification_notify_failure", test_validate_webpa_notification_notify_failure);
    CU_add_test( *suite, "test validate_webpa_notification_write_id_failure", test_validate_webpa_notification_write_id_failure);

}

int main( int argc, char *argv[] )
{
    unsigned rv = 1;
    CU_pSuite suite = NULL;
 
    (void ) argc;
    (void ) argv;
    
    if( CUE_SUCCESS == CU_initialize_registry() ) {
        add_suites( &suite );

        if( NULL != suite ) {
            CU_basic_set_mode( CU_BRM_VERBOSE );
            CU_basic_run_tests();
            printf( "\n" );
            CU_basic_show_failures( CU_get_failure_list() );
            printf( "\n\n" );
            rv = CU_get_number_of_tests_failed();
        }

        CU_cleanup_registry();

    }

    return rv;
}