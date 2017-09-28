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

void test_singleGetAttr()
{
    char *reqPayload = "{ \"names\":[\"Device.DeviceInfo.Webpa.Enable\"],\"attributes\":\"notify\",\"command\": \"GET_ATTRIBUTES\"}";
    char *resPayload = NULL;
    cJSON *response = NULL, *paramArray = NULL, *resParamObj = NULL, *attrObj = NULL;
    int totalCount = 1;

    getCompDetails();
    parameterAttributeStruct_t **attrList = (parameterAttributeStruct_t **) malloc(sizeof(parameterAttributeStruct_t*)*totalCount);
    attrList[0] = (parameterAttributeStruct_t *) malloc(sizeof(parameterAttributeStruct_t));
    attrList[0]->parameterName =strdup("Device.DeviceInfo.Webpa.Enable");
    attrList[0]->notification = 1;

    will_return(get_global_attributes, attrList);
    will_return(get_global_parameters_count, totalCount);
    expect_function_call(CcspBaseIf_getParameterAttributes);
    will_return(CcspBaseIf_getParameterAttributes, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterAttributes, size, 1);
    expect_function_call(free_parameterAttributeStruct_t);

    processRequest(reqPayload, NULL, &resPayload);
    WalInfo("resPayload : %s\n",resPayload);

    assert_non_null(resPayload);
    response = cJSON_Parse(resPayload);
    assert_non_null(response);
    paramArray = cJSON_GetObjectItem(response, "parameters");
    assert_int_equal(totalCount, cJSON_GetArraySize(paramArray));
    resParamObj = cJSON_GetArrayItem(paramArray, 0);
    assert_non_null(resParamObj);
    assert_string_equal("Device.DeviceInfo.Webpa.Enable",cJSON_GetObjectItem(resParamObj, "name")->valuestring);
    attrObj = cJSON_GetObjectItem(resParamObj, "attributes");
    assert_non_null(attrObj);
    assert_int_equal(1, cJSON_GetObjectItem(attrObj, "notify")->valueint);
    assert_string_equal("Success",cJSON_GetObjectItem(resParamObj, "message")->valuestring );
    assert_int_equal(200, cJSON_GetObjectItem(response, "statusCode")->valueint);
    cJSON_Delete(response);
}

void test_get_attr_with_same_component_multiple_parameters()
{
    char *reqPayload = "{ \"names\":[\"Device.WiFi.SSID.10001.Enable\",\"Device.WiFi.Radio.10000.Enable\",\"Device.WiFi.AccessPoint.10001.Enable\",\"Device.WiFi.RadioNumberOfEntries\"],\"attributes\":\"notify\",\"command\": \"GET_ATTRIBUTES\"}";
    char *resPayload = NULL;
    cJSON *response = NULL, *paramArray = NULL, *resParamObj = NULL, *attrObj = NULL;
    int totalCount = 4, i = 0;
    char *getNames[MAX_PARAMETER_LEN] = {"Device.WiFi.SSID.10001.Enable", "Device.WiFi.Radio.10000.Enable", "Device.WiFi.AccessPoint.10001.Enable","Device.WiFi.RadioNumberOfEntries"};
    int notifyValues[MAX_PARAMETER_LEN] = {0,1,1,0};
    getCompDetails();
    parameterAttributeStruct_t **attrList = (parameterAttributeStruct_t **) malloc(sizeof(parameterAttributeStruct_t*)*totalCount);

    for(i = 0; i<totalCount; i++)
    {
        attrList[i] = (parameterAttributeStruct_t *) malloc(sizeof(parameterAttributeStruct_t));
        attrList[i]->parameterName =strdup(getNames[i]);
        attrList[i]->notification = notifyValues[i];
    }

    will_return(get_global_attributes, attrList);
    will_return(get_global_parameters_count, totalCount);
    expect_function_call(CcspBaseIf_getParameterAttributes);
    will_return(CcspBaseIf_getParameterAttributes, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterAttributes, size, 4);
    expect_function_call(free_parameterAttributeStruct_t);

    processRequest(reqPayload, "abcd-1234-ddfg-6gd7", &resPayload);
    WalInfo("resPayload : %s\n",resPayload);

    assert_non_null(resPayload);
    response = cJSON_Parse(resPayload);
    assert_non_null(response);
    paramArray = cJSON_GetObjectItem(response, "parameters");
    assert_int_equal(totalCount, cJSON_GetArraySize(paramArray));

    for(i=0; i<totalCount; i++)
    {
        resParamObj = cJSON_GetArrayItem(paramArray, i);
        assert_non_null(resParamObj);
        assert_string_equal(getNames[i],cJSON_GetObjectItem(resParamObj, "name")->valuestring);
        attrObj = cJSON_GetObjectItem(resParamObj, "attributes");
        assert_non_null(attrObj);
        assert_int_equal(notifyValues[i], cJSON_GetObjectItem(attrObj, "notify")->valueint);
        assert_string_equal("Success",cJSON_GetObjectItem(resParamObj, "message")->valuestring );
    }
    assert_int_equal(200, cJSON_GetObjectItem(response, "statusCode")->valueint);
    cJSON_Delete(response);
}

