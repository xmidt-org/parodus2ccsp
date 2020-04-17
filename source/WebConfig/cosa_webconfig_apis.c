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
#include <webcfg_log.h>

int initConfigFileWithURL(char *Url, ULONG InstanceNumber);
void CosaDmlRemoveValueFromDb(char *ParamName);
void CosaDmlGetValueFromDb( char* ParamName, char* pString );

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
	
	WebcfgDebug("------ pMyObject -------\n");
	WebcfgDebug("pMyObject->RfcEnable: %d\n",pMyObject->RfcEnable);
	WebcfgDebug("pMyObject->ForceSync: %s\n",pMyObject->ForceSync);
	
	WebcfgDebug("------ pMyObject -------\n");
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
    char URL[256] = { 0 };
    BOOL bValue = FALSE;
    WebcfgDebug("CosaDmlGetRFCEnableFromDB\n");
    CosaDmlGetRFCEnableFromDB(&bValue);
    if(bValue == TRUE)
    {
        pMyObject->RfcEnable = true;
    }
    else
    {
        pMyObject->RfcEnable = false;
    }
    WebcfgInfo("pMyObject->RfcEnable: %d\n",pMyObject->RfcEnable);
	/*Removing RFC check for now as data re-load is not yet implemented*/
    //if(pMyObject->RfcEnable == true)
    //{
    
	_ansc_memset(pMyObject->ForceSync, 0, 256);
	AnscCopyString( pMyObject->ForceSync, "" );

	_ansc_memset(pMyObject->ForceSyncTransID, 0, 256);
	AnscCopyString( pMyObject->ForceSyncTransID, "" );
        WebcfgDebug("pMyObject->ForceSync:%s\n",pMyObject->ForceSync);
	WebcfgDebug("pMyObject->ForceSyncTransID:%s\n",pMyObject->ForceSync);

	_ansc_memset(pMyObject->URL, 0, 256);
	Get_Webconfig_URL(URL);
	if( (URL !=NULL) && strlen(URL)>0 )
	{
		AnscCopyString(pMyObject->URL, URL);
		WebcfgInfo("pMyObject->URL:%s\n", pMyObject->URL);
	}
	WebcfgDebug("URL initialization done\n");
/*	else
	{
	    WebcfgInfo("RFC disabled. Hence not loading ConfigFile entries\n");
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
    
    /* Remove self */
    AnscFreeMemory((ANSC_HANDLE)pMyObject);
	WebcfgDebug("-------- %s ----- EXIT ------\n",__FUNCTION__);
    return returnStatus;
}

