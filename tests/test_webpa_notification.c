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
#include <stdlib.h>
#include <cJSON.h>
#include <sys/time.h>

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

extern componentStruct_t **getDeviceInfoCompDetails();
extern void loadCfgFile();
extern void set_global_cloud_status(char*);
extern void processDeviceStatusNotification(int);
extern void getCompDetails();
extern void sendNotificationForFactoryReset();
extern void sendNotificationForFirmwareUpgrade();
extern void processNotification(NotifyData *notifyData);
extern void* FactoryResetCloudSync();


/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
extern char deviceMAC[32];
extern int wakeUpFlag;
extern int numLoops;
extern int g_checkSyncNotifyRetry;
extern int g_syncNotifyInProgress;
extern char* cloud_status;
extern int g_syncRetryThreadStarted;
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

int pthread_cond_timedwait(pthread_cond_t *cloud_con, pthread_mutex_t *cloud_mut, const struct timespec *ts )
{

    pthread_cond_signal(cloud_con);
    wakeUpFlag=1;
    if(numLoops==1)
    {
		wakeUpFlag = 0;
    }
    if(strcmp(cloud_status, "offline")==0 && (numLoops ==0))
    {
		wakeUpFlag = 0;
		cloud_status = strdup("online");
    }
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

unsigned int sleep(unsigned int seconds)
{
    struct timespec delay;

    delay.tv_sec = seconds / 1000;
    delay.tv_nsec = seconds % 1000 * 1000000;

    nanosleep( &delay, NULL );

    return seconds;
}


/*----------------------------------------------------------------------------*/
/*                                   Tests                                    */
/*----------------------------------------------------------------------------*/
void test_device_status_notification()
{
    strcpy(deviceMAC, "abcdeg1234");
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
    strcpy(deviceMAC, "abcdeg1234");

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

    will_return(get_global_values, cmcList1);
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
    strcpy(deviceMAC, "abcdeg1234");

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
    strcpy(deviceMAC, "abcdeg1234");
    will_return(pthread_cond_signal, (intptr_t)0);
    expect_function_call(pthread_cond_signal);

    set_global_cloud_status(strdup("online"));
    wakeUpFlag = 0;
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

    will_return(pthread_cond_timedwait, (intptr_t)0);
    expect_function_call(pthread_cond_timedwait);

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

    will_return(get_global_values, cmcList);
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
    strcpy(deviceMAC, "abcdeg1234");
    will_return(pthread_cond_signal, (intptr_t)0);
    expect_function_call(pthread_cond_signal);

    set_global_cloud_status(strdup("online"));
    wakeUpFlag = 0;
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

    parameterValStruct_t **oldCmcList = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*));
    oldCmcList[0] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t)*1);
    oldCmcList[0]->parameterName = strndup(PARAM_CMC,MAX_PARAMETER_LEN);
    oldCmcList[0]->parameterValue = strndup("1",MAX_PARAMETER_LEN);
    oldCmcList[0]->type = ccsp_int;

    will_return(libparodus_send, (intptr_t)0);
    expect_function_call(libparodus_send);

    will_return(pthread_cond_signal, (intptr_t)0);
    expect_function_call(pthread_cond_signal);

    will_return(pthread_cond_timedwait, (intptr_t)0);
    expect_function_call(pthread_cond_timedwait);

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

    will_return(get_global_values, oldCmcList);
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

    will_return(libparodus_send, (intptr_t)0);
    expect_function_call(libparodus_send);

    FactoryResetCloudSync();
}

void test_FR_notify_cloud_status_retry()
{
    numLoops = 3;
    pthread_t threadId;
    getCompDetails();
    strcpy(deviceMAC, "abcdeg1234");
    will_return(pthread_cond_signal, (intptr_t)0);
    expect_function_call(pthread_cond_signal);

    set_global_cloud_status(strdup("offline"));
    wakeUpFlag = 0;
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

    will_return(pthread_cond_timedwait, (intptr_t)ETIMEDOUT);
    expect_function_call(pthread_cond_timedwait);

    will_return(libparodus_send, (intptr_t)0);
    expect_function_call(libparodus_send);

    will_return(pthread_cond_signal, (intptr_t)0);
    expect_function_call(pthread_cond_signal);

    will_return(pthread_cond_timedwait, (intptr_t)0);
    expect_function_call(pthread_cond_timedwait);
    will_return(pthread_cond_signal, (intptr_t)0);
    expect_function_call(pthread_cond_signal);

    will_return(pthread_cond_timedwait, (intptr_t)0);
    expect_function_call(pthread_cond_timedwait);

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

    will_return(get_global_values, cmcList);
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

// Test to verify that no parodus retrieve request is sent when mac is empty 
void test_FR_notify_cloud_status_empty_mac()
{
	numLoops = 1;
	strcpy(deviceMAC, "");
	FactoryResetCloudSync();
}

void err_loadCfgFile()
{
    loadCfgFile();
}

void test_manageable_notification()
{
    parameterValStruct_t **notification = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*));
    notification[0] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t)*1);
    notification[0]->parameterName = strndup("Device.DeviceInfo.X_RDKCENTRAL-COM_xOpsDeviceMgmt.RPC.DeviceManageableNotification",MAX_PARAMETER_LEN);
    notification[0]->parameterValue = strndup("12345",MAX_PARAMETER_LEN);
    notification[0]->type = ccsp_string;

    will_return(get_global_components, getDeviceInfoCompDetails());
    will_return(get_global_component_size, 1);
    expect_function_call(CcspBaseIf_discComponentSupportingNamespace);
    will_return(CcspBaseIf_discComponentSupportingNamespace, CCSP_SUCCESS);
    expect_function_call(free_componentStruct_t);

    will_return(get_global_values, notification);
    will_return(get_global_parameters_count, 1);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterValues, size, 1);
    will_return(get_global_faultParam, NULL);
    will_return(CcspBaseIf_setParameterValues, CCSP_SUCCESS);
    expect_function_call(CcspBaseIf_setParameterValues);
    expect_value(CcspBaseIf_setParameterValues, size, 1);
    processDeviceManageableNotification();
}

void err_manageable_notification()
{
    will_return(get_global_components, NULL);
    will_return(get_global_component_size, 0);
    expect_function_call(CcspBaseIf_discComponentSupportingNamespace);
    will_return(CcspBaseIf_discComponentSupportingNamespace, CCSP_CR_ERR_UNSUPPORTED_NAMESPACE);
    expect_function_call(free_componentStruct_t);

    processDeviceManageableNotification();
}

void test_factory_reset_notification_with_cmc_512()
{
    getCompDetails();
    strcpy(deviceMAC, "abcdeg1234");

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
    cmcList1[0]->parameterValue = strndup("512",MAX_PARAMETER_LEN);
    cmcList1[0]->type = ccsp_int;

    will_return(get_global_values, cidList);
    will_return(get_global_parameters_count, 1);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterValues, size, 1);

    will_return(get_global_values, rebootReasonList);
    will_return(get_global_parameters_count, 1);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterValues, size, 1);

    will_return(get_global_components, getDeviceInfoCompDetails());
    will_return(get_global_component_size, 1);
    expect_function_call(CcspBaseIf_discComponentSupportingNamespace);
    will_return(CcspBaseIf_discComponentSupportingNamespace, CCSP_SUCCESS);
    expect_function_call(free_componentStruct_t);

    will_return(get_global_values, cmcList1);
    will_return(get_global_parameters_count, 1);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterValues, size, 1);

    sendNotificationForFactoryReset();
}

