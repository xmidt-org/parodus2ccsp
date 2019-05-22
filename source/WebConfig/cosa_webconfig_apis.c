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
//#include <syscfg/syscfg.h>
#include "plugin_main_apis.h"
#include "cosa_webconfig_apis.h"

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
	WalInfo("-------- %s ----- Enter ------\n",__FUNCTION__);
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
	
	WalInfo("------ pMyObject -------\n");
	WalInfo("pMyObject->RfcEnable: %d\n",pMyObject->RfcEnable);
	WalInfo("pMyObject->PeriodicSyncCheckInterval: %d\n",pMyObject->PeriodicSyncCheckInterval);
	if(pMyObject->pConfigFileContainer != NULL)
	{
		WalInfo("pMyObject->pConfigFileContainer->ConfigFileEntryCount: %d\n",pMyObject->pConfigFileContainer->ConfigFileEntryCount);
		if(pMyObject->pConfigFileContainer->pConfigFileTable != NULL)
		{
			for(i=0; i<pMyObject->pConfigFileContainer->ConfigFileEntryCount; i++)
			{
				WalInfo("pMyObject->pConfigFileContainer->pConfigFileTable[%d].InstanceNumber = %d\n",i,pMyObject->pConfigFileContainer->pConfigFileTable[i].InstanceNumber);
				WalInfo("pMyObject->pConfigFileContainer->pConfigFileTable[%d].URL = %s\n",i,pMyObject->pConfigFileContainer->pConfigFileTable[i].URL);
				WalInfo("pMyObject->pConfigFileContainer->pConfigFileTable[%d].Version = %s\n",i,pMyObject->pConfigFileContainer->pConfigFileTable[i].Version);
				WalInfo("pMyObject->pConfigFileContainer->pConfigFileTable[%d].ForceSyncCheck = %d\n",i,pMyObject->pConfigFileContainer->pConfigFileTable[i].ForceSyncCheck);
				WalInfo("pMyObject->pConfigFileContainer->pConfigFileTable[%d].SyncCheckOK = %d\n",i,pMyObject->pConfigFileContainer->pConfigFileTable[i].SyncCheckOK);
				WalInfo("pMyObject->pConfigFileContainer->pConfigFileTable[%d].PreviousSyncDateTime = %s\n",i,pMyObject->pConfigFileContainer->pConfigFileTable[i].PreviousSyncDateTime);
			}
		}
	}
	WalInfo("------ pMyObject -------\n");
	WalInfo("-------- %s ----- Exit ------\n",__FUNCTION__);
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
	WalInfo("-------- %s ----- Enter ------\n",__FUNCTION__);
    AnscSListInitializeHeader( &pMyObject->ConfigFileList );
    pMyObject->MaxInstanceNumber        = 0;
    pMyObject->ulWebConfigNextInstanceNumber   = 1;
#ifdef RDKB_BUILD
    CHAR tmpbuf[ 128 ] = { 0 };
    WalInfo("------- %s ---------\n",__FUNCTION__);
    // Initialize syscfg to make syscfg calls
    if (0 != syscfg_init())
    {
    	WalError("CosaWebConfigInitialize Error: syscfg_init() failed!! \n");
    	return ANSC_STATUS_FAILURE;
    }
    else
    {
        if( 0 == syscfg_get( NULL, "RfcEnable", tmpbuf, sizeof(tmpbuf)))
        {
            if( tmpbuf != NULL && AnscEqualString(tmpbuf, "true", TRUE))
            {
                pMyObject->RfcEnable = true;
            }
            else
            {
                pMyObject->RfcEnable = false;
            }
            WalInfo("pMyObject->RfcEnable : %d\n",pMyObject->RfcEnable);
        }
        if( 0 == syscfg_get( NULL, "PeriodicSyncCheckInterval", tmpbuf, sizeof(tmpbuf)))
        {
            if(tmpbuf != NULL)
            {
                pMyObject->PeriodicSyncCheckInterval = atoi(tmpbuf);
            }
            WalInfo("pMyObject->PeriodicSyncCheckInterval:%d\n",pMyObject->PeriodicSyncCheckInterval);
        }
    }
