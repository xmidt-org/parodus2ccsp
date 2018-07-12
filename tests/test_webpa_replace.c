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
#include <string.h>

#include "../source/include/webpa_adapter.h"
#include "../source/broadband/include/webpa_internal.h"
#include <cimplog/cimplog.h>
#include <wdmp-c.h>
#include <cJSON.h>
#include <ccsp_base_api.h>
#include "mock_stack.h"

#define MAX_PARAMETER_LEN			512
/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/*                                   Mocks                                    */
/*----------------------------------------------------------------------------*/
int getWebpaParameterValues(char **parameterNames, int paramCount, int *val_size, parameterValStruct_t ***val)
{
    UNUSED(parameterNames); UNUSED(paramCount); UNUSED(val_size); UNUSED(val);
    return (int) mock();
}

int setWebpaParameterValues(parameterValStruct_t *val, int paramCount, char **faultParam )
{
    UNUSED(faultParam); UNUSED(paramCount); UNUSED(val);
    return (int) mock();
}

char *get_global_new_row()
{
    return (char *) mock();
}

WDMP_STATUS get_global_status()
{
    return (WDMP_STATUS) mock();
}
void addRowTable(char *objectName, TableData *list,char **retObject, money_trace_spans *timeSpan, WDMP_STATUS *retStatus)
{
    UNUSED(objectName); UNUSED(list);
    (*timeSpan).count = 1;
    (*timeSpan).spans = (money_trace_span *)malloc(sizeof(money_trace_span));
    (*timeSpan).spans[0].name = strdup("component name");
    (*timeSpan).spans[0].duration = 23;
    *retObject = get_global_new_row();
    *retStatus = get_global_status();
    function_called();
}
void deleteRowTable(char *object, money_trace_spans *timeSpan, WDMP_STATUS *retStatus)
{
    UNUSED(object); UNUSED(timeSpan); UNUSED(retStatus);
    function_called();
}

int deleteRow(char *object, money_trace_spans *timeSpan)
{
    UNUSED(object);
    (*timeSpan).count = 1;
    (*timeSpan).spans = (money_trace_span *)malloc(sizeof(money_trace_span));
    (*timeSpan).spans[0].name = strdup("component name");
    (*timeSpan).spans[0].duration = 20;
    function_called();
    return (int) mock();
}
/*----------------------------------------------------------------------------*/
/*                                   Tests                                    */
/*----------------------------------------------------------------------------*/

