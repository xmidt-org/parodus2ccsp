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

/**********************************************************************
   Copyright [2014] [Cisco Systems, Inc.]
 
   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at
 
       http://www.apache.org/licenses/LICENSE-2.0
 
   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
**********************************************************************/

/**************************************************************************

    module: cosa_webconfig_apis.c

        For COSA Data Model Library Development

    -------------------------------------------------------------------

    description:

        This file implementes back-end apis for the COSA Data Model Library

        *  CosaWebConfigCreate
        *  CosaWebConfigInitialize
        *  CosaWebConfigRemove
    -------------------------------------------------------------------

    environment:

        platform independent

    -------------------------------------------------------------------

    author:

        COSA XML TOOL CODE GENERATOR 1.0

    -------------------------------------------------------------------

    revision:

        01/11/2011    initial revision.

**************************************************************************/
#include "plugin_main_apis.h"
#include "cosa_webconfig_apis.h"
#include "webconfig_log.h"
/**********************************************************************

    caller:     owner of the object

    prototype:

        ANSC_HANDLE
        CosaWebConfigCreate
            (
                VOID
            );

    description:

        This function constructs cosa WebConfig object and return handle.

    argument:

    return:     newly created nat object.

**********************************************************************/

ANSC_HANDLE
CosaWebConfigCreate
    (
        VOID
    )
{
    ANSC_STATUS                     returnStatus = ANSC_STATUS_SUCCESS;
    PCOSA_DATAMODEL_WEBCONFIG            pMyObject    = (PCOSA_DATAMODEL_WEBCONFIG)NULL;
	int i = 0;
	WebConfigLog("-------- %s ----- Enter ------\n",__FUNCTION__);
    /*
     * We create object by first allocating memory for holding the variables and member functions.
     */
    pMyObject = (PCOSA_DATAMODEL_WEBCONFIG)AnscAllocateMemory(sizeof(COSA_DATAMODEL_WEBCONFIG));
    if ( !pMyObject )
    {
        return  (ANSC_HANDLE)NULL;
    }

    /*
     * Initialize the common variables and functions for a container object.
     */
    pMyObject->Oid               = COSA_DATAMODEL_WEBCONFIG_OID;
    pMyObject->Create            = CosaWebConfigCreate;
    pMyObject->Remove            = CosaWebConfigRemove;
    pMyObject->Initialize        = CosaWebConfigInitialize;

    pMyObject->Initialize   ((ANSC_HANDLE)pMyObject);
	
	WebConfigLog("------ pMyObject -------\n");
	WebConfigLog("pMyObject->RfcEnable: %d\n",pMyObject->RfcEnable);
	WebConfigLog("pMyObject->PeriodicSyncCheckInterval: %d\n",pMyObject->PeriodicSyncCheckInterval);
	if(pMyObject->pConfigFileContainer != NULL)
	{
		WebConfigLog("pMyObject->pConfigFileContainer->ConfigFileEntryCount: %d\n",pMyObject->pConfigFileContainer->ConfigFileEntryCount);
		if(pMyObject->pConfigFileContainer->pConfigFileTable != NULL)
		{
			for(i=0; i<pMyObject->pConfigFileContainer->ConfigFileEntryCount; i++)
			{
				WebConfigLog("pMyObject->pConfigFileContainer->pConfigFileTable[%d].InstanceNumber = %d\n",i,pMyObject->pConfigFileContainer->pConfigFileTable[i].InstanceNumber);
				WebConfigLog("pMyObject->pConfigFileContainer->pConfigFileTable[%d].URL = %s\n",i,pMyObject->pConfigFileContainer->pConfigFileTable[i].URL);
				WebConfigLog("pMyObject->pConfigFileContainer->pConfigFileTable[%d].Version = %s\n",i,pMyObject->pConfigFileContainer->pConfigFileTable[i].Version);
				WebConfigLog("pMyObject->pConfigFileContainer->pConfigFileTable[%d].ForceSyncCheck = %d\n",i,pMyObject->pConfigFileContainer->pConfigFileTable[i].ForceSyncCheck);
				WebConfigLog("pMyObject->pConfigFileContainer->pConfigFileTable[%d].SyncCheckOK = %d\n",i,pMyObject->pConfigFileContainer->pConfigFileTable[i].SyncCheckOK);
				WebConfigLog("pMyObject->pConfigFileContainer->pConfigFileTable[%d].PreviousSyncDateTime = %s\n",i,pMyObject->pConfigFileContainer->pConfigFileTable[i].PreviousSyncDateTime);
			}
		}
	}
	WebConfigLog("------ pMyObject -------\n");
	WebConfigLog("-------- %s ----- Exit ------\n",__FUNCTION__);
    return  (ANSC_HANDLE)pMyObject;
}

/**********************************************************************

    caller:     self

    prototype:

        ANSC_STATUS
        CosaWebConfigInitialize
            (
                ANSC_HANDLE                 hThisObject
            );

    description:

        This function initiate  cosa WebConfig object and return handle.

    argument:   ANSC_HANDLE                 hThisObject
                This handle is actually the pointer of this object
                itself.

    return:     operation status.

**********************************************************************/

