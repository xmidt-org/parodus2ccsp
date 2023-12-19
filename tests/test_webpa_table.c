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

void test_processRequest_add_row()
{
    char *reqPayload = "{ \"row\":{\"DeviceName\":\"Device1\",\"MacAddress\":\"12:2:3:5:11\"},\"table\":\"Device.DeviceInfo.Webpa.\",\"command\": \"ADD_ROW\"}";
    char *transactionId = "aasfsdfgehhdysy";
    char *resPayload = NULL;    
    headers_t *res_headers = NULL;
    headers_t *req_headers = NULL;
    int count = 1;
    cJSON *response = NULL;

    componentStruct_t **list = (componentStruct_t **) malloc(sizeof(componentStruct_t *)*count);
    list[0] = (componentStruct_t *) malloc(sizeof(componentStruct_t));
    list[0]->componentName = strdup("com.cisco.spvtg.ccsp.webpaagent");
    list[0]->dbusPath = strdup("/com/ccsp/webpa");
    will_return(get_global_components, list);
    will_return(get_global_component_size, count);
    expect_function_call(CcspBaseIf_discComponentSupportingNamespace);
    will_return(CcspBaseIf_discComponentSupportingNamespace, CCSP_SUCCESS); 
    expect_function_call(free_componentStruct_t);    

    will_return(get_global_row_id, 1);
    expect_function_call(CcspBaseIf_AddTblRow);  
    will_return(CcspBaseIf_AddTblRow, CCSP_SUCCESS);  


    parameterValStruct_t **valueList = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*)*2);
    valueList[0] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t));
    valueList[0]->parameterName = (char *) malloc(sizeof(char) * MAX_PARAMETER_LEN);
    strncpy(valueList[0]->parameterName, "Device.DeviceInfo.Webpa.1.DeviceName",MAX_PARAMETER_LEN);
    valueList[0]->parameterValue = (char *) malloc(sizeof(char) * MAX_PARAMETER_LEN);
    strncpy(valueList[0]->parameterValue, "Device1",MAX_PARAMETER_LEN);
    valueList[0]->type = ccsp_string;
    valueList[1] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t));
    valueList[1]->parameterName = (char *) malloc(sizeof(char) * MAX_PARAMETER_LEN);
    strncpy(valueList[1]->parameterName, "Device.DeviceInfo.Webpa.1.MacAddress",MAX_PARAMETER_LEN);
    valueList[1]->parameterValue = (char *) malloc(sizeof(char) * MAX_PARAMETER_LEN);
    strncpy(valueList[1]->parameterValue, "12:2:3:5:11",MAX_PARAMETER_LEN);
    valueList[1]->type = ccsp_string;

    will_return(get_global_values, valueList);
    will_return(get_global_parameters_count, 2);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterValues, size, 2);         
    expect_function_call(free_parameterValStruct_t);

    will_return(get_global_faultParam, NULL);
    will_return(CcspBaseIf_setParameterValues, CCSP_SUCCESS);
    expect_function_call(CcspBaseIf_setParameterValues);
    expect_value(CcspBaseIf_setParameterValues, size, 2);
    
    processRequest(reqPayload, transactionId, &resPayload, req_headers, res_headers);
    WalInfo("resPayload : %s\n",resPayload);
    assert_non_null(resPayload);
    response = cJSON_Parse(resPayload);
    assert_non_null(response);
    assert_string_equal("Success",cJSON_GetObjectItem(response, "message")->valuestring);
    assert_int_equal(201, cJSON_GetObjectItem(response, "statusCode")->valueint);            
    if(resPayload !=NULL)
    {
	    free(resPayload);
    }    
}

