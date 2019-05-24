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
#include "cosa_webconfig_apis.h"
#include "cosa_webconfig_dml.h"
#include "plugin_main_apis.h"
#include "webconfig_log.h"

/***********************************************************************

 APIs for Object:

    X_RDK_WebConfig.

    *  X_RDK_WebConfig_GetParamBoolValue
    *  X_RDK_WebConfig_SetParamBoolValue
    *  X_RDK_WebConfig_GetParamIntValue
    *  X_RDK_WebConfig_SetParamIntValue
    *  X_RDK_WebConfig_GetParamUlongValue

***********************************************************************/
BOOL
X_RDK_WebConfig_GetParamBoolValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        BOOL*                       pBool
    )
{
    PCOSA_DATAMODEL_WEBCONFIG            pMyObject           = (PCOSA_DATAMODEL_WEBCONFIG)g_pCosaBEManager->hWebConfig;
    WebConfigLog("------- %s ----- ENTER ----\n",__FUNCTION__);
    if( AnscEqualString(ParamName, "RfcEnable", TRUE))
    {
        /* collect value */
        *pBool = pMyObject->RfcEnable;
        return TRUE;
    }
	WebConfigLog("------- %s ----- EXIT ----\n",__FUNCTION__);
    return FALSE;
}

BOOL
X_RDK_WebConfig_SetParamBoolValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        BOOL                        bValue
    )
{
	PCOSA_DATAMODEL_WEBCONFIG            pMyObject           = (PCOSA_DATAMODEL_WEBCONFIG)g_pCosaBEManager->hWebConfig;
	char buf[16] = {0};
	WebConfigLog("------- %s ----- ENTER ----\n",__FUNCTION__);
	/* check the parameter name and set the corresponding value */
	if( AnscEqualString(ParamName, "RfcEnable", TRUE))
	{
		if(bValue == TRUE)
		{
			sprintf(buf, "%s", "true");	 
		}
		else
		{
			sprintf(buf, "%s", "false");	 
		}  

		if(syscfg_set(NULL, "WebConfigRfcEnabled", buf) != 0)
		{
			WebConfigLog("syscfg_set failed\n");
		}
		else
		{
			if (syscfg_commit() != 0)
			{
				WebConfigLog("syscfg_commit failed\n");
			}
			else
			{
				pMyObject->RfcEnable = bValue;
			}
		}

		return TRUE;
	}
	WebConfigLog("------- %s ----- EXIT ----\n",__FUNCTION__);
	return FALSE;
}

BOOL
X_RDK_WebConfig_GetParamUlongValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        ULONG*                      puLong
    )
{
    PCOSA_DATAMODEL_WEBCONFIG            pMyObject           = (PCOSA_DATAMODEL_WEBCONFIG)g_pCosaBEManager->hWebConfig;
	WebConfigLog("------- %s ----- ENTER ----\n",__FUNCTION__);
    /* check the parameter name and return the corresponding value */
    if( AnscEqualString(ParamName, "ConfigFileNumberOfEntries", TRUE))
    {
        /* collect value */
        *puLong = pMyObject->pConfigFileContainer->ConfigFileEntryCount;
        return TRUE;
    }
	WebConfigLog("------- %s ----- EXIT ----\n",__FUNCTION__);
    return FALSE;
}