ANSC_STATUS
CosaWebConfigInitialize
    (
        ANSC_HANDLE                 hThisObject
    )
{
    ANSC_STATUS                     returnStatus         = ANSC_STATUS_SUCCESS;
    PCOSA_DATAMODEL_WEBCONFIG            pMyObject            = (PCOSA_DATAMODEL_WEBCONFIG )hThisObject;
	WebConfigLog("-------- %s ----- Enter ------\n",__FUNCTION__);
    AnscSListInitializeHeader( &pMyObject->ConfigFileList );
    pMyObject->MaxInstanceNumber        = 0;
    CHAR tmpbuf[ 128 ] = { 0 };
#ifdef RDKB_BUILD
    WebConfigLog("------- %s ---------\n",__FUNCTION__);
    // Initialize syscfg to make syscfg calls
    if (0 != syscfg_init())
    {
    	WalError("CosaWebConfigInitialize Error: syscfg_init() failed!! \n");
    	return ANSC_STATUS_FAILURE;
    }
    else
#endif
    {
        CosaDmlGetValueFromDb("WebConfig_NextInstanceNumber",tmpbuf);
        if(tmpbuf[0] != '\0')
        {
            pMyObject->ulWebConfigNextInstanceNumber   = atoi(tmpbuf);
        }
        else
        {
            pMyObject->ulWebConfigNextInstanceNumber   = 1;
        }
        WebConfigLog("pMyObject->ulWebConfigNextInstanceNumber: %d\n",pMyObject->ulWebConfigNextInstanceNumber);
		CosaDmlGetValueFromDb("WebConfigRfcEnabled", tmpbuf);
        if( tmpbuf != NULL && AnscEqualString(tmpbuf, "true", TRUE))
        {
            pMyObject->RfcEnable = true;
        }
        else
        {
            pMyObject->RfcEnable = false;
        }
        WalInfo("pMyObject->RfcEnable : %d\n",pMyObject->RfcEnable);
		CosaDmlGetValueFromDb("PeriodicSyncCheckInterval", tmpbuf);
        if(tmpbuf != NULL)
        {
            pMyObject->PeriodicSyncCheckInterval = atoi(tmpbuf);
        }
        WalInfo("pMyObject->PeriodicSyncCheckInterval:%d\n",pMyObject->PeriodicSyncCheckInterval);
    }
    WebConfigLog("B4 CosaDmlGetConfigFile\n");
    pMyObject->pConfigFileContainer = CosaDmlGetConfigFile((ANSC_HANDLE)pMyObject);
    WebConfigLog("After CosaDmlGetConfigFile\n");
	WebConfigLog("##### ConfigFile container data #####\n");
	WebConfigLog("pMyObject->pConfigFileContainer->ConfigFileEntryCount: %d\n",pMyObject->pConfigFileContainer->ConfigFileEntryCount);
	int i = 0;
	if(pMyObject->pConfigFileContainer->pConfigFileTable != NULL)
	{
		for(i=0; i<pMyObject->pConfigFileContainer->ConfigFileEntryCount; i++)
		{
			WebConfigLog("pMyObject->pConfigFileContainer->pConfigFileTable[%d].InstanceNumber = %d\n",i,pMyObject->pConfigFileContainer->pConfigFileTable[i].InstanceNumber);
			WebConfigLog("pMyObject->pConfigFileContainer->pConfigFileTable[%d].URL = %s\n",i,pMyObject->pConfigFileContainer->pConfigFileTable[i].URL);
			WebConfigLog("pMyObject->pConfigFileContainer->pConfigFileTable[%d].Version = %s\n",i,pMyObject->pConfigFileContainer->pConfigFileTable[i].Version);
			WebConfigLog("pMyObject->pConfigFileContainer->pConfigFileTable[%d].ForceSyncCheck = %d\n",i,pMyObject->pConfigFileContainer->pConfigFileTable[i].ForceSyncCheck);
			WebConfigLog("pMyObject->pConfigFileContainer->pConfigFileTable[%d].SyncCheckOK = %d\n",i,pMyObject->pConfigFileContainer->pConfigFileTable[i].SyncCheckOK);
			WebConfigLog("pMyObject->pConfigFileContainer->pConfigFileTable[%d].PreviousSyncDateTime = %s\n",i,pMyObject->pConfigFileContainer->pConfigFileTable[i].PreviousSyncDateTime);
		}
	}
	WebConfigLog("##### ConfigFile container data #####\n");
    WebConfigLog("#### CosaWebConfigInitialize done. return %d\n", returnStatus);

    return returnStatus;
}

/**********************************************************************

    caller:     self

    prototype:

        ANSC_STATUS
        CosaWebConfigRemove
            (
                ANSC_HANDLE                 hThisObject
            );

    description:

        This function initiate  cosa webconfig object and return handle.

    argument:   ANSC_HANDLE                 hThisObject
                This handle is actually the pointer of this object
                itself.

    return:     operation status.

**********************************************************************/

ANSC_STATUS
CosaWebConfigRemove
    (
        ANSC_HANDLE                 hThisObject
    )
{
    ANSC_STATUS                     returnStatus = ANSC_STATUS_SUCCESS;
    PCOSA_DATAMODEL_WEBCONFIG            pMyObject    = (PCOSA_DATAMODEL_WEBCONFIG)hThisObject;
	WebConfigLog("-------- %s ----- Enter ------\n",__FUNCTION__);
    if ( pMyObject->pConfigFileContainer)
    {
        /* Remove necessary resounce */
        if ( pMyObject->pConfigFileContainer->pConfigFileTable)
        {
            AnscFreeMemory(pMyObject->pConfigFileContainer->pConfigFileTable );
        }

        AnscFreeMemory(pMyObject->pConfigFileContainer);
        pMyObject->pConfigFileContainer = NULL;
    }

    /* Remove self */
    AnscFreeMemory((ANSC_HANDLE)pMyObject);
	WebConfigLog("-------- %s ----- EXIT ------\n",__FUNCTION__);
    return returnStatus;
}

