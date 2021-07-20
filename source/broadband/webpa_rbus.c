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

#include <rbus/rbus.h>
#include <rbus/rbus_object.h>
#include <rbus/rbus_property.h>
#include <rbus/rbus_value.h>
#include <rbus-core/rbus_core.h>
#include <stdlib.h>
#include <wdmp-c.h>
#include <cimplog.h>
#include "webpa_adapter.h"
#include "webpa_rbus.h"
#include "webpa_internal.h"
#include "webpa_notification.h"

static rbusHandle_t rbus_handle;

static uint32_t  CMCVal = 0 ;
static char* CIDVal = NULL ;
static char* syncVersionVal = NULL ;

static bool isRbus = false ;

void (*rbusnotifyCbFnPtr)(NotifyData*) = NULL;

typedef struct
{
    char *paramName;
    char *paramValue;
    rbusValueType_t type;
} rbusParamVal_t;

static char *PSMPrefix  = "eRT.com.cisco.spvtg.ccsp.webpa.";

bool get_global_isRbus(void)
{
    return isRbus;
}

bool isRbusEnabled() 
{
	if(RBUS_ENABLED == rbus_checkStatus()) 
	{
		isRbus = true;
	}
	else
	{
		isRbus = false;
	}
	WalInfo("Webpa RBUS mode active status = %s\n", isRbus ? "true":"false");
	return isRbus;
}

bool isRbusInitialized( ) 
{
    return rbus_handle != NULL ? true : false;
}

WDMP_STATUS webpaRbusInit(const char *pComponentName) 
{
	int ret = RBUS_ERROR_SUCCESS;   

	WalInfo("rbus_open for component %s\n", pComponentName);
	ret = rbus_open(&rbus_handle, pComponentName);
	if(ret != RBUS_ERROR_SUCCESS)
	{
		WalError("webpaRbusInit failed with error code %d\n", ret);
		return WDMP_FAILURE;
	}
	WalInfo("webpaRbusInit is success. ret is %d\n", ret);
	return WDMP_SUCCESS;
}

static void webpaRbus_Uninit()
{
    rbus_close(rbus_handle);
}

/**
 * Data set handler for Webpa parameters
 */