#endif
    WalInfo("B4 CosaDmlGetConfigFile\n");
    pMyObject->pConfigFileContainer = CosaDmlGetConfigFile((ANSC_HANDLE)pMyObject);
    WalInfo("After CosaDmlGetConfigFile\n");
	WalInfo("##### ConfigFile container data #####\n");
	WalInfo("pMyObject->pConfigFileContainer->ConfigFileEntryCount: %d\n",pMyObject->pConfigFileContainer->ConfigFileEntryCount);
	int i = 0;
	if(pMyObject->pConfigFileContainer->pConfigFileTable != NULL)
	{
		for(i=0; i<pMyObject->pConfigFileContainer->ConfigFileEntryCount; i++)
		{
			WalInfo("pMyObject->pConfigFileContainer->pConfigFileTable[%d].InstanceNumber = %d\n",i,pMyObject->pConfigFileContainer->pConfigFileTable[i].InstanceNumber);
			WalInfo("pMyObject->pConfigFileContainer->pConfigFileTable[%d].URL = %s\n",i,pMyObject->pConfigFileContainer->pConfigFileTable[i].URL);
			WalInfo("pMyObject->pConfigFileContainer->pConfigFileTable[%d].Version = %s\n",i,pMyObject->pConfigFileContainer->pConfigFileTable[i].Version);
			WalInfo("pMyObject->pConfigFileContainer->pConfigFileTable[%d].ForceSyncCheck = %d\n",i,pMyObject->pConfigFileContainer->pConfigFileTable[i].ForceSyncCheck);
			WalInfo("pMyObject->pConfigFileContainer->pConfigFileTable[%d].SyncCheckOK = %d\n",i,pMyObject->pConfigFileContainer->pConfigFileTable[i].SyncCheckOK);
			WalInfo("pMyObject->pConfigFileContainer->pConfigFileTable[%d].PreviousSyncDateTime = %s\n",i,pMyObject->pConfigFileContainer->pConfigFileTable[i].PreviousSyncDateTime);
		}
	}
	WalInfo("##### ConfigFile container data #####\n");
    WalInfo("#### CosaWebConfigInitialize done. return %d\n", returnStatus);

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
	WalInfo("-------- %s ----- Enter ------\n",__FUNCTION__);
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
	WalInfo("-------- %s ----- Enter ------\n",__FUNCTION__);
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
    WalInfo("------- %s ---------\n",__FUNCTION__);
	memset(pConfigFileContainer, 0, sizeof(PCOSA_DML_CONFIGFILE_CONTAINER));
#ifdef RDKB_BUILD
    CHAR tmpbuf[ 128 ] = { 0 };
    if( 0 == syscfg_get( NULL, "ConfigFileNumberOfEntries", tmpbuf, sizeof(tmpbuf)))
    {
        if(tmpbuf != NULL)
        {
            configFileCount = atoi(tmpbuf);
            WalInfo("configFileCount: %d\n",configFileCount);
        }
    }
    pConfigFileContainer->ConfigFileEntryCount = configFileCount;
    if(configFileCount > 0)
    {
        pConfigFileContainer->pConfigFileTable = (PCOSA_DML_WEBCONFIG_CONFIGFILE_ENTRY)AnscAllocateMemory(sizeof(PCOSA_DML_WEBCONFIG_CONFIGFILE_ENTRY)*(configFileCount));
        if( 0 == syscfg_get( NULL, "WebConfig_IndexesList", strInstance, sizeof(strInstance)))
        {
            if(strInstance != NULL)
            {
                WalInfo("strInstance: %s\n",strInstance);
                char *tok = strtok(strInstance, ",");
                while(tok != NULL)
                {
                    index = atoi(tok);
                    WalInfo("index at %d: %d\n",i, index);
                    WalInfo("B4 CosaDmlGetConfigFileEntry\n");
                    pConfigFileEntry = CosaDmlGetConfigFileEntry(index);
                    WalInfo("After CosaDmlGetConfigFileEntry\n");
                    //pConfigFileContainer->pConfigFileTable[i] = pConfigFileEntry;
                    WalInfo("pConfigFileEntry->InstanceNumber: %d\n",pConfigFileEntry->InstanceNumber);
                    pConfigFileContainer->pConfigFileTable[i].InstanceNumber = pConfigFileEntry->InstanceNumber;
                    WalInfo("pConfigFileEntry->URL: %s\n",pConfigFileEntry->URL);
                    AnscCopyString(pConfigFileContainer->pConfigFileTable[i].URL, pConfigFileEntry->URL);
                    WalInfo("pConfigFileEntry->Version: %s\n",pConfigFileEntry->Version);
                    AnscCopyString(pConfigFileContainer->pConfigFileTable[i].Version, pConfigFileEntry->Version);
                    WalInfo("pConfigFileEntry->ForceSyncCheck: %d\n",pConfigFileEntry->ForceSyncCheck);
                    pConfigFileContainer->pConfigFileTable[i].ForceSyncCheck = pConfigFileEntry->ForceSyncCheck;
                    WalInfo("pConfigFileEntry->SyncCheckOK: %d\n",pConfigFileEntry->SyncCheckOK);
                    pConfigFileContainer->pConfigFileTable[i].SyncCheckOK = pConfigFileEntry->SyncCheckOK;
                    WalInfo("pConfigFileEntry->PreviousSyncDateTime: %s\n",pConfigFileEntry->PreviousSyncDateTime);
                    pConfigFileContainer->pConfigFileTable[i].PreviousSyncDateTime = AnscCloneString(pConfigFileEntry->PreviousSyncDateTime);
                    i++;
                    tok = strtok(NULL, ",");
                }
            }
        }
    }