//Test add row with invalid Radio index
void err1_processRequest_add_row()
{
    char *reqPayload = "{ \"row\":{\"DeviceName\":\"Device1\",\"MacAddress\":\"12:2:3:5:11\"},\"table\":\"Device.WiFi.Radio.Test\",\"command\": \"ADD_ROW\"}";
    char *transactionId = "aasfsdfgehhdysy";
    char *resPayload = NULL;    
    headers_t *res_headers = NULL;
    headers_t *req_headers = NULL;
    cJSON *response = NULL;

    processRequest(reqPayload, transactionId, &resPayload, req_headers, res_headers);
    WalInfo("resPayload : %s\n",resPayload);
    assert_non_null(resPayload);
    response = cJSON_Parse(resPayload);
    assert_non_null(response);
    assert_string_equal("Invalid Radio index, valid indexes are 10000, 10100 and 10200",cJSON_GetObjectItem(response, "message")->valuestring);
    assert_int_equal(520, cJSON_GetObjectItem(response, "statusCode")->valueint);   
    if(resPayload !=NULL)
    {
	    free(resPayload);
    }    
}

//Test add row with invalid WiFi index
void err2_processRequest_add_row()
{
    char *reqPayload = "{ \"row\":{\"DeviceName\":\"Device1\",\"MacAddress\":\"12:2:3:5:11\"},\"table\":\"Device.WiFi.SSID.Test\",\"command\": \"ADD_ROW\"}";
    char *transactionId = "aasfsdfgehhdysy";
    char *resPayload = NULL;    
    headers_t *res_headers = NULL;
    headers_t *req_headers = NULL;
    cJSON *response = NULL;

    processRequest(reqPayload, transactionId, &resPayload, req_headers, res_headers);
    WalInfo("resPayload : %s\n",resPayload);
    assert_non_null(resPayload);
    response = cJSON_Parse(resPayload);
    assert_non_null(response);
    assert_string_equal("Invalid WiFi index, valid range is between 10001-10008, 10101-10108 and 10201-10208",cJSON_GetObjectItem(response, "message")->valuestring);
    assert_int_equal(520, cJSON_GetObjectItem(response, "statusCode")->valueint);        
    if(resPayload !=NULL)
    {
	    free(resPayload);
    }    
}