BOOL
X_RDK_WebConfig_GetParamIntValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        int*                        pInt
    )
{
    PCOSA_DATAMODEL_WEBCONFIG            pMyObject           = (PCOSA_DATAMODEL_WEBCONFIG)g_pCosaBEManager->hWebConfig;
	WebConfigLog("------- %s ----- ENTER ----\n",__FUNCTION__);
    /* check the parameter name and return the corresponding value */
    if( AnscEqualString(ParamName, "PeriodicSyncCheckInterval", TRUE))
    {
        *pInt = pMyObject->PeriodicSyncCheckInterval;
        return TRUE;
    }
	WebConfigLog("------- %s ----- EXIT ----\n",__FUNCTION__);
    /* CcspTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

int Get_PeriodicSyncCheckInterval()
{
       PCOSA_DATAMODEL_WEBCONFIG            pMyObject           = (PCOSA_DATAMODEL_WEBCONFIG)g_pCosaBEManager->hWebConfig;
       
       return pMyObject->PeriodicSyncCheckInterval; 
}

BOOL
X_RDK_WebConfig_SetParamIntValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        int                         iValue
    )
{
	PCOSA_DATAMODEL_WEBCONFIG            pMyObject           = (PCOSA_DATAMODEL_WEBCONFIG)g_pCosaBEManager->hWebConfig;
	char buf[16]={0};
	WebConfigLog("------- %s ----- ENTER ----\n",__FUNCTION__);
	/* check the parameter name and set the corresponding value */
	if( AnscEqualString(ParamName, "PeriodicSyncCheckInterval", TRUE))
	{
		sprintf(buf, "%d", iValue);
		if(syscfg_set( NULL, "PeriodicSyncCheckInterval", buf) != 0)
		{
			WebConfigLog("syscfg_set failed\n");
		}
		else 
		{
			if (syscfg_commit() != 0)
			{
				WebConfigLog("syscfg_commit failed\n");
			}
			else
			{
				pMyObject->PeriodicSyncCheckInterval = iValue;
		             /* sending signal to WebConfigTask to update the sync time interval*/
		               pthread_mutex_lock (get_global_periodicsync_mutex());
		               pthread_cond_signal(get_global_periodicsync_condition());
		               pthread_mutex_unlock(get_global_periodicsync_mutex());
			}
		}
		return TRUE;
	}
	WebConfigLog("------- %s ----- EXIT ----\n",__FUNCTION__);
	/* CcspTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
	return FALSE;
}
/***********************************************************************

 APIs for Object:

    X_RDK_WebConfig.ConfigFile.{i}.

    *  ConfigFile_GetEntryCount
    *  ConfigFile_GetEntry
    *  ConfigFile_IsUpdated
    *  ConfigFile_Synchronize
    *  ConfigFile_AddEntry
    *  ConfigFile_DelEntry
    *  ConfigFile_GetParamBoolValue
    *  ConfigFile_SetParamBoolValue
    *  ConfigFile_GetParamStringValue
    *  ConfigFile_SetParamBoolValue
    *  ConfigFile_SetParamStringValue
    *  ConfigFile_Validate
    *  ConfigFile_Commit
    *  ConfigFile_Rollback

***********************************************************************/

ULONG
ConfigFile_GetEntryCount
    (
        ANSC_HANDLE hInsContext
    )

{
    PCOSA_DATAMODEL_WEBCONFIG            pMyObject           = (PCOSA_DATAMODEL_WEBCONFIG)g_pCosaBEManager->hWebConfig;
	WebConfigLog("------- %s ----- ENTER ----\n",__FUNCTION__);
    int Qdepth = AnscSListQueryDepth( &pMyObject->ConfigFileList );
	WebConfigLog("------- %s ----- EXIT ----\n",__FUNCTION__);
    return Qdepth;
}

ANSC_HANDLE
ConfigFile_GetEntry
    (
        ANSC_HANDLE                 hInsContext,
        ULONG                       nIndex,
        ULONG*                      pInsNumber
    )
{
    PCOSA_DATAMODEL_WEBCONFIG                   pMyObject         = (PCOSA_DATAMODEL_WEBCONFIG)g_pCosaBEManager->hWebConfig;
    PSINGLE_LINK_ENTRY                    pSListEntry       = NULL;
    PCOSA_CONTEXT_WEBCONFIG_LINK_OBJECT    pCxtLink          = NULL;
	WebConfigLog("------- %s ----- ENTER ----\n",__FUNCTION__);
    pSListEntry       = AnscSListGetEntryByIndex(&pMyObject->ConfigFileList, nIndex);
    if ( pSListEntry )
    {
        pCxtLink      = ACCESS_COSA_CONTEXT_WEBCONFIG_LINK_OBJECT(pSListEntry);
        *pInsNumber   = pCxtLink->InstanceNumber;
    }
	WebConfigLog("------- %s ----- EXIT ----\n",__FUNCTION__);
    return (ANSC_HANDLE)pSListEntry;
}

BOOL
ConfigFile_IsUpdated
    (
        ANSC_HANDLE                 hInsContext
    )
{
    PCOSA_DATAMODEL_WEBCONFIG             pWebConfig    = (PCOSA_DATAMODEL_WEBCONFIG)g_pCosaBEManager->hWebConfig;
    BOOL                            bIsUpdated   = TRUE;
	WebConfigLog("------- %s ----- ENTER ----\n",__FUNCTION__);
	WebConfigLog("------- %s ----- EXIT ----\n",__FUNCTION__);
    return bIsUpdated;
}

ULONG
ConfigFile_Synchronize
    (
        ANSC_HANDLE                 hInsContext
    )
{

    ANSC_STATUS                           returnStatus      = ANSC_STATUS_FAILURE;
    PCOSA_DATAMODEL_WEBCONFIG             pWebConfig    = (PCOSA_DATAMODEL_WEBCONFIG)g_pCosaBEManager->hWebConfig;
    PCOSA_CONTEXT_WEBCONFIG_LINK_OBJECT    pCxtLink          = NULL;
    PSINGLE_LINK_ENTRY                    pSListEntry       = NULL;
    PSINGLE_LINK_ENTRY                    pSListEntry2      = NULL;
    ULONG                                 entryCount        = 0;
	WebConfigLog("------- %s ----- ENTER ----\n",__FUNCTION__);
	WebConfigLog("------- %s ----- EXIT ----\n",__FUNCTION__);
}

ANSC_HANDLE
ConfigFile_AddEntry
    (
        ANSC_HANDLE                 hInsContext,
        ULONG*                      pInsNumber
    )
{

	PCOSA_DATAMODEL_WEBCONFIG             pWebConfig              = (PCOSA_DATAMODEL_WEBCONFIG)g_pCosaBEManager->hWebConfig;
    PCOSA_DML_WEBCONFIG_CONFIGFILE_ENTRY pConfigFileEntry = NULL;
    PCOSA_CONTEXT_WEBCONFIG_LINK_OBJECT   pWebConfigCxtLink  = NULL;
    WebConfigLog("------- %s ----- ENTER ----\n",__FUNCTION__);

    pConfigFileEntry = (PCOSA_DML_WEBCONFIG_CONFIGFILE_ENTRY)AnscAllocateMemory(sizeof(COSA_DML_WEBCONFIG_CONFIGFILE_ENTRY));
    if ( !pConfigFileEntry )
    {
		WebConfigLog("%s resource allocation failed\n",__FUNCTION__);
        return NULL;
    }
 
    pWebConfigCxtLink = (PCOSA_CONTEXT_WEBCONFIG_LINK_OBJECT)AnscAllocateMemory(sizeof(COSA_CONTEXT_WEBCONFIG_LINK_OBJECT));

    if ( !pWebConfigCxtLink )
    {
        goto EXIT;
    }        
	
	pWebConfigCxtLink->InstanceNumber =  pWebConfig->ulWebConfigNextInstanceNumber;
	pConfigFileEntry->InstanceNumber = pWebConfig->ulWebConfigNextInstanceNumber;
    pWebConfig->ulWebConfigNextInstanceNumber++;

    /* now we have this link content */
	pWebConfigCxtLink->hContext = (ANSC_HANDLE)pConfigFileEntry;
	pWebConfig->pConfigFileContainer->ConfigFileEntryCount++;
    *pInsNumber = pWebConfigCxtLink->InstanceNumber;

	CosaSListPushEntryByInsNum(&pWebConfig->ConfigFileList, (PCOSA_CONTEXT_LINK_OBJECT)pWebConfigCxtLink);
	WebConfigLog("------- %s ----- EXIT ----\n",__FUNCTION__);

    return (ANSC_HANDLE)pWebConfigCxtLink;

EXIT:
    AnscFreeMemory(pConfigFileEntry);

    return NULL;

}

ULONG
ConfigFile_DelEntry
    (
        ANSC_HANDLE                 hInsContext,
        ANSC_HANDLE                 hInstance
    )

{
    ANSC_STATUS                          returnStatus      = ANSC_STATUS_SUCCESS;
    PCOSA_DATAMODEL_WEBCONFIG             pWebConfig               = (PCOSA_DATAMODEL_WEBCONFIG)g_pCosaBEManager->hWebConfig;
    PCOSA_CONTEXT_WEBCONFIG_LINK_OBJECT   pWebConfigCxtLink   = (PCOSA_CONTEXT_WEBCONFIG_LINK_OBJECT)hInstance;
    PCOSA_DML_WEBCONFIG_CONFIGFILE_ENTRY pConfigFileEntry      = (PCOSA_DML_WEBCONFIG_CONFIGFILE_ENTRY)pWebConfigCxtLink->hContext;
    WebConfigLog(" %s : ENTER \n", __FUNCTION__ );
	/* Remove entery from the database */

    //TODO;

    if ( returnStatus == ANSC_STATUS_SUCCESS )
	{
			/* Remove entery from the Queue */
        if(AnscSListPopEntryByLink(&pWebConfig->ConfigFileList, &pWebConfigCxtLink->Linkage) == TRUE)
		{
			AnscFreeMemory(pWebConfigCxtLink->hContext);

			AnscFreeMemory(pWebConfigCxtLink);
		}
		else
		{
			return ANSC_STATUS_FAILURE;
		}
	}
    WebConfigLog(" %s : EXIT \n", __FUNCTION__ );
    return returnStatus;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        BOOL
        ConfigFile_GetParamBoolValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                BOOL*                       pBool
            );

    description:

        This function is called to retrieve Boolean parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                BOOL*                       pBool
                The buffer of returned boolean value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
ConfigFile_GetParamBoolValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        BOOL*                       pBool
    )
{

    PCOSA_CONTEXT_WEBCONFIG_LINK_OBJECT   pWebConfigCxtLink     = (PCOSA_CONTEXT_WEBCONFIG_LINK_OBJECT)hInsContext;
    PCOSA_DML_WEBCONFIG_CONFIGFILE_ENTRY pConfigFileEntry  = (PCOSA_DML_WEBCONFIG_CONFIGFILE_ENTRY)pWebConfigCxtLink->hContext;
    WebConfigLog("------- %s ----- ENTER ----\n",__FUNCTION__);
    if( AnscEqualString(ParamName, "ForceSyncCheck", TRUE))
    {
        /* all read must return FALSE */
        *pBool = FALSE;
        return TRUE;
    }
    
    if( AnscEqualString(ParamName, "SyncCheckOK", TRUE))
    {
        /* collect value */
        *pBool = pConfigFileEntry->SyncCheckOK;
        return TRUE;
    }
    WebConfigLog(" %s : EXIT \n", __FUNCTION__ );
    /* AnscTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

ULONG
ConfigFile_GetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pValue,
        ULONG*                      pUlSize
    )

{
    PCOSA_CONTEXT_WEBCONFIG_LINK_OBJECT   pWebConfigCxtLink     = (PCOSA_CONTEXT_WEBCONFIG_LINK_OBJECT)hInsContext;
    PCOSA_DML_WEBCONFIG_CONFIGFILE_ENTRY pConfigFileEntry  = (PCOSA_DML_WEBCONFIG_CONFIGFILE_ENTRY)pWebConfigCxtLink->hContext;
    PUCHAR                                    pString       = NULL;
    WebConfigLog(" %s : ENTER \n", __FUNCTION__ );

    if( AnscEqualString(ParamName, "URL", TRUE))
    {
        /* collect value */
        if ( AnscSizeOfString(pConfigFileEntry->URL) < *pUlSize)
        {
            AnscCopyString(pValue, pConfigFileEntry->URL);
            return 0;
        }
        else
        {
            *pUlSize = AnscSizeOfString(pConfigFileEntry->URL)+1;
            return 1;
        }
    }

    if( AnscEqualString(ParamName, "Version", TRUE))
    {
        /* collect value */
        if ( AnscSizeOfString(pConfigFileEntry->Version) < *pUlSize)
        {
            AnscCopyString(pValue, pConfigFileEntry->Version);
            return 0;
        }
        else
        {
            *pUlSize = AnscSizeOfString(pConfigFileEntry->Version)+1;
            return 1;
        }
    }

    if( AnscEqualString(ParamName, "PreviousSyncDateTime", TRUE))
    {
        /* collect value */
        if ( AnscSizeOfString(pConfigFileEntry->PreviousSyncDateTime) < *pUlSize)
        {
            AnscCopyString(pValue, pConfigFileEntry->PreviousSyncDateTime);
            return 0;
        }
        else
        {
            *pUlSize = AnscSizeOfString(pConfigFileEntry->PreviousSyncDateTime)+1;
            return 1;
        }
    }

    WebConfigLog(" %s : EXIT \n", __FUNCTION__ );

    return -1;
}

