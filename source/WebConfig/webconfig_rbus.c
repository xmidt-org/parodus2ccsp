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
#include <webcfg_db.h>
#include <webcfg_metadata.h>
#include <webcfg_generic.h>

#include "webpa_adapter.h"
#include "webconfig_rbus.h"
#include "webpa_rbus.h"
#include "cosa_webconfig_internal.h"

#define buffLen 1024
#define maxParamLen 128

#define NUM_WEBCONFIG_ELEMENTS 7
#define RBUS_PATH_PSM         "/com/cisco/spvtg/ccsp/PSM"
#define RBUS_PSM              "com.cisco.spvtg.ccsp.psm"
#define RBUS_UNREFERENCED_PARAMETER(_p_)         (void)(_p_)

static bool  RfcVal = false ;
static char* ForceSyncVal = NULL ;
static char* URLVal = NULL ;
static char* BinDataVal = NULL ;
static char* SupportedDocsVal = NULL ;
static char* SupportedVersionVal = NULL ;
static char* SupplementaryUrlVal = NULL ;

int  timeout_seconds        = 60; //seconds
int  timeout_getval_seconds = 120; //seconds
#define  TIMEOUT_RBUS  (timeout_seconds * 1000) // in milliseconds
#define  TIMEOUT_GETVAL_RBUS  (timeout_getval_seconds * 1000) // in milliseconds

typedef struct
{
    char *parameterName;
    char *parameterValue;
    rbusValueType_t type;
} rbusParameterValStruct_t;

typedef unsigned int rbus_bool ;

rbusError_t rbus_psm_get(rbusHandle_t rbus_handle, char const * const pSubSystemPrefix, char const * const pRecordName, unsigned int * ulRecordType, char** pVal);

rbusError_t rbus_psm_set(rbusHandle_t bus_handle, char const * const pSubSystemPrefix, char const * const pRecordName, unsigned int const ulRecordType, char const * const pVal);

rbusError_t rbus_GetParamValues(rbusHandle_t rbus_handle, const char* dst_component_id, char* rbus_path, char* parameterNames[], int param_size, int *val_size, rbusParameterValStruct_t ***parameterVal);

rbusError_t rbus_SetParamValues(rbusHandle_t rbus_handle, const char* dst_component_id, char* rbus_path, int sessionId, char* writeID_str, rbusParameterValStruct_t *val, int size, rbus_bool commit, char ** invalidParameterName);

void free_rbusParameterValStruct_t(int size,rbusParameterValStruct_t **val);

int set_Webconfig_Url_Rbus(char * url)
{
    rbusError_t retPsmSet = RBUS_ERROR_SUCCESS;
    rbusHandle_t rbus_handle = get_global_rbus_handle();

    if(rbus_handle ==  NULL)
    {
        WebcfgError("Failed in getting rbus_handle in %s\n", __FUNCTION__);
        return 0;
    }

    retPsmSet = rbus_psm_set(rbus_handle, "eRT.", WEBCONFIG_PARAM_URL, 0, url );

    if(retPsmSet != RBUS_ERROR_SUCCESS)
    {
        WebcfgError("psm_set failed ret %d for parameter %s and value %s\n", retPsmSet, WEBCONFIG_PARAM_URL, url);
        return 0;
    }
    else
    {
        WebcfgDebug("psm_set success ret %d for parameter %s and value %s\n", retPsmSet, WEBCONFIG_PARAM_URL, url);
    }

    return 1;
}

