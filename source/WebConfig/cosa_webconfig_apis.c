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

bool FillDefaultConfigFileEntryToDB();

ANSC_HANDLE
CosaWebConfigCreate
    (
        VOID
    )
{
    ANSC_STATUS                     returnStatus = ANSC_STATUS_SUCCESS;
    PCOSA_DATAMODEL_WEBCONFIG            pMyObject    = (PCOSA_DATAMODEL_WEBCONFIG)NULL;
	int i = 0;
	WebcfgDebug("-------- %s ----- Enter ------\n",__FUNCTION__);
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
	WebcfgDebug("-------- %s ----- Exit ------\n",__FUNCTION__);
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
    WebcfgDebug("-------- %s ----- Enter ------\n",__FUNCTION__);
    pMyObject->MaxInstanceNumber        = 0;
    CHAR tmpbuf[ 128 ] = { 0 };
#ifdef RDKB_BUILD
    WebcfgDebug("------- %s ---------\n",__FUNCTION__);
    // Initialize syscfg to make syscfg calls
    if (0 != syscfg_init())
    {
    	WebConfigLog("CosaWebConfigInitialize Error: syscfg_init() failed!! \n");
    	return ANSC_STATUS_FAILURE;
    }
    else
#endif
    CosaDmlGetValueFromDb("WebConfigRfcEnabled", tmpbuf);
    if( tmpbuf != NULL && AnscEqualString(tmpbuf, "true", TRUE))
    {
        pMyObject->RfcEnable = true;
    }
    else
    {
        pMyObject->RfcEnable = false;
    }
    WebcfgDebug("pMyObject->RfcEnable : %d\n",pMyObject->RfcEnable);
	/*Removing RFC check for now as data re-load is not yet implemented*/
    //if(pMyObject->RfcEnable == true)
    {
        CosaDmlGetValueFromDb("PeriodicSyncCheckInterval", tmpbuf);
        if(tmpbuf != NULL)
        {
            pMyObject->PeriodicSyncCheckInterval = atoi(tmpbuf);
        }
        WebcfgDebug("pMyObject->PeriodicSyncCheckInterval:%d\n",pMyObject->PeriodicSyncCheckInterval);
    
        AnscSListInitializeHeader( &pMyObject->ConfigFileList );
        WebcfgDebug("B4 CosaDmlGetConfigFile\n");
        pMyObject->pConfigFileContainer = CosaDmlGetConfigFile((ANSC_HANDLE)pMyObject);
        WebcfgDebug("After CosaDmlGetConfigFile\n");
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
	    WebcfgDebug("##### ConfigFile container data #####\n");
	    WebcfgDebug("pMyObject->pConfigFileContainer->ConfigFileEntryCount: %d\n",pMyObject->pConfigFileContainer->ConfigFileEntryCount);
	    int i = 0;
	    if(pMyObject->pConfigFileContainer->pConfigFileTable != NULL)
	    {
		    for(i=0; i<pMyObject->pConfigFileContainer->ConfigFileEntryCount; i++)
		    {
			    WebcfgDebug("pMyObject->pConfigFileContainer->pConfigFileTable[%d].InstanceNumber = %d\n",i,pMyObject->pConfigFileContainer->pConfigFileTable[i].InstanceNumber);
			    WebcfgDebug("pMyObject->pConfigFileContainer->pConfigFileTable[%d].URL = %s\n",i,pMyObject->pConfigFileContainer->pConfigFileTable[i].URL);
			    WebcfgDebug("pMyObject->pConfigFileContainer->pConfigFileTable[%d].Version = %s\n",i,pMyObject->pConfigFileContainer->pConfigFileTable[i].Version);
			    WebcfgDebug("pMyObject->pConfigFileContainer->pConfigFileTable[%d].ForceSyncCheck = %d\n",i,pMyObject->pConfigFileContainer->pConfigFileTable[i].ForceSyncCheck);
			    WebcfgDebug("pMyObject->pConfigFileContainer->pConfigFileTable[%d].SyncCheckOK = %d\n",i,pMyObject->pConfigFileContainer->pConfigFileTable[i].SyncCheckOK);
			    WebcfgDebug("pMyObject->pConfigFileContainer->pConfigFileTable[%d].PreviousSyncDateTime = %s\n",i,pMyObject->pConfigFileContainer->pConfigFileTable[i].PreviousSyncDateTime);
		    }
	    }
	    WebcfgDebug("##### ConfigFile container data #####\n");
	}
/*	else
	{
	    WebConfigLog("RFC disabled. Hence not loading ConfigFile entries\n");
	    pMyObject->PeriodicSyncCheckInterval = 0;
	    pMyObject->pConfigFileContainer = NULL;
	}*/
    WebcfgDebug("#### CosaWebConfigInitialize done. return %d\n", returnStatus);

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
	WebcfgDebug("-------- %s ----- Enter ------\n",__FUNCTION__);
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
	WebcfgDebug("-------- %s ----- EXIT ------\n",__FUNCTION__);
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
    int configFileCount = -1;
    char strInstance[516] = { 0 };
    int index = 0, i = 0, j = 0;

	pConfigFileContainer = (PCOSA_DML_CONFIGFILE_CONTAINER)AnscAllocateMemory(sizeof(COSA_DML_CONFIGFILE_CONTAINER));
    WebcfgDebug("------- %s ---------\n",__FUNCTION__);
	memset(pConfigFileContainer, 0, sizeof(COSA_DML_CONFIGFILE_CONTAINER));

    CHAR tmpbuf[ 128 ] = { 0 };
	CosaDmlGetValueFromDb("ConfigFileNumberOfEntries", tmpbuf);
    if(tmpbuf[0] != '\0')
    {
        configFileCount = atoi(tmpbuf);
        WebConfigLog("configFileCount: %d\n",configFileCount);
    }
    // Check number of entries and if 0, add default row directly to DB 
    if((configFileCount == 0) && FillDefaultConfigFileEntryToDB())
    {
		WebConfigLog("Filled Default Config File Entry to the DB\n");
		configFileCount = 1;
    }

    pConfigFileContainer->ConfigFileEntryCount = configFileCount;
    if(configFileCount > 0)
    {
        pConfigFileContainer->pConfigFileTable = (PCOSA_DML_WEBCONFIG_CONFIGFILE_ENTRY)AnscAllocateMemory(sizeof(COSA_DML_WEBCONFIG_CONFIGFILE_ENTRY)*(configFileCount));
        memset(pConfigFileContainer->pConfigFileTable, 0, sizeof(COSA_DML_WEBCONFIG_CONFIGFILE_ENTRY)*(configFileCount));
	CosaDmlGetValueFromDb("WebConfig_IndexesList", strInstance);
        if(strInstance != NULL)
        {
            WebcfgDebug("strInstance: %s\n",strInstance);
            char *tok = strtok(strInstance, ",");
            while(tok != NULL)
            {
                index = atoi(tok);
                WebcfgDebug("index at %d: %d\n",i, index);
                WebcfgDebug("B4 CosaDmlGetConfigFileEntry\n");
                pConfigFileEntry = CosaDmlGetConfigFileEntry(index);
                WebcfgDebug("After CosaDmlGetConfigFileEntry\n");
                FillEntryInList(pMyObject, pConfigFileEntry);
                WebcfgDebug("After FillEntryInList\n");
                //pConfigFileContainer->pConfigFileTable[i] = pConfigFileEntry;
                WebcfgDebug("pConfigFileEntry->InstanceNumber: %d\n",pConfigFileEntry->InstanceNumber);
                pConfigFileContainer->pConfigFileTable[i].InstanceNumber = pConfigFileEntry->InstanceNumber;
                WebcfgDebug("pConfigFileEntry->URL: %s\n",pConfigFileEntry->URL);
                AnscCopyString(pConfigFileContainer->pConfigFileTable[i].URL, pConfigFileEntry->URL);
                WebcfgDebug("pConfigFileEntry->Version: %s\n",pConfigFileEntry->Version);
                AnscCopyString(pConfigFileContainer->pConfigFileTable[i].Version, pConfigFileEntry->Version);
                WebcfgDebug("pConfigFileEntry->ForceSyncCheck: %d\n",pConfigFileEntry->ForceSyncCheck);
                pConfigFileContainer->pConfigFileTable[i].ForceSyncCheck = pConfigFileEntry->ForceSyncCheck;
                WebcfgDebug("pConfigFileEntry->SyncCheckOK: %d\n",pConfigFileEntry->SyncCheckOK);
                pConfigFileContainer->pConfigFileTable[i].SyncCheckOK = pConfigFileEntry->SyncCheckOK;
                WebcfgDebug("pConfigFileEntry->PreviousSyncDateTime: %s\n",pConfigFileEntry->PreviousSyncDateTime);
                AnscCopyString(pConfigFileContainer->pConfigFileTable[i].PreviousSyncDateTime, pConfigFileEntry->PreviousSyncDateTime);
                i++;
                tok = strtok(NULL, ",");
            }
        }
    }
    WebcfgDebug("######### ConfigFile data : %d ########\n",configFileCount);
    for(j=0; j<configFileCount; j++)
    {
        WebcfgDebug("%d: %s %s %d %d %s\n",pConfigFileContainer->pConfigFileTable[j].InstanceNumber, pConfigFileContainer->pConfigFileTable[j].URL, pConfigFileContainer->pConfigFileTable[j].Version, pConfigFileContainer->pConfigFileTable[j].ForceSyncCheck, pConfigFileContainer->pConfigFileTable[j].SyncCheckOK, pConfigFileContainer->pConfigFileTable[j].PreviousSyncDateTime);
    }
    WebcfgDebug("######### ConfigFile data ########\n");
    WebcfgDebug("------- %s -----EXIT----\n",__FUNCTION__);
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
    WebcfgDebug("-------- %s ----- Enter ------\n",__FUNCTION__);
    PCOSA_DML_WEBCONFIG_CONFIGFILE_ENTRY pConfigFileEntry = (PCOSA_DML_WEBCONFIG_CONFIGFILE_ENTRY)AnscAllocateMemory(sizeof(COSA_DML_WEBCONFIG_CONFIGFILE_ENTRY));
    memset(pConfigFileEntry, 0, sizeof(COSA_DML_WEBCONFIG_CONFIGFILE_ENTRY));
    pConfigFileEntry->InstanceNumber = InstanceNumber;
	sprintf(ParamName, "configfile_%d_Url", InstanceNumber);
	CosaDmlGetValueFromDb(ParamName, tmpbuf);
    WebcfgDebug("Url at %d:%s\n",InstanceNumber,tmpbuf);
	AnscCopyString( pConfigFileEntry->URL, tmpbuf );

	sprintf(ParamName, "configfile_%d_Version", InstanceNumber);
	CosaDmlGetValueFromDb(ParamName, tmpbuf);
    WebcfgDebug("Version at %d:%s\n",InstanceNumber,tmpbuf);
	AnscCopyString( pConfigFileEntry->Version, tmpbuf );

	sprintf(ParamName, "configfile_%d_SyncCheckOk", InstanceNumber);
	CosaDmlGetValueFromDb(ParamName, tmpbuf);
    WebcfgDebug("SyncCheckOK at %d:%s\n",InstanceNumber,tmpbuf);
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
	WebcfgDebug("PreviousSyncDateTime at %d:%s\n",InstanceNumber,tmpbuf);
	AnscCopyString(pConfigFileEntry->PreviousSyncDateTime,tmpbuf);

    WebcfgDebug("-------- %s ----- EXIT ------\n",__FUNCTION__);
    return pConfigFileEntry;
}

