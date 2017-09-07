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
#include "../source/broadband/include/webpa_notification.h"
#include <cimplog/cimplog.h>
#include <wdmp-c.h>
#include <wrp-c.h>
#include <libparodus.h>
#include <cJSON.h>
#include <ccsp_base_api.h>
#include "mock_stack.h"

#define MAX_PARAMETER_LEN			512
#define UNUSED(x) (void )(x)
/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
extern int cachingStatus;
extern ComponentVal ComponentValArray[RDKB_TR181_OBJECT_LEVEL1_COUNT];
extern ComponentVal SubComponentValArray[RDKB_TR181_OBJECT_LEVEL2_COUNT];
extern int compCacheSuccessCnt;
extern int subCompCacheSuccessCnt;
extern char deviceMAC[32];
/*----------------------------------------------------------------------------*/
/*                                   Mocks                                    */
/*----------------------------------------------------------------------------*/
void getCompDetails()
{
    int i=0;
    int compSizeList[] = {1,1,2,1};
    char *compNameList[] = {RDKB_WIFI_FULL_COMPONENT_NAME,"com.ccsp.webpa","com.ccsp.pam","com.ccsp.nat"};
    char *dbusPathList[] = {RDKB_WIFI_DBUS_PATH,"/com/ccsp/webpa","/com/ccsp/pam","/com/ccsp/nat"};
    char *objList[]={"Device.WiFi.","Device.Webpa.","Device.DeviceInfo.","Device.NAT."};
    int subCompSizeList[] = {1,1};
    char *subCompNameList[] = {"com.ccsp.webpa","com.ccsp.nat"};
    char *subDbusPathList[] = {"/com/ccsp/webpa","/com/ccsp/nat"};
    char *subObjList[]={"Device.DeviceInfo.Webpa.","Device.NAT.PortMapping."};

    cachingStatus = 1;
    compCacheSuccessCnt = 4;
    subCompCacheSuccessCnt = 2;
    for(i=0; i<compCacheSuccessCnt; i++)
    {
        ComponentValArray[i].comp_id=i;
        ComponentValArray[i].comp_size=compSizeList[i];
        ComponentValArray[i].obj_name=objList[i];
        ComponentValArray[i].comp_name=compNameList[i];
        ComponentValArray[i].dbus_path=dbusPathList[i];
    }

    for(i=0; i<subCompCacheSuccessCnt; i++)
    {
        SubComponentValArray[i].comp_id=i;
        SubComponentValArray[i].comp_size=subCompSizeList[i];
        SubComponentValArray[i].obj_name=subObjList[i];
        SubComponentValArray[i].comp_name=subCompNameList[i];
        SubComponentValArray[i].dbus_path=subDbusPathList[i];
    }
}

int libparodus_send (libpd_instance_t instance, wrp_msg_t *msg)
{
    UNUSED(instance);
    UNUSED(msg);
    function_called();
    return (int) mock();
}
/*----------------------------------------------------------------------------*/
/*                                   Tests                                    */
/*----------------------------------------------------------------------------*/
void test_device_status_notification()
{
    strcpy(deviceMAC, "14cfe2142112");
    will_return(libparodus_send, (intptr_t)0);
    expect_function_call(libparodus_send);
    processDeviceStatusNotification();
}

void test_factory_reset_notification()
{
    getCompDetails();
    strcpy(deviceMAC, "14cfe2142112");
    componentStruct_t **compList = (componentStruct_t **) malloc(sizeof(componentStruct_t *));
    compList[0] = (componentStruct_t *) malloc(sizeof(componentStruct_t));
    compList[0]->componentName = strndup("com.ccsp.pam",MAX_PARAMETER_LEN);
    compList[0]->dbusPath = strndup("/com/ccsp/pam",MAX_PARAMETER_LEN);

    parameterValStruct_t **cidList = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*));
    cidList[0] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t)*1);
    cidList[0]->parameterName = strndup(PARAM_CID,MAX_PARAMETER_LEN);
    cidList[0]->parameterValue = strndup("abcd",MAX_PARAMETER_LEN);;
    cidList[0]->type = ccsp_string;

    parameterValStruct_t **rebootReasonList = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*));
    rebootReasonList[0] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t)*1);
    rebootReasonList[0]->parameterName = strndup(PARAM_REBOOT_REASON,MAX_PARAMETER_LEN);
    rebootReasonList[0]->parameterValue = strndup("factory-reset",MAX_PARAMETER_LEN);;
    rebootReasonList[0]->type = ccsp_string;

    parameterValStruct_t **cmcList = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*));
    cmcList[0] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t)*1);
    cmcList[0]->parameterName = strndup(PARAM_CMC,MAX_PARAMETER_LEN);
    cmcList[0]->parameterValue = strndup("32",MAX_PARAMETER_LEN);;
    cmcList[0]->type = ccsp_int;

    will_return(get_global_values, cidList);
    will_return(get_global_parameters_count, 1);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterValues, size, 1);

    will_return(get_global_components, compList);
    will_return(get_global_component_size, 1);
    expect_function_call(CcspBaseIf_discComponentSupportingNamespace);
    will_return(CcspBaseIf_discComponentSupportingNamespace, CCSP_SUCCESS);
    expect_function_call(free_componentStruct_t);

    will_return(get_global_values, rebootReasonList);
    will_return(get_global_parameters_count, 1);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterValues, size, 1);

    will_return(get_global_values, cmcList);
    will_return(get_global_parameters_count, 1);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterValues, size, 1);

    will_return(get_global_faultParam, NULL);
    will_return(CcspBaseIf_setParameterValues, CCSP_SUCCESS);
    expect_function_call(CcspBaseIf_setParameterValues);
    expect_value(CcspBaseIf_setParameterValues, size, 1);

    will_return(libparodus_send, (intptr_t)0);
    expect_function_call(libparodus_send);
    sendNotificationForFactoryReset();
}