int get_Webconfig_Url_Rbus(char ** url)
{
    rbusError_t retPsmGet = RBUS_ERROR_SUCCESS;
    rbusHandle_t rbus_handle = get_global_rbus_handle();

    if(rbus_handle ==  NULL)
    {
        WebcfgError("Failed in getting rbus_handle in %s\n", __FUNCTION__);
        return 0;
    }

    if(URLVal != NULL && strlen(URLVal) > 0)
    {
        *url = malloc(strlen(URLVal)+1);
        strcpy(*url, URLVal);
        WebcfgInfo("urlvalue is %s\n", *url);
    }
    else
    {
        char *tempUrl = NULL;
        rbusError_t retPsmGet = rbus_psm_get(rbus_handle, "eRT.", WEBCONFIG_PARAM_URL, NULL, &tempUrl);
        WebcfgInfo("tempUrl is %s\n", tempUrl);

        if (retPsmGet == RBUS_ERROR_SUCCESS)
        {
            WebcfgDebug("retPsmGet success\n");
            if(tempUrl != NULL && strlen(tempUrl) > 0)
            {
	        *url = malloc(strlen(tempUrl)+1);
	        strcpy(*url, tempUrl);
	        WebcfgInfo("url %s\n",*url);
            }
            else
            {
                WebcfgError("tempUrl value Fetched is empty\n");
                return 0;
            }
        }
        else
        {
            WebcfgError("psm_get failed ret %d for parameter %s\n", retPsmGet, WEBCONFIG_PARAM_URL);
            return 0;
        }
    }

    return 1;
}

/**
 * Data set handler for WebConfig parameters
 */
rbusError_t webConfigDataSetHandler(rbusHandle_t handle, rbusProperty_t prop, rbusSetHandlerOptions_t* opts)
{

    rbusError_t retStatus = RBUS_ERROR_SUCCESS;
    WebcfgInfo("Inside webConfigDataSetHandler\n");
    (void) opts;

    char const* paramName = rbusProperty_GetName(prop);
    if( (strncmp(paramName, WEBCONFIG_PARAM_RFC_ENABLE, maxParamLen) != 0) &&
        (strncmp(paramName, WEBCONFIG_PARAM_FORCE_SYNC, maxParamLen) != 0) &&
        (strncmp(paramName, WEBCONFIG_PARAM_URL, maxParamLen) != 0) &&
        (strncmp(paramName, WEBCONFIG_PARAM_DATA, maxParamLen) != 0) &&
        (strncmp(paramName, WEBCONFIG_PARAM_SUPPORTED_DOCS, maxParamLen) != 0) &&
        (strncmp(paramName, WEBCONFIG_PARAM_SUPPORTED_VERSION, maxParamLen) != 0) &&
        (strncmp(paramName, WEBCONFIG_PARAM_SUPPLEMENTARY_TELEMETRY, maxParamLen) != 0) )
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

    if(strncmp(paramName, WEBCONFIG_PARAM_RFC_ENABLE, maxParamLen) == 0)
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
            else
            {
                WebcfgError("set value type is invalid\n");
                retStatus = RBUS_ERROR_INVALID_INPUT;
            }
        }
        else
        {
            WebcfgError("Unexpected value type for property %s \n", paramName);
            retStatus = RBUS_ERROR_INVALID_INPUT;
        }

    }
    else if(strncmp(paramName, WEBCONFIG_PARAM_FORCE_SYNC, maxParamLen) == 0)
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
            else
            {
                WebcfgError("set value type is invalid\n");
                retStatus = RBUS_ERROR_INVALID_INPUT;
            }
        }
        else
        {
            WebcfgError("Unexpected value type for property %s \n", paramName);
            retStatus = RBUS_ERROR_INVALID_INPUT;
        }

    }
    else if(strncmp(paramName, WEBCONFIG_PARAM_URL, maxParamLen) == 0)
    {
        WebcfgInfo("Inside URL datamodel handler \n");
	int retPsmSet = 0;

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

		retPsmSet = set_Webconfig_Url_Rbus(URLVal);

		if(!retPsmSet)
		{
			WebcfgError("psm set failed\n");
			retStatus = RBUS_ERROR_BUS_ERROR;
		}
		else
		{
			WebcfgInfo("URLVal after processing %s\n", URLVal);
		}
            }
            else
            {
                WebcfgError("set value type is invalid\n");
                retStatus = RBUS_ERROR_INVALID_INPUT;
            }
        }
        else
        {
            WebcfgError("Unexpected value type for property %s \n", paramName);
            retStatus = RBUS_ERROR_INVALID_INPUT;
        }

    }
    else if(strncmp(paramName, WEBCONFIG_PARAM_DATA, maxParamLen) == 0)
    {
        WebcfgError("Data Set is not allowed\n");
        retStatus = RBUS_ERROR_ACCESS_NOT_ALLOWED;

    }
    else if(strncmp(paramName, WEBCONFIG_PARAM_SUPPORTED_DOCS, maxParamLen) == 0)
    {
        WebcfgError("SupportedDocs Set is not allowed\n");
        retStatus = RBUS_ERROR_ACCESS_NOT_ALLOWED;
    }
    else if(strncmp(paramName, WEBCONFIG_PARAM_SUPPORTED_VERSION, maxParamLen) == 0)
    {
        WebcfgError("SupportedSchemaVersion Set is not allowed\n");
        retStatus = RBUS_ERROR_ACCESS_NOT_ALLOWED;
    }
    else if(strncmp(paramName, WEBCONFIG_PARAM_SUPPLEMENTARY_TELEMETRY, maxParamLen) == 0)
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
            else
            {
                WebcfgError("set value type is invalid\n");
                retStatus = RBUS_ERROR_INVALID_INPUT;
            }
        }
        else
        {
            WebcfgError("Unexpected value type for property %s \n", paramName);
            retStatus = RBUS_ERROR_INVALID_INPUT;
        }

    }
    WebcfgInfo("webConfigDataSetHandler End\n");
    return retStatus;
}

