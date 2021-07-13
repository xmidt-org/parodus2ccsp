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

#include <stdbool.h>
#include <string.h>

#include <stdlib.h>
#include <wdmp-c.h>
#include <webcfg_log.h>

#include "webpa_adapter.h"
#include "webconfig_rbus.h"
#include "webpa_rbus.h"

#define buffLen 1024
#define maxParamLen 128

#define NUM_WEBCONFIG_ELEMENTS 7

static bool  RfcVal = false ;
static char* ForceSyncVal = NULL ;
static char* URLVal = NULL ;
static char* BinDataVal = NULL ;
static char* SupportedDocsVal = NULL ;
static char* SupportedVersionVal = NULL ;
static char* SupplementaryUrlVal = NULL ;

/**
 * Data set handler for Webpa parameters
 */
rbusError_t webConfigDataSetHandler(rbusHandle_t handle, rbusProperty_t prop, rbusSetHandlerOptions_t* opts)
{

    WebcfgInfo("Inside webConfigDataSetHandler\n");
    (void) opts;

    char const* paramName = rbusProperty_GetName(prop);
    if( (strncmp(paramName, WEBCONFIG_RBUS_PARAM_RFC_ENABLE, maxParamLen) != 0) && 
        (strncmp(paramName, WEBCONFIG_RBUS_PARAM_FORCE_SYNC, maxParamLen) != 0) &&
        (strncmp(paramName, WEBCONFIG_RBUS_PARAM_URL, maxParamLen) != 0) &&
        (strncmp(paramName, WEBCONFIG_RBUS_PARAM_DATA, maxParamLen) != 0) &&
        (strncmp(paramName, WEBCONFIG_RBUS_PARAM_SUPPORTED_DOCS, maxParamLen) != 0) &&
        (strncmp(paramName, WEBCONFIG_RBUS_PARAM_SUPPORTED_VERSION, maxParamLen) != 0) &&
        (strncmp(paramName, WEBCONFIG_RBUS_PARAM_SUPPLEMENTARY_TELEMETRY, maxParamLen) != 0) )
    {
        WebcfgError("Unexpected parameter = %s\n", paramName);
        return RBUS_ERROR_ELEMENT_DOES_NOT_EXIST;
    }

    WebcfgInfo("Parameter name is %s \n", paramName);
    rbusValueType_t type_t;
    rbusValue_t paramValue_t = rbusProperty_GetValue(prop);
    if(paramValue_t)
    {
        type_t = rbusValue_GetType(paramValue_t);
    }
    else
    {
	WebcfgError("Invalid input to set\n");
        return RBUS_ERROR_INVALID_INPUT;
    }

    if(strncmp(paramName, WEBCONFIG_RBUS_PARAM_RFC_ENABLE, maxParamLen) == 0)
    {
        WebcfgInfo("Inside Rfc datamodel handler \n");

        if(type_t == RBUS_BOOLEAN)
        {
            bool data = rbusValue_GetBoolean(paramValue_t);
            if(data)
            {
                WebcfgInfo("Call Rfc datamodel function  with data %s \n", (1==data)?"true":"false");
                RfcVal = data;
                WebcfgInfo("RfcVal after processing %s\n", (1==RfcVal)?"true":"false");
            }
        }
        else
        {
            WebcfgError("Unexpected value type for property %s \n", paramName);
        }

    }
    else if(strncmp(paramName, WEBCONFIG_RBUS_PARAM_FORCE_SYNC, maxParamLen) == 0)
    {
        WebcfgInfo("Inside ForceSync datamodel handler \n");

        if(type_t == RBUS_STRING)
        {
            char* data = rbusValue_ToString(paramValue_t, NULL, 0);
            if(data)
            {
                WebcfgInfo("Call ForceSync datamodel function  with data %s \n", data);

                if (ForceSyncVal)
                {
                    free(ForceSyncVal);
                    ForceSyncVal = NULL;
                }
                ForceSyncVal = strdup(data);
                free(data);
                WebcfgInfo("ForceSyncVal after processing %s\n", ForceSyncVal);
            }
        }
        else
        {
            WebcfgError("Unexpected value type for property %s \n", paramName);
        }

    }
    else if(strncmp(paramName, WEBCONFIG_RBUS_PARAM_URL, maxParamLen) == 0)
    {
        WebcfgInfo("Inside URL datamodel handler \n");
	int retPsmSet = WDMP_SUCCESS;

        if(type_t == RBUS_STRING)
        {
            char* data = rbusValue_ToString(paramValue_t, NULL, 0);
            if(data)
            {
                WebcfgInfo("Call URL datamodel function  with data %s \n", data);

                if (URLVal)
                {
                    free(URLVal);
                    URLVal = NULL;
                }
                URLVal = strdup(data);
                free(data);
                WebcfgInfo("URLVal after processing %s\n", URLVal);
            }
        }
        else
        {
            WebcfgError("Unexpected value type for property %s \n", paramName);
        }

    }
    else if(strncmp(paramName, WEBCONFIG_RBUS_PARAM_DATA, maxParamLen) == 0)
    {
        WebcfgInfo("Inside BinData datamodel handler \n");

        if(type_t == RBUS_STRING)
        {
            char* data = rbusValue_ToString(paramValue_t, NULL, 0);
            if(data)
            {
                WebcfgInfo("Call BinData datamodel function  with data %s \n", data);

                if (BinDataVal)
                {
                    free(BinDataVal);
                    BinDataVal = NULL;
                }
                BinDataVal = strdup(data);
                free(data);
                WebcfgInfo("BinDataVal after processing %s\n", BinDataVal);
            }
        }
        else
        {
            WebcfgError("Unexpected value type for property %s \n", paramName);
        }

    }
    else if(strncmp(paramName, WEBCONFIG_RBUS_PARAM_SUPPORTED_DOCS, maxParamLen) == 0)
    {
        WebcfgInfo("Inside SupportedDocs datamodel handler \n");

        if(type_t == RBUS_STRING)
        {
            char* data = rbusValue_ToString(paramValue_t, NULL, 0);
            if(data)
            {
                WebcfgInfo("Call SupportedDocs datamodel function  with data %s \n", data);

                if (SupportedDocsVal)
                {
                    free(SupportedDocsVal);
                    SupportedDocsVal = NULL;
                }
                SupportedDocsVal = strdup(data);
                free(data);
                WebcfgInfo("SupportedDocsVal after processing %s\n", SupportedDocsVal);
            }
        }
        else
        {
            WebcfgError("Unexpected value type for property %s \n", paramName);
        }

    }
    else if(strncmp(paramName, WEBCONFIG_RBUS_PARAM_SUPPORTED_VERSION, maxParamLen) == 0)
    {
        WebcfgInfo("Inside SupportedVersion datamodel handler \n");

        if(type_t == RBUS_STRING)
        {
            char* data = rbusValue_ToString(paramValue_t, NULL, 0);
            if(data)
            {
                WebcfgInfo("Call SupportedVersion datamodel function  with data %s \n", data);

                if (SupportedVersionVal)
                {
                    free(SupportedVersionVal);
                    SupportedVersionVal = NULL;
                }
                SupportedVersionVal = strdup(data);
                free(data);
                WebcfgInfo("SupportedVersionVal after processing %s\n", SupportedVersionVal);
            }
        }
        else
        {
            WebcfgError("Unexpected value type for property %s \n", paramName);
        }

    }
    else if(strncmp(paramName, WEBCONFIG_RBUS_PARAM_SUPPLEMENTARY_TELEMETRY, maxParamLen) == 0)
    {
        WebcfgInfo("Inside SupplementaryUrl datamodel handler \n");

        if(type_t == RBUS_STRING)
        {
            char* data = rbusValue_ToString(paramValue_t, NULL, 0);
            if(data)
            {
                WebcfgInfo("Call SupplementaryUrl datamodel function  with data %s \n", data);

                if (SupplementaryUrlVal)
                {
                    free(SupplementaryUrlVal);
                    SupplementaryUrlVal = NULL;
                }
                SupplementaryUrlVal = strdup(data);
                free(data);
                WebcfgInfo("SupplementaryUrlVal after processing %s\n", SupplementaryUrlVal);
            }
        }
        else
        {
            WebcfgError("Unexpected value type for property %s \n", paramName);
        }

    }
    WebcfgInfo("webConfigDataSetHandler End\n");
    return RBUS_ERROR_SUCCESS;
}