void test_firmware_upgrade_notification()
{
    getCompDetails();
    strcpy(deviceMAC, "14cfe2142112");
    componentStruct_t **compList = (componentStruct_t **) malloc(sizeof(componentStruct_t *));
    compList[0] = (componentStruct_t *) malloc(sizeof(componentStruct_t));
    compList[0]->componentName = strndup("com.ccsp.pam",MAX_PARAMETER_LEN);
    compList[0]->dbusPath = strndup("/com/ccsp/pam",MAX_PARAMETER_LEN);

    parameterValStruct_t **firmwareList = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*));
    firmwareList[0] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t)*1);
    firmwareList[0]->parameterName = strndup(PARAM_FIRMWARE_VERSION,MAX_PARAMETER_LEN);
    firmwareList[0]->parameterValue = strndup("TG1459_20170901Syd",MAX_PARAMETER_LEN);;
    firmwareList[0]->type = ccsp_string;

    parameterValStruct_t **cmcList = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*));
    cmcList[0] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t)*1);
    cmcList[0]->parameterName = strndup(PARAM_CMC,MAX_PARAMETER_LEN);
    cmcList[0]->parameterValue = strndup("12",MAX_PARAMETER_LEN);;
    cmcList[0]->type = ccsp_int;

    parameterValStruct_t **cmcList1 = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*));
    cmcList1[0] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t)*1);
    cmcList1[0]->parameterName = strndup(PARAM_CMC,MAX_PARAMETER_LEN);
    cmcList1[0]->parameterValue = strndup("32",MAX_PARAMETER_LEN);;
    cmcList1[0]->type = ccsp_int;

    parameterValStruct_t **cidList = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*));
    cidList[0] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t)*1);
    cidList[0]->parameterName = strndup(PARAM_CID,MAX_PARAMETER_LEN);
    cidList[0]->parameterValue = strndup("abcd",MAX_PARAMETER_LEN);;
    cidList[0]->type = ccsp_string;

    will_return(get_global_components, compList);
    will_return(get_global_component_size, 1);
    expect_function_call(CcspBaseIf_discComponentSupportingNamespace);
    will_return(CcspBaseIf_discComponentSupportingNamespace, CCSP_SUCCESS);
    expect_function_call(free_componentStruct_t);

    will_return(get_global_values, firmwareList);
    will_return(get_global_parameters_count, 1);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterValues, size, 1);

    will_return(get_global_values, cmcList);
    will_return(get_global_parameters_count, 1);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterValues, size, 1);

    will_return(get_global_values, cidList);
    will_return(get_global_parameters_count, 1);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterValues, size, 1);

    will_return(get_global_values, cmcList1);
    will_return(get_global_parameters_count, 1);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterValues, size, 1);

    will_return(get_global_faultParam, NULL);
    will_return(CcspBaseIf_setParameterValues, CCSP_SUCCESS);
    expect_function_call(CcspBaseIf_setParameterValues);
    expect_value(CcspBaseIf_setParameterValues, size, 1);

    will_return(libparodus_send, (intptr_t)0);
    expect_function_call(libparodus_send);
    sendNotificationForFirmwareUpgrade();
}

void test_transaction_status_notification()
{
    NotifyData *notifyData = (NotifyData *) malloc(sizeof(NotifyData));
    notifyData->type= TRANS_STATUS;
    notifyData->u.status= (TransData *)malloc(sizeof(TransData));
    notifyData->u.status->transId = strndup("qwkgfg75sgqwdgfhasfg", MAX_PARAMETER_LEN);;
    will_return(libparodus_send, (intptr_t)0);
    expect_function_call(libparodus_send);
    processNotification(notifyData);
}
/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_device_status_notification),
        cmocka_unit_test(test_factory_reset_notification),
        cmocka_unit_test(test_firmware_upgrade_notification),
        cmocka_unit_test(test_transaction_status_notification)
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
