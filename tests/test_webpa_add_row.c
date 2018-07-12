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

void replaceTable(char *objectName,TableData * list,unsigned int paramcount, money_trace_spans *timeSpan, WDMP_STATUS *retStatus)
{
    UNUSED(objectName); UNUSED(list); UNUSED(paramcount); UNUSED(retStatus);
    function_called();
}
/*----------------------------------------------------------------------------*/
/*                                   Tests                                    */
/*----------------------------------------------------------------------------*/

void test_addRow()
{
    char *reqPayload = "{\"row\":{\"DeviceName\":\"Device1\",\"MacAddress\":\"12:2:3:5:11\"},\"table\":\"Device.WiFi.AccessPoint.10001.X_CISCO_COM_MacFilterTable.\",\"command\":\"ADD_ROW\"}";
    char *transactionId = "aasfsdfgeh";
    char *resPayload = NULL;
    cJSON *response = NULL;
    money_trace_spans timeSpan;
    int compCount = 1, i = 0, paramCount = 2;
    memset(&timeSpan, 0, sizeof(money_trace_spans));
    componentStruct_t **list = (componentStruct_t **) malloc(sizeof(componentStruct_t *)*compCount);
    list[0] = (componentStruct_t *) malloc(sizeof(componentStruct_t));
    list[0]->componentName = strdup(RDKB_WIFI_FULL_COMPONENT_NAME);
    list[0]->dbusPath = strdup(RDKB_WIFI_DBUS_PATH);
    char *names[MAX_PARAMETER_LEN] = {"Device.WiFi.AccessPoint.10001.X_CISCO_COM_MacFilterTable.1.DeviceName", "Device.WiFi.AccessPoint.10001.X_CISCO_COM_MacFilterTable.1.MacAddress"};
    char *values[MAX_PARAMETER_LEN] = {"Device1","12:2:3:5:11", "1"};
    parameterValStruct_t **valueList = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*)*paramCount);
    for(i = 0; i<paramCount; i++)
    {
        valueList[i] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t));
        valueList[i][0].parameterName = strdup(names[i]);
        valueList[i][0].parameterValue = strdup(values[i]);
        valueList[i][0].type = ccsp_string;
    }

    will_return(get_global_components, list);
    will_return(get_global_component_size, compCount);
    expect_function_call(CcspBaseIf_discComponentSupportingNamespace);
    will_return(CcspBaseIf_discComponentSupportingNamespace, CCSP_SUCCESS);
    expect_function_call(free_componentStruct_t);
    expect_function_call(CcspBaseIf_AddTblRow);
    will_return(CcspBaseIf_AddTblRow, CCSP_SUCCESS);
    will_return(get_global_row_id, 1);
    will_return(get_global_values, valueList);
    will_return(get_global_parameters_count, paramCount);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterValues, size, paramCount);
    expect_function_call(free_parameterValStruct_t);
    expect_function_call(CcspBaseIf_setParameterValues);
    will_return(CcspBaseIf_setParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_setParameterValues, size, paramCount);
    will_return(get_global_faultParam, NULL);
    processRequest(reqPayload, transactionId, true, &resPayload, &timeSpan);
    WalInfo("resPayload : %s\n",resPayload);
    assert_true(timeSpan.count > 0);
    assert_non_null(timeSpan.spans);
    assert_non_null(resPayload);
    response = cJSON_Parse(resPayload);
    assert_string_equal("Success", cJSON_GetObjectItem(response, "message")->valuestring);
    assert_int_equal(201, cJSON_GetObjectItem(response, "statusCode")->valueint);
    assert_non_null(response);
    cJSON_Delete(response);
}