void test_processNotification()
{
	strcpy(deviceMAC, "abcdeg1234");
	NotifyData *notifyData = (NotifyData *)malloc(sizeof(NotifyData));
	memset(notifyData,0,sizeof(NotifyData));

	notifyData->type = CONNECTED_CLIENT_NOTIFY;
	NodeData * node = NULL;
	node = (NodeData *) malloc(sizeof(NodeData) * 1);
	memset(node, 0, sizeof(NodeData));
    node->nodeMacId = strdup("14cfe2142144");
	node->status = strdup("Connected");
	node->interface = strdup("eth0");
	node->hostname = strdup("wifi");
	notifyData->u.node = node;

	parameterValStruct_t **version = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*));
    version[0] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t));
    version[0]->parameterName = strdup("Device.Hosts.X_RDKCENTRAL-COM_HostVersionId");
    version[0]->parameterValue =strdup("123456");
    version[0]->type = ccsp_string;

	parameterValStruct_t **systemTime = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*));
    systemTime[0] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t));
    systemTime[0]->parameterName = strdup("Device.DeviceInfo.X_RDKCENTRAL-COM_SystemTime");
    systemTime[0]->parameterValue =strdup("546543280");
    systemTime[0]->type = ccsp_string;
	getCompDetails();
	will_return(get_global_values, version);
    will_return(get_global_parameters_count, 1);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterValues, size, 1);
	will_return(get_global_components, getDeviceInfoCompDetails());
    will_return(get_global_component_size, 1);
    expect_function_call(CcspBaseIf_discComponentSupportingNamespace);
    will_return(CcspBaseIf_discComponentSupportingNamespace, CCSP_SUCCESS);
    expect_function_call(free_componentStruct_t);
	will_return(get_global_values, systemTime);
    will_return(get_global_parameters_count, 1);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterValues, size, 1);
	will_return(libparodus_send, (intptr_t)0);
    expect_function_call(libparodus_send);
    g_syncRetryThreadStarted = 1; //skip SyncNotifyRetryTask thread creation
	processNotification(notifyData);
}

void test_processNotification_PARAM_NOTIFY()
{
    strcpy(deviceMAC, "abcdeg1234");
	NotifyData *notifyData = (NotifyData *)malloc(sizeof(NotifyData));
	memset(notifyData,0,sizeof(NotifyData));

	notifyData->type = PARAM_NOTIFY;

	NodeData * node = NULL;
    notifyData->u.notify = (ParamNotify*)malloc(sizeof(ParamNotify));
        if (notifyData->u.notify != NULL) 
        {
            notifyData->u.notify->paramName = PARAM_FIRMWARE_VERSION;
            notifyData->u.notify->oldValue = "abcd";
            notifyData->u.notify->newValue = "dcba";
            notifyData->u.notify->type = WDMP_STRING;
            notifyData->u.notify->changeSource = CHANGED_BY_WEBPA;
        } 
	node = (NodeData *) malloc(sizeof(NodeData) * 1);
	memset(node, 0, sizeof(NodeData));
    node->nodeMacId = strdup("14cfe2142144");
	node->status = strdup("Connected");
	node->interface = strdup("eth0");
	node->hostname = strdup("wifi");
	notifyData->u.node = node;

	parameterValStruct_t **version = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*));
    version[0] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t));
    version[0]->parameterName = strdup("Device.Hosts.X_RDKCENTRAL-COM_HostVersionId");
    version[0]->parameterValue =strdup("123456");
    version[0]->type = ccsp_string;

    parameterValStruct_t **version2 = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*));
    version2[0] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t));
    version2[0]->parameterName = strdup("Device.Hosts.X_RDKCENTRAL-COM_HostVersionId");
    version2[0]->parameterValue =strdup("123456");
    version2[0]->type = ccsp_string;

	parameterValStruct_t **systemTime = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*));
    systemTime[0] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t));
    systemTime[0]->parameterName = strdup("Device.DeviceInfo.X_RDKCENTRAL-COM_SystemTime");
    systemTime[0]->parameterValue =strdup("546543280");
    systemTime[0]->type = ccsp_string;
	getCompDetails();
    
	will_return(get_global_values, version);
    will_return(get_global_parameters_count, 1);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterValues, size, 1);
	
    will_return(get_global_values, version2);
    will_return(get_global_parameters_count, 1);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterValues, size, 1);

    will_return(get_global_faultParam, NULL);
    will_return(CcspBaseIf_setParameterValues, CCSP_SUCCESS);
    expect_function_call(CcspBaseIf_setParameterValues);
    expect_value(CcspBaseIf_setParameterValues, size, 1);

    will_return(get_global_values, systemTime);
    will_return(get_global_parameters_count, 1);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterValues, size, 1);
	
    will_return(libparodus_send, (intptr_t)0);
    expect_function_call(libparodus_send);
	processNotification(notifyData);
}

void test_processNotification_DEVICE_STATUS_PAM_FAILED()
{
    strcpy(deviceMAC, "abcdeg1234");
	NotifyData *notifyData = (NotifyData *)malloc(sizeof(NotifyData));
	memset(notifyData,0,sizeof(NotifyData));

	notifyData->type = DEVICE_STATUS;

    notifyData->u.device = (ParamNotify*)malloc(sizeof(ParamNotify));
    if (notifyData->u.device != NULL)
    {
        notifyData->u.device->status = 1;
    }

    parameterValStruct_t **version = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*));
    version[0] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t));
    version[0]->parameterName = strdup("Device.Hosts.X_RDKCENTRAL-COM_HostVersionId");
    version[0]->parameterValue =strdup("123456");
    version[0]->type = ccsp_string;

    will_return(get_global_components, getDeviceInfoCompDetails());
    will_return(get_global_component_size, 1);
    expect_function_call(CcspBaseIf_discComponentSupportingNamespace);
    will_return(CcspBaseIf_discComponentSupportingNamespace, CCSP_SUCCESS);
    expect_function_call(free_componentStruct_t);

    will_return(get_global_values, version);
    will_return(get_global_parameters_count, 1);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterValues, size, 1);

    will_return(libparodus_send, (intptr_t)0);
    expect_function_call(libparodus_send);

	processNotification(notifyData);

}