/**
 * Common data get handler for all parameters owned by Webpa
 */
rbusError_t webConfigDataGetHandler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts)
{

    WebcfgInfo("In webConfigDataGetHandler\n");
    (void) handle;
    (void) opts;
    char const* propertyName;
    char* componentName = NULL;

    propertyName = strdup(rbusProperty_GetName(property));
    if(propertyName)
    {
        WebcfgInfo("Property Name is %s \n", propertyName);
    }
    else
    {
        WebcfgError("Unable to handle get request for property \n");
        return RBUS_ERROR_INVALID_INPUT;
    }

    if(strncmp(propertyName, WEBCONFIG_RBUS_PARAM_RFC_ENABLE, maxParamLen) == 0)
    {
        rbusValue_t value;
        rbusValue_Init(&value);

        rbusValue_SetBoolean(value, RfcVal); 

        rbusProperty_SetValue(property, value);
        rbusValue_Release(value);
	WebcfgInfo("Rfc value fetched is %s\n", value);

    }
    else if(strncmp(propertyName, WEBCONFIG_RBUS_PARAM_FORCE_SYNC, maxParamLen) == 0)
    {
        rbusValue_t value;
        rbusValue_Init(&value);
        if(ForceSyncVal)
        {
            rbusValue_SetString(value, ForceSyncVal);
        }
        else
        {
            rbusValue_SetString(value, "");
        }
        rbusProperty_SetValue(property, value);
        rbusValue_Release(value);
	WebcfgInfo("ForceSync value fetched is %s\n", value);

    }
    else if(strncmp(propertyName, WEBCONFIG_RBUS_PARAM_URL, maxParamLen) == 0)
    {
        rbusValue_t value;
        rbusValue_Init(&value);
        if(URLVal)
        {
            rbusValue_SetString(value, URLVal);
        }
        else
        {
            rbusValue_SetString(value, "");
        }
        rbusProperty_SetValue(property, value);
        rbusValue_Release(value);
	WebcfgInfo("URL value fetched is %s\n", value);

    }
    else if(strncmp(propertyName, WEBCONFIG_RBUS_PARAM_DATA, maxParamLen) == 0)
    {
        rbusValue_t value;
        rbusValue_Init(&value);
        if(BinDataVal)
        {
            rbusValue_SetString(value, BinDataVal);
        }
        else
        {
            rbusValue_SetString(value, "");
        }
        rbusProperty_SetValue(property, value);
        rbusValue_Release(value);
	WebcfgInfo("BinData value fetched is %s\n", value);

    }
    else if(strncmp(propertyName, WEBCONFIG_RBUS_PARAM_SUPPORTED_DOCS, maxParamLen) == 0)
    {
        rbusValue_t value;
        rbusValue_Init(&value);
        if(SupportedDocsVal)
        {
            rbusValue_SetString(value, SupportedDocsVal);
        }
        else
        {
            rbusValue_SetString(value, "");
        }
        rbusProperty_SetValue(property, value);
        rbusValue_Release(value);
	WebcfgInfo("SupportedDocs value fetched is %s\n", value);

    }
    else if(strncmp(propertyName, WEBCONFIG_RBUS_PARAM_SUPPORTED_VERSION, maxParamLen) == 0)
    {
        rbusValue_t value;
        rbusValue_Init(&value);
        if(SupportedVersionVal)
        {
            rbusValue_SetString(value, SupportedVersionVal);
        }
        else
        {
            rbusValue_SetString(value, "");
        }
        rbusProperty_SetValue(property, value);
        rbusValue_Release(value);
	WebcfgInfo("SupportedVersion value fetched is %s\n", value);

    }
    else if(strncmp(propertyName, WEBCONFIG_RBUS_PARAM_SUPPLEMENTARY_TELEMETRY, maxParamLen) == 0)
    {
        rbusValue_t value;
        rbusValue_Init(&value);
        if(SupplementaryUrlVal)
        {
            rbusValue_SetString(value, SupplementaryUrlVal);
        }
        else
        {
            rbusValue_SetString(value, "");
        }
        rbusProperty_SetValue(property, value);
        rbusValue_Release(value);
	WebcfgInfo("SupplementaryUrl value fetched is %s\n", value);

    }

    if(propertyName) {
        free((char*)propertyName);
        propertyName = NULL;
    }

    WebcfgInfo("webConfigDataGetHandler End\n");
    return RBUS_ERROR_SUCCESS;
}

