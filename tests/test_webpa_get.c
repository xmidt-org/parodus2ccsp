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
/*----------------------------------------------------------------------------*/
/*                                   Tests                                    */
/*----------------------------------------------------------------------------*/

void test_singleGet()
{
    char *reqPayload = "{ \"names\":[\"Device.DeviceInfo.Webpa.Enable\"],\"command\": \"GET\"}";
    char *transactionId = "aasfsdfgeh";
    char *resPayload = NULL;
    cJSON *response = NULL, *paramArray = NULL, *resParamObj = NULL;
    int count = 1;
    int totalCount = 1;

    getCompDetails();
    parameterValStruct_t **valueList = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*));
    valueList[0] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t)*totalCount);
    valueList[0]->parameterName = (char *) malloc(sizeof(char) * MAX_PARAMETER_LEN);
    strncpy(valueList[0]->parameterName, "Device.DeviceInfo.Webpa.Enable",MAX_PARAMETER_LEN);
    valueList[0]->parameterValue = (char *) malloc(sizeof(char) * MAX_PARAMETER_LEN);
    strncpy(valueList[0]->parameterValue, "true",MAX_PARAMETER_LEN);
    valueList[0]->type = ccsp_boolean;

    will_return(get_global_values, valueList);
    will_return(get_global_parameters_count, totalCount);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterValues, size, 1);

    processRequest(reqPayload, transactionId, &resPayload);
    WalInfo("resPayload : %s\n",resPayload);

    assert_non_null(resPayload);
    response = cJSON_Parse(resPayload);
    assert_non_null(response);
    paramArray = cJSON_GetObjectItem(response, "parameters");
    assert_int_equal(count, cJSON_GetArraySize(paramArray));
    resParamObj = cJSON_GetArrayItem(paramArray, 0);
    assert_int_equal(totalCount,cJSON_GetObjectItem(resParamObj, "parameterCount")->valueint);
    assert_string_equal("Device.DeviceInfo.Webpa.Enable",cJSON_GetObjectItem(resParamObj, "name")->valuestring);
    assert_string_equal("true",cJSON_GetObjectItem(resParamObj, "value")->valuestring );
    assert_int_equal(WDMP_BOOLEAN, cJSON_GetObjectItem(resParamObj, "dataType")->valueint);
    assert_string_equal("Success",cJSON_GetObjectItem(resParamObj, "message")->valuestring );
    assert_int_equal(200, cJSON_GetObjectItem(response, "statusCode")->valueint);
    cJSON_Delete(response);
}

void test_singleWildcardGet()
{
    char *reqPayload = "{ \"names\":[\"Device.DeviceInfo.Webpa.\"],\"command\": \"GET\"}";
    char *transactionId = "aasfsdfgeh";
    char *resPayload = NULL;
    char *names[MAX_PARAMETER_LEN] = {"Device.DeviceInfo.Webpa.CMC", "Device.DeviceInfo.Webpa.CID", "Device.DeviceInfo.Webpa.Version"};
    char *values[MAX_PARAMETER_LEN] = {"32","abcd", "1"};
    cJSON *response = NULL, *paramArray = NULL, *resParamObj = NULL, *value = NULL, *valueObj = NULL;
    int count = 1, i=0;
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
        valueList[i][0].type = ccsp_string;
    }

    will_return(get_global_values, valueList);
    will_return(get_global_parameters_count, totalCount);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterValues, size, 1);

    processRequest(reqPayload, transactionId, &resPayload);
    WalInfo("resPayload : %s\n",resPayload);

    assert_non_null(resPayload);
    response = cJSON_Parse(resPayload);
    assert_non_null(response);
    paramArray = cJSON_GetObjectItem(response, "parameters");
    assert_int_equal(count, cJSON_GetArraySize(paramArray));
    resParamObj = cJSON_GetArrayItem(paramArray, 0);
    assert_int_equal(totalCount,cJSON_GetObjectItem(resParamObj, "parameterCount")->valueint);
    assert_string_equal("Device.DeviceInfo.Webpa.",cJSON_GetObjectItem(resParamObj, "name")->valuestring);
    value = cJSON_GetObjectItem(resParamObj, "value");
    assert_non_null(value);
    assert_int_equal(totalCount, cJSON_GetArraySize(value));
    for(i=0; i<totalCount; i++)
    {
        valueObj = cJSON_GetArrayItem(value, i);
        assert_non_null(valueObj);
        assert_string_equal(names[i],cJSON_GetObjectItem(valueObj, "name")->valuestring );
        assert_string_equal(values[i],cJSON_GetObjectItem(valueObj, "value")->valuestring );
        assert_int_equal(WDMP_STRING, cJSON_GetObjectItem(valueObj, "dataType")->valueint);
    }
    assert_int_equal(WDMP_NONE, cJSON_GetObjectItem(resParamObj, "dataType")->valueint);
    assert_string_equal("Success",cJSON_GetObjectItem(resParamObj, "message")->valuestring );
    assert_int_equal(200, cJSON_GetObjectItem(response, "statusCode")->valueint);
    cJSON_Delete(response);
}