void test_processNotification_DEVICE_STATUS_success()
{
    strcpy(deviceMAC, "abcdeg1234");
	NotifyData *notifyData = (NotifyData *)malloc(sizeof(NotifyData));
	memset(notifyData,0,sizeof(NotifyData));

	notifyData->type = DEVICE_STATUS;

    notifyData->u.device = (ParamNotify*)malloc(sizeof(ParamNotify));
    if (notifyData->u.device != NULL)
    {
        notifyData->u.device->status = 0;
    }

    parameterValStruct_t **version = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*));
    version[0] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t));
    version[0]->parameterName = strdup("Device.Hosts.X_RDKCENTRAL-COM_HostVersionId");
    version[0]->parameterValue =strdup("123456");
    version[0]->type = ccsp_string;

    will_return(get_global_components, getDeviceInfoCompDetails());
    will_return(get_global_component_size, 1);
    expect_function_call(CcspBaseIf_discComponentSupportingNamespace);
    will_return(CcspBaseIf_discComponentSupportingNamespace, CCSP_SUCCESS);
    expect_function_call(free_componentStruct_t);

    will_return(get_global_values, version);
    will_return(get_global_parameters_count, 1);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterValues, size, 1);

    will_return(libparodus_send, (intptr_t)0);
    expect_function_call(libparodus_send);

	processNotification(notifyData);

}

void test_processNotification_DEVICE_STATUS_epon_fail()
{
    strcpy(deviceMAC, "abcdeg1234");
	NotifyData *notifyData = (NotifyData *)malloc(sizeof(NotifyData));
	memset(notifyData,0,sizeof(NotifyData));

	notifyData->type = DEVICE_STATUS;

    notifyData->u.device = (ParamNotify*)malloc(sizeof(ParamNotify));
    if (notifyData->u.device != NULL)
    {
        notifyData->u.device->status = 2;
    }

    parameterValStruct_t **version = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*));
    version[0] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t));
    version[0]->parameterName = strdup("Device.Hosts.X_RDKCENTRAL-COM_HostVersionId");
    version[0]->parameterValue =strdup("123456");
    version[0]->type = ccsp_string;

    will_return(get_global_components, getDeviceInfoCompDetails());
    will_return(get_global_component_size, 1);
    expect_function_call(CcspBaseIf_discComponentSupportingNamespace);
    will_return(CcspBaseIf_discComponentSupportingNamespace, CCSP_SUCCESS);
    expect_function_call(free_componentStruct_t);

    will_return(get_global_values, version);
    will_return(get_global_parameters_count, 1);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterValues, size, 1);

    will_return(libparodus_send, (intptr_t)0);
    expect_function_call(libparodus_send);

	processNotification(notifyData);
}

void test_processNotification_DEVICE_STATUS_cm_fail()
{
    strcpy(deviceMAC, "abcdeg1234");
	NotifyData *notifyData = (NotifyData *)malloc(sizeof(NotifyData));
	memset(notifyData,0,sizeof(NotifyData));

	notifyData->type = DEVICE_STATUS;

    notifyData->u.device = (ParamNotify*)malloc(sizeof(ParamNotify));
    if (notifyData->u.device != NULL)
    {
        notifyData->u.device->status = 3;
    }

    parameterValStruct_t **version = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*));
    version[0] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t));
    version[0]->parameterName = strdup("Device.Hosts.X_RDKCENTRAL-COM_HostVersionId");
    version[0]->parameterValue =strdup("123456");
    version[0]->type = ccsp_string;

    will_return(get_global_components, getDeviceInfoCompDetails());
    will_return(get_global_component_size, 1);
    expect_function_call(CcspBaseIf_discComponentSupportingNamespace);
    will_return(CcspBaseIf_discComponentSupportingNamespace, CCSP_SUCCESS);
    expect_function_call(free_componentStruct_t);

    will_return(get_global_values, version);
    will_return(get_global_parameters_count, 1);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterValues, size, 1);

    will_return(libparodus_send, (intptr_t)0);
    expect_function_call(libparodus_send);

	processNotification(notifyData);
}

void test_processNotification_DEVICE_STATUS_psm_fail()
{
    strcpy(deviceMAC, "abcdeg1234");
	NotifyData *notifyData = (NotifyData *)malloc(sizeof(NotifyData));
	memset(notifyData,0,sizeof(NotifyData));

	notifyData->type = DEVICE_STATUS;

    notifyData->u.device = (ParamNotify*)malloc(sizeof(ParamNotify));
    if (notifyData->u.device != NULL)
    {
        notifyData->u.device->status = 4;
    }

    parameterValStruct_t **version = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*));
    version[0] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t));
    version[0]->parameterName = strdup("Device.Hosts.X_RDKCENTRAL-COM_HostVersionId");
    version[0]->parameterValue =strdup("123456");
    version[0]->type = ccsp_string;

    will_return(get_global_components, getDeviceInfoCompDetails());
    will_return(get_global_component_size, 1);
    expect_function_call(CcspBaseIf_discComponentSupportingNamespace);
    will_return(CcspBaseIf_discComponentSupportingNamespace, CCSP_SUCCESS);
    expect_function_call(free_componentStruct_t);

    will_return(get_global_values, version);
    will_return(get_global_parameters_count, 1);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterValues, size, 1);

    will_return(libparodus_send, (intptr_t)0);
    expect_function_call(libparodus_send);

	processNotification(notifyData);
}

void test_processNotification_DEVICE_STATUS_wifi_fail()
{
    strcpy(deviceMAC, "abcdeg1234");
	NotifyData *notifyData = (NotifyData *)malloc(sizeof(NotifyData));
	memset(notifyData,0,sizeof(NotifyData));

	notifyData->type = DEVICE_STATUS;

    notifyData->u.device = (ParamNotify*)malloc(sizeof(ParamNotify));
    if (notifyData->u.device != NULL)
    {
        notifyData->u.device->status = 5;
    }

    parameterValStruct_t **version = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*));
    version[0] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t));
    version[0]->parameterName = strdup("Device.Hosts.X_RDKCENTRAL-COM_HostVersionId");
    version[0]->parameterValue =strdup("123456");
    version[0]->type = ccsp_string;

    will_return(get_global_components, getDeviceInfoCompDetails());
    will_return(get_global_component_size, 1);
    expect_function_call(CcspBaseIf_discComponentSupportingNamespace);
    will_return(CcspBaseIf_discComponentSupportingNamespace, CCSP_SUCCESS);
    expect_function_call(free_componentStruct_t);

    will_return(get_global_values, version);
    will_return(get_global_parameters_count, 1);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterValues, size, 1);

    will_return(libparodus_send, (intptr_t)0);
    expect_function_call(libparodus_send);

	processNotification(notifyData);
}

void test_processNotification_DEVICE_STATUS_fail()
{
    strcpy(deviceMAC, "abcdeg1234");
	NotifyData *notifyData = (NotifyData *)malloc(sizeof(NotifyData));
	memset(notifyData,0,sizeof(NotifyData));

	notifyData->type = DEVICE_STATUS;

    notifyData->u.device = (ParamNotify*)malloc(sizeof(ParamNotify));
    if (notifyData->u.device != NULL)
    {
        notifyData->u.device->status = 6;
    }

    parameterValStruct_t **version = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*));
    version[0] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t));
    version[0]->parameterName = strdup("Device.Hosts.X_RDKCENTRAL-COM_HostVersionId");
    version[0]->parameterValue =strdup("123456");
    version[0]->type = ccsp_string;

    will_return(get_global_components, getDeviceInfoCompDetails());
    will_return(get_global_component_size, 1);
    expect_function_call(CcspBaseIf_discComponentSupportingNamespace);
    will_return(CcspBaseIf_discComponentSupportingNamespace, CCSP_SUCCESS);
    expect_function_call(free_componentStruct_t);

    will_return(get_global_values, version);
    will_return(get_global_parameters_count, 1);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterValues, size, 1);

    will_return(libparodus_send, (intptr_t)0);
    expect_function_call(libparodus_send);

	processNotification(notifyData);
}

