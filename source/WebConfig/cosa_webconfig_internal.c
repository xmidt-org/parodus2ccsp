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
#include "webconfig_log.h"
#include "webconfig_internal.h"

#define WEBCONFIG_PARAM_RFC_ENABLE          "Device.X_RDK_WebConfig.RfcEnable"
#define WEBCONFIG_PARAM_URL                 "Device.X_RDK_WebConfig.URL"
#define WEBCONFIG_PARAM_CONFIGFILE_ENTRIES  "Device.X_RDK_WebConfig.ConfigFileNumberOfEntries"
#define WEBCONFIG_PARAM_PERIODIC_INTERVAL   "Device.X_RDK_WebConfig.PeriodicSyncCheckInterval"
#define WEBCONFIG_PARAM_FORCE_SYNC   	    "Device.X_RDK_WebConfig.ForceSync"
#define WEBCONFIG_TABLE_CONFIGFILE          "Device.X_RDK_WebConfig.ConfigFile."
#define CONFIGFILE_PARAM_URL                "URL"
#define CONFIGFILE_PARAM_VERSION            "Version"
#define CONFIGFILE_PARAM_FORCE_SYNC         "ForceSyncCheck"
#define CONFIGFILE_PARAM_SYNC_CHECK_OK      "SyncCheckOK"
#define CONFIGFILE_PARAM_REQUEST_TIME_STAMP     "RequestTimeStamp"

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
	if(bValue == TRUE)
	{
		sprintf(buf, "%s", "true");
		WebcfgDebug("Received RFC enable. updating g_shutdown\n");
		if(pMyObject->RfcEnable == false)
		{
			pthread_mutex_lock (get_global_periodicsync_mutex());
			g_shutdown  = false;
			pthread_mutex_unlock(get_global_periodicsync_mutex());
			WebConfigLog("RfcEnable dynamic change from false to true. start initWebConfigMultipartTask.\n");
			initWebConfigMultipartTask();
		}
	}
	else
	{
		sprintf(buf, "%s", "false");
		WebConfigLog("Received RFC disable. updating g_shutdown\n");
		/* sending signal to kill initWebConfigMultipartTask thread*/
		pthread_mutex_lock (get_global_periodicsync_mutex());
		g_shutdown  = true;
		pthread_cond_signal(get_global_periodicsync_condition());
		pthread_mutex_unlock(get_global_periodicsync_mutex());
	}  
#ifdef RDKB_BUILD
	if(syscfg_set(NULL, "WebConfigRfcEnabled", buf) != 0)
	{
		WebConfigLog("syscfg_set failed for RfcEnable\n");
		return 1;
	}
	else
	{
		if (syscfg_commit() != 0)
		{
			WebConfigLog("syscfg_commit failed\n");
			return 1;
		}
		else
		{
			pMyObject->RfcEnable = bValue;
			return 0;
		}
	}
#endif
	return 0;
}

int Get_Webconfig_URL( char *pString)
{
    PCOSA_DATAMODEL_WEBCONFIG            pMyObject           = (PCOSA_DATAMODEL_WEBCONFIG)g_pCosaBEManager->hWebConfig;
    WebConfigLog("-------- %s ----- Enter-- ---\n",__FUNCTION__);

        if((pMyObject != NULL) && (pMyObject->URL != NULL) && (strlen(pMyObject->URL)>0))
        {
        		WebConfigLog("pMyObject->URL %s\n", pMyObject->URL);
                WebConfigLog("%s ----- updating pString ------\n",__FUNCTION__);
		
		AnscCopyString( pString,pMyObject->URL );
		WebConfigLog("pString %s\n",pString);
        }
        else
        {
		char *tempDBUrl = NULL;
                int   retPsmGet    = CCSP_SUCCESS;
		WebConfigLog("B4 PSM_Get_Record_Value2\n");
                retPsmGet = PSM_Get_Record_Value2(bus_handle,g_Subsystem, WEBCONFIG_PARAM_URL, NULL, &tempDBUrl);
		WebConfigLog("After PSM_Get_Record_Value2\n");
		WebConfigLog("tempDBUrl is %s\n", tempDBUrl);
                if (retPsmGet == CCSP_SUCCESS)
                {
			WebConfigLog("retPsmGet success\n");
			AnscCopyString( pString,tempDBUrl);
                        WebConfigLog("pString %s\n",pString);
                }
                else
                {
                        WebConfigLog("psm_get failed ret %d for parameter %s\n", retPsmGet, WEBCONFIG_PARAM_URL);
                        return 0;
                }
        }

        WebConfigLog("-------- %s ----- Exit ------\n",__FUNCTION__);
        return 1;
}

int Set_Webconfig_URL( char *pString)
{
    PCOSA_DATAMODEL_WEBCONFIG            pMyObject           = (PCOSA_DATAMODEL_WEBCONFIG)g_pCosaBEManager->hWebConfig;
    WebConfigLog("-------- %s ----- Enter ------\n",__FUNCTION__);
    int retPsmSet = CCSP_SUCCESS;

        memset( pMyObject->URL, 0, sizeof( pMyObject->URL ));
        AnscCopyString( pMyObject->URL, pString );


        retPsmSet = PSM_Set_Record_Value2(bus_handle,g_Subsystem, WEBCONFIG_PARAM_URL, ccsp_string, pString);
        if (retPsmSet != CCSP_SUCCESS)
        {
                WebConfigLog("psm_set failed ret %d for parameter %s and value %s\n", retPsmSet, WEBCONFIG_PARAM_URL, pString);
                return 0;
        }
        else
        {
                WebConfigLog("psm_set success ret %d for parameter %s and value %s\n", retPsmSet, WEBCONFIG_PARAM_URL, pString);
        }

        WebConfigLog("-------- %s ----- Exit ------\n",__FUNCTION__);
        return 1;
}


int getConfigNumberOfEntries()
{
	PCOSA_DATAMODEL_WEBCONFIG            pMyObject           = (PCOSA_DATAMODEL_WEBCONFIG)g_pCosaBEManager->hWebConfig;
	WebcfgDebug("----------- %s --------- Enter -----\n",__FUNCTION__);
	int count = AnscSListQueryDepth( &pMyObject->ConfigFileList );
	WebcfgDebug("-------- %s ----- Exit ------\n",__FUNCTION__);
	return count;
}

int Get_PeriodicSyncCheckInterval()
{
	PCOSA_DATAMODEL_WEBCONFIG pMyObject = (PCOSA_DATAMODEL_WEBCONFIG)g_pCosaBEManager->hWebConfig;
	return pMyObject->PeriodicSyncCheckInterval; 
}

int setPeriodicSyncCheckInterval(int iValue)
{
	PCOSA_DATAMODEL_WEBCONFIG            pMyObject           = (PCOSA_DATAMODEL_WEBCONFIG)g_pCosaBEManager->hWebConfig;
	char buf[16] = {0};
	sprintf(buf, "%d", iValue);
#ifdef RDKB_BUILD
	if(syscfg_set( NULL, "PeriodicSyncCheckInterval", buf) != 0)
	{
		WebConfigLog("syscfg_set failed\n");
		return 1;
	}
	else 
	{
		if (syscfg_commit() != 0)
		{
			WebConfigLog("syscfg_commit failed\n");
			return 1;
		}
		else
		{
			pMyObject->PeriodicSyncCheckInterval = iValue;
			/* sending signal to initWebConfigMultipartTask to update the sync time interval*/
			pthread_mutex_lock (get_global_periodicsync_mutex());
			pthread_cond_signal(get_global_periodicsync_condition());
			pthread_mutex_unlock(get_global_periodicsync_mutex());
		}
	}
#endif
	return 0;
}