rbusError_t webpaDataSetHandler(rbusHandle_t handle, rbusProperty_t prop, rbusSetHandlerOptions_t* opts) {

    WalInfo("Inside webpaDataSetHandler\n");

    char const* paramName = rbusProperty_GetName(prop);
    if((strncmp(paramName,  WEBPA_CMC_PARAM, maxParamLen) != 0) &&
	(strncmp(paramName, WEBPA_CID_PARAM, maxParamLen) != 0) &&
	(strncmp(paramName, WEBPA_SYNCVERSION_PARAM, maxParamLen) != 0) &&
	(strncmp(paramName, WEBPA_CONNECTED_CLIENT_PARAM, maxParamLen) != 0) &&
	(strncmp(paramName, WEBPA_NOTIFY_PARAM, maxParamLen) != 0) &&
	(strncmp(paramName, WEBPA_VERSION_PARAM, maxParamLen) != 0)
	) {
        WalError("Unexpected parameter = %s\n", paramName); //free paramName req.?
        return RBUS_ERROR_ELEMENT_DOES_NOT_EXIST;
    }

    #ifdef USE_NOTIFY_COMPONENT
	char* p_write_id;
	char* p_new_val;
	char* p_old_val;
	char* p_notify_param_name;
	char* st;
	char* p_interface_name = NULL;
	char* p_mac_id = NULL;
	char* p_status = NULL;
	char* p_hostname = NULL;
	char* p_val_type;
	unsigned int value_type,write_id;
	//TODO:parameterSigStruct_t param = {0};
   #endif

    rbusError_t retPsmSet = RBUS_ERROR_BUS_ERROR;
    WalInfo("Parameter name is %s \n", paramName);
    rbusValueType_t type_t;
    rbusValue_t paramValue_t = rbusProperty_GetValue(prop);
    if(paramValue_t) {
        type_t = rbusValue_GetType(paramValue_t);
    } else {
	WalError("Invalid input to set\n");
        return RBUS_ERROR_INVALID_INPUT;
    }

    if(strncmp(paramName, WEBPA_CMC_PARAM, maxParamLen) == 0) {
        WalInfo("Inside CMC datamodel handler \n");
        if(type_t == RBUS_UINT32) {
            CMCVal = rbusValue_GetUInt32(paramValue_t);
	    WalInfo("CMCVal after processing\n");
        } else {
            WalError("Unexpected value type for property %s \n", paramName);
	    return RBUS_ERROR_INVALID_INPUT;
        }

    }else if(strncmp(paramName, WEBPA_CID_PARAM, maxParamLen) == 0) {
        WalInfo("Inside datamodel handler for CID \n");

        if(type_t == RBUS_STRING) {
            char* data = rbusValue_ToString(paramValue_t, NULL, 0);
            if(data) {
                WalInfo("Call datamodel function  with data %s \n", data);

                if(CIDVal) {
                    free(CIDVal);
                    CIDVal = NULL;
                }
                CIDVal = strdup(data);
                free(data);
		WalInfo("CIDVal after processing %s\n", CIDVal);
		WalInfo("CIDVal bus_StoreValueIntoDB\n");
		retPsmSet = rbus_StoreValueIntoDB( "X_COMCAST-COM_CID", CIDVal );
		if (retPsmSet != RBUS_ERROR_SUCCESS)
		{
			WalError("psm_set failed ret %d for parameter %s and value %s\n", retPsmSet, paramName, CIDVal);
			return retPsmSet;
		}
		else
		{
			WalInfo("psm_set success ret %d for parameter %s and value %s\n", retPsmSet, paramName, CIDVal);
		}
            }
        } else {
            WalError("Unexpected value type for property %s\n", paramName);
	    return RBUS_ERROR_INVALID_INPUT;
        }
    }else if(strncmp(paramName, WEBPA_SYNCVERSION_PARAM, maxParamLen) == 0) {
        WalInfo("Inside SYNC VERSION datamodel handler \n");
        if(type_t == RBUS_STRING) {
            char* data = rbusValue_ToString(paramValue_t, NULL, 0);
            if(data) {
                WalInfo("Call datamodel function  with data %s \n", data);

                if (syncVersionVal){
                    free(syncVersionVal);
                    syncVersionVal = NULL;
                }
                syncVersionVal = strdup(data);
                free(data);
		WalInfo("syncVersionVal after processing %s\n", syncVersionVal);
            }
        } else {
            WalError("Unexpected value type for property %s \n", paramName);
            return RBUS_ERROR_INVALID_INPUT;
        }

    }else if(strncmp(paramName, WEBPA_CONNECTED_CLIENT_PARAM, maxParamLen) == 0) {
        WalInfo("Inside datamodel handler for CONNECTED CLIENT \n");

        if(type_t == RBUS_STRING)
	{
		if(opts !=NULL)
		{
			WalInfo("opts.requestingComponent is %s\n", opts->requestingComponent);
		}

		if((opts != NULL) && (strncmp(opts->requestingComponent, COMPONENT_ID_NOTIFY_COMP, strlen(COMPONENT_ID_NOTIFY_COMP)) == 0))
		{
			#ifdef USE_NOTIFY_COMPONENT
			char * ConnClientVal = NULL;
			char* data = rbusValue_ToString(paramValue_t, NULL, 0);
			if(data)
			{
				WalInfo("Call datamodel function  with data %s \n", data);
				ConnClientVal = strdup(data);
				free(data);
				WalInfo("ConnClientVal is %s\n", ConnClientVal);
			}
			WalInfo("...Connected client notification rbus..\n");
			WalInfo(" \n WebPA : Connected-Client Received \n");
			p_notify_param_name = strtok_r(ConnClientVal, ",", &st);
			WalInfo("ConnClientVal value for X_RDKCENTRAL-COM_Connected-Client:%s\n", ConnClientVal);

			p_interface_name = strtok_r(NULL, ",", &st);
			p_mac_id = strtok_r(NULL, ",", &st);
			p_status = strtok_r(NULL, ",", &st);
			p_hostname = strtok_r(NULL, ",", &st);

			if(p_hostname !=NULL && p_notify_param_name !=NULL && p_interface_name !=NULL && p_mac_id !=NULL && p_status !=NULL)
			{
				if(validate_conn_client_notify_data(p_notify_param_name,p_interface_name,p_mac_id,p_status,p_hostname) == WDMP_SUCCESS)
				{
					WalInfo(" \n Notification : Parameter Name = %s \n", p_notify_param_name);
					WalInfo(" \n Notification : Interface = %s \n", p_interface_name);
					WalInfo(" \n Notification : MAC = %s \n", p_mac_id);
					WalInfo(" \n Notification : Status = %s \n", p_status);
					WalInfo(" \n Notification : HostName = %s \n", p_hostname);

					rbusnotifyCbFnPtr = getNotifyCB();

					if (NULL == rbusnotifyCbFnPtr)
					{
						WalError("Fatal: rbusnotifyCbFnPtr is NULL\n");
						return RBUS_ERROR_BUS_ERROR;
					}
					else
					{
						// Data received from stack is not sent upstream to server for Connected Client
						WalInfo("B4 sendConnectedClientNotification\n");
						sendConnectedClientNotification(p_mac_id, p_status, p_interface_name, p_hostname);
						WalInfo("After sendConnectedClientNotification\n");
					}
				}
				else
				{
					WalError("Received incorrect data for connected client notification\n");
				}
			}
			else
			{
				WalError("Received insufficient data to process connected client notification\n");
			}

		#endif
		}
		else
		{
			WalError("Operation not allowed\n");
			return RBUS_ERROR_INVALID_OPERATION;
		}
        } else {
            WalError("Unexpected value type for property %s\n", paramName);
	    return RBUS_ERROR_INVALID_INPUT;
        }
    }else if(strncmp(paramName, WEBPA_NOTIFY_PARAM, maxParamLen) == 0) {
        WalInfo("Inside datamodel handler for NOTIFY PARAM \n");

        if(type_t == RBUS_STRING) {
		if(opts !=NULL)
		{
			WalInfo("opts.requestingComponent is %s\n", opts->requestingComponent);
		}

		if((opts != NULL) && (strncmp(opts->requestingComponent, COMPONENT_ID_NOTIFY_COMP, strlen(COMPONENT_ID_NOTIFY_COMP)) == 0))
		{
			char *notifyVal = NULL;
			char* data = rbusValue_ToString(paramValue_t, NULL, 0);
			if(data)
			{
				WalInfo("Call datamodel function  with data %s \n", data);
				notifyVal = strdup(data);
				WAL_FREE(data);
				WalInfo("notifyVal is %s\n", notifyVal);
			}

			#ifdef USE_NOTIFY_COMPONENT

		        WalInfo(" \n WebPA : Notification Received \n");
		        char *tmpStr, *notifyStr;
		        tmpStr = notifyStr = strdup(notifyVal);

		        p_notify_param_name = strsep(&notifyStr, ",");
		        p_write_id = strsep(&notifyStr,",");
		        p_new_val = strsep(&notifyStr,",");
		        p_old_val = strsep(&notifyStr,",");
		        p_val_type = strsep(&notifyStr, ",");

		        if(p_notify_param_name != NULL && p_val_type !=NULL && p_write_id !=NULL)
		        {
				if(validate_webpa_notification_data(p_notify_param_name, p_write_id) == WDMP_SUCCESS)
				{
				        value_type = atoi(p_val_type);
				        write_id = atoi(p_write_id);

				        WalInfo(" \n Notification : Parameter Name = %s \n", p_notify_param_name);
				        WalInfo(" \n Notification : Value Type = %d \n", value_type);
				        WalInfo(" \n Notification : Component ID = %d \n", write_id);
					#if 0 /*Removing Logging of Password due to security requirement*/
				        WalPrint(" \n Notification : New Value = %s \n", p_new_val);
				        WalPrint(" \n Notification : Old Value = %s \n", p_old_val);
					#endif

					if(NULL != p_notify_param_name && (strcmp(p_notify_param_name, WiFi_FactoryResetRadioAndAp)== 0))
					{
						// sleep for 90s to delay the notification and give wifi time to reset and apply to driver
						WalInfo("Delay wifi factory reset notification by 90s so that wifi is reset completely\n");
						sleep(90);
					}

				       /* param.parameterName = p_notify_param_name;
				        param.oldValue = p_old_val;
				        param.newValue = p_new_val;
				        param.type = value_type;
				        param.writeID = write_id;

				        ccspWebPaValueChangedCB(&param,0,NULL);*/ //TODO:implement sync notify
				}
				else
				{
					WalError("Received incorrect data for notification\n");
				}
		        }
			else
			{
				WalError("Received insufficient data to process notification\n");
			}
			WAL_FREE(tmpStr);
		#endif
		}
		else
		{
			WalError("Operation not allowed\n");
			return RBUS_ERROR_INVALID_OPERATION;
		}

        } else {
            WalError("Unexpected value type for property %s\n", paramName);
	    return RBUS_ERROR_INVALID_INPUT;
        }
    }else if(strncmp(paramName, WEBPA_VERSION_PARAM, maxParamLen) == 0) {
	WalError("Version param is not writable\n");
	return RBUS_ERROR_ACCESS_NOT_ALLOWED;
    }
    WalInfo("webpaDataSetHandler End\n");
    return RBUS_ERROR_SUCCESS;
}