int writeFile(const char* fileName, const char* content) {
	FILE *fp;
	fp = fopen(fileName , "w+");
    if (fp == NULL) {
        perror("Error opening file:");
        return 1;  // Return an error code
    }

    fprintf(fp, "%s\n", content);

    fclose(fp);

    WalInfo("Content written to %s\n", fileName);

    return 0;  // Return success
}

void test_addOrUpdateFirmwareVerToConfigFile()
{
    getCompDetails();
    int count = 1;
    strcpy(deviceMAC, "abcdeg1234");
    const char* fileContent = "{\n\"oldFirmwareVersion\": \"CGM4331COM_DEV_23Q4_sprint_20231121134451sdy\"\n}";
    int result = writeFile(WEBPA_CFG_FILE, fileContent);

    parameterValStruct_t **firmwareList = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*));
    firmwareList[0] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t)*1);
    firmwareList[0]->parameterName = strndup(PARAM_FIRMWARE_VERSION,MAX_PARAMETER_LEN);
    firmwareList[0]->parameterValue = strndup("CGM4331COM_DEV_23Q4_sprint_20231121134451sdy",MAX_PARAMETER_LEN);
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

    componentStruct_t **list = (componentStruct_t **) malloc(sizeof(componentStruct_t *)*count);
    list[0] = (componentStruct_t *) malloc(sizeof(componentStruct_t));
    list[0]->componentName = strdup(RDKB_WIFI_FULL_COMPONENT_NAME);
    list[0]->dbusPath = strdup(RDKB_WIFI_DBUS_PATH);

    componentStruct_t **list1 = (componentStruct_t **) malloc(sizeof(componentStruct_t *)*count);
    list1[0] = (componentStruct_t *) malloc(sizeof(componentStruct_t));
    list1[0]->componentName = strdup(RDKB_WIFI_FULL_COMPONENT_NAME);
    list1[0]->dbusPath = strdup(RDKB_WIFI_DBUS_PATH);

    componentStruct_t **list3 = (componentStruct_t **) malloc(sizeof(componentStruct_t *)*count);
    list3[0] = (componentStruct_t *) malloc(sizeof(componentStruct_t));
    list3[0]->componentName = strdup(RDKB_WIFI_FULL_COMPONENT_NAME);
    list3[0]->dbusPath = strdup(RDKB_WIFI_DBUS_PATH);

    will_return(get_global_components, list);
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

void test_loadCfgFile_success()
{
    const char* fileContent = "{\n\"oldFirmwareVersion\": \"CGM4331COM_DEV_23Q4_sprint_20231121134451sdy\"\n}";
    int result = writeFile(WEBPA_CFG_FILE, fileContent);

    loadCfgFile();
}

void test_getDeviceMac()
{
    getCompDetails();
    strcpy(deviceMAC, "abcdeg1234");
    getDeviceMac();
}

void test_syncNotifyRetry()
{

    numLoops = 1;
    getCompDetails();
    strcpy(deviceMAC, "abcdeg1234");
    
    will_return(pthread_cond_signal, (intptr_t)0);
    expect_function_call(pthread_cond_signal);
   
    will_return(pthread_cond_timedwait, (intptr_t)0);
    expect_function_call(pthread_cond_timedwait);
 
    g_checkSyncNotifyRetry = 1;
    g_syncNotifyInProgress = 0;
    
    parameterValStruct_t **cidList = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*));
    cidList[0] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t)*1);
    cidList[0]->parameterName = strndup(PARAM_CID,MAX_PARAMETER_LEN);
    cidList[0]->parameterValue = strndup("61f4db9",MAX_PARAMETER_LEN);
    cidList[0]->type = ccsp_string;
    
    parameterValStruct_t **cmcList = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*));
    cmcList[0] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t)*1);
    cmcList[0]->parameterName = strndup(PARAM_CMC,MAX_PARAMETER_LEN);
    cmcList[0]->parameterValue = strndup("768",MAX_PARAMETER_LEN);
    cmcList[0]->type = ccsp_int;

    
    parameterValStruct_t **cmcList2 = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*));
    cmcList2[0] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t)*1);
    cmcList2[0]->parameterName = strndup(PARAM_CMC,MAX_PARAMETER_LEN);
    cmcList2[0]->parameterValue = strndup("512",MAX_PARAMETER_LEN);
    cmcList2[0]->type = ccsp_int;
    
    
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
    
    will_return(get_global_values, cidList);
    will_return(get_global_parameters_count, 1);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterValues, size, 1);
    
    will_return(libparodus_send, (intptr_t)0);
    expect_function_call(libparodus_send);

    SyncNotifyRetry();
}