void test_largeWildcardGet()
{
    char *reqPayload = "{ \"names\":[\"Device.DeviceInfo.\"],\"command\": \"GET\"}";
    char *transactionId = "aasfsdfgeh";
    char *resPayload = NULL;
    char *names[MAX_PARAMETER_LEN] = {"Device.DeviceInfo.Enable", "Device.DeviceInfo.Version","Device.DeviceInfo.Webpa.CMC", "Device.DeviceInfo.Webpa.CID", "Device.DeviceInfo.Webpa.Version"};
    char *values[MAX_PARAMETER_LEN] = {"true","2","32","abcd", "1"};
    cJSON *response = NULL, *paramArray = NULL, *resParamObj = NULL, *value = NULL, *valueObj = NULL;
    int count = 1, i=0, j=0;
    int totalCount = 5;

    getCompDetails();

    parameterValStruct_t **valueList = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*)*2);
    for(i = 0; i<2; i++)
    {
        valueList[i] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t));
        valueList[i][0].parameterName = (char *) malloc(sizeof(char) * MAX_PARAMETER_LEN);
        strncpy(valueList[i][0].parameterName, names[j],MAX_PARAMETER_LEN);
        valueList[i][0].parameterValue = (char *) malloc(sizeof(char) * MAX_PARAMETER_LEN);
        strncpy(valueList[i][0].parameterValue, values[j],MAX_PARAMETER_LEN);
        valueList[i][0].type = ccsp_string;
        j++;
    }

    parameterValStruct_t **valueList1 = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*)*3);
    for(i = 0; i<3; i++)
    {
        valueList1[i] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t));
        valueList1[i][0].parameterName = (char *) malloc(sizeof(char) * MAX_PARAMETER_LEN);
        strncpy(valueList1[i][0].parameterName, names[j],MAX_PARAMETER_LEN);
        valueList1[i][0].parameterValue = (char *) malloc(sizeof(char) * MAX_PARAMETER_LEN);
        strncpy(valueList1[i][0].parameterValue, values[j],MAX_PARAMETER_LEN);
        valueList1[i][0].type = ccsp_string;
        j++;
    }

    will_return(get_global_components, getDeviceInfoCompDetails());
    will_return(get_global_component_size, 2);
    expect_function_call(CcspBaseIf_discComponentSupportingNamespace);
    will_return(CcspBaseIf_discComponentSupportingNamespace, CCSP_SUCCESS);
    expect_function_call(free_componentStruct_t);

    will_return(get_global_values, valueList);
    will_return(get_global_parameters_count, 2);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterValues, size, 1);

    will_return(get_global_values, valueList1);
    will_return(get_global_parameters_count, 3);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterValues, size, 1);

    processRequest(reqPayload, transactionId, &resPayload);
    WalInfo("resPayload : %s\n",resPayload);

    assert_non_null(resPayload);
    response = cJSON_Parse(resPayload);
    assert_non_null(response);
    paramArray = cJSON_GetObjectItem(response, "parameters");
    assert_int_equal(count, cJSON_GetArraySize(paramArray));
    resParamObj = cJSON_GetArrayItem(paramArray, 0);
    assert_int_equal(totalCount,cJSON_GetObjectItem(resParamObj, "parameterCount")->valueint);
    assert_string_equal("Device.DeviceInfo.",cJSON_GetObjectItem(resParamObj, "name")->valuestring);
    value = cJSON_GetObjectItem(resParamObj, "value");
    assert_non_null(value);
    assert_int_equal(totalCount, cJSON_GetArraySize(value));
    for(i=0; i<totalCount; i++)
    {
        valueObj = cJSON_GetArrayItem(value, i);
        assert_non_null(valueObj);
        assert_string_equal(names[i],cJSON_GetObjectItem(valueObj, "name")->valuestring );
        assert_string_equal(values[i],cJSON_GetObjectItem(valueObj, "value")->valuestring );
        assert_int_equal(WDMP_STRING, cJSON_GetObjectItem(valueObj, "dataType")->valueint);
    }
    assert_int_equal(WDMP_NONE, cJSON_GetObjectItem(resParamObj, "dataType")->valueint);
    assert_string_equal("Success",cJSON_GetObjectItem(resParamObj, "message")->valuestring );
    assert_int_equal(200, cJSON_GetObjectItem(response, "statusCode")->valueint);
    cJSON_Delete(response);
}