/**
 * Common data get handler for all parameters owned by Webpa
 */
rbusError_t webpaDataGetHandler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts) {

    WalInfo("In webpaDataGetHandler\n");
    (void) handle;
    (void) opts;
    char const* propertyName;
    char* componentName = NULL;
    rbusError_t retPsmGet = RBUS_ERROR_BUS_ERROR;

    propertyName = strdup(rbusProperty_GetName(property));
    if(propertyName) {
        WalInfo("Property Name is %s \n", propertyName);
    } else {
        WalError("Unable to handle get request for property \n");
        return RBUS_ERROR_INVALID_INPUT;
    }

    if(strncmp(propertyName, WEBPA_CMC_PARAM, maxParamLen) == 0) {
        rbusValue_t value;
        rbusValue_Init(&value);

        rbusValue_SetUInt32(value, CMCVal);

        rbusProperty_SetValue(property, value);
        rbusValue_Release(value);
	//WalInfo("CMC value fetched is %s\n", value);

    }else if(strncmp(propertyName, WEBPA_CID_PARAM, maxParamLen) == 0) {
        rbusValue_t value;
        rbusValue_Init(&value);

        if(CIDVal){
            rbusValue_SetString(value, CIDVal);
	}
        else{
		WalInfo("B4 rbus_GetValueFromDB\n");
		retPsmGet = rbus_GetValueFromDB( "X_COMCAST-COM_CID", &CIDVal );
		if (retPsmGet != RBUS_ERROR_SUCCESS){
			WalError("psm_get failed ret %d for parameter %s and value %s\n", retPsmGet, propertyName, CIDVal);
			return retPsmGet;
		}
		else{
			WalInfo("psm_get success ret %d for parameter %s and value %s\n", retPsmGet, propertyName, CIDVal);
			rbusValue_SetString(value, CIDVal);
			WalInfo("After CIDVal set to value\n");
		}
	}
        rbusProperty_SetValue(property, value);
	WalInfo("CID value fetched is %s\n", value);
        rbusValue_Release(value);

    }else if(strncmp(propertyName, WEBPA_SYNCVERSION_PARAM, maxParamLen) == 0) {
        rbusValue_t value;
        rbusValue_Init(&value);
        if(syncVersionVal)
            rbusValue_SetString(value, syncVersionVal);
        else
            rbusValue_SetString(value, "");
        rbusProperty_SetValue(property, value);
        rbusValue_Release(value);
	WalInfo("Sync protocol version fetched is %s\n", value);
    }else if(strncmp(propertyName, WEBPA_CONNECTED_CLIENT_PARAM, maxParamLen) == 0) {
        rbusValue_t value;
        rbusValue_Init(&value);
        rbusValue_SetString(value, "");
        rbusProperty_SetValue(property, value);
	WalInfo("ConnClientVal value fetched is %s\n", value);
        rbusValue_Release(value);

    }else if(strncmp(propertyName, WEBPA_NOTIFY_PARAM, maxParamLen) == 0) {
        rbusValue_t value;
        rbusValue_Init(&value);
        rbusValue_SetString(value, "");
        rbusProperty_SetValue(property, value);
	WalInfo("notifyVal value fetched is %s\n", value);
        rbusValue_Release(value);

    }else if(strncmp(propertyName, WEBPA_VERSION_PARAM, maxParamLen) == 0) {
        rbusValue_t value;
        rbusValue_Init(&value);
	char VersionVal[32] ={'\0'};
	snprintf(VersionVal, sizeof(VersionVal), "%s-%s", WEBPA_PROTOCOL, WEBPA_GIT_VERSION);
        if(VersionVal)
            rbusValue_SetString(value, VersionVal);
        else
            rbusValue_SetString(value, "");
        rbusProperty_SetValue(property, value);
	WalInfo("VersionVal value fetched is %s\n", value);
        rbusValue_Release(value);

    }


    if(propertyName) {
        free((char*)propertyName);
        propertyName = NULL;
    }

    WalInfo("webpaDataGetHandler End\n");
    return RBUS_ERROR_SUCCESS;
}

