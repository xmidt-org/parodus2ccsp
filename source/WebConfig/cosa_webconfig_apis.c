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
	PCOSA_DML_CONFIGFILE_CONTAINER    pConfigFileContainer            = (PCOSA_DML_CONFIGFILE_CONTAINER)NULL;

    AnscSListInitializeHeader( &pMyObject->ConfigFileList );
    pMyObject->MaxInstanceNumber        = 0;
    pMyObject->ulWebConfigNextInstanceNumber   = 1;
#ifdef RDKB_BUILD
    CHAR tmpbuf[ 128 ] = { 0 };
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
        }
        if( 0 == syscfg_get( NULL, "PeriodicSyncCheckInterval", tmpbuf, sizeof(tmpbuf)))
        {
            if(tmpbuf != NULL)
            {
                pMyObject->PeriodicSyncCheckInterval = atoi(tmpbuf);
            }
        }
    }
#endif
    pMyObject->pConfigFileContainer = CosaDmlGetConfigFile((ANSC_HANDLE)pMyObject);

    WalInfo("#### CosaWebConfigInitialize done. return %d", returnStatus);

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
    pConfigFileEntry = (PCOSA_DML_WEBCONFIG_CONFIGFILE_ENTRY)AnscAllocateMemory(sizeof(PCOSA_DML_WEBCONFIG_CONFIGFILE_ENTRY));
#ifdef RDKB_BUILD
    if(pMyObject->RfcEnable == true)
    {
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
        pConfigFileContainer->pConfigFileTable = (PCOSA_DML_WEBCONFIG_CONFIGFILE_ENTRY)AnscAllocateMemory(sizeof(PCOSA_DML_WEBCONFIG_CONFIGFILE_ENTRY)*(configFileCount));
        if( 0 == syscfg_get( NULL, "WebConfig_IndexesList", strInstance, sizeof(strInstance)))
        {
            if(strInstance != NULL)
            {
                char *tok = strtok(strInstance, ",");
                while(tok != NULL)
                {
                    index = atoi(tok);
                    WalInfo("index at %d: %d\n",i, index);
                    if(ANSC_STATUS_SUCCESS == CosaDmlGetConfigFileEntry(index, pConfigFileEntry))
                    {
                       //pConfigFileContainer->pConfigFileTable[i] = pConfigFileEntry;
                       pConfigFileContainer->pConfigFileTable[i].InstanceNumber = pConfigFileEntry->InstanceNumber;
                       AnscCopyString(pConfigFileContainer->pConfigFileTable[i].URL, pConfigFileEntry->URL);
                       AnscCopyString(pConfigFileContainer->pConfigFileTable[i].Version, pConfigFileEntry->Version);
                       pConfigFileContainer->pConfigFileTable[i].ForceSyncCheck = pConfigFileEntry->ForceSyncCheck;
                       pConfigFileContainer->pConfigFileTable[i].SyncCheckOK = pConfigFileEntry->SyncCheckOK;
                       AnscCopyString(pConfigFileContainer->pConfigFileTable[i].DocVersionSyncSuccessDateTime, pConfigFileEntry->DocVersionSyncSuccessDateTime);
                       i++;
                    }
                    tok = strtok(NULL, ",");
                }
            }
        }
    }
#endif
    WalInfo("######### ConfigFile data ########\n");
    for(j=0; j<configFileCount; j++)
    {
        WalInfo("%d: %s %s %d %d %s\n",pConfigFileContainer->pConfigFileTable[j].InstanceNumber, pConfigFileContainer->pConfigFileTable[j].URL, pConfigFileContainer->pConfigFileTable[j].Version, pConfigFileContainer->pConfigFileTable[j].ForceSyncCheck, pConfigFileContainer->pConfigFileTable[j].SyncCheckOK, pConfigFileContainer->pConfigFileTable[j].DocVersionSyncSuccessDateTime);
    }
    WalInfo("######### ConfigFile data ########\n");

	return pConfigFileContainer;
}

ANSC_STATUS
CosaDmlGetConfigFileEntry
    (
        ULONG InstanceNumber,
        PCOSA_DML_WEBCONFIG_CONFIGFILE_ENTRY   pConfigFileEntry 
    )
{
    CHAR tmpbuf[ 128 ] = { 0 };
	char ParamName[128] = { 0 };
    WalInfo("------- %s ---------\n",__FUNCTION__);
    pConfigFileEntry->InstanceNumber = InstanceNumber;
#ifdef RDKB_BUILD
	sprintf(ParamName, "configfile_%d url", InstanceNumber);
	if( 0 == syscfg_get( NULL, ParamName, tmpbuf, sizeof(tmpbuf)))
	{
	    WalInfo("Url at %d:%s\n",InstanceNumber,tmpbuf);
		AnscCopyString( pConfigFileEntry->URL, tmpbuf );
	}

	sprintf(ParamName, "configfile_%d version", InstanceNumber);
	if( 0 == syscfg_get( NULL, ParamName, tmpbuf, sizeof(tmpbuf)))
	{
	    WalInfo("Version at %d:%s\n",InstanceNumber,tmpbuf);
		AnscCopyString( pConfigFileEntry->Version, tmpbuf );
	}

	sprintf(ParamName, "configfile_%d force_sync_check", InstanceNumber);
	if( 0 == syscfg_get( NULL, ParamName, tmpbuf, sizeof(tmpbuf)))
	{
	    WalInfo("ForceSyncCheck at %d:%s\n",InstanceNumber,tmpbuf);
		if(strcmp( tmpbuf, "true" ) == 0)
	    {
		    pConfigFileEntry->ForceSyncCheck = TRUE;
	    }
	}

	sprintf(ParamName, "configfile_%d sync_check_ok", InstanceNumber);
	if( 0 == syscfg_get( NULL, ParamName, tmpbuf, sizeof(tmpbuf)))
	{
	    WalInfo("SyncCheckOK at %d:%s\n",InstanceNumber,tmpbuf);
	    if(strcmp( tmpbuf, "true" ) == 0)
	    {
		    pConfigFileEntry->SyncCheckOK = TRUE;
	    }
    }

	sprintf(ParamName, "configfile_%d sync_success_time", InstanceNumber);
	if( 0 == syscfg_get( NULL, ParamName, tmpbuf, sizeof(tmpbuf)))
	{
		WalInfo("DocVersionSyncSuccessDateTime at %d:%s\n",InstanceNumber,tmpbuf);
		AnscCopyString( pConfigFileEntry->DocVersionSyncSuccessDateTime, tmpbuf );
	}
#endif
    WalInfo("------- %s ---------\n",__FUNCTION__);
    return ANSC_STATUS_SUCCESS;
}