//Test add_row when CcspBaseIf_setParameterValues execution fail
void err3_processRequest_add_row()
{
    char *reqPayload = "{ \"row\":{\"DeviceName\":\"Device1\",\"MacAddress\":\"12:2:3:5:11\"},\"table\":\"Device.DeviceInfo.Webpa.\",\"command\": \"ADD_ROW\"}";
    char *transactionId = "aasfsdfgehhdysy";
    char *resPayload = NULL;    
    headers_t *res_headers = NULL;
    headers_t *req_headers = NULL;
    int count = 1;
    cJSON *response = NULL;

    componentStruct_t **list = (componentStruct_t **) malloc(sizeof(componentStruct_t *)*count);
    list[0] = (componentStruct_t *) malloc(sizeof(componentStruct_t));
    list[0]->componentName = strdup("com.cisco.spvtg.ccsp.webpaagent");
    list[0]->dbusPath = strdup("/com/ccsp/webpa");
    will_return(get_global_components, list);
    will_return(get_global_component_size, count);
    expect_function_call(CcspBaseIf_discComponentSupportingNamespace);
    will_return(CcspBaseIf_discComponentSupportingNamespace, CCSP_SUCCESS); 
    expect_function_call(free_componentStruct_t);    

    will_return(get_global_row_id, 1);
    expect_function_call(CcspBaseIf_AddTblRow);  
    will_return(CcspBaseIf_AddTblRow, CCSP_SUCCESS);  


    parameterValStruct_t **valueList = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*)*2);
    valueList[0] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t));
    valueList[0]->parameterName = (char *) malloc(sizeof(char) * MAX_PARAMETER_LEN);
    strncpy(valueList[0]->parameterName, "Device.DeviceInfo.Webpa.1.DeviceName",MAX_PARAMETER_LEN);
    valueList[0]->parameterValue = (char *) malloc(sizeof(char) * MAX_PARAMETER_LEN);
    strncpy(valueList[0]->parameterValue, "Device1",MAX_PARAMETER_LEN);
    valueList[0]->type = ccsp_string;
    valueList[1] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t));
    valueList[1]->parameterName = (char *) malloc(sizeof(char) * MAX_PARAMETER_LEN);
    strncpy(valueList[1]->parameterName, "Device.DeviceInfo.Webpa.1.MacAddress",MAX_PARAMETER_LEN);
    valueList[1]->parameterValue = (char *) malloc(sizeof(char) * MAX_PARAMETER_LEN);
    strncpy(valueList[1]->parameterValue, "12:2:3:5:11",MAX_PARAMETER_LEN);
    valueList[1]->type = ccsp_string;

    will_return(get_global_values, valueList);
    will_return(get_global_parameters_count, 2);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterValues, size, 2);         
    expect_function_call(free_parameterValStruct_t);

    will_return(get_global_faultParam, NULL);
    will_return(CcspBaseIf_setParameterValues, CCSP_FAILURE);
    expect_function_call(CcspBaseIf_setParameterValues);
    expect_value(CcspBaseIf_setParameterValues, size, 2);

    componentStruct_t **list1 = (componentStruct_t **) malloc(sizeof(componentStruct_t *)*count);
    list1[0] = (componentStruct_t *) malloc(sizeof(componentStruct_t));
    list1[0]->componentName = strdup("com.cisco.spvtg.ccsp.webpaagent");
    list1[0]->dbusPath = strdup("/com/ccsp/webpa");
    will_return(get_global_components, list1);
    will_return(get_global_component_size, count);
    expect_function_call(CcspBaseIf_discComponentSupportingNamespace);
    will_return(CcspBaseIf_discComponentSupportingNamespace, CCSP_SUCCESS); 
    expect_function_call(free_componentStruct_t);

    expect_function_call(CcspBaseIf_DeleteTblRow);  
    will_return(CcspBaseIf_DeleteTblRow, CCSP_SUCCESS);    

    processRequest(reqPayload, transactionId, &resPayload, req_headers, res_headers);
    WalInfo("resPayload : %s\n",resPayload);
    assert_non_null(resPayload);
    response = cJSON_Parse(resPayload);
    assert_non_null(response);
    assert_string_equal("Failure",cJSON_GetObjectItem(response, "message")->valuestring);
    assert_int_equal(520, cJSON_GetObjectItem(response, "statusCode")->valueint);         
    if(resPayload !=NULL)
    {
	    free(resPayload);
    }      
}