int setForceSync(char* pString, char *transactionId,int *pStatus)
{
	PCOSA_DATAMODEL_WEBCONFIG            pMyObject           = (PCOSA_DATAMODEL_WEBCONFIG)g_pCosaBEManager->hWebConfig;
	WebConfigLog("setForceSync\n");
	memset( pMyObject->ForceSync, 0, sizeof( pMyObject->ForceSync ));
	AnscCopyString( pMyObject->ForceSync, pString );
	WebConfigLog("pMyObject->ForceSync is %s\n", pMyObject->ForceSync);

	if((pMyObject->ForceSync !=NULL) && (strlen(pMyObject->ForceSync)>0))
	{
		if(strlen(pMyObject->ForceSyncTransID)>0)
		{
			WebConfigLog("Force sync is already in progress, Ignoring this request.\n");
			*pStatus = 1;
			return 0;
		}
		else
		{
			/* sending signal to initWebConfigMultipartTask to update the sync time interval*/
			pthread_mutex_lock (get_global_periodicsync_mutex());

			//Update ForceSyncTransID to access webpa transactionId in webConfig sync.
			if(transactionId !=NULL && (strlen(transactionId)>0))
			{
				AnscCopyString(pMyObject->ForceSyncTransID, transactionId);
				WebConfigLog("pMyObject->ForceSyncTransID is %s\n", pMyObject->ForceSyncTransID);
			}
			WebConfigLog("Trigger force sync\n");
			pthread_cond_signal(get_global_periodicsync_condition());
			pthread_mutex_unlock(get_global_periodicsync_mutex());
		}
	}
	else
	{
		WebConfigLog("Force sync param set with empty value\n");
		memset(pMyObject->ForceSyncTransID,0,sizeof(pMyObject->ForceSyncTransID));
	}
	WebConfigLog("setForceSync returns success 1\n");
	return 1;
}

int getForceSync(char** pString, char **transactionId )
{
	PCOSA_DATAMODEL_WEBCONFIG pMyObject = (PCOSA_DATAMODEL_WEBCONFIG)g_pCosaBEManager->hWebConfig;
	WebConfigLog("-------- %s ----- Enter ------\n",__FUNCTION__);

	if((pMyObject->ForceSync != NULL) && strlen(pMyObject->ForceSync)>0)
	{
		WebConfigLog("%s ----- updating pString ------\n",__FUNCTION__);
		*pString = strdup(pMyObject->ForceSync);
		WebConfigLog("%s ----- updating transactionId ------\n",__FUNCTION__);
		*transactionId = strdup(pMyObject->ForceSyncTransID);
	}
	else
	{
		*pString = NULL;
		*transactionId = NULL;
		return 0;
	}
	WebConfigLog("*transactionId is %s\n",*transactionId);
	WebConfigLog("-------- %s ----- Exit ------\n",__FUNCTION__);
	return 1;
}

int getInstanceNumberAtIndex(int index)
{
    PCOSA_DATAMODEL_WEBCONFIG            pMyObject           = (PCOSA_DATAMODEL_WEBCONFIG)g_pCosaBEManager->hWebConfig;
    PSINGLE_LINK_ENTRY              pSLinkEntry       = (PSINGLE_LINK_ENTRY       )NULL;
    PCOSA_CONTEXT_LINK_OBJECT       pCosaContextEntry = (PCOSA_CONTEXT_LINK_OBJECT)NULL;
    int count , i;
    WebcfgDebug("----------- %s --------- Enter -----\n",__FUNCTION__);
    WebcfgDebug("index: %d\n",index);
    count = getConfigNumberOfEntries();
    WebcfgDebug("count: %d\n",count);
    pSLinkEntry = AnscSListGetFirstEntry(&pMyObject->ConfigFileList);
    for(i = 0; i<count; i++)
    {
        pCosaContextEntry = ACCESS_COSA_CONTEXT_LINK_OBJECT(pSLinkEntry);
        pSLinkEntry       = AnscSListGetNextEntry(pSLinkEntry);
        if(index == i)
        {
            WebcfgDebug("InstanceNumber at %d is: %d\n",i, pCosaContextEntry->InstanceNumber);
            return pCosaContextEntry->InstanceNumber;
        }
    }
    WebcfgDebug("-------- %s ----- Exit ------\n",__FUNCTION__);
    return 0;
}
BOOL isValidInstanceNumber(int instNum)
{
    PCOSA_DATAMODEL_WEBCONFIG            pMyObject           = (PCOSA_DATAMODEL_WEBCONFIG)g_pCosaBEManager->hWebConfig;
    PSINGLE_LINK_ENTRY              pSLinkEntry       = (PSINGLE_LINK_ENTRY       )NULL;
    PCOSA_CONTEXT_LINK_OBJECT       pCosaContextEntry = (PCOSA_CONTEXT_LINK_OBJECT)NULL;
    int count , i;
    WebcfgDebug("----------- %s --------- Enter -----\n",__FUNCTION__);
    count = getConfigNumberOfEntries();
    WebcfgDebug("count: %d\n",count);
    pSLinkEntry = AnscSListGetFirstEntry(&pMyObject->ConfigFileList);
    for(i = 0; i<count; i++)
    {
        pCosaContextEntry = ACCESS_COSA_CONTEXT_LINK_OBJECT(pSLinkEntry);
        pSLinkEntry       = AnscSListGetNextEntry(pSLinkEntry);
        if(pCosaContextEntry->InstanceNumber == instNum)
        {
            WebcfgDebug("%d instanceNumber is valid\n", instNum);
            return TRUE;
        }
    }
    WebcfgDebug("-------- %s ----- Exit ------\n",__FUNCTION__);
    return FALSE;
}

BOOL getConfigURLFromWebConfigCtx(ANSC_HANDLE hInsContext, char *pValue)
{
	PCOSA_CONTEXT_WEBCONFIG_LINK_OBJECT pWebConfigCxtLink = (PCOSA_CONTEXT_WEBCONFIG_LINK_OBJECT)hInsContext;
	PCOSA_DML_WEBCONFIG_CONFIGFILE_ENTRY pConfigFileEntry = (PCOSA_DML_WEBCONFIG_CONFIGFILE_ENTRY)pWebConfigCxtLink->hContext;
	if(pConfigFileEntry)
	{
		AnscCopyString(pValue, pConfigFileEntry->URL);
		return TRUE;
	}
	return FALSE;
}

BOOL getConfigURL(int index,char **configURL)
{
	PCOSA_DATAMODEL_WEBCONFIG                   pMyObject         = (PCOSA_DATAMODEL_WEBCONFIG)g_pCosaBEManager->hWebConfig;
	PCOSA_CONTEXT_WEBCONFIG_LINK_OBJECT    pCxtLink          = NULL;
	char pValue[256] = {'\0'};
	WebcfgDebug("-------- %s ----- Enter ------\n",__FUNCTION__);

	pCxtLink = CosaSListGetEntryByInsNum(&pMyObject->ConfigFileList, index);
	if(pCxtLink)
	{
		if(getConfigURLFromWebConfigCtx(pCxtLink, pValue))
		{
			*configURL = strdup(pValue);
		}
		else
		{
			*configURL = NULL;
			return FALSE;
		}
	}
	else
	{
		WebConfigLog("Table with %d index is not available\n", index);
		return FALSE;
	}

	WebcfgDebug("-------- %s ----- Exit ------\n",__FUNCTION__);
	return TRUE;
}