void test_replaceRow()
{
    char *reqPayload = "{\"rows\":{\"0\":{\"DeviceName\":\"Device1\",\"MacAddress\":\"12:2:3:5:11\"},\"1\":{\"DeviceName\":\"Device2\",\"MacAddress\":\"2:1:3:5:7\"} },\"table\" : \"Device.WiFi.AccessPoint.10001.X_CISCO_COM_MacFilterTable.\",\"command\":\"REPLACE_ROWS\"}";
    char *transactionId = "aasfsdfgeh";
    char *resPayload = NULL;
    cJSON *response = NULL;
    money_trace_spans timeSpan;
    int count = 1, paramCount = 2, i = 0;
    memset(&timeSpan, 0, sizeof(money_trace_spans));
    componentStruct_t **list = (componentStruct_t **) malloc(sizeof(componentStruct_t *)*count);
    list[0] = (componentStruct_t *) malloc(sizeof(componentStruct_t));
    list[0]->componentName = strdup(RDKB_WIFI_FULL_COMPONENT_NAME);
    list[0]->dbusPath = strdup(RDKB_WIFI_DBUS_PATH);
    
    componentStruct_t **list1 = (componentStruct_t **) malloc(sizeof(componentStruct_t *)*count);
    list1[0] = (componentStruct_t *) malloc(sizeof(componentStruct_t));
    list1[0]->componentName = strdup(RDKB_WIFI_FULL_COMPONENT_NAME);
    list1[0]->dbusPath = strdup(RDKB_WIFI_DBUS_PATH);

    char *names[MAX_PARAMETER_LEN] = {"Device.WiFi.AccessPoint.1.X_CISCO_COM_MacFilterTable.1.DeviceName", "Device.WiFi.AccessPoint.1.X_CISCO_COM_MacFilterTable.1.MacAddress"};
    char *values[MAX_PARAMETER_LEN] = {"Device1","12:2:3:5:11"};
    parameterValStruct_t **valueList = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*)*paramCount);
    for(i = 0; i<paramCount; i++)
    {
        valueList[i] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t));
        valueList[i][0].parameterName = strdup(names[i]);
        valueList[i][0].parameterValue = strdup(values[i]);
        valueList[i][0].type = ccsp_string;
    }

    parameterInfoStruct_t **info = (parameterInfoStruct_t **)malloc(sizeof(parameterInfoStruct_t *)*paramCount);
    for(i = 0; i<paramCount; i++)
    {
        info[i] = (parameterInfoStruct_t *) malloc(sizeof(parameterInfoStruct_t));
        info[i][0].parameterName = strdup(names[i]);
        info[i][0].writable = 1;
    }

    will_return(get_global_components, list);
    will_return(get_global_component_size, count);
    expect_function_call(CcspBaseIf_discComponentSupportingNamespace);
    will_return(CcspBaseIf_discComponentSupportingNamespace, CCSP_SUCCESS);
    will_return(get_global_values, valueList);
    will_return(get_global_parameters_count, paramCount);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterValues, size, 1);
    will_return(get_global_components, list1);
    will_return(get_global_component_size, count);
    expect_function_call(CcspBaseIf_discComponentSupportingNamespace);
    will_return(CcspBaseIf_discComponentSupportingNamespace, CCSP_SUCCESS);
    expect_function_call(CcspBaseIf_getParameterNames);
    will_return(CcspBaseIf_getParameterNames, CCSP_SUCCESS);
    will_return(get_global_parameters_count, paramCount);
    will_return(get_global_parameterInfo, info);
    expect_function_call(free_componentStruct_t);
    expect_function_call(free_parameterInfoStruct_t);
    expect_function_call(free_componentStruct_t);
    expect_function_call(free_parameterValStruct_t);
    expect_function_call(deleteRow);
    will_return(deleteRow, CCSP_SUCCESS);
    expect_function_call(addRowTable);
    will_return(get_global_new_row, strdup("Device.WiFi.AccessPoint.1.X_CISCO_COM_MacFilterTable.2."));
    will_return(get_global_status, WDMP_SUCCESS);
    expect_function_call(addRowTable);
    will_return(get_global_new_row, strdup("Device.WiFi.AccessPoint.1.X_CISCO_COM_MacFilterTable.3."));
    will_return(get_global_status, WDMP_SUCCESS);
    processRequest(reqPayload, transactionId, true, &resPayload, &timeSpan);
    WalInfo("resPayload : %s\n",resPayload);
    assert_true(timeSpan.count > 0);
    assert_non_null(timeSpan.spans);
    assert_non_null(resPayload);
    response = cJSON_Parse(resPayload);
    assert_string_equal("Success", cJSON_GetObjectItem(response, "message")->valuestring);
    assert_int_equal(200, cJSON_GetObjectItem(response, "statusCode")->valueint);
    assert_non_null(response);
    cJSON_Delete(response);
}

