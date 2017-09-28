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
extern BOOL applySettingsFlag;
/*----------------------------------------------------------------------------*/
/*                                   Mocks                                    */
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/*                                   Tests                                    */
/*----------------------------------------------------------------------------*/

void test_set_with_single_parameter()
{
    char *reqPayload = "{\"parameters\":[{\"name\":\"Device.DeviceInfo.Webpa.Count\",\"value\":\"4\",\"dataType\":1}],\"command\":\"SET\"}";
    char *resPayload = NULL;
    cJSON *response = NULL, *paramArray = NULL, *resParamObj = NULL;
    int count = 1;
    int totalCount = 1;

    getCompDetails();
    parameterValStruct_t **valueList = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*));
    valueList[0] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t)*totalCount);
    valueList[0]->parameterName = (char *) malloc(sizeof(char) * MAX_PARAMETER_LEN);
    strncpy(valueList[0]->parameterName, "Device.DeviceInfo.Webpa.Count",MAX_PARAMETER_LEN);
    valueList[0]->parameterValue = (char *) malloc(sizeof(char) * MAX_PARAMETER_LEN);
    strncpy(valueList[0]->parameterValue, "3",MAX_PARAMETER_LEN);
    valueList[0]->type = ccsp_int;

    will_return(get_global_values, valueList);
    will_return(get_global_parameters_count, totalCount);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterValues, size, 1);

    will_return(get_global_faultParam, NULL);
    will_return(CcspBaseIf_setParameterValues, CCSP_SUCCESS);
    expect_function_call(CcspBaseIf_setParameterValues);
    expect_value(CcspBaseIf_setParameterValues, size, 1);

    processRequest(reqPayload, NULL, &resPayload);
    WalInfo("resPayload : %s\n",resPayload);

    assert_non_null(resPayload);
    response = cJSON_Parse(resPayload);
    assert_non_null(response);
    paramArray = cJSON_GetObjectItem(response, "parameters");
    assert_int_equal(count, cJSON_GetArraySize(paramArray));
    resParamObj = cJSON_GetArrayItem(paramArray, 0);
    assert_string_equal("Device.DeviceInfo.Webpa.Count",cJSON_GetObjectItem(resParamObj, "name")->valuestring);
    assert_string_equal("Success",cJSON_GetObjectItem(resParamObj, "message")->valuestring );
    assert_int_equal(200, cJSON_GetObjectItem(response, "statusCode")->valueint);
    cJSON_Delete(response);
}

void test_set_with_multiple_parameters()
{
    char *reqPayload = "{\"parameters\":[{\"name\":\"Device.WiFi.SSID.10001.name\",\"value\":\"DeviceXB3\",\"dataType\":0},{\"name\":\"Device.WiFi.AccessPoint.10002.Enable\",\"value\":\"false\",\"dataType\":3},{\"name\":\"Device.WiFi.Radio.10000.Enable\",\"value\":\"true\",\"dataType\":3}],\"command\":\"SET\"}";
    char *transactionId = "aasfsdfgeheqegehsa";
    char *resPayload = NULL;
    cJSON *response = NULL, *paramArray = NULL, *resParamObj = NULL;
    int count = 1, i=0;
    char *names[MAX_PARAMETER_LEN] = {"Device.WiFi.SSID.10001.name", "Device.WiFi.AccessPoint.10002.Enable","Device.WiFi.Radio.10000.Enable"};
    char *values[MAX_PARAMETER_LEN] = {"test1234","false","false"};
    int type[MAX_PARAMETER_LEN] = {ccsp_string, ccsp_boolean, ccsp_boolean};
    int totalCount = 3;

    getCompDetails();
    parameterValStruct_t **valueList = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*)*totalCount);
    for(i = 0; i<totalCount; i++)
    {
        valueList[i] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t));
        valueList[i][0].parameterName = (char *) malloc(sizeof(char) * MAX_PARAMETER_LEN);
        strncpy(valueList[i][0].parameterName, names[i],MAX_PARAMETER_LEN);
        valueList[i][0].parameterValue = (char *) malloc(sizeof(char) * MAX_PARAMETER_LEN);
        strncpy(valueList[i][0].parameterValue, values[i],MAX_PARAMETER_LEN);
        valueList[i][0].type = type[i];
    }

    will_return(get_global_values, valueList);
    will_return(get_global_parameters_count, totalCount);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterValues, size, 3);

    will_return(get_global_faultParam, NULL);
    will_return(CcspBaseIf_setParameterValues, CCSP_SUCCESS);
    expect_function_call(CcspBaseIf_setParameterValues);
    expect_value(CcspBaseIf_setParameterValues, size, 3);

    processRequest(reqPayload, transactionId, &resPayload);
    WalInfo("resPayload : %s\n",resPayload);

    assert_non_null(resPayload);
    response = cJSON_Parse(resPayload);
    assert_non_null(response);
    paramArray = cJSON_GetObjectItem(response, "parameters");
    assert_int_equal(totalCount, cJSON_GetArraySize(paramArray));
    for(i=0; i<totalCount; i++)
    {
        resParamObj = cJSON_GetArrayItem(paramArray, i);
        assert_string_equal(names[i],cJSON_GetObjectItem(resParamObj, "name")->valuestring);
        assert_string_equal("Success",cJSON_GetObjectItem(resParamObj, "message")->valuestring );
    }
    assert_int_equal(200, cJSON_GetObjectItem(response, "statusCode")->valueint);
    cJSON_Delete(response);
}

