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

    componentStruct_t **compList = (componentStruct_t **) malloc(sizeof(componentStruct_t *));
    compList[0] = (componentStruct_t *) malloc(sizeof(componentStruct_t));
    compList[0]->componentName = (char *) malloc(sizeof(char) * MAX_PARAMETER_LEN);
    strncpy(compList[0]->componentName, "com.ccsp.webpa",MAX_PARAMETER_LEN);
    compList[0]->dbusPath = (char *) malloc(sizeof(char) * MAX_PARAMETER_LEN);
    strncpy(compList[0]->dbusPath, "/com/ccsp/webpa",MAX_PARAMETER_LEN);

    int totalCount = 1;
    parameterValStruct_t **valueList = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*));
    valueList[0] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t)*totalCount);
    valueList[0]->parameterName = (char *) malloc(sizeof(char) * MAX_PARAMETER_LEN);
    strncpy(valueList[0]->parameterName, "Device.DeviceInfo.Webpa.Enable",MAX_PARAMETER_LEN);
    valueList[0]->parameterValue = (char *) malloc(sizeof(char) * MAX_PARAMETER_LEN);
    strncpy(valueList[0]->parameterValue, "true",MAX_PARAMETER_LEN);
    valueList[0]->type = ccsp_boolean;
    
    will_return(get_global_components, compList);
    will_return(get_global_component_size, 1);
    expect_function_call(CcspBaseIf_discComponentSupportingNamespace);
    will_return(CcspBaseIf_discComponentSupportingNamespace, CCSP_SUCCESS);
    expect_function_call(free_componentStruct_t);
    
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

    componentStruct_t **compList = (componentStruct_t **) malloc(sizeof(componentStruct_t *));
    compList[0] = (componentStruct_t *) malloc(sizeof(componentStruct_t));
    compList[0]->componentName = (char *) malloc(sizeof(char) * MAX_PARAMETER_LEN);
    strncpy(compList[0]->componentName, "com.ccsp.webpa",MAX_PARAMETER_LEN);
    compList[0]->dbusPath = (char *) malloc(sizeof(char) * MAX_PARAMETER_LEN);
    strncpy(compList[0]->dbusPath, "/com/ccsp/webpa",MAX_PARAMETER_LEN);

    int totalCount = 3;
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
    
    will_return(get_global_components, compList);
    will_return(get_global_component_size, 1);
    expect_function_call(CcspBaseIf_discComponentSupportingNamespace);
    will_return(CcspBaseIf_discComponentSupportingNamespace, CCSP_SUCCESS);
    expect_function_call(free_componentStruct_t);
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

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_singleGet),
        cmocka_unit_test(test_singleWildcardGet),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