ANSC_STATUS
CosaDmlSetConfigFileEntry
(
	PCOSA_DML_WEBCONFIG_CONFIGFILE_ENTRY configFileEntry
)
{
	char ParamName[128] = { 0 };

	WebcfgDebug("-------- %s ----- Enter ------\n",__FUNCTION__);
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

	WebcfgDebug("-------- %s ----- Exit ------\n",__FUNCTION__);
	return ANSC_STATUS_SUCCESS;
}

void FillEntryInList(PCOSA_DATAMODEL_WEBCONFIG pWebConfig, PCOSA_DML_WEBCONFIG_CONFIGFILE_ENTRY configFileEntry)
{
	PCOSA_CONTEXT_WEBCONFIG_LINK_OBJECT   pWebConfigCxtLink = NULL;
    WebcfgDebug("-------- %s ----- Enter ------\n",__FUNCTION__);
    pWebConfigCxtLink = (PCOSA_CONTEXT_WEBCONFIG_LINK_OBJECT)AnscAllocateMemory(sizeof(COSA_CONTEXT_WEBCONFIG_LINK_OBJECT));
    if ( !pWebConfigCxtLink )
    {
        fprintf(stderr, "Allocation failed \n");
        return;
    }

	pWebConfigCxtLink->InstanceNumber =  configFileEntry->InstanceNumber;
	pWebConfigCxtLink->hContext = (ANSC_HANDLE)configFileEntry;
	CosaSListPushEntryByInsNum(&pWebConfig->ConfigFileList, (PCOSA_CONTEXT_LINK_OBJECT)pWebConfigCxtLink);
	WebcfgDebug("-------- %s ----- Exit ------\n",__FUNCTION__);
}