BOOL
ConfigFile_SetParamBoolValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        BOOL                        bValue
    )
{
	PCOSA_CONTEXT_WEBCONFIG_LINK_OBJECT   pWebConfigCxtLink     = (PCOSA_CONTEXT_WEBCONFIG_LINK_OBJECT)hInsContext;
	PCOSA_DML_WEBCONFIG_CONFIGFILE_ENTRY pConfigFileEntry  = (PCOSA_DML_WEBCONFIG_CONFIGFILE_ENTRY)pWebConfigCxtLink->hContext;
	WebConfigLog("------- %s ----- ENTER ----\n",__FUNCTION__);
	/* check the parameter name and set the corresponding value */
	if( AnscEqualString(ParamName, "SyncCheckOK", TRUE))
	{
		pConfigFileEntry->SyncCheckOK = bValue;   // update in memory and commit to syscfg in commit func after validate
		return TRUE;
	}
	else if(AnscEqualString(ParamName, "ForceSyncCheck", TRUE)) 
	{
		// TODO: trigger sync by sending pthread condition signal
		return TRUE;
	}
	WebConfigLog("------- %s ----- EXIT ----\n",__FUNCTION__);
	return FALSE;
}

BOOL
ConfigFile_SetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       strValue
    )

{
	PCOSA_CONTEXT_WEBCONFIG_LINK_OBJECT   pWebConfigCxtLink     = (PCOSA_CONTEXT_WEBCONFIG_LINK_OBJECT)hInsContext;
    PCOSA_DML_WEBCONFIG_CONFIGFILE_ENTRY pConfigFileEntry  = (PCOSA_DML_WEBCONFIG_CONFIGFILE_ENTRY)pWebConfigCxtLink->hContext;
    BOOL ret = FALSE;
    WebConfigLog(" %s : ENTER \n", __FUNCTION__ );
    if( AnscEqualString(ParamName, "URL", TRUE))
    {
	/* save update to backup */
        AnscCopyString( pConfigFileEntry->URL, strValue );
        return TRUE;
    }
    else if (AnscEqualString(ParamName, "Version", TRUE))
    {
	/* save update to backup */
        AnscCopyString( pConfigFileEntry->Version, strValue );
	return TRUE;
    }
    else if( AnscEqualString(ParamName, "PreviousSyncDateTime", TRUE))
    {
	/* save update to backup */
        AnscCopyString( pConfigFileEntry->PreviousSyncDateTime, strValue );
        return TRUE;
    }
    WebConfigLog(" %s : EXIT \n", __FUNCTION__ );

    return ret;
}

