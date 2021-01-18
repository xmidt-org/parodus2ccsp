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

#include "cosa_apis.h"
#include "dslh_definitions_tr143.h"
#include "webpa_adapter.h"

#define MAX_BUFF_SIZE 256                                    \


/***********************************
    Actual definition declaration
************************************/
#define  COSA_IREP_FOLDER_NAME_WEBCONFIG                       "WebConfig"
#define  COSA_DML_RR_NAME_WebConfigNextInsNumber               "NextInstanceNumber"
#define  COSA_DML_RR_NAME_WebConfigAlias                       "Alias"
#define  COSA_DML_RR_NAME_WebConfigbNew                        "bNew"

#define   COSA_DML_CONFIGFILE_ACCESS_INTERVAL   10 /* seconds*/

#define DEVICE_PROPS_FILE          "/etc/device.properties"

#define  COSA_DATAMODEL_WEBCONFIG_CLASS_CONTENT                                                  \
    /* duplication of the base object class content */                                      \
    COSA_BASE_CONTENT                                                                       \
	ULONG                       MaxInstanceNumber;                                    \
	ULONG                       ulWebConfigNextInstanceNumber;                                    \
	ULONG                           PreviousVisitTime;                                      \
    BOOL                        RfcEnable;                                         \
    char                        URL[256];                                    \
    char 		    ForceSync[256];					\
    char 		    ForceSyncTransID[256];					\
    char                        Telemetry[256];						\
	ANSC_HANDLE                     hIrepFolderWebConfig;                                         \
    ANSC_HANDLE                     hIrepFolderWebConfigMapCont;                                       \


typedef  struct
_COSA_DATAMODEL_WEBCONFIG
{
    COSA_DATAMODEL_WEBCONFIG_CLASS_CONTENT
}
COSA_DATAMODEL_WEBCONFIG,  *PCOSA_DATAMODEL_WEBCONFIG;


/**********************************
    Standard function declaration
***********************************/
ANSC_HANDLE
CosaWebConfigCreate
    (
        VOID
    );

ANSC_STATUS
CosaWebConfigInitialize
    (
        ANSC_HANDLE                 hThisObject
    );

ANSC_STATUS
CosaWebConfigRemove
    (
        ANSC_HANDLE                 hThisObject
    );

void CosaDmlStoreValueIntoDb(char *ParamName, char *pString);