void test_set_with_multiple_parameters_different_components()
{
    char *reqPayload = "{\"parameters\":[{\"name\":\"Device.WiFi.SSID.10001.name\",\"value\":\"DeviceXB6\",\"dataType\":0},{\"name\":\"Device.WiFi.SSID.10002.Enable\",\"value\":\"true\",\"dataType\":4},{\"name\":\"Device.DeviceInfo.Webpa.Name\",\"value\":\"webpa-adapter\",\"dataType\":0},{\"name\":\"Device.Webpa.Enable\",\"value\":\"true\",\"dataType\":5},{\"name\":\"Device.NAT.EnablePortMapping\",\"value\":\"true\",\"dataType\":6}],\"command\":\"SET\"}";
    char *resPayload = NULL;
    cJSON *response = NULL, *paramArray = NULL, *resParamObj = NULL;
    int count = 5, i=0, j=0;
    char *wifiNames[MAX_PARAMETER_LEN] = {"Device.WiFi.SSID.10001.name", "Device.WiFi.SSID.10002.Enable"};
    char *wifiValues[MAX_PARAMETER_LEN] = {"test1234","false"};
    int wifiType[MAX_PARAMETER_LEN] = {ccsp_string, ccsp_dateTime};
    char *webpaNames[MAX_PARAMETER_LEN] = {"Device.DeviceInfo.Webpa.Name","Device.Webpa.Enable"};
    char *webpaValues[MAX_PARAMETER_LEN] = {"true","webpa"};
    int webpaType[MAX_PARAMETER_LEN] = {ccsp_string,ccsp_base64};

    getCompDetails();
    parameterValStruct_t **valueList = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*));
    valueList[0] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t));
    valueList[0]->parameterName = (char *) malloc(sizeof(char) * MAX_PARAMETER_LEN);
    strncpy(valueList[0]->parameterName, "Device.NAT.EnablePortMapping",MAX_PARAMETER_LEN);
    valueList[0]->parameterValue = (char *) malloc(sizeof(char) * MAX_PARAMETER_LEN);
    strncpy(valueList[0]->parameterValue, "false",MAX_PARAMETER_LEN);
    valueList[0]->type = ccsp_long;

    parameterValStruct_t **valueList2 = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*)*2);
    for(i = 0; i<2; i++)
    {
        valueList2[i] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t));
        valueList2[i][0].parameterName = (char *) malloc(sizeof(char) * MAX_PARAMETER_LEN);
        strncpy(valueList2[i][0].parameterName, wifiNames[i],MAX_PARAMETER_LEN);
        valueList2[i][0].parameterValue = (char *) malloc(sizeof(char) * MAX_PARAMETER_LEN);
        strncpy(valueList2[i][0].parameterValue, wifiValues[i],MAX_PARAMETER_LEN);
        valueList2[i][0].type = wifiType[i];
    }

    parameterValStruct_t **valueList1 = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*)*2);
    for(i = 0; i<2; i++)
    {
        valueList1[i] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t));
        valueList1[i][0].parameterName = (char *) malloc(sizeof(char) * MAX_PARAMETER_LEN);
        strncpy(valueList1[i][0].parameterName, webpaNames[i],MAX_PARAMETER_LEN);
        valueList1[i][0].parameterValue = (char *) malloc(sizeof(char) * MAX_PARAMETER_LEN);
        strncpy(valueList1[i][0].parameterValue, webpaValues[i],MAX_PARAMETER_LEN);
        valueList1[i][0].type = webpaType[i];
    }

    will_return(get_global_values, valueList2);
    will_return(get_global_parameters_count, 2);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterValues, size, 2);

    will_return(get_global_values, valueList1);
    will_return(get_global_parameters_count, 2);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterValues, size, 2);

    will_return(get_global_values, valueList);
    will_return(get_global_parameters_count, 1);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterValues, size, 1);

    will_return(get_global_faultParam, NULL);
    will_return(CcspBaseIf_setParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_setParameterValues, size, 2);
    will_return(get_global_faultParam, NULL);
    will_return(CcspBaseIf_setParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_setParameterValues, size, 1);
    will_return(get_global_faultParam, NULL);
    will_return(CcspBaseIf_setParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_setParameterValues, size, 2);
    expect_function_calls(CcspBaseIf_setParameterValues,3);

    processRequest(reqPayload, NULL, &resPayload);
    WalInfo("resPayload : %s\n",resPayload);
    assert_non_null(resPayload);
    response = cJSON_Parse(resPayload);
    assert_non_null(response);
    paramArray = cJSON_GetObjectItem(response, "parameters");
    assert_int_equal(count, cJSON_GetArraySize(paramArray));
    assert_int_equal(200, cJSON_GetObjectItem(response, "statusCode")->valueint);
    for(i=0; i<count-3; i++)
    {
        resParamObj = cJSON_GetArrayItem(paramArray, i);
        assert_string_equal(wifiNames[i],cJSON_GetObjectItem(resParamObj, "name")->valuestring);
        assert_string_equal("Success",cJSON_GetObjectItem(resParamObj, "message")->valuestring );
    }

    for(i=2; i<count-1; i++)
    {
        resParamObj = cJSON_GetArrayItem(paramArray, i);
        assert_string_equal(webpaNames[j],cJSON_GetObjectItem(resParamObj, "name")->valuestring);
        assert_string_equal("Success",cJSON_GetObjectItem(resParamObj, "message")->valuestring );
        j++;
    }

    resParamObj = cJSON_GetArrayItem(paramArray, count-1);
    assert_string_equal("Device.NAT.EnablePortMapping",cJSON_GetObjectItem(resParamObj, "name")->valuestring);
    assert_string_equal("Success",cJSON_GetObjectItem(resParamObj, "message")->valuestring );
    cJSON_Delete(response);
}

void err_set_with_wildcard_parameter()
{
    char *reqPayload = "{\"parameters\":[{\"name\":\"Device.DeviceInfo.Webpa.\",\"value\":\"true\",\"dataType\":3}],\"command\":\"SET\"}";
    char *resPayload = NULL;
    cJSON *response = NULL;

    processRequest(reqPayload, NULL, &resPayload);
    WalInfo("resPayload : %s\n",resPayload);

    assert_non_null(resPayload);
    response = cJSON_Parse(resPayload);
    assert_non_null(response);
    assert_string_equal("Wildcard is not supported",cJSON_GetObjectItem(response, "message")->valuestring );
    assert_int_equal(552, cJSON_GetObjectItem(response, "statusCode")->valueint);
    cJSON_Delete(response);
}

void err_set_with_cid_parameter()
{
    char *reqPayload = "{\"parameters\":[{\"name\":\"Device.DeviceInfo.Webpa.X_COMCAST-COM_CID\",\"value\":\"abcd\",\"dataType\":0}],\"command\":\"SET\"}";
    char *resPayload = NULL;
    cJSON *response = NULL;

    processRequest(reqPayload, NULL, &resPayload);
    WalInfo("resPayload : %s\n",resPayload);

    assert_non_null(resPayload);
    response = cJSON_Parse(resPayload);
    assert_non_null(response);
    assert_string_equal("SET of CMC or CID is not supported",cJSON_GetObjectItem(response, "message")->valuestring );
    assert_int_equal(552, cJSON_GetObjectItem(response, "statusCode")->valueint);
    cJSON_Delete(response);
}

