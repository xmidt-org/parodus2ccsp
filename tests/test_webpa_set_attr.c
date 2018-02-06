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
/*----------------------------------------------------------------------------*/
/*                                   Tests                                    */
/*----------------------------------------------------------------------------*/

void test_setAttrWithSingleParameterNotifyOn()
{
    char *resPayload = NULL;
    cJSON *response = NULL, *paramArray = NULL, *resParamObj = NULL;
    char *reqPayload = "{\"parameters\":[{\"name\":\"Device.DeviceInfo.Webpa.Enable\",\"attributes\": { \"notify\": 1}}],\"command\":\"SET_ATTRIBUTES\"}";

    getCompDetails();
    expect_function_call(CcspBaseIf_Register_Event);
    will_return(CcspBaseIf_Register_Event, CCSP_SUCCESS);
    expect_function_call(CcspBaseIf_SetCallback2);
    expect_function_call(CcspBaseIf_setParameterAttributes);
    will_return(CcspBaseIf_setParameterAttributes, CCSP_SUCCESS);
    expect_value(CcspBaseIf_setParameterAttributes, size, 1);

    processRequest(reqPayload, "abcd-1234-efgh-5678", &resPayload);
    WalInfo("resPayload : %s\n",resPayload);

    assert_non_null(resPayload);
    response = cJSON_Parse(resPayload);
    assert_non_null(response);
    paramArray = cJSON_GetObjectItem(response, "parameters");
    assert_int_equal(1, cJSON_GetArraySize(paramArray));
    resParamObj = cJSON_GetArrayItem(paramArray, 0);
    assert_non_null(resParamObj);
    assert_string_equal("Device.DeviceInfo.Webpa.Enable",cJSON_GetObjectItem(resParamObj, "name")->valuestring);
    assert_string_equal("Success",cJSON_GetObjectItem(resParamObj, "message")->valuestring );
    assert_int_equal(200, cJSON_GetObjectItem(response, "statusCode")->valueint);
    cJSON_Delete(response);
}

void test_setAttrWithSingleParameterNotifyOff()
{
    char *resPayload = NULL;
    cJSON *response = NULL, *paramArray = NULL, *resParamObj = NULL;
    char *reqPayload = "{\"parameters\":[{\"name\":\"Device.DeviceInfo.SerialNumber\",\"attributes\": { \"notify\": 0}}],\"command\":\"SET_ATTRIBUTES\"}";

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

    expect_function_call(CcspBaseIf_setParameterAttributes);
    will_return(CcspBaseIf_setParameterAttributes, CCSP_SUCCESS);
    expect_value(CcspBaseIf_setParameterAttributes, size, 1);

    processRequest(reqPayload, NULL, &resPayload);
    WalInfo("resPayload : %s\n",resPayload);

    assert_non_null(resPayload);
    response = cJSON_Parse(resPayload);
    assert_non_null(response);
    paramArray = cJSON_GetObjectItem(response, "parameters");
    assert_int_equal(1, cJSON_GetArraySize(paramArray));
    resParamObj = cJSON_GetArrayItem(paramArray, 0);
    assert_non_null(resParamObj);
    assert_string_equal("Device.DeviceInfo.SerialNumber",cJSON_GetObjectItem(resParamObj, "name")->valuestring);
    assert_string_equal("Success",cJSON_GetObjectItem(resParamObj, "message")->valuestring );
    assert_int_equal(200, cJSON_GetObjectItem(response, "statusCode")->valueint);
    cJSON_Delete(response);
}