BOOL setConfigURLWithWebConfigCtx(ANSC_HANDLE hInsContext, char *pValue)
{
	PCOSA_CONTEXT_WEBCONFIG_LINK_OBJECT pWebConfigCxtLink = (PCOSA_CONTEXT_WEBCONFIG_LINK_OBJECT)hInsContext;
	PCOSA_DML_WEBCONFIG_CONFIGFILE_ENTRY pConfigFileEntry = (PCOSA_DML_WEBCONFIG_CONFIGFILE_ENTRY)pWebConfigCxtLink->hContext;
	if(pConfigFileEntry)
	{
		AnscCopyString(pConfigFileEntry->URL, pValue);
		return TRUE;
	}
	return FALSE;
}

int setConfigURL(int index, char *configURL)
{
	PCOSA_DATAMODEL_WEBCONFIG                   pMyObject         = (PCOSA_DATAMODEL_WEBCONFIG)g_pCosaBEManager->hWebConfig;
	PCOSA_CONTEXT_WEBCONFIG_LINK_OBJECT    pCxtLink          = NULL;
	char ParamName[MAX_BUFF_SIZE] = { 0 };
	WebcfgDebug("-------- %s ----- Enter ------\n",__FUNCTION__);

	pCxtLink = CosaSListGetEntryByInsNum(&pMyObject->ConfigFileList, index);
	if(pCxtLink)
	{
		if(setConfigURLWithWebConfigCtx(pCxtLink, configURL))
		{
			snprintf(ParamName,MAX_BUFF_SIZE, "configfile_%d_Url", index);
			CosaDmlStoreValueIntoDb(ParamName, configURL);
		}
		else
		{
			return 1;
		}
	}
	else
	{
		WebConfigLog("Table with %d index is not available\n", index);
		return 1;
	}

	WebcfgDebug("-------- %s ----- Exit ------\n",__FUNCTION__);
	return 0;
}

BOOL getRequestTimeStampFromWebConfigCtx(ANSC_HANDLE hInsContext, char *pValue)
{
	PCOSA_CONTEXT_WEBCONFIG_LINK_OBJECT pWebConfigCxtLink = (PCOSA_CONTEXT_WEBCONFIG_LINK_OBJECT)hInsContext;
	PCOSA_DML_WEBCONFIG_CONFIGFILE_ENTRY pConfigFileEntry = (PCOSA_DML_WEBCONFIG_CONFIGFILE_ENTRY)pWebConfigCxtLink->hContext;
	if(pConfigFileEntry)
	{
		AnscCopyString(pValue, pConfigFileEntry->RequestTimeStamp);
		return TRUE;
	}
	return FALSE;
}

BOOL getRequestTimeStamp(int index,char **RequestTimeStamp)
{
	PCOSA_DATAMODEL_WEBCONFIG                   pMyObject         = (PCOSA_DATAMODEL_WEBCONFIG)g_pCosaBEManager->hWebConfig;
	PCOSA_CONTEXT_WEBCONFIG_LINK_OBJECT    pCxtLink          = NULL;
	char pValue[256] = {'\0'};
	WebcfgDebug("-------- %s ----- Enter ------\n",__FUNCTION__);

	pCxtLink = CosaSListGetEntryByInsNum(&pMyObject->ConfigFileList, index);
	if(pCxtLink)
	{
		if(getRequestTimeStampFromWebConfigCtx(pCxtLink, pValue))
		{
			*RequestTimeStamp = strdup(pValue);
		}
		else
		{
			*RequestTimeStamp = NULL;
			return FALSE;
		}
	}
	else
	{
		WebConfigLog("Table with %d index is not available\n", index);
		return FALSE;
	}

	WebcfgDebug("-------- %s ----- Exit ------\n",__FUNCTION__);
	return TRUE;
}

BOOL setRequestTimeStampWithWebConfigCtx(ANSC_HANDLE hInsContext, char *pValue)
{
	PCOSA_CONTEXT_WEBCONFIG_LINK_OBJECT pWebConfigCxtLink = (PCOSA_CONTEXT_WEBCONFIG_LINK_OBJECT)hInsContext;
	PCOSA_DML_WEBCONFIG_CONFIGFILE_ENTRY pConfigFileEntry = (PCOSA_DML_WEBCONFIG_CONFIGFILE_ENTRY)pWebConfigCxtLink->hContext;
	if(pConfigFileEntry)
	{
		AnscCopyString(pConfigFileEntry->RequestTimeStamp, pValue);
		return TRUE;
	}
	return FALSE;
}

int setRequestTimeStamp(int index)
{
	PCOSA_DATAMODEL_WEBCONFIG                   pMyObject         = (PCOSA_DATAMODEL_WEBCONFIG)g_pCosaBEManager->hWebConfig;
	PCOSA_CONTEXT_WEBCONFIG_LINK_OBJECT    pCxtLink          = NULL;
	char ParamName[MAX_BUFF_SIZE] = { 0 };
	char str[MAX_BUFF_SIZE]={0};
	WebcfgDebug("-------- %s ----- Enter ------\n",__FUNCTION__);

	pCxtLink = CosaSListGetEntryByInsNum(&pMyObject->ConfigFileList, index);
	if(pCxtLink)
	{
		snprintf(str,sizeof(str),"%ld",(unsigned long)time(NULL));
		if(setRequestTimeStampWithWebConfigCtx(pCxtLink, str))
		{
			snprintf(ParamName,MAX_BUFF_SIZE, "configfile_%d_RequestTimeStamp", index);
			CosaDmlStoreValueIntoDb(ParamName, str);
		}
		else
		{
			return 1;
		}
	}
	else
	{
		WebConfigLog("Table with %d index is not available\n", index);
		return 1;
	}
	WebcfgDebug("-------- %s ----- Exit ------\n",__FUNCTION__);
	return 0;
}

BOOL getConfigVersionFromWebConfigCtx(ANSC_HANDLE hInsContext, char *pValue)
{
	PCOSA_CONTEXT_WEBCONFIG_LINK_OBJECT pWebConfigCxtLink = (PCOSA_CONTEXT_WEBCONFIG_LINK_OBJECT)hInsContext;
	PCOSA_DML_WEBCONFIG_CONFIGFILE_ENTRY pConfigFileEntry = (PCOSA_DML_WEBCONFIG_CONFIGFILE_ENTRY)pWebConfigCxtLink->hContext;
	if(pConfigFileEntry)
	{
		AnscCopyString(pValue, pConfigFileEntry->Version);
		return TRUE;
	}
	return FALSE;
}

BOOL getConfigVersion(int index, char **version)
{
	PCOSA_DATAMODEL_WEBCONFIG                   pMyObject         = (PCOSA_DATAMODEL_WEBCONFIG)g_pCosaBEManager->hWebConfig;
	PCOSA_CONTEXT_WEBCONFIG_LINK_OBJECT    pCxtLink          = NULL;
	char pValue[256] = {'\0'};
	WebcfgDebug("-------- %s ----- Enter ------\n",__FUNCTION__);
	pCxtLink = CosaSListGetEntryByInsNum(&pMyObject->ConfigFileList, index);
	if(pCxtLink)
	{
		if(getConfigVersionFromWebConfigCtx(pCxtLink, pValue))
		{
			*version = strdup(pValue);
		}
		else
		{
			*version = NULL;
			return FALSE;
		}
	}
	else
	{
		WebConfigLog("Table with %d index is not available\n", index);
		return FALSE;
	}
	WebcfgDebug("-------- %s ----- Exit ------\n",__FUNCTION__);
	return TRUE;
}

