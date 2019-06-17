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
#define WEBCONFIG_PARAM_CONFIGFILE_ENTRIES  "Device.X_RDK_WebConfig.ConfigFileNumberOfEntries"
#define WEBCONFIG_PARAM_PERIODIC_INTERVAL   "Device.X_RDK_WebConfig.PeriodicSyncCheckInterval"
#define WEBCONFIG_TABLE_CONFIGFILE          "Device.X_RDK_WebConfig.ConfigFile."
#define CONFIGFILE_PARAM_URL                "URL"
#define CONFIGFILE_PARAM_VERSION            "Version"
#define CONFIGFILE_PARAM_FORCE_SYNC         "ForceSyncCheck"
#define CONFIGFILE_PARAM_SYNC_CHECK_OK      "SyncCheckOK"
#define CONFIGFILE_PARAM_PREV_SYNC_TIME     "PreviousSyncDateTime"

extern PCOSA_BACKEND_MANAGER_OBJECT g_pCosaBEManager;

BOOL Get_RfcEnable()
{
    PCOSA_DATAMODEL_WEBCONFIG            pMyObject           = (PCOSA_DATAMODEL_WEBCONFIG)g_pCosaBEManager->hWebConfig;
    return pMyObject->RfcEnable;
}

int getConfigNumberOfEntries()
{
	PCOSA_DATAMODEL_WEBCONFIG            pMyObject           = (PCOSA_DATAMODEL_WEBCONFIG)g_pCosaBEManager->hWebConfig;
	WebcfgDebug("----------- %s --------- Enter -----\n",__FUNCTION__);
	int count = AnscSListQueryDepth( &pMyObject->ConfigFileList );
	WebcfgDebug("-------- %s ----- Exit ------\n",__FUNCTION__);
	return count;
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

BOOL getConfigURL(int index,char **configURL)
{
	PCOSA_DATAMODEL_WEBCONFIG                   pMyObject         = (PCOSA_DATAMODEL_WEBCONFIG)g_pCosaBEManager->hWebConfig;
        PSINGLE_LINK_ENTRY                    pSListEntry       = NULL;
        PCOSA_CONTEXT_WEBCONFIG_LINK_OBJECT    pCxtLink          = NULL;
        PCOSA_DML_WEBCONFIG_CONFIGFILE_ENTRY pConfigFileEntry    = NULL;
        int i, count, indexFound = 0;
	WebcfgDebug("-------- %s ----- Enter ------\n",__FUNCTION__);
	count = getConfigNumberOfEntries();
	WebcfgDebug("count : %d\n",count);

        for(i=0;i<count;i++)
        {
                pSListEntry       = AnscSListGetEntryByIndex(&pMyObject->ConfigFileList, i);
                if ( pSListEntry )
                {
                        pCxtLink      = ACCESS_COSA_CONTEXT_WEBCONFIG_LINK_OBJECT(pSListEntry);
                }
                pConfigFileEntry  = (PCOSA_DML_WEBCONFIG_CONFIGFILE_ENTRY)pCxtLink->hContext;
                if(pConfigFileEntry->InstanceNumber==index)
                {
                        *configURL = strdup(pConfigFileEntry->URL);
			indexFound = 1;
			break;
                }
        }
	if(indexFound == 0)
	{
		WebConfigLog("Table with %d index is not available\n", index);
		return FALSE;
	}
        
	WebcfgDebug("-------- %s ----- Exit ------\n",__FUNCTION__);
        return TRUE;
}

int setConfigURL(int index, char *configURL)
{
	PCOSA_DATAMODEL_WEBCONFIG                   pMyObject         = (PCOSA_DATAMODEL_WEBCONFIG)g_pCosaBEManager->hWebConfig;
        PSINGLE_LINK_ENTRY                    pSListEntry       = NULL;
        PCOSA_CONTEXT_WEBCONFIG_LINK_OBJECT    pCxtLink          = NULL;
        PCOSA_DML_WEBCONFIG_CONFIGFILE_ENTRY pConfigFileEntry    = NULL;
	char ParamName[MAX_BUFF_SIZE] = { 0 };
        int i, count, indexFound = 0;
	WebcfgDebug("-------- %s ----- Enter ------\n",__FUNCTION__);
	count = getConfigNumberOfEntries();
	WebcfgDebug("count : %d\n",count);
        
        for(i=0;i<count;i++)
        {
                pSListEntry       = AnscSListGetEntryByIndex(&pMyObject->ConfigFileList, i);
                if ( pSListEntry )
                {
                        pCxtLink      = ACCESS_COSA_CONTEXT_WEBCONFIG_LINK_OBJECT(pSListEntry);
                }
                pConfigFileEntry  = (PCOSA_DML_WEBCONFIG_CONFIGFILE_ENTRY)pCxtLink->hContext;
                if(pConfigFileEntry->InstanceNumber==index)
                {
                        AnscCopyString(pConfigFileEntry->URL,configURL);
			snprintf(ParamName,MAX_BUFF_SIZE, "configfile_%d_Url", index);
			CosaDmlStoreValueIntoDb(ParamName, configURL);
			indexFound = 1;
			break;
                }
        }
        if(indexFound == 0)
	{
		WebConfigLog("Table with %d index is not available\n", index);
		return 1;
	}
	WebcfgDebug("-------- %s ----- Exit ------\n",__FUNCTION__);
        return 0;
}

BOOL getPreviousSyncDateTime(int index,char **PreviousSyncDateTime)
{
        PCOSA_DATAMODEL_WEBCONFIG                   pMyObject         = (PCOSA_DATAMODEL_WEBCONFIG)g_pCosaBEManager->hWebConfig;
        PSINGLE_LINK_ENTRY                    pSListEntry       = NULL;
        PCOSA_CONTEXT_WEBCONFIG_LINK_OBJECT    pCxtLink          = NULL;
        PCOSA_DML_WEBCONFIG_CONFIGFILE_ENTRY pConfigFileEntry    = NULL;

        int i, count, indexFound = 0;
	WebcfgDebug("-------- %s ----- Enter ------\n",__FUNCTION__);
	count = getConfigNumberOfEntries();
	WebcfgDebug("count : %d\n",count);

        for(i=0;i<count;i++)
        {
                pSListEntry       = AnscSListGetEntryByIndex(&pMyObject->ConfigFileList, i);
                if ( pSListEntry )
                {
                        pCxtLink      = ACCESS_COSA_CONTEXT_WEBCONFIG_LINK_OBJECT(pSListEntry);
                }
                pConfigFileEntry  = (PCOSA_DML_WEBCONFIG_CONFIGFILE_ENTRY)pCxtLink->hContext;
                if(pConfigFileEntry->InstanceNumber==index)
                {
			*PreviousSyncDateTime=strdup(pConfigFileEntry->PreviousSyncDateTime);
			indexFound = 1;
                        break;
                }
        }
	if(indexFound == 0)
	{
		WebConfigLog("Table with %d index is not available\n", index);
		return FALSE;
	}
	WebcfgDebug("-------- %s ----- Exit ------\n",__FUNCTION__);
        
        return TRUE;

}

int setPreviousSyncDateTime(int index)
{
        PCOSA_DATAMODEL_WEBCONFIG                   pMyObject         = (PCOSA_DATAMODEL_WEBCONFIG)g_pCosaBEManager->hWebConfig;
        PSINGLE_LINK_ENTRY                    pSListEntry       = NULL;
        PCOSA_CONTEXT_WEBCONFIG_LINK_OBJECT    pCxtLink          = NULL;
        PCOSA_DML_WEBCONFIG_CONFIGFILE_ENTRY pConfigFileEntry    = NULL;
	char ParamName[MAX_BUFF_SIZE] = { 0 };
        int i, count, indexFound = 0;
        WebcfgDebug("-------- %s ----- Enter ------\n",__FUNCTION__);
        count = getConfigNumberOfEntries();
        WebcfgDebug("count : %d\n",count);
	char str[MAX_BUFF_SIZE]={0};
        for(i=0;i<count;i++)
        {
                pSListEntry       = AnscSListGetEntryByIndex(&pMyObject->ConfigFileList, i);
                if ( pSListEntry )
                {
                        pCxtLink      = ACCESS_COSA_CONTEXT_WEBCONFIG_LINK_OBJECT(pSListEntry);
                }
                pConfigFileEntry  = (PCOSA_DML_WEBCONFIG_CONFIGFILE_ENTRY)pCxtLink->hContext;
                if(pConfigFileEntry->InstanceNumber==index)
                {
       			snprintf(str,sizeof(str),"%ld",(unsigned long)time(NULL));
       			AnscCopyString(pConfigFileEntry->PreviousSyncDateTime,str);
			snprintf(ParamName,MAX_BUFF_SIZE, "configfile_%d_SyncDateTime", pConfigFileEntry->InstanceNumber);
			CosaDmlStoreValueIntoDb(ParamName, pConfigFileEntry->PreviousSyncDateTime);
			indexFound = 1;
                        break;
                }
        }

        if(indexFound == 0)
        {
                WebConfigLog("Table with %d index is not available\n", index);
                return 1;
        }
        WebcfgDebug("-------- %s ----- Exit ------\n",__FUNCTION__);
	return 0;


}

BOOL getConfigVersion(int index, char **version)
{
        PCOSA_DATAMODEL_WEBCONFIG                   pMyObject         = (PCOSA_DATAMODEL_WEBCONFIG)g_pCosaBEManager->hWebConfig;
        PSINGLE_LINK_ENTRY                    pSListEntry       = NULL;
        PCOSA_CONTEXT_WEBCONFIG_LINK_OBJECT    pCxtLink          = NULL;
        PCOSA_DML_WEBCONFIG_CONFIGFILE_ENTRY pConfigFileEntry    = NULL;

        int i, count, indexFound = 0;
	WebcfgDebug("-------- %s ----- Enter ------\n",__FUNCTION__);
	count = getConfigNumberOfEntries();
	WebcfgDebug("count : %d\n",count);

        for(i=0;i<count;i++)
        {
                pSListEntry       = AnscSListGetEntryByIndex(&pMyObject->ConfigFileList, i);
                if ( pSListEntry )
                {
                        pCxtLink      = ACCESS_COSA_CONTEXT_WEBCONFIG_LINK_OBJECT(pSListEntry);
                }
                pConfigFileEntry  = (PCOSA_DML_WEBCONFIG_CONFIGFILE_ENTRY)pCxtLink->hContext;
                if(pConfigFileEntry->InstanceNumber==index)
                {
			*version=strdup(pConfigFileEntry->Version);
			indexFound = 1;
                        break;
                }
        }
	if(indexFound == 0)
	{
		WebConfigLog("Table with %d index is not available\n", index);
		return FALSE;
	}
	WebcfgDebug("-------- %s ----- Exit ------\n",__FUNCTION__);
        
        return TRUE;

}

int setConfigVersion(int index, char *version)
{
        PCOSA_DATAMODEL_WEBCONFIG                   pMyObject         = (PCOSA_DATAMODEL_WEBCONFIG)g_pCosaBEManager->hWebConfig;
        PSINGLE_LINK_ENTRY                    pSListEntry       = NULL;
        PCOSA_CONTEXT_WEBCONFIG_LINK_OBJECT    pCxtLink          = NULL;
        PCOSA_DML_WEBCONFIG_CONFIGFILE_ENTRY pConfigFileEntry    = NULL;
        char ParamName[MAX_BUFF_SIZE] = { 0 };
	
	int i, count, indexFound = 0;
	WebcfgDebug("-------- %s ----- Enter ------\n",__FUNCTION__);
	count = getConfigNumberOfEntries();
	WebcfgDebug("count : %d\n",count);

        for(i=0;i<count;i++)
        {
                pSListEntry       = AnscSListGetEntryByIndex(&pMyObject->ConfigFileList, i);
                if ( pSListEntry )
                {
                        pCxtLink      = ACCESS_COSA_CONTEXT_WEBCONFIG_LINK_OBJECT(pSListEntry);
                }
                pConfigFileEntry  = (PCOSA_DML_WEBCONFIG_CONFIGFILE_ENTRY)pCxtLink->hContext;
                if(pConfigFileEntry->InstanceNumber==index)
                {
			AnscCopyString(pConfigFileEntry->Version,version);
			snprintf(ParamName,MAX_BUFF_SIZE, "configfile_%d_Version", pConfigFileEntry->InstanceNumber);
			CosaDmlStoreValueIntoDb(ParamName, pConfigFileEntry->Version);
			indexFound = 1;
                        break;
                }
        }
        if(indexFound == 0)
	{
		WebConfigLog("Table with %d index is not available\n", index);
		return 1;
	}
	WebcfgDebug("-------- %s ----- Exit ------\n",__FUNCTION__);
        return 0;

}

BOOL getSyncCheckOK(int index,BOOL *pvalue )
{
        PCOSA_DATAMODEL_WEBCONFIG                   pMyObject         = (PCOSA_DATAMODEL_WEBCONFIG)g_pCosaBEManager->hWebConfig;
        PSINGLE_LINK_ENTRY                    pSListEntry       = NULL;
        PCOSA_CONTEXT_WEBCONFIG_LINK_OBJECT    pCxtLink          = NULL;
        PCOSA_DML_WEBCONFIG_CONFIGFILE_ENTRY pConfigFileEntry    = NULL;
        int i, count, indexFound = 0;
        WebcfgDebug("-------- %s ----- Enter ------\n",__FUNCTION__);
        count = getConfigNumberOfEntries();
        WebcfgDebug("count : %d\n",count);
        for(i=0;i<count;i++)
        {
                pSListEntry       = AnscSListGetEntryByIndex(&pMyObject->ConfigFileList, i);
                if ( pSListEntry )
                {
                        pCxtLink      = ACCESS_COSA_CONTEXT_WEBCONFIG_LINK_OBJECT(pSListEntry);
                }
                pConfigFileEntry  = (PCOSA_DML_WEBCONFIG_CONFIGFILE_ENTRY)pCxtLink->hContext;
                if(pConfigFileEntry->InstanceNumber==index)
                {
			*pvalue=pConfigFileEntry->SyncCheckOK;
			indexFound = 1;
                        break;
                }
        }

        if(indexFound == 0)
        {
                WebConfigLog("Table with %d index is not available\n", index);
                return FALSE;
        }
        WebcfgDebug("-------- %s ----- Exit ------\n",__FUNCTION__);
        return TRUE;
}

int setSyncCheckOK(int index, BOOL status)
{
    PCOSA_DATAMODEL_WEBCONFIG                   pMyObject         = (PCOSA_DATAMODEL_WEBCONFIG)g_pCosaBEManager->hWebConfig;
    PSINGLE_LINK_ENTRY                    pSListEntry       = NULL;
    PCOSA_CONTEXT_WEBCONFIG_LINK_OBJECT    pCxtLink          = NULL;
    PCOSA_DML_WEBCONFIG_CONFIGFILE_ENTRY pConfigFileEntry    = NULL;
    int i, count, indexFound = 0;
    char ParamName[MAX_BUFF_SIZE] = { 0 };
    WebcfgDebug("-------- %s ----- Enter ------\n",__FUNCTION__);
    count = getConfigNumberOfEntries();
    WebcfgDebug("count : %d\n",count);
    for(i=0;i<count;i++)
    {
        pSListEntry       = AnscSListGetEntryByIndex(&pMyObject->ConfigFileList, i);
        if ( pSListEntry )
        {
            pCxtLink      = ACCESS_COSA_CONTEXT_WEBCONFIG_LINK_OBJECT(pSListEntry);
        }
        pConfigFileEntry  = (PCOSA_DML_WEBCONFIG_CONFIGFILE_ENTRY)pCxtLink->hContext;
        if(pConfigFileEntry->InstanceNumber == index)
        {
            pConfigFileEntry->SyncCheckOK=status;
            snprintf(ParamName,MAX_BUFF_SIZE, "configfile_%d_SyncCheckOk", index);
			if(status == true)
			{
				CosaDmlStoreValueIntoDb(ParamName, "true");
			}
			else
			{
				CosaDmlStoreValueIntoDb(ParamName, "false");
			}
            indexFound = 1;
            break;
        }
    }

	if(indexFound == 0)
	{
		WebConfigLog("Table with %d index is not available\n", index);
		return 1;
	}
	WebcfgDebug("-------- %s ----- Exit ------\n",__FUNCTION__);
	return 0;
}

BOOL getForceSyncCheck(int index,BOOL *pvalue )
{
        PCOSA_DATAMODEL_WEBCONFIG                   pMyObject         = (PCOSA_DATAMODEL_WEBCONFIG)g_pCosaBEManager->hWebConfig;
        PSINGLE_LINK_ENTRY                    pSListEntry       = NULL;
        PCOSA_CONTEXT_WEBCONFIG_LINK_OBJECT    pCxtLink          = NULL;
        PCOSA_DML_WEBCONFIG_CONFIGFILE_ENTRY pConfigFileEntry    = NULL;
        int i, count, indexFound = 0;
        WebcfgDebug("-------- %s ----- Enter ------\n",__FUNCTION__);
        count = getConfigNumberOfEntries();
	WebcfgDebug("count : %d\n",count);
        for(i=0;i<count;i++)
        {
                pSListEntry       = AnscSListGetEntryByIndex(&pMyObject->ConfigFileList, i);
                if ( pSListEntry )
                {
                        pCxtLink      = ACCESS_COSA_CONTEXT_WEBCONFIG_LINK_OBJECT(pSListEntry);
                }
                pConfigFileEntry  = (PCOSA_DML_WEBCONFIG_CONFIGFILE_ENTRY)pCxtLink->hContext;
                if(pConfigFileEntry->InstanceNumber==index)
                {
			*pvalue=pConfigFileEntry->ForceSyncCheck;
			indexFound = 1;
                        break;
                }
        }

        if(indexFound == 0)
        {
                WebConfigLog("Table with %d index is not available\n", index);
                return FALSE;
        }
        WebcfgDebug("-------- %s ----- Exit ------\n",__FUNCTION__);

        return TRUE;
}

BOOL setForceSyncCheck(int index, BOOL pvalue)
{
	PCOSA_DATAMODEL_WEBCONFIG                   pMyObject         = (PCOSA_DATAMODEL_WEBCONFIG)g_pCosaBEManager->hWebConfig;
	PSINGLE_LINK_ENTRY                    pSListEntry       = NULL;
	PCOSA_CONTEXT_WEBCONFIG_LINK_OBJECT    pCxtLink          = NULL;
	PCOSA_DML_WEBCONFIG_CONFIGFILE_ENTRY pConfigFileEntry    = NULL;
	int i, count, indexFound = 0;
	WebcfgDebug("-------- %s ----- Enter ------\n",__FUNCTION__);
	count = getConfigNumberOfEntries();
	WebcfgDebug("count : %d\n",count);
	for(i=0;i<count;i++)
	{
		pSListEntry       = AnscSListGetEntryByIndex(&pMyObject->ConfigFileList, i);
		if ( pSListEntry )
		{
			pCxtLink      = ACCESS_COSA_CONTEXT_WEBCONFIG_LINK_OBJECT(pSListEntry);
		}
		pConfigFileEntry  = (PCOSA_DML_WEBCONFIG_CONFIGFILE_ENTRY)pCxtLink->hContext;
		if(pConfigFileEntry->InstanceNumber==index)
		{
			pConfigFileEntry->ForceSyncCheck=pvalue;
			if(pvalue)
			{
				pthread_mutex_lock (get_global_periodicsync_mutex());
				pthread_cond_signal(get_global_periodicsync_condition());
				pthread_mutex_unlock(get_global_periodicsync_mutex());
			}
			indexFound = 1;
			break;
		}
	}

	if(indexFound == 0)
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
	snprintf(paramVal[valIndex]->parameterName, MAX_PARAMETERNAME_LEN, "%s%d.%s",WEBCONFIG_TABLE_CONFIGFILE, index, CONFIGFILE_PARAM_PREV_SYNC_TIME);
	ret = getPreviousSyncDateTime(index, &valueStr); 
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

    PCOSA_DATAMODEL_WEBCONFIG   pWebConfig = (PCOSA_DATAMODEL_WEBCONFIG)g_pCosaBEManager->hWebConfig;
    RFC_ENABLE = Get_RfcEnable();
    WebcfgDebug("paramCount = %d\n",paramCount);
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
                            else if(strcmp(restDmlString, CONFIGFILE_PARAM_PREV_SYNC_TIME) == 0)
                            {
                                ret = getPreviousSyncDateTime(index, &valueStr);
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
                                WebcfgDebug("pWebConfig->RfcEnable is %d\n",pWebConfig->RfcEnable);
                                if(pWebConfig->RfcEnable == true)
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
                                paramVal[k]->parameterName = strndup(WEBCONFIG_PARAM_PERIODIC_INTERVAL, MAX_PARAMETERNAME_LEN);
                                WebcfgDebug("pWebConfig->PeriodicSyncCheckInterval is %d\n",pWebConfig->PeriodicSyncCheckInterval);
                                paramVal[k]->parameterValue = (char *)malloc(sizeof(char)*MAX_PARAMETERVALUE_LEN);
                                snprintf(paramVal[k]->parameterValue,MAX_PARAMETERVALUE_LEN,"%d",pWebConfig->PeriodicSyncCheckInterval);
                                paramVal[k]->type = ccsp_int;
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
                                localCount = localCount+2+(count*5);
                            }
                            paramVal = (parameterValStruct_t **) realloc(paramVal, sizeof(parameterValStruct_t *)*localCount);
                            paramVal[k] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t));
                            memset(paramVal[k], 0, sizeof(parameterValStruct_t));
                            paramVal[k]->parameterName = strndup(WEBCONFIG_PARAM_RFC_ENABLE, MAX_PARAMETERNAME_LEN);
                            WebcfgDebug("paramVal[%d]->parameterName: %s\n",k,paramVal[k]->parameterName);
                            WebcfgDebug("pWebConfig->RfcEnable is %d\n",pWebConfig->RfcEnable);
                            if(pWebConfig->RfcEnable == true)
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
                                WebcfgDebug("pWebConfig->PeriodicSyncCheckInterval is %d\n",pWebConfig->PeriodicSyncCheckInterval);
                                paramVal[k]->parameterValue = (char *)malloc(sizeof(char)*MAX_PARAMETERVALUE_LEN);
                                snprintf(paramVal[k]->parameterValue,MAX_PARAMETERVALUE_LEN,"%d",pWebConfig->PeriodicSyncCheckInterval);
                                paramVal[k]->type = ccsp_int;
                                k++;
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
        WebcfgDebug("Final-> %s %s %d\n",(*val)[i]->parameterName, (*val)[i]->parameterValue, (*val)[i]->type);
    }
    *val_size = k;
    WebcfgDebug("Final count is %d\n",*val_size);
    WebcfgDebug("*********** %s ***************\n",__FUNCTION__);
    return CCSP_SUCCESS;
}