void test_setAttrWithMultipleParameters()
{
    char *resPayload = NULL;
    cJSON *response = NULL, *paramArray = NULL, *resParamObj = NULL;
    char *reqPayload = "{\"parameters\":[{\"name\":\"Device.WiFi.RadioNumberOfEntries\",\"attributes\": { \"notify\": 1}},{\"name\":\"Device.WiFi.SSID.10001.SSID\",\"attributes\": { \"notify\": 0}},{\"name\":\"Device.WiFi.Radio.10000.Enable\",\"attributes\": { \"notify\": 1}},{\"name\":\"Device.WiFi.AccessPoint.10001.Name\",\"attributes\": { \"notify\": 0}}],\"command\":\"SET_ATTRIBUTES\"}";
    char *getNames[] = {"Device.WiFi.RadioNumberOfEntries", "Device.WiFi.SSID.10001.SSID", "Device.WiFi.Radio.10000.Enable","Device.WiFi.AccessPoint.10001.Name"};
    int paramCount = sizeof(getNames)/sizeof(getNames[0]);
    int i = 0;
    getCompDetails();

    expect_function_call(CcspBaseIf_Register_Event);
    will_return(CcspBaseIf_Register_Event, CCSP_SUCCESS);
    expect_function_call(CcspBaseIf_SetCallback2);

    expect_function_call(CcspBaseIf_Register_Event);
    will_return(CcspBaseIf_Register_Event, CCSP_SUCCESS);
    expect_function_call(CcspBaseIf_SetCallback2);

    expect_function_call(CcspBaseIf_setParameterAttributes);
    will_return(CcspBaseIf_setParameterAttributes, CCSP_SUCCESS);
    expect_value(CcspBaseIf_setParameterAttributes, size, paramCount);

    processRequest(reqPayload, "abcd-5678-1234-efgh", &resPayload);
    WalInfo("resPayload : %s\n",resPayload);

    assert_non_null(resPayload);
    response = cJSON_Parse(resPayload);
    assert_non_null(response);
    paramArray = cJSON_GetObjectItem(response, "parameters");
    assert_int_equal(paramCount, cJSON_GetArraySize(paramArray));
    for(i=0; i<paramCount; i++)
    {
        resParamObj = cJSON_GetArrayItem(paramArray, i);
        assert_non_null(resParamObj);
        assert_string_equal(getNames[i],cJSON_GetObjectItem(resParamObj, "name")->valuestring);
        assert_string_equal("Success",cJSON_GetObjectItem(resParamObj, "message")->valuestring );
    }
    assert_int_equal(200, cJSON_GetObjectItem(response, "statusCode")->valueint);
    cJSON_Delete(response);
}

void err_SetAttrWithDifferentComponents()
{
    char *resPayload = NULL;
    cJSON *response = NULL, *paramArray = NULL, *resParamObj = NULL;
    char *reqPayload = "{\"parameters\":[{\"name\":\"Device.WiFi.RadioNumberOfEntries\",\"attributes\": { \"notify\": 1}},{\"name\":\"Device.NAT.EnablePortMapping\",\"attributes\": { \"notify\": 1}}],\"command\":\"SET_ATTRIBUTES\"}";
    char *getNames[] = {"Device.WiFi.RadioNumberOfEntries", "Device.NAT.EnablePortMapping"};
    int paramCount = sizeof(getNames)/sizeof(getNames[0]);
    int i = 0;
    getCompDetails();

    expect_function_call(CcspBaseIf_Register_Event);
    will_return(CcspBaseIf_Register_Event, CCSP_SUCCESS);
    expect_function_call(CcspBaseIf_SetCallback2);

    processRequest(reqPayload, "abcd-5678-1234-efgh", &resPayload);
    WalInfo("resPayload : %s\n",resPayload);

    assert_non_null(resPayload);
    response = cJSON_Parse(resPayload);
    assert_non_null(response);
    paramArray = cJSON_GetObjectItem(response, "parameters");
    assert_int_equal(paramCount, cJSON_GetArraySize(paramArray));
    for(i=0; i<paramCount; i++)
    {
        resParamObj = cJSON_GetArrayItem(paramArray, i);
        assert_non_null(resParamObj);
        assert_string_equal(getNames[i],cJSON_GetObjectItem(resParamObj, "name")->valuestring);
        assert_string_equal("Failure",cJSON_GetObjectItem(resParamObj, "message")->valuestring );
    }
    assert_int_equal(520, cJSON_GetObjectItem(response, "statusCode")->valueint);
    cJSON_Delete(response);
}

void err_setAttrWithInvalidParam()
{
    cJSON *response = NULL, *paramArray = NULL, *resParamObj = NULL;
    char *resPayload = NULL;
    char *reqPayload = "{\"parameters\":[{\"name\":\"Device.WiFi.RadioNumberOfEntry\",\"attributes\": { \"notify\": 0}}],\"command\":\"SET_ATTRIBUTES\"}";

    getCompDetails();
    expect_function_call(CcspBaseIf_setParameterAttributes);
    will_return(CcspBaseIf_setParameterAttributes, CCSP_CR_ERR_UNSUPPORTED_NAMESPACE);
    expect_value(CcspBaseIf_setParameterAttributes, size, 1);

    processRequest(reqPayload, NULL, &resPayload);
    WalInfo("resPayload : %s\n",resPayload);

    assert_non_null(resPayload);
    response = cJSON_Parse(resPayload);
    assert_non_null(response);
    paramArray = cJSON_GetObjectItem(response, "parameters");
    assert_int_equal(1, cJSON_GetArraySize(paramArray));
    resParamObj = cJSON_GetArrayItem(paramArray, 0);
    assert_non_null(resParamObj);
    assert_string_equal("Device.WiFi.RadioNumberOfEntry",cJSON_GetObjectItem(resParamObj, "name")->valuestring);
    assert_string_equal("Error unsupported namespace",cJSON_GetObjectItem(resParamObj, "message")->valuestring );
    assert_int_equal(520, cJSON_GetObjectItem(response, "statusCode")->valueint);
    cJSON_Delete(response);
}