void test_addRow_without_include_spans()
{
    char *reqPayload = "{\"row\":{\"ExternalPort\":\"2\",\"ExternalPortEndRange\":\"3\",\"InternalClient\":\"10.0.0.3\",\"Protocol\":\"UDP\",\"Description\":\"test_POST\"},\"table\":\"Device.NAT.PortMapping.\",\"command\":\"ADD_ROW\"}";
    char *transactionId = "aasfsdfgeh";
    char *resPayload = NULL;
    cJSON *response = NULL;
    money_trace_spans timeSpan;
    int compCount = 1, i = 0, paramCount = 5;
    memset(&timeSpan, 0, sizeof(money_trace_spans));
    componentStruct_t **list = (componentStruct_t **) malloc(sizeof(componentStruct_t *)*compCount);
    list[0] = (componentStruct_t *) malloc(sizeof(componentStruct_t));
    list[0]->componentName = strdup("com.ccsp.nat");
    list[0]->dbusPath = strdup("/com/ccsp/nat");
    char *names[MAX_PARAMETER_LEN] = {"Device.NAT.PortMapping.1.ExternalPort", "Device.NAT.PortMapping.1.ExternalPortEndRange","Device.NAT.PortMapping.1.InternalClient", "Device.NAT.PortMapping.1.Protocol","Device.NAT.PortMapping.1.Description"};
    char *values[MAX_PARAMETER_LEN] = {"2","3", "10.0.0.3", "UDP", "test_POST"};
    parameterValStruct_t **valueList = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*)*paramCount);
    for(i = 0; i<paramCount; i++)
    {
        valueList[i] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t));
        valueList[i][0].parameterName = strdup(names[i]);
        valueList[i][0].parameterValue = strdup(values[i]);
        valueList[i][0].type = ccsp_string;
    }

    will_return(get_global_components, list);
    will_return(get_global_component_size, compCount);
    expect_function_call(CcspBaseIf_discComponentSupportingNamespace);
    will_return(CcspBaseIf_discComponentSupportingNamespace, CCSP_SUCCESS);
    expect_function_call(free_componentStruct_t);
    expect_function_call(CcspBaseIf_AddTblRow);
    will_return(CcspBaseIf_AddTblRow, CCSP_SUCCESS);
    will_return(get_global_row_id, 1);
    will_return(get_global_values, valueList);
    will_return(get_global_parameters_count, paramCount);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterValues, size, paramCount);
    expect_function_call(free_parameterValStruct_t);
    expect_function_call(CcspBaseIf_setParameterValues);
    will_return(CcspBaseIf_setParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_setParameterValues, size, paramCount);
    will_return(get_global_faultParam, NULL);
    processRequest(reqPayload, transactionId, false, &resPayload, &timeSpan);
    WalInfo("resPayload : %s\n",resPayload);
    assert_true(timeSpan.count == 0);
    assert_null(timeSpan.spans);
    assert_non_null(resPayload);
    response = cJSON_Parse(resPayload);
    assert_string_equal("Success", cJSON_GetObjectItem(response, "message")->valuestring);
    assert_int_equal(201, cJSON_GetObjectItem(response, "statusCode")->valueint);
    assert_non_null(response);
    cJSON_Delete(response);
}

void err_addRow_invalid_table_length()
{
    char *reqPayload = "{\"row\":{\"DeviceName\":\"Device1\",\"MacAddress\":\"12:2:3:5:11\"},\"table\":\"Device.WiFi.AccessPoint.10001.sfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGAQWWEGYTEHERHAEGTWERHTBQR4WYTRsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGAQWWEGYTEHERHAEGTWERHTBQR4WYTRsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGAQWWEGYTEHERHAEGTWERHTBQR4WYTRsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGAQWWEGYTEHERHAEGTWERHTBQR4WYTRsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGAQWWEGYTEHERHAEGTWERHTBQR4WYTRsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGAQWWEGYTEHERHAEGTWERHTBQR4WYTRFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGAQWWEGYTEHERHAEGTWERHTBQR4WYTRsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGAQWWEGYTEHERHAEGTWERHTBQR4WYTRegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGAQWWEGYTEHERHAEGTWERHTBQR4WYTRegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGAQWWEGYTEHERHAEGTWERHTBQR4WYTRejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdferfddfffddfgffdddff.\",\"command\":\"ADD_ROW\"}";
    char *transactionId = "aasfsdfgeh";
    char *resPayload = NULL;
    cJSON *response = NULL;
    money_trace_spans timeSpan;
    int compCount = 0;
    memset(&timeSpan, 0, sizeof(money_trace_spans));

    processRequest(reqPayload, transactionId, true, &resPayload, &timeSpan);
    WalInfo("resPayload : %s\n",resPayload);
    assert_true(timeSpan.count > 0);
    assert_non_null(timeSpan.spans);
    assert_non_null(resPayload);
    response = cJSON_Parse(resPayload);
    assert_int_equal(520, cJSON_GetObjectItem(response, "statusCode")->valueint);
    assert_non_null(response);
    cJSON_Delete(response);
}