BOOL setConfigVersionWithWebConfigCtx(ANSC_HANDLE hInsContext, char *pValue)
{
	PCOSA_CONTEXT_WEBCONFIG_LINK_OBJECT pWebConfigCxtLink = (PCOSA_CONTEXT_WEBCONFIG_LINK_OBJECT)hInsContext;
	PCOSA_DML_WEBCONFIG_CONFIGFILE_ENTRY pConfigFileEntry = (PCOSA_DML_WEBCONFIG_CONFIGFILE_ENTRY)pWebConfigCxtLink->hContext;
	if(pConfigFileEntry)
	{
		AnscCopyString(pConfigFileEntry->Version, pValue);
		return TRUE;
	}
	return FALSE;
}

int setConfigVersion(int index, char *version)
{
	PCOSA_DATAMODEL_WEBCONFIG                   pMyObject         = (PCOSA_DATAMODEL_WEBCONFIG)g_pCosaBEManager->hWebConfig;
	PCOSA_CONTEXT_WEBCONFIG_LINK_OBJECT    pCxtLink          = NULL;
	char ParamName[MAX_BUFF_SIZE] = { 0 };
	WebcfgDebug("-------- %s ----- Enter ------\n",__FUNCTION__);

	pCxtLink = CosaSListGetEntryByInsNum(&pMyObject->ConfigFileList, index);
	if(pCxtLink)
	{
		if(setConfigVersionWithWebConfigCtx(pCxtLink, version))
		{
			snprintf(ParamName,MAX_BUFF_SIZE, "configfile_%d_Version", index);
			CosaDmlStoreValueIntoDb(ParamName, version);
		}
		else
		{
			return 1;
		}
	}
	else
	{
		WebConfigLog("Table with %d index is not available\n", index);
		return 1;
	}
	WebcfgDebug("-------- %s ----- Exit ------\n",__FUNCTION__);
	return 0;
}

BOOL getSyncCheckOKFromWebConfigCtx(ANSC_HANDLE hInsContext, BOOL *pBool )
{
	PCOSA_CONTEXT_WEBCONFIG_LINK_OBJECT   pWebConfigCxtLink     = (PCOSA_CONTEXT_WEBCONFIG_LINK_OBJECT)hInsContext;
	PCOSA_DML_WEBCONFIG_CONFIGFILE_ENTRY pConfigFileEntry  = (PCOSA_DML_WEBCONFIG_CONFIGFILE_ENTRY)pWebConfigCxtLink->hContext;
	WebcfgDebug("------- %s ----- ENTER ----\n",__FUNCTION__);
	if(pConfigFileEntry)
	{
		*pBool = pConfigFileEntry->SyncCheckOK;
		return TRUE;
	}
	WebcfgDebug("------- %s ----- EXIT ----\n",__FUNCTION__);
	return FALSE;
}

BOOL getSyncCheckOK(int index,BOOL *pvalue )
{
	PCOSA_DATAMODEL_WEBCONFIG                   pMyObject         = (PCOSA_DATAMODEL_WEBCONFIG)g_pCosaBEManager->hWebConfig;
	PCOSA_CONTEXT_WEBCONFIG_LINK_OBJECT    pCtxLink          = NULL;
	WebcfgDebug("-------- %s ----- Enter ------\n",__FUNCTION__);
	pCtxLink = CosaSListGetEntryByInsNum(&pMyObject->ConfigFileList, index);
	if(pCtxLink)
	{
		if(!getSyncCheckOKFromWebConfigCtx(pCtxLink, pvalue))
		{
			return FALSE;
		}
	}
	else
	{
		WebConfigLog("Table with %d index is not available\n", index);
		return FALSE;
	}
	WebcfgDebug("-------- %s ----- Exit ------\n",__FUNCTION__);
	return TRUE;
}

BOOL setSyncCheckOKWithWebConfigCtx(ANSC_HANDLE hInsContext, BOOL status)
{
	PCOSA_CONTEXT_WEBCONFIG_LINK_OBJECT pWebConfigCxtLink = (PCOSA_CONTEXT_WEBCONFIG_LINK_OBJECT)hInsContext;
	PCOSA_DML_WEBCONFIG_CONFIGFILE_ENTRY pConfigFileEntry = (PCOSA_DML_WEBCONFIG_CONFIGFILE_ENTRY)pWebConfigCxtLink->hContext;
	if(pConfigFileEntry)
	{
		pConfigFileEntry->SyncCheckOK = status;
		return TRUE;
	}
	return FALSE;
}

int setSyncCheckOK(int index, BOOL status)
{
	PCOSA_DATAMODEL_WEBCONFIG                   pMyObject         = (PCOSA_DATAMODEL_WEBCONFIG)g_pCosaBEManager->hWebConfig;
	PCOSA_CONTEXT_WEBCONFIG_LINK_OBJECT    pCtxLink          = NULL;
	char ParamName[MAX_BUFF_SIZE] = { 0 };
	WebcfgDebug("-------- %s ----- Enter ------\n",__FUNCTION__);
	pCtxLink = CosaSListGetEntryByInsNum(&pMyObject->ConfigFileList, index);
	if(pCtxLink)
	{
		if(!setSyncCheckOKWithWebConfigCtx(pCtxLink, status))
		{
			return 1;
		}
		snprintf(ParamName,MAX_BUFF_SIZE, "configfile_%d_SyncCheckOk", index);
		if(status == true)
		{
			CosaDmlStoreValueIntoDb(ParamName, "true");
		}
		else
		{
			CosaDmlStoreValueIntoDb(ParamName, "false");
		}
	}
	else
	{
		WebConfigLog("Table with %d index is not available\n", index);
		return 0;
	}
	WebcfgDebug("-------- %s ----- Exit ------\n",__FUNCTION__);
	return 0;
}

BOOL getForceSyncCheckFromWebConfigCtx(ANSC_HANDLE hInsContext, BOOL *pBool, char *pTransValue )
{
	PCOSA_CONTEXT_WEBCONFIG_LINK_OBJECT   pWebConfigCxtLink     = (PCOSA_CONTEXT_WEBCONFIG_LINK_OBJECT)hInsContext;
	PCOSA_DML_WEBCONFIG_CONFIGFILE_ENTRY pConfigFileEntry  = (PCOSA_DML_WEBCONFIG_CONFIGFILE_ENTRY)pWebConfigCxtLink->hContext;
	WebcfgDebug("------- %s ----- ENTER ----\n",__FUNCTION__);
	if(pConfigFileEntry)
	{
		*pBool = pConfigFileEntry->ForceSyncCheck;
		AnscCopyString(pTransValue, pConfigFileEntry->ForceSyncTransID);
		return TRUE;
	}
	WebcfgDebug("------- %s ----- EXIT ----\n",__FUNCTION__);
	return FALSE;
}