PCOSA_DML_CONFIGFILE_CONTAINER
CosaDmlGetConfigFile(    
        ANSC_HANDLE                 hThisObject
    )
{
	PCOSA_DATAMODEL_WEBCONFIG      pMyObject            = (PCOSA_DATAMODEL_WEBCONFIG)hThisObject;
	PCOSA_DML_CONFIGFILE_CONTAINER    pConfigFileContainer            = (PCOSA_DML_CONFIGFILE_CONTAINER)NULL;
    PCOSA_DML_WEBCONFIG_CONFIGFILE_ENTRY pConfigFileEntry = NULL;
    int configFileCount = 0;
    char strInstance[516] = { 0 };
    int index = 0, i = 0, j = 0;

	pConfigFileContainer = (PCOSA_DML_CONFIGFILE_CONTAINER)AnscAllocateMemory(sizeof(COSA_DML_CONFIGFILE_CONTAINER));
    WebConfigLog("------- %s ---------\n",__FUNCTION__);
	memset(pConfigFileContainer, 0, sizeof(COSA_DML_CONFIGFILE_CONTAINER));

    CHAR tmpbuf[ 128 ] = { 0 };
	CosaDmlGetValueFromDb("ConfigFileNumberOfEntries", tmpbuf);
    if(tmpbuf[0] != '\0')
    {
        configFileCount = atoi(tmpbuf);
        WalInfo("configFileCount: %d\n",configFileCount);
    }
    pConfigFileContainer->ConfigFileEntryCount = configFileCount;
    if(configFileCount > 0)
    {
        pConfigFileContainer->pConfigFileTable = (PCOSA_DML_WEBCONFIG_CONFIGFILE_ENTRY)AnscAllocateMemory(sizeof(COSA_DML_WEBCONFIG_CONFIGFILE_ENTRY)*(configFileCount));
        memset(pConfigFileContainer->pConfigFileTable, 0, sizeof(COSA_DML_WEBCONFIG_CONFIGFILE_ENTRY)*(configFileCount));
		CosaDmlGetValueFromDb("WebConfig_IndexesList", strInstance);
        if(strInstance != NULL)
        {
            WalInfo("strInstance: %s\n",strInstance);
            char *tok = strtok(strInstance, ",");
            while(tok != NULL)
            {
                index = atoi(tok);
                WalInfo("index at %d: %d\n",i, index);
                WebConfigLog("B4 CosaDmlGetConfigFileEntry\n");
                pConfigFileEntry = CosaDmlGetConfigFileEntry(index);
                WebConfigLog("After CosaDmlGetConfigFileEntry\n");
                FillEntryInList(pMyObject, pConfigFileEntry);
                WebConfigLog("After FillEntryInList\n");
                //pConfigFileContainer->pConfigFileTable[i] = pConfigFileEntry;
                WebConfigLog("pConfigFileEntry->InstanceNumber: %d\n",pConfigFileEntry->InstanceNumber);
                pConfigFileContainer->pConfigFileTable[i].InstanceNumber = pConfigFileEntry->InstanceNumber;
                WebConfigLog("pConfigFileEntry->URL: %s\n",pConfigFileEntry->URL);
                AnscCopyString(pConfigFileContainer->pConfigFileTable[i].URL, pConfigFileEntry->URL);
                WebConfigLog("pConfigFileEntry->Version: %s\n",pConfigFileEntry->Version);
                AnscCopyString(pConfigFileContainer->pConfigFileTable[i].Version, pConfigFileEntry->Version);
                WebConfigLog("pConfigFileEntry->ForceSyncCheck: %d\n",pConfigFileEntry->ForceSyncCheck);
                pConfigFileContainer->pConfigFileTable[i].ForceSyncCheck = pConfigFileEntry->ForceSyncCheck;
                WebConfigLog("pConfigFileEntry->SyncCheckOK: %d\n",pConfigFileEntry->SyncCheckOK);
                pConfigFileContainer->pConfigFileTable[i].SyncCheckOK = pConfigFileEntry->SyncCheckOK;
                WebConfigLog("pConfigFileEntry->PreviousSyncDateTime: %s\n",pConfigFileEntry->PreviousSyncDateTime);
                AnscCopyString(pConfigFileContainer->pConfigFileTable[i].PreviousSyncDateTime, pConfigFileEntry->PreviousSyncDateTime);
                i++;
                tok = strtok(NULL, ",");
            }
        }
    }
    WebConfigLog("######### ConfigFile data : %d ########\n",configFileCount);
    for(j=0; j<configFileCount; j++)
    {
        WebConfigLog("%d: %s %s %d %d %s\n",pConfigFileContainer->pConfigFileTable[j].InstanceNumber, pConfigFileContainer->pConfigFileTable[j].URL, pConfigFileContainer->pConfigFileTable[j].Version, pConfigFileContainer->pConfigFileTable[j].ForceSyncCheck, pConfigFileContainer->pConfigFileTable[j].SyncCheckOK, pConfigFileContainer->pConfigFileTable[j].PreviousSyncDateTime);
    }
    WebConfigLog("######### ConfigFile data ########\n");
    WebConfigLog("------- %s -----EXIT----\n",__FUNCTION__);
	return pConfigFileContainer;
}