void err_set_with_cmc_parameter()
{
    char *reqPayload = "{\"parameters\":[{\"name\":\"Device.DeviceInfo.Webpa.X_COMCAST-COM_CMC\",\"value\":\"32\",\"dataType\":2}],\"command\":\"SET\"}";
    char *resPayload = NULL;
    cJSON *response = NULL;

    processRequest(reqPayload, NULL, &resPayload);
    WalInfo("resPayload : %s\n",resPayload);

    assert_non_null(resPayload);
    response = cJSON_Parse(resPayload);
    assert_non_null(response);
    assert_string_equal("SET of CMC or CID is not supported",cJSON_GetObjectItem(response, "message")->valuestring );
    assert_int_equal(552, cJSON_GetObjectItem(response, "statusCode")->valueint);
    cJSON_Delete(response);
}

void err_set_without_value()
{
    char *reqPayload = "{\"parameters\":[{\"name\":\"Device.Webpa.Name\",\"dataType\":0}],\"command\":\"SET\"}";
    char *resPayload = NULL;
    cJSON *response = NULL;

    processRequest(reqPayload, NULL, &resPayload);
    WalInfo("resPayload : %s\n",resPayload);

    assert_non_null(resPayload);
    response = cJSON_Parse(resPayload);
    assert_non_null(response);
    assert_string_equal("Parameter value is null",cJSON_GetObjectItem(response, "message")->valuestring );
    assert_int_equal(552, cJSON_GetObjectItem(response, "statusCode")->valueint);
    cJSON_Delete(response);
}

void err_set_invalid_parameter()
{
    char *reqPayload = "{\"parameters\":[{\"name\":\"Device.Webpa.SSID\",\"value\":\"1234\",\"dataType\":0}],\"command\":\"SET\"}";
    char *resPayload = NULL;
    cJSON *response = NULL, *paramArray = NULL, *resParamObj = NULL;
    getCompDetails();

    will_return(get_global_parameters_count,0);
    will_return(get_global_values,NULL);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_ERR_INVALID_PARAMETER_NAME);
    expect_value(CcspBaseIf_getParameterValues, size, 1);
    processRequest(reqPayload, NULL, &resPayload);
    WalInfo("resPayload : %s\n",resPayload);

    assert_non_null(resPayload);
    response = cJSON_Parse(resPayload);
    assert_non_null(response);
    paramArray = cJSON_GetObjectItem(response, "parameters");
    assert_int_equal(1, cJSON_GetArraySize(paramArray));
    resParamObj = cJSON_GetArrayItem(paramArray, 0);
    assert_string_equal("Device.Webpa.SSID",cJSON_GetObjectItem(resParamObj, "name")->valuestring);
    assert_string_equal("Invalid parameter name",cJSON_GetObjectItem(resParamObj, "message")->valuestring );
    assert_int_equal(520, cJSON_GetObjectItem(response, "statusCode")->valueint);
    cJSON_Delete(response);
}

void err_set_invalid_component()
{
    char *reqPayload = "{\"parameters\":[{\"name\":\"Device.Abcd.SSID\",\"value\":\"1234\",\"dataType\":0}],\"command\":\"SET\"}";
    char *resPayload = NULL;
    cJSON *response = NULL, *paramArray = NULL, *resParamObj = NULL;
    getCompDetails();

    will_return(get_global_component_size,0);
    will_return(get_global_components,NULL);

    expect_function_call(CcspBaseIf_discComponentSupportingNamespace);
    will_return(CcspBaseIf_discComponentSupportingNamespace, CCSP_CR_ERR_UNSUPPORTED_NAMESPACE);
    expect_function_call(free_componentStruct_t);

    processRequest(reqPayload, NULL, &resPayload);
    WalInfo("resPayload : %s\n",resPayload);

    assert_non_null(resPayload);
    response = cJSON_Parse(resPayload);
    assert_non_null(response);
    paramArray = cJSON_GetObjectItem(response, "parameters");
    assert_int_equal(1, cJSON_GetArraySize(paramArray));
    resParamObj = cJSON_GetArrayItem(paramArray, 0);
    assert_string_equal("Device.Abcd.SSID",cJSON_GetObjectItem(resParamObj, "name")->valuestring);
    assert_string_equal("Error unsupported namespace",cJSON_GetObjectItem(resParamObj, "message")->valuestring );
    assert_int_equal(520, cJSON_GetObjectItem(response, "statusCode")->valueint);
    cJSON_Delete(response);
}

void err_set_with_wifi_busy()
{
    char *reqPayload = "{\"parameters\":[{\"name\":\"Device.WiFi.SSID\",\"value\":\"1234\",\"dataType\":0}],\"command\":\"SET\"}";
    char *resPayload = NULL;
    cJSON *response = NULL, *paramArray = NULL, *resParamObj = NULL;
    applySettingsFlag = TRUE;

    getCompDetails();
    processRequest(reqPayload, NULL, &resPayload);
    WalInfo("resPayload : %s\n",resPayload);

    assert_non_null(resPayload);
    response = cJSON_Parse(resPayload);
    assert_non_null(response);
    paramArray = cJSON_GetObjectItem(response, "parameters");
    assert_int_equal(1, cJSON_GetArraySize(paramArray));
    resParamObj = cJSON_GetArrayItem(paramArray, 0);
    assert_string_equal("Device.WiFi.SSID",cJSON_GetObjectItem(resParamObj, "name")->valuestring);
    assert_string_equal("WiFi is busy",cJSON_GetObjectItem(resParamObj, "message")->valuestring );
    assert_int_equal(530, cJSON_GetObjectItem(response, "statusCode")->valueint);
    cJSON_Delete(response);
}

