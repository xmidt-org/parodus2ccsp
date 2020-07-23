/*
 * Copyright 2020 Comcast Cable Communications Management, LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "webpa_internal.h"
#include <webcfg_generic.h>
/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
#define SERIAL_NUMBER                "Device.DeviceInfo.SerialNumber"
#define FIRMWARE_VERSION             "Device.DeviceInfo.X_CISCO_COM_FirmwareName"
#define DEVICE_BOOT_TIME             "Device.DeviceInfo.X_RDKCENTRAL-COM_BootTime"
#define MODEL_NAME		     "Device.DeviceInfo.ModelName"
#define PRODUCT_CLASS		     "Device.DeviceInfo.ProductClass"
#define CONN_CLIENT_PARAM	     "Device.NotifyComponent.X_RDKCENTRAL-COM_Connected-Client"
/*----------------------------------------------------------------------------*/
/*                               Data Structures                              */
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/
char * getSerialNumber()
{
	char *serialNum = NULL;
	serialNum = getParameterValue(SERIAL_NUMBER);
	return serialNum;
}

char * getDeviceBootTime()
{
	char *bootTime = NULL;
	bootTime = getParameterValue(DEVICE_BOOT_TIME);
	return bootTime;
}

char * getProductClass()
{
	char *productClass = NULL;
	productClass = getParameterValue(PRODUCT_CLASS);
	return productClass;
}

char * getModelName()
{
	char *modelName = NULL;
	modelName = getParameterValue(MODEL_NAME);
	return modelName;
}

char * getFirmwareVersion()
{
	char *firmware = NULL;
	firmware = getParameterValue(FIRMWARE_VERSION);
	return firmware;
}

char * getConnClientParamName()
{
	return CONN_CLIENT_PARAM;
}