/**
 * Common data get handler for all parameters owned by WebConfig
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

    if(strncmp(propertyName, WEBCONFIG_PARAM_RFC_ENABLE, maxParamLen) == 0)
    {
        rbusValue_t value;
        rbusValue_Init(&value);

        rbusValue_SetBoolean(value, RfcVal); 

        rbusProperty_SetValue(property, value);
        WebcfgInfo("Rfc value fetched is %s\n", value);
        rbusValue_Release(value);

    }
    else if(strncmp(propertyName, WEBCONFIG_PARAM_FORCE_SYNC, maxParamLen) == 0)
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
        WebcfgInfo("ForceSync value fetched is %s\n", value);
        rbusValue_Release(value);

    }
    else if(strncmp(propertyName, WEBCONFIG_PARAM_URL, maxParamLen) == 0)
    {
        rbusValue_t value;
        rbusValue_Init(&value);
        if(URLVal)
        {
            rbusValue_SetString(value, URLVal);
        }
        else
        {
            char * localUrl = NULL;
            if(get_Webconfig_Url_Rbus(&localUrl))
            {
                rbusValue_SetString(value, localUrl);
            }
            else
            {
                rbusValue_SetString(value, "");
            }
        }
        rbusProperty_SetValue(property, value);
        WebcfgInfo("URL value fetched is %s\n", rbusValue_GetString(value, NULL));
        rbusValue_Release(value);

    }
    else if(strncmp(propertyName, WEBCONFIG_PARAM_DATA, maxParamLen) == 0)
    {
        rbusValue_t value;
        rbusValue_Init(&value);

        BinDataVal = get_DB_BLOB_base64();

        if(BinDataVal)
        {
            rbusValue_SetString(value, BinDataVal);
        }
        else
        {
            rbusValue_SetString(value, "");
        }
        rbusProperty_SetValue(property, value);
        WebcfgInfo("BinData value fetched is %s\n", value);
        rbusValue_Release(value);

    }
    else if(strncmp(propertyName, WEBCONFIG_PARAM_SUPPORTED_DOCS, maxParamLen) == 0)
    {
        rbusValue_t value;
        rbusValue_Init(&value);

        SupportedDocsVal = getsupportedDocs();

        if(SupportedDocsVal)
        {
            rbusValue_SetString(value, SupportedDocsVal);
        }
        else
        {
            rbusValue_SetString(value, "");
        }
        rbusProperty_SetValue(property, value);
        WebcfgInfo("SupportedDocs value fetched is %s\n", value);
        rbusValue_Release(value);

    }
    else if(strncmp(propertyName, WEBCONFIG_PARAM_SUPPORTED_VERSION, maxParamLen) == 0)
    {
        rbusValue_t value;
        rbusValue_Init(&value);

        SupportedVersionVal = getsupportedVersion();

        if(SupportedVersionVal)
        {
            rbusValue_SetString(value, SupportedVersionVal);
        }
        else
        {
            rbusValue_SetString(value, "");
        }
        rbusProperty_SetValue(property, value);
        WebcfgInfo("SupportedVersion value fetched is %s\n", value);
        rbusValue_Release(value);

    }
    else if(strncmp(propertyName, WEBCONFIG_PARAM_SUPPLEMENTARY_TELEMETRY, maxParamLen) == 0)
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
        WebcfgInfo("SupplementaryUrl value fetched is %s\n", value);
        rbusValue_Release(value);
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
	rbusError_t ret = RBUS_ERROR_SUCCESS;
	WDMP_STATUS status = WDMP_SUCCESS;

	rbusHandle_t webcfg_rbus_handle = get_global_rbus_handle();

	WebcfgInfo("Registering parameters datamodel\n");

	if(!webcfg_rbus_handle)
	{
		WebcfgError("regRbusWebConfigDataModel Failed in getting bus handle\n");
		return WDMP_FAILURE;
	}

	rbusDataElement_t dataElements[NUM_WEBCONFIG_ELEMENTS] = {

{WEBCONFIG_PARAM_RFC_ENABLE, RBUS_ELEMENT_TYPE_PROPERTY, {webConfigDataGetHandler, webConfigDataSetHandler, NULL, NULL, NULL, NULL}},
{WEBCONFIG_PARAM_FORCE_SYNC, RBUS_ELEMENT_TYPE_PROPERTY, {webConfigDataGetHandler, webConfigDataSetHandler, NULL, NULL, NULL, NULL}},
{WEBCONFIG_PARAM_URL, RBUS_ELEMENT_TYPE_PROPERTY, {webConfigDataGetHandler, webConfigDataSetHandler, NULL, NULL, NULL, NULL}},
{WEBCONFIG_PARAM_DATA, RBUS_ELEMENT_TYPE_PROPERTY, {webConfigDataGetHandler, webConfigDataSetHandler, NULL, NULL, NULL, NULL}},
{WEBCONFIG_PARAM_SUPPORTED_DOCS, RBUS_ELEMENT_TYPE_PROPERTY, {webConfigDataGetHandler, webConfigDataSetHandler, NULL, NULL, NULL, NULL}},
{WEBCONFIG_PARAM_SUPPORTED_VERSION, RBUS_ELEMENT_TYPE_PROPERTY, {webConfigDataGetHandler, webConfigDataSetHandler, NULL, NULL, NULL, NULL}},
{WEBCONFIG_PARAM_SUPPLEMENTARY_TELEMETRY, RBUS_ELEMENT_TYPE_PROPERTY, {webConfigDataGetHandler, webConfigDataSetHandler, NULL, NULL, NULL, NULL}}

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

rbusError_t rbus_psm_get(rbusHandle_t rbus_handle, char const * const pSubSystemPrefix, char const * const pRecordName, unsigned int * ulRecordType, char** pVal)
{
	char * parameterNames[1] = {NULL};
	char psmName[256] = {0};
	int size = 0;
	rbusParameterValStruct_t **val = 0;
	rbusError_t ret = RBUS_ERROR_SUCCESS;

	*pVal = NULL;

	if ( pSubSystemPrefix && pSubSystemPrefix[0] != 0 )
	{
		sprintf(psmName, "%s%s", pSubSystemPrefix, RBUS_PSM);
	}
	else
	{
		strcpy(psmName, RBUS_PSM);
	}

	parameterNames[0] = (char *)pRecordName;

	ret = rbus_GetParamValues(rbus_handle, psmName, RBUS_PATH_PSM, parameterNames, 1, &size, &val);

	if(ret == RBUS_ERROR_SUCCESS)
	{
		WebcfgInfo("Inside success case\n");
		if(val && val[0] && size)
		{
			WebcfgInfo("Inisde val check condition\n");
			/*if(ulRecordType != NULL)
			//{
				WebcfgInfo("Inside the ulRecordType\n");
				*ulRecordType = val[0]->type;
			}*/

			*pVal = (char *)malloc(strlen(val[0]->parameterValue)+1);
			strcpy(*pVal,  val[0]->parameterValue);
		}
		else
		{
			ret = RBUS_ERROR_INVALID_INPUT;
		}
	}

	free_rbusParameterValStruct_t(size, val);

	return ret;
}


