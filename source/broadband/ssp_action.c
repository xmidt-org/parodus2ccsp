/**
 * @file ssp_action.c
 *
 * @description This file creates WEBPA component which would register with RDK-B stack, uses functions for registration/deregistration functions of components provided by RDK-B.
 * 
 */

#include "ssp_global.h"
#include "dslh_dmagnt_interface.h"
#include "ccsp_trace.h"
#include "plugin_main.h"
#ifdef RDKB_BUILD
#include "dm_pack_create_func.h"
#endif
/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
#ifndef RDKB_BUILD
#define  CCSP_DATAMODEL_XML_FILE           "/usr/ccsp/webpa/WebpaAgent.xml"
#endif

/*----------------------------------------------------------------------------*/
/*                               Library Objects                              */
/*----------------------------------------------------------------------------*/

PDSLH_CPE_CONTROLLER_OBJECT     pDslhCpeController              = NULL;
PCOMPONENT_COMMON_WEBPAAGENT    g_pComponent_COMMON_webpaagent  = NULL;
PCCSP_CCD_INTERFACE             pSsdCcdIf                       = (PCCSP_CCD_INTERFACE)NULL;
PDSLH_LCB_INTERFACE             pDslhLcbIf                      = (PDSLH_LCB_INTERFACE)NULL;


/*----------------------------------------------------------------------------*/
/*                           External Variables                               */
/*----------------------------------------------------------------------------*/

extern char                     g_Subsystem[32];
extern  ANSC_HANDLE             bus_handle;
extern  ULONG                   g_ulAllocatedSizePeak;

/*----------------------------------------------------------------------------*/
/*                           External Functions                               */
/*----------------------------------------------------------------------------*/

/**
 * @brief 
 * This function creates component object in the CR(Component Registrar).
 */
ANSC_STATUS ssp_create()
{
    /* Create component common data model object */

     g_pComponent_COMMON_webpaagent = (PCOMPONENT_COMMON_WEBPAAGENT)AnscAllocateMemory(sizeof(COMPONENT_COMMON_WEBPAAGENT));

    if ( ! g_pComponent_COMMON_webpaagent )
    {
        return ANSC_STATUS_RESOURCES;
    }

    ComponentCommonDmInit( g_pComponent_COMMON_webpaagent);

     g_pComponent_COMMON_webpaagent->Name     = AnscCloneString(CCSP_COMPONENT_NAME_WEBPAAGENT);
     g_pComponent_COMMON_webpaagent->Version  = 1;
     g_pComponent_COMMON_webpaagent->Author   = AnscCloneString("Your name");

    /* Create ComponentCommonDatamodel interface*/
    if ( !pSsdCcdIf )
    {
        pSsdCcdIf = (PCCSP_CCD_INTERFACE)AnscAllocateMemory(sizeof(CCSP_CCD_INTERFACE));

        if ( !pSsdCcdIf )
        {
            return ANSC_STATUS_RESOURCES;
        }
        else
        {
            AnscCopyString(pSsdCcdIf->Name, CCSP_CCD_INTERFACE_NAME);

            pSsdCcdIf->InterfaceId              = CCSP_CCD_INTERFACE_ID;
            pSsdCcdIf->hOwnerContext            = NULL;
            pSsdCcdIf->Size                     = sizeof(CCSP_CCD_INTERFACE);

            pSsdCcdIf->GetComponentName         = ssp_CcdIfGetComponentName;
            pSsdCcdIf->GetComponentVersion      = ssp_CcdIfGetComponentVersion;
            pSsdCcdIf->GetComponentAuthor       = ssp_CcdIfGetComponentAuthor;
            pSsdCcdIf->GetComponentHealth       = ssp_CcdIfGetComponentHealth;
            pSsdCcdIf->GetComponentState        = ssp_CcdIfGetComponentState;
            pSsdCcdIf->GetLoggingEnabled        = ssp_CcdIfGetLoggingEnabled;
            pSsdCcdIf->SetLoggingEnabled        = ssp_CcdIfSetLoggingEnabled;
            pSsdCcdIf->GetLoggingLevel          = ssp_CcdIfGetLoggingLevel;
            pSsdCcdIf->SetLoggingLevel          = ssp_CcdIfSetLoggingLevel;
            pSsdCcdIf->GetMemMaxUsage           = ssp_CcdIfGetMemMaxUsage;
            pSsdCcdIf->GetMemMinUsage           = ssp_CcdIfGetMemMinUsage;
            pSsdCcdIf->GetMemConsumed           = ssp_CcdIfGetMemConsumed;
            pSsdCcdIf->ApplyChanges             = ssp_CcdIfApplyChanges;
        }
    }

    /* Create ComponentCommonDatamodel interface*/
    if ( !pDslhLcbIf )
    {
        pDslhLcbIf = (PDSLH_LCB_INTERFACE)AnscAllocateMemory(sizeof(DSLH_LCB_INTERFACE));

        if ( !pDslhLcbIf )
        {
            return ANSC_STATUS_RESOURCES;
        }
        else
        {
            AnscCopyString(pDslhLcbIf->Name, CCSP_LIBCBK_INTERFACE_NAME);

            pDslhLcbIf->InterfaceId              = CCSP_LIBCBK_INTERFACE_ID;
            pDslhLcbIf->hOwnerContext            = NULL;
            pDslhLcbIf->Size                     = sizeof(DSLH_LCB_INTERFACE);
            pDslhLcbIf->InitLibrary              = COSA_Init;
        }
    }

    pDslhCpeController = DslhCreateCpeController(NULL, NULL, NULL);

    if ( !pDslhCpeController )
    {
        CcspTraceWarning(("CANNOT Create pDslhCpeController... Exit!\n"));

        return ANSC_STATUS_RESOURCES;
    }

    return ANSC_STATUS_SUCCESS;
}