void err_addRow_invalid_parameter_name_length()
{
    char *reqPayload = "{\"row\":{\"DeviceNamesfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGAQWWEGYTEHERHAEGTWERHTBQR4WYTRsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGAQWWEGYTEHERHAEGTWERHTBQR4WYTRsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGAQWWEGYTEHERHAEGTWERHTBQR4WYTRsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGAQWWEGYTEHERHAEGTWERHTBQR4WYTRsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGAQWWEGYTEHERHAEGTWERHTBQR4WYTRsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGAQWWEGYTEHERHAEGTWERHTBQR4WYTRFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGAQWWEGYTEHERHAEGTWERHTBQR4WYTRsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGAQWWEGYTEHERHAEGTWERHTBQR4WYTRegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGAQWWEGYTEHERHAEGTWERHTBQR4WYTRegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGAQWWEGYTEHERHAEGTWERHTBQR4WYTRejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdferfddfffddfgffdddff\":\"Device1\",\"MacAddress\":\"12:2:3:5:11\"},\"table\":\"Device.WiFi.AccessPoint.10011.X_CISCO_COM_MacFilterTable.\",\"command\":\"ADD_ROW\"}";
    char *transactionId = "aasfsdfgeh";
    char *resPayload = NULL;
    cJSON *response = NULL;
    money_trace_spans timeSpan;
    int compCount = 0;
    memset(&timeSpan, 0, sizeof(money_trace_spans));

    processRequest(reqPayload, transactionId, true, &resPayload, &timeSpan);
    WalInfo("resPayload : %s\n",resPayload);
    assert_true(timeSpan.count > 0);
    assert_non_null(timeSpan.spans);
    assert_non_null(resPayload);
    response = cJSON_Parse(resPayload);
    assert_int_equal(520, cJSON_GetObjectItem(response, "statusCode")->valueint);
    assert_non_null(response);
    cJSON_Delete(response);
}

void err_addRow_invalid_parameter_value_length()
{
    char *reqPayload = "{\"row\":{\"DeviceName\":\"Device1DeviceNamesfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGAQWWEGYTEHERHAEGTWERHTBQR4WYTRsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGAQWWEGYTEHERHAEGTWERHTBQR4WYTRsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGAQWWEGYTEHERHAEGTWERHTBQR4WYTRsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGAQWWEGYTEHERHAEGTWERHTBQR4WYTRsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGAQWWEGYTEHERHAEGTWERHTBQR4WYTRsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGAQWWEGYTEHERHAEGTWERHTBQR4WYTRFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGAQWWEGYTEHERHAEGTWERHTBQR4WYTRsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGAQWWEGYTEHERHAEGTWERHTBQR4WYTRegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGAQWWEGYTEHERHAEGTWERHTBQR4WYTRegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGAQWWEGYTEHERHAEGTWERHTBQR4WYTRejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdferfddfffddfgffdddff\",\"MacAddress\":\"12:2:3:5:11\"},\"table\":\"Device.WiFi.AccessPoint.10011.X_CISCO_COM_MacFilterTable.\",\"command\":\"ADD_ROW\"}";
    char *transactionId = "aasfsdfgeh";
    char *resPayload = NULL;
    cJSON *response = NULL;
    money_trace_spans timeSpan;
    int compCount = 0;
    memset(&timeSpan, 0, sizeof(money_trace_spans));

    processRequest(reqPayload, transactionId, true, &resPayload, &timeSpan);
    WalInfo("resPayload : %s\n",resPayload);
    assert_true(timeSpan.count > 0);
    assert_non_null(timeSpan.spans);
    assert_non_null(resPayload);
    response = cJSON_Parse(resPayload);
    assert_int_equal(520, cJSON_GetObjectItem(response, "statusCode")->valueint);
    assert_non_null(response);
    cJSON_Delete(response);
}

