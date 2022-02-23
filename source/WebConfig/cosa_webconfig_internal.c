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
#include <webcfg_metadata.h>
#include <webcfg.h>
#include <webcfg_generic.h>

#define WEBCONFIG_PARAM_RFC_ENABLE               "Device.X_RDK_WebConfig.RfcEnable"
#define WEBCONFIG_PARAM_URL                      "Device.X_RDK_WebConfig.URL"
#define WEBCONFIG_PARAM_FORCE_SYNC   	         "Device.X_RDK_WebConfig.ForceSync"
#define WEBCONFIG_PARAM_DATA   	    	         "Device.X_RDK_WebConfig.Data"
#define WEBCONFIG_PARAM_SUPPORTED_DOCS	         "Device.X_RDK_WebConfig.SupportedDocs"
#define WEBCONFIG_PARAM_SUPPORTED_VERSION        "Device.X_RDK_WebConfig.SupportedSchemaVersion"
#define WEBCONFIG_PARAM_SUPPLEMENTARY_SERVICE   "Device.X_RDK_WebConfig.SupplementaryServiceUrls."
#define WEBCONFIG_PARAM_SUPPLEMENTARY_TELEMETRY  "Device.X_RDK_WebConfig.SupplementaryServiceUrls.Telemetry"

static char *paramRFCEnable = "eRT.com.cisco.spvtg.ccsp.webpa.WebConfigRfcEnable";

