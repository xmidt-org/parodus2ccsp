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
#define PARAM_FIRMWARE_VERSION		        "Device.DeviceInfo.X_CISCO_COM_FirmwareName"
#define DEVICE_BOOT_TIME                "Device.DeviceInfo.X_RDKCENTRAL-COM_BootTime"
/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
extern char deviceMAC[32];
int numLoops = 1;
extern pthread_mutex_t cloud_mut;
extern pthread_cond_t cloud_con;
/*----------------------------------------------------------------------------*/
/*                                   Mocks                                    */
/*----------------------------------------------------------------------------*/
int libparodus_send (libpd_instance_t instance, wrp_msg_t *msg)
{
    UNUSED(instance);
    UNUSED(msg);
    function_called();
    return (int) mock();
}

int pthread_cond_signal(pthread_cond_t *cloud_con)
{
    function_called();
    return (int) mock();
}

int pthread_cond_wait(pthread_cond_t *cloud_con, pthread_mutex_t *cloud_mut)
{
    pthread_cond_signal(&cloud_con);
    function_called();
    return (int) mock();
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
void test_device_status_notification()
{
    strcpy(deviceMAC, "14cfe2142112");
    parameterValStruct_t **bootTime = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*));
    bootTime[0] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t)*1);
    bootTime[0]->parameterName = strndup(DEVICE_BOOT_TIME,MAX_PARAMETER_LEN);
    bootTime[0]->parameterValue = strndup("157579132",MAX_PARAMETER_LEN);
    bootTime[0]->type = ccsp_string;

    will_return(get_global_components, getDeviceInfoCompDetails());
    will_return(get_global_component_size, 1);
    expect_function_call(CcspBaseIf_discComponentSupportingNamespace);
    will_return(CcspBaseIf_discComponentSupportingNamespace, CCSP_SUCCESS);
    expect_function_call(free_componentStruct_t);

    will_return(get_global_values, bootTime);
    will_return(get_global_parameters_count, 1);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterValues, size, 1);

    will_return(libparodus_send, (intptr_t)0);
    expect_function_call(libparodus_send);
    processDeviceStatusNotification(0);
}

