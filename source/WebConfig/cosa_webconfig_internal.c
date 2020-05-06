/*
 * If not stated otherwise in this file or this component's Licenses.txt file the
 * following copyright and licenses apply:
 *
 * Copyright 2016 RDK Management
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
#include "ansc_platform.h"
#include "plugin_main_apis.h"
#include "cosa_webconfig_apis.h"
#include "cosa_webconfig_dml.h"
#include "cosa_webconfig_internal.h"
#include <webcfg_log.h>
#include <webcfg_db.h>
#include <webcfg.h>
#include <webcfg_generic.h>

#define WEBCONFIG_PARAM_RFC_ENABLE          "Device.X_RDK_WebConfig.RfcEnable"
#define WEBCONFIG_PARAM_URL                 "Device.X_RDK_WebConfig.URL"
#define WEBCONFIG_PARAM_FORCE_SYNC   	    "Device.X_RDK_WebConfig.ForceSync"
#define WEBCONFIG_PARAM_DATA   	    	    "Device.X_RDK_WebConfig.Data"

static char *paramRFCEnable = "eRT.com.cisco.spvtg.ccsp.webpa.Device.X_RDK_WebConfig.RfcEnable";

extern PCOSA_BACKEND_MANAGER_OBJECT g_pCosaBEManager;
extern ANSC_HANDLE bus_handle;
extern char        g_Subsystem[32];

BOOL Get_RfcEnable()
{
    PCOSA_DATAMODEL_WEBCONFIG            pMyObject           = (PCOSA_DATAMODEL_WEBCONFIG)g_pCosaBEManager->hWebConfig;
    return pMyObject->RfcEnable;
}

int setRfcEnable(BOOL bValue)
{
	PCOSA_DATAMODEL_WEBCONFIG            pMyObject           = (PCOSA_DATAMODEL_WEBCONFIG)g_pCosaBEManager->hWebConfig;
	char buf[16] = {0};
	int retPsmSet = CCSP_SUCCESS;

	if(bValue == TRUE)
	{
		sprintf(buf, "%s", "true");
		WebcfgDebug("Received RFC enable. updating g_shutdown\n");
		if(pMyObject->RfcEnable == false)
		{
			pthread_mutex_lock (get_global_sync_mutex());
			set_global_shutdown(false);
			pthread_mutex_unlock(get_global_sync_mutex());
			WebcfgInfo("RfcEnable dynamic change from false to true. start initWebConfigMultipartTask.\n");
			initWebConfigMultipartTask(0);
		}
	}
	else
	{
		sprintf(buf, "%s", "false");
		WebcfgInfo("Received RFC disable. updating g_shutdown\n");
		/* sending signal to kill initWebConfigMultipartTask thread*/
		pthread_mutex_lock (get_global_sync_mutex());
		set_global_shutdown(true);
		pthread_cond_signal(get_global_sync_condition());
		pthread_mutex_unlock(get_global_sync_mutex());
	}  
#ifdef RDKB_BUILD
	retPsmSet = PSM_Set_Record_Value2(bus_handle,g_Subsystem, paramRFCEnable, ccsp_string, buf);
        if (retPsmSet != CCSP_SUCCESS)
        {
                WebcfgError("psm_set failed ret %d for parameter %s and value %s\n", retPsmSet, paramRFCEnable, buf);
                return 1;
        }
        else
        {
                WebcfgDebug("psm_set success ret %d for parameter %s and value %s\n", retPsmSet, paramRFCEnable, buf);
		pMyObject->RfcEnable = bValue;
		WebcfgDebug("pMyObject->RfcEnable is %d\n", pMyObject->RfcEnable);
		return 0;
        }
#endif
	return 0;
}

BOOL CosaDmlGetRFCEnableFromDB(BOOL *pbValue)
{
    char* strValue = NULL;

    WebcfgDebug("-------- %s ----- Enter-- ---\n",__FUNCTION__);

    *pbValue = FALSE;
    if (CCSP_SUCCESS == PSM_Get_Record_Value2(bus_handle,
                g_Subsystem, paramRFCEnable, NULL, &strValue))
    {
        if(((strcmp (strValue, "true") == 0)) || (strcmp (strValue, "TRUE") == 0))
	{
            *pbValue = TRUE;
        }
        ((CCSP_MESSAGE_BUS_INFO *)bus_handle)->freefunc( strValue );
        return TRUE;
    }
    WebcfgDebug("-------- %s ----- Exit-- ---\n",__FUNCTION__);
    return FALSE;
}

