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
        *  CosaWebConfiggRemove
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
    // Initialize syscfg to make syscfg calls
    if (0 != syscfg_init())
    {
    	WalError("CosaWebConfigInitialize Error: syscfg_init() failed!! \n");
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

	pConfigFileContainer = (PCOSA_DML_CONFIGFILE_CONTAINER)AnscAllocateMemory(sizeof(COSA_DML_CONFIGFILE_CONTAINER));

    //TODO

	return pConfigFileContainer;
}