/**
 * @brief 
 * This function establish connection from the component to the CR(Component Registrar).
 */
ANSC_STATUS ssp_engage()
{
	ANSC_STATUS					    returnStatus                = ANSC_STATUS_SUCCESS;
    PCCC_MBI_INTERFACE              pSsdMbiIf                   = (PCCC_MBI_INTERFACE)MsgHelper_CreateCcdMbiIf((void*)bus_handle, g_Subsystem);
    char                            CrName[256];

     g_pComponent_COMMON_webpaagent->Health = CCSP_COMMON_COMPONENT_HEALTH_Yellow;

    /* data model configuration */
    pDslhCpeController->AddInterface((ANSC_HANDLE)pDslhCpeController, (ANSC_HANDLE)pDslhLcbIf);
    pDslhCpeController->AddInterface((ANSC_HANDLE)pDslhCpeController, (ANSC_HANDLE)pSsdMbiIf);
    pDslhCpeController->AddInterface((ANSC_HANDLE)pDslhCpeController, (ANSC_HANDLE)pSsdCcdIf);
    pDslhCpeController->SetDbusHandle((ANSC_HANDLE)pDslhCpeController, (ANSC_HANDLE)bus_handle);
    pDslhCpeController->Engage((ANSC_HANDLE)pDslhCpeController);

    if ( g_Subsystem[0] != 0 )
    {
        _ansc_sprintf(CrName, "%s%s", g_Subsystem, CCSP_DBUS_INTERFACE_CR);
    }
    else
    {
        _ansc_sprintf(CrName, "%s", CCSP_DBUS_INTERFACE_CR);
    }

#ifdef RDKB_BUILD
    returnStatus =
        pDslhCpeController->RegisterCcspDataModel2
#else
	returnStatus =
        pDslhCpeController->RegisterCcspDataModel
#endif
            (
                (ANSC_HANDLE)pDslhCpeController,
                CrName,                                    /* CCSP_DBUS_INTERFACE_CR, CCSP CR ID */
#ifdef RDKB_BUILD
                DMPackCreateDataModelXML,                  /* Comcast generated code to create XML. */
#else
		CCSP_DATAMODEL_XML_FILE,                   /* Data Model XML file. Can be empty if only base data model supported. */
#endif
                CCSP_COMPONENT_NAME_WEBPAAGENT,            /* Component Name    */
                CCSP_COMPONENT_VERSION_WEBPAAGENT,         /* Component Version */
                CCSP_COMPONENT_PATH_WEBPAAGENT,            /* Component Path    */
                g_Subsystem                                /* Component Prefix  */
            );

    if ( returnStatus == ANSC_STATUS_SUCCESS )
    {
        /* System is fully initialized */
         g_pComponent_COMMON_webpaagent->Health = CCSP_COMMON_COMPONENT_HEALTH_Green;
    }

    return ANSC_STATUS_SUCCESS;
}

/**
 * @brief 
 * This function cancel the connection and delete component object from CR(Component Registrar).
 */