int Get_Webconfig_URL( char *pString)
{
    PCOSA_DATAMODEL_WEBCONFIG            pMyObject           = (PCOSA_DATAMODEL_WEBCONFIG)g_pCosaBEManager->hWebConfig;
    WebcfgDebug("-------- %s ----- Enter-- ---\n",__FUNCTION__);

        if((pMyObject != NULL) && (pMyObject->URL != NULL) && (strlen(pMyObject->URL)>0))
        {
		WebcfgDebug("pMyObject->URL %s\n", pMyObject->URL);
                WebcfgDebug("%s ----- updating pString ------\n",__FUNCTION__);
		
		AnscCopyString( pString,pMyObject->URL );
		WebcfgDebug("pString %s\n",pString);
        }
        else
        {
		char *tempDBUrl = NULL;
                int   retPsmGet    = CCSP_SUCCESS;
                retPsmGet = PSM_Get_Record_Value2(bus_handle,g_Subsystem, WEBCONFIG_PARAM_URL, NULL, &tempDBUrl);
		WebcfgDebug("tempDBUrl is %s\n", tempDBUrl);
                if (retPsmGet == CCSP_SUCCESS)
                {
			WebcfgDebug("retPsmGet success\n");
			AnscCopyString( pString,tempDBUrl);
                        WebcfgDebug("pString %s\n",pString);
                }
                else
                {
                        WebcfgError("psm_get failed ret %d for parameter %s\n", retPsmGet, WEBCONFIG_PARAM_URL);
                        return 0;
                }
        }

        WebcfgDebug("-------- %s ----- Exit ------\n",__FUNCTION__);
        return 1;
}

int Set_Webconfig_URL( char *pString)
{
    PCOSA_DATAMODEL_WEBCONFIG            pMyObject           = (PCOSA_DATAMODEL_WEBCONFIG)g_pCosaBEManager->hWebConfig;
    WebcfgDebug("-------- %s ----- Enter ------\n",__FUNCTION__);
    int retPsmSet = CCSP_SUCCESS;

        memset( pMyObject->URL, 0, sizeof( pMyObject->URL ));
        AnscCopyString( pMyObject->URL, pString );


        retPsmSet = PSM_Set_Record_Value2(bus_handle,g_Subsystem, WEBCONFIG_PARAM_URL, ccsp_string, pString);
        if (retPsmSet != CCSP_SUCCESS)
        {
                WebcfgError("psm_set failed ret %d for parameter %s and value %s\n", retPsmSet, WEBCONFIG_PARAM_URL, pString);
                return 0;
        }
        else
        {
                WebcfgDebug("psm_set success ret %d for parameter %s and value %s\n", retPsmSet, WEBCONFIG_PARAM_URL, pString);
        }

        WebcfgDebug("-------- %s ----- Exit ------\n",__FUNCTION__);
        return 1;
}


int setForceSync(char* pString, char *transactionId,int *pStatus)
{
	PCOSA_DATAMODEL_WEBCONFIG            pMyObject           = (PCOSA_DATAMODEL_WEBCONFIG)g_pCosaBEManager->hWebConfig;
	WebcfgDebug("setForceSync\n");
	memset( pMyObject->ForceSync, 0, sizeof( pMyObject->ForceSync ));
	AnscCopyString( pMyObject->ForceSync, pString );
	WebcfgDebug("pMyObject->ForceSync is %s\n", pMyObject->ForceSync);

	if((pMyObject->ForceSync !=NULL) && (strlen(pMyObject->ForceSync)>0))
	{
		if(strlen(pMyObject->ForceSyncTransID)>0)
		{
			WebcfgInfo("Force sync is already in progress, Ignoring this request.\n");
			*pStatus = 1;
			return 0;
		}
		else
		{
			/* sending signal to initWebConfigMultipartTask to update the sync time interval*/
			pthread_mutex_lock (get_global_sync_mutex());

			//Update ForceSyncTransID to access webpa transactionId in webConfig sync.
			if(transactionId !=NULL && (strlen(transactionId)>0))
			{
				AnscCopyString(pMyObject->ForceSyncTransID, transactionId);
				WebcfgInfo("pMyObject->ForceSyncTransID is %s\n", pMyObject->ForceSyncTransID);
			}
			WebcfgInfo("Trigger force sync\n");
			pthread_cond_signal(get_global_sync_condition());
			pthread_mutex_unlock(get_global_sync_mutex());
		}
	}
	else
	{
		WebcfgDebug("Force sync param set with empty value\n");
		memset(pMyObject->ForceSyncTransID,0,sizeof(pMyObject->ForceSyncTransID));
	}
	WebcfgDebug("setForceSync returns success 1\n");
	return 1;
}