//Test add_row when CcspBaseIf_discComponentSupportingNamespace execution fail in delete row
void err4_processRequest_add_row()
{
    char *reqPayload = "{ \"row\":{\"DeviceName\":\"Device1\",\"MacAddress\":\"12:2:3:5:11\"},\"table\":\"Device.DeviceInfo.Webpa.\",\"command\": \"ADD_ROW\"}";
    char *transactionId = "aasfsdfgehhdysy";
    char *resPayload = NULL;    
    headers_t *res_headers = NULL;
    headers_t *req_headers = NULL;
    int count = 1;
    cJSON *response = NULL;

    componentStruct_t **list = (componentStruct_t **) malloc(sizeof(componentStruct_t *)*count);
    list[0] = (componentStruct_t *) malloc(sizeof(componentStruct_t));
    list[0]->componentName = strdup("com.cisco.spvtg.ccsp.webpaagent");
    list[0]->dbusPath = strdup("/com/ccsp/webpa");
    will_return(get_global_components, list);
    will_return(get_global_component_size, count);
    expect_function_call(CcspBaseIf_discComponentSupportingNamespace);
    will_return(CcspBaseIf_discComponentSupportingNamespace, CCSP_SUCCESS); 
    expect_function_call(free_componentStruct_t);    

    will_return(get_global_row_id, 1);
    expect_function_call(CcspBaseIf_AddTblRow);  
    will_return(CcspBaseIf_AddTblRow, CCSP_SUCCESS);  


    parameterValStruct_t **valueList = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*)*2);
    valueList[0] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t));
    valueList[0]->parameterName = (char *) malloc(sizeof(char) * MAX_PARAMETER_LEN);
    strncpy(valueList[0]->parameterName, "Device.DeviceInfo.Webpa.1.DeviceName",MAX_PARAMETER_LEN);
    valueList[0]->parameterValue = (char *) malloc(sizeof(char) * MAX_PARAMETER_LEN);
    strncpy(valueList[0]->parameterValue, "Device1",MAX_PARAMETER_LEN);
    valueList[0]->type = ccsp_string;
    valueList[1] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t));
    valueList[1]->parameterName = (char *) malloc(sizeof(char) * MAX_PARAMETER_LEN);
    strncpy(valueList[1]->parameterName, "Device.DeviceInfo.Webpa.1.MacAddress",MAX_PARAMETER_LEN);
    valueList[1]->parameterValue = (char *) malloc(sizeof(char) * MAX_PARAMETER_LEN);
    strncpy(valueList[1]->parameterValue, "12:2:3:5:11",MAX_PARAMETER_LEN);
    valueList[1]->type = ccsp_string;

    will_return(get_global_values, valueList);
    will_return(get_global_parameters_count, 2);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterValues, size, 2);         
    expect_function_call(free_parameterValStruct_t);

    will_return(get_global_faultParam, NULL);
    will_return(CcspBaseIf_setParameterValues, CCSP_FAILURE);
    expect_function_call(CcspBaseIf_setParameterValues);
    expect_value(CcspBaseIf_setParameterValues, size, 2);

    componentStruct_t **list1 = (componentStruct_t **) malloc(sizeof(componentStruct_t *)*count);
    list1[0] = (componentStruct_t *) malloc(sizeof(componentStruct_t));
    list1[0]->componentName = strdup("com.cisco.spvtg.ccsp.webpaagent");
    list1[0]->dbusPath = strdup("/com/ccsp/webpa");
    will_return(get_global_components, list1);
    will_return(get_global_component_size, count);
    expect_function_call(CcspBaseIf_discComponentSupportingNamespace);
    will_return(CcspBaseIf_discComponentSupportingNamespace, CCSP_FAILURE); 
    expect_function_call(free_componentStruct_t);

    processRequest(reqPayload, transactionId, &resPayload, req_headers, res_headers);
    WalInfo("resPayload : %s\n",resPayload);
    assert_non_null(resPayload);
    response = cJSON_Parse(resPayload);
    assert_non_null(response);
    assert_string_equal("Failure",cJSON_GetObjectItem(response, "message")->valuestring);
    assert_int_equal(520, cJSON_GetObjectItem(response, "statusCode")->valueint);           
    if(resPayload !=NULL)
    {
	    free(resPayload);
    }      
}


//Test add_row when CcspBaseIf_discComponentSupportingNamespace execution fail
void err5_processRequest_add_row()
{
    char *reqPayload = "{ \"row\":{\"DeviceName\":\"Device1\",\"MacAddress\":\"12:2:3:5:11\"},\"table\":\"Device.DeviceInfo.Webpa.\",\"command\": \"ADD_ROW\"}";
    char *transactionId = "aasfsdfgehhdysy";
    char *resPayload = NULL;    
    headers_t *res_headers = NULL;
    headers_t *req_headers = NULL;
    int count = 1;
    cJSON *response = NULL;
    
    componentStruct_t **list = (componentStruct_t **) malloc(sizeof(componentStruct_t *)*count);
    list[0] = (componentStruct_t *) malloc(sizeof(componentStruct_t));
    list[0]->componentName = strdup("com.cisco.spvtg.ccsp.webpaagent");
    list[0]->dbusPath = strdup("/com/ccsp/webpa");
    will_return(get_global_components, list);
    will_return(get_global_component_size, count);
    expect_function_call(CcspBaseIf_discComponentSupportingNamespace);
    will_return(CcspBaseIf_discComponentSupportingNamespace, CCSP_FAILURE); 
    expect_function_call(free_componentStruct_t);    
    
    processRequest(reqPayload, transactionId, &resPayload, req_headers, res_headers);
    WalInfo("resPayload : %s\n",resPayload);
    assert_non_null(resPayload);
    response = cJSON_Parse(resPayload);
    assert_non_null(response);
    assert_string_equal("Failure",cJSON_GetObjectItem(response, "message")->valuestring);
    assert_int_equal(520, cJSON_GetObjectItem(response, "statusCode")->valueint);       
    if(resPayload !=NULL)
    {
	    free(resPayload);
    }        
}