rbusError_t rbus_psm_set(rbusHandle_t rbus_handle, char const * const pSubSystemPrefix, char const * const pRecordName, unsigned int const ulRecordType, char const * const pVal)
{
	rbusParameterValStruct_t val[1];
	char *str = NULL;
	char psmName[256];
	rbusError_t ret;

	val[0].parameterName  = (char *)pRecordName;
	val[0].type = ulRecordType;

	if ( pSubSystemPrefix && pSubSystemPrefix[0] != 0 )
	{
		sprintf(psmName, "%s%s", pSubSystemPrefix, RBUS_PSM);
	}
	else
	{
		strcpy(psmName, RBUS_PSM);
	}

	val[0].parameterValue = (char *)pVal;

	ret = rbus_SetParamValues(rbus_handle, psmName, RBUS_PATH_PSM, 0, "writeid_cli", val, 1, 1, &str);

	if(str)
	{
		free(str);
	}

	return ret;
}

rbusError_t rbus_GetParamValues(rbusHandle_t rbus_handle, const char* dst_component_id, char* rbus_path, char* parameterNames[], int param_size, int *val_size, rbusParameterValStruct_t ***parameterVal)
{
	RBUS_UNREFERENCED_PARAMETER(rbus_path);
	rbusParameterValStruct_t **val = 0;
	*val_size = 0;
	rbusError_t ret = RBUS_ERROR_BUS_ERROR;
	int i = 0;
	int param_len = 0;
	int32_t type = 0;

	rbusMessage request, response;

	/* There is a case which we have seen in RDKB-29328, where set is called with Size as 0.
	 * No action to be taken for that..
	 */
	if (0 == param_size)
	{
		WebcfgInfo("component calls GET without the dml element name. Returning success as there no action taken\n");
		ret = RBUS_ERROR_SUCCESS;
		*val_size = 0;
		return ret;
	}

	rbusMessage_Init(&request);
	rbusMessage_SetString(request, dst_component_id);
	rbusMessage_SetInt32(request, (int32_t)param_size);

	for(i = 0; i < param_size; i++)
	{
		rbusMessage_SetString(request, parameterNames[i]);
	}

	/* If the param_size is 0, parameterNames is NULL; We avoided it in the above if condition per RDKB-29328 */

	param_len = strlen(parameterNames[0]);
	const char *object_name = parameterNames[0];

	if(dst_component_id)
	{
		if((parameterNames[0][param_len - 1] == '.') || strstr(dst_component_id, ".psm"))
		{
			object_name = dst_component_id;
		}
	}

	WebcfgInfo("%s Calling rbus_invokeRemoteMethod for %s\n", __FUNCTION__, object_name);
	if((ret = rbus_invokeRemoteMethod(object_name, METHOD_GETPARAMETERVALUES, request, TIMEOUT_GETVAL_RBUS, &response)) != RTMESSAGE_BUS_SUCCESS)
	{
		WebcfgError("%s rbus_invokeRemoteMethod: for param[0]=%s failed with Err: %d\n", __FUNCTION__, parameterNames[0], ret);
		return ret;
	}

	rbusMessage_GetInt32(response, &ret);
	WebcfgInfo("The invoke method output is %d\n", ret);
	if(ret == 100)
	{
		rbusMessage_GetInt32(response, val_size);
		WebcfgInfo("No. of output params: %d\n", *val_size);
		if(*val_size)
		{
			val = malloc(*val_size*sizeof(rbusParameterValStruct_t *));
			memset(val, 0, *val_size*sizeof(rbusParameterValStruct_t *));
			const char *tmpbuf = NULL;

			for(i = 0; i < *val_size; i++)
			{
				val[i] = malloc(sizeof(parameterValStruct_t));
				memset(val[i], 0, sizeof(parameterValStruct_t));

				/* Get Name */
				tmpbuf = NULL;
				rbusMessage_GetString(response, &tmpbuf);
				val[i]->parameterName = malloc(strlen(tmpbuf)+1);
				strcpy(val[i]->parameterName, tmpbuf);

				/* Get Type */
				rbusMessage_GetInt32(response, &type);

				/* Update the Type */
				val[i]->type = type;

				/* Get Value */
				tmpbuf = NULL;
				rbusMessage_GetString(response, &tmpbuf);
				val[i]->parameterValue = malloc(strlen(tmpbuf)+1);
				strcpy(val[i]->parameterValue, tmpbuf);

				WebcfgInfo("Param [%d] Name = %s, Type = %d, Value = %s\n", i,val[i]->parameterName, val[i]->type, val[i]->parameterValue);
			}
		}
	}

	rbusMessage_Release(response);
	*parameterVal = val;
	return RBUS_ERROR_SUCCESS;
}