void test_wildcardGetWithNoChilds()
{
    char *reqPayload = "{ \"names\":[\"Device.NAT.PortMapping.\"],\"command\": \"GET\"}";
    char *transactionId = "aasfsdfgeh";
    char *resPayload = NULL;
    cJSON *response = NULL, *paramArray = NULL, *resParamObj = NULL;
    int count = 1;
    int totalCount = 0;

    getCompDetails();
    will_return(get_global_parameters_count, totalCount);
    will_return(get_global_values, NULL);

    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterValues, size, 1);

    processRequest(reqPayload, transactionId, &resPayload);
    WalInfo("resPayload : %s\n",resPayload);

    assert_non_null(resPayload);
    response = cJSON_Parse(resPayload);
    assert_non_null(response);
    paramArray = cJSON_GetObjectItem(response, "parameters");
    assert_int_equal(count, cJSON_GetArraySize(paramArray));
    resParamObj = cJSON_GetArrayItem(paramArray, 0);
    assert_int_equal(totalCount,cJSON_GetObjectItem(resParamObj, "parameterCount")->valueint);
    assert_string_equal("Device.NAT.PortMapping.",cJSON_GetObjectItem(resParamObj, "name")->valuestring);
    assert_string_equal("EMPTY",cJSON_GetObjectItem(resParamObj, "value")->valuestring);
    assert_int_equal(200, cJSON_GetObjectItem(response, "statusCode")->valueint);
    cJSON_Delete(response);
}

void test_multipleParameterGet()
{
    char *reqPayload = "{ \"names\":[\"Device.WiFi.SSID.10001.name\",\"Device.WiFi.SSID.10002.Enable\"],\"command\": \"GET\"}";
    char *transactionId = "aasfsdfgeheqegehsa";
    char *resPayload = NULL;
    cJSON *response = NULL, *paramArray = NULL, *resParamObj = NULL;
    int count = 1, i=0;
    char *names[MAX_PARAMETER_LEN] = {"Device.WiFi.SSID.10001.name", "Device.WiFi.SSID.10002.Enable"};
    char *values[MAX_PARAMETER_LEN] = {"test1234","false"};
    int type[MAX_PARAMETER_LEN] = {ccsp_string, ccsp_boolean};
    int totalCount = 2;

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
    expect_value(CcspBaseIf_getParameterValues, size, 2);

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
        assert_int_equal(count,cJSON_GetObjectItem(resParamObj, "parameterCount")->valueint);
        assert_string_equal(names[i],cJSON_GetObjectItem(resParamObj, "name")->valuestring);
        assert_string_equal(values[i],cJSON_GetObjectItem(resParamObj, "value")->valuestring );
        assert_int_equal(type[i], cJSON_GetObjectItem(resParamObj, "dataType")->valueint);
        assert_string_equal("Success",cJSON_GetObjectItem(resParamObj, "message")->valuestring );
    }
    assert_int_equal(200, cJSON_GetObjectItem(response, "statusCode")->valueint);
    cJSON_Delete(response);
}