PCOSA_DML_WEBCONFIG_CONFIGFILE_ENTRY
CosaDmlGetConfigFileEntry
    (
        ULONG InstanceNumber
    )
{
    CHAR tmpbuf[ 256 ] = { 0 };
	char ParamName[128] = { 0 };
    WebConfigLog("-------- %s ----- Enter ------\n",__FUNCTION__);
    PCOSA_DML_WEBCONFIG_CONFIGFILE_ENTRY pConfigFileEntry = (PCOSA_DML_WEBCONFIG_CONFIGFILE_ENTRY)AnscAllocateMemory(sizeof(COSA_DML_WEBCONFIG_CONFIGFILE_ENTRY));
    memset(pConfigFileEntry, 0, sizeof(COSA_DML_WEBCONFIG_CONFIGFILE_ENTRY));
    pConfigFileEntry->InstanceNumber = InstanceNumber;
	sprintf(ParamName, "configfile_%d_Url", InstanceNumber);
	CosaDmlGetValueFromDb(ParamName, tmpbuf);
    WalInfo("Url at %d:%s\n",InstanceNumber,tmpbuf);
	AnscCopyString( pConfigFileEntry->URL, tmpbuf );

	sprintf(ParamName, "configfile_%d_Version", InstanceNumber);
	CosaDmlGetValueFromDb(ParamName, tmpbuf);
    WalInfo("Version at %d:%s\n",InstanceNumber,tmpbuf);
	AnscCopyString( pConfigFileEntry->Version, tmpbuf );

	sprintf(ParamName, "configfile_%d_ForceSyncCheck", InstanceNumber);
	CosaDmlGetValueFromDb(ParamName, tmpbuf);
    WalInfo("ForceSyncCheck at %d:%s\n",InstanceNumber,tmpbuf);
	if(strcmp( tmpbuf, "true" ) == 0)
    {
	    pConfigFileEntry->ForceSyncCheck = true;
    }
    else
    {
        pConfigFileEntry->ForceSyncCheck = false;
    }

	sprintf(ParamName, "configfile_%d_SyncCheckOk", InstanceNumber);
	CosaDmlGetValueFromDb(ParamName, tmpbuf);
    WalInfo("SyncCheckOK at %d:%s\n",InstanceNumber,tmpbuf);
    if(strcmp( tmpbuf, "true" ) == 0)
    {
	    pConfigFileEntry->SyncCheckOK = true;
    }
    else
    {
        pConfigFileEntry->SyncCheckOK = false;
    }

	sprintf(ParamName, "configfile_%d_SyncDateTime", InstanceNumber);
	CosaDmlGetValueFromDb(ParamName, tmpbuf);
	WalInfo("PreviousSyncDateTime at %d:%s\n",InstanceNumber,tmpbuf);
	AnscCopyString(pConfigFileEntry->PreviousSyncDateTime,tmpbuf);

    WalInfo("-------- %s ----- EXIT ------\n",__FUNCTION__);
    return pConfigFileEntry;
}

ANSC_STATUS
CosaDmlSetConfigFileEntry
(
	PCOSA_DML_WEBCONFIG_CONFIGFILE_ENTRY configFileEntry
)
{
	char ParamName[128] = { 0 };

	WalInfo("-------- %s ----- Enter ------\n",__FUNCTION__);
	sprintf(ParamName, "configfile_%d_Url", configFileEntry->InstanceNumber);
	if((configFileEntry->URL)[0] != '\0')
	{
		CosaDmlStoreValueIntoDb(ParamName, configFileEntry->URL);
	}
	else
	{
		CosaDmlStoreValueIntoDb(ParamName, "");
	}

	sprintf(ParamName, "configfile_%d_Version", configFileEntry->InstanceNumber);
	if((configFileEntry->Version)[0] != '\0')
	{
		CosaDmlStoreValueIntoDb(ParamName, configFileEntry->Version);
	}
	else
	{
		CosaDmlStoreValueIntoDb(ParamName, "");
	}

	sprintf(ParamName, "configfile_%d_ForceSyncCheck", configFileEntry->InstanceNumber);
	if(configFileEntry->ForceSyncCheck == true)
	{
		CosaDmlStoreValueIntoDb(ParamName, "true");
	}
	else
	{
		CosaDmlStoreValueIntoDb(ParamName, "false");
	}
	sprintf(ParamName, "configfile_%d_SyncCheckOk", configFileEntry->InstanceNumber);
	if(configFileEntry->SyncCheckOK == true)
	{
		CosaDmlStoreValueIntoDb(ParamName, "true");
	}
	else
	{
		CosaDmlStoreValueIntoDb(ParamName, "false");
	}

	sprintf(ParamName, "configfile_%d_SyncDateTime", configFileEntry->InstanceNumber);
	if((configFileEntry->PreviousSyncDateTime)[0] != '\0')
	{
		CosaDmlStoreValueIntoDb(ParamName, configFileEntry->PreviousSyncDateTime);
	}
	else
	{
		CosaDmlStoreValueIntoDb(ParamName, "");
	}

	WalInfo("-------- %s ----- Exit ------\n",__FUNCTION__);
	return ANSC_STATUS_SUCCESS;
}

void FillEntryInList(PCOSA_DATAMODEL_WEBCONFIG pWebConfig, PCOSA_DML_WEBCONFIG_CONFIGFILE_ENTRY configFileEntry)
{
	PCOSA_CONTEXT_WEBCONFIG_LINK_OBJECT   pWebConfigCxtLink = NULL;
    WalInfo("-------- %s ----- Enter ------\n",__FUNCTION__);
    pWebConfigCxtLink = (PCOSA_CONTEXT_WEBCONFIG_LINK_OBJECT)AnscAllocateMemory(sizeof(COSA_CONTEXT_WEBCONFIG_LINK_OBJECT));
    if ( !pWebConfigCxtLink )
    {
        fprintf(stderr, "Allocation failed \n");
        return;
    }

	pWebConfigCxtLink->InstanceNumber =  configFileEntry->InstanceNumber;
	pWebConfigCxtLink->hContext = (ANSC_HANDLE)configFileEntry;
	CosaSListPushEntryByInsNum(&pWebConfig->ConfigFileList, (PCOSA_CONTEXT_LINK_OBJECT)pWebConfigCxtLink);
	WalInfo("-------- %s ----- Exit ------\n",__FUNCTION__);
}