void test_syncNotifyRetry_timedout()
{

    numLoops = 1;
    getCompDetails();
    strcpy(deviceMAC, "abcdeg1234");
    
    will_return(pthread_cond_signal, (intptr_t)0);
    expect_function_call(pthread_cond_signal);
   
    will_return(pthread_cond_timedwait, (intptr_t)ETIMEDOUT);
    expect_function_call(pthread_cond_timedwait);

    g_checkSyncNotifyRetry = 1;
    g_syncNotifyInProgress = 0;
    
    parameterValStruct_t **cidList = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*));
    cidList[0] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t)*1);
    cidList[0]->parameterName = strndup(PARAM_CID,MAX_PARAMETER_LEN);
    cidList[0]->parameterValue = strndup("61f4db9",MAX_PARAMETER_LEN);
    cidList[0]->type = ccsp_string;
    
    parameterValStruct_t **cmcList = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*));
    cmcList[0] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t)*1);
    cmcList[0]->parameterName = strndup(PARAM_CMC,MAX_PARAMETER_LEN);
    cmcList[0]->parameterValue = strndup("768",MAX_PARAMETER_LEN);
    cmcList[0]->type = ccsp_int;

    
    parameterValStruct_t **cmcList2 = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*));
    cmcList2[0] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t)*1);
    cmcList2[0]->parameterName = strndup(PARAM_CMC,MAX_PARAMETER_LEN);
    cmcList2[0]->parameterValue = strndup("512",MAX_PARAMETER_LEN);
    cmcList2[0]->type = ccsp_int;
    

    
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
    
    will_return(get_global_values, cidList);
    will_return(get_global_parameters_count, 1);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterValues, size, 1);
    
    will_return(libparodus_send, (intptr_t)0);
    expect_function_call(libparodus_send);

    SyncNotifyRetry();
}
void test_syncNotifyRetry_pthread_error()
{
    numLoops = 2;
    getCompDetails();
    strcpy(deviceMAC, "abcdeg1234");
    
    will_return(pthread_cond_signal, (intptr_t)0);
    expect_function_call(pthread_cond_signal);
   
    will_return(pthread_cond_timedwait, (intptr_t)2);
    expect_function_call(pthread_cond_timedwait);
 
     will_return(pthread_cond_signal, (intptr_t)0);
    expect_function_call(pthread_cond_signal);
    
    will_return(pthread_cond_timedwait, (intptr_t)0); 
    expect_function_call(pthread_cond_timedwait);
    
    g_checkSyncNotifyRetry = 1;
    g_syncNotifyInProgress = 0;
    
    parameterValStruct_t **cidList = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*));
    cidList[0] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t)*1);
    cidList[0]->parameterName = strndup(PARAM_CID,MAX_PARAMETER_LEN);
    cidList[0]->parameterValue = strndup("61f4db9",MAX_PARAMETER_LEN);
    cidList[0]->type = ccsp_string;
    
    parameterValStruct_t **cmcList = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*));
    cmcList[0] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t)*1);
    cmcList[0]->parameterName = strndup(PARAM_CMC,MAX_PARAMETER_LEN);
    cmcList[0]->parameterValue = strndup("768",MAX_PARAMETER_LEN);
    cmcList[0]->type = ccsp_int;
  
    parameterValStruct_t **cmcList2 = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*));
    cmcList2[0] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t)*1);
    cmcList2[0]->parameterName = strndup(PARAM_CMC,MAX_PARAMETER_LEN);
    cmcList2[0]->parameterValue = strndup("512",MAX_PARAMETER_LEN);
    cmcList2[0]->type = ccsp_int;
    
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
    
    will_return(get_global_values, cidList);
    will_return(get_global_parameters_count, 1);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterValues, size, 1);
    
    will_return(libparodus_send, (intptr_t)0);
    expect_function_call(libparodus_send);

    SyncNotifyRetry();
}

void test_syncNotifyRetry_NotifyInProgress()
{

    numLoops = 1;
    g_checkSyncNotifyRetry = 1;
    g_syncNotifyInProgress = 1;

    getCompDetails();
    strcpy(deviceMAC, "abcdeg1234");

    will_return(pthread_cond_signal, (intptr_t)0);
    expect_function_call(pthread_cond_signal);
    
    will_return(pthread_cond_timedwait, (intptr_t)0); 
    expect_function_call(pthread_cond_timedwait);

    SyncNotifyRetry();
    
    numLoops = 1;
    g_syncNotifyInProgress = 0;
    
    will_return(pthread_cond_signal, (intptr_t)0);
    expect_function_call(pthread_cond_signal);
    
    will_return(pthread_cond_timedwait, (intptr_t)0); 
    expect_function_call(pthread_cond_timedwait);

    parameterValStruct_t **cidList = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*));
    cidList[0] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t)*1);
    cidList[0]->parameterName = strndup(PARAM_CID,MAX_PARAMETER_LEN);
    cidList[0]->parameterValue = strndup("61f4db9",MAX_PARAMETER_LEN);
    cidList[0]->type = ccsp_string;
    
    parameterValStruct_t **cmcList = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*));
    cmcList[0] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t)*1);
    cmcList[0]->parameterName = strndup(PARAM_CMC,MAX_PARAMETER_LEN);
    cmcList[0]->parameterValue = strndup("768",MAX_PARAMETER_LEN);
    cmcList[0]->type = ccsp_int;
    
    parameterValStruct_t **cmcList2 = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*));
    cmcList2[0] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t)*1);
    cmcList2[0]->parameterName = strndup(PARAM_CMC,MAX_PARAMETER_LEN);
    cmcList2[0]->parameterValue = strndup("512",MAX_PARAMETER_LEN);
    cmcList2[0]->type = ccsp_int;
    
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
    
    will_return(get_global_values, cidList);
    will_return(get_global_parameters_count, 1);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterValues, size, 1);
    
    will_return(libparodus_send, (intptr_t)0);
    expect_function_call(libparodus_send);

    SyncNotifyRetry();
}

void test_syncNotifyRetry_Skip_CMC_Check()
{
    numLoops = 1;
    getCompDetails();
    strcpy(deviceMAC, "abcdeg1234");
    
    g_checkSyncNotifyRetry = 0;
    g_syncNotifyInProgress = 0;
    
    will_return(pthread_cond_signal, (intptr_t)0);
    expect_function_call(pthread_cond_signal);
   
    will_return(pthread_cond_timedwait, (intptr_t)ETIMEDOUT); 
    expect_function_call(pthread_cond_timedwait);
    
    SyncNotifyRetry();
    
    numLoops = 1;
    g_checkSyncNotifyRetry = 1;
    
    will_return(pthread_cond_signal, (intptr_t)0);
    expect_function_call(pthread_cond_signal);
    
    will_return(pthread_cond_timedwait, (intptr_t)0); 
    expect_function_call(pthread_cond_timedwait);
      
    parameterValStruct_t **cidList = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*));
    cidList[0] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t)*1);
    cidList[0]->parameterName = strndup(PARAM_CID,MAX_PARAMETER_LEN);
    cidList[0]->parameterValue = strndup("61f4db9",MAX_PARAMETER_LEN);
    cidList[0]->type = ccsp_string;
    
    parameterValStruct_t **cmcList = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*));
    cmcList[0] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t)*1);
    cmcList[0]->parameterName = strndup(PARAM_CMC,MAX_PARAMETER_LEN);
    cmcList[0]->parameterValue = strndup("768",MAX_PARAMETER_LEN);
    cmcList[0]->type = ccsp_int;

    parameterValStruct_t **cmcList2 = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*));
    cmcList2[0] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t)*1);
    cmcList2[0]->parameterName = strndup(PARAM_CMC,MAX_PARAMETER_LEN);
    cmcList2[0]->parameterValue = strndup("512",MAX_PARAMETER_LEN);
    cmcList2[0]->type = ccsp_int;
       
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
    
    will_return(get_global_values, cidList);
    will_return(get_global_parameters_count, 1);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterValues, size, 1);
    
    will_return(libparodus_send, (intptr_t)0);
    expect_function_call(libparodus_send);

    SyncNotifyRetry();    
}