void test_mixedGet()
{
    char *reqPayload = "{ \"names\":[\"Device.WiFi.SSID.10001.name\",\"Device.Webpa.\"],\"command\": \"GET\"}";
    char *transactionId = "aasfsdfgeheqegehsa";
    char *resPayload = NULL;
    cJSON *response = NULL, *paramArray = NULL, *resParamObj = NULL, *value = NULL, *valueObj = NULL;
    int count = 2, i=0, j=0;
    char *getParamList[] = {"Device.WiFi.SSID.10001.name", "Device.Webpa."};
    char *names[MAX_PARAMETER_LEN] = {"Device.Webpa.Enable", "Device.Webpa.CID", "Device.Webpa.CMC", "Device.Webpa.Version"};
    char *values[MAX_PARAMETER_LEN] = {"false","abcd","32","1"};
    int type[MAX_PARAMETER_LEN] = {ccsp_boolean,ccsp_int,ccsp_string,ccsp_string};
    parameterValStruct_t **valueList = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*));
    valueList[0] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t));
    valueList[0][0].parameterName = (char *) malloc(sizeof(char) * MAX_PARAMETER_LEN);
    strncpy(valueList[0][0].parameterName, getParamList[0],MAX_PARAMETER_LEN);
    valueList[0][0].parameterValue = (char *) malloc(sizeof(char) * MAX_PARAMETER_LEN);
    strncpy(valueList[0][0].parameterValue, "test_mixedget",MAX_PARAMETER_LEN);
    valueList[0][0].type = ccsp_string;

    getCompDetails();
    parameterValStruct_t **valueList1 = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*)*4);
    for(i = 0; i<4; i++)
    {
        valueList1[i] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t));
        valueList1[i][0].parameterName = (char *) malloc(sizeof(char) * MAX_PARAMETER_LEN);
        strncpy(valueList1[i][0].parameterName, names[i],MAX_PARAMETER_LEN);
        valueList1[i][0].parameterValue = (char *) malloc(sizeof(char) * MAX_PARAMETER_LEN);
        strncpy(valueList1[i][0].parameterValue, values[i],MAX_PARAMETER_LEN);
        valueList1[i][0].type = type[i];
    }

    will_return(get_global_values, valueList);
    will_return(get_global_parameters_count, 1);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterValues, size, 1);

    will_return(get_global_values, valueList1);
    will_return(get_global_parameters_count, 4);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterValues, size, 1);

    processRequest(reqPayload, transactionId, &resPayload);
    WalInfo("resPayload : %s\n",resPayload);

    assert_non_null(resPayload);
    response = cJSON_Parse(resPayload);
    assert_non_null(response);
    assert_int_equal(200, cJSON_GetObjectItem(response, "statusCode")->valueint);
    paramArray = cJSON_GetObjectItem(response, "parameters");
    assert_int_equal(count, cJSON_GetArraySize(paramArray));
    for(i=0; i<count; i++)
    {
        resParamObj = cJSON_GetArrayItem(paramArray, i);
        assert_string_equal(getParamList[i],cJSON_GetObjectItem(resParamObj, "name")->valuestring);
        if(cJSON_GetObjectItem(resParamObj, "parameterCount")->valueint > 1)
        {
            assert_int_equal(4,cJSON_GetObjectItem(resParamObj, "parameterCount")->valueint);
            value = cJSON_GetObjectItem(resParamObj, "value");
            assert_non_null(value);
            assert_int_equal(4, cJSON_GetArraySize(value));
            for(j=0;j<4;j++)
            {
                valueObj = cJSON_GetArrayItem(value, j);
                assert_non_null(valueObj);
                assert_string_equal(names[j],cJSON_GetObjectItem(valueObj, "name")->valuestring );
                assert_string_equal(values[j],cJSON_GetObjectItem(valueObj, "value")->valuestring );
                assert_int_equal(type[j], cJSON_GetObjectItem(valueObj, "dataType")->valueint);
            }
            assert_int_equal(WDMP_NONE, cJSON_GetObjectItem(resParamObj, "dataType")->valueint);
            assert_string_equal("Success",cJSON_GetObjectItem(resParamObj, "message")->valuestring );
        }
        else
        {
            assert_int_equal(1,cJSON_GetObjectItem(resParamObj, "parameterCount")->valueint);
            assert_string_equal("test_mixedget",cJSON_GetObjectItem(resParamObj, "value")->valuestring );
            assert_int_equal(WDMP_STRING, cJSON_GetObjectItem(resParamObj, "dataType")->valueint);
            assert_string_equal("Success",cJSON_GetObjectItem(resParamObj, "message")->valuestring );
        }
    }
    cJSON_Delete(response);
}