void test_get_attr_with_different_component_multiple_parameters()
{
    char *reqPayload = "{ \"names\":[\"Device.WiFi.SSID.10101.Enable\",\"Device.DeviceInfo.SerialNumber\",\"Device.DeviceInfo.Model\",\"Device.NAT.EnablePortMapping\",\"Device.NAT.PortMappingNumbers\",\"Device.NAT.PortMapping.1.Alias\",\"Device.Webpa.Version\",\"Device.Webpa.PostData\"],\"attributes\":\"notify\",\"command\": \"GET_ATTRIBUTES\"}";
    char *resPayload = NULL;
    cJSON *response = NULL, *paramArray = NULL, *resParamObj = NULL, *attrObj = NULL;
    int totalCount = 8, i = 0, j = 0;
    char *getNames[MAX_PARAMETER_LEN] = {"Device.WiFi.SSID.10101.Enable", "Device.DeviceInfo.SerialNumber", "Device.DeviceInfo.Model","Device.NAT.EnablePortMapping","Device.NAT.PortMappingNumbers","Device.NAT.PortMapping.1.Alias","Device.Webpa.Version","Device.Webpa.PostData"};
    int notifyValues[MAX_PARAMETER_LEN] = {1,1,0,1,0,1,1,0};

    getCompDetails();
    will_return(get_global_components, getDeviceInfoCompDetails());
    will_return(get_global_component_size, 1);
    expect_function_call(CcspBaseIf_discComponentSupportingNamespace);
    will_return(CcspBaseIf_discComponentSupportingNamespace, CCSP_SUCCESS);
    expect_function_call(free_componentStruct_t);

    will_return(get_global_components, getDeviceInfoCompDetails());
    will_return(get_global_component_size, 1);
    expect_function_call(CcspBaseIf_discComponentSupportingNamespace);
    will_return(CcspBaseIf_discComponentSupportingNamespace, CCSP_SUCCESS);
    expect_function_call(free_componentStruct_t);

    parameterAttributeStruct_t **wifiAttr = (parameterAttributeStruct_t **) malloc(sizeof(parameterAttributeStruct_t*));
    wifiAttr[0] = (parameterAttributeStruct_t *) malloc(sizeof(parameterAttributeStruct_t));
    wifiAttr[0]->parameterName =strdup(getNames[0]);
    wifiAttr[0]->notification = notifyValues[0];

    will_return(get_global_attributes, wifiAttr);
    will_return(get_global_parameters_count, 1);
    expect_function_call(CcspBaseIf_getParameterAttributes);
    will_return(CcspBaseIf_getParameterAttributes, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterAttributes, size, 1);
    expect_function_call(free_parameterAttributeStruct_t);

    parameterAttributeStruct_t **pamAttr = (parameterAttributeStruct_t **) malloc(sizeof(parameterAttributeStruct_t*)*2);
    j = 1;
    for(i = 0; i<2; i++)
    {
        pamAttr[i] = (parameterAttributeStruct_t *) malloc(sizeof(parameterAttributeStruct_t));
        pamAttr[i]->parameterName =strdup(getNames[j]);
        pamAttr[i]->notification = notifyValues[j];
        j++;
    }

    will_return(get_global_attributes, pamAttr);
    will_return(get_global_parameters_count, 2);
    expect_function_call(CcspBaseIf_getParameterAttributes);
    will_return(CcspBaseIf_getParameterAttributes, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterAttributes, size, 2);
    expect_function_call(free_parameterAttributeStruct_t);

    parameterAttributeStruct_t **natAttr = (parameterAttributeStruct_t **) malloc(sizeof(parameterAttributeStruct_t*)*3);
    for(i = 0; i<3; i++)
    {
        natAttr[i] = (parameterAttributeStruct_t *) malloc(sizeof(parameterAttributeStruct_t));
        natAttr[i]->parameterName =strdup(getNames[j]);
        natAttr[i]->notification = notifyValues[j];
        j++;
    }

    will_return(get_global_attributes, natAttr);
    will_return(get_global_parameters_count, 3);
    expect_function_call(CcspBaseIf_getParameterAttributes);
    will_return(CcspBaseIf_getParameterAttributes, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterAttributes, size, 3);
    expect_function_call(free_parameterAttributeStruct_t);

    parameterAttributeStruct_t **webpaAttr = (parameterAttributeStruct_t **) malloc(sizeof(parameterAttributeStruct_t*)*2);
    for(i = 0; i<2; i++)
    {
        webpaAttr[i] = (parameterAttributeStruct_t *) malloc(sizeof(parameterAttributeStruct_t));
        webpaAttr[i]->parameterName =strdup(getNames[j]);
        webpaAttr[i]->notification = notifyValues[j];
        j++;
    }

    will_return(get_global_attributes, webpaAttr);
    will_return(get_global_parameters_count, 2);
    expect_function_call(CcspBaseIf_getParameterAttributes);
    will_return(CcspBaseIf_getParameterAttributes, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterAttributes, size, 2);
    expect_function_call(free_parameterAttributeStruct_t);

    processRequest(reqPayload, "abcd-1234-ddfg-6sff", &resPayload);
    WalInfo("resPayload : %s\n",resPayload);

    assert_non_null(resPayload);
    response = cJSON_Parse(resPayload);
    assert_non_null(response);
    paramArray = cJSON_GetObjectItem(response, "parameters");
    assert_int_equal(totalCount, cJSON_GetArraySize(paramArray));

    for(i=0; i<totalCount; i++)
    {
        resParamObj = cJSON_GetArrayItem(paramArray, i);
        assert_non_null(resParamObj);
        assert_string_equal(getNames[i],cJSON_GetObjectItem(resParamObj, "name")->valuestring);
        attrObj = cJSON_GetObjectItem(resParamObj, "attributes");
        assert_non_null(attrObj);
        assert_int_equal(notifyValues[i], cJSON_GetObjectItem(attrObj, "notify")->valueint);
        assert_string_equal("Success",cJSON_GetObjectItem(resParamObj, "message")->valuestring );
    }
    assert_int_equal(200, cJSON_GetObjectItem(response, "statusCode")->valueint);
    cJSON_Delete(response);
}