void test_syncNotifyRetry_dbCMC_NULL()
{
    numLoops = 1;
    g_checkSyncNotifyRetry = 1;
    g_syncNotifyInProgress = 0;

    getCompDetails();
    strcpy(deviceMAC, "abcdeg1234");

    will_return(pthread_cond_signal, (intptr_t)0);
    expect_function_call(pthread_cond_signal);
    
    will_return(pthread_cond_timedwait, (intptr_t)0); 
    expect_function_call(pthread_cond_timedwait);

    parameterValStruct_t **cmcList = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*));
    cmcList[0] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t)*1);
    cmcList[0]->parameterName = strndup(PARAM_CMC,MAX_PARAMETER_LEN);
    cmcList[0]->parameterValue = strndup("768",MAX_PARAMETER_LEN);
    cmcList[0]->type = ccsp_int;
    
    will_return(get_global_values, cmcList);
    will_return(get_global_parameters_count, 1);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_FAILURE);
    expect_value(CcspBaseIf_getParameterValues, size, 1);
    
    SyncNotifyRetry();
    
    numLoops = 1;
    g_syncNotifyInProgress = 0;
    
    will_return(pthread_cond_signal, (intptr_t)0);
    expect_function_call(pthread_cond_signal);
    
    will_return(pthread_cond_timedwait, (intptr_t)0); 
    expect_function_call(pthread_cond_timedwait);
      
    parameterValStruct_t **cidList = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*));
    cidList[0] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t)*1);
    cidList[0]->parameterName = strndup(PARAM_CID,MAX_PARAMETER_LEN);
    cidList[0]->parameterValue = strndup("61f4db9",MAX_PARAMETER_LEN);
    cidList[0]->type = ccsp_string;
    
    parameterValStruct_t **cmcList1 = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*));
    cmcList1[0] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t)*1);
    cmcList1[0]->parameterName = strndup(PARAM_CMC,MAX_PARAMETER_LEN);
    cmcList1[0]->parameterValue = strndup("768",MAX_PARAMETER_LEN);
    cmcList1[0]->type = ccsp_int;

    parameterValStruct_t **cmcList2 = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*));
    cmcList2[0] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t)*1);
    cmcList2[0]->parameterName = strndup(PARAM_CMC,MAX_PARAMETER_LEN);
    cmcList2[0]->parameterValue = strndup("512",MAX_PARAMETER_LEN);
    cmcList2[0]->type = ccsp_int;
       
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
    
    will_return(get_global_values, cidList);
    will_return(get_global_parameters_count, 1);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterValues, size, 1);
    
    will_return(libparodus_send, (intptr_t)0);
    expect_function_call(libparodus_send);

    SyncNotifyRetry();    
}

void test_syncNotifyRetry_CMC_Is_512()
{
    numLoops = 1;
    getCompDetails();
    strcpy(deviceMAC, "abcdeg1234");
    
    will_return(pthread_cond_signal, (intptr_t)0);
    expect_function_call(pthread_cond_signal);
    
    will_return(pthread_cond_timedwait, (intptr_t)0); 
    expect_function_call(pthread_cond_timedwait);
    
    g_checkSyncNotifyRetry = 1;
    g_syncNotifyInProgress = 0;
    
    parameterValStruct_t **cmcList = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*));
    cmcList[0] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t)*1);
    cmcList[0]->parameterName = strndup(PARAM_CMC,MAX_PARAMETER_LEN);
    cmcList[0]->parameterValue = strndup("512",MAX_PARAMETER_LEN);
    cmcList[0]->type = ccsp_int;
    
    will_return(get_global_values, cmcList);
    will_return(get_global_parameters_count, 1);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterValues, size, 1);

    SyncNotifyRetry();
}

void test_processNotification_PARAM_NOTIFY_RETRY_SUCCESS()
{
    strcpy(deviceMAC, "abcdeg1234");
    NotifyData *notifyData = (NotifyData *)malloc(sizeof(NotifyData));
    memset(notifyData,0,sizeof(NotifyData));

    notifyData->type = PARAM_NOTIFY_RETRY;

    NodeData * node = NULL;
    notifyData->u.notify = (ParamNotify*)malloc(sizeof(ParamNotify));
    if (notifyData->u.notify != NULL) 
    {
        notifyData->u.notify->paramName = PARAM_FIRMWARE_VERSION;
        notifyData->u.notify->oldValue = "abcd";
        notifyData->u.notify->newValue = "dcba";
        notifyData->u.notify->type = WDMP_STRING;
        notifyData->u.notify->changeSource = CHANGED_BY_WEBPA;
    } 
    node = (NodeData *) malloc(sizeof(NodeData) * 1);
    memset(node, 0, sizeof(NodeData));
    node->nodeMacId = strdup("14cfe2142144");
    node->status = strdup("Connected");
    node->interface = strdup("eth0");
    node->hostname = strdup("wifi");
    notifyData->u.node = node;

    parameterValStruct_t **version = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*));
    version[0] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t));
    version[0]->parameterName = strdup("Device.Hosts.X_RDKCENTRAL-COM_HostVersionId");
    version[0]->parameterValue =strdup("123456");
    version[0]->type = ccsp_string;

    parameterValStruct_t **version2 = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*));
    version2[0] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t));
    version2[0]->parameterName = strdup("Device.Hosts.X_RDKCENTRAL-COM_HostVersionId");
    version2[0]->parameterValue =strdup("123456");
    version2[0]->type = ccsp_string;

    getCompDetails();
    
    will_return(get_global_values, version);
    will_return(get_global_parameters_count, 1);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterValues, size, 1);
	
    will_return(get_global_values, version2);
    will_return(get_global_parameters_count, 1);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterValues, size, 1);
	
    will_return(libparodus_send, (intptr_t)0);
    expect_function_call(libparodus_send);
    
    processNotification(notifyData);
}

void test_processNotification_PARAM_NOTIFY_RETRY_FAIL()
{
    strcpy(deviceMAC, "abcdeg1234");
    NotifyData *notifyData = (NotifyData *)malloc(sizeof(NotifyData));
    memset(notifyData,0,sizeof(NotifyData));
    
    notifyData->type = PARAM_NOTIFY_RETRY;
    
    NodeData * node = NULL;
    notifyData->u.notify = (ParamNotify*)malloc(sizeof(ParamNotify));
    if (notifyData->u.notify != NULL) 
    {
        notifyData->u.notify->paramName = PARAM_FIRMWARE_VERSION;
        notifyData->u.notify->oldValue = "abcd";
        notifyData->u.notify->newValue = "dcba";
        notifyData->u.notify->type = WDMP_STRING;
        notifyData->u.notify->changeSource = CHANGED_BY_WEBPA;
    } 
    node = (NodeData *) malloc(sizeof(NodeData) * 1);
    memset(node, 0, sizeof(NodeData));
    node->nodeMacId = strdup("14cfe2142144");
    node->status = strdup("Connected");
    node->interface = strdup("eth0");
    node->hostname = strdup("wifi");
    notifyData->u.node = node;

    parameterValStruct_t **version = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*));
    version[0] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t));
    version[0]->parameterName = strdup("Device.Hosts.X_RDKCENTRAL-COM_HostVersionId");
    version[0]->parameterValue =strdup("NULL");
    version[0]->type = ccsp_string;

    getCompDetails();
    
    will_return(get_global_values, version);
    will_return(get_global_parameters_count, 1);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_FAILURE);
    expect_value(CcspBaseIf_getParameterValues, size, 1);
	
    processNotification(notifyData);
}