void test_processRequest_delete_row()
{
    char *reqPayload = "{ \"row\":\"Device.DeviceInfo.Webpa.1.\",\"command\": \"DELETE_ROW\"}";
    char *transactionId = "aasfsdfgehhdysy";
    char *resPayload = NULL;    
    headers_t *res_headers = NULL;
    headers_t *req_headers = NULL;
    int count = 1;
    cJSON *response = NULL;

    componentStruct_t **list = (componentStruct_t **) malloc(sizeof(componentStruct_t *)*count);
    list[0] = (componentStruct_t *) malloc(sizeof(componentStruct_t));
    list[0]->componentName = strdup("com.cisco.spvtg.ccsp.webpaagent");
    list[0]->dbusPath = strdup("/com/ccsp/webpa");
    will_return(get_global_components, list);
    will_return(get_global_component_size, count);
    expect_function_call(CcspBaseIf_discComponentSupportingNamespace);
    will_return(CcspBaseIf_discComponentSupportingNamespace, CCSP_SUCCESS); 
    expect_function_call(free_componentStruct_t);

    expect_function_call(CcspBaseIf_DeleteTblRow);  
    will_return(CcspBaseIf_DeleteTblRow, CCSP_SUCCESS);      

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

//Test delete row with invalid Radio index
void err1_processRequest_delete_row()
{
    char *reqPayload = "{ \"row\":\"Device.WiFi.Radio.Test\",\"command\": \"DELETE_ROW\"}";
    char *transactionId = "aasfsdfgehhdysy";
    char *resPayload = NULL;    
    headers_t *res_headers = NULL;
    headers_t *req_headers = NULL;
    cJSON *response = NULL;

    processRequest(reqPayload, transactionId, &resPayload, req_headers, res_headers);
    WalInfo("resPayload : %s\n",resPayload);
    assert_non_null(resPayload);
    response = cJSON_Parse(resPayload);
    assert_non_null(response);
    assert_string_equal("Invalid Radio index, valid indexes are 10000, 10100 and 10200",cJSON_GetObjectItem(response, "message")->valuestring);
    assert_int_equal(520, cJSON_GetObjectItem(response, "statusCode")->valueint);      
    if(resPayload !=NULL)
    {
	    free(resPayload);
    }
}

//Test add row with invalid WiFi index
void err2_processRequest_delete_row()
{
    char *reqPayload = "{ \"row\":\"Device.WiFi.SSID.Test\",\"command\": \"DELETE_ROW\"}";
    char *transactionId = "aasfsdfgehhdysy";
    char *resPayload = NULL;    
    headers_t *res_headers = NULL;
    headers_t *req_headers = NULL;
    cJSON *response = NULL;

    processRequest(reqPayload, transactionId, &resPayload, req_headers, res_headers);
    WalInfo("resPayload : %s\n",resPayload);
    assert_non_null(resPayload);
    response = cJSON_Parse(resPayload);
    assert_non_null(response);
    assert_string_equal("Invalid WiFi index, valid range is between 10001-10008, 10101-10108 and 10201-10208",cJSON_GetObjectItem(response, "message")->valuestring);
    assert_int_equal(520, cJSON_GetObjectItem(response, "statusCode")->valueint);     
    if(resPayload !=NULL)
    {
	    free(resPayload);
    }
}

//Test add_row when Parameter name is not supported
void err3_processRequest_delete_row()
{
    char *reqPayload = "{ \"row\":\"Device.DeviceInfo.Webp.1.\",\"command\": \"DELETE_ROW\"}";
    char *transactionId = "aasfsdfgehhdysy";
    char *resPayload = NULL;    
    headers_t *res_headers = NULL;
    headers_t *req_headers = NULL;
    int count = 1;
    cJSON *response = NULL;

    componentStruct_t **list = (componentStruct_t **) malloc(sizeof(componentStruct_t *)*count);
    list[0] = (componentStruct_t *) malloc(sizeof(componentStruct_t));
    list[0]->componentName = strdup("com.cisco.spvtg.ccsp.webpaagent");
    list[0]->dbusPath = strdup("/com/ccsp/webpa");
    will_return(get_global_components, list);
    will_return(get_global_component_size, count);
    expect_function_call(CcspBaseIf_discComponentSupportingNamespace);
    will_return(CcspBaseIf_discComponentSupportingNamespace, CCSP_FAILURE); 
    expect_function_call(free_componentStruct_t);

    processRequest(reqPayload, transactionId, &resPayload, req_headers, res_headers);
    WalInfo("resPayload : %s\n",resPayload);
    assert_non_null(resPayload);
    response = cJSON_Parse(resPayload);
    assert_non_null(response);
    assert_string_equal("Failure",cJSON_GetObjectItem(response, "message")->valuestring);
    assert_int_equal(520, cJSON_GetObjectItem(response, "statusCode")->valueint);       
    if(resPayload !=NULL)
    {
	    free(resPayload);
    }    
}

//Test add_row when CcspBaseIf_DeleteTblRow execution fail
void err4_processRequest_delete_row()
{
    char *reqPayload = "{ \"row\":\"Device.DeviceInfo.Webpa.1.\",\"command\": \"DELETE_ROW\"}";
    char *transactionId = "aasfsdfgehhdysy";
    char *resPayload = NULL;    
    headers_t *res_headers = NULL;
    headers_t *req_headers = NULL;
    int count = 1;
    cJSON *response = NULL;
    
    componentStruct_t **list = (componentStruct_t **) malloc(sizeof(componentStruct_t *)*count);
    list[0] = (componentStruct_t *) malloc(sizeof(componentStruct_t));
    list[0]->componentName = strdup("com.cisco.spvtg.ccsp.webpaagent");
    list[0]->dbusPath = strdup("/com/ccsp/webpa");
    will_return(get_global_components, list);
    will_return(get_global_component_size, count);
    expect_function_call(CcspBaseIf_discComponentSupportingNamespace);
    will_return(CcspBaseIf_discComponentSupportingNamespace, CCSP_SUCCESS); 
    expect_function_call(free_componentStruct_t);

    expect_function_call(CcspBaseIf_DeleteTblRow);  
    will_return(CcspBaseIf_DeleteTblRow, CCSP_FAILURE);      

    processRequest(reqPayload, transactionId, &resPayload, req_headers, res_headers);
    WalInfo("resPayload : %s\n",resPayload);
    assert_non_null(resPayload);
    response = cJSON_Parse(resPayload);
    assert_non_null(response);
    assert_string_equal("Failure",cJSON_GetObjectItem(response, "message")->valuestring);
    assert_int_equal(520, cJSON_GetObjectItem(response, "statusCode")->valueint);        
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
        cmocka_unit_test(test_processRequest_add_row),
        cmocka_unit_test(err1_processRequest_add_row),
        cmocka_unit_test(err2_processRequest_add_row),
        cmocka_unit_test(err3_processRequest_add_row),
        cmocka_unit_test(err4_processRequest_add_row),
        cmocka_unit_test(err5_processRequest_add_row),        
        cmocka_unit_test(test_processRequest_delete_row),
        cmocka_unit_test(err1_processRequest_delete_row),
        cmocka_unit_test(err2_processRequest_delete_row), 
        cmocka_unit_test(err3_processRequest_delete_row),  
        cmocka_unit_test(err4_processRequest_delete_row)      
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}