void err_get_attr_with_invalid_param()
{
    char *reqPayload = "{ \"names\":[\"Device.DeviceInfo.Webpa.abcd\"],\"attributes\":\"notify\",\"command\": \"GET_ATTRIBUTES\"}";
    char *resPayload = NULL;
    cJSON *response = NULL;

    getCompDetails();

    will_return(get_global_attributes, NULL);
    will_return(get_global_parameters_count, 0);
    expect_function_call(CcspBaseIf_getParameterAttributes);
    will_return(CcspBaseIf_getParameterAttributes, CCSP_CR_ERR_UNSUPPORTED_NAMESPACE);
    expect_value(CcspBaseIf_getParameterAttributes, size, 1);

    processRequest(reqPayload, NULL, &resPayload);
    WalInfo("resPayload : %s\n",resPayload);

    assert_non_null(resPayload);
    response = cJSON_Parse(resPayload);
    assert_non_null(response);
    assert_string_equal("Error unsupported namespace",cJSON_GetObjectItem(response, "message")->valuestring );
    assert_int_equal(520, cJSON_GetObjectItem(response, "statusCode")->valueint);
    cJSON_Delete(response);
}

void err_get_attr_with_wildcard_param()
{
    char *reqPayload = "{ \"names\":[\"Device.DeviceInfo.\"],\"attributes\":\"notify\",\"command\": \"GET_ATTRIBUTES\"}";
    char *resPayload = NULL;
    cJSON *response = NULL;

    processRequest(reqPayload, "addf-sdfw-12ed-3fea", &resPayload);
    WalInfo("resPayload : %s\n",resPayload);

    assert_non_null(resPayload);
    response = cJSON_Parse(resPayload);
    assert_non_null(response);
    assert_string_equal("Wildcard is not supported",cJSON_GetObjectItem(response, "message")->valuestring );
    assert_int_equal(552, cJSON_GetObjectItem(response, "statusCode")->valueint);
    cJSON_Delete(response);
}