void test_FR_CloudSyncCheck()
{
    getCompDetails();    
    parameterValStruct_t **cmcList = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*));
    cmcList[0] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t)*1);
    cmcList[0]->parameterName = strndup(PARAM_CMC,MAX_PARAMETER_LEN);
    cmcList[0]->parameterValue = strndup("512",MAX_PARAMETER_LEN);
    cmcList[0]->type = ccsp_int;
    
    parameterValStruct_t **cidList = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*));
    cidList[0] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t)*1);
    cidList[0]->parameterName = strndup(PARAM_CID,MAX_PARAMETER_LEN);
    cidList[0]->parameterValue = strndup("6789",MAX_PARAMETER_LEN);
    cidList[0]->type = ccsp_string;

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

    FR_CloudSyncCheck();
}

void test_FR_CloudSyncCheck_FRSyncFailed()
{
    getCompDetails();

    parameterValStruct_t **cmcList1 = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*));
    cmcList1[0] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t)*1);
    cmcList1[0]->parameterName = strndup(PARAM_CMC,MAX_PARAMETER_LEN);
    cmcList1[0]->parameterValue = strndup("1",MAX_PARAMETER_LEN);
    cmcList1[0]->type = ccsp_int;
    
    parameterValStruct_t **cidList1 = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*));
    cidList1[0] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t)*1);
    cidList1[0]->parameterName = strndup(PARAM_CID,MAX_PARAMETER_LEN);
    cidList1[0]->parameterValue = strndup("0",MAX_PARAMETER_LEN);
    cidList1[0]->type = ccsp_string;

    parameterValStruct_t **cmcList2 = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*));
    cmcList2[0] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t)*1);
    cmcList2[0]->parameterName = strndup(PARAM_CMC,MAX_PARAMETER_LEN);
    cmcList2[0]->parameterValue = strndup("1",MAX_PARAMETER_LEN);
    cmcList2[0]->type = ccsp_int;
    
    parameterValStruct_t **cidList2 = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*));
    cidList2[0] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t)*1);
    cidList2[0]->parameterName = strndup(PARAM_CID,MAX_PARAMETER_LEN);
    cidList2[0]->parameterValue = strndup("0",MAX_PARAMETER_LEN);
    cidList2[0]->type = ccsp_string;
    
    parameterValStruct_t **cmcList3 = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*));
    cmcList3[0] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t)*1);
    cmcList3[0]->parameterName = strndup(PARAM_CMC,MAX_PARAMETER_LEN);
    cmcList3[0]->parameterValue = strndup("1",MAX_PARAMETER_LEN);
    cmcList3[0]->type = ccsp_int;
    
    parameterValStruct_t **cidList3 = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*));
    cidList3[0] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t)*1);
    cidList3[0]->parameterName = strndup(PARAM_CID,MAX_PARAMETER_LEN);
    cidList3[0]->parameterValue = strndup("0",MAX_PARAMETER_LEN);
    cidList3[0]->type = ccsp_string;

    parameterValStruct_t **cmcList4 = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*));
    cmcList4[0] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t)*1);
    cmcList4[0]->parameterName = strndup(PARAM_CMC,MAX_PARAMETER_LEN);
    cmcList4[0]->parameterValue = strndup("1",MAX_PARAMETER_LEN);
    cmcList4[0]->type = ccsp_int;
    
    parameterValStruct_t **cidList4 = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*));
    cidList4[0] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t)*1);
    cidList4[0]->parameterName = strndup(PARAM_CID,MAX_PARAMETER_LEN);
    cidList4[0]->parameterValue = strndup("0",MAX_PARAMETER_LEN);
    cidList4[0]->type = ccsp_string;

    parameterValStruct_t **cmcList5 = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*));
    cmcList5[0] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t)*1);
    cmcList5[0]->parameterName = strndup(PARAM_CMC,MAX_PARAMETER_LEN);
    cmcList5[0]->parameterValue = strndup("1",MAX_PARAMETER_LEN);
    cmcList5[0]->type = ccsp_int;
    
    parameterValStruct_t **cidList5 = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*));
    cidList5[0] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t)*1);
    cidList5[0]->parameterName = strndup(PARAM_CID,MAX_PARAMETER_LEN);
    cidList5[0]->parameterValue = strndup("0",MAX_PARAMETER_LEN);
    cidList5[0]->type = ccsp_string;

    parameterValStruct_t **cmcList6 = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*));
    cmcList6[0] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t)*1);
    cmcList6[0]->parameterName = strndup(PARAM_CMC,MAX_PARAMETER_LEN);
    cmcList6[0]->parameterValue = strndup("1",MAX_PARAMETER_LEN);
    cmcList6[0]->type = ccsp_int;
    
    parameterValStruct_t **cidList6 = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*));
    cidList6[0] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t)*1);
    cidList6[0]->parameterName = strndup(PARAM_CID,MAX_PARAMETER_LEN);
    cidList6[0]->parameterValue = strndup("0",MAX_PARAMETER_LEN);
    cidList6[0]->type = ccsp_string;

    parameterValStruct_t **cmcList7 = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*));
    cmcList7[0] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t)*1);
    cmcList7[0]->parameterName = strndup(PARAM_CMC,MAX_PARAMETER_LEN);
    cmcList7[0]->parameterValue = strndup("1",MAX_PARAMETER_LEN);
    cmcList7[0]->type = ccsp_int;
    
    parameterValStruct_t **cidList7 = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*));
    cidList7[0] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t)*1);
    cidList7[0]->parameterName = strndup(PARAM_CID,MAX_PARAMETER_LEN);
    cidList7[0]->parameterValue = strndup("0",MAX_PARAMETER_LEN);
    cidList7[0]->type = ccsp_string;

    parameterValStruct_t **cmcList8 = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*));
    cmcList8[0] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t)*1);
    cmcList8[0]->parameterName = strndup(PARAM_CMC,MAX_PARAMETER_LEN);
    cmcList8[0]->parameterValue = strndup("1",MAX_PARAMETER_LEN);
    cmcList8[0]->type = ccsp_int;
    
    parameterValStruct_t **cidList8 = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*));
    cidList8[0] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t)*1);
    cidList8[0]->parameterName = strndup(PARAM_CID,MAX_PARAMETER_LEN);
    cidList8[0]->parameterValue = strndup("0",MAX_PARAMETER_LEN);
    cidList8[0]->type = ccsp_string;
    
    parameterValStruct_t **cmcList9 = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*));
    cmcList9[0] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t)*1);
    cmcList9[0]->parameterName = strndup(PARAM_CMC,MAX_PARAMETER_LEN);
    cmcList9[0]->parameterValue = strndup("1",MAX_PARAMETER_LEN);
    cmcList9[0]->type = ccsp_int;
    
    parameterValStruct_t **cidList9 = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*));
    cidList9[0] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t)*1);
    cidList9[0]->parameterName = strndup(PARAM_CID,MAX_PARAMETER_LEN);
    cidList9[0]->parameterValue = strndup("0",MAX_PARAMETER_LEN);
    cidList9[0]->type = ccsp_string;

    parameterValStruct_t **cmcList10 = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*));
    cmcList10[0] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t)*1);
    cmcList10[0]->parameterName = strndup(PARAM_CMC,MAX_PARAMETER_LEN);
    cmcList10[0]->parameterValue = strndup("1",MAX_PARAMETER_LEN);
    cmcList10[0]->type = ccsp_int;
    
    parameterValStruct_t **cidList10 = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*));
    cidList10[0] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t)*1);
    cidList10[0]->parameterName = strndup(PARAM_CID,MAX_PARAMETER_LEN);
    cidList10[0]->parameterValue = strndup("0",MAX_PARAMETER_LEN);
    cidList10[0]->type = ccsp_string;

    parameterValStruct_t **cmcList11 = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*));
    cmcList11[0] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t)*1);
    cmcList11[0]->parameterName = strndup(PARAM_CMC,MAX_PARAMETER_LEN);
    cmcList11[0]->parameterValue = strndup("1",MAX_PARAMETER_LEN);
    cmcList11[0]->type = ccsp_int;
    
    parameterValStruct_t **cidList11 = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*));
    cidList11[0] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t)*1);
    cidList11[0]->parameterName = strndup(PARAM_CID,MAX_PARAMETER_LEN);
    cidList11[0]->parameterValue = strndup("0",MAX_PARAMETER_LEN);
    cidList11[0]->type = ccsp_string;

    parameterValStruct_t **cmcList12 = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*));
    cmcList12[0] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t)*1);
    cmcList12[0]->parameterName = strndup(PARAM_CMC,MAX_PARAMETER_LEN);
    cmcList12[0]->parameterValue = strndup("1",MAX_PARAMETER_LEN);
    cmcList12[0]->type = ccsp_int;
    
    parameterValStruct_t **cidList12 = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*));
    cidList12[0] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t)*1);
    cidList12[0]->parameterName = strndup(PARAM_CID,MAX_PARAMETER_LEN);
    cidList12[0]->parameterValue = strndup("0",MAX_PARAMETER_LEN);
    cidList12[0]->type = ccsp_string;

  
    will_return(get_global_values, cmcList1);
    will_return(get_global_parameters_count, 1);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterValues, size, 1);
	
    will_return(get_global_values, cidList1);
    will_return(get_global_parameters_count, 1);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterValues, size, 1);

    will_return(get_global_values, cmcList2);
    will_return(get_global_parameters_count, 1);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterValues, size, 1);
	
    will_return(get_global_values, cidList2);
    will_return(get_global_parameters_count, 1);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterValues, size, 1);
    
    will_return(get_global_values, cmcList3);
    will_return(get_global_parameters_count, 1);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterValues, size, 1);
	
    will_return(get_global_values, cidList3);
    will_return(get_global_parameters_count, 1);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterValues, size, 1);

    will_return(get_global_values, cmcList4);
    will_return(get_global_parameters_count, 1);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterValues, size, 1);
	
    will_return(get_global_values, cidList4);
    will_return(get_global_parameters_count, 1);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterValues, size, 1);

    will_return(get_global_values, cmcList5);
    will_return(get_global_parameters_count, 1);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterValues, size, 1);
	
    will_return(get_global_values, cidList5);
    will_return(get_global_parameters_count, 1);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterValues, size, 1);

    will_return(get_global_values, cmcList6);
    will_return(get_global_parameters_count, 1);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterValues, size, 1);
	
    will_return(get_global_values, cidList6);
    will_return(get_global_parameters_count, 1);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterValues, size, 1);

    will_return(get_global_values, cmcList7);
    will_return(get_global_parameters_count, 1);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterValues, size, 1);
	
    will_return(get_global_values, cidList7);
    will_return(get_global_parameters_count, 1);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterValues, size, 1);

    will_return(get_global_values, cmcList8);
    will_return(get_global_parameters_count, 1);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterValues, size, 1);
	
    will_return(get_global_values, cidList8);
    will_return(get_global_parameters_count, 1);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterValues, size, 1);

    will_return(get_global_values, cmcList9);
    will_return(get_global_parameters_count, 1);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterValues, size, 1);
	
    will_return(get_global_values, cidList9);
    will_return(get_global_parameters_count, 1);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterValues, size, 1);

    will_return(get_global_values, cmcList10);
    will_return(get_global_parameters_count, 1);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterValues, size, 1);
	
    will_return(get_global_values, cidList10);
    will_return(get_global_parameters_count, 1);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterValues, size, 1);

    will_return(get_global_values, cmcList11);
    will_return(get_global_parameters_count, 1);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterValues, size, 1);
	
    will_return(get_global_values, cidList11);
    will_return(get_global_parameters_count, 1);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterValues, size, 1);

    will_return(get_global_values, cmcList12);
    will_return(get_global_parameters_count, 1);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterValues, size, 1);
	
    will_return(get_global_values, cidList12);
    will_return(get_global_parameters_count, 1);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterValues, size, 1);            

    FR_CloudSyncCheck();
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
        cmocka_unit_test(err_loadCfgFile),
        cmocka_unit_test(test_transaction_status_notification),   
        cmocka_unit_test(test_FR_cloud_sync_notification_retry),
	    cmocka_unit_test(test_FR_notify_cloud_status_retry),
	    cmocka_unit_test(test_FR_notify_cloud_status_empty_mac),
	    cmocka_unit_test(test_manageable_notification),
	    cmocka_unit_test(err_manageable_notification),
	cmocka_unit_test(test_factory_reset_notification_with_cmc_512),
		cmocka_unit_test(test_processNotification),
        cmocka_unit_test(test_processNotification_PARAM_NOTIFY),
        cmocka_unit_test(test_processNotification_DEVICE_STATUS_PAM_FAILED),
        cmocka_unit_test(test_processNotification_DEVICE_STATUS_success),
        cmocka_unit_test(test_processNotification_DEVICE_STATUS_epon_fail),
        cmocka_unit_test(test_processNotification_DEVICE_STATUS_cm_fail),
        cmocka_unit_test(test_processNotification_DEVICE_STATUS_psm_fail),
        cmocka_unit_test(test_processNotification_DEVICE_STATUS_wifi_fail),
        cmocka_unit_test(test_processNotification_DEVICE_STATUS_fail),
        cmocka_unit_test(test_addOrUpdateFirmwareVerToConfigFile),
        cmocka_unit_test(test_loadCfgFile_success),
        cmocka_unit_test(test_getDeviceMac),
        cmocka_unit_test(test_syncNotifyRetry),
        cmocka_unit_test(test_syncNotifyRetry_timedout),
        cmocka_unit_test(test_syncNotifyRetry_pthread_error),
        cmocka_unit_test(test_syncNotifyRetry_NotifyInProgress),
        cmocka_unit_test(test_syncNotifyRetry_Skip_CMC_Check),
        cmocka_unit_test(test_syncNotifyRetry_dbCMC_NULL),
        cmocka_unit_test(test_syncNotifyRetry_CMC_Is_512),
        cmocka_unit_test(test_processNotification_PARAM_NOTIFY_RETRY_SUCCESS),
        cmocka_unit_test(test_processNotification_PARAM_NOTIFY_RETRY_FAIL),
        cmocka_unit_test(test_FR_CloudSyncCheck),
        cmocka_unit_test(test_FR_CloudSyncCheck_FRSyncFailed),
    };

    numLoops = 1;

    return cmocka_run_group_tests(tests, NULL, NULL);
}
