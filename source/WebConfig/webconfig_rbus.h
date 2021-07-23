/*
 * If not stated otherwise in this file or this component's Licenses.txt file the
 * following copyright and licenses apply:
 *
 * Copyright 2021 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/

#ifndef _WEBCONFIG_RBUS_H_
#define _WEBCONFIG_RBUS_H_

#include <stdio.h>
#include "webpa_adapter.h"
#include <wdmp-c.h>
#include <cimplog.h>

// Data elements provided by webconfig

#define WEBCONFIG_RBUS_PARAM_RFC_ENABLE                  "Device.X_RDK_WebConfig.RfcEnable"
#define WEBCONFIG_RBUS_PARAM_FORCE_SYNC   	         "Device.X_RDK_WebConfig.ForceSync"
#define WEBCONFIG_RBUS_PARAM_URL                         "Device.X_RDK_WebConfig.URL"
#define WEBCONFIG_RBUS_PARAM_DATA   	    	         "Device.X_RDK_WebConfig.Data"
#define WEBCONFIG_RBUS_PARAM_SUPPORTED_DOCS	         "Device.X_RDK_WebConfig.SupportedDocs"
#define WEBCONFIG_RBUS_PARAM_SUPPORTED_VERSION           "Device.X_RDK_WebConfig.SupportedSchemaVersion"
#define WEBCONFIG_RBUS_PARAM_SUPPLEMENTARY_TELEMETRY     "Device.X_RDK_WebConfig.SupplementaryServiceUrls.Telemetry"

WDMP_STATUS regWebConfigDataModel();
bool get_RfcEnable_From_DB(bool *bValue);
int set_Webconfig_Url_Rbus(char * url);
int get_Webconfig_Url_Rbus(char ** url);
#endif