int setWebConfigParameterValues(parameterValStruct_t *val, int paramCount, char **faultParam )
{
	int i=0;
	char *subStr = NULL;
	BOOL RFC_ENABLE;
	WebcfgDebug("*********** %s ***************\n",__FUNCTION__);

	char *webConfigObject = "Device.X_RDK_WebConfig.";
	RFC_ENABLE = Get_RfcEnable();
	PCOSA_DATAMODEL_WEBCONFIG   pWebConfig = (PCOSA_DATAMODEL_WEBCONFIG)g_pCosaBEManager->hWebConfig;

	WebcfgDebug("paramCount = %d\n",paramCount);
	for(i=0; i<paramCount; i++)
	{
		if(strstr(val[i].parameterName, webConfigObject) != NULL)
		{
			if(strcmp(val[i].parameterName, WEBCONFIG_PARAM_RFC_ENABLE) == 0)
			{
				CosaDmlStoreValueIntoDb( "WebConfigRfcEnabled", val[i].parameterValue );
				if((val[i].parameterValue != NULL) && (strcmp(val[i].parameterValue, "true") == 0))
				{
					pWebConfig->RfcEnable = true;
				}
				else
				{
					pWebConfig->RfcEnable = false;
				}
			}
			else if((strcmp(val[i].parameterName, WEBCONFIG_PARAM_PERIODIC_INTERVAL) == 0) && (RFC_ENABLE == true))
			{
				subStr = val[i].parameterName+strlen(webConfigObject);
				CosaDmlStoreValueIntoDb( subStr, val[i].parameterValue );
				if(val[i].parameterValue != NULL)
				{
					pWebConfig->PeriodicSyncCheckInterval = atoi(val[i].parameterValue);
				}
				else
				{
					pWebConfig->PeriodicSyncCheckInterval = 0;
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
						ret = setForceSyncCheck(index, true);
					}
					else
					{
						ret = setForceSyncCheck(index, false);
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
	WebcfgDebug("*********** %s ***************\n",__FUNCTION__);
	return CCSP_SUCCESS;
}