ANSC_STATUS
CosaDmlRemoveConfigFileEntry
    (
        ULONG InstanceNumber
    )
{
	char ParamName[128] = { 0 };
    WalInfo("-------- %s ----- Enter ------\n",__FUNCTION__);
    sprintf(ParamName, "configfile_%d_Url", InstanceNumber);
	CosaDmlRemoveValueFromDb(ParamName);

	sprintf(ParamName, "configfile_%d_Version", InstanceNumber);
	CosaDmlRemoveValueFromDb(ParamName);

	sprintf(ParamName, "configfile_%d_ForceSyncCheck", InstanceNumber);
	CosaDmlRemoveValueFromDb(ParamName);

	sprintf(ParamName, "configfile_%d_SyncCheckOk", InstanceNumber);
	CosaDmlRemoveValueFromDb(ParamName);

	sprintf(ParamName, "configfile_%d_SyncDateTime", InstanceNumber);
	CosaDmlRemoveValueFromDb(ParamName);
	
    WalInfo("Remove %d from WebConfig_IndexesList\n",InstanceNumber);
    RemoveEntryFromIndexesList(InstanceNumber);
    WalInfo("-------- %s ----- Exit ------\n",__FUNCTION__);
    return ANSC_STATUS_SUCCESS;
}

void appendToIndexesList(char *index, char *indexesList)
{
    WalInfo("-------- %s ----- Enter ------\n",__FUNCTION__);
    if(indexesList[0] == '\0')
    {
        sprintf(indexesList, "%s", index);
    }
    else
    {
        sprintf(indexesList, "%s,%s",indexesList,index);
    }
    WalInfo("-------- %s ----- Exit ------\n",__FUNCTION__);
}

void RemoveEntryFromIndexesList(ULONG InstanceNumber)
{
    int index = 0;
    char* st;
    char newList[516]={};
    char *IndexsList = NULL;
    char strInstance[516] = { 0 };
    WalInfo("-------%s------- ENTER ------\n",__FUNCTION__);
    CosaDmlGetValueFromDb("WebConfig_IndexesList", strInstance);
    if(strInstance[0] != '\0')
    {
        IndexsList = strdup(strInstance);
        if(strInstance != NULL)
        {
            WalInfo("strInstance: %s\n",strInstance);
            char *tok = strtok_r(strInstance, ",",&st);
            while(tok != NULL)
            {
                index = atoi(tok);
                if(index == InstanceNumber)
                {
                    if(st[0] != '\0')
                    {
                        appendToIndexesList(st, newList);
                    }
                    WalInfo("Final newList: %s\n",newList);
                    CosaDmlStoreValueIntoDb("WebConfig_IndexesList",newList);
                    return;
                }
                appendToIndexesList(tok, newList);
                WalInfo("newList: %s\n",newList);
                tok = strtok_r(NULL, ",",&st);
            }
        }
    }
    WalInfo("-------%s------- EXIT ------\n",__FUNCTION__);
}

void CosaDmlGetValueFromDb( char* ParamName, char* pString )
{
    CHAR tmpbuf[ 516 ] = { 0 };
#ifdef RDKB_BUILD
	if( 0 != syscfg_get( NULL, ParamName, tmpbuf, sizeof(tmpbuf)))
	{
		WalError("syscfg_get failed\n");
	}
#endif	
	AnscCopyString( pString, tmpbuf );
}

void CosaDmlStoreValueIntoDb(char *ParamName, char *pString)
{
#ifdef RDKB_BUILD
    if ( 0 != syscfg_set(NULL, ParamName , pString ) ) 
	{
		WalError("syscfg_set failed\n");
	}
	else 
	{
		if ( 0 != syscfg_commit( ) ) 
		{
			WalError("syscfg_commit failed\n");
		}
	}
#endif
}

void CosaDmlRemoveValueFromDb(char *ParamName)
{
#ifdef RDKB_BUILD
    if ( 0 != syscfg_unset(NULL, ParamName) ) 
	{
		WalError("syscfg_unset failed\n");
	}
	else 
	{
		if ( 0 != syscfg_commit( ) ) 
		{
			WalError("syscfg_commit failed\n");
		}
	}
#endif
}

void updateConfigFileNumberOfEntries(ULONG count)
{
	char buf[16] = { 0 };
	WalInfo("-------- %s ----- Enter ------\n",__FUNCTION__);
	sprintf(buf, "%d", count);
	WalInfo("Updated count:%d\n",count);
	CosaDmlStoreValueIntoDb("ConfigFileNumberOfEntries",buf);
	WalInfo("-------- %s ----- Exit ------\n",__FUNCTION__);
}

void updateConfigFileIndexsList(ULONG index)
{
	char strInstance[516] = { 0 };
	char instance[16] = { 0 };
	WalInfo("-------- %s ----- Enter ------\n",__FUNCTION__);
	CosaDmlGetValueFromDb("WebConfig_IndexesList", strInstance);
	sprintf(instance, "%d", index);
	appendToIndexesList(instance, strInstance);
    WalInfo("Updated list: %s\n",strInstance);
    CosaDmlStoreValueIntoDb("WebConfig_IndexesList",strInstance);
	WalInfo("-------- %s ----- Exit ------\n",__FUNCTION__);
}

void updateConfigFileNextInstanceNumber(ULONG index)
{
    char buf[16] = { 0 };
	WalInfo("-------- %s ----- Enter ------\n",__FUNCTION__);
	sprintf(buf, "%d", index);
	WalInfo("Updated NextInstanceNumber:%d\n",index);
	CosaDmlStoreValueIntoDb("WebConfig_NextInstanceNumber",buf);
	WalInfo("-------- %s ----- Exit ------\n",__FUNCTION__);
}