void err_addRow_invalid_wifi_index()
{
    char *reqPayload = "{\"row\":{\"DeviceName\":\"Device1\",\"MacAddress\":\"12:2:3:5:11\"},\"table\":\"Device.WiFi.AccessPoint.10011.X_CISCO_COM_MacFilterTable.\",\"command\":\"ADD_ROW\"}";
    char *transactionId = "aasfsdfgeh";
    char *resPayload = NULL;
    cJSON *response = NULL;
    money_trace_spans timeSpan;
    int compCount = 0;
    memset(&timeSpan, 0, sizeof(money_trace_spans));

    processRequest(reqPayload, transactionId, true, &resPayload, &timeSpan);
    WalInfo("resPayload : %s\n",resPayload);
    assert_true(timeSpan.count > 0);
    assert_non_null(timeSpan.spans);
    assert_non_null(resPayload);
    response = cJSON_Parse(resPayload);
    assert_int_equal(520, cJSON_GetObjectItem(response, "statusCode")->valueint);
    assert_non_null(response);
    cJSON_Delete(response);
}

void err_addRow_invalid_radio_index()
{
    char *reqPayload = "{\"row\":{\"Name\":\"Device1\",\"Enable\":\"true\"},\"table\":\"Device.WiFi.Radio.10111.\",\"command\":\"ADD_ROW\"}";
    char *transactionId = "aasfsdfgeh";
    char *resPayload = NULL;
    cJSON *response = NULL;
    money_trace_spans timeSpan;
    int compCount = 0;
    memset(&timeSpan, 0, sizeof(money_trace_spans));

    processRequest(reqPayload, transactionId, true, &resPayload, &timeSpan);
    WalInfo("resPayload : %s\n",resPayload);
    assert_true(timeSpan.count > 0);
    assert_non_null(timeSpan.spans);
    assert_non_null(resPayload);
    response = cJSON_Parse(resPayload);
    assert_int_equal(520, cJSON_GetObjectItem(response, "statusCode")->valueint);
    assert_non_null(response);
    cJSON_Delete(response);
}

void err_addRow_invalid_component()
{
    char *reqPayload = "{\"row\":{\"DeviceName\":\"Device1\",\"MacAddress\":\"12:2:3:5:11\"},\"table\":\"Device.abc.AccessPoint.10011.X_CISCO_COM_MacFilterTable.\",\"command\":\"ADD_ROW\"}";
    char *transactionId = "aasfsdfgeh";
    char *resPayload = NULL;
    cJSON *response = NULL;
    money_trace_spans timeSpan;
    memset(&timeSpan, 0, sizeof(money_trace_spans));

    will_return(get_global_components, NULL);
    will_return(get_global_component_size, 0);
    expect_function_call(CcspBaseIf_discComponentSupportingNamespace);
    will_return(CcspBaseIf_discComponentSupportingNamespace, CCSP_CR_ERR_UNSUPPORTED_NAMESPACE);
    expect_function_call(free_componentStruct_t);
    processRequest(reqPayload, transactionId, false, &resPayload, &timeSpan);
    WalInfo("resPayload : %s\n",resPayload);
    assert_true(timeSpan.count == 0);
    assert_null(timeSpan.spans);
    assert_non_null(resPayload);
    response = cJSON_Parse(resPayload);
    assert_int_equal(520, cJSON_GetObjectItem(response, "statusCode")->valueint);
    assert_non_null(response);
    cJSON_Delete(response);
}