/**
 * Register data elements for dataModel implementation using rbus.
 * Data element over bus will be Device.X_RDK_WebConfig.RfcEnable, Device.X_RDK_WebConfig.ForceSync,
 * Device.X_RDK_WebConfig.URL, Device.X_RDK_WebConfig.Data, Device.X_RDK_WebConfig.SupportedDocs,
 * Device.X_RDK_WebConfig.SupplementaryServiceUrls.Telemetry
 */
WDMP_STATUS regWebConfigDataModel()
{
        char deRfc[125] = { '\0' };
        char deForceSync[125] = { '\0' };
        char deURL[125] = { '\0' };
        char deBinData[125] = { '\0' };
        char deSupportedDocs[125] = { '\0' };
        char deSupportedVersion[125] = { '\0' };
        char deSupplementaryURL[125] = { '\0' };
	rbusError_t ret = RBUS_ERROR_SUCCESS;
	WDMP_STATUS status = WDMP_SUCCESS;

	rbusHandle_t webcfg_rbus_handle = get_global_rbus_handle();
	snprintf(deRfc, 124 , "%s", WEBCONFIG_RBUS_PARAM_RFC_ENABLE);
	snprintf(deForceSync, 124 , "%s", WEBCONFIG_RBUS_PARAM_FORCE_SYNC);
	snprintf(deURL, 124 , "%s", WEBCONFIG_RBUS_PARAM_URL);
	snprintf(deBinData, 124 , "%s", WEBCONFIG_RBUS_PARAM_DATA);
	snprintf(deSupportedDocs, 124 , "%s", WEBCONFIG_RBUS_PARAM_SUPPORTED_DOCS);
	snprintf(deSupportedVersion, 124 , "%s", WEBCONFIG_RBUS_PARAM_SUPPORTED_VERSION);
	snprintf(deSupplementaryURL, 124 , "%s", WEBCONFIG_RBUS_PARAM_SUPPLEMENTARY_TELEMETRY);

	WebcfgInfo("Registering parameters deRfc %s\n, deForceSync %s\n, deURL %s\n, deBinData %s\n, deSupportedDocs %s\n, deSupportedVersion %s\n, deSupplementaryURL %s\n", deRfc, deForceSync, deURL, deBinData, deSupportedDocs, deSupportedVersion, deSupplementaryURL);

	if(!webcfg_rbus_handle)
	{
		WebcfgError("regRbusWebConfigDataModel Failed in getting bus handles\n");
		return WDMP_FAILURE;
	}

	rbusDataElement_t dataElements[NUM_WEBCONFIG_ELEMENTS] = {

{deRfc, RBUS_ELEMENT_TYPE_PROPERTY, {webConfigDataGetHandler, webConfigDataSetHandler, NULL, NULL, NULL, NULL}}, 
{deForceSync, RBUS_ELEMENT_TYPE_PROPERTY, {webConfigDataGetHandler, webConfigDataSetHandler, NULL, NULL, NULL, NULL}}, {deURL, RBUS_ELEMENT_TYPE_PROPERTY, {webConfigDataGetHandler, webConfigDataSetHandler, NULL, NULL, NULL, NULL}}, {deBinData, RBUS_ELEMENT_TYPE_PROPERTY, {webConfigDataGetHandler, webConfigDataSetHandler, NULL, NULL, NULL, NULL}}, 
{deSupportedDocs, RBUS_ELEMENT_TYPE_PROPERTY, {webConfigDataGetHandler, webConfigDataSetHandler, NULL, NULL, NULL, NULL}}, 
{deSupportedVersion, RBUS_ELEMENT_TYPE_PROPERTY, {webConfigDataGetHandler, webConfigDataSetHandler, NULL, NULL, NULL, NULL}}, 
{deSupplementaryURL, RBUS_ELEMENT_TYPE_PROPERTY, {webConfigDataGetHandler, webConfigDataSetHandler, NULL, NULL, NULL, NULL}}

	};
	ret = rbus_regDataElements(webcfg_rbus_handle, NUM_WEBCONFIG_ELEMENTS, dataElements);
	if(ret == RBUS_ERROR_SUCCESS)
	{
		WebcfgInfo("Registered data element with rbus\n");
	}
	else
	{
		WebcfgError("Failed in registering data element\n");
		status = WDMP_FAILURE;
	}

	WebcfgInfo("rbus reg status returned is %d\n", status);
	return status;
}