void err_get_attr_with_invalid_component()
{
    char *reqPayload = "{ \"names\":[\"Device.DeviInfo.fefrg3ef\"],\"attributes\":\"notify\",\"command\": \"GET_ATTRIBUTES\"}";
    char *resPayload = NULL;
    cJSON *response = NULL;

    getCompDetails();

    will_return(get_global_components, NULL);
    will_return(get_global_component_size, 0);
    expect_function_call(CcspBaseIf_discComponentSupportingNamespace);
    will_return(CcspBaseIf_discComponentSupportingNamespace, CCSP_CR_ERR_UNSUPPORTED_NAMESPACE);
    expect_function_call(free_componentStruct_t);
    processRequest(reqPayload, NULL, &resPayload);
    WalInfo("resPayload : %s\n",resPayload);

    assert_non_null(resPayload);
    response = cJSON_Parse(resPayload);
    assert_non_null(response);
    assert_string_equal("Error unsupported namespace",cJSON_GetObjectItem(response, "message")->valuestring );
    assert_int_equal(520, cJSON_GetObjectItem(response, "statusCode")->valueint);
    cJSON_Delete(response);
}

void err_get_attr_with_wifi_busy()
{
    char *reqPayload = "{ \"names\":[\"Device.WiFi.SSID.10001.SSID\",\"Device.NAT.PortMapping.1.Alias\",\"Device.Webpa.Version\"],\"attributes\":\"notify\",\"command\": \"GET_ATTRIBUTES\"}";
    char *resPayload = NULL;
    cJSON *response = NULL;
    applySettingsFlag = TRUE;

    processRequest(reqPayload, NULL, &resPayload);
    WalInfo("resPayload : %s\n",resPayload);

    assert_non_null(resPayload);
    response = cJSON_Parse(resPayload);
    assert_non_null(response);
    assert_string_equal("WiFi is busy",cJSON_GetObjectItem(response, "message")->valuestring );
    assert_int_equal(530, cJSON_GetObjectItem(response, "statusCode")->valueint);
    cJSON_Delete(response);
}

void err_get_attr_with_wifi_busy_at_end()
{
    char *reqPayload = "{ \"names\":[\"Device.NAT.PortMapping.1.Alias\",\"Device.Webpa.Version\",\"Device.WiFi.SSID.10001.SSID\"],\"attributes\":\"notify\",\"command\": \"GET_ATTRIBUTES\"}";
    char *resPayload = NULL;
    cJSON *response = NULL;
    applySettingsFlag = TRUE;

    parameterAttributeStruct_t **natAttr = (parameterAttributeStruct_t **) malloc(sizeof(parameterAttributeStruct_t*));
    natAttr[0] = (parameterAttributeStruct_t *) malloc(sizeof(parameterAttributeStruct_t));
    natAttr[0]->parameterName =strdup("Device.NAT.PortMapping.1.Alias");
    natAttr[0]->notification = 1;

    will_return(get_global_attributes, natAttr);
    will_return(get_global_parameters_count, 1);
    expect_function_call(CcspBaseIf_getParameterAttributes);
    will_return(CcspBaseIf_getParameterAttributes, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterAttributes, size, 1);
    expect_function_call(free_parameterAttributeStruct_t);

    parameterAttributeStruct_t **webpaAttr = (parameterAttributeStruct_t **) malloc(sizeof(parameterAttributeStruct_t*));
    webpaAttr[0] = (parameterAttributeStruct_t *) malloc(sizeof(parameterAttributeStruct_t));
    webpaAttr[0]->parameterName =strdup("Device.Webpa.Version");
    webpaAttr[0]->notification = 0;

    will_return(get_global_attributes, webpaAttr);
    will_return(get_global_parameters_count, 1);
    expect_function_call(CcspBaseIf_getParameterAttributes);
    will_return(CcspBaseIf_getParameterAttributes, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterAttributes, size, 1);
    expect_function_call(free_parameterAttributeStruct_t);

    processRequest(reqPayload, NULL, &resPayload);
    WalInfo("resPayload : %s\n",resPayload);

    assert_non_null(resPayload);
    response = cJSON_Parse(resPayload);
    assert_non_null(response);
    assert_string_equal("WiFi is busy",cJSON_GetObjectItem(response, "message")->valuestring );
    assert_int_equal(530, cJSON_GetObjectItem(response, "statusCode")->valueint);
    cJSON_Delete(response);
}