int getForceSync(char** pString, char **transactionId )
{
	PCOSA_DATAMODEL_WEBCONFIG pMyObject = (PCOSA_DATAMODEL_WEBCONFIG)g_pCosaBEManager->hWebConfig;
	WebcfgDebug("-------- %s ----- Enter ------\n",__FUNCTION__);

	if((pMyObject->ForceSync != NULL) && strlen(pMyObject->ForceSync)>0)
	{
		WebcfgDebug("%s ----- updating pString ------\n",__FUNCTION__);
		*pString = strdup(pMyObject->ForceSync);
		WebcfgDebug("%s ----- updating transactionId ------\n",__FUNCTION__);
		*transactionId = strdup(pMyObject->ForceSyncTransID);
	}
	else
	{
		*pString = NULL;
		*transactionId = NULL;
		return 0;
	}
	WebcfgDebug("*transactionId is %s\n",*transactionId);
	WebcfgDebug("-------- %s ----- Exit ------\n",__FUNCTION__);
	return 1;
}

int getWebConfigParameterValues(char **parameterNames, int paramCount, int *val_size, parameterValStruct_t ***val)
{
    char *object ="Device.X_RDK_WebConfig.";
    parameterValStruct_t **paramVal = NULL;
    paramVal = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t *)*paramCount);
    int i=0, k=0, isWildcard = 0, matchFound = 0;
    int localCount = paramCount;
    BOOL RFC_ENABLE;
    WebcfgDebug("*********** %s ***************\n",__FUNCTION__);

    RFC_ENABLE = Get_RfcEnable();
    WebcfgDebug("paramCount = %d\n",paramCount);

    for(i=0; i<paramCount; i++)
    {
        if(parameterNames[i][strlen(parameterNames[i])-1] == '.')
        {
            isWildcard = 1;
        }
        else
        {
            isWildcard = 0;
        }
            if(strstr(parameterNames[i],object) != NULL)
            {
                matchFound = 1;
                        if(isWildcard == 0)
                        {
                            paramVal[k] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t));
                            memset(paramVal[k], 0, sizeof(parameterValStruct_t));
                            if((strcmp(parameterNames[i], WEBCONFIG_PARAM_RFC_ENABLE) == 0))
                            {
                                paramVal[k]->parameterName = strndup(WEBCONFIG_PARAM_RFC_ENABLE, MAX_PARAMETERNAME_LEN);
                                WebcfgDebug("paramVal[%d]->parameterName: %s\n",k,paramVal[k]->parameterName);
                                WebcfgDebug("RfcEnable is %d\n",RFC_ENABLE);
                                if(RFC_ENABLE == true)
                                {
                                    paramVal[k]->parameterValue = strndup("true",MAX_PARAMETERVALUE_LEN);
                                }
                                else
                                {
                                    paramVal[k]->parameterValue = strndup("false",MAX_PARAMETERVALUE_LEN);
                                }
                                paramVal[k]->type = ccsp_boolean;
                                k++;
                            }
			    else if((strcmp(parameterNames[i], WEBCONFIG_PARAM_FORCE_SYNC) == 0) && (RFC_ENABLE == true))
                            {
				WebcfgError("Force Sync GET is not supported\n");
                                paramVal[k]->parameterName = strndup(WEBCONFIG_PARAM_FORCE_SYNC, MAX_PARAMETERNAME_LEN);
				paramVal[k]->parameterValue = strndup("",MAX_PARAMETERVALUE_LEN);
                                paramVal[k]->type = ccsp_string;
                                k++;
                            }
			    else if((strcmp(parameterNames[i], WEBCONFIG_PARAM_URL) == 0) && (RFC_ENABLE == true))
                            {
				char valuestr[256] = {0};
				Get_Webconfig_URL(valuestr);
				if( (valuestr != NULL) && strlen(valuestr) >0 )
				{
		                        paramVal[k]->parameterName = strndup(WEBCONFIG_PARAM_URL, MAX_PARAMETERNAME_LEN);
					paramVal[k]->parameterValue = strndup(valuestr,MAX_PARAMETERVALUE_LEN);
		                        paramVal[k]->type = ccsp_string;
		                        k++;
				}
                            }
				 else if((strcmp(parameterNames[i], WEBCONFIG_PARAM_DATA) == 0) && (RFC_ENABLE == true))	
				{
					WebcfgDebug("B4 Get_Webconfig_Data\n");
					char* b64buffer =  get_DB_BLOB_base64();
			  		if( (b64buffer != NULL) && strlen(b64buffer) >0 )
					{
						WebcfgDebug("Webpa get : Datafetched %s\n", b64buffer);
				                paramVal[k]->parameterName = strndup(WEBCONFIG_PARAM_DATA, MAX_PARAMETERNAME_LEN);
						paramVal[k]->parameterValue = strdup(b64buffer);
				                paramVal[k]->type = ccsp_string;
				                k++;
	     				}
			      }
                            else
                            {
                                WAL_FREE(paramVal[k]);
                                matchFound = 0;
                            }
                        }
                        else
                        {
                            if(RFC_ENABLE)
                            {
				WebcfgDebug("Updating localCount %d in wildcard GET case\n", localCount);
                                localCount = localCount+3;
                            }
				WebcfgDebug("Updated localCount %d\n", localCount);
                            paramVal = (parameterValStruct_t **) realloc(paramVal, sizeof(parameterValStruct_t *)*localCount);
                            paramVal[k] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t));
                            memset(paramVal[k], 0, sizeof(parameterValStruct_t));
                            paramVal[k]->parameterName = strndup(WEBCONFIG_PARAM_RFC_ENABLE, MAX_PARAMETERNAME_LEN);
                            WebcfgDebug("paramVal[%d]->parameterName: %s\n",k,paramVal[k]->parameterName);
                            WebcfgDebug("RfcEnable is %d\n",RFC_ENABLE);
                            if(RFC_ENABLE == true)
                            {
                                paramVal[k]->parameterValue = strndup("true",MAX_PARAMETERVALUE_LEN);
                            }
                            else
                            {
                                paramVal[k]->parameterValue = strndup("false",MAX_PARAMETERVALUE_LEN);
                            }
                            paramVal[k]->type = ccsp_boolean;
                            k++;
                            if(RFC_ENABLE)
                              {
                                WebcfgDebug("Webpa wildcard get for force sync\n");
				paramVal[k] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t));
                                memset(paramVal[k], 0, sizeof(parameterValStruct_t));
                                paramVal[k]->parameterName = strndup(WEBCONFIG_PARAM_FORCE_SYNC, MAX_PARAMETERNAME_LEN);
				paramVal[k]->parameterValue = strndup("",MAX_PARAMETERVALUE_LEN);
                                paramVal[k]->type = ccsp_string;
                                k++;

				WebcfgDebug("Webpa wildcard get for URL\n");
				paramVal[k] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t));
                                memset(paramVal[k], 0, sizeof(parameterValStruct_t));
                                paramVal[k]->parameterName = strndup(WEBCONFIG_PARAM_URL, MAX_PARAMETERNAME_LEN);
				char webcfg_url[256] = {0};
				Get_Webconfig_URL(webcfg_url);
				if( (webcfg_url !=NULL) && strlen(webcfg_url)>0 )
				{
					WebcfgDebug("webcfg_url fetched %s\n", webcfg_url);
					paramVal[k]->parameterValue = strndup(webcfg_url,MAX_PARAMETERVALUE_LEN);
				}
				else
				{
					paramVal[k]->parameterValue = strndup("",MAX_PARAMETERVALUE_LEN);
				}
                                paramVal[k]->type = ccsp_string;
                                k++;

				WebcfgDebug("Webpa wildcard get for Data\n");
				paramVal[k] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t));
                                memset(paramVal[k], 0, sizeof(parameterValStruct_t));
                                paramVal[k]->parameterName = strndup(WEBCONFIG_PARAM_DATA, MAX_PARAMETERNAME_LEN);
				char* b64buffer =  get_DB_BLOB_base64();
		  		if( (b64buffer != NULL) && strlen(b64buffer) >0 )
				{
					WebcfgDebug("Webpa get : Datafetched %s\n", b64buffer);
					paramVal[k]->parameterValue = strdup(b64buffer);
					paramVal[k]->type = ccsp_string;
		                        k++;
     				}
                            }
                        }
            }
        if(matchFound == 0)
        {
            if(!RFC_ENABLE)
            {
                WebcfgError("RFC disabled. Hence not proceeding with GET\n");
            }
            else
            {
                WebcfgError("%s is invalid parameter\n",parameterNames[i]);
            }
            *val = NULL;
            *val_size = 0;
            for(k=k-1;k>=0;k--)
            {
                WAL_FREE(paramVal[k]->parameterName);
                WAL_FREE(paramVal[k]->parameterValue);
                WAL_FREE(paramVal[k]);
            }
            WAL_FREE(paramVal);
            return CCSP_CR_ERR_UNSUPPORTED_NAMESPACE;
        }
    }
    *val = paramVal;
    for(i=0; i<k; i++)
    {
        WebcfgDebug("Final-> %s %s %d\n",(*val)[i]->parameterName, (*val)[i]->parameterValue, (*val)[i]->type);
    }
    *val_size = k;
    WebcfgDebug("Final count is %d\n",*val_size);
    WebcfgDebug("*********** %s ***************\n",__FUNCTION__);
    return CCSP_SUCCESS;
}