ANSC_STATUS
CosaDmlRemoveConfigFileEntry
    (
        ULONG InstanceNumber
    )
{
	char ParamName[128] = { 0 };
    WebcfgDebug("-------- %s ----- Enter ------\n",__FUNCTION__);
    sprintf(ParamName, "configfile_%d_Url", InstanceNumber);
	CosaDmlRemoveValueFromDb(ParamName);

	sprintf(ParamName, "configfile_%d_Version", InstanceNumber);
	CosaDmlRemoveValueFromDb(ParamName);

	sprintf(ParamName, "configfile_%d_SyncCheckOk", InstanceNumber);
	CosaDmlRemoveValueFromDb(ParamName);

	sprintf(ParamName, "configfile_%d_SyncDateTime", InstanceNumber);
	CosaDmlRemoveValueFromDb(ParamName);
	
    WebConfigLog("Remove %d from WebConfig_IndexesList\n",InstanceNumber);
    RemoveEntryFromIndexesList(InstanceNumber);
    WebcfgDebug("-------- %s ----- Exit ------\n",__FUNCTION__);
    return ANSC_STATUS_SUCCESS;
}

void appendToIndexesList(char *index, char *indexesList)
{
    WebcfgDebug("-------- %s ----- Enter ------\n",__FUNCTION__);
    if(indexesList[0] == '\0')
    {
        sprintf(indexesList, "%s", index);
    }
    else
    {
        sprintf(indexesList, "%s,%s",indexesList,index);
    }
    WebcfgDebug("-------- %s ----- Exit ------\n",__FUNCTION__);
}