BOOL getForceSyncCheck(int index,BOOL *pvalue, char** transactionID )
{
	PCOSA_DATAMODEL_WEBCONFIG                   pMyObject         = (PCOSA_DATAMODEL_WEBCONFIG)g_pCosaBEManager->hWebConfig;
	PCOSA_CONTEXT_WEBCONFIG_LINK_OBJECT    pCtxLink          = NULL;
	char pTransValue[256] = {'\0'};
	WebcfgDebug("-------- %s ----- Enter ------\n",__FUNCTION__);
	pCtxLink = CosaSListGetEntryByInsNum(&pMyObject->ConfigFileList, index);
	if(pCtxLink)
	{
		if(!getForceSyncCheckFromWebConfigCtx(pCtxLink, pvalue, pTransValue))
		{
			*transactionID = NULL;
			return FALSE;
		}
		else
		{
			*transactionID = strdup(pTransValue);
		}
	}
	else
	{
		WebConfigLog("Table with %d index is not available\n", index);
		return FALSE;
	}
	WebcfgDebug("-------- %s ----- Exit ------\n",__FUNCTION__);
	return TRUE;
}

BOOL setForceSyncCheckWithWebConfigCtx(ANSC_HANDLE hInsContext, BOOL bValue, char *transactionId, int *pStatus)
{
	PCOSA_CONTEXT_WEBCONFIG_LINK_OBJECT   pWebConfigCxtLink     = (PCOSA_CONTEXT_WEBCONFIG_LINK_OBJECT)hInsContext;
	PCOSA_DML_WEBCONFIG_CONFIGFILE_ENTRY pConfigFileEntry  = (PCOSA_DML_WEBCONFIG_CONFIGFILE_ENTRY)pWebConfigCxtLink->hContext;
	WebcfgDebug("------- %s ----- ENTER ----\n",__FUNCTION__);
	if(pConfigFileEntry)
	{
		pConfigFileEntry->ForceSyncCheck = bValue;
		if(bValue)
		{
			if(strlen(pConfigFileEntry->ForceSyncTransID)>0)
			{
				WebConfigLog("Force sync is already in progress, Ignoring this request.\n");
				*pStatus = 1;
				return FALSE;
			}
			else
			{
				pthread_mutex_lock (get_global_periodicsync_mutex());
				//Update ForceSyncTransID to access webpa transactionId in webConfig sync.
				if(transactionId !=NULL && (strlen(transactionId)>0))
				{
					AnscCopyString(pConfigFileEntry->ForceSyncTransID, transactionId);
				}
				pthread_cond_signal(get_global_periodicsync_condition());
				pthread_mutex_unlock(get_global_periodicsync_mutex());
			}
		}
		else
		{
			memset(pConfigFileEntry->ForceSyncTransID,0,sizeof(pConfigFileEntry->ForceSyncTransID));
		}
		return TRUE;
	}
	WebcfgDebug("------- %s ----- EXIT ----\n",__FUNCTION__);
	return FALSE;
}

BOOL setForceSyncCheck(int index, BOOL pvalue, char *transactionId, int *session_status)
{
	PCOSA_DATAMODEL_WEBCONFIG                   pMyObject         = (PCOSA_DATAMODEL_WEBCONFIG)g_pCosaBEManager->hWebConfig;
	PCOSA_CONTEXT_WEBCONFIG_LINK_OBJECT    pCtxLink          = NULL;
	int pStatus = 0;
	WebcfgDebug("-------- %s ----- Enter ------\n",__FUNCTION__);
	pCtxLink = CosaSListGetEntryByInsNum(&pMyObject->ConfigFileList, index);
	if(pCtxLink)
	{
		if(!setForceSyncCheckWithWebConfigCtx(pCtxLink, pvalue, transactionId, &pStatus))
		{
			*session_status = pStatus;
			return FALSE;
		}
	}
	else
	{
		WebConfigLog("Table with %d index is not available\n", index);
		return FALSE;
	}
	WebcfgDebug("-------- %s ----- Exit ------\n",__FUNCTION__);
	return TRUE;
}

void updateParamValStructWIthConfigFileDataAtIndex(parameterValStruct_t **paramVal, int index, int valIndex, int *finalIndex)
{
	int ret = 0;
	char *valueStr = NULL;
	BOOL bValue = false;
	WebcfgDebug("--------- %s ------ ENter -----\n",__FUNCTION__);
	paramVal[valIndex] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t));
	memset(paramVal[valIndex], 0, sizeof(parameterValStruct_t));
	paramVal[valIndex]->parameterName = (char *)malloc(sizeof(char)*MAX_PARAMETERNAME_LEN);
	snprintf(paramVal[valIndex]->parameterName, MAX_PARAMETERNAME_LEN, "%s%d.%s",WEBCONFIG_TABLE_CONFIGFILE, index, CONFIGFILE_PARAM_URL);
	ret = getConfigURL(index, &valueStr); 
	if(ret)
	{
		WebcfgDebug("valueStr: %s\n",valueStr);
		paramVal[valIndex]->parameterValue = strndup(valueStr,MAX_PARAMETERVALUE_LEN);
		WAL_FREE(valueStr);
	}
	else
	{
		paramVal[valIndex]->parameterValue = strndup("",MAX_PARAMETERVALUE_LEN);
	}
	paramVal[valIndex]->type = ccsp_string;
	valIndex++;
	paramVal[valIndex] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t));
	memset(paramVal[valIndex], 0, sizeof(parameterValStruct_t));
	paramVal[valIndex]->parameterName = (char *)malloc(sizeof(char)*MAX_PARAMETERNAME_LEN);
	snprintf(paramVal[valIndex]->parameterName, MAX_PARAMETERNAME_LEN, "%s%d.%s",WEBCONFIG_TABLE_CONFIGFILE, index, CONFIGFILE_PARAM_VERSION);
	ret = getConfigVersion(index, &valueStr); 
	if(ret)
	{
		WebcfgDebug("valueStr: %s\n",valueStr);
		paramVal[valIndex]->parameterValue = strndup(valueStr,MAX_PARAMETERVALUE_LEN);
		WAL_FREE(valueStr);
	}
	else
	{
		paramVal[valIndex]->parameterValue = strndup("",MAX_PARAMETERVALUE_LEN);
	}
	paramVal[valIndex]->type = ccsp_string;
	valIndex++;
	paramVal[valIndex] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t));
	memset(paramVal[valIndex], 0, sizeof(parameterValStruct_t));
	paramVal[valIndex]->parameterName = (char *)malloc(sizeof(char)*MAX_PARAMETERNAME_LEN);
	snprintf(paramVal[valIndex]->parameterName, MAX_PARAMETERNAME_LEN, "%s%d.%s",WEBCONFIG_TABLE_CONFIGFILE, index, CONFIGFILE_PARAM_FORCE_SYNC);
        paramVal[valIndex]->parameterValue = strndup("false",MAX_PARAMETERVALUE_LEN);
	paramVal[valIndex]->type = ccsp_boolean;
	valIndex++;
	paramVal[valIndex] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t));
	memset(paramVal[valIndex], 0, sizeof(parameterValStruct_t));
	paramVal[valIndex]->parameterName = (char *)malloc(sizeof(char)*MAX_PARAMETERNAME_LEN);
	snprintf(paramVal[valIndex]->parameterName, MAX_PARAMETERNAME_LEN, "%s%d.%s",WEBCONFIG_TABLE_CONFIGFILE, index, CONFIGFILE_PARAM_SYNC_CHECK_OK);
	getSyncCheckOK(index,&bValue);
    WebcfgDebug("SyncCheckOK is %d\n",bValue);
    if(bValue == true)
    {
        paramVal[valIndex]->parameterValue = strndup("true",MAX_PARAMETERVALUE_LEN);
    }
    else
    {
        paramVal[valIndex]->parameterValue = strndup("false",MAX_PARAMETERVALUE_LEN);
    }
	paramVal[valIndex]->type = ccsp_boolean;
	valIndex++;
	paramVal[valIndex] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t));
	memset(paramVal[valIndex], 0, sizeof(parameterValStruct_t));
	paramVal[valIndex]->parameterName = (char *)malloc(sizeof(char)*MAX_PARAMETERNAME_LEN);
	snprintf(paramVal[valIndex]->parameterName, MAX_PARAMETERNAME_LEN, "%s%d.%s",WEBCONFIG_TABLE_CONFIGFILE, index, CONFIGFILE_PARAM_REQUEST_TIME_STAMP);
	ret = getRequestTimeStamp(index, &valueStr);
	if(ret)
	{
		WebcfgDebug("valueStr: %s\n",valueStr);
		paramVal[valIndex]->parameterValue = strndup(valueStr,MAX_PARAMETERVALUE_LEN);
		WAL_FREE(valueStr);
	}
	else
	{
		paramVal[valIndex]->parameterValue = strndup("",MAX_PARAMETERVALUE_LEN);
	}
	paramVal[valIndex]->type = ccsp_string;
	valIndex++;
	*finalIndex = valIndex;
	WebcfgDebug("*finalIndex: %d\n",*finalIndex);
	WebcfgDebug("--------- %s ------ EXIT -----\n",__FUNCTION__);
}