/**
 * To persist TR181 parameter values in PSM DB.
 */
rbusError_t rbus_StoreValueIntoDB(char *paramName, char *value)
{
	char recordName[ 256] = {'\0'};
	char psmName[256] = {'\0'};
	rbusParamVal_t val[1];
	bool commit = TRUE;
	rbusError_t errorcode = RBUS_ERROR_INVALID_INPUT;
	rbus_error_t err = RTMESSAGE_BUS_SUCCESS;

	sprintf(recordName, "%s%s",PSMPrefix, paramName);
	WalInfo("rbus_StoreValueIntoDB recordName is %s\n", recordName);

	val[0].paramName  = recordName;
	val[0].paramValue = value;
	val[0].type = 0;

	sprintf(psmName, "%s%s", "eRT.", DEST_COMP_ID_PSM);
	WalInfo("rbus_StoreValueIntoDB psmName is %s\n", psmName);

	rbusMessage request, response;

	rbusMessage_Init(&request);
	rbusMessage_SetInt32(request, 0); //sessionId
	rbusMessage_SetString(request, WEBPA_COMPONENT_NAME); //component name that invokes the set
	rbusMessage_SetInt32(request, (int32_t)1); //size of params

	rbusMessage_SetString(request, val[0].paramName); //param details
	rbusMessage_SetInt32(request, val[0].type);
	rbusMessage_SetString(request, val[0].paramValue);
	rbusMessage_SetString(request, commit ? "TRUE" : "FALSE");


	if((err = rbus_invokeRemoteMethod(psmName, METHOD_SETPARAMETERVALUES, request, 6000, &response)) != RTMESSAGE_BUS_SUCCESS)
        {
            WalError("rbus_invokeRemoteMethod failed with err %d", err);
            errorcode = RBUS_ERROR_BUS_ERROR;
        }
        else
        {
            int ret = -1;
            char const* pErrorReason = NULL;
            rbusMessage_GetInt32(response, &ret);

            WalInfo("Response from the remote method is [%d]!", ret);
            errorcode = (rbusError_t) ret;

            if((errorcode == RBUS_ERROR_SUCCESS) || (errorcode == CCSP_SUCCESS)) //legacy error codes returned from component PSM.
            {
                errorcode = RBUS_ERROR_SUCCESS;
                WalInfo("Successfully Set the Value");
            }
            else
            {
                rbusMessage_GetString(response, &pErrorReason);
                WalError("Failed to Set the Value for %s", pErrorReason);
            }

            rbusMessage_Release(response);
        }

	return errorcode;
}