void RemoveEntryFromIndexesList(ULONG InstanceNumber)
{
    int index = 0;
    char* st;
    char newList[516]={};
    char *IndexsList = NULL;
    char strInstance[516] = { 0 };
    WebcfgDebug("-------%s------- ENTER ------\n",__FUNCTION__);
    CosaDmlGetValueFromDb("WebConfig_IndexesList", strInstance);
    if(strInstance[0] != '\0')
    {
        IndexsList = strdup(strInstance);
        if(strInstance != NULL)
        {
            WebcfgDebug("strInstance: %s\n",strInstance);
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
                    WebcfgDebug("Final newList: %s\n",newList);
                    CosaDmlStoreValueIntoDb("WebConfig_IndexesList",newList);
                    return;
                }
                appendToIndexesList(tok, newList);
                WebcfgDebug("newList: %s\n",newList);
                tok = strtok_r(NULL, ",",&st);
            }
        }
    }
    WebcfgDebug("-------%s------- EXIT ------\n",__FUNCTION__);
}

void CosaDmlGetValueFromDb( char* ParamName, char* pString )
{
    CHAR tmpbuf[ 516 ] = { 0 };
#ifdef RDKB_BUILD
	if( 0 != syscfg_get( NULL, ParamName, tmpbuf, sizeof(tmpbuf)))
	{
		WebConfigLog("syscfg_get failed %s\n", ParamName);
	}
#endif	
	AnscCopyString( pString, tmpbuf );
}

void CosaDmlStoreValueIntoDb(char *ParamName, char *pString)
{
#ifdef RDKB_BUILD
    if ( 0 != syscfg_set(NULL, ParamName , pString ) ) 
	{
		WebConfigLog("syscfg_set failed %s\n", ParamName);
	}
	else 
	{
		if ( 0 != syscfg_commit( ) ) 
		{
			WebConfigLog("syscfg_commit failed %s\n", ParamName);
		}
	}
#endif
}

void CosaDmlRemoveValueFromDb(char *ParamName)
{
#ifdef RDKB_BUILD
    if ( 0 != syscfg_unset(NULL, ParamName) ) 
	{
		WebConfigLog("syscfg_unset failed %s\n", ParamName);
	}
	else 
	{
		if ( 0 != syscfg_commit( ) ) 
		{
			WebConfigLog("syscfg_commit failed %s\n", ParamName);
		}
	}
#endif
}

