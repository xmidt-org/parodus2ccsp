/**
 *  Copyright 2010-2013 Comcast Cable Communications Management, LLC
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
#include <rbus/rbus.h>
#include <string.h>
#include "../source/broadband/include/webpa_internal.h"
#include "../source/include/webpa_adapter.h"
#include <cimplog/cimplog.h>
#include <wdmp-c.h>
#include <cJSON.h>
#include "ccsp_dm_api.h"

#define UNUSED(x) (void )(x)
#define MAX_PARAMETER_LEN			512

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

void test_cid_test_filed()
{
    char *reqPayload = "{\"command\":\"TEST_AND_SET\",\"old-cid\":\"61f4db9\",\"new-cid\":\"0\",\"sync-cmc\":\"512\",\"parameters\":[{\"name\":\"Device.X_RDK_WebConfig.URL\",\"dataType\":0,\"value\":\"https://webpa.com\",\"attributes\":{\"notify\":1}}]}";
    char *transactionId = "aasfsdfgehhdysy";
    char *resPayload = NULL;    
    headers_t *res_headers = NULL;
    headers_t *req_headers = NULL;
    cJSON *response = NULL; 

    getCompDetails();

    parameterValStruct_t **cmcList1 = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*));
    cmcList1[0] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t)*1);
    cmcList1[0]->parameterName = strndup(PARAM_CMC,MAX_PARAMETER_LEN);
    cmcList1[0]->parameterValue = strndup("512",MAX_PARAMETER_LEN);
    cmcList1[0]->type = ccsp_int;
    will_return(get_global_values, cmcList1);
    will_return(get_global_parameters_count, 1);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterValues, size, 1);

    parameterValStruct_t **cidList = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*));
    cidList[0] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t)*1);
    cidList[0]->parameterName = strndup(PARAM_CID,MAX_PARAMETER_LEN);
    cidList[0]->parameterValue = strndup("0",MAX_PARAMETER_LEN);
    cidList[0]->type = ccsp_string;
    will_return(get_global_values, cidList);
    will_return(get_global_parameters_count, 1);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterValues, size, 1);

    processRequest(reqPayload, transactionId, &resPayload, req_headers, res_headers);
    WalInfo("resPayload : %s\n",resPayload);
    assert_non_null(resPayload);
    response = cJSON_Parse(resPayload);
    assert_non_null(response);
    assert_string_equal("CID test failed",cJSON_GetObjectItem(response, "message")->valuestring);
    assert_int_equal(550, cJSON_GetObjectItem(response, "statusCode")->valueint);

    if(resPayload !=NULL)
    {
	    free(resPayload);
    }
}

void test_processRequest_test_and_set()
{
    char *reqPayload = "{\"command\":\"TEST_AND_SET\",\"old-cid\":\"61f4db9\",\"new-cid\":\"0\",\"sync-cmc\":\"512\",\"parameters\":[{\"name\":\"Device.X_RDK_WebConfig.URL\",\"dataType\":0,\"value\":\"https://webpa.com\",\"attributes\":{\"notify\":1}}]}";
    char *transactionId = "aasfsdfgehhdysy";
    char *resPayload = NULL;    
    headers_t *res_headers = NULL;
    headers_t *req_headers = NULL;
    cJSON *response = NULL; 

    getCompDetails();

    parameterValStruct_t **cmcList1 = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*));
    cmcList1[0] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t)*1);
    cmcList1[0]->parameterName = strndup(PARAM_CMC,MAX_PARAMETER_LEN);
    cmcList1[0]->parameterValue = strndup("512",MAX_PARAMETER_LEN);
    cmcList1[0]->type = ccsp_int;
    will_return(get_global_values, cmcList1);
    will_return(get_global_parameters_count, 1);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterValues, size, 1);

    parameterValStruct_t **cidList = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*));
    cidList[0] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t)*1);
    cidList[0]->parameterName = strndup(PARAM_CID,MAX_PARAMETER_LEN);
    cidList[0]->parameterValue = strndup("61f4db9",MAX_PARAMETER_LEN);
    cidList[0]->type = ccsp_string;
    will_return(get_global_values, cidList);
    will_return(get_global_parameters_count, 1);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterValues, size, 1);

    parameterValStruct_t **cid2List = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*));
    cid2List[0] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t)*1);
    cid2List[0]->parameterName = strndup(PARAM_CID,MAX_PARAMETER_LEN);
    cid2List[0]->parameterValue = strndup("61f4db9",MAX_PARAMETER_LEN);
    cid2List[0]->type = ccsp_string;
    will_return(get_global_values, cid2List);
    will_return(get_global_parameters_count, 1);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterValues, size, 1);    

    will_return(get_global_faultParam, NULL);
    will_return(CcspBaseIf_setParameterValues, CCSP_SUCCESS);
    expect_function_call(CcspBaseIf_setParameterValues);
    expect_value(CcspBaseIf_setParameterValues, size, 1);

    componentStruct_t **list = (componentStruct_t **) malloc(sizeof(componentStruct_t *)*1);
    list[0] = (componentStruct_t *) malloc(sizeof(componentStruct_t));
    list[0]->componentName = strdup("Device.X_RDK_WebConfig.");
    list[0]->dbusPath = strdup(RDKB_WEBPA_COMPONENT_NAME);
    will_return(get_global_components, list);
    will_return(get_global_component_size, 1);
    expect_function_call(CcspBaseIf_discComponentSupportingNamespace);
    will_return(CcspBaseIf_discComponentSupportingNamespace, CCSP_SUCCESS);
    expect_function_call(free_componentStruct_t);

    parameterValStruct_t **valueList = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*));
    valueList[0] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t)*1);
    valueList[0]->parameterName = (char *) malloc(sizeof(char) * MAX_PARAMETER_LEN);
    strncpy(valueList[0]->parameterName, "Device.X_RDK_WebConfig.URL",MAX_PARAMETER_LEN);
    valueList[0]->parameterValue = (char *) malloc(sizeof(char) * MAX_PARAMETER_LEN);
    strncpy(valueList[0]->parameterValue, "https://webpa.com",MAX_PARAMETER_LEN);
    valueList[0]->type = ccsp_string;
    will_return(get_global_values, valueList);
    will_return(get_global_parameters_count, 1);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterValues, size, 1); 

    will_return(get_global_faultParam, NULL);
    will_return(CcspBaseIf_setParameterValues, CCSP_SUCCESS);
    expect_function_call(CcspBaseIf_setParameterValues);
    expect_value(CcspBaseIf_setParameterValues, size, 1);       

    processRequest(reqPayload, transactionId, &resPayload, req_headers, res_headers);
    WalInfo("resPayload : %s\n",resPayload);   
    assert_non_null(resPayload);
    response = cJSON_Parse(resPayload);
    assert_non_null(response);
    assert_string_equal("Success",cJSON_GetObjectItem(response, "message")->valuestring);
    assert_int_equal(200, cJSON_GetObjectItem(response, "statusCode")->valueint);

    if(resPayload !=NULL)
    {
	    free(resPayload);
    }
}


void test2_test_and_set_with_reboot_parameter()
{
    char *reqPayload = "{\"command\":\"TEST_AND_SET\",\"old-cid\":\"61f4db9\",\"new-cid\":\"0\",\"sync-cmc\":\"512\",\"parameters\":[{\"name\":\"Device.X_CISCO_COM_DeviceControl.RebootDevice\",\"dataType\":0,\"value\":\"Device\",\"attributes\":{\"notify\":1}}]}";
    char *transactionId = "aasfsdfgehhdysy";
    char *resPayload = NULL;    
    headers_t *res_headers = NULL;
    headers_t *req_headers = NULL;
    cJSON *response = NULL; 

    res_headers = (headers_t *)malloc(sizeof(headers_t) + sizeof( char * ));
    req_headers = (headers_t *)malloc(sizeof(headers_t) + sizeof( char * ));
    memset(res_headers, 0, sizeof(headers_t));
    memset(req_headers, 0, sizeof(headers_t));
    req_headers->count = 2;
    req_headers->headers[0] = strdup("traceparent: 00-foo-bar-05");
    req_headers->headers[1] = strdup("tracestate: congo=00k056ccc0tu34d56");

    getCompDetails();

    parameterValStruct_t **cmcList1 = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*));
    cmcList1[0] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t)*1);
    cmcList1[0]->parameterName = strndup(PARAM_CMC,MAX_PARAMETER_LEN);
    cmcList1[0]->parameterValue = strndup("512",MAX_PARAMETER_LEN);
    cmcList1[0]->type = ccsp_int;
    will_return(get_global_values, cmcList1);
    will_return(get_global_parameters_count, 1);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterValues, size, 1);

    parameterValStruct_t **cidList = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*));
    cidList[0] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t)*1);
    cidList[0]->parameterName = strndup(PARAM_CID,MAX_PARAMETER_LEN);
    cidList[0]->parameterValue = strndup("61f4db9",MAX_PARAMETER_LEN);
    cidList[0]->type = ccsp_string;
    will_return(get_global_values, cidList);
    will_return(get_global_parameters_count, 1);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterValues, size, 1);

    componentStruct_t **list1 = (componentStruct_t **) malloc(sizeof(componentStruct_t *)*1);
    list1[0] = (componentStruct_t *) malloc(sizeof(componentStruct_t));
    list1[0]->componentName = strdup("Device.X_CISCO_COM_DeviceControl.");
    list1[0]->dbusPath = strdup(RDKB_WEBPA_COMPONENT_NAME);
    will_return(get_global_components, list1);
    will_return(get_global_component_size, 1);
    expect_function_call(CcspBaseIf_discComponentSupportingNamespace);
    will_return(CcspBaseIf_discComponentSupportingNamespace, CCSP_SUCCESS);
    expect_function_call(free_componentStruct_t);

	parameterValStruct_t **valueList1 = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*));
    valueList1[0] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t)*1);
    valueList1[0]->parameterName = (char *) malloc(sizeof(char) * MAX_PARAMETER_LEN);
    strncpy(valueList1[0]->parameterName, "Device.X_CISCO_COM_DeviceControl.RebootDevice",MAX_PARAMETER_LEN);
    valueList1[0]->parameterValue = (char *) malloc(sizeof(char) * MAX_PARAMETER_LEN);
    strncpy(valueList1[0]->parameterValue, "Device",MAX_PARAMETER_LEN);
    valueList1[0]->type = ccsp_string;

    will_return(get_global_values, valueList1);
    will_return(get_global_parameters_count, 1);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterValues, size, 1);

    will_return(get_global_faultParam, NULL);
    will_return(CcspBaseIf_setParameterValues, CCSP_SUCCESS);
    expect_function_call(CcspBaseIf_setParameterValues);
    expect_value(CcspBaseIf_setParameterValues, size, 1);


    parameterValStruct_t **cid2List = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*));
    cid2List[0] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t)*1);
    cid2List[0]->parameterName = strndup(PARAM_CID,MAX_PARAMETER_LEN);
    cid2List[0]->parameterValue = strndup("61f4db9",MAX_PARAMETER_LEN);
    cid2List[0]->type = ccsp_string;
    will_return(get_global_values, cid2List);
    will_return(get_global_parameters_count, 1);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterValues, size, 1);    

    will_return(get_global_faultParam, NULL);
    will_return(CcspBaseIf_setParameterValues, CCSP_SUCCESS);
    expect_function_call(CcspBaseIf_setParameterValues);
    expect_value(CcspBaseIf_setParameterValues, size, 1);

    componentStruct_t **list = (componentStruct_t **) malloc(sizeof(componentStruct_t *)*1);
    list[0] = (componentStruct_t *) malloc(sizeof(componentStruct_t));
    list[0]->componentName = strdup("Device.X_CISCO_COM_DeviceControl.");
    list[0]->dbusPath = strdup(RDKB_WEBPA_COMPONENT_NAME);
    will_return(get_global_components, list);
    will_return(get_global_component_size, 1);
    expect_function_call(CcspBaseIf_discComponentSupportingNamespace);
    will_return(CcspBaseIf_discComponentSupportingNamespace, CCSP_SUCCESS);
    expect_function_call(free_componentStruct_t);

    parameterValStruct_t **valueList = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*));
    valueList[0] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t)*1);
    valueList[0]->parameterName = (char *) malloc(sizeof(char) * MAX_PARAMETER_LEN);
    strncpy(valueList[0]->parameterName, "Device.X_CISCO_COM_DeviceControl.RebootDevice",MAX_PARAMETER_LEN);
    valueList[0]->parameterValue = (char *) malloc(sizeof(char) * MAX_PARAMETER_LEN);
    strncpy(valueList[0]->parameterValue, "Device",MAX_PARAMETER_LEN);
    valueList[0]->type = ccsp_string;
    will_return(get_global_values, valueList);
    will_return(get_global_parameters_count, 1);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterValues, size, 1); 

    will_return(get_global_faultParam, NULL);
    will_return(CcspBaseIf_setParameterValues, CCSP_SUCCESS);
    expect_function_call(CcspBaseIf_setParameterValues);
    expect_value(CcspBaseIf_setParameterValues, size, 1);     

    processRequest(reqPayload, transactionId, &resPayload, req_headers, res_headers);
    WalInfo("resPayload : %s\n",resPayload); 
    assert_non_null(resPayload);
    response = cJSON_Parse(resPayload);
    assert_non_null(response);
    assert_string_equal("Success",cJSON_GetObjectItem(response, "message")->valuestring);
    assert_int_equal(200, cJSON_GetObjectItem(response, "statusCode")->valueint);

    if(resPayload !=NULL)
    {
	    free(resPayload);
    }
}

void test_cms_test_filed()
{
    char *reqPayload = "{\"command\":\"TEST_AND_SET\",\"old-cid\":\"61f4db9\",\"new-cid\":\"0\",\"sync-cmc\":\"512\",\"parameters\":[{\"name\":\"Device.X_RDK_WebConfig.URL\",\"dataType\":0,\"value\":\"https://webpa.com\",\"attributes\":{\"notify\":1}}]}";
    char *transactionId = "aasfsdfgehhdysy";
    char *resPayload = NULL;    
    headers_t *res_headers = NULL;
    headers_t *req_headers = NULL;
    cJSON *response = NULL; 

    getCompDetails();

    parameterValStruct_t **cmcList1 = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*));
    cmcList1[0] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t)*1);
    cmcList1[0]->parameterName = strndup(PARAM_CMC,MAX_PARAMETER_LEN);
    cmcList1[0]->parameterValue = strndup("511",MAX_PARAMETER_LEN);
    cmcList1[0]->type = ccsp_int;
    will_return(get_global_values, cmcList1);
    will_return(get_global_parameters_count, 1);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterValues, size, 1);

    parameterValStruct_t **cidList = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*));
    cidList[0] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t)*1);
    cidList[0]->parameterName = strndup(PARAM_CID,MAX_PARAMETER_LEN);
    cidList[0]->parameterValue = strndup("0",MAX_PARAMETER_LEN);
    cidList[0]->type = ccsp_string;
    will_return(get_global_values, cidList);
    will_return(get_global_parameters_count, 1);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterValues, size, 1);

    processRequest(reqPayload, transactionId, &resPayload, req_headers, res_headers);
    WalInfo("resPayload : %s\n",resPayload); 
    assert_non_null(resPayload);
    response = cJSON_Parse(resPayload);
    assert_non_null(response);
    assert_string_equal("CMC test failed",cJSON_GetObjectItem(response, "message")->valuestring);
    assert_int_equal(551, cJSON_GetObjectItem(response, "statusCode")->valueint);

    if(resPayload !=NULL)
    {
	    free(resPayload);
    }
}

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_processRequest_test_and_set),
        cmocka_unit_test(test2_test_and_set_with_reboot_parameter),
        cmocka_unit_test(test_cid_test_filed),        
        cmocka_unit_test(test_cms_test_filed),             
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}