void err_addRow_invalid_row()
{
    char *reqPayload = "{\"row\":{\"ExternalPort\":\"2\",\"ExternalPortEndRange\":\"3\",\"InternalClient\":\"10.0.0.3\",\"Protocol\":\"UDP\",\"Description\":\"test_POST\"},\"table\":\"Device.NAT.PortMapping.\",\"command\":\"ADD_ROW\"}";
    char *transactionId = "aasfsdfgeh";
    char *resPayload = NULL;
    cJSON *response = NULL;
    money_trace_spans timeSpan;
    int compCount = 1;
    memset(&timeSpan, 0, sizeof(money_trace_spans));
    componentStruct_t **list = (componentStruct_t **) malloc(sizeof(componentStruct_t *)*compCount);
    list[0] = (componentStruct_t *) malloc(sizeof(componentStruct_t));
    list[0]->componentName = strdup("com.ccsp.nat");
    list[0]->dbusPath = strdup("/com/ccsp/nat");

    will_return(get_global_components, list);
    will_return(get_global_component_size, compCount);
    expect_function_call(CcspBaseIf_discComponentSupportingNamespace);
    will_return(CcspBaseIf_discComponentSupportingNamespace, CCSP_SUCCESS);
    expect_function_call(free_componentStruct_t);
    expect_function_call(CcspBaseIf_AddTblRow);
    will_return(CcspBaseIf_AddTblRow, CCSP_FAILURE);
    will_return(get_global_row_id, 0);
    processRequest(reqPayload, transactionId, true, &resPayload, &timeSpan);
    WalInfo("resPayload : %s\n",resPayload);
    assert_true(timeSpan.count > 0);
    assert_non_null(timeSpan.spans);
    assert_non_null(resPayload);
    response = cJSON_Parse(resPayload);
    assert_int_equal(520, cJSON_GetObjectItem(response, "statusCode")->valueint);
    assert_non_null(response);
    cJSON_Delete(response);
}

void err_addRow_invalid_get()
{
    char *reqPayload = "{\"row\":{\"ExternalPort\":\"2\",\"ExternalPortEndRange\":\"3\",\"InternalClient\":\"10.0.0.3\",\"Protocol\":\"UDP\",\"Description\":\"test_POST\"},\"table\":\"Device.NAT.PortMapping.\",\"command\":\"ADD_ROW\"}";
    char *transactionId = "aasfsdfgeh";
    char *resPayload = NULL;
    cJSON *response = NULL;
    money_trace_spans timeSpan;
    int compCount = 1, paramCount = 5, i = 0;
    memset(&timeSpan, 0, sizeof(money_trace_spans));
    componentStruct_t **list = (componentStruct_t **) malloc(sizeof(componentStruct_t *)*compCount);
    list[0] = (componentStruct_t *) malloc(sizeof(componentStruct_t));
    list[0]->componentName = strdup("com.ccsp.nat");
    list[0]->dbusPath = strdup("/com/ccsp/nat");

    componentStruct_t **list1 = (componentStruct_t **) malloc(sizeof(componentStruct_t *)*compCount);
    list1[0] = (componentStruct_t *) malloc(sizeof(componentStruct_t));
    list1[0]->componentName = strdup("com.ccsp.nat");
    list1[0]->dbusPath = strdup("/com/ccsp/nat");

    will_return(get_global_components, list);
    will_return(get_global_component_size, compCount);
    expect_function_call(CcspBaseIf_discComponentSupportingNamespace);
    will_return(CcspBaseIf_discComponentSupportingNamespace, CCSP_SUCCESS);
    expect_function_call(free_componentStruct_t);
    expect_function_call(CcspBaseIf_AddTblRow);
    will_return(CcspBaseIf_AddTblRow, CCSP_SUCCESS);
    will_return(get_global_row_id, 2);
    will_return(get_global_values, NULL);
    will_return(get_global_parameters_count, 0);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_FAILURE);
    expect_value(CcspBaseIf_getParameterValues, size, paramCount);
    will_return(get_global_components, list1);
    will_return(get_global_component_size, compCount);
    expect_function_call(CcspBaseIf_discComponentSupportingNamespace);
    will_return(CcspBaseIf_discComponentSupportingNamespace, CCSP_SUCCESS);
    expect_function_call(free_componentStruct_t);
    expect_function_call(CcspBaseIf_DeleteTblRow);
    will_return(CcspBaseIf_DeleteTblRow, CCSP_SUCCESS);
    processRequest(reqPayload, transactionId, true, &resPayload, &timeSpan);
    WalInfo("resPayload : %s\n",resPayload);
    assert_true(timeSpan.count > 0);
    assert_non_null(timeSpan.spans);
    assert_non_null(resPayload);
    response = cJSON_Parse(resPayload);
    assert_int_equal(520, cJSON_GetObjectItem(response, "statusCode")->valueint);
    assert_non_null(response);
    cJSON_Delete(response);
}