int getWebConfigParameterValues(char **parameterNames, int paramCount, int *val_size, parameterValStruct_t ***val)
{
    char *objects[] ={"Device.X_RDK_WebConfig.ConfigFile.","Device.X_RDK_WebConfig."};
    int objSize = sizeof(objects)/sizeof(objects[0]);
    parameterValStruct_t **paramVal = NULL;
    paramVal = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t *)*paramCount);
    int i=0, j=0, k=0, isWildcard = 0, matchFound = 0, count = 0;
    int localCount = paramCount;
    BOOL RFC_ENABLE;
    WebcfgDebug("*********** %s ***************\n",__FUNCTION__);

    RFC_ENABLE = Get_RfcEnable();
    WebConfigLog("paramCount = %d\n",paramCount);
    if(RFC_ENABLE)
    {
        count = getConfigNumberOfEntries();
    }
    WebcfgDebug("count: %d\n",count);
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
        for(j=0; j<objSize; j++)
        {
            if(strstr(parameterNames[i],objects[j]) != NULL)
            {
                matchFound = 1;
                switch(j)
                {
                    case 0:
                    {
                        if(!RFC_ENABLE)
                        {
                            matchFound = 0;
                            break;
                        }
                        if(isWildcard == 0)
                        {
                            char* instNumStart = NULL, *valueStr = NULL;
                            char restDmlString[128] = {'\0'};
                            int index = 0, ret = 0;
                            WebcfgDebug("parameterNames[%d]: %s\n",i,parameterNames[i]);
                            instNumStart = parameterNames[i]+strlen(WEBCONFIG_TABLE_CONFIGFILE);
                            WebcfgDebug("instNumStart: %s\n",instNumStart);
                            sscanf(instNumStart, "%d.%s", &index, restDmlString);
                            WebcfgDebug("index: %d restDmlString: %s\n",index,restDmlString);
                            
                            paramVal[k] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t));
                            memset(paramVal[k], 0, sizeof(parameterValStruct_t));
                            if(strcmp(restDmlString, CONFIGFILE_PARAM_URL) == 0)
                            {
								ret = getConfigURL(index, &valueStr);
								if(ret)
								{
									WebcfgDebug("valueStr: %s\n",valueStr);
									paramVal[k]->parameterName = strndup(parameterNames[i], MAX_PARAMETERNAME_LEN);
									paramVal[k]->parameterValue = strndup(valueStr,MAX_PARAMETERVALUE_LEN);
									paramVal[k]->type = ccsp_string;
                                    k++;
									WAL_FREE(valueStr);
								}
								else
								{
								    WAL_FREE(paramVal[k]);
									matchFound = 0;
								}
                            }
                            else if(strcmp(restDmlString, CONFIGFILE_PARAM_VERSION) == 0)
                            {
                                ret = getConfigVersion(index, &valueStr);
								if(ret)
								{
									WebcfgDebug("valueStr: %s\n",valueStr);
									paramVal[k]->parameterName = strndup(parameterNames[i], MAX_PARAMETERNAME_LEN);
									paramVal[k]->parameterValue = strndup(valueStr,MAX_PARAMETERVALUE_LEN);
									paramVal[k]->type = ccsp_string;
									k++;
									WAL_FREE(valueStr);
								}
								else
								{
								    WAL_FREE(paramVal[k]);
									matchFound = 0;
								}
                            }
                            else if(strcmp(restDmlString, CONFIGFILE_PARAM_FORCE_SYNC) == 0)
                            {
                                if(isValidInstanceNumber(index))
                                {
                                    paramVal[k]->parameterName = strndup(parameterNames[i], MAX_PARAMETERNAME_LEN);
                                    paramVal[k]->parameterValue = strndup("false",MAX_PARAMETERVALUE_LEN);
                                    paramVal[k]->type = ccsp_boolean;
                                    k++;
                                }
                                else
								{
								    WAL_FREE(paramVal[k]);
									matchFound = 0;
								}
                            }
                            else if(strcmp(restDmlString, CONFIGFILE_PARAM_SYNC_CHECK_OK) == 0)
                            {
                                BOOL bValue;
                                ret = getSyncCheckOK(index,&bValue);
                                if(ret)
                                {
                                    WebcfgDebug("SyncCheckOK is %d\n",bValue);
                                    paramVal[k]->parameterName = strndup(parameterNames[i], MAX_PARAMETERNAME_LEN);
                                    if(bValue == true)
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
                                else
								{
								    WAL_FREE(paramVal[k]);
									matchFound = 0;
								}
                            }
                            else if(strcmp(restDmlString, CONFIGFILE_PARAM_REQUEST_TIME_STAMP) == 0)
                            {
                                ret = getRequestTimeStamp(index, &valueStr);
								if(ret)
								{
									WebcfgDebug("valueStr: %s\n",valueStr);
									paramVal[k]->parameterName = strndup(parameterNames[i], MAX_PARAMETERNAME_LEN);
									paramVal[k]->parameterValue = strndup(valueStr,MAX_PARAMETERVALUE_LEN);
									paramVal[k]->type = ccsp_string;
                                    k++;
									WAL_FREE(valueStr);
								}
								else
								{
								    WAL_FREE(paramVal[k]);
									matchFound = 0;
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
                            int index = 0;
                            if(strcmp(parameterNames[i], WEBCONFIG_TABLE_CONFIGFILE) == 0)
                            {
                                if(count > 0)
                                {
                                    localCount = localCount+(count*5)-1;
                                    paramVal = (parameterValStruct_t **) realloc(paramVal, sizeof(parameterValStruct_t *)*localCount);
                                    int n = 0, index = 0;
                                    for(n = 0; n<count; n++)
                                    {
                                        index = getInstanceNumberAtIndex(n);
                                        WebcfgDebug("InstNum: %d\n",index);
                                        if(index != 0)
                                        {
                                            WebcfgDebug("B4 updateParamValStructWIthConfigFileDataAtIndex\n");
								            updateParamValStructWIthConfigFileDataAtIndex(paramVal, index, k, &k);
								            WebcfgDebug("k = %d\n",k);
							            }
							            else
							            {
							                matchFound = 0;
							            }
                                    }                    
                                }
                                else
                                {
                                    paramVal[k] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t));
                                    memset(paramVal[k], 0, sizeof(parameterValStruct_t));
                                    paramVal[k]->parameterName = strndup(WEBCONFIG_TABLE_CONFIGFILE, MAX_PARAMETERNAME_LEN);
                                    paramVal[k]->parameterValue = strndup("EMPTY",MAX_PARAMETERVALUE_LEN);
                                    k++;
                                }
                            }
                            else
                            {
                                char* instNumStart = NULL;
                                WebcfgDebug("parameterNames[%d]: %s\n",i, parameterNames[i]);
                                instNumStart = parameterNames[i]+strlen(WEBCONFIG_TABLE_CONFIGFILE);
                                WebcfgDebug("instNumStart: %s\n",instNumStart);
                                sscanf(instNumStart, "%d.", &index);
								WebcfgDebug("index: %d\n",index);
                                localCount = localCount+4;
                                if(isValidInstanceNumber(index) == TRUE)
                                {
                                    paramVal = (parameterValStruct_t **) realloc(paramVal, sizeof(parameterValStruct_t *)*localCount);
								    WebcfgDebug("B4 updateParamValStructWIthConfigFileDataAtIndex\n");
								    updateParamValStructWIthConfigFileDataAtIndex(paramVal, index, k, &k);
								    WebcfgDebug("k = %d\n",k);
								}
								else
								{
								    matchFound = 0;
								}
                            }
                        }
                        break;
                    }
					case 1:
                    {
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
                            else if((strcmp(parameterNames[i], WEBCONFIG_PARAM_CONFIGFILE_ENTRIES) == 0) && (RFC_ENABLE == true))
                            {
                                paramVal[k]->parameterName = strndup(WEBCONFIG_PARAM_CONFIGFILE_ENTRIES, MAX_PARAMETERNAME_LEN);
                                int count = getConfigNumberOfEntries();
                                WebcfgDebug("count is %d\n",count);
                                paramVal[k]->parameterValue = (char *)malloc(sizeof(char)*MAX_PARAMETERVALUE_LEN);
                                snprintf(paramVal[k]->parameterValue,MAX_PARAMETERVALUE_LEN,"%d",count);
                                paramVal[k]->type = ccsp_unsignedInt;
                                k++;
                            }
                            else if((strcmp(parameterNames[i], WEBCONFIG_PARAM_PERIODIC_INTERVAL) == 0) && (RFC_ENABLE == true))
                            {
								int interval = Get_PeriodicSyncCheckInterval();
                                paramVal[k]->parameterName = strndup(WEBCONFIG_PARAM_PERIODIC_INTERVAL, MAX_PARAMETERNAME_LEN);
                                WebcfgDebug("PeriodicSyncCheckInterval is %d\n",interval);
                                paramVal[k]->parameterValue = (char *)malloc(sizeof(char)*MAX_PARAMETERVALUE_LEN);
                                snprintf(paramVal[k]->parameterValue,MAX_PARAMETERVALUE_LEN,"%d",interval);
                                paramVal[k]->type = ccsp_int;
                                k++;
                            }
			    else if((strcmp(parameterNames[i], WEBCONFIG_PARAM_FORCE_SYNC) == 0) && (RFC_ENABLE == true))
                            {
				WebConfigLog("Force Sync GET is not supported\n");
                                paramVal[k]->parameterName = strndup(WEBCONFIG_PARAM_FORCE_SYNC, MAX_PARAMETERNAME_LEN);
				paramVal[k]->parameterValue = strndup("",MAX_PARAMETERVALUE_LEN);
                                paramVal[k]->type = ccsp_string;
                                k++;
				WebConfigLog("Webpa force sync done\n");
                            }
			    else if((strcmp(parameterNames[i], WEBCONFIG_PARAM_URL) == 0) && (RFC_ENABLE == true))
                            {
				char valuestr[256] = {0};
				WebConfigLog("B4 Get_Webconfig_URL\n");
				Get_Webconfig_URL(valuestr);
				if( (valuestr != NULL) && strlen(valuestr) >0 )
				{
					WebConfigLog("Webpa get : URL fetched %s\n", valuestr);
		                        paramVal[k]->parameterName = strndup(WEBCONFIG_PARAM_URL, MAX_PARAMETERNAME_LEN);
					paramVal[k]->parameterValue = strndup(valuestr,MAX_PARAMETERVALUE_LEN);
		                        paramVal[k]->type = ccsp_string;
		                        k++;
					WebConfigLog("Webpa get : URL done\n");
				}
				else
				{
					WebConfigLog("Webpa get : URL not found\n");
					WAL_FREE(paramVal[k]);
					matchFound = 0;
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
				WebConfigLog("Updating localCount %d in wildcard GET case\n", localCount);
                                localCount = localCount+4+(count*5);
                            }
				WebConfigLog("Updated localCount %d\n", localCount);
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
                                paramVal[k] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t));
                                memset(paramVal[k], 0, sizeof(parameterValStruct_t));
                                paramVal[k]->parameterName = strndup(WEBCONFIG_PARAM_CONFIGFILE_ENTRIES, MAX_PARAMETERNAME_LEN);
                                WebcfgDebug("paramVal[%d]->parameterName: %s\n",k,paramVal[k]->parameterName);
                                paramVal[k]->parameterValue = (char *)malloc(sizeof(char)*MAX_PARAMETERVALUE_LEN);
                                WebcfgDebug("count is %d\n",count);
                                snprintf(paramVal[k]->parameterValue,MAX_PARAMETERVALUE_LEN,"%d",count);
                                paramVal[k]->type = ccsp_unsignedInt;
                                k++;
                                paramVal[k] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t));
                                memset(paramVal[k], 0, sizeof(parameterValStruct_t));
                                paramVal[k]->parameterName = strndup(WEBCONFIG_PARAM_PERIODIC_INTERVAL, MAX_PARAMETERNAME_LEN);
								int interval = Get_PeriodicSyncCheckInterval();
                                WebcfgDebug("PeriodicSyncCheckInterval is %d\n",interval);
                                paramVal[k]->parameterValue = (char *)malloc(sizeof(char)*MAX_PARAMETERVALUE_LEN);
                                snprintf(paramVal[k]->parameterValue,MAX_PARAMETERVALUE_LEN,"%d",interval);
                                paramVal[k]->type = ccsp_int;
                                k++;

				WebConfigLog("Webpa wildcard get for force sync\n");
				paramVal[k] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t));
                                memset(paramVal[k], 0, sizeof(parameterValStruct_t));
                                paramVal[k]->parameterName = strndup(WEBCONFIG_PARAM_FORCE_SYNC, MAX_PARAMETERNAME_LEN);
				paramVal[k]->parameterValue = strndup("",MAX_PARAMETERVALUE_LEN);
                                paramVal[k]->type = ccsp_string;
                                k++;
				WebConfigLog("After Webpa wildcard get for force sync\n");

				WebConfigLog("Webpa wildcard get for URL\n");
				paramVal[k] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t));
                                memset(paramVal[k], 0, sizeof(parameterValStruct_t));
                                paramVal[k]->parameterName = strndup(WEBCONFIG_PARAM_URL, MAX_PARAMETERNAME_LEN);
				char webcfg_url[256] = {0};
				WebConfigLog("Wildcard get : B4 Get_Webconfig_URL\n");
				Get_Webconfig_URL(webcfg_url);
				if( (webcfg_url !=NULL) && strlen(webcfg_url)>0 )
				{
					WebConfigLog("webcfg_url fetched %s\n", webcfg_url);
					paramVal[k]->parameterValue = strndup(webcfg_url,MAX_PARAMETERVALUE_LEN);
					WebConfigLog("Wildcard get : paramVal[k]->parameterValue:%s\n", paramVal[k]->parameterValue);
				}
				else
				{
					paramVal[k]->parameterValue = strndup("",MAX_PARAMETERVALUE_LEN);
				}
                                paramVal[k]->type = ccsp_string;
                                k++;
				WebConfigLog("After Webpa wildcard get for URL\n");

                                int n = 0, index = 0;
                                for(n = 0; n<count; n++)
                                {
                                    index = getInstanceNumberAtIndex(n);
                                    WebConfigLog("InstNum: %d\n",index);
                                    if(index != 0)
                                    {
                                        WebcfgDebug("B4 updateParamValStructWIthConfigFileDataAtIndex\n");
					                    updateParamValStructWIthConfigFileDataAtIndex(paramVal, index, k, &k);
					                    WebcfgDebug("k = %d\n",k);
				                    }
				                    else
				                    {
				                        matchFound = 0;
				                    }
                                }
                            }
                        }
                        break;
                    }
                }
                break;
            }
        }
        if(matchFound == 0)
        {
            if(!RFC_ENABLE)
            {
                WebConfigLog("RFC disabled. Hence not proceeding with GET\n");
            }
            else
            {
                WebConfigLog("%s is invalid parameter\n",parameterNames[i]);
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
        WebConfigLog("Final-> %s %s %d\n",(*val)[i]->parameterName, (*val)[i]->parameterValue, (*val)[i]->type);
    }
    *val_size = k;
    WebConfigLog("Final count is %d\n",*val_size);
    WebConfigLog("*********** %s ***************\n",__FUNCTION__);
    return CCSP_SUCCESS;
}