/**
 * To fetch TR181 parameter values from PSM DB.
 */
rbusError_t rbus_GetValueFromDB( char* paramName, char** paramValue)
{
	char recordName[ 256] = {'\0'};
	char psmName[256] = {'\0'};
	//rbusParamVal_t val[1]= {NULL};
	char * parameterNames[1] = {NULL};
	int32_t type = 0;
	rbusError_t errorcode = RBUS_ERROR_INVALID_INPUT;
	rbus_error_t err = RTMESSAGE_BUS_SUCCESS;
	*paramValue = NULL;

	sprintf(recordName, "%s%s",PSMPrefix, paramName);
	WalInfo("rbus_GetValueFromDB recordName is %s\n", recordName);

	//val[0].paramName  = (char*)recordName;
	//val[0].paramValue = NULL;
	//val[0].type = 0; //string type
	parameterNames[0] = (char*)recordName;
	WalInfo("rbus_GetValueFromDB parameterNames[0] is %s\n", parameterNames[0]);
	//WalInfo("rbus_GetValueFromDB val[0].paramName is %s\n", val[0].paramName);

	sprintf(psmName, "%s%s", "eRT.", DEST_COMP_ID_PSM);
	WalInfo("rbus_GetValueFromDB psmName is %s\n", psmName);

	rbusMessage request, response;

	rbusMessage_Init(&request);
	//rbusMessage_SetInt32(request, 0); //sessionId
	rbusMessage_SetString(request, psmName); //component name that invokes the get
	rbusMessage_SetInt32(request, (int32_t)1); //size of params

	//rbusMessage_SetString(request, val[0].paramName); //param details
	//rbusMessage_SetInt32(request, val[0].type);
	//rbusMessage_SetString(request, val[0].paramValue);
	rbusMessage_SetString(request, parameterNames[0]); //param details
	//rbusMessage_SetInt32(request, 0); //for string type

	WalInfo("B4 rbus_invokeRemoteMethod..\n");
	if((err = rbus_invokeRemoteMethod(psmName, METHOD_GETPARAMETERVALUES, request, 6000, &response)) != RTMESSAGE_BUS_SUCCESS)
        {
            WalError("rbus_invokeRemoteMethod GET failed with err %d\n", err);
            errorcode = RBUS_ERROR_BUS_ERROR;
        }
        else
        {
            int valSize=0;
	    int ret = -1;
	    rbusMessage_GetInt32(response, &ret);
	    WalInfo("Response from the remote method is [%d]!\n",ret);
            errorcode = (rbusError_t) ret;

	    if((errorcode == RBUS_ERROR_SUCCESS) || (errorcode == CCSP_SUCCESS)) //legacy error code from component.
            {
                WalInfo("Successfully Get the Value\n");
		errorcode = RBUS_ERROR_SUCCESS;
		WalInfo("Received valid response!\n");
		rbusMessage_GetInt32(response, &valSize);
		if(1/*valSize*/)
		{
			char const *buff = NULL;

			//Param Name
			rbusMessage_GetString(response, &buff);
			WalInfo("Requested param buff [%s]\n", buff);
			if(buff && (strcmp(recordName, buff) == 0))
			{
				rbusMessage_GetInt32(response, &type);
				WalInfo("Requested param type [%d]\n", type);
				buff = NULL;
				rbusMessage_GetString(response, &buff);
				*paramValue = strdup(buff); //free buff
				WalInfo("Requested param DB value [%s]\n", *paramValue);
			}
			else
			{
			    WalError("Requested param: [%s], Received Param: [%s]\n", recordName, buff);
			    errorcode = RBUS_ERROR_BUS_ERROR;
			}
		}
            }
            else
            {
		WalError("Response from remote Get method failed!!\n");
		errorcode = RBUS_ERROR_BUS_ERROR;
            }

            rbusMessage_Release(response);
        }
	WalInfo("rbus_GetValueFromDB End\n");
	return errorcode;
}