void test_multipleParameterGetWithDifferentComponents()
{
    char *reqPayload = "{ \"names\":[\"Device.WiFi.SSID.10001.name\",\"Device.Webpa.Enable\",\"Device.WiFi.SSID.10002.Enable\",\"Device.DeviceInfo.Webpa.Version\",\"Device.NAT.PortMapping.\"],\"command\": \"GET\"}";
    char *transactionId = "aasfsdfgeheqegehsa";
    char *resPayload = NULL;
    cJSON *response = NULL, *paramArray = NULL, *resParamObj = NULL;
    int count = 5, i=0, j=0;
    char *wifiNames[MAX_PARAMETER_LEN] = {"Device.WiFi.SSID.10001.name", "Device.WiFi.SSID.10002.Enable"};
    char *wifiValues[MAX_PARAMETER_LEN] = {"test1234","false"};
    int wifiType[MAX_PARAMETER_LEN] = {ccsp_string, ccsp_boolean};
    char *webpaNames[MAX_PARAMETER_LEN] = {"Device.Webpa.Enable", "Device.DeviceInfo.Webpa.Version"};
    char *webpaValues[MAX_PARAMETER_LEN] = {"true","2"};
    int webpaType[MAX_PARAMETER_LEN] = {ccsp_boolean,ccsp_int};

    getCompDetails();
    parameterValStruct_t **valueList = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*)*2);
    for(i = 0; i<2; i++)
    {
        valueList[i] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t));
        valueList[i][0].parameterName = (char *) malloc(sizeof(char) * MAX_PARAMETER_LEN);
        strncpy(valueList[i][0].parameterName, wifiNames[i],MAX_PARAMETER_LEN);
        valueList[i][0].parameterValue = (char *) malloc(sizeof(char) * MAX_PARAMETER_LEN);
        strncpy(valueList[i][0].parameterValue, wifiValues[i],MAX_PARAMETER_LEN);
        valueList[i][0].type = wifiType[i];
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

    will_return(get_global_values, valueList);
    will_return(get_global_parameters_count, 2);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterValues, size, 2);

    will_return(get_global_values, valueList1);
    will_return(get_global_parameters_count, 2);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterValues, size, 2);

    will_return(get_global_values, NULL);
    will_return(get_global_parameters_count, 0);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterValues, size, 1);

    processRequest(reqPayload, transactionId, &resPayload);
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
        assert_int_equal(1,cJSON_GetObjectItem(resParamObj, "parameterCount")->valueint);
        assert_string_equal(wifiNames[i],cJSON_GetObjectItem(resParamObj, "name")->valuestring);
        assert_string_equal(wifiValues[i],cJSON_GetObjectItem(resParamObj, "value")->valuestring );
        assert_int_equal(wifiType[i], cJSON_GetObjectItem(resParamObj, "dataType")->valueint);
        assert_string_equal("Success",cJSON_GetObjectItem(resParamObj, "message")->valuestring );
    }

    for(i=2; i<count-1; i++)
    {
        resParamObj = cJSON_GetArrayItem(paramArray, i);
        assert_int_equal(1,cJSON_GetObjectItem(resParamObj, "parameterCount")->valueint);
        assert_string_equal(webpaNames[j],cJSON_GetObjectItem(resParamObj, "name")->valuestring);
        assert_string_equal(webpaValues[j],cJSON_GetObjectItem(resParamObj, "value")->valuestring );
        assert_int_equal(webpaType[j], cJSON_GetObjectItem(resParamObj, "dataType")->valueint);
        assert_string_equal("Success",cJSON_GetObjectItem(resParamObj, "message")->valuestring );
        j++;
    }

    resParamObj = cJSON_GetArrayItem(paramArray, count-1);
    assert_int_equal(0,cJSON_GetObjectItem(resParamObj, "parameterCount")->valueint);
    assert_string_equal("Device.NAT.PortMapping.",cJSON_GetObjectItem(resParamObj, "name")->valuestring);
    assert_string_equal("EMPTY",cJSON_GetObjectItem(resParamObj, "value")->valuestring );
    cJSON_Delete(response);
}