void test_replaceRow_empty_table()
{
    char *reqPayload = "{\"rows\":{},\"table\" : \"Device.WiFi.AccessPoint.10001.X_CISCO_COM_MacFilterTable.\",\"command\":\"REPLACE_ROWS\"}";
    char *transactionId = "aasfsdfgeh";
    char *resPayload = NULL;
    cJSON *response = NULL;
    money_trace_spans timeSpan;
    int count = 1, paramCount = 4, i = 0;
    memset(&timeSpan, 0, sizeof(money_trace_spans));
    componentStruct_t **list = (componentStruct_t **) malloc(sizeof(componentStruct_t *)*count);
    list[0] = (componentStruct_t *) malloc(sizeof(componentStruct_t));
    list[0]->componentName = strdup(RDKB_WIFI_FULL_COMPONENT_NAME);
    list[0]->dbusPath = strdup(RDKB_WIFI_DBUS_PATH);

    char *names[MAX_PARAMETER_LEN] = {"Device.WiFi.AccessPoint.1.X_CISCO_COM_MacFilterTable.2.DeviceName", "Device.WiFi.AccessPoint.1.X_CISCO_COM_MacFilterTable.2.MacAddress", "Device.WiFi.AccessPoint.1.X_CISCO_COM_MacFilterTable.3.DeviceName", "Device.WiFi.AccessPoint.1.X_CISCO_COM_MacFilterTable.3.MacAddress"};
    char *values[MAX_PARAMETER_LEN] = {"Device2","12:2:3:5:22", "Device3","12:2:3:5:33"};
    parameterValStruct_t **valueList = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*)*paramCount);
    for(i = 0; i<paramCount; i++)
    {
        valueList[i] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t));
        valueList[i][0].parameterName = strdup(names[i]);
        valueList[i][0].parameterValue = strdup(values[i]);
        valueList[i][0].type = ccsp_string;
    }

    will_return(get_global_components, list);
    will_return(get_global_component_size, count);
    expect_function_call(CcspBaseIf_discComponentSupportingNamespace);
    will_return(CcspBaseIf_discComponentSupportingNamespace, CCSP_SUCCESS);
    will_return(get_global_values, valueList);
    will_return(get_global_parameters_count, paramCount);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterValues, size, 1);
    expect_function_call(free_componentStruct_t);
    expect_function_call(free_parameterValStruct_t);
    expect_function_call(deleteRow);
    will_return(deleteRow, CCSP_SUCCESS);
    expect_function_call(deleteRow);
    will_return(deleteRow, CCSP_SUCCESS);
    processRequest(reqPayload, transactionId, true, &resPayload, &timeSpan);
    WalInfo("resPayload : %s\n",resPayload);
    assert_true(timeSpan.count > 0);
    assert_non_null(timeSpan.spans);
    assert_non_null(resPayload);
    response = cJSON_Parse(resPayload);
    assert_string_equal("Success", cJSON_GetObjectItem(response, "message")->valuestring);
    assert_int_equal(200, cJSON_GetObjectItem(response, "statusCode")->valueint);
    assert_non_null(response);
    cJSON_Delete(response);
}

void test_replaceRow_to_empty_table()
{
    char *reqPayload = "{\"rows\":{\"0\":{\"DeviceName\":\"Device1\",\"MacAddress\":\"12:2:3:5:11\"} },\"table\" : \"Device.WiFi.AccessPoint.10001.X_CISCO_COM_MacFilterTable.\",\"command\":\"REPLACE_ROWS\"}";
    char *transactionId = "aasfsdfgeh";
    char *resPayload = NULL;
    cJSON *response = NULL;
    money_trace_spans timeSpan;
    int count = 1, paramCount = 2, i = 0;
    memset(&timeSpan, 0, sizeof(money_trace_spans));
    componentStruct_t **list = (componentStruct_t **) malloc(sizeof(componentStruct_t *)*count);
    list[0] = (componentStruct_t *) malloc(sizeof(componentStruct_t));
    list[0]->componentName = strdup(RDKB_WIFI_FULL_COMPONENT_NAME);
    list[0]->dbusPath = strdup(RDKB_WIFI_DBUS_PATH);

    will_return(get_global_components, list);
    will_return(get_global_component_size, count);
    expect_function_call(CcspBaseIf_discComponentSupportingNamespace);
    will_return(CcspBaseIf_discComponentSupportingNamespace, CCSP_SUCCESS);
    will_return(get_global_values, NULL);
    will_return(get_global_parameters_count, 0);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterValues, size, 1);
    expect_function_call(free_componentStruct_t);
    expect_function_call(free_parameterValStruct_t);
    expect_function_call(addRowTable);
    will_return(get_global_new_row, strdup("Device.WiFi.AccessPoint.1.X_CISCO_COM_MacFilterTable.5."));
    will_return(get_global_status, WDMP_SUCCESS);
    processRequest(reqPayload, transactionId, true, &resPayload, &timeSpan);
    WalInfo("resPayload : %s\n",resPayload);
    assert_true(timeSpan.count > 0);
    assert_non_null(timeSpan.spans);
    assert_non_null(resPayload);
    response = cJSON_Parse(resPayload);
    assert_string_equal("Success", cJSON_GetObjectItem(response, "message")->valuestring);
    assert_int_equal(200, cJSON_GetObjectItem(response, "statusCode")->valueint);
    assert_non_null(response);
    cJSON_Delete(response);
}

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_replaceRow),
        cmocka_unit_test(test_replaceRow_empty_table),
        cmocka_unit_test(test_replaceRow_to_empty_table),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