void err_addRow_invalid_update()
{
    char *reqPayload = "{\"row\":{\"ExternalPort\":\"2\",\"ExternalPortEndRange\":\"3\",\"InternalClient\":\"10.0.0.3\",\"Protocol\":\"UDP\",\"Description\":\"test_POST\"},\"table\":\"Device.NAT.PortMapping.\",\"command\":\"ADD_ROW\"}";
    char *transactionId = "aasfsdfgeh";
    char *resPayload = NULL;
    cJSON *response = NULL;
    money_trace_spans timeSpan;
    int compCount = 1, paramCount = 5, i = 0;
    memset(&timeSpan, 0, sizeof(money_trace_spans));
    componentStruct_t **list = (componentStruct_t **) malloc(sizeof(componentStruct_t *)*compCount);
    list[0] = (componentStruct_t *) malloc(sizeof(componentStruct_t));
    list[0]->componentName = strdup("com.ccsp.nat");
    list[0]->dbusPath = strdup("/com/ccsp/nat");

    componentStruct_t **list1 = (componentStruct_t **) malloc(sizeof(componentStruct_t *)*compCount);
    list1[0] = (componentStruct_t *) malloc(sizeof(componentStruct_t));
    list1[0]->componentName = strdup("com.ccsp.nat");
    list1[0]->dbusPath = strdup("/com/ccsp/nat");

    char *names[MAX_PARAMETER_LEN] = {"Device.NAT.PortMapping.2.ExternalPort", "Device.NAT.PortMapping.2.ExternalPortEndRange","Device.NAT.PortMapping.2.InternalClient", "Device.NAT.PortMapping.2.Protoco","Device.NAT.PortMapping.2.Description"};
    char *values[MAX_PARAMETER_LEN] = {"2","3", "10.0.0.3", "UDP", "test_POST"};
    parameterValStruct_t **valueList = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*)*paramCount);
    for(i = 0; i<paramCount; i++)
    {
        valueList[i] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t));
        valueList[i][0].parameterName = strdup(names[i]);
        valueList[i][0].parameterValue = strdup(values[i]);
        valueList[i][0].type = ccsp_string;
    }

    will_return(get_global_components, list);
    will_return(get_global_component_size, compCount);
    expect_function_call(CcspBaseIf_discComponentSupportingNamespace);
    will_return(CcspBaseIf_discComponentSupportingNamespace, CCSP_SUCCESS);
    expect_function_call(free_componentStruct_t);
    expect_function_call(CcspBaseIf_AddTblRow);
    will_return(CcspBaseIf_AddTblRow, CCSP_SUCCESS);
    will_return(get_global_row_id, 2);
    will_return(get_global_values, valueList);
    will_return(get_global_parameters_count, paramCount);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterValues, size, paramCount);
    expect_function_call(free_parameterValStruct_t);
    expect_function_call(CcspBaseIf_setParameterValues);
    will_return(CcspBaseIf_setParameterValues, CCSP_FAILURE);
    expect_value(CcspBaseIf_setParameterValues, size, paramCount);
    will_return(get_global_faultParam, strdup(names[3]));
    will_return(get_global_components, list1);
    will_return(get_global_component_size, compCount);
    expect_function_call(CcspBaseIf_discComponentSupportingNamespace);
    will_return(CcspBaseIf_discComponentSupportingNamespace, CCSP_SUCCESS);
    expect_function_call(free_componentStruct_t);
    expect_function_call(CcspBaseIf_DeleteTblRow);
    will_return(CcspBaseIf_DeleteTblRow, CCSP_SUCCESS);
    processRequest(reqPayload, transactionId, true, &resPayload, &timeSpan);
    WalInfo("resPayload : %s\n",resPayload);
    assert_true(timeSpan.count > 0);
    assert_non_null(timeSpan.spans);
    assert_non_null(resPayload);
    response = cJSON_Parse(resPayload);
    assert_int_equal(520, cJSON_GetObjectItem(response, "statusCode")->valueint);
    assert_non_null(response);
    cJSON_Delete(response);
}