void err_singleGetInvalidParam()
{
    char *reqPayload = "{ \"names\":[\"Device.WiFi.SSID.10001.En\"],\"command\": \"GET\"}";
    char *transactionId = "erejujaasfsdfgeh";
    char *resPayload = NULL;
    cJSON *response = NULL;

    getCompDetails();
    will_return(get_global_parameters_count,0);
    will_return(get_global_values,NULL);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_ERR_INVALID_PARAMETER_NAME);
    expect_value(CcspBaseIf_getParameterValues, size, 1);

    processRequest(reqPayload, transactionId, &resPayload);
    WalInfo("resPayload : %s\n",resPayload);

    assert_non_null(resPayload);
    response = cJSON_Parse(resPayload);
    assert_non_null(response);
    assert_string_equal("Invalid parameter name",cJSON_GetObjectItem(response, "message")->valuestring );
    assert_int_equal(520, cJSON_GetObjectItem(response, "statusCode")->valueint);
    cJSON_Delete(response);
}

void err_singleGetComponentErr()
{
    char *reqPayload = "{ \"names\":[\"Device.DeviceInfo.Webp.\"],\"command\": \"GET\"}";
    char *transactionId = "erejujaasfsdfgeh";
    char *resPayload = NULL;
    cJSON *response = NULL;

    will_return(get_global_component_size,0);
    will_return(get_global_components,NULL);

    expect_function_call(CcspBaseIf_discComponentSupportingNamespace);
    will_return(CcspBaseIf_discComponentSupportingNamespace, CCSP_CR_ERR_UNSUPPORTED_NAMESPACE);
    expect_function_call(free_componentStruct_t);

    processRequest(reqPayload, transactionId, &resPayload);
    WalInfo("resPayload : %s\n",resPayload);

    assert_non_null(resPayload);
    response = cJSON_Parse(resPayload);
    assert_non_null(response);
    assert_string_equal("Error unsupported namespace",cJSON_GetObjectItem(response, "message")->valuestring );
    assert_int_equal(520, cJSON_GetObjectItem(response, "statusCode")->valueint);
    cJSON_Delete(response);
}