void err_get_attr_with_invalid_wifi_index()
{
    char *reqPayload = "{ \"names\":[\"Device.WiFi.SSID.101101.SSID\",\"Device.WiFi.Radio.10001.Enable\"],\"attributes\":\"notify\",\"command\":\"GET_ATTRIBUTES\"}";
    char *resPayload = NULL;
    cJSON *response = NULL;
    applySettingsFlag = FALSE;

    processRequest(reqPayload, NULL, &resPayload);
    WalInfo("resPayload : %s\n",resPayload);

    assert_non_null(resPayload);
    response = cJSON_Parse(resPayload);
    assert_non_null(response);
    assert_string_equal("Invalid WiFi index, valid range is between 10001-10008 and 10101-10108",cJSON_GetObjectItem(response, "message")->valuestring );
    assert_int_equal(520, cJSON_GetObjectItem(response, "statusCode")->valueint);
    cJSON_Delete(response);
}

void err_get_attr_with_invalid_radio_index()
{
    char *reqPayload = "{ \"names\":[\"Device.DeviceInfo.Webpa.Enable\",\"Device.WiFi.Radio.10001.Enable\",\"Device.WiFi.Radio.10000.Enable\",\"Device.NAT.EnablePortMapping\"],\"attributes\":\"notify\",\"command\":\"GET_ATTRIBUTES\"}";
    char *resPayload = NULL;
    cJSON *response = NULL;
    applySettingsFlag = FALSE;

    parameterAttributeStruct_t **webpaAttr = (parameterAttributeStruct_t **) malloc(sizeof(parameterAttributeStruct_t*));
    webpaAttr[0] = (parameterAttributeStruct_t *) malloc(sizeof(parameterAttributeStruct_t));
    webpaAttr[0]->parameterName =strdup("Device.DeviceInfo.Webpa.Enable");
    webpaAttr[0]->notification = 1;

    will_return(get_global_attributes, webpaAttr);
    will_return(get_global_parameters_count, 1);
    expect_function_call(CcspBaseIf_getParameterAttributes);
    will_return(CcspBaseIf_getParameterAttributes, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterAttributes, size, 1);
    expect_function_call(free_parameterAttributeStruct_t);

    processRequest(reqPayload, "hfsh-tdtt-56te-ehg6", &resPayload);
    WalInfo("resPayload : %s\n",resPayload);

    assert_non_null(resPayload);
    response = cJSON_Parse(resPayload);
    assert_non_null(response);
    assert_string_equal("Invalid Radio index, valid idexes are 10000 and 10100",cJSON_GetObjectItem(response, "message")->valuestring );
    assert_int_equal(520, cJSON_GetObjectItem(response, "statusCode")->valueint);
    cJSON_Delete(response);
}