ANSC_STATUS ssp_cancel()
{
	int                             nRet  = 0;
    char                            CrName[256];
    char                            CpName[256];

    if(  g_pComponent_COMMON_webpaagent == NULL)
    {
        return ANSC_STATUS_SUCCESS;
    }

    if ( g_Subsystem[0] != 0 )
    {
        _ansc_sprintf(CrName, "%s%s", g_Subsystem, CCSP_DBUS_INTERFACE_CR);
        _ansc_sprintf(CpName, "%s%s", g_Subsystem, CCSP_COMPONENT_NAME_WEBPAAGENT);
    }
    else
    {
        _ansc_sprintf(CrName, "%s", CCSP_DBUS_INTERFACE_CR);
        _ansc_sprintf(CpName, "%s", CCSP_COMPONENT_NAME_WEBPAAGENT);
    }
    /* unregister component */
    nRet = CcspBaseIf_unregisterComponent(bus_handle, CrName, CpName );  
    AnscTrace("unregisterComponent returns %d\n", nRet);

    pDslhCpeController->Cancel((ANSC_HANDLE)pDslhCpeController);
    AnscFreeMemory(pDslhCpeController);

    if ( pSsdCcdIf ) AnscFreeMemory(pSsdCcdIf);
    if (  g_pComponent_COMMON_webpaagent ) AnscFreeMemory( g_pComponent_COMMON_webpaagent);

     g_pComponent_COMMON_webpaagent = NULL;
    pSsdCcdIf                = NULL;
    pDslhCpeController       = NULL;

    return ANSC_STATUS_SUCCESS;
}


char* ssp_CcdIfGetComponentName(ANSC_HANDLE  hThisObject)
{
    return  g_pComponent_COMMON_webpaagent->Name;
}


ULONG ssp_CcdIfGetComponentVersion(ANSC_HANDLE  hThisObject)
{
    return  g_pComponent_COMMON_webpaagent->Version;
}


char* ssp_CcdIfGetComponentAuthor(ANSC_HANDLE hThisObject)
{
    return  g_pComponent_COMMON_webpaagent->Author;
}


ULONG ssp_CcdIfGetComponentHealth(ANSC_HANDLE  hThisObject)
{
    return  g_pComponent_COMMON_webpaagent->Health;
}


ULONG ssp_CcdIfGetComponentState(ANSC_HANDLE hThisObject)
{
    return  g_pComponent_COMMON_webpaagent->State;
}



BOOL ssp_CcdIfGetLoggingEnabled(ANSC_HANDLE hThisObject)
{
    return  g_pComponent_COMMON_webpaagent->LogEnable;
}


ANSC_STATUS ssp_CcdIfSetLoggingEnabled(ANSC_HANDLE  hThisObject,BOOL bEnabled)
{
    if( g_pComponent_COMMON_webpaagent->LogEnable == bEnabled) return ANSC_STATUS_SUCCESS;
     g_pComponent_COMMON_webpaagent->LogEnable = bEnabled;
    if(bEnabled) g_iTraceLevel = (INT)  g_pComponent_COMMON_webpaagent->LogLevel;
    else g_iTraceLevel = CCSP_TRACE_INVALID_LEVEL;

    return ANSC_STATUS_SUCCESS;
}


ULONG ssp_CcdIfGetLoggingLevel(ANSC_HANDLE hThisObject)
{
    return  g_pComponent_COMMON_webpaagent->LogLevel;
}


ANSC_STATUS ssp_CcdIfSetLoggingLevel(ANSC_HANDLE hThisObject,ULONG LogLevel)
{
    if( g_pComponent_COMMON_webpaagent->LogLevel == LogLevel) return ANSC_STATUS_SUCCESS;
     g_pComponent_COMMON_webpaagent->LogLevel = LogLevel;
    if( g_pComponent_COMMON_webpaagent->LogEnable) g_iTraceLevel = (INT)  g_pComponent_COMMON_webpaagent->LogLevel;

    return ANSC_STATUS_SUCCESS;
}


ULONG ssp_CcdIfGetMemMaxUsage(ANSC_HANDLE hThisObject)
{
    return g_ulAllocatedSizePeak;
}


ULONG ssp_CcdIfGetMemMinUsage(ANSC_HANDLE  hThisObject)
{
    return  g_pComponent_COMMON_webpaagent->MemMinUsage;
}


ULONG ssp_CcdIfGetMemConsumed(ANSC_HANDLE  hThisObject)
{
    LONG             size = 0;

    size = AnscGetComponentMemorySize(CCSP_COMPONENT_NAME_WEBPAAGENT);
    if (size == -1 )
        size = 0;

    return size;
}


ANSC_STATUS ssp_CcdIfApplyChanges(ANSC_HANDLE hThisObject)
{
    ANSC_STATUS                         returnStatus    = ANSC_STATUS_SUCCESS;
    /* Assume the parameter settings are committed immediately. */
    /* AnscSetTraceLevel((INT) g_pComponent_COMMON_webpaagent->LogLevel); */

    return returnStatus;
}