void err_set_with_invalid_type()
{
    char *reqPayload = "{\"parameters\":[{\"name\":\"Device.WiFi.SSID.10001.SSID\",\"value\":\"1234\",\"dataType\":31}],\"command\":\"SET\"}";
    char *transactionId = "aasfsdfgeh";
    char *resPayload = NULL;
    cJSON *response = NULL, *paramArray = NULL, *resParamObj = NULL;
    applySettingsFlag = FALSE;

    parameterValStruct_t **valueList = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*));
    valueList[0] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t));
    valueList[0][0].parameterName = (char *) malloc(sizeof(char) * MAX_PARAMETER_LEN);
    strncpy(valueList[0][0].parameterName, "Device.WiFi.SSID.10001.SSID",MAX_PARAMETER_LEN);
    valueList[0][0].parameterValue = (char *) malloc(sizeof(char) * MAX_PARAMETER_LEN);
    strncpy(valueList[0][0].parameterValue, "abcd",MAX_PARAMETER_LEN);
    valueList[0][0].type = ccsp_string;

    getCompDetails();
    will_return(get_global_values, valueList);
    will_return(get_global_parameters_count, 1);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterValues, size, 1);

    char *faultParam = strdup(valueList[0][0].parameterName);
    will_return(get_global_faultParam, faultParam);
    expect_function_call(CcspBaseIf_setParameterValues);
    will_return(CcspBaseIf_setParameterValues, CCSP_ERR_INVALID_PARAMETER_TYPE);
    expect_value(CcspBaseIf_setParameterValues, size, 1);

    processRequest(reqPayload, transactionId, &resPayload);
    WalInfo("resPayload : %s\n",resPayload);

    assert_non_null(resPayload);
    response = cJSON_Parse(resPayload);
    assert_non_null(response);
    paramArray = cJSON_GetObjectItem(response, "parameters");
    assert_int_equal(1, cJSON_GetArraySize(paramArray));
    resParamObj = cJSON_GetArrayItem(paramArray, 0);
    assert_string_equal("Device.WiFi.SSID.10001.SSID",cJSON_GetObjectItem(resParamObj, "name")->valuestring);
    assert_string_equal("Invalid parameter type",cJSON_GetObjectItem(resParamObj, "message")->valuestring );
    assert_int_equal(520, cJSON_GetObjectItem(response, "statusCode")->valueint);
    cJSON_Delete(response);
}

void err_set_with_large_parameter_name()
{
    char *reqPayload = "{\"parameters\":[{\"name\":\"Device.WiFi.SSID.10001.sfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGAQWWEGYTEHERHAEGTWERHTBQR4WYTR\",\"value\":\"1234\",\"dataType\":0}],\"command\":\"SET\"}";
    char *resPayload = NULL;
    cJSON *response = NULL;
    applySettingsFlag = FALSE;

    processRequest(reqPayload, NULL, &resPayload);
    WalInfo("resPayload : %s\n",resPayload);

    assert_non_null(resPayload);
    response = cJSON_Parse(resPayload);
    assert_non_null(response);
    assert_string_equal("Invalid Param",cJSON_GetObjectItem(response, "message")->valuestring );
    assert_int_equal(520, cJSON_GetObjectItem(response, "statusCode")->valueint);
    cJSON_Delete(response);
}

void err_set_with_large_parameter_value()
{
    char *reqPayload = "{\"parameters\":[{\"name\":\"Device.WiFi.SSID.10001.SSID\",\"value\":\"sfdgfgherejfjfjkksfhjgkeraskkkkkkkkkkdweqlfdrkfhgsrsjljrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGAQWWEGYTEHERHAEGTWERHTBQR4WYTR\",\"dataType\":0}],\"command\":\"SET\"}";
    char *resPayload = NULL;
    cJSON *response = NULL;
    applySettingsFlag = FALSE;

    processRequest(reqPayload, NULL, &resPayload);
    WalInfo("resPayload : %s\n",resPayload);

    assert_non_null(resPayload);
    response = cJSON_Parse(resPayload);
    assert_non_null(response);
    assert_string_equal("Invalid Param",cJSON_GetObjectItem(response, "message")->valuestring );
    assert_int_equal(520, cJSON_GetObjectItem(response, "statusCode")->valueint);
    cJSON_Delete(response);
}

void err_set_not_writable_parameter()
{
    char *reqPayload = "{\"parameters\":[{\"name\":\"Device.WiFi.RadioNumberOfEntries\",\"value\":\"1234\",\"dataType\":7}],\"command\":\"SET\"}";
    char *resPayload = NULL;
    cJSON *response = NULL, *paramArray = NULL, *resParamObj = NULL;
    applySettingsFlag = FALSE;

    parameterValStruct_t **valueList = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*));
    valueList[0] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t));
    valueList[0][0].parameterName = (char *) malloc(sizeof(char) * MAX_PARAMETER_LEN);
    strncpy(valueList[0][0].parameterName, "Device.WiFi.RadioNumberOfEntries",MAX_PARAMETER_LEN);
    valueList[0][0].parameterValue = (char *) malloc(sizeof(char) * MAX_PARAMETER_LEN);
    strncpy(valueList[0][0].parameterValue, "2",MAX_PARAMETER_LEN);
    valueList[0][0].type = ccsp_unsignedLong;

    getCompDetails();
    will_return(get_global_values, valueList);
    will_return(get_global_parameters_count, 1);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterValues, size, 1);

    char *faultParam = strdup(valueList[0][0].parameterName);
    will_return(get_global_faultParam, faultParam);
    expect_function_call(CcspBaseIf_setParameterValues);
    will_return(CcspBaseIf_setParameterValues, CCSP_ERR_NOT_WRITABLE);
    expect_value(CcspBaseIf_setParameterValues, size, 1);

    processRequest(reqPayload, NULL, &resPayload);
    WalInfo("resPayload : %s\n",resPayload);

    assert_non_null(resPayload);
    response = cJSON_Parse(resPayload);
    assert_non_null(response);
    paramArray = cJSON_GetObjectItem(response, "parameters");
    assert_int_equal(1, cJSON_GetArraySize(paramArray));
    resParamObj = cJSON_GetArrayItem(paramArray, 0);
    assert_string_equal("Device.WiFi.RadioNumberOfEntries",cJSON_GetObjectItem(resParamObj, "name")->valuestring);
    assert_string_equal("Parameter is not writable",cJSON_GetObjectItem(resParamObj, "message")->valuestring );
    assert_int_equal(520, cJSON_GetObjectItem(response, "statusCode")->valueint);
    cJSON_Delete(response);
}

