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

#include "../source/include/webpa_adapter.h"
#include <cimplog/cimplog.h>
#include <wdmp-c.h>
#include <cJSON.h>
#include "ccsp_dm_api.h"

#define UNUSED(x) (void )(x)
#define MAX_PARAMETER_LEN			512
/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
param_t **parameterList;
int count;
WDMP_STATUS status;
/*----------------------------------------------------------------------------*/
/*                                   Mocks                                    */
/*----------------------------------------------------------------------------*/
void getValues(const char *paramName[], const unsigned int paramCount, int index, money_trace_spans *timeSpan, param_t ***paramArr, int *retValCount, WDMP_STATUS *retStatus)
{
    UNUSED(paramName); UNUSED(paramCount); UNUSED(index); UNUSED(timeSpan);
    check_expected(paramCount);
    check_expected(index);
    *paramArr = parameterList;
    *retValCount = count;
    *retStatus = status;
    function_called();
}

void getAttributes(const char *paramName[], const unsigned int paramCount, money_trace_spans *timeSpan, param_t **attr, int *retAttrCount, WDMP_STATUS *retStatus)
{
    UNUSED(paramName); UNUSED(paramCount); UNUSED(timeSpan); UNUSED(attr); UNUSED(retAttrCount); UNUSED(retStatus);
    function_called();
}

void setValues(const param_t paramVal[], const unsigned int paramCount, const WEBPA_SET_TYPE setType, char *transactionId, money_trace_spans *timeSpan, WDMP_STATUS *retStatus, int *ccspStatus)
{
    UNUSED(paramVal); UNUSED(paramCount); UNUSED(setType); UNUSED(transactionId); UNUSED(timeSpan); UNUSED(retStatus); UNUSED(ccspStatus);
    function_called();
}

void setAttributes(param_t *attArr, const unsigned int paramCount, money_trace_spans *timeSpan, WDMP_STATUS *retStatus)
{
    UNUSED(attArr); UNUSED(paramCount); UNUSED(timeSpan); UNUSED(retStatus);
    function_called();
}

void addRowTable(char *objectName, TableData *list,char **retObject, WDMP_STATUS *retStatus)
{
    UNUSED(objectName); UNUSED(list); UNUSED(retObject); UNUSED(retStatus);
    function_called();
}
void deleteRowTable(char *object,WDMP_STATUS *retStatus)
{
    UNUSED(object); UNUSED(retStatus);
    function_called();
}

void replaceTable(char *objectName,TableData * list,unsigned int paramcount,WDMP_STATUS *retStatus)
{
    UNUSED(objectName); UNUSED(list); UNUSED(paramcount); UNUSED(retStatus);
    function_called();
}

char * getParameterValue(char *paramName)
{
    UNUSED(paramName);
    function_called();
    return (char *) mock();
}

WDMP_STATUS setParameterValue(char *paramName, char* value, DATA_TYPE type)
{
    UNUSED(paramName); UNUSED(value); UNUSED(type);
    function_called();
    return (WDMP_STATUS) mock();
}

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