void err_addRow_invalid_delete_parameter()
{
    char *reqPayload = "{\"row\":{\"ExternalPort\":\"2\",\"ExternalPortEndRange\":\"3\",\"InternalClient\":\"10.0.0.3\",\"Protocol\":\"UDP\",\"Description\":\"test_POST\"},\"table\":\"Device.NAT.PortMapping.\",\"command\":\"ADD_ROW\"}";
    char *transactionId = "aasfsdfgeh";
    char *resPayload = NULL;
    cJSON *response = NULL;
    money_trace_spans timeSpan;
    int compCount = 1, paramCount = 5, i = 0;
    memset(&timeSpan, 0, sizeof(money_trace_spans));
    componentStruct_t **list = (componentStruct_t **) malloc(sizeof(componentStruct_t *)*compCount);
    list[0] = (componentStruct_t *) malloc(sizeof(componentStruct_t));
    list[0]->componentName = strdup("com.ccsp.nat");
    list[0]->dbusPath = strdup("/com/ccsp/nat");

    char *names[MAX_PARAMETER_LEN] = {"Device.NAT.PortMapping.2.ExternalPort", "Device.NAT.PortMapping.2.ExternalPortEndRange","Device.NAT.PortMapping.2.InternalClient", "Device.NAT.PortMapping.2.Protoco","Device.NAT.PortMapping.2.Description"};
    char *values[MAX_PARAMETER_LEN] = {"2","3", "10.0.0.3", "UDP", "test_POST"};
    parameterValStruct_t **valueList = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*)*paramCount);
    for(i = 0; i<paramCount; i++)
    {
        valueList[i] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t));
        valueList[i][0].parameterName = strdup(names[i]);
        valueList[i][0].parameterValue = strdup(values[i]);
        valueList[i][0].type = ccsp_string;
    }

    will_return(get_global_components, list);
    will_return(get_global_component_size, compCount);
    expect_function_call(CcspBaseIf_discComponentSupportingNamespace);
    will_return(CcspBaseIf_discComponentSupportingNamespace, CCSP_SUCCESS);
    expect_function_call(free_componentStruct_t);
    expect_function_call(CcspBaseIf_AddTblRow);
    will_return(CcspBaseIf_AddTblRow, CCSP_SUCCESS);
    will_return(get_global_row_id, 2);
    will_return(get_global_values, valueList);
    will_return(get_global_parameters_count, paramCount);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterValues, size, paramCount);
    expect_function_call(free_parameterValStruct_t);
    expect_function_call(CcspBaseIf_setParameterValues);
    will_return(CcspBaseIf_setParameterValues, CCSP_FAILURE);
    expect_value(CcspBaseIf_setParameterValues, size, paramCount);
    will_return(get_global_faultParam, strdup(names[3]));
    will_return(get_global_components, NULL);
    will_return(get_global_component_size, 0);
    expect_function_call(CcspBaseIf_discComponentSupportingNamespace);
    will_return(CcspBaseIf_discComponentSupportingNamespace, CCSP_CR_ERR_UNSUPPORTED_NAMESPACE);
    expect_function_call(free_componentStruct_t);
    processRequest(reqPayload, transactionId, true, &resPayload, &timeSpan);
    WalInfo("resPayload : %s\n",resPayload);
    assert_true(timeSpan.count > 0);
    assert_non_null(timeSpan.spans);
    assert_non_null(resPayload);
    response = cJSON_Parse(resPayload);
    assert_int_equal(520, cJSON_GetObjectItem(response, "statusCode")->valueint);
    assert_non_null(response);
    cJSON_Delete(response);
}