void err_get_attr_with_multiple_parameters()
{
    char *reqPayload = "{ \"names\":[\"Device.WiFi.SSID.10101.Enable\",\"Device.DeviceInfo.SerialNumber\",\"Device.DeviceInfo.Model\",\"Device.NAT.EnablePortMapping\",\"Device.NAT.PortMappingNumbe\",\"Device.NAT.PortMapping.1.Alias\",\"Device.Webpa.Version\",\"Device.Webpa.PostData\"],\"attributes\":\"notify\",\"command\": \"GET_ATTRIBUTES\"}";
    char *resPayload = NULL;
    cJSON *response = NULL;
    int totalCount = 8, i = 0, j = 0;
    char *getNames[MAX_PARAMETER_LEN] = {"Device.WiFi.SSID.10101.Enable", "Device.DeviceInfo.SerialNumber", "Device.DeviceInfo.Model","Device.NAT.EnablePortMapping","Device.NAT.PortMappingNumbers","Device.NAT.PortMapping.1.Alias","Device.Webpa.Version","Device.Webpa.PostData"};
    int notifyValues[MAX_PARAMETER_LEN] = {1,1,0,1,0,1,1,0};

    getCompDetails();
    will_return(get_global_components, getDeviceInfoCompDetails());
    will_return(get_global_component_size, 1);
    expect_function_call(CcspBaseIf_discComponentSupportingNamespace);
    will_return(CcspBaseIf_discComponentSupportingNamespace, CCSP_SUCCESS);
    expect_function_call(free_componentStruct_t);

    will_return(get_global_components, getDeviceInfoCompDetails());
    will_return(get_global_component_size, 1);
    expect_function_call(CcspBaseIf_discComponentSupportingNamespace);
    will_return(CcspBaseIf_discComponentSupportingNamespace, CCSP_SUCCESS);
    expect_function_call(free_componentStruct_t);

    parameterAttributeStruct_t **wifiAttr = (parameterAttributeStruct_t **) malloc(sizeof(parameterAttributeStruct_t*));
    wifiAttr[0] = (parameterAttributeStruct_t *) malloc(sizeof(parameterAttributeStruct_t));
    wifiAttr[0]->parameterName =strdup(getNames[0]);
    wifiAttr[0]->notification = notifyValues[0];

    will_return(get_global_attributes, wifiAttr);
    will_return(get_global_parameters_count, 1);
    expect_function_call(CcspBaseIf_getParameterAttributes);
    will_return(CcspBaseIf_getParameterAttributes, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterAttributes, size, 1);
    expect_function_call(free_parameterAttributeStruct_t);

    parameterAttributeStruct_t **pamAttr = (parameterAttributeStruct_t **) malloc(sizeof(parameterAttributeStruct_t*)*2);
    j = 1;
    for(i = 0; i<2; i++)
    {
        pamAttr[i] = (parameterAttributeStruct_t *) malloc(sizeof(parameterAttributeStruct_t));
        pamAttr[i]->parameterName =strdup(getNames[j]);
        pamAttr[i]->notification = notifyValues[j];
        j++;
    }

    will_return(get_global_attributes, pamAttr);
    will_return(get_global_parameters_count, 2);
    expect_function_call(CcspBaseIf_getParameterAttributes);
    will_return(CcspBaseIf_getParameterAttributes, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterAttributes, size, 2);
    expect_function_call(free_parameterAttributeStruct_t);

    parameterAttributeStruct_t **natAttr = (parameterAttributeStruct_t **) malloc(sizeof(parameterAttributeStruct_t*)*3);
    for(i = 0; i<3; i++)
    {
        natAttr[i] = (parameterAttributeStruct_t *) malloc(sizeof(parameterAttributeStruct_t));
        natAttr[i]->parameterName =strdup(getNames[j]);
        natAttr[i]->notification = notifyValues[j];
        j++;
    }

    will_return(get_global_attributes, NULL);
    will_return(get_global_parameters_count, 0);
    expect_function_call(CcspBaseIf_getParameterAttributes);
    will_return(CcspBaseIf_getParameterAttributes, CCSP_CR_ERR_UNSUPPORTED_NAMESPACE);
    expect_value(CcspBaseIf_getParameterAttributes, size, 3);

    processRequest(reqPayload, "qfqf-ertw-ddfg-3r13", &resPayload);
    WalInfo("resPayload : %s\n",resPayload);

    assert_non_null(resPayload);
    response = cJSON_Parse(resPayload);
    assert_non_null(response);
    assert_string_equal("Error unsupported namespace",cJSON_GetObjectItem(response, "message")->valuestring );
    assert_int_equal(520, cJSON_GetObjectItem(response, "statusCode")->valueint);
    cJSON_Delete(response);
}
/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_singleGetAttr),
        cmocka_unit_test(test_get_attr_with_same_component_multiple_parameters),
        cmocka_unit_test(test_get_attr_with_different_component_multiple_parameters),
        cmocka_unit_test(err_get_attr_with_invalid_param),
        cmocka_unit_test(err_get_attr_with_wildcard_param),
        cmocka_unit_test(err_get_attr_with_invalid_component),
        cmocka_unit_test(err_get_attr_with_wifi_busy),
        cmocka_unit_test(err_get_attr_with_wifi_busy_at_end),
        cmocka_unit_test(err_get_attr_with_invalid_wifi_index),
        cmocka_unit_test(err_get_attr_with_invalid_radio_index),
        cmocka_unit_test(err_get_attr_with_multiple_parameters),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