void err_set_with_empty_value()
{
    char *reqPayload = "{\"parameters\":[{\"name\":\"Device.Webpa.Name\",\"value\":\"\",\"dataType\":0}],\"command\":\"SET\"}";
    char *resPayload = NULL;
    cJSON *response = NULL;

    processRequest(reqPayload, NULL, &resPayload);
    WalInfo("resPayload : %s\n",resPayload);

    assert_non_null(resPayload);
    response = cJSON_Parse(resPayload);
    assert_non_null(response);
    assert_string_equal("Parameter value is null",cJSON_GetObjectItem(response, "message")->valuestring );
    assert_int_equal(552, cJSON_GetObjectItem(response, "statusCode")->valueint);
    cJSON_Delete(response);
}

void err_set_with_multiple_parameters()
{
    char *reqPayload = "{\"parameters\":[{\"name\":\"Device.WiFi.SSID.10001.name\",\"value\":\"DeviceXB3\",\"dataType\":0},{\"name\":\"Device.WiFi.SSID.10002.Enable\",\"value\":\"false\",\"dataType\":3},{\"name\":\"Device.WiFi.Radio.10000.Enab\",\"value\":\"true\",\"dataType\":3}],\"command\":\"SET\"}";
    char *transactionId = "aasfsdfgeheqegehsa";
    char *resPayload = NULL;
    cJSON *response = NULL, *paramArray = NULL, *resParamObj = NULL;
    int count = 1, i=0;
    char *names[MAX_PARAMETER_LEN] = {"Device.WiFi.SSID.10001.name", "Device.WiFi.SSID.10002.Enable","Device.WiFi.Radio.10000.Enab"};
    int totalCount = 3;

    getCompDetails();

    will_return(get_global_values, NULL);
    will_return(get_global_parameters_count, 0);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_CR_ERR_UNSUPPORTED_NAMESPACE);
    expect_value(CcspBaseIf_getParameterValues, size, 3);

    processRequest(reqPayload, transactionId, &resPayload);
    WalInfo("resPayload : %s\n",resPayload);

    assert_non_null(resPayload);
    response = cJSON_Parse(resPayload);
    assert_non_null(response);
    paramArray = cJSON_GetObjectItem(response, "parameters");
    assert_int_equal(totalCount, cJSON_GetArraySize(paramArray));
    for(i=0; i<totalCount; i++)
    {
        resParamObj = cJSON_GetArrayItem(paramArray, i);
        assert_string_equal(names[i],cJSON_GetObjectItem(resParamObj, "name")->valuestring);
        assert_string_equal("Error unsupported namespace",cJSON_GetObjectItem(resParamObj, "message")->valuestring );
    }
    assert_int_equal(520, cJSON_GetObjectItem(response, "statusCode")->valueint);
    cJSON_Delete(response);
}

void err_set_with_multiple_parameters_different_component()
{
    char *reqPayload = "{\"parameters\":[{\"name\":\"Device.WiFi.SSID.10001.name\",\"value\":\"DeviceXB6\",\"dataType\":0},{\"name\":\"Device.DeviceInfo.Webpa.Alias\",\"value\":\"webpa-adapter\",\"dataType\":0},{\"name\":\"Device.WiFi.RadioNumberOfEntries\",\"value\":\"12\",\"dataType\":2},{\"name\":\"Device.NAT.EnablePortMapping\",\"value\":\"true\",\"dataType\":3}],\"command\":\"SET\"}";
    char *transactionId = "aasfsdfgeheqegehsa";
    char *resPayload = NULL;
    cJSON *response = NULL, *paramArray = NULL, *resParamObj = NULL;
    int count = 4, i=0;
    char *nameList[]={"Device.WiFi.SSID.10001.name", "Device.DeviceInfo.Webpa.Alias", "Device.WiFi.RadioNumberOfEntries", "Device.NAT.EnablePortMapping"};
    char *wifiNames[MAX_PARAMETER_LEN] = {"Device.WiFi.SSID.10001.name", "Device.WiFi.RadioNumberOfEntries"};
    char *wifiValues[MAX_PARAMETER_LEN] = {"test1234","2"};
    int wifiType[MAX_PARAMETER_LEN] = {ccsp_string, ccsp_double};
    char *webpaNames[MAX_PARAMETER_LEN] = {"Device.DeviceInfo.Webpa.Alias"};
    char *webpaValues[MAX_PARAMETER_LEN] = {"webpa"};
    int webpaType[MAX_PARAMETER_LEN] = {ccsp_float};

    getCompDetails();

    parameterValStruct_t **valueList = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*));
    valueList[0] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t));
    valueList[0]->parameterName = (char *) malloc(sizeof(char) * MAX_PARAMETER_LEN);
    strncpy(valueList[0]->parameterName, "Device.NAT.EnablePortMapping",MAX_PARAMETER_LEN);
    valueList[0]->parameterValue = (char *) malloc(sizeof(char) * MAX_PARAMETER_LEN);
    strncpy(valueList[0]->parameterValue, "false",MAX_PARAMETER_LEN);
    valueList[0]->type = ccsp_boolean;

    parameterValStruct_t **valueList2 = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*)*2);
    for(i = 0; i<2; i++)
    {
        valueList2[i] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t));
        valueList2[i][0].parameterName = (char *) malloc(sizeof(char) * MAX_PARAMETER_LEN);
        strncpy(valueList2[i][0].parameterName, wifiNames[i],MAX_PARAMETER_LEN);
        valueList2[i][0].parameterValue = (char *) malloc(sizeof(char) * MAX_PARAMETER_LEN);
        strncpy(valueList2[i][0].parameterValue, wifiValues[i],MAX_PARAMETER_LEN);
        valueList2[i][0].type = wifiType[i];
    }

    parameterValStruct_t **valueList1 = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*)*1);
    for(i = 0; i<1; i++)
    {
        valueList1[i] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t));
        valueList1[i][0].parameterName = (char *) malloc(sizeof(char) * MAX_PARAMETER_LEN);
        strncpy(valueList1[i][0].parameterName, webpaNames[i],MAX_PARAMETER_LEN);
        valueList1[i][0].parameterValue = (char *) malloc(sizeof(char) * MAX_PARAMETER_LEN);
        strncpy(valueList1[i][0].parameterValue, webpaValues[i],MAX_PARAMETER_LEN);
        valueList1[i][0].type = webpaType[i];
    }

    will_return(get_global_values, valueList2);
    will_return(get_global_parameters_count, 2);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterValues, size, 2);

    will_return(get_global_values, valueList1);
    will_return(get_global_parameters_count, 1);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterValues, size, 1);

    will_return(get_global_values, valueList);
    will_return(get_global_parameters_count, 1);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterValues, size, 1);

    will_return(get_global_faultParam, NULL);
    will_return(CcspBaseIf_setParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_setParameterValues, size, 1);
    char *faultParam = strdup(nameList[1]);
    will_return(get_global_faultParam, faultParam);
    will_return(CcspBaseIf_setParameterValues, CCSP_ERR_NOT_WRITABLE);
    expect_value(CcspBaseIf_setParameterValues, size, 1);
    will_return(get_global_faultParam, NULL);
    will_return(CcspBaseIf_setParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_setParameterValues, size, 1);
    expect_function_calls(CcspBaseIf_setParameterValues,3);

    processRequest(reqPayload, transactionId, &resPayload);
    WalInfo("resPayload : %s\n",resPayload);

    assert_non_null(resPayload);
    response = cJSON_Parse(resPayload);
    assert_non_null(response);
    paramArray = cJSON_GetObjectItem(response, "parameters");
    assert_int_equal(count, cJSON_GetArraySize(paramArray));
    assert_int_equal(520, cJSON_GetObjectItem(response, "statusCode")->valueint);
    for(i=0; i<count; i++)
    {
        resParamObj = cJSON_GetArrayItem(paramArray, i);
        assert_string_equal(nameList[i],cJSON_GetObjectItem(resParamObj, "name")->valuestring);
        assert_string_equal("Parameter is not writable",cJSON_GetObjectItem(resParamObj, "message")->valuestring );
    }
    cJSON_Delete(response);
}