extern PCOSA_BACKEND_MANAGER_OBJECT g_pCosaBEManager;
extern ANSC_HANDLE bus_handle;
extern char        g_Subsystem[32];
static int supplementary_flag = 0;

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
			if(get_global_mpThreadId() == NULL)
			{
				initWebConfigMultipartTask((unsigned long)get_global_operationalStatus());
			}
			else
	    		{
				WebcfgInfo("Webconfig is already started, so not starting again for dynamic rfc change.\n");
	    		}
		}
	}
	else
	{
		sprintf(buf, "%s", "false");
		WebcfgInfo("Received RFC disable. updating g_shutdown\n");
		if(pMyObject->RfcEnable == true)
		{
			/* sending signal to kill initWebConfigMultipartTask thread*/
			pthread_mutex_lock (get_global_sync_mutex());
			set_global_shutdown(true);
			pthread_cond_signal(get_global_sync_condition());
			pthread_mutex_unlock(get_global_sync_mutex());
		}
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

        if((pMyObject != NULL) &&  (strlen(pMyObject->URL)>0))
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

int Get_Supplementary_URL( char *name, char *pString)
{
    PCOSA_DATAMODEL_WEBCONFIG            pMyObject           = (PCOSA_DATAMODEL_WEBCONFIG)g_pCosaBEManager->hWebConfig;
    WebcfgDebug("-------- %s ----- Enter-- ---\n",__FUNCTION__);

        if((pMyObject != NULL) &&  (strlen(pMyObject->Telemetry)>0) && ((name != NULL) && strncmp(name, "Telemetry",strlen(name)+1)) == 0)
        {
		WebcfgDebug("pMyObject->Telemetry %s\n", pMyObject->Telemetry);
                WebcfgDebug("%s ----- updating pString ------\n",__FUNCTION__);

		AnscCopyString( pString,pMyObject->Telemetry );
		WebcfgDebug("pString %s\n",pString);
        }
        else
        {
		char *tempDBUrl = NULL;
                int   retPsmGet    = CCSP_SUCCESS;
		char *tempParam = (char *) malloc (sizeof(char)*MAX_BUFF_SIZE);

                snprintf(tempParam, MAX_BUFF_SIZE, "%s%s", WEBCONFIG_PARAM_SUPPLEMENTARY_SERVICE, name);
                WebcfgDebug("tempParam is %s\n", tempParam);
                retPsmGet = PSM_Get_Record_Value2(bus_handle,g_Subsystem, tempParam, NULL, &tempDBUrl);
		WebcfgDebug("tempDBUrl is %s\n", tempDBUrl);
                if (retPsmGet == CCSP_SUCCESS)
                {
			WebcfgDebug("retPsmGet success\n");
			AnscCopyString( pString,tempDBUrl);
                        WebcfgDebug("pString %s\n",pString);
			free(tempParam);
                }
                else
                {
                        WebcfgError("psm_get failed ret %d for parameter %s%s\n", retPsmGet, WEBCONFIG_PARAM_SUPPLEMENTARY_SERVICE, name);
			free(tempParam);
                        return 0;
                }
        }

        WebcfgDebug("-------- %s ----- Exit ------\n",__FUNCTION__);
        return 1;
}

int Set_Supplementary_URL( char *name, char *pString)
{
    PCOSA_DATAMODEL_WEBCONFIG            pMyObject           = (PCOSA_DATAMODEL_WEBCONFIG)g_pCosaBEManager->hWebConfig;
    WebcfgDebug("-------- %s ----- Enter ------\n",__FUNCTION__);
    int retPsmSet = CCSP_FAILURE;  
    char *tempParam = (char *) malloc (sizeof(char)*MAX_BUFF_SIZE);	      

        if ((name != NULL) && (strncmp(name, "Telemetry",strlen(name)+1)) == 0)
        {		
                memset( pMyObject->Telemetry, 0, sizeof( pMyObject->Telemetry ));
                AnscCopyString( pMyObject->Telemetry, pString );
                snprintf(tempParam, MAX_BUFF_SIZE, "%s%s", WEBCONFIG_PARAM_SUPPLEMENTARY_SERVICE, name);
		WebcfgDebug("tempParam is %s\n", tempParam);
		retPsmSet = PSM_Set_Record_Value2(bus_handle,g_Subsystem, tempParam, ccsp_string, pString);
        }
        else
        {
                WebcfgError("Invalid supplementary doc name\n");
		if(tempParam != NULL){
			free(tempParam);
		}
                return 0;
        }

        if (retPsmSet != CCSP_SUCCESS)
        {
                WebcfgError("psm_set failed ret %d for parameter %s%s and value %s\n", retPsmSet, WEBCONFIG_PARAM_SUPPLEMENTARY_SERVICE, name, pString);
                if(tempParam != NULL){
			free(tempParam);
		}
                return 0;
        }
        else
        {
                WebcfgDebug("psm_set success ret %d for parameter %s%s and value %s\n",retPsmSet, WEBCONFIG_PARAM_SUPPLEMENTARY_SERVICE, name, pString);
        }

        WebcfgDebug("-------- %s ----- Exit ------\n",__FUNCTION__);
	if(tempParam != NULL){
		free(tempParam);
	}
	return 1;
}

int get_supplementary_flag()
{
	return supplementary_flag;
}

void set_supplementary_flag(int flag)
{
	supplementary_flag = flag;
}

int setForceSync(char* pString, char *transactionId,int *pStatus)
{
	PCOSA_DATAMODEL_WEBCONFIG            pMyObject           = (PCOSA_DATAMODEL_WEBCONFIG)g_pCosaBEManager->hWebConfig;
	WebcfgDebug("setForceSync\n");
	WebcfgInfo("SYNC_CRASH: pStatus value is %d\n", pStatus);
	memset( pMyObject->ForceSync, 0, sizeof( pMyObject->ForceSync ));
	AnscCopyString( pMyObject->ForceSync, pString );
	WebcfgDebug("pMyObject->ForceSync is %s\n", pMyObject->ForceSync);

	if(((pMyObject->ForceSync)[0] !='\0') && (strlen(pMyObject->ForceSync)>0))
	{
		if(get_bootSync())
		{
			WebcfgInfo("Bootup sync is already in progress, Ignoring this request.\n");
			WebcfgInfo("SYNC_CRASH: pStatus value before assigned in bootsync is %d\n", pStatus);
			*pStatus = 1;
			WebcfgInfo("SYNC_CRASH: pStatus value after assigned in bootsync is %d\n", pStatus);
			return 0;
		}
		else if(get_maintenanceSync())
		{
			WebcfgInfo("Maintenance window sync is in progress, Ignoring this request.\n");
			*pStatus = 1;
			return 0;
		}
		else if(strlen(pMyObject->ForceSyncTransID)>0)
		{
			WebcfgInfo("Force sync is already in progress, Ignoring this request.\n");
			WebcfgInfo("SYNC_CRASH: pStatus value before assigned in ForceSync is %d\n", pStatus);
			*pStatus = 1;
			WebcfgInfo("SYNC_CRASH: pStatus value after assigned in ForceSync is %d\n", pStatus);
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

	if(((pMyObject->ForceSync)[0] != '\0') && strlen(pMyObject->ForceSync)>0)
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
				WebcfgDebug("Force Sync GET is not supported\n");
                                paramVal[k]->parameterName = strndup(WEBCONFIG_PARAM_FORCE_SYNC, MAX_PARAMETERNAME_LEN);
				paramVal[k]->parameterValue = strndup("",MAX_PARAMETERVALUE_LEN);
                                paramVal[k]->type = ccsp_string;
                                k++;
                            }
			    else if((strcmp(parameterNames[i], WEBCONFIG_PARAM_URL) == 0) && (RFC_ENABLE == true))
                            {
				char valuestr[256] = {0};
				Get_Webconfig_URL(valuestr);
				paramVal[k]->parameterName = strndup(WEBCONFIG_PARAM_URL, MAX_PARAMETERNAME_LEN);     	                
				if( strlen(valuestr) >0 )
				{
		                       	paramVal[k]->parameterValue = strndup(valuestr,MAX_PARAMETERVALUE_LEN);
				}
				else
				{
					WebcfgDebug("Webpa get : Get_Webconfig_URL %s is null\n", valuestr);
					paramVal[k]->parameterValue = strndup("",MAX_PARAMETERVALUE_LEN);
					
				}
				paramVal[k]->type = ccsp_string;
				k++;
                            }
				 else if((strcmp(parameterNames[i], WEBCONFIG_PARAM_DATA) == 0) && (RFC_ENABLE == true))	
				{
					WebcfgDebug("B4 Get_Webconfig_Data\n");
					char* b64buffer =  get_DB_BLOB_base64();
					paramVal[k]->parameterName = strndup(WEBCONFIG_PARAM_DATA, MAX_PARAMETERNAME_LEN);    						if( (b64buffer != NULL) && strlen(b64buffer) >0 )
					{
						WebcfgDebug("Webpa get : Datafetched %s\n", b64buffer);
				                paramVal[k]->parameterValue = strdup(b64buffer);      			                
	     				}
					else
					{
						WebcfgDebug("Webpa get : B4 Get_Webconfig_Data %s is empty\n", b64buffer);
						paramVal[k]->parameterValue = strdup("");                               
					}
					paramVal[k]->type = ccsp_string;
					k++;
			      }
                            else if((strcmp(parameterNames[i], WEBCONFIG_PARAM_SUPPORTED_DOCS) == 0) && (RFC_ENABLE == true))
                            {
                                  WebcfgDebug("B4 Get_Webconfig_Supported_Docs\n");
                                  char* docvalue =  getsupportedDocs();
                                  paramVal[k]->parameterName = strndup(WEBCONFIG_PARAM_SUPPORTED_DOCS, MAX_PARAMETERNAME_LEN);
                                  if( (docvalue != NULL) && strlen(docvalue) >0 )
                                  {
                                      WebcfgDebug("Webpa get : Datafetched %s\n", docvalue);
                                      paramVal[k]->parameterValue = strdup(docvalue);
				  }
                                  else
				  {
				      paramVal[k]->parameterValue = strdup("");
				  }
                                  paramVal[k]->type = ccsp_string;
				  k++;
                            }
                            else if((strcmp(parameterNames[i], WEBCONFIG_PARAM_SUPPORTED_VERSION) == 0) && (RFC_ENABLE == true))
                            {
                                  WebcfgDebug("B4 Get_Webconfig_Supported_Version\n");
                                  char* versionvalue = getsupportedVersion();
                                  paramVal[k]->parameterName = strndup(WEBCONFIG_PARAM_SUPPORTED_VERSION, MAX_PARAMETERNAME_LEN);
                                  if( (versionvalue != NULL) && strlen(versionvalue) >0 )
                                  {
                                      WebcfgDebug("Webpa get : Datafetched %s\n", versionvalue);
                                      paramVal[k]->parameterValue = strdup(versionvalue);
                                  }
                                  else
				  {
				      paramVal[k]->parameterValue = strdup("");
				  }
                                  paramVal[k]->type = ccsp_string;
				  k++;
                            }
                            else if((strcmp(parameterNames[i], WEBCONFIG_PARAM_SUPPLEMENTARY_TELEMETRY) == 0) && (RFC_ENABLE == true))
                            {
				char valuestr[256] = {0};
				Get_Supplementary_URL("Telemetry", valuestr);
				paramVal[k]->parameterName = strndup(WEBCONFIG_PARAM_SUPPLEMENTARY_TELEMETRY, MAX_PARAMETERNAME_LEN);
				if( strlen(valuestr) >0 )
				{
                                        paramVal[k]->parameterValue = strndup(valuestr,MAX_PARAMETERVALUE_LEN);
				}
				else
				{
					WebcfgDebug("Webpa get : Get_Supplementary_URL %s is null\n", valuestr);
					paramVal[k]->parameterValue = strndup("",MAX_PARAMETERVALUE_LEN);
				}
				paramVal[k]->type = ccsp_string;
				k++;
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
                                localCount = localCount+6;
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
				if( (webcfg_url[0] !='\0') && strlen(webcfg_url)>0 )
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
				}
				else
				{
					paramVal[k]->parameterValue = strndup("",MAX_PARAMETERVALUE_LEN);
				}
                                paramVal[k]->type = ccsp_string;
                                k++;

				WebcfgDebug("Webpa wildcard get for SupportedDocs\n");
				paramVal[k] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t));
                                memset(paramVal[k], 0, sizeof(parameterValStruct_t));
                                paramVal[k]->parameterName = strndup(WEBCONFIG_PARAM_SUPPORTED_DOCS, MAX_PARAMETERNAME_LEN);
				char* docvalue =  getsupportedDocs();
				if( (docvalue != NULL) && strlen(docvalue) >0 )
				{
					WebcfgDebug("Webpa get : Datafetched %s\n", docvalue);
					paramVal[k]->parameterValue = strdup(docvalue);
     				}
				else
				{
					paramVal[k]->parameterValue = strdup("");
				}
                                paramVal[k]->type = ccsp_string;
				k++;

				WebcfgDebug("Webpa wildcard get for SupportedVersions\n");
				paramVal[k] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t));
                                memset(paramVal[k], 0, sizeof(parameterValStruct_t));
                                paramVal[k]->parameterName = strndup(WEBCONFIG_PARAM_SUPPORTED_VERSION, MAX_PARAMETERNAME_LEN);
				char* versionvalue =  getsupportedVersion();
				if( (versionvalue != NULL) && strlen(versionvalue) >0 )
				{
					WebcfgDebug("Webpa get : Datafetched %s\n", versionvalue);
					paramVal[k]->parameterValue = strdup(versionvalue);
}
				else
				{
					paramVal[k]->parameterValue = strdup("");
				}
                                paramVal[k]->type = ccsp_string;
				k++;

				WebcfgDebug("Webpa wildcard get for Telemetry URL\n");
				paramVal[k] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t));
                                memset(paramVal[k], 0, sizeof(parameterValStruct_t));
                                paramVal[k]->parameterName = strndup(WEBCONFIG_PARAM_SUPPLEMENTARY_TELEMETRY, MAX_PARAMETERNAME_LEN);
				char telemetry_url[256] = {0};
				Get_Supplementary_URL("Telemetry", telemetry_url);
				if( (telemetry_url[0] !='\0') && strlen(telemetry_url)>0 )
				{
					WebcfgDebug("webcfg_url fetched %s\n", telemetry_url);
					paramVal[k]->parameterValue = strndup(telemetry_url,MAX_PARAMETERVALUE_LEN);
				}
				else
				{
					paramVal[k]->parameterValue = strndup("",MAX_PARAMETERVALUE_LEN);
				}
                                paramVal[k]->type = ccsp_string;
                                k++;
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
	WebcfgInfo("SYNC_CRASH: session_status before set is %d\n", session_status);
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
					if(strcmp(val[i].parameterValue, "telemetry") == 0)
					{
						char telemetryUrl[256] = {0};
						Get_Supplementary_URL("Telemetry", telemetryUrl);
						if(strncmp(telemetryUrl,"NULL",strlen("NULL")) == 0)
						{
							WebcfgError("Force Sync param failed due to telemtry url is null\n");
							return CCSP_CR_ERR_UNSUPPORTED_NAMESPACE;
						}
					}
					ret = setForceSync(val[i].parameterValue, transactionId, &session_status);
					WebcfgInfo("SYNC_CRASH: session_status after set is %d\n", session_status);
					WebcfgDebug("After setForceSync ret %d\n", ret);
				}
				else //pass empty transaction id when Force sync is with empty doc
				{
					WebcfgDebug("setWebConfigParameterValues empty setForceSync\n");
					ret = setForceSync(val[i].parameterValue, "", 0);
					WebcfgInfo("SYNC_CRASH: session_status after set in else case is %d\n", session_status);
				}
				WebcfgInfo("SYNC_CRASH: session_status outside if else case is %d\n", session_status);
				if(session_status)
				{
					//session_status = 0;  //To reset the static variable
					return CCSP_CR_ERR_SESSION_IN_PROGRESS;
				}
				//session_status = 0;          //To reset the static variable
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
					WebcfgDebug("After Set_Webconfig_URL ret %d\n", ret);
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
			else if((strcmp(val[i].parameterName, WEBCONFIG_PARAM_SUPPORTED_DOCS) == 0) && (RFC_ENABLE == true))
			{
				WebcfgError("%s is not writable\n",val[i].parameterName);
				*faultParam = strdup(val[i].parameterName);
				return CCSP_ERR_NOT_WRITABLE;
			}
			else if((strcmp(val[i].parameterName, WEBCONFIG_PARAM_SUPPORTED_VERSION) == 0) && (RFC_ENABLE == true))
			{
				WebcfgError("%s is not writable\n",val[i].parameterName);
				*faultParam = strdup(val[i].parameterName);
				return CCSP_ERR_NOT_WRITABLE;
			}
			else if((strcmp(val[i].parameterName, WEBCONFIG_PARAM_SUPPLEMENTARY_TELEMETRY) == 0) && (RFC_ENABLE == true))
			{
				WebcfgDebug("Processing Telemetry URL param %s\n", val[i].parameterValue);
				set_supplementary_flag(1);
				if(isValidUrl(val[i].parameterValue) == TRUE || strncmp(val[i].parameterValue,"NULL",strlen("NULL")) == 0)
				{
					set_supplementary_flag(0);
					ret = Set_Supplementary_URL("Telemetry", val[i].parameterValue);
					WebcfgDebug("After Set_Webconfig_URL ret %d\n", ret);
					if(ret != 1)
					{
						WebcfgError("Set_Supplementry_URL failed\n");
						return CCSP_FAILURE;
					}
				}
				else
				{
					WebcfgError("Telemetry URL validation failed\n");
					return CCSP_ERR_INVALID_PARAMETER_VALUE;
				}
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

int unregisterWebcfgEvent()
{
	int ret = 0;

	ret = CcspBaseIf_UnRegister_Event(bus_handle, NULL, "webconfigSignal");
	WebcfgInfo("unregisterWebcfgEvent ret is %d\n", ret);
	if (ret != 100)
	{
		WebcfgError("CcspBaseIf_UnRegister_Event failed for webconfigSignal\n");
	}
	else
	{
		WebcfgInfo("Un Registration with CCSP Bus is success\n");
		return 1;
	}
	return 0;
}