int getConfigNumberOfEntries()
{
        PCOSA_DATAMODEL_WEBCONFIG            pMyObject           = (PCOSA_DATAMODEL_WEBCONFIG)g_pCosaBEManager->hWebConfig;

        return pMyObject->pConfigFileContainer->ConfigFileEntryCount;
}


BOOL getConfigURL(int index,char **configURL)
{
        PCOSA_DATAMODEL_WEBCONFIG                   pMyObject         = (PCOSA_DATAMODEL_WEBCONFIG)g_pCosaBEManager->hWebConfig;
        PSINGLE_LINK_ENTRY                    pSListEntry       = NULL;
        PCOSA_CONTEXT_WEBCONFIG_LINK_OBJECT    pCxtLink          = NULL;
        PCOSA_DML_WEBCONFIG_CONFIGFILE_ENTRY pConfigFileEntry    = NULL;
        int i;
        for(i=0;i<=pMyObject->pConfigFileContainer->ConfigFileEntryCount;i++)
        {
                pSListEntry       = AnscSListGetEntryByIndex(&pMyObject->ConfigFileList, i);
                if ( pSListEntry )
                {
                        pCxtLink      = ACCESS_COSA_CONTEXT_WEBCONFIG_LINK_OBJECT(pSListEntry);
                }
                pConfigFileEntry  = (PCOSA_DML_WEBCONFIG_CONFIGFILE_ENTRY)pCxtLink->hContext;
                if(pConfigFileEntry->InstanceNumber==index)
                {
                        break;
                }
                else if(i==pMyObject->pConfigFileContainer->ConfigFileEntryCount && pConfigFileEntry->InstanceNumber!=index)
                {
                                WalInfo("----%s---- %d index table not found\n",__FUNCTION__,index);
                                return FALSE;
                }
        }
        *configURL = pConfigFileEntry->URL;
        return TRUE;

}

int setConfigURL(int index, char *configURL)
{
        PCOSA_DATAMODEL_WEBCONFIG                   pMyObject         = (PCOSA_DATAMODEL_WEBCONFIG)g_pCosaBEManager->hWebConfig;
        PSINGLE_LINK_ENTRY                    pSListEntry       = NULL;
        PCOSA_CONTEXT_WEBCONFIG_LINK_OBJECT    pCxtLink          = NULL;
        PCOSA_DML_WEBCONFIG_CONFIGFILE_ENTRY pConfigFileEntry    = NULL;
        int i;
        char ParamName[MAX_BUFF_SIZE] = { 0 };
	WalInfo("Inside setConfigURL\n");
	WalInfo("pMyObject->pConfigFileContainer->ConfigFileEntryCount is %d\n", pMyObject->pConfigFileContainer->ConfigFileEntryCount);
        for(i=0;i<=pMyObject->pConfigFileContainer->ConfigFileEntryCount;i++)
        {
		WalInfo("Inside setConfigURL for\n");
                pSListEntry       = AnscSListGetEntryByIndex(&pMyObject->ConfigFileList, i);
                if ( pSListEntry )
                {
                        pCxtLink      = ACCESS_COSA_CONTEXT_WEBCONFIG_LINK_OBJECT(pSListEntry);
                }
                pConfigFileEntry  = (PCOSA_DML_WEBCONFIG_CONFIGFILE_ENTRY)pCxtLink->hContext;
                if(pConfigFileEntry->InstanceNumber==index)
                {
			WalInfo("setConfigURL break\n");
                        break;
                }
                else if(i==pMyObject->pConfigFileContainer->ConfigFileEntryCount && pConfigFileEntry->InstanceNumber!=index)
                {
                                WalInfo("----%s---- %d index table not found\n",__FUNCTION__,index);
                                return 1;
                }
        }
	WalInfo("Inside setConfigURL b4 configURL\n");
        AnscCopyString(pConfigFileEntry->URL,configURL);
	WalInfo("Inside setConfigURL ParamName\n");
        snprintf(ParamName,MAX_BUFF_SIZE, "configfile_%d_Url", pConfigFileEntry->InstanceNumber);
	WalInfo("B4 CosaDmlStoreValueIntoDb\n");
        CosaDmlStoreValueIntoDb(ParamName, pConfigFileEntry->URL);
	WalInfo("CosaDmlStoreValueIntoDb done\n");
        return 0;

}

BOOL getPreviousSyncDateTime(int index,char **PreviousSyncDateTime)
{
        PCOSA_DATAMODEL_WEBCONFIG                   pMyObject         = (PCOSA_DATAMODEL_WEBCONFIG)g_pCosaBEManager->hWebConfig;
        PSINGLE_LINK_ENTRY                    pSListEntry       = NULL;
        PCOSA_CONTEXT_WEBCONFIG_LINK_OBJECT    pCxtLink          = NULL;
        PCOSA_DML_WEBCONFIG_CONFIGFILE_ENTRY pConfigFileEntry    = NULL;
        int i;
        for(i=0;i<=pMyObject->pConfigFileContainer->ConfigFileEntryCount;i++)
        {
                pSListEntry       = AnscSListGetEntryByIndex(&pMyObject->ConfigFileList, i);
                if ( pSListEntry )
                {
                        pCxtLink      = ACCESS_COSA_CONTEXT_WEBCONFIG_LINK_OBJECT(pSListEntry);
                }
                pConfigFileEntry  = (PCOSA_DML_WEBCONFIG_CONFIGFILE_ENTRY)pCxtLink->hContext;
                if(pConfigFileEntry->InstanceNumber==index)
                {
                        break;
                }
                else if(i==pMyObject->pConfigFileContainer->ConfigFileEntryCount && pConfigFileEntry->InstanceNumber!=index)
                {
                                WalInfo("----%s---- %d index table not found\n",__FUNCTION__,index);
                                return FALSE;
                }
        }

        *PreviousSyncDateTime=pConfigFileEntry->PreviousSyncDateTime;
        return TRUE;

}