void err_set_with_multiple_parameters_failure_in_get()
{
    char *reqPayload = "{\"parameters\":[{\"name\":\"Device.WiFi.AccessPoint.10001.name\",\"value\":\"DeviceXB6\",\"dataType\":0},{\"name\":\"Device.DeviceInfo.Webpa.Alias\",\"value\":\"webpa-adapter\",\"dataType\":0},{\"name\":\"Device.WiFi.RadioNumberOfEntries\",\"value\":\"12\",\"dataType\":2},{\"name\":\"Device.NAT.EnablePortMapp\",\"value\":\"true\",\"dataType\":3}],\"command\":\"SET\"}";
    char *transactionId = "aasfsdfgeheqegehsa";
    char *resPayload = NULL;
    cJSON *response = NULL, *paramArray = NULL, *resParamObj = NULL;
    int count = 4, i=0;
    char *nameList[]={"Device.WiFi.AccessPoint.10001.name", "Device.DeviceInfo.Webpa.Alias", "Device.WiFi.RadioNumberOfEntries", "Device.NAT.EnablePortMapp"};
    char *wifiNames[MAX_PARAMETER_LEN] = {"Device.WiFi.SSID.10001.name", "Device.WiFi.RadioNumberOfEntries"};
    char *wifiValues[MAX_PARAMETER_LEN] = {"test1234","2"};
    int wifiType[MAX_PARAMETER_LEN] = {ccsp_string, ccsp_int};
    char *webpaNames[MAX_PARAMETER_LEN] = {"Device.DeviceInfo.Webpa.Alias"};
    char *webpaValues[MAX_PARAMETER_LEN] = {"webpa"};
    int webpaType[MAX_PARAMETER_LEN] = {ccsp_byte};

    getCompDetails();

    parameterValStruct_t **valueList2 = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*)*2);
    for(i = 0; i<2; i++)
    {
        valueList2[i] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t));
        valueList2[i][0].parameterName = (char *) malloc(sizeof(char) * MAX_PARAMETER_LEN);
        strncpy(valueList2[i][0].parameterName, wifiNames[i],MAX_PARAMETER_LEN);
        valueList2[i][0].parameterValue = (char *) malloc(sizeof(char) * MAX_PARAMETER_LEN);
        strncpy(valueList2[i][0].parameterValue, wifiValues[i],MAX_PARAMETER_LEN);
        valueList2[i][0].type = wifiType[i];
    }

    parameterValStruct_t **valueList1 = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*)*1);
    for(i = 0; i<1; i++)
    {
        valueList1[i] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t));
        valueList1[i][0].parameterName = (char *) malloc(sizeof(char) * MAX_PARAMETER_LEN);
        strncpy(valueList1[i][0].parameterName, webpaNames[i],MAX_PARAMETER_LEN);
        valueList1[i][0].parameterValue = (char *) malloc(sizeof(char) * MAX_PARAMETER_LEN);
        strncpy(valueList1[i][0].parameterValue, webpaValues[i],MAX_PARAMETER_LEN);
        valueList1[i][0].type = webpaType[i];
    }

    will_return(get_global_values, valueList2);
    will_return(get_global_parameters_count, 2);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterValues, size, 2);
    will_return(get_global_values, valueList1);
    will_return(get_global_parameters_count, 1);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterValues, size, 1);

    will_return(get_global_values, NULL);
    will_return(get_global_parameters_count, 0);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_ERR_INVALID_PARAMETER_NAME);
    expect_value(CcspBaseIf_getParameterValues, size, 1);

    processRequest(reqPayload, transactionId, &resPayload);
    WalInfo("resPayload : %s\n",resPayload);

    assert_non_null(resPayload);
    response = cJSON_Parse(resPayload);
    assert_non_null(response);
    paramArray = cJSON_GetObjectItem(response, "parameters");
    assert_int_equal(count, cJSON_GetArraySize(paramArray));
    assert_int_equal(520, cJSON_GetObjectItem(response, "statusCode")->valueint);
    for(i=0; i<count; i++)
    {
        resParamObj = cJSON_GetArrayItem(paramArray, i);
        assert_string_equal(nameList[i],cJSON_GetObjectItem(resParamObj, "name")->valuestring);
        assert_string_equal("Invalid parameter name",cJSON_GetObjectItem(resParamObj, "message")->valuestring );
    }
    cJSON_Delete(response);
}