#endif
    WalInfo("######### ConfigFile data : %d ########\n",configFileCount);
    for(j=0; j<configFileCount; j++)
    {
        WalInfo("%d: %s %s %d %d %s\n",pConfigFileContainer->pConfigFileTable[j].InstanceNumber, pConfigFileContainer->pConfigFileTable[j].URL, pConfigFileContainer->pConfigFileTable[j].Version, pConfigFileContainer->pConfigFileTable[j].ForceSyncCheck, pConfigFileContainer->pConfigFileTable[j].SyncCheckOK, pConfigFileContainer->pConfigFileTable[j].PreviousSyncDateTime);
    }
    WalInfo("######### ConfigFile data ########\n");
    WalInfo("------- %s ---------\n",__FUNCTION__);
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
    WalInfo("------- %s ---------\n",__FUNCTION__);
    PCOSA_DML_WEBCONFIG_CONFIGFILE_ENTRY pConfigFileEntry = (PCOSA_DML_WEBCONFIG_CONFIGFILE_ENTRY)AnscAllocateMemory(sizeof(PCOSA_DML_WEBCONFIG_CONFIGFILE_ENTRY));
    
    pConfigFileEntry->InstanceNumber = InstanceNumber;
#ifdef RDKB_BUILD
	sprintf(ParamName, "configfile_%d_Url", InstanceNumber);
	if( 0 == syscfg_get( NULL, ParamName, tmpbuf, sizeof(tmpbuf)))
	{
	    WalInfo("Url at %d:%s\n",InstanceNumber,tmpbuf);
		AnscCopyString( pConfigFileEntry->URL, tmpbuf );
	}

	sprintf(ParamName, "configfile_%d_Version", InstanceNumber);
	if( 0 == syscfg_get( NULL, ParamName, tmpbuf, sizeof(tmpbuf)))
	{
	    WalInfo("Version at %d:%s\n",InstanceNumber,tmpbuf);
		AnscCopyString( pConfigFileEntry->Version, tmpbuf );
	}

	sprintf(ParamName, "configfile_%d_ForceSyncCheck", InstanceNumber);
	if( 0 == syscfg_get( NULL, ParamName, tmpbuf, sizeof(tmpbuf)))
	{
	    WalInfo("ForceSyncCheck at %d:%s\n",InstanceNumber,tmpbuf);
		if(strcmp( tmpbuf, "true" ) == 0)
	    {
		    pConfigFileEntry->ForceSyncCheck = true;
	    }
	    else
	    {
	        pConfigFileEntry->ForceSyncCheck = false;
	    }
	}

	sprintf(ParamName, "configfile_%d_SyncCheckOk", InstanceNumber);
	if( 0 == syscfg_get( NULL, ParamName, tmpbuf, sizeof(tmpbuf)))
	{
	    WalInfo("SyncCheckOK at %d:%s\n",InstanceNumber,tmpbuf);
	    if(strcmp( tmpbuf, "true" ) == 0)
	    {
		    pConfigFileEntry->SyncCheckOK = true;
	    }
	    else
	    {
	        pConfigFileEntry->SyncCheckOK = false;
	    }
    }

	sprintf(ParamName, "configfile_%d_SyncDateTime", InstanceNumber);
	if( 0 == syscfg_get( NULL, ParamName, tmpbuf, sizeof(tmpbuf)))
	{
		WalInfo("PreviousSyncDateTime at %d:%s\n",InstanceNumber,tmpbuf);
		pConfigFileEntry->PreviousSyncDateTime=strndup(tmpbuf,sizeof(tmpbuf));
	}
#endif
    WalInfo("------- %s ---------\n",__FUNCTION__);
    return pConfigFileEntry;
}