void updateConfigFileNumberOfEntries(ULONG count)
{
	char buf[16] = { 0 };
	WebcfgDebug("-------- %s ----- Enter ------\n",__FUNCTION__);
	sprintf(buf, "%d", count);
	WebConfigLog("Updated count:%d\n",count);
	CosaDmlStoreValueIntoDb("ConfigFileNumberOfEntries",buf);
	WebcfgDebug("-------- %s ----- Exit ------\n",__FUNCTION__);
}

void updateConfigFileIndexsList(ULONG index)
{
	char strInstance[516] = { 0 };
	char instance[16] = { 0 };
	WebcfgDebug("-------- %s ----- Enter ------\n",__FUNCTION__);
	CosaDmlGetValueFromDb("WebConfig_IndexesList", strInstance);
	sprintf(instance, "%d", index);
	appendToIndexesList(instance, strInstance);
    WebConfigLog("Updated list: %s\n",strInstance);
    CosaDmlStoreValueIntoDb("WebConfig_IndexesList",strInstance);
	WebcfgDebug("-------- %s ----- Exit ------\n",__FUNCTION__);
}

void updateConfigFileNextInstanceNumber(ULONG index)
{
    char buf[16] = { 0 };
	WebcfgDebug("-------- %s ----- Enter ------\n",__FUNCTION__);
	sprintf(buf, "%d", index);
	WebConfigLog("Updated NextInstanceNumber:%d\n",index);
	CosaDmlStoreValueIntoDb("WebConfig_NextInstanceNumber",buf);
	WebcfgDebug("-------- %s ----- Exit ------\n",__FUNCTION__);
}

int initConfigFileWithURL(char *Url, ULONG InstanceNumber)
{
    WebcfgDebug("-------- %s ----- Enter ------\n",__FUNCTION__);
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
	    WebcfgDebug("configCount: %d\n",configCount);
	    updateConfigFileNumberOfEntries(configCount);
	    updateConfigFileIndexsList(InstanceNumber);
	    updateConfigFileNextInstanceNumber(InstanceNumber+1);
	    CosaDmlSetConfigFileEntry(pConfigFileEntry);
	    WebcfgDebug("-------- %s ----- Exit ------\n",__FUNCTION__);
	    return 0;
	}
	WebcfgDebug("-------- %s ----- Exit ------\n",__FUNCTION__);
	return 1;
}

//loadInitURLFromFile
static void loadInitURLFromFile(char **url)
{
	FILE *fp = fopen(DEVICE_PROPS_FILE, "r");

	if (NULL != fp)
	{
		char str[255] = {'\0'};
		while(fscanf(fp,"%s", str) != EOF)
		{
		    char *value = NULL;

		    if(NULL != (value = strstr(str, "WEBCONFIG_INIT_URL=")))
		    {
			value = value + strlen("WEBCONFIG_INIT_URL=");
			*url = strdup(value);
		    }

		}
		fclose(fp);
	}
	else
	{
		WebConfigLog("Failed to open device.properties file:%s\n", DEVICE_PROPS_FILE);
	}

	if (NULL == *url)
	{
		WebConfigLog("WebConfig url is not present in device.properties\n");
	}
	else
	{
		WebConfigLog("url fetched is %s\n", *url);
	}
}


bool FillDefaultConfigFileEntryToDB()
{
	char *url = NULL;

	WebcfgDebug("-------- %s ----- Enter ------\n",__FUNCTION__);

	loadInitURLFromFile(&url);

	if(url[0] != '\0')
	{
		CosaDmlStoreValueIntoDb("configfile_1_Url", url);
	}
	else
	{
		WebConfigLog("Could not load default url into DB\n");
		return false;
	}
	
	CosaDmlStoreValueIntoDb("configfile_1_Version", "");
	
	CosaDmlStoreValueIntoDb("configfile_1_SyncCheckOk", "false");
	
	CosaDmlStoreValueIntoDb("configfile_1_SyncDateTime", "");

	updateConfigFileNumberOfEntries(1);
	updateConfigFileIndexsList(1);
	updateConfigFileNextInstanceNumber(2);

	WebcfgDebug("-------- %s ----- Exit ------\n",__FUNCTION__);
	return true;
}