int setPreviousSyncDateTime(int index)
{
        PCOSA_DATAMODEL_WEBCONFIG                   pMyObject         = (PCOSA_DATAMODEL_WEBCONFIG)g_pCosaBEManager->hWebConfig;
        PSINGLE_LINK_ENTRY                    pSListEntry       = NULL;
        PCOSA_CONTEXT_WEBCONFIG_LINK_OBJECT    pCxtLink          = NULL;
        PCOSA_DML_WEBCONFIG_CONFIGFILE_ENTRY pConfigFileEntry    = NULL;
        int i;
        char ParamName[MAX_BUFF_SIZE] = { 0 };
        char current_time[MAX_BUFF_SIZE] = { 0 };
        for(i=0;i<=pMyObject->pConfigFileContainer->ConfigFileEntryCount;i++)
        {
                pSListEntry       = AnscSListGetEntryByIndex(&pMyObject->ConfigFileList, i);
                if ( pSListEntry )
                {
                        pCxtLink      = ACCESS_COSA_CONTEXT_WEBCONFIG_LINK_OBJECT(pSListEntry);
                }
                pConfigFileEntry  = (PCOSA_DML_WEBCONFIG_CONFIGFILE_ENTRY)pCxtLink->hContext;
                if(pConfigFileEntry->InstanceNumber==index)
                {
                        break;
                }
                else if(i==pMyObject->pConfigFileContainer->ConfigFileEntryCount && pConfigFileEntry->InstanceNumber!=index)
                {
                                WalInfo("----%s---- %d index table not found\n",__FUNCTION__,index);
                                return 1;
                }
        }
        time_t curr_time = time(NULL);
        struct tm *tm = localtime(&curr_time);
        strftime(current_time, MAX_BUFF_SIZE, "%c", tm);
        AnscCopyString(pConfigFileEntry->PreviousSyncDateTime,current_time);
        snprintf(ParamName,MAX_BUFF_SIZE, "configfile_%d_SyncDateTime", pConfigFileEntry->InstanceNumber);
        CosaDmlStoreValueIntoDb(ParamName, pConfigFileEntry->PreviousSyncDateTime);
        return 0;

}

BOOL getConfigVersion(int index, char **version)
{
        PCOSA_DATAMODEL_WEBCONFIG                   pMyObject         = (PCOSA_DATAMODEL_WEBCONFIG)g_pCosaBEManager->hWebConfig;
        PSINGLE_LINK_ENTRY                    pSListEntry       = NULL;
        PCOSA_CONTEXT_WEBCONFIG_LINK_OBJECT    pCxtLink          = NULL;
        PCOSA_DML_WEBCONFIG_CONFIGFILE_ENTRY pConfigFileEntry    = NULL;
        int i;
	WalInfo("Inside getConfigVersion\n");
	WalInfo("pMyObject->pConfigFileContainer->ConfigFileEntryCount is %d\n", pMyObject->pConfigFileContainer->ConfigFileEntryCount);
        for(i=0;i<=pMyObject->pConfigFileContainer->ConfigFileEntryCount;i++)
        {
		WalInfo("Inside for\n");
                pSListEntry       = AnscSListGetEntryByIndex(&pMyObject->ConfigFileList, i);
                if ( pSListEntry )
                {
                        pCxtLink      = ACCESS_COSA_CONTEXT_WEBCONFIG_LINK_OBJECT(pSListEntry);
                }
                pConfigFileEntry  = (PCOSA_DML_WEBCONFIG_CONFIGFILE_ENTRY)pCxtLink->hContext;
                if(pConfigFileEntry->InstanceNumber==index)
                {
			WalInfo("Before break\n");
                        break;
                }
                else if(i==pMyObject->pConfigFileContainer->ConfigFileEntryCount && pConfigFileEntry->InstanceNumber!=index)
                {
                                WalInfo("----%s---- %d index table not found\n",__FUNCTION__,index);
                                return FALSE;
                }
        }
	WalInfo("getConfigVersion assigining\n");
        *version=pConfigFileEntry->Version;
	WalInfo("*version is %s\n", *version);
        return TRUE;

}

int setConfigVersion(int index, char *version)
{
        PCOSA_DATAMODEL_WEBCONFIG                   pMyObject         = (PCOSA_DATAMODEL_WEBCONFIG)g_pCosaBEManager->hWebConfig;
        PSINGLE_LINK_ENTRY                    pSListEntry       = NULL;
        PCOSA_CONTEXT_WEBCONFIG_LINK_OBJECT    pCxtLink          = NULL;
        PCOSA_DML_WEBCONFIG_CONFIGFILE_ENTRY pConfigFileEntry    = NULL;
        int i;
        char ParamName[MAX_BUFF_SIZE] = { 0 };
        for(i=0;i<=pMyObject->pConfigFileContainer->ConfigFileEntryCount;i++)
        {
                pSListEntry       = AnscSListGetEntryByIndex(&pMyObject->ConfigFileList, i);
                if ( pSListEntry )
                {
                        pCxtLink      = ACCESS_COSA_CONTEXT_WEBCONFIG_LINK_OBJECT(pSListEntry);
                }
                pConfigFileEntry  = (PCOSA_DML_WEBCONFIG_CONFIGFILE_ENTRY)pCxtLink->hContext;
                if(pConfigFileEntry->InstanceNumber==index)
                {
                        break;
                }
                else if(i==pMyObject->pConfigFileContainer->ConfigFileEntryCount && pConfigFileEntry->InstanceNumber!=index)
                {
                                WalInfo("----%s---- %d index table not found\n",__FUNCTION__,index);
                                return 1;
                }
        }
        AnscCopyString(pConfigFileEntry->Version,version);
        snprintf(ParamName,MAX_BUFF_SIZE, "configfile_%d_Version", pConfigFileEntry->InstanceNumber);
        CosaDmlStoreValueIntoDb(ParamName, pConfigFileEntry->Version);
        return 0;

}