void test_processRequest_singleGet()
{
    char *reqPayload = "{ \"names\":[\"Device.DeviceInfo.Webpa.Enable\"],\"command\": \"GET\"}";
    char *transactionId = "aasfsdfgeh"; 
    char *resPayload = NULL;
    cJSON *response = NULL, *paramArray = NULL, *resParamObj = NULL;
    count = 1;
    parameterList = (param_t **) malloc(sizeof(param_t*));
    parameterList[0] = (param_t *) malloc(sizeof(param_t));
    parameterList[0]->name = (char *) malloc(sizeof(char) * MAX_PARAMETER_LEN);
    strncpy(parameterList[0]->name, "Device.DeviceInfo.Webpa.Enable",MAX_PARAMETER_LEN);
    parameterList[0]->value = (char *) malloc(sizeof(char) * MAX_PARAMETER_LEN);
    strncpy(parameterList[0]->value, "true",MAX_PARAMETER_LEN);
    parameterList[0]->type = WDMP_BOOLEAN;
    status = WDMP_SUCCESS;
    expect_value(getValues, paramCount, 1);
    expect_value(getValues, index, 0);
    expect_function_call(getValues);
    processRequest(reqPayload, transactionId, &resPayload);
    WalInfo("resPayload : %s\n",resPayload);
    assert_non_null(resPayload);
    response = cJSON_Parse(resPayload);
    assert_non_null(response);
    paramArray = cJSON_GetObjectItem(response, "parameters");
    assert_int_equal(1, cJSON_GetArraySize(paramArray));
    resParamObj = cJSON_GetArrayItem(paramArray, 0);
    assert_int_equal(count,cJSON_GetObjectItem(resParamObj, "parameterCount")->valueint);
    assert_string_equal("Device.DeviceInfo.Webpa.Enable",cJSON_GetObjectItem(resParamObj, "name")->valuestring);
    assert_string_equal("true",cJSON_GetObjectItem(resParamObj, "value")->valuestring );
    assert_int_equal(WDMP_BOOLEAN, cJSON_GetObjectItem(resParamObj, "dataType")->valueint);
    assert_string_equal("Success",cJSON_GetObjectItem(resParamObj, "message")->valuestring );
    assert_int_equal(200, cJSON_GetObjectItem(response, "statusCode")->valueint);
    if(resPayload !=NULL)
    {
	free(resPayload);
    }
    cJSON_Delete(response);
}

void test_processRequest_WildcardsGet()
{
    char *reqPayload = "{ \"names\":[\"Device.DeviceInfo.Webpa.\"],\"command\": \"GET\"}";
    char *transactionId = "aasfsdfgehhdysy";
    char *resPayload = NULL;
    int i = 0;
    cJSON *response = NULL, *paramArray = NULL, *resParamObj = NULL, *value = NULL, *valueObj = NULL;
    count = 3;
    char *names[MAX_PARAMETER_LEN] = {"Device.DeviceInfo.Webpa.CMC", "Device.DeviceInfo.Webpa.CID", "Device.DeviceInfo.Webpa.Version"};
    char *values[MAX_PARAMETER_LEN] = {"32","abcd", "1"};
    parameterList = (param_t **) malloc(sizeof(param_t*));
    parameterList[0] = (param_t *) malloc(sizeof(param_t)*count);

    for(i = 0; i<count; i++)
    {
        parameterList[0][i].name = (char *) malloc(sizeof(char) * MAX_PARAMETER_LEN);
        strncpy(parameterList[0][i].name, names[i],MAX_PARAMETER_LEN);
        parameterList[0][i].value = (char *) malloc(sizeof(char) * MAX_PARAMETER_LEN);
        strncpy(parameterList[0][i].value, values[i],MAX_PARAMETER_LEN);
        parameterList[0][i].type = WDMP_STRING;
    }
    status = WDMP_SUCCESS;
    expect_value(getValues, paramCount, 1);
    expect_value(getValues, index, 0);
    expect_function_call(getValues);
    processRequest(reqPayload, transactionId, &resPayload);
    WalInfo("resPayload : %s\n",resPayload);
    assert_non_null(resPayload);
    response = cJSON_Parse(resPayload);
    assert_non_null(response);
    paramArray = cJSON_GetObjectItem(response, "parameters");
    assert_int_equal(1, cJSON_GetArraySize(paramArray));
    resParamObj = cJSON_GetArrayItem(paramArray, 0);
    assert_int_equal(count,cJSON_GetObjectItem(resParamObj, "parameterCount")->valueint);
    assert_string_equal("Device.DeviceInfo.Webpa.",cJSON_GetObjectItem(resParamObj, "name")->valuestring);
    value = cJSON_GetObjectItem(resParamObj, "value");
    assert_non_null(value);
    assert_int_equal(count, cJSON_GetArraySize(value));
    for(i=0; i<count; i++)
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
    if(resPayload !=NULL)
    {
	free(resPayload);
    }
    cJSON_Delete(response);
}

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_processRequest_singleGet),
        cmocka_unit_test(test_processRequest_WildcardsGet),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