rbusError_t rbus_SetParamValues(rbusHandle_t rbus_handle, const char* dst_component_id, char* rbus_path, int sessionId, char* writeID_str, rbusParameterValStruct_t *val, int size, rbus_bool commit, char ** invalidParameterName)
{
	RBUS_UNREFERENCED_PARAMETER(rbus_path);
	int i = 0;
	rbusError_t ret = RBUS_ERROR_BUS_ERROR;

	if (*invalidParameterName)
	{
		*invalidParameterName = NULL; // initialize
	}

	rbusMessage request, response;

	/* There is a case which we have seen in RDKB-29328, where set is called with Size as 0.
	 * No action to be taken for that..
	 */
	if (0 == size)
	{
		//WebcfgInfo(("%s component calls SET without the dml element name. Returning success as there no action taken\n", rbus_handlecomponentName));
		*invalidParameterName = 0;
		ret = RBUS_ERROR_SUCCESS;
		return ret;
	}

	rbusMessage_Init(&request);
	rbusMessage_SetInt32(request, sessionId);
	rbusMessage_SetString(request, writeID_str);
	rbusMessage_SetInt32(request, size);

	for(i = 0; i < size; i++)
	{
		rbusMessage_SetString(request, val[i].parameterName);
		rbusMessage_SetInt32(request, val[i].type);
		rbusMessage_SetString(request, val[i].parameterValue);
	}
	rbusMessage_SetString(request, commit ? "TRUE" : "FALSE");

	/* If the size is 0, val itself is NULL; val[0].parameterName is NULL pointer dereferencing. We avoided it in the above if condition per RDKB-29328 */
	const char *object_name = val[0].parameterName;

	if(dst_component_id && (strstr(dst_component_id, ".psm")))
	{
		object_name = dst_component_id;
	}

	WebcfgInfo("%s Calling rbus_invokeRemoteMethod for param on %s\n", __FUNCTION__, object_name);
	if((ret = rbus_invokeRemoteMethod(object_name, METHOD_SETPARAMETERVALUES, request, TIMEOUT_RBUS, &response)) != RTMESSAGE_BUS_SUCCESS)
	{
		WebcfgError("%s rbus_invokeRemoteMethod: for param[0]=%s failed with Err: %d\n", __FUNCTION__, val[0].parameterName, ret);
		return ret;
	}

	rbusMessage_GetInt32(response, &ret);
        WebcfgInfo("The ret value before return is %d\n", ret);
	if(ret == RBUS_ERROR_SUCCESS)
	{
		const char *str = NULL;
		rbusMessage_GetString(response, &str); //invalid param
		if(str)
		{
			*invalidParameterName = malloc(strlen(str)+1);
			strcpy(*invalidParameterName, str);
		}
		else
		{
			*invalidParameterName = 0;
		}

	}

	rbusMessage_Release(response);
	return RBUS_ERROR_SUCCESS;

}

void free_rbusParameterValStruct_t (int size,rbusParameterValStruct_t **val)
{
    int i;

    if(val)
    {
        if(size)
        {
            for(i = 0; i < size; i++)
            {
                if(val[i])
                {
                    if(val[i]->parameterName)
                    {
                        free(val[i]->parameterName);
                    }

                    if(val[i]->parameterValue)
                    {
                        free(val[i]->parameterValue);
                    }

                    free(val[i]);
                }
            }
        }

        free(val);
    }
}