void err_singleGetLargeReq()
{
    char *reqPayload = "{ \"names\":[\"Device.abcd.1.sfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGsfdgfgherejrehigeiruwegwegwiegfuwgfegfwegfwefegaugdfosFYQWPIYRSDAWIEUFTGJVHEDFKWJESDFWEGFWEGFGAQWWEGYTEHERHAEGTWERHTBQR4WYTR\"],\"command\": \"GET\"}";
    char *transactionId = "erejujaasfsdfgeh";
    char *resPayload = NULL;
    cJSON *response = NULL;

    processRequest(reqPayload, transactionId, &resPayload);
    WalInfo("resPayload : %s\n",resPayload);

    assert_non_null(resPayload);
    response = cJSON_Parse(resPayload);
    assert_non_null(response);
    assert_string_equal("Invalid Param",cJSON_GetObjectItem(response, "message")->valuestring );
    assert_int_equal(520, cJSON_GetObjectItem(response, "statusCode")->valueint);
    cJSON_Delete(response);
}

void err_getWithWiFiBusy()
{
    char *reqPayload = "{ \"names\":[\"Device.WiFi.Enable\"],\"command\": \"GET\"}";
    char *transactionId = "erejujaasfsdfgeh";
    char *resPayload = NULL;
    cJSON *response = NULL;

    applySettingsFlag = TRUE;
    processRequest(reqPayload, transactionId, &resPayload);
    WalInfo("resPayload : %s\n",resPayload);

    assert_non_null(resPayload);
    response = cJSON_Parse(resPayload);
    assert_non_null(response);
    assert_string_equal("WiFi is busy",cJSON_GetObjectItem(response, "message")->valuestring );
    assert_int_equal(530, cJSON_GetObjectItem(response, "statusCode")->valueint);
    cJSON_Delete(response);
}

void err_getWithInvalidWiFiIndex()
{
    char *reqPayload = "{ \"names\":[\"Device.WiFi.SSID.1.\"],\"command\": \"GET\"}";
    char *transactionId = "erejujaasfsdfgeh";
    char *resPayload = NULL;
    cJSON *response = NULL;

    applySettingsFlag = FALSE;
    processRequest(reqPayload, transactionId, &resPayload);
    WalInfo("resPayload : %s\n",resPayload);

    assert_non_null(resPayload);
    response = cJSON_Parse(resPayload);
    assert_non_null(response);
    assert_string_equal("Invalid WiFi index, valid range is between 10001-10008 and 10101-10108",cJSON_GetObjectItem(response, "message")->valuestring );
    assert_int_equal(520, cJSON_GetObjectItem(response, "statusCode")->valueint);
    cJSON_Delete(response);
}

void err_getWithInvalidRadioIndex()
{
    char *reqPayload = "{ \"names\":[\"Device.WiFi.Radio.10001.\"],\"command\": \"GET\"}";
    char *transactionId = "erejujaasfsdfgeh";
    char *resPayload = NULL;
    cJSON *response = NULL;
    applySettingsFlag = FALSE;

    processRequest(reqPayload, transactionId, &resPayload);
    WalInfo("resPayload : %s\n",resPayload);

    assert_non_null(resPayload);
    response = cJSON_Parse(resPayload);
    assert_non_null(response);
    assert_string_equal("Invalid Radio index, valid idexes are 10000 and 10100",cJSON_GetObjectItem(response, "message")->valuestring );
    assert_int_equal(520, cJSON_GetObjectItem(response, "statusCode")->valueint);
    cJSON_Delete(response);
}

