/*********************************************************************** 
  
    module: plugin_main.c

        Implement COSA Data Model Library Init and Unload apis.
 
    ---------------------------------------------------------------

    copyright:

        Cisco Systems, Inc.
        All Rights Reserved.

    ---------------------------------------------------------------

    author:

        COSA XML TOOL CODE GENERATOR 1.0

    ---------------------------------------------------------------

    revision:

        09/28/2011    initial revision.

**********************************************************************/

#include "ansc_platform.h"
#include "ansc_load_library.h"
#include "cosa_plugin_api.h"
#include "plugin_main.h"
#include "cosa_webpa_dml.h"
#ifdef FEATURE_SUPPORT_WEBCONFIG
#include "cosa_webconfig_dml.h"
#endif
#include "plugin_main_apis.h"
#include "webpa_adapter.h"

#define THIS_PLUGIN_VERSION                         1
PCOSA_BACKEND_MANAGER_OBJECT g_pCosaBEManager;

int ANSC_EXPORT_API
COSA_Init
    (
        ULONG                       uMaxVersionSupported, 
        void*                       hCosaPlugInfo         /* PCOSA_PLUGIN_INFO passed in by the caller */
    )
{
    PCOSA_PLUGIN_INFO               pPlugInfo  = (PCOSA_PLUGIN_INFO)hCosaPlugInfo;

    if ( uMaxVersionSupported < THIS_PLUGIN_VERSION )
    {
      /* this version is not supported */
        return -1;
    }   
    
    pPlugInfo->uPluginVersion       = THIS_PLUGIN_VERSION;
    /* register the back-end apis for the data model */
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "Webpa_SetParamStringValue", Webpa_SetParamStringValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "Webpa_GetParamStringValue", Webpa_GetParamStringValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "Webpa_GetParamUlongValue",  Webpa_GetParamUlongValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "Webpa_SetParamUlongValue",  Webpa_SetParamUlongValue);
#ifdef FEATURE_SUPPORT_WEBCONFIG
	pPlugInfo->RegisterFunction(pPlugInfo->hContext, "X_RDK_WebConfig_GetParamBoolValue",  X_RDK_WebConfig_GetParamBoolValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "X_RDK_WebConfig_SetParamBoolValue",  X_RDK_WebConfig_SetParamBoolValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "X_RDK_WebConfig_GetParamUlongValue",  X_RDK_WebConfig_GetParamUlongValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "X_RDK_WebConfig_GetParamIntValue",  X_RDK_WebConfig_GetParamIntValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "X_RDK_WebConfig_SetParamIntValue",  X_RDK_WebConfig_SetParamIntValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "X_RDK_WebConfig_SetParamStringValue",  X_RDK_WebConfig_SetParamStringValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "X_RDK_WebConfig_GetParamStringValue",  X_RDK_WebConfig_GetParamStringValue);

	pPlugInfo->RegisterFunction(pPlugInfo->hContext, "ConfigFile_GetEntryCount", ConfigFile_GetEntryCount);
	pPlugInfo->RegisterFunction(pPlugInfo->hContext, "ConfigFile_GetEntry",ConfigFile_GetEntry);
	pPlugInfo->RegisterFunction(pPlugInfo->hContext, "ConfigFile_AddEntry", ConfigFile_AddEntry);
	pPlugInfo->RegisterFunction(pPlugInfo->hContext, "ConfigFile_DelEntry", ConfigFile_DelEntry);
	pPlugInfo->RegisterFunction(pPlugInfo->hContext, "ConfigFile_IsUpdated", ConfigFile_IsUpdated);
	pPlugInfo->RegisterFunction(pPlugInfo->hContext, "ConfigFile_Synchronize" , ConfigFile_Synchronize);
	pPlugInfo->RegisterFunction(pPlugInfo->hContext, "ConfigFile_GetParamBoolValue", ConfigFile_GetParamBoolValue);
	pPlugInfo->RegisterFunction(pPlugInfo->hContext, "ConfigFile_GetParamStringValue", ConfigFile_GetParamStringValue);
	pPlugInfo->RegisterFunction(pPlugInfo->hContext, "ConfigFile_SetParamBoolValue", ConfigFile_SetParamBoolValue);
	pPlugInfo->RegisterFunction(pPlugInfo->hContext, "ConfigFile_SetParamStringValue", ConfigFile_SetParamStringValue);
	pPlugInfo->RegisterFunction(pPlugInfo->hContext, "ConfigFile_Commit", ConfigFile_Commit);
	pPlugInfo->RegisterFunction(pPlugInfo->hContext, "ConfigFile_Validate", ConfigFile_Validate);
	pPlugInfo->RegisterFunction(pPlugInfo->hContext, "ConfigFile_Rollback", ConfigFile_Rollback);
#endif
    /* Create backend framework */
    g_pCosaBEManager = (PCOSA_BACKEND_MANAGER_OBJECT)CosaBackEndManagerCreate();

    if ( g_pCosaBEManager && g_pCosaBEManager->Initialize )
    {
        g_pCosaBEManager->hCosaPluginInfo = pPlugInfo;

        g_pCosaBEManager->Initialize   ((ANSC_HANDLE)g_pCosaBEManager);
    }

    return  0;
}

BOOL ANSC_EXPORT_API
COSA_IsObjectSupported
    (
        char*                        pObjName
    )
{
    return TRUE;
}

void ANSC_EXPORT_API
COSA_Unload
    (
        void
    )
{
    ANSC_STATUS                     returnStatus            = ANSC_STATUS_SUCCESS;

    /* unload the memory here */

    returnStatus  =  CosaBackEndManagerRemove(g_pCosaBEManager);

    if ( returnStatus == ANSC_STATUS_SUCCESS )
    {
        g_pCosaBEManager = NULL;
    }
    else
    {
        /* print error trace*/
        g_pCosaBEManager = NULL;
    }
}