void err_addRow_invalid_delete()
{
    char *reqPayload = "{\"row\":{\"ExternalPort\":\"2\",\"ExternalPortEndRange\":\"3\",\"InternalClient\":\"10.0.0.3\",\"Protocol\":\"UDP\",\"Description\":\"test_POST\"},\"table\":\"Device.NAT.PortMapping.\",\"command\":\"ADD_ROW\"}";
    char *transactionId = "aasfsdfgeh";
    char *resPayload = NULL;
    cJSON *response = NULL;
    money_trace_spans timeSpan;
    int compCount = 1, paramCount = 5, i = 0;
    memset(&timeSpan, 0, sizeof(money_trace_spans));
    componentStruct_t **list = (componentStruct_t **) malloc(sizeof(componentStruct_t *)*compCount);
    list[0] = (componentStruct_t *) malloc(sizeof(componentStruct_t));
    list[0]->componentName = strdup("com.ccsp.nat");
    list[0]->dbusPath = strdup("/com/ccsp/nat");

    componentStruct_t **list1 = (componentStruct_t **) malloc(sizeof(componentStruct_t *)*compCount);
    list1[0] = (componentStruct_t *) malloc(sizeof(componentStruct_t));
    list1[0]->componentName = strdup("com.ccsp.nat");
    list1[0]->dbusPath = strdup("/com/ccsp/nat");

    char *names[MAX_PARAMETER_LEN] = {"Device.NAT.PortMapping.2.ExternalPort", "Device.NAT.PortMapping.2.ExternalPortEndRange","Device.NAT.PortMapping.2.InternalClient", "Device.NAT.PortMapping.2.Protoco","Device.NAT.PortMapping.2.Description"};
    char *values[MAX_PARAMETER_LEN] = {"2","3", "10.0.0.3", "UDP", "test_POST"};
    parameterValStruct_t **valueList = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*)*paramCount);
    for(i = 0; i<paramCount; i++)
    {
        valueList[i] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t));
        valueList[i][0].parameterName = strdup(names[i]);
        valueList[i][0].parameterValue = strdup(values[i]);
        valueList[i][0].type = ccsp_string;
    }

    will_return(get_global_components, list);
    will_return(get_global_component_size, compCount);
    expect_function_call(CcspBaseIf_discComponentSupportingNamespace);
    will_return(CcspBaseIf_discComponentSupportingNamespace, CCSP_SUCCESS);
    expect_function_call(free_componentStruct_t);
    expect_function_call(CcspBaseIf_AddTblRow);
    will_return(CcspBaseIf_AddTblRow, CCSP_SUCCESS);
    will_return(get_global_row_id, 2);
    will_return(get_global_values, valueList);
    will_return(get_global_parameters_count, paramCount);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterValues, size, paramCount);
    expect_function_call(free_parameterValStruct_t);
    expect_function_call(CcspBaseIf_setParameterValues);
    will_return(CcspBaseIf_setParameterValues, CCSP_FAILURE);
    expect_value(CcspBaseIf_setParameterValues, size, paramCount);
    will_return(get_global_faultParam, strdup(names[3]));
    will_return(get_global_components, list1);
    will_return(get_global_component_size, compCount);
    expect_function_call(CcspBaseIf_discComponentSupportingNamespace);
    will_return(CcspBaseIf_discComponentSupportingNamespace, CCSP_SUCCESS);
    expect_function_call(free_componentStruct_t);
    expect_function_call(CcspBaseIf_DeleteTblRow);
    will_return(CcspBaseIf_DeleteTblRow, CCSP_FAILURE);
    processRequest(reqPayload, transactionId, true, &resPayload, &timeSpan);
    WalInfo("resPayload : %s\n",resPayload);
    assert_true(timeSpan.count > 0);
    assert_non_null(timeSpan.spans);
    assert_non_null(resPayload);
    response = cJSON_Parse(resPayload);
    assert_int_equal(520, cJSON_GetObjectItem(response, "statusCode")->valueint);
    assert_non_null(response);
    cJSON_Delete(response);
}
/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_addRow),
        cmocka_unit_test(test_addRow_without_include_spans),
        cmocka_unit_test(err_addRow_invalid_table_length),
        cmocka_unit_test(err_addRow_invalid_parameter_name_length),
        cmocka_unit_test(err_addRow_invalid_parameter_value_length),
        cmocka_unit_test(err_addRow_invalid_wifi_index),
        cmocka_unit_test(err_addRow_invalid_radio_index),
        cmocka_unit_test(err_addRow_invalid_component),
        cmocka_unit_test(err_addRow_invalid_row),
        cmocka_unit_test(err_addRow_invalid_get),
        cmocka_unit_test(err_addRow_invalid_update),
        cmocka_unit_test(err_addRow_invalid_delete_parameter),
        cmocka_unit_test(err_addRow_invalid_delete)
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