void err_multipleGet()
{
    char *reqPayload = "{ \"names\":[\"Device.WiFi.Radio.10000.Enable\",\"Device.NAT.PortMapping.\",\"Device.SSID.aadd\"],\"command\": \"GET\"}";
    char *transactionId = "erejujaasfsdfgeh";
    char *resPayload = NULL;
    cJSON *response = NULL;

    applySettingsFlag = FALSE;
    getCompDetails();
    will_return(get_global_component_size,0);
    will_return(get_global_components,NULL);
    expect_function_call(CcspBaseIf_discComponentSupportingNamespace);
    will_return(CcspBaseIf_discComponentSupportingNamespace, CCSP_CR_ERR_UNSUPPORTED_NAMESPACE);
    expect_function_call(free_componentStruct_t);

    will_return(get_global_values, NULL);
    will_return(get_global_parameters_count, 0);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterValues, size, 1);

    processRequest(reqPayload, transactionId, &resPayload);
    WalInfo("resPayload : %s\n",resPayload);

    assert_non_null(resPayload);
    response = cJSON_Parse(resPayload);
    assert_non_null(response);
    assert_int_equal(520, cJSON_GetObjectItem(response, "statusCode")->valueint);
    cJSON_Delete(response);
}

void err_multipleGetWildCardErr()
{
    char *reqPayload = "{ \"names\":[\"Device.DeviceInfo.X_RDKCENTRAL-COM_xOpsDeviceMgmt.Mesh.\",\"Device.WiFi.SSID.10007.SSID\",\"Device.WiFi.SSID.10107.SSID\"],\"command\": \"GET\"}";
    char *transactionId = "erejujaasfsdfgeh";
    char *wifiNames[MAX_PARAMETER_LEN] = {"Device.WiFi.SSID.10007.SSID", "Device.WiFi.SSID.10107.SSID"};
    char *wifiValues[MAX_PARAMETER_LEN] = {"testssid7","testssid17"};
    int wifiType[MAX_PARAMETER_LEN] = {ccsp_string, ccsp_string};
    char *resPayload = NULL;
    cJSON *response = NULL;
    int i;

    applySettingsFlag = FALSE;
    getCompDetails();

    parameterValStruct_t **valueList = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*)*2);
    for(i = 0; i<2; i++)
    {
        valueList[i] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t));
        valueList[i][0].parameterName = (char *) malloc(sizeof(char) * MAX_PARAMETER_LEN);
        strncpy(valueList[i][0].parameterName, wifiNames[i],MAX_PARAMETER_LEN);
        valueList[i][0].parameterValue = (char *) malloc(sizeof(char) * MAX_PARAMETER_LEN);
        strncpy(valueList[i][0].parameterValue, wifiValues[i],MAX_PARAMETER_LEN);
        valueList[i][0].type = wifiType[i];
    }

    will_return(get_global_values, valueList);
    will_return(get_global_parameters_count, 2);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterValues, size, 2);

    will_return(get_global_components, NULL);
    will_return(get_global_component_size, 0);
    expect_function_call(CcspBaseIf_discComponentSupportingNamespace);
    will_return(CcspBaseIf_discComponentSupportingNamespace, CCSP_CR_ERR_UNSUPPORTED_NAMESPACE);
    expect_function_call(free_componentStruct_t);

    processRequest(reqPayload, transactionId, &resPayload);
    WalInfo("resPayload : %s\n",resPayload);

    assert_non_null(resPayload);
    response = cJSON_Parse(resPayload);
    assert_non_null(response);
    assert_int_equal(520, cJSON_GetObjectItem(response, "statusCode")->valueint);
    cJSON_Delete(response);
}

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_singleGet),
        cmocka_unit_test(test_singleWildcardGet),
        cmocka_unit_test(test_wildcardGetWithNoChilds),
        cmocka_unit_test(test_largeWildcardGet),
        cmocka_unit_test(test_multipleParameterGet),
        cmocka_unit_test(test_mixedGet),
        cmocka_unit_test(test_multipleParameterGetWithDifferentComponents),
        cmocka_unit_test(err_singleGetInvalidParam),
        cmocka_unit_test(err_singleGetComponentErr),
        cmocka_unit_test(err_singleGetLargeReq),
        cmocka_unit_test(err_getWithWiFiBusy),
        cmocka_unit_test(err_getWithInvalidWiFiIndex),
        cmocka_unit_test(err_getWithInvalidRadioIndex),
        cmocka_unit_test(err_multipleGet),
        cmocka_unit_test(err_multipleGetWildCardErr)
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
