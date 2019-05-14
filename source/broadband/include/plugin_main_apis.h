/*
 * If not stated otherwise in this file or this component's Licenses.txt file the
 * following copyright and licenses apply:
 *
 * Copyright 2015 RDK Management
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

    module: plugin_main_apis.h

        For COSA Data Model Library Development

    -------------------------------------------------------------------

    description:

        This file defines the apis for objects to support Data Model Library.

    -------------------------------------------------------------------


    author:

        COSA XML TOOL CODE GENERATOR 1.0

    -------------------------------------------------------------------

    revision:

        01/11/2011    initial revision.

**************************************************************************/


#ifndef  _PLUGIN_MAIN_APIS_H
#define  _PLUGIN_MAIN_APIS_H

#include "ansc_platform.h"
#include "cosa_apis.h"
#include "dslh_cpeco_interface.h"
#include "plugin_main_apis.h"
#include "cosa_plugin_api.h"

extern ANSC_HANDLE                        g_MessageBusHandle;
extern char*                              g_SubsystemPrefix;

/* The OID for all objects s*/
#define COSA_DATAMODEL_BASE_OID                                 0
#define COSA_DATAMODEL_WEBPA_OID                                1

/*
 * This is the cosa datamodel backend manager which is used to manager all backend object
 */
#define  COSA_BACKEND_MANAGER_CLASS_CONTENT                                                 \
    /* duplication of the base object class content */                                      \
    COSA_BASE_CONTENT                                                                       \
    /* start of NAT object class content */                                                 \
    ANSC_HANDLE                  hWebpa;                                                      \
    PCOSA_PLUGIN_INFO            hCosaPluginInfo;								   		

typedef  struct
_COSA_BACKEND_MANAGER_OBJECT
{
    COSA_BACKEND_MANAGER_CLASS_CONTENT
                                                                                      
}
COSA_BACKEND_MANAGER_OBJECT,  *PCOSA_BACKEND_MANAGER_OBJECT;

extern PCOSA_BACKEND_MANAGER_OBJECT g_pCosaBEManager;



ANSC_HANDLE
CosaBackEndManagerCreate
    (
        VOID
    );

ANSC_STATUS
CosaBackEndManagerInitialize
    (
        ANSC_HANDLE                 hThisObject
    );

ANSC_STATUS
CosaBackEndManagerRemove
    (
        ANSC_HANDLE                 hThisObject
    );

#endif