void err_set_with_multiple_parameters_failure_in_rollback()
{
    char *reqPayload = "{\"parameters\":[{\"name\":\"Device.WiFi.SSID.10001.name\",\"value\":\"DeviceXB6\",\"dataType\":0},{\"name\":\"Device.DeviceInfo.Webpa.Alias\",\"value\":\"webpa-adapter\",\"dataType\":0},{\"name\":\"Device.WiFi.RadioNumberOfEntries\",\"value\":\"12\",\"dataType\":2},{\"name\":\"Device.NAT.EnablePortMapping\",\"value\":\"true\",\"dataType\":3}],\"command\":\"SET\"}";
    char *transactionId = "aasfsdfgeheqegehsa";
    char *resPayload = NULL;
    cJSON *response = NULL, *paramArray = NULL, *resParamObj = NULL;
    int count = 4, i=0;
    char *nameList[]={"Device.WiFi.SSID.10001.name", "Device.DeviceInfo.Webpa.Alias", "Device.WiFi.RadioNumberOfEntries", "Device.NAT.EnablePortMapping"};
    char *wifiNames[MAX_PARAMETER_LEN] = {"Device.WiFi.SSID.10001.name", "Device.WiFi.RadioNumberOfEntries"};
    char *wifiValues[MAX_PARAMETER_LEN] = {"test1234","2"};
    int wifiType[MAX_PARAMETER_LEN] = {ccsp_byte, ccsp_int};
    char *webpaNames[MAX_PARAMETER_LEN] = {"Device.DeviceInfo.Webpa.Alias"};
    char *webpaValues[MAX_PARAMETER_LEN] = {"webpa"};
    int webpaType[MAX_PARAMETER_LEN] = {ccsp_unsignedLong};

    getCompDetails();

    parameterValStruct_t **valueList = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*));
    valueList[0] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t));
    valueList[0]->parameterName = (char *) malloc(sizeof(char) * MAX_PARAMETER_LEN);
    strncpy(valueList[0]->parameterName, "Device.NAT.EnablePortMapping",MAX_PARAMETER_LEN);
    valueList[0]->parameterValue = (char *) malloc(sizeof(char) * MAX_PARAMETER_LEN);
    strncpy(valueList[0]->parameterValue, "false",MAX_PARAMETER_LEN);
    valueList[0]->type = ccsp_boolean;

    parameterValStruct_t **valueList2 = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*)*2);
    for(i = 0; i<2; i++)
    {
        valueList2[i] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t));
        valueList2[i][0].parameterName = (char *) malloc(sizeof(char) * MAX_PARAMETER_LEN);
        strncpy(valueList2[i][0].parameterName, wifiNames[i],MAX_PARAMETER_LEN);
        valueList2[i][0].parameterValue = (char *) malloc(sizeof(char) * MAX_PARAMETER_LEN);
        strncpy(valueList2[i][0].parameterValue, wifiValues[i],MAX_PARAMETER_LEN);
        valueList2[i][0].type = wifiType[i];
    }

    parameterValStruct_t **valueList1 = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*)*1);
    for(i = 0; i<1; i++)
    {
        valueList1[i] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t));
        valueList1[i][0].parameterName = (char *) malloc(sizeof(char) * MAX_PARAMETER_LEN);
        strncpy(valueList1[i][0].parameterName, webpaNames[i],MAX_PARAMETER_LEN);
        valueList1[i][0].parameterValue = (char *) malloc(sizeof(char) * MAX_PARAMETER_LEN);
        strncpy(valueList1[i][0].parameterValue, webpaValues[i],MAX_PARAMETER_LEN);
        valueList1[i][0].type = webpaType[i];
    }

    will_return(get_global_values, valueList2);
    will_return(get_global_parameters_count, 2);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterValues, size, 2);

    will_return(get_global_values, valueList1);
    will_return(get_global_parameters_count, 1);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterValues, size, 1);

    will_return(get_global_values, valueList);
    will_return(get_global_parameters_count, 1);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterValues, size, 1);

    will_return(get_global_faultParam, NULL);
    will_return(CcspBaseIf_setParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_setParameterValues, size, 1);
    char *faultParam = strdup(nameList[1]);
    will_return(get_global_faultParam, faultParam);
    will_return(CcspBaseIf_setParameterValues, CCSP_ERR_NOT_WRITABLE);
    expect_value(CcspBaseIf_setParameterValues, size, 1);
    will_return(get_global_faultParam, NULL);
    will_return(CcspBaseIf_setParameterValues, CCSP_ERR_INVALID_PARAMETER_NAME);
    expect_value(CcspBaseIf_setParameterValues, size, 1);
    expect_function_calls(CcspBaseIf_setParameterValues,3);

    processRequest(reqPayload, transactionId, &resPayload);
    WalInfo("resPayload : %s\n",resPayload);

    assert_non_null(resPayload);
    response = cJSON_Parse(resPayload);
    assert_non_null(response);
    paramArray = cJSON_GetObjectItem(response, "parameters");
    assert_int_equal(count, cJSON_GetArraySize(paramArray));
    assert_int_equal(520, cJSON_GetObjectItem(response, "statusCode")->valueint);
    for(i=0; i<count; i++)
    {
        resParamObj = cJSON_GetArrayItem(paramArray, i);
        assert_string_equal(nameList[i],cJSON_GetObjectItem(resParamObj, "name")->valuestring);
        assert_string_equal("Parameter is not writable",cJSON_GetObjectItem(resParamObj, "message")->valuestring );
    }
    cJSON_Delete(response);
}