BOOL
ConfigFile_Validate
    (
        ANSC_HANDLE                 hInsContext,
        char*                       pReturnParamName,
        ULONG*                      puLength
    )

{
    PCOSA_CONTEXT_WEBCONFIG_LINK_OBJECT   pWebConfigCxtLink     = (PCOSA_CONTEXT_WEBCONFIG_LINK_OBJECT)hInsContext;
    PCOSA_DML_WEBCONFIG_CONFIGFILE_ENTRY pConfigFileEntry  = (PCOSA_DML_WEBCONFIG_CONFIGFILE_ENTRY)pWebConfigCxtLink->hContext;
    WebConfigLog(" %s : ENTER \n", __FUNCTION__ );

    BOOL ret = FALSE;
    //TODO
    WebConfigLog(" %s : EXIT \n", __FUNCTION__ );

    return ret;
}

ULONG
ConfigFile_Commit
    (
        ANSC_HANDLE                 hInsContext
    )

{
    PCOSA_CONTEXT_WEBCONFIG_LINK_OBJECT   pWebConfigCxtLink     = (PCOSA_CONTEXT_WEBCONFIG_LINK_OBJECT)hInsContext;
    PCOSA_DML_WEBCONFIG_CONFIGFILE_ENTRY pConfigFileEntry  = (PCOSA_DML_WEBCONFIG_CONFIGFILE_ENTRY)pWebConfigCxtLink->hContext;
    WebConfigLog(" %s : ENTER \n", __FUNCTION__ );
    //TODO
    WebConfigLog(" %s : EXIT \n", __FUNCTION__ );
}

ULONG
ConfigFile_Rollback
    (
        ANSC_HANDLE                 hInsContext
    )

{
    PCOSA_CONTEXT_WEBCONFIG_LINK_OBJECT   pWebConfigCxtLink     = (PCOSA_CONTEXT_WEBCONFIG_LINK_OBJECT)hInsContext;
    PCOSA_DML_WEBCONFIG_CONFIGFILE_ENTRY pConfigFileEntry  = (PCOSA_DML_WEBCONFIG_CONFIGFILE_ENTRY)pWebConfigCxtLink->hContext;

    WebConfigLog(" %s : ENTER \n", __FUNCTION__ );
    //TODO
    WebConfigLog(" %s : EXIT \n", __FUNCTION__ );
}