void err_setAttrWithWildcardParam()
{
    char *reqPayload = "{\"parameters\":[{\"name\":\"Device.WiFi.\",\"attributes\": { \"notify\": 0}}],\"command\":\"SET_ATTRIBUTES\"}";
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

void err_setAttrWithInvalidComponent()
{
    cJSON *response = NULL, *paramArray = NULL, *resParamObj = NULL;
    char *resPayload = NULL;
    char *reqPayload = "{\"parameters\":[{\"name\":\"Device.Abcd.1234\",\"attributes\": { \"notify\": 0}}],\"command\":\"SET_ATTRIBUTES\"}";

    getCompDetails();

    will_return(get_global_components, NULL);
    will_return(get_global_component_size, 0);
    expect_function_call(CcspBaseIf_discComponentSupportingNamespace);
    will_return(CcspBaseIf_discComponentSupportingNamespace, CCSP_SUCCESS);
    expect_function_call(free_componentStruct_t);

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
    paramArray = cJSON_GetObjectItem(response, "parameters");
    assert_int_equal(1, cJSON_GetArraySize(paramArray));
    resParamObj = cJSON_GetArrayItem(paramArray, 0);
    assert_non_null(resParamObj);
    assert_string_equal("Device.Abcd.1234",cJSON_GetObjectItem(resParamObj, "name")->valuestring);
    assert_string_equal("Error unsupported namespace",cJSON_GetObjectItem(resParamObj, "message")->valuestring );
    assert_int_equal(520, cJSON_GetObjectItem(response, "statusCode")->valueint);
    cJSON_Delete(response);
}

void err_setAttrWithInvalidComponentError()
{
    cJSON *response = NULL, *paramArray = NULL, *resParamObj = NULL;
    char *resPayload = NULL;
    char *reqPayload = "{\"parameters\":[{\"name\":\"Device.DeviceInfo.abcd\",\"attributes\": { \"notify\": 0}}],\"command\":\"SET_ATTRIBUTES\"}";

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
    paramArray = cJSON_GetObjectItem(response, "parameters");
    assert_int_equal(1, cJSON_GetArraySize(paramArray));
    resParamObj = cJSON_GetArrayItem(paramArray, 0);
    assert_non_null(resParamObj);
    assert_string_equal("Device.DeviceInfo.abcd",cJSON_GetObjectItem(resParamObj, "name")->valuestring);
    assert_string_equal("Error unsupported namespace",cJSON_GetObjectItem(resParamObj, "message")->valuestring );
    assert_int_equal(520, cJSON_GetObjectItem(response, "statusCode")->valueint);
    cJSON_Delete(response);
}

void err_setAttrWithInvalidWiFiIndex()
{
    cJSON *response = NULL, *paramArray = NULL, *resParamObj = NULL;
    char *resPayload = NULL;
    char *reqPayload = "{\"parameters\":[{\"name\":\"Device.WiFi.SSID.10000.SSID\",\"attributes\": { \"notify\": 0}},{\"name\":\"Device.WiFi.SSID.10001.Alias\",\"attributes\": { \"notify\": 0}},{\"name\":\"Device.WiFi.Version\",\"attributes\": { \"notify\": 0}}],\"command\":\"SET_ATTRIBUTES\"}";
    char *getNames[]={"Device.WiFi.SSID.10000.SSID","Device.WiFi.SSID.10001.Alias","Device.WiFi.Version"};
    int paramCount = sizeof(getNames)/sizeof(getNames[0]);
    int i = 0;

    processRequest(reqPayload, NULL, &resPayload);
    WalInfo("resPayload : %s\n",resPayload);

    assert_non_null(resPayload);
    response = cJSON_Parse(resPayload);
    assert_non_null(response);
    paramArray = cJSON_GetObjectItem(response, "parameters");
    assert_int_equal(paramCount, cJSON_GetArraySize(paramArray));
    for(i=0; i<paramCount; i++)
    {
        resParamObj = cJSON_GetArrayItem(paramArray, i);
        assert_non_null(resParamObj);
        assert_string_equal(getNames[i],cJSON_GetObjectItem(resParamObj, "name")->valuestring);
        assert_string_equal("Invalid WiFi index, valid range is between 10001-10008 and 10101-10108",cJSON_GetObjectItem(resParamObj, "message")->valuestring );
    }
    assert_int_equal(520, cJSON_GetObjectItem(response, "statusCode")->valueint);
    cJSON_Delete(response);
}

void err_setAttrWithInvalidRadioIndex()
{
    cJSON *response = NULL, *paramArray = NULL, *resParamObj = NULL;
    char *resPayload = NULL;
    char *reqPayload = "{\"parameters\":[{\"name\":\"Device.WiFi.SSID.10001.SSID\",\"attributes\": { \"notify\": 1}},{\"name\":\"Device.WiFi.AccessPoint.10001.Alias\",\"attributes\": { \"notify\": 1}},{\"name\":\"Device.WiFi.Radio.1.Enable\",\"attributes\": { \"notify\": 0}}],\"command\":\"SET_ATTRIBUTES\"}";
    char *getNames[]={"Device.WiFi.SSID.10001.SSID","Device.WiFi.AccessPoint.10001.Alias","Device.WiFi.Radio.1.Enable"};
    int paramCount = sizeof(getNames)/sizeof(getNames[0]);
    int i = 0;

    expect_function_call(CcspBaseIf_Register_Event);
    will_return(CcspBaseIf_Register_Event, CCSP_FAILURE);
    expect_function_call(CcspBaseIf_SetCallback2);

    expect_function_call(CcspBaseIf_Register_Event);
    will_return(CcspBaseIf_Register_Event, CCSP_SUCCESS);
    expect_function_call(CcspBaseIf_SetCallback2);

    processRequest(reqPayload, NULL, &resPayload);
    WalInfo("resPayload : %s\n",resPayload);

    assert_non_null(resPayload);
    response = cJSON_Parse(resPayload);
    assert_non_null(response);
    paramArray = cJSON_GetObjectItem(response, "parameters");
    assert_int_equal(paramCount, cJSON_GetArraySize(paramArray));
    for(i=0; i<paramCount; i++)
    {
        resParamObj = cJSON_GetArrayItem(paramArray, i);
        assert_non_null(resParamObj);
        assert_string_equal(getNames[i],cJSON_GetObjectItem(resParamObj, "name")->valuestring);
        assert_string_equal("Invalid Radio index, valid idexes are 10000 and 10100",cJSON_GetObjectItem(resParamObj, "message")->valuestring );
    }
    assert_int_equal(520, cJSON_GetObjectItem(response, "statusCode")->valueint);
    cJSON_Delete(response);
}

void err_SetAttrWithDifferentComponentsMultipleParameters()
{
    char *resPayload = NULL;
    cJSON *response = NULL, *paramArray = NULL, *resParamObj = NULL;
    char *reqPayload = "{\"parameters\":[{\"name\":\"Device.WiFi.RadioNumberOfEntries\",\"attributes\": { \"notify\": 1}},{\"name\":\"Device.WiFi.Version\",\"attributes\": { \"notify\": 0}},{\"name\":\"Device.WiFi.Name\",\"attributes\": { \"notify\": 1}},{\"name\":\"Device.NAT.Version\",\"attributes\": { \"notify\": 1}},{\"name\":\"Device.NAT.EnablePortMapping\",\"attributes\": { \"notify\": 1}},{\"name\":\"Device.NAT.PortMapping.1.Alias\",\"attributes\": { \"notify\": 1}}],\"command\":\"SET_ATTRIBUTES\"}";
    char *getNames[] = {"Device.WiFi.RadioNumberOfEntries","Device.WiFi.Version","Device.WiFi.Name","Device.NAT.Version","Device.NAT.EnablePortMapping","Device.NAT.PortMapping.1.Alias"};
    int paramCount = sizeof(getNames)/sizeof(getNames[0]);
    int i = 0;
    getCompDetails();

    expect_function_call(CcspBaseIf_Register_Event);
    will_return(CcspBaseIf_Register_Event, CCSP_SUCCESS);
    expect_function_call(CcspBaseIf_SetCallback2);

    expect_function_call(CcspBaseIf_Register_Event);
    will_return(CcspBaseIf_Register_Event, CCSP_FAILURE);
    expect_function_call(CcspBaseIf_SetCallback2);

    processRequest(reqPayload, "abcd-5678-1234-efgh", &resPayload);
    WalInfo("resPayload : %s\n",resPayload);

    assert_non_null(resPayload);
    response = cJSON_Parse(resPayload);
    assert_non_null(response);
    paramArray = cJSON_GetObjectItem(response, "parameters");
    assert_int_equal(paramCount, cJSON_GetArraySize(paramArray));
    for(i=0; i<paramCount; i++)
    {
        resParamObj = cJSON_GetArrayItem(paramArray, i);
        assert_non_null(resParamObj);
        assert_string_equal(getNames[i],cJSON_GetObjectItem(resParamObj, "name")->valuestring);
        assert_string_equal("Failure",cJSON_GetObjectItem(resParamObj, "message")->valuestring );
    }
    assert_int_equal(520, cJSON_GetObjectItem(response, "statusCode")->valueint);
    cJSON_Delete(response);
}

void err_setAttrWithMultipleParametersOneInvalidParam()
{
    char *resPayload = NULL;
    cJSON *response = NULL, *paramArray = NULL, *resParamObj = NULL;
    char *reqPayload = "{\"parameters\":[{\"name\":\"Device.NAT.PortMapping.1.Name\",\"attributes\": { \"notify\": 1}},{\"name\":\"Device.NAT.PortMapping.1.Alias\",\"attributes\": { \"notify\": 12}},{\"name\":\"Device.NAT.PortMapping.2.Port\",\"attributes\": { \"notify\": 123}},{\"name\":\"Device.NAT.PortMapping.20.Ver\",\"attributes\": { \"notify\": 012}}],\"command\":\"SET_ATTRIBUTES\"}";
    char *getNames[] = {"Device.NAT.PortMapping.1.Name", "Device.NAT.PortMapping.1.Alias", "Device.NAT.PortMapping.2.Port","Device.NAT.PortMapping.20.Ver"};
    int paramCount = sizeof(getNames)/sizeof(getNames[0]);
    int i = 0;
    getCompDetails();

    expect_function_call(CcspBaseIf_Register_Event);
    will_return(CcspBaseIf_Register_Event, CCSP_FAILURE);
    expect_function_call(CcspBaseIf_SetCallback2);

    expect_function_call(CcspBaseIf_setParameterAttributes);
    will_return(CcspBaseIf_setParameterAttributes, CCSP_ERR_INVALID_PARAMETER_NAME);
    expect_value(CcspBaseIf_setParameterAttributes, size, paramCount);

    processRequest(reqPayload, "abcd-5678-1234-efgh", &resPayload);
    WalInfo("resPayload : %s\n",resPayload);

    assert_non_null(resPayload);
    response = cJSON_Parse(resPayload);
    assert_non_null(response);
    paramArray = cJSON_GetObjectItem(response, "parameters");
    assert_int_equal(paramCount, cJSON_GetArraySize(paramArray));
    for(i=0; i<paramCount; i++)
    {
        resParamObj = cJSON_GetArrayItem(paramArray, i);
        assert_non_null(resParamObj);
        assert_string_equal(getNames[i],cJSON_GetObjectItem(resParamObj, "name")->valuestring);
        assert_string_equal("Invalid parameter name",cJSON_GetObjectItem(resParamObj, "message")->valuestring );
    }
    assert_int_equal(520, cJSON_GetObjectItem(response, "statusCode")->valueint);
    cJSON_Delete(response);
}
/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_setAttrWithSingleParameterNotifyOn),
        cmocka_unit_test(test_setAttrWithSingleParameterNotifyOff),
        cmocka_unit_test(test_setAttrWithMultipleParameters),
        cmocka_unit_test(err_SetAttrWithDifferentComponents),
        cmocka_unit_test(err_setAttrWithInvalidParam),
        cmocka_unit_test(err_setAttrWithWildcardParam),
        cmocka_unit_test(err_setAttrWithInvalidComponent),
        cmocka_unit_test(err_setAttrWithInvalidComponentError),
        cmocka_unit_test(err_setAttrWithInvalidWiFiIndex),
        cmocka_unit_test(err_setAttrWithInvalidRadioIndex),
        cmocka_unit_test(err_SetAttrWithDifferentComponentsMultipleParameters),
        cmocka_unit_test(err_setAttrWithMultipleParametersOneInvalidParam)
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
