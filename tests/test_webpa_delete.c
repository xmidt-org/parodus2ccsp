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

void test_deleteRow()
{
    char *reqPayload = "{\"row\":\"Device.WiFi.AccessPoint.10001.X_CISCO_COM_MacFilterTable.1.\",\"command\":\"DELETE_ROW\"}";
    char *transactionId = "aasfsdfgeh";
    char *resPayload = NULL;
    cJSON *response = NULL;
    money_trace_spans timeSpan;
    int count = 1;
    memset(&timeSpan, 0, sizeof(money_trace_spans));
    componentStruct_t **list = (componentStruct_t **) malloc(sizeof(componentStruct_t *)*count);
    list[0] = (componentStruct_t *) malloc(sizeof(componentStruct_t));
    list[0]->componentName = strdup(RDKB_WIFI_FULL_COMPONENT_NAME);
    list[0]->dbusPath = strdup(RDKB_WIFI_DBUS_PATH);

    will_return(get_global_components, list);
    will_return(get_global_component_size, count);
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
    assert_string_equal("Success", cJSON_GetObjectItem(response, "message")->valuestring);
    assert_int_equal(200, cJSON_GetObjectItem(response, "statusCode")->valueint);
    assert_non_null(response);
    cJSON_Delete(response);
}

void test_deleteRow_without_include_spans()
{
    char *reqPayload = "{\"row\":\"Device.NAT.PortMapping.1.\",\"command\":\"DELETE_ROW\"}";
    char *transactionId = "aasfsdfgeh";
    char *resPayload = NULL;
    cJSON *response = NULL;
    money_trace_spans timeSpan;
    int count = 1;
    memset(&timeSpan, 0, sizeof(money_trace_spans));
    componentStruct_t **list = (componentStruct_t **) malloc(sizeof(componentStruct_t *)*count);
    list[0] = (componentStruct_t *) malloc(sizeof(componentStruct_t));
    list[0]->componentName = strdup("com.ccsp.nat");
    list[0]->dbusPath = strdup("/com/ccsp/nat");

    will_return(get_global_components, list);
    will_return(get_global_component_size, count);
    expect_function_call(CcspBaseIf_discComponentSupportingNamespace);
    will_return(CcspBaseIf_discComponentSupportingNamespace, CCSP_SUCCESS);
    expect_function_call(free_componentStruct_t);
    expect_function_call(CcspBaseIf_DeleteTblRow);
    will_return(CcspBaseIf_DeleteTblRow, CCSP_SUCCESS);
    processRequest(reqPayload, transactionId, false, &resPayload, &timeSpan);
    WalInfo("resPayload : %s\n",resPayload);
    assert_int_equal(timeSpan.count, 0);
    assert_null(timeSpan.spans);
    assert_non_null(resPayload);
    response = cJSON_Parse(resPayload);
    assert_string_equal("Success", cJSON_GetObjectItem(response, "message")->valuestring);
    assert_int_equal(200, cJSON_GetObjectItem(response, "statusCode")->valueint);
    assert_non_null(response);
    cJSON_Delete(response);
}

void err_deleteRow_invalid_table_legth()
{
    char *reqPayload = "{\"row\":\"Device.WiFi.AccessPoint.10001.X_CISCO_COM_MacFilterTableDevice1DeviceNamesfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGAQWWEGYTEHERHAEGTWERHTBQR4WYTRsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGAQWWEGYTEHERHAEGTWERHTBQR4WYTRsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGAQWWEGYTEHERHAEGTWERHTBQR4WYTRsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGAQWWEGYTEHERHAEGTWERHTBQR4WYTRsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGAQWWEGYTEHERHAEGTWERHTBQR4WYTRsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGAQWWEGYTEHERHAEGTWERHTBQR4WYTRFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGAQWWEGYTEHERHAEGTWERHTBQR4WYTRsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGAQWWEGYTEHERHAEGTWERHTBQR4WYTRegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGAQWWEGYTEHERHAEGTWERHTBQR4WYTRegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGAQWWEGYTEHERHAEGTWERHTBQR4WYTRejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdferfddfffddfgffdddff.1.\",\"command\":\"DELETE_ROW\"}";
    char *transactionId = "aasfsdfgeh";
    char *resPayload = NULL;
    cJSON *response = NULL;
    money_trace_spans timeSpan;
    int count = 1;
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

void err_deleteRow_invalid_wifi_index()
{
    char *reqPayload = "{\"row\":\"Device.WiFi.AccessPoint.10011.X_CISCO_COM_MacFilterTable.1.\",\"command\":\"DELETE_ROW\"}";
    char *transactionId = "aasfsdfgeh";
    char *resPayload = NULL;
    cJSON *response = NULL;
    money_trace_spans timeSpan;
    int count = 1;
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

void err_deleteRow_invalid_radio_index()
{
    char *reqPayload = "{\"row\":\"Device.WiFi.Radio.10111.X_CISCO_COM_MacFilterTable.1.\",\"command\":\"DELETE_ROW\"}";
    char *transactionId = "aasfsdfgeh";
    char *resPayload = NULL;
    cJSON *response = NULL;
    money_trace_spans timeSpan;
    int count = 1;
    memset(&timeSpan, 0, sizeof(money_trace_spans));

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

void err_deleteRow_invalid_component()
{
    char *reqPayload = "{\"row\":\"Device.abcd.AccessPoint.10001.X_CISCO_COM_MacFilterTable.1.\",\"command\":\"DELETE_ROW\"}";
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
    assert_int_equal(timeSpan.count, 0);
    assert_null(timeSpan.spans);
    assert_non_null(resPayload);
    response = cJSON_Parse(resPayload);
    assert_int_equal(520, cJSON_GetObjectItem(response, "statusCode")->valueint);
    assert_non_null(response);
    cJSON_Delete(response);
}

void err_deleteRow_invalid_row()
{
    char *reqPayload = "{\"row\":\"Device.NAT.PortMapping.23.\",\"command\":\"DELETE_ROW\"}";
    char *transactionId = "aasfsdfgeh";
    char *resPayload = NULL;
    cJSON *response = NULL;
    money_trace_spans timeSpan;
    int count = 1;
    memset(&timeSpan, 0, sizeof(money_trace_spans));
    componentStruct_t **list = (componentStruct_t **) malloc(sizeof(componentStruct_t *)*count);
    list[0] = (componentStruct_t *) malloc(sizeof(componentStruct_t));
    list[0]->componentName = strdup("com.ccsp.nat");
    list[0]->dbusPath = strdup("/com/ccsp/nat");

    will_return(get_global_components, list);
    will_return(get_global_component_size, count);
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
        cmocka_unit_test(test_deleteRow),
        cmocka_unit_test(test_deleteRow_without_include_spans),
        cmocka_unit_test(err_deleteRow_invalid_table_legth),
        cmocka_unit_test(err_deleteRow_invalid_wifi_index),
        cmocka_unit_test(err_deleteRow_invalid_radio_index),
        cmocka_unit_test(err_deleteRow_invalid_component),
        cmocka_unit_test(err_deleteRow_invalid_row),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