int setWebConfigParameterValues(parameterValStruct_t *val, int paramCount, char **faultParam, char *transactionId )
{
	int i=0;
	char *subStr = NULL;
	BOOL RFC_ENABLE;
	int session_status = 0;
	int ret = 0;
	WebConfigLog("*********** %s ***************\n",__FUNCTION__);

	char *webConfigObject = "Device.X_RDK_WebConfig.";
	RFC_ENABLE = Get_RfcEnable();

	WebConfigLog("paramCount = %d\n",paramCount);
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
			else if((strcmp(val[i].parameterName, WEBCONFIG_PARAM_PERIODIC_INTERVAL) == 0) && (RFC_ENABLE == true))
			{
				subStr = val[i].parameterName+strlen(webConfigObject);
				CosaDmlStoreValueIntoDb( subStr, val[i].parameterValue );
				if(val[i].parameterValue != NULL)
				{
					setPeriodicSyncCheckInterval(atoi(val[i].parameterValue));
				}
				else
				{
					setPeriodicSyncCheckInterval(0);
				}
			}
			else if((strcmp(val[i].parameterName, WEBCONFIG_PARAM_FORCE_SYNC) == 0) && (RFC_ENABLE == true))
			{
				WebConfigLog("Processing Force Sync param\n");
				if((val[i].parameterValue !=NULL) && (strlen(val[i].parameterValue)>0))
				{
					WebConfigLog("setWebConfigParameterValues setForceSync\n");
					ret = setForceSync(val[i].parameterValue, transactionId, &session_status);
					WebConfigLog("After setForceSync ret %d\n", ret);
				}
				else //pass empty transaction id when Force sync is with empty doc
				{
					WebConfigLog("setWebConfigParameterValues empty setForceSync\n");
					ret = setForceSync(val[i].parameterValue, "", 0);
				}
				if(session_status)
				{
					return CCSP_CR_ERR_SESSION_IN_PROGRESS;
				}
				if(!ret)
				{
					WebConfigLog("setForceSync failed\n");
					return CCSP_FAILURE;
				}
			}
			else if((strcmp(val[i].parameterName, WEBCONFIG_PARAM_URL) == 0) && (RFC_ENABLE == true))
			{
				WebConfigLog("Processing Webcfg URL param\n");
				if(isValidUrl(val[i].parameterValue) == TRUE)
				{
					WebConfigLog("setWebConfigParameterValues Set_Webconfig_URL\n");
					ret = Set_Webconfig_URL(val[i].parameterValue);
					WebConfigLog("After Set_Webconfig_UR ret %d\n", ret);
					if(ret != 1)
					{
						WebConfigLog("Set_Webconfig_URL failed\n");
						return CCSP_FAILURE;
					}
				}
				else
				{
					WebConfigLog("Webcfg URL validation failed\n");
					return CCSP_FAILURE;
				}
			}
			else if((strstr(val[i].parameterName, WEBCONFIG_TABLE_CONFIGFILE) != NULL) && (RFC_ENABLE == true))
			{
				subStr = val[i].parameterName+strlen(WEBCONFIG_TABLE_CONFIGFILE);
				int index = 0, ret = 0;
				char dmlString[128] = {'\0'};
				sscanf(subStr, "%d.%s",&index, dmlString);
				WebcfgDebug("index: %d dmlString:%s\n",index, dmlString);
				if(strcmp(dmlString, CONFIGFILE_PARAM_URL) == 0)
				{
					if(isValidUrl(val[i].parameterValue) == TRUE)
					{
						ret = setConfigURL(index, val[i].parameterValue);
						if(ret != 0)
						{
							WebConfigLog("setConfigURL failed\n");
							return CCSP_FAILURE;
						}
					}
					else
					{
						WebConfigLog("URL validation failed\n");
						return CCSP_FAILURE;
					}
				}
				else if(strcmp(dmlString, CONFIGFILE_PARAM_FORCE_SYNC) == 0)
				{
					if(strcmp(val[i].parameterValue, "true") == 0)
					{
						ret = setForceSyncCheck(index, true, transactionId, &session_status);
					}
					else //pass empty transaction id when Force sync is false
					{
						ret = setForceSyncCheck(index, false, "", 0);
					}
					if(session_status)
					{
						return CCSP_CR_ERR_SESSION_IN_PROGRESS;
					}
					if(!ret)
					{
						WebConfigLog("setForceSyncCheck failed\n");
						return CCSP_FAILURE;
					}
				}
				else
				{
					WebConfigLog("%s is not writable\n",val[i].parameterName);
					*faultParam = strdup(val[i].parameterName);
					return CCSP_ERR_NOT_WRITABLE;
				}
			}
			else if(!RFC_ENABLE)
			{
				WebConfigLog("RFC disabled. Hence not proceeding with SET\n");
				return CCSP_ERR_INVALID_PARAMETER_VALUE;
			}
		}
		else
		{
			WebConfigLog("%s is not writable\n",val[i].parameterName);
			*faultParam = strdup(val[i].parameterName);
			return CCSP_ERR_NOT_WRITABLE;
		}
	}
	WebConfigLog("*********** %s ***************\n",__FUNCTION__);
	return CCSP_SUCCESS;
}