void err_set_with_multiple_parameters_failure_in_wifi_rollback()
{
    char *reqPayload = "{\"parameters\":[{\"name\":\"Device.WiFi.SSID.10001.name\",\"value\":\"DeviceXB6\",\"dataType\":0},{\"name\":\"Device.DeviceInfo.Webpa.Alias\",\"value\":\"webpa-adapter\",\"dataType\":0},{\"name\":\"Device.WiFi.RadioNumberOfEntries\",\"value\":\"12\",\"dataType\":2},{\"name\":\"Device.NAT.EnablePortMapping\",\"value\":\"true\",\"dataType\":3}],\"command\":\"SET\"}";
    char *transactionId = "aasfsdfgeheqegehsa";
    char *resPayload = NULL;
    cJSON *response = NULL, *paramArray = NULL, *resParamObj = NULL;
    int count = 4, i=0;
    char *nameList[]={"Device.WiFi.SSID.10001.name", "Device.DeviceInfo.Webpa.Alias", "Device.WiFi.RadioNumberOfEntries", "Device.NAT.EnablePortMapping"};
    char *wifiNames[MAX_PARAMETER_LEN] = {"Device.WiFi.SSID.10001.name", "Device.WiFi.RadioNumberOfEntries"};
    char *wifiValues[MAX_PARAMETER_LEN] = {"test1234","2"};
    int wifiType[MAX_PARAMETER_LEN] = {ccsp_string, ccsp_int};
    char *webpaNames[MAX_PARAMETER_LEN] = {"Device.DeviceInfo.Webpa.Alias"};
    char *webpaValues[MAX_PARAMETER_LEN] = {"webpa"};
    int webpaType[MAX_PARAMETER_LEN] = {ccsp_string};

    getCompDetails();

    parameterValStruct_t **valueList = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*));
    valueList[0] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t));
    valueList[0]->parameterName = (char *) malloc(sizeof(char) * MAX_PARAMETER_LEN);
    strncpy(valueList[0]->parameterName, "Device.NAT.EnablePortMapping",MAX_PARAMETER_LEN);
    valueList[0]->parameterValue = (char *) malloc(sizeof(char) * MAX_PARAMETER_LEN);
    strncpy(valueList[0]->parameterValue, "false",MAX_PARAMETER_LEN);
    valueList[0]->type = ccsp_boolean;

    parameterValStruct_t **valueList2 = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*)*2);
    for(i = 0; i<2; i++)
    {
        valueList2[i] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t));
        valueList2[i][0].parameterName = (char *) malloc(sizeof(char) * MAX_PARAMETER_LEN);
        strncpy(valueList2[i][0].parameterName, wifiNames[i],MAX_PARAMETER_LEN);
        valueList2[i][0].parameterValue = (char *) malloc(sizeof(char) * MAX_PARAMETER_LEN);
        strncpy(valueList2[i][0].parameterValue, wifiValues[i],MAX_PARAMETER_LEN);
        valueList2[i][0].type = wifiType[i];
    }

    parameterValStruct_t **valueList1 = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*)*1);
    for(i = 0; i<1; i++)
    {
        valueList1[i] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t));
        valueList1[i][0].parameterName = (char *) malloc(sizeof(char) * MAX_PARAMETER_LEN);
        strncpy(valueList1[i][0].parameterName, webpaNames[i],MAX_PARAMETER_LEN);
        valueList1[i][0].parameterValue = (char *) malloc(sizeof(char) * MAX_PARAMETER_LEN);
        strncpy(valueList1[i][0].parameterValue, webpaValues[i],MAX_PARAMETER_LEN);
        valueList1[i][0].type = webpaType[i];
    }

    will_return(get_global_values, valueList2);
    will_return(get_global_parameters_count, 2);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterValues, size, 2);

    will_return(get_global_values, valueList1);
    will_return(get_global_parameters_count, 1);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterValues, size, 1);

    will_return(get_global_values, valueList);
    will_return(get_global_parameters_count, 1);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterValues, size, 1);

    will_return(get_global_faultParam, NULL);
    will_return(CcspBaseIf_setParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_setParameterValues, size, 1);
    will_return(get_global_faultParam, NULL);
    will_return(CcspBaseIf_setParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_setParameterValues, size, 1);

    char *faultParam = strdup(nameList[2]);
    will_return(get_global_faultParam, faultParam);
    will_return(CcspBaseIf_setParameterValues, CCSP_ERR_NOT_WRITABLE);
    expect_value(CcspBaseIf_setParameterValues, size, 2);
    will_return(get_global_faultParam, NULL);
    will_return(CcspBaseIf_setParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_setParameterValues, size, 1);
    will_return(get_global_faultParam, strdup(nameList[1]));
    will_return(CcspBaseIf_setParameterValues, CCSP_ERR_INVALID_PARAMETER_VALUE);
    expect_value(CcspBaseIf_setParameterValues, size, 1);
    expect_function_calls(CcspBaseIf_setParameterValues,5);

    processRequest(reqPayload, transactionId, &resPayload);
    WalInfo("resPayload : %s\n",resPayload);

    assert_non_null(resPayload);
    response = cJSON_Parse(resPayload);
    assert_non_null(response);
    paramArray = cJSON_GetObjectItem(response, "parameters");
    assert_int_equal(count, cJSON_GetArraySize(paramArray));
    assert_int_equal(520, cJSON_GetObjectItem(response, "statusCode")->valueint);
    for(i=0; i<count; i++)
    {
        resParamObj = cJSON_GetArrayItem(paramArray, i);
        assert_string_equal(nameList[i],cJSON_GetObjectItem(resParamObj, "name")->valuestring);
        assert_string_equal("Parameter is not writable",cJSON_GetObjectItem(resParamObj, "message")->valuestring );
    }
    cJSON_Delete(response);
}
/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_set_with_single_parameter),
        cmocka_unit_test(test_set_with_multiple_parameters),
        cmocka_unit_test(test_set_with_multiple_parameters_different_components),
        cmocka_unit_test(err_set_with_wildcard_parameter),
        cmocka_unit_test(err_set_with_cid_parameter),
        cmocka_unit_test(err_set_with_cmc_parameter),
        cmocka_unit_test(err_set_without_value),
        cmocka_unit_test(err_set_invalid_parameter),
        cmocka_unit_test(err_set_invalid_component),
        cmocka_unit_test(err_set_with_wifi_busy),
        cmocka_unit_test(err_set_with_invalid_type),
        cmocka_unit_test(err_set_with_large_parameter_name),
        cmocka_unit_test(err_set_with_large_parameter_value),
        cmocka_unit_test(err_set_not_writable_parameter),
        cmocka_unit_test(err_set_with_empty_value),
        cmocka_unit_test(err_set_with_multiple_parameters),
        cmocka_unit_test(err_set_with_multiple_parameters_different_component),
        cmocka_unit_test(err_set_with_multiple_parameters_failure_in_get),
        cmocka_unit_test(err_set_with_multiple_parameters_failure_in_rollback),
        cmocka_unit_test(err_set_with_multiple_parameters_failure_in_wifi_rollback)
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