void test_factory_reset_notification()
{
    getCompDetails();
    strcpy(deviceMAC, "14cfe2142112");

    parameterValStruct_t **cidList = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*));
    cidList[0] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t)*1);
    cidList[0]->parameterName = strndup(PARAM_CID,MAX_PARAMETER_LEN);
    cidList[0]->parameterValue = strndup("abcd",MAX_PARAMETER_LEN);
    cidList[0]->type = ccsp_string;

    parameterValStruct_t **rebootReasonList = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*));
    rebootReasonList[0] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t)*1);
    rebootReasonList[0]->parameterName = strndup(PARAM_REBOOT_REASON,MAX_PARAMETER_LEN);
    rebootReasonList[0]->parameterValue = strndup("factory-reset",MAX_PARAMETER_LEN);
    rebootReasonList[0]->type = ccsp_string;

    parameterValStruct_t **cmcList1 = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*));
    cmcList1[0] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t)*1);
    cmcList1[0]->parameterName = strndup(PARAM_CMC,MAX_PARAMETER_LEN);
    cmcList1[0]->parameterValue = strndup("0",MAX_PARAMETER_LEN);
    cmcList1[0]->type = ccsp_int;

    parameterValStruct_t **cmcList2 = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*));
    cmcList2[0] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t)*1);
    cmcList2[0]->parameterName = strndup(PARAM_CMC,MAX_PARAMETER_LEN);
    cmcList2[0]->parameterValue = strndup("0",MAX_PARAMETER_LEN);
    cmcList2[0]->type = ccsp_int;

    will_return(get_global_values, cidList);
    will_return(get_global_parameters_count, 1);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterValues, size, 1);

    will_return(get_global_components, getDeviceInfoCompDetails());
    will_return(get_global_component_size, 1);
    expect_function_call(CcspBaseIf_discComponentSupportingNamespace);
    will_return(CcspBaseIf_discComponentSupportingNamespace, CCSP_SUCCESS);
    expect_function_call(free_componentStruct_t);

    will_return(get_global_values, rebootReasonList);
    will_return(get_global_parameters_count, 1);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterValues, size, 1);

    will_return(get_global_values, cmcList1);
    will_return(get_global_parameters_count, 1);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterValues, size, 1);

    will_return(get_global_values, cmcList2);
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

    parameterValStruct_t **firmwareList = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*));
    firmwareList[0] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t)*1);
    firmwareList[0]->parameterName = strndup(PARAM_FIRMWARE_VERSION,MAX_PARAMETER_LEN);
    firmwareList[0]->parameterValue = strndup("TG1459_20170901Syd",MAX_PARAMETER_LEN);
    firmwareList[0]->type = ccsp_string;

    parameterValStruct_t **cmcList = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*));
    cmcList[0] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t)*1);
    cmcList[0]->parameterName = strndup(PARAM_CMC,MAX_PARAMETER_LEN);
    cmcList[0]->parameterValue = strndup("12",MAX_PARAMETER_LEN);
    cmcList[0]->type = ccsp_int;

    parameterValStruct_t **cmcList1 = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*));
    cmcList1[0] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t)*1);
    cmcList1[0]->parameterName = strndup(PARAM_CMC,MAX_PARAMETER_LEN);
    cmcList1[0]->parameterValue = strndup("32",MAX_PARAMETER_LEN);
    cmcList1[0]->type = ccsp_int;

    parameterValStruct_t **cidList = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*));
    cidList[0] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t)*1);
    cidList[0]->parameterName = strndup(PARAM_CID,MAX_PARAMETER_LEN);
    cidList[0]->parameterValue = strndup("abcd",MAX_PARAMETER_LEN);
    cidList[0]->type = ccsp_string;

    will_return(get_global_components, getDeviceInfoCompDetails());
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
    notifyData->u.status->transId = strndup("qwkgfg75sgqwdgfhasfg", MAX_PARAMETER_LEN);
    will_return(libparodus_send, (intptr_t)0);
    expect_function_call(libparodus_send);
    processNotification(notifyData);
}

void test_FR_cloud_sync_notification()
{
    numLoops = 2;
    pthread_t threadId;
    getCompDetails();
    strcpy(deviceMAC, "14cfe2142112");
    will_return(pthread_cond_signal, (intptr_t)0);
    expect_function_call(pthread_cond_signal);

    set_global_cloud_status(strdup("online"));

    parameterValStruct_t **cidList = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*));
    cidList[0] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t)*1);
    cidList[0]->parameterName = strndup(PARAM_CID,MAX_PARAMETER_LEN);
    cidList[0]->parameterValue = strndup("0",MAX_PARAMETER_LEN);
    cidList[0]->type = ccsp_string;

    parameterValStruct_t **cid2List = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*));
    cid2List[0] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t)*1);
    cid2List[0]->parameterName = strndup(PARAM_CID,MAX_PARAMETER_LEN);
    cid2List[0]->parameterValue = strndup("0",MAX_PARAMETER_LEN);
    cid2List[0]->type = ccsp_string;

    parameterValStruct_t **rebootReasonList = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*));
    rebootReasonList[0] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t)*1);
    rebootReasonList[0]->parameterName = strndup(PARAM_REBOOT_REASON,MAX_PARAMETER_LEN);
    rebootReasonList[0]->parameterValue = strndup("factory-reset",MAX_PARAMETER_LEN);
    rebootReasonList[0]->type = ccsp_string;

    parameterValStruct_t **cmcList = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*));
    cmcList[0] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t)*1);
    cmcList[0]->parameterName = strndup(PARAM_CMC,MAX_PARAMETER_LEN);
    cmcList[0]->parameterValue = strndup("32",MAX_PARAMETER_LEN);
    cmcList[0]->type = ccsp_int;

    parameterValStruct_t **cmcList2 = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*));
    cmcList2[0] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t)*1);
    cmcList2[0]->parameterName = strndup(PARAM_CMC,MAX_PARAMETER_LEN);
    cmcList2[0]->parameterValue = strndup("0",MAX_PARAMETER_LEN);
    cmcList2[0]->type = ccsp_int;

    will_return(libparodus_send, (intptr_t)0);
    expect_function_call(libparodus_send);

    will_return(pthread_cond_signal, (intptr_t)0);
    expect_function_call(pthread_cond_signal);

    will_return(pthread_cond_wait, (intptr_t)0);
    expect_function_call(pthread_cond_wait);

    will_return(get_global_values, cidList);
    will_return(get_global_parameters_count, 1);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterValues, size, 1);

    will_return(get_global_values, cid2List);
    will_return(get_global_parameters_count, 1);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterValues, size, 1);

    will_return(get_global_components, getDeviceInfoCompDetails());
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

    will_return(get_global_values, cmcList2);
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

    FactoryResetCloudSync();
}