BOOL getSyncCheckOK(int index)
{
        PCOSA_DATAMODEL_WEBCONFIG                   pMyObject         = (PCOSA_DATAMODEL_WEBCONFIG)g_pCosaBEManager->hWebConfig;
        PSINGLE_LINK_ENTRY                    pSListEntry       = NULL;
        PCOSA_CONTEXT_WEBCONFIG_LINK_OBJECT    pCxtLink          = NULL;
        PCOSA_DML_WEBCONFIG_CONFIGFILE_ENTRY pConfigFileEntry    = NULL;
        int i;
        for(i=0;i<=pMyObject->pConfigFileContainer->ConfigFileEntryCount;i++)
        {
                pSListEntry       = AnscSListGetEntryByIndex(&pMyObject->ConfigFileList, i);
                if ( pSListEntry )
                {
                        pCxtLink      = ACCESS_COSA_CONTEXT_WEBCONFIG_LINK_OBJECT(pSListEntry);
                }
                pConfigFileEntry  = (PCOSA_DML_WEBCONFIG_CONFIGFILE_ENTRY)pCxtLink->hContext;
                if(pConfigFileEntry->InstanceNumber==index)
                {
                        break;
                }
                else if(i==pMyObject->pConfigFileContainer->ConfigFileEntryCount && pConfigFileEntry->InstanceNumber!=index)
                {
                                WalInfo("----%s---- %d index table not found\n",__FUNCTION__,index);
                                return FALSE;
                }
        }

        return pConfigFileEntry->SyncCheckOK;

}

int setSyncCheckOK(int index, BOOL status)
{
        PCOSA_DATAMODEL_WEBCONFIG                   pMyObject         = (PCOSA_DATAMODEL_WEBCONFIG)g_pCosaBEManager->hWebConfig;
        PSINGLE_LINK_ENTRY                    pSListEntry       = NULL;
        PCOSA_CONTEXT_WEBCONFIG_LINK_OBJECT    pCxtLink          = NULL;
        PCOSA_DML_WEBCONFIG_CONFIGFILE_ENTRY pConfigFileEntry    = NULL;
        int i;
        char ParamName[MAX_BUFF_SIZE] = { 0 };
        for(i=0;i<=pMyObject->pConfigFileContainer->ConfigFileEntryCount;i++)
        {
                pSListEntry       = AnscSListGetEntryByIndex(&pMyObject->ConfigFileList, i);
                if ( pSListEntry )
                {
                        pCxtLink      = ACCESS_COSA_CONTEXT_WEBCONFIG_LINK_OBJECT(pSListEntry);
                }
                pConfigFileEntry  = (PCOSA_DML_WEBCONFIG_CONFIGFILE_ENTRY)pCxtLink->hContext;
                if(pConfigFileEntry->InstanceNumber==index)
                {
                        break;
                }
                else if(i==pMyObject->pConfigFileContainer->ConfigFileEntryCount && pConfigFileEntry->InstanceNumber!=index)
                {
                                WalInfo("----%s---- %d index table not found\n",__FUNCTION__,index);
                                return 1;
                }
        }
        pConfigFileEntry->SyncCheckOK=status;
        snprintf(ParamName,MAX_BUFF_SIZE, "configfile_%d_SyncCheckOk", pConfigFileEntry->InstanceNumber);
        if(pConfigFileEntry->SyncCheckOK)
        {
                CosaDmlStoreValueIntoDb(ParamName, "true");
        }
        else
        {
                CosaDmlStoreValueIntoDb(ParamName, "false");
        }
        return 0;

}

int initConfigFileWithURL(char *Url, ULONG InstanceNumber)
{
    WalInfo("-------- %s ----- Enter ------\n",__FUNCTION__);
	if(isValidUrl(Url) == TRUE)
	{
        PCOSA_DATAMODEL_WEBCONFIG             pWebConfig              = (PCOSA_DATAMODEL_WEBCONFIG)g_pCosaBEManager->hWebConfig;
        PCOSA_DML_WEBCONFIG_CONFIGFILE_ENTRY pConfigFileEntry = NULL;
        pConfigFileEntry = (PCOSA_DML_WEBCONFIG_CONFIGFILE_ENTRY)AnscAllocateMemory(sizeof(COSA_DML_WEBCONFIG_CONFIGFILE_ENTRY));
        memset(pConfigFileEntry, 0, sizeof(COSA_DML_WEBCONFIG_CONFIGFILE_ENTRY));
	    pConfigFileEntry->InstanceNumber = InstanceNumber;
	    AnscCopyString( pConfigFileEntry->URL, Url );
        FillEntryInList(pWebConfig, pConfigFileEntry);
	    int configCount = AnscSListQueryDepth( &pWebConfig->ConfigFileList );
	    WalInfo("configCount: %d\n",configCount);
	    updateConfigFileNumberOfEntries(configCount);
	    updateConfigFileIndexsList(InstanceNumber);
	    updateConfigFileNextInstanceNumber(InstanceNumber+1);
	    CosaDmlSetConfigFileEntry(pConfigFileEntry);
	    WalInfo("-------- %s ----- Exit ------\n",__FUNCTION__);
	    return 0;
	}
	WalInfo("-------- %s ----- Exit ------\n",__FUNCTION__);
	return 1;
}