/**
 * Register data elements for dataModel implementation using rbus.
 * Data element over bus will be Device.DeviceInfo.Webpa.X_COMCAST-COM_CMC,
 *    Device.DeviceInfo.Webpa.X_COMCAST-COM_CID
 */
WDMP_STATUS regWebpaDataModel()
{
	rbusError_t ret = RBUS_ERROR_SUCCESS;
	WDMP_STATUS status = WDMP_SUCCESS;

	WalInfo("Registering parameters deCMC %s, deCID %s, deSyncVersion %s deConnClient %s,deNotify %s, deVersion %s\n", WEBPA_CMC_PARAM, WEBPA_CID_PARAM, WEBPA_SYNCVERSION_PARAM,WEBPA_CONNECTED_CLIENT_PARAM,WEBPA_NOTIFY_PARAM, WEBPA_VERSION_PARAM);
	if(!rbus_handle)
	{
		WalError("regRbusWebpaDataModel Failed in getting bus handles\n");
		return WDMP_FAILURE;
	}

	rbusDataElement_t dataElements[NUM_WEBPA_ELEMENTS] = {

		{WEBPA_CMC_PARAM, RBUS_ELEMENT_TYPE_PROPERTY, {webpaDataGetHandler, webpaDataSetHandler, NULL, NULL, NULL, NULL}},
		{WEBPA_CID_PARAM, RBUS_ELEMENT_TYPE_PROPERTY, {webpaDataGetHandler, webpaDataSetHandler, NULL, NULL, NULL, NULL}},
		{WEBPA_SYNCVERSION_PARAM, RBUS_ELEMENT_TYPE_PROPERTY, {webpaDataGetHandler, webpaDataSetHandler, NULL, NULL, NULL, NULL}},
		{WEBPA_CONNECTED_CLIENT_PARAM, RBUS_ELEMENT_TYPE_PROPERTY, {webpaDataGetHandler, webpaDataSetHandler, NULL, NULL, NULL, NULL}},
		{WEBPA_NOTIFY_PARAM, RBUS_ELEMENT_TYPE_PROPERTY, {webpaDataGetHandler, webpaDataSetHandler, NULL, NULL, NULL, NULL}},
		{WEBPA_VERSION_PARAM, RBUS_ELEMENT_TYPE_PROPERTY, {webpaDataGetHandler, webpaDataSetHandler, NULL, NULL, NULL, NULL}}

	};
	ret = rbus_regDataElements(rbus_handle, NUM_WEBPA_ELEMENTS, dataElements);
	if(ret == RBUS_ERROR_SUCCESS)
	{
		WalInfo("Registered data element %s with rbus \n ", WEBPA_CMC_PARAM);
	}
	else
	{
		WalError("Failed in registering data element %s \n", WEBPA_CMC_PARAM);
		status = WDMP_FAILURE;
	}

	WalInfo("rbus reg status returned is %d\n", status);
	return status;
}
