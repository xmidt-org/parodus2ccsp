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

#define MAX_BUFF_SIZE 256

#define  COSA_CONTEXT_WEBCONFIG_LINK_CLASS_CONTENT                                  \
        COSA_CONTEXT_LINK_CLASS_CONTENT                                            \
        BOOL                            bFound;                                    \


/***********************************
    Actual definition declaration
************************************/
#define  COSA_IREP_FOLDER_NAME_WEBCONFIG                       "WebConfig"
#define  COSA_DML_RR_NAME_WebConfigNextInsNumber               "NextInstanceNumber"
#define  COSA_DML_RR_NAME_WebConfigAlias                       "Alias"
#define  COSA_DML_RR_NAME_WebConfigbNew                        "bNew"

#define   COSA_DML_CONFIGFILE_ACCESS_INTERVAL   10 /* seconds*/
typedef  struct
_COSA_CONTEXT_WEBCONFIG_LINK_OBJECT
{
    COSA_CONTEXT_WEBCONFIG_LINK_CLASS_CONTENT
}
COSA_CONTEXT_WEBCONFIG_LINK_OBJECT,  *PCOSA_CONTEXT_WEBCONFIG_LINK_OBJECT;

typedef  struct
_COSA_DML_WEBCONFIG_CONFIGFILE_ENTRY
{
    ULONG                           InstanceNumber;
    char                            URL[256];
    char                            Version[64];
    BOOLEAN                         ForceSyncCheck;
    BOOLEAN                         SyncCheckOK;
    char                            PreviousSyncDateTime[64];
}
COSA_DML_WEBCONFIG_CONFIGFILE_ENTRY,  *PCOSA_DML_WEBCONFIG_CONFIGFILE_ENTRY;

typedef  struct
_PCOSA_DML_CONFIGFILE_CONTAINER
{
    ULONG                      	    ConfigFileEntryCount;                                    
    PCOSA_DML_WEBCONFIG_CONFIGFILE_ENTRY    pConfigFileTable;    
}
COSA_DML_CONFIGFILE_CONTAINER,  *PCOSA_DML_CONFIGFILE_CONTAINER;

#define  COSA_DATAMODEL_WEBCONFIG_CLASS_CONTENT                                                  \
    /* duplication of the base object class content */                                      \
    COSA_BASE_CONTENT                                                                       \
	ULONG                       MaxInstanceNumber;                                    \
	ULONG                       ulWebConfigNextInstanceNumber;                                    \
	ULONG                           PreviousVisitTime;                                      \
    BOOL                        RfcEnable;                                         \
    int                     PeriodicSyncCheckInterval;                      \
    SLIST_HEADER                ConfigFileList;                                        \
    PCOSA_DML_CONFIGFILE_CONTAINER    pConfigFileContainer;                                        \
	ANSC_HANDLE                     hIrepFolderWebConfig;                                         \
    ANSC_HANDLE                     hIrepFolderWebConfigMapCont;                                       \


typedef  struct
_COSA_DATAMODEL_WEBCONFIG
{
    COSA_DATAMODEL_WEBCONFIG_CLASS_CONTENT
}
COSA_DATAMODEL_WEBCONFIG,  *PCOSA_DATAMODEL_WEBCONFIG;

#define  ACCESS_COSA_CONTEXT_WEBCONFIG_LINK_OBJECT(p)              \
         ACCESS_CONTAINER(p, COSA_CONTEXT_WEBCONFIG_LINK_OBJECT, Linkage)

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

PCOSA_DML_CONFIGFILE_CONTAINER
CosaDmlGetConfigFile(    
        ANSC_HANDLE                 hThisObject
    );

PCOSA_DML_WEBCONFIG_CONFIGFILE_ENTRY
CosaDmlGetConfigFileEntry
    (
        ULONG InstanceNumber
    );


ANSC_STATUS
CosaDmlSetConfigFileEntry
    (
	    PCOSA_DML_WEBCONFIG_CONFIGFILE_ENTRY configFileEntry
    );

ANSC_STATUS
CosaDmlRemoveConfigFileEntry
    (
        ULONG InstanceNumber
    );


int initConfigFileWithURL(char *Url, ULONG InstanceNumber);