void test_FR_cloud_sync_notification_retry()
{
    numLoops = 2;
    pthread_t threadId;
    getCompDetails();
    strcpy(deviceMAC, "14cfe2142112");
    will_return(pthread_cond_signal, (intptr_t)0);
    expect_function_call(pthread_cond_signal);

    set_global_cloud_status(strdup("online"));

    parameterValStruct_t **cidList = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*));
    cidList[0] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t)*1);
    cidList[0]->parameterName = strndup(PARAM_CID,MAX_PARAMETER_LEN);
    cidList[0]->parameterValue = strndup("0",MAX_PARAMETER_LEN);
    cidList[0]->type = ccsp_string;

    parameterValStruct_t **cid2List = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*));
    cid2List[0] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t)*1);
    cid2List[0]->parameterName = strndup(PARAM_CID,MAX_PARAMETER_LEN);
    cid2List[0]->parameterValue = strndup("0",MAX_PARAMETER_LEN);
    cid2List[0]->type = ccsp_string;

    parameterValStruct_t **rebootReasonList = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*));
    rebootReasonList[0] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t)*1);
    rebootReasonList[0]->parameterName = strndup(PARAM_REBOOT_REASON,MAX_PARAMETER_LEN);
    rebootReasonList[0]->parameterValue = strndup("factory-reset",MAX_PARAMETER_LEN);
    rebootReasonList[0]->type = ccsp_string;

    parameterValStruct_t **cmcList = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*));
    cmcList[0] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t)*1);
    cmcList[0]->parameterName = strndup(PARAM_CMC,MAX_PARAMETER_LEN);
    cmcList[0]->parameterValue = strndup("32",MAX_PARAMETER_LEN);
    cmcList[0]->type = ccsp_int;

    parameterValStruct_t **oldCmcList = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*));
    oldCmcList[0] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t)*1);
    oldCmcList[0]->parameterName = strndup(PARAM_CMC,MAX_PARAMETER_LEN);
    oldCmcList[0]->parameterValue = strndup("1",MAX_PARAMETER_LEN);
    oldCmcList[0]->type = ccsp_int;

    will_return(libparodus_send, (intptr_t)0);
    expect_function_call(libparodus_send);

    will_return(pthread_cond_signal, (intptr_t)0);
    expect_function_call(pthread_cond_signal);

    will_return(pthread_cond_wait, (intptr_t)0);
    expect_function_call(pthread_cond_wait);

    will_return(get_global_values, cidList);
    will_return(get_global_parameters_count, 1);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterValues, size, 1);

    will_return(get_global_values, cid2List);
    will_return(get_global_parameters_count, 1);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterValues, size, 1);

    will_return(get_global_components, getDeviceInfoCompDetails());
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

    will_return(get_global_values, oldCmcList);
    will_return(get_global_parameters_count, 1);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterValues, size, 1);

    FactoryResetCloudSync();
}
/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_device_status_notification),
        cmocka_unit_test(test_factory_reset_notification),
	    cmocka_unit_test(test_FR_cloud_sync_notification),
        cmocka_unit_test(test_firmware_upgrade_notification),
        cmocka_unit_test(test_transaction_status_notification),
        cmocka_unit_test(test_FR_cloud_sync_notification_retry)
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