int setWebConfigParameterValues(parameterValStruct_t *val, int paramCount, char **faultParam, char *transactionId )
{
	int i=0;
	char *subStr = NULL;
	BOOL RFC_ENABLE;
	int session_status = 0;
	int ret = 0;
	WebcfgDebug("*********** %s ***************\n",__FUNCTION__);

	char *webConfigObject = "Device.X_RDK_WebConfig.";
	RFC_ENABLE = Get_RfcEnable();

	WebcfgDebug("paramCount = %d\n",paramCount);
	for(i=0; i<paramCount; i++)
	{
		if(strstr(val[i].parameterName, webConfigObject) != NULL)
		{
			if(strcmp(val[i].parameterName, WEBCONFIG_PARAM_RFC_ENABLE) == 0)
			{
				if((val[i].parameterValue != NULL) && (strcmp(val[i].parameterValue, "true") == 0))
				{
					setRfcEnable(true);
				}
				else
				{
					setRfcEnable(false);
				}
			}
			else if((strcmp(val[i].parameterName, WEBCONFIG_PARAM_FORCE_SYNC) == 0) && (RFC_ENABLE == true))
			{
				WebcfgDebug("Processing Force Sync param\n");
				if((val[i].parameterValue !=NULL) && (strlen(val[i].parameterValue)>0))
				{
					ret = setForceSync(val[i].parameterValue, transactionId, &session_status);
					WebcfgDebug("After setForceSync ret %d\n", ret);
				}
				else //pass empty transaction id when Force sync is with empty doc
				{
					WebcfgDebug("setWebConfigParameterValues empty setForceSync\n");
					ret = setForceSync(val[i].parameterValue, "", 0);
				}
				if(session_status)
				{
					return CCSP_CR_ERR_SESSION_IN_PROGRESS;
				}
				if(!ret)
				{
					WebcfgError("setForceSync failed\n");
					return CCSP_FAILURE;
				}
			}
			else if((strcmp(val[i].parameterName, WEBCONFIG_PARAM_URL) == 0) && (RFC_ENABLE == true))
			{
				WebcfgDebug("Processing Webcfg URL param %s\n", val[i].parameterValue);
				if(isValidUrl(val[i].parameterValue) == TRUE)
				{
					ret = Set_Webconfig_URL(val[i].parameterValue);
					WebcfgDebug("After Set_Webconfig_UR ret %d\n", ret);
					if(ret != 1)
					{
						WebcfgError("Set_Webconfig_URL failed\n");
						return CCSP_FAILURE;
					}
				}
				else
				{
					WebcfgError("Webcfg URL validation failed\n");
					return CCSP_ERR_INVALID_PARAMETER_VALUE;
				}
			}
			else if((strcmp(val[i].parameterName, WEBCONFIG_PARAM_DATA) == 0) && (RFC_ENABLE == true))
			{
				WebcfgError("%s is not writable\n",val[i].parameterName);
				*faultParam = strdup(val[i].parameterName);
				return CCSP_ERR_NOT_WRITABLE;
			}
			else if(!RFC_ENABLE)
			{
				WebcfgError("RFC disabled. Hence not proceeding with SET\n");
				return CCSP_ERR_INVALID_PARAMETER_VALUE;
			}
		}
		else
		{
			WebcfgError("%s is not writable\n",val[i].parameterName);
			*faultParam = strdup(val[i].parameterName);
			return CCSP_ERR_NOT_WRITABLE;
		}
	}
	WebcfgDebug("*********** %s ***************\n",__FUNCTION__);
	return CCSP_SUCCESS;
}

int registerWebcfgEvent(WebConfigEventCallback webcfgEventCB)
{
	int ret = 0;

	CcspBaseIf_SetCallback2(bus_handle, "webconfigSignal",
            webcfgEventCB, NULL);

	ret = CcspBaseIf_Register_Event(bus_handle, NULL, "webconfigSignal");
	WebcfgInfo("registerWebcfgEvent ret is %d\n", ret);
	if (ret != 100)
	{
		WebcfgError("CcspBaseIf_Register_Event failed for webconfigSignal\n");
	}
	else
	{
		WebcfgInfo("Registration with CCSP Bus is success, waiting for events from components\n");
		return 1;
	}
	return 0;
}
