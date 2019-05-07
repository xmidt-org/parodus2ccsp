/**
*@file ssp_messagebus_interface.c
*
*@description This file is for Message Bus initalization of the component and component path registration.
*
*/
#include "ssp_global.h"
#include "webpa_adapter.h"

ANSC_HANDLE                 bus_handle               = NULL;
extern char                 g_Subsystem[32];
extern ANSC_HANDLE          g_MessageBusHandle_Irep; 
extern char                 g_SubSysPrefix_Irep[32];

#ifdef _ANSC_LINUX

DBusHandlerResult CcspComp_path_message_func(DBusConnection  *conn,DBusMessage *message,void  *user_data)
{
    CCSP_MESSAGE_BUS_INFO *bus_info =(CCSP_MESSAGE_BUS_INFO *) user_data;
    const char *interface = dbus_message_get_interface(message);
    const char *method   = dbus_message_get_member(message);
    DBusMessage *reply;
	WalInfo("--------- %s ----------\n",__FUNCTION__);
    reply = dbus_message_new_method_return (message);
    if (reply == NULL)
    {
        return DBUS_HANDLER_RESULT_HANDLED;
    }
	WalInfo("--------- %s ----------\n",__FUNCTION__);
    return CcspBaseIf_base_path_message_func
               (
                   conn,
                   message,
                   reply,
                   interface,
                   method,
                   bus_info
               );
}

ANSC_STATUS ssp_Mbi_MessageBusEngage(char * component_id,char * config_file,char * path)
{
    ANSC_STATUS                 returnStatus       = ANSC_STATUS_SUCCESS;
    CCSP_Base_Func_CB           cb                 = {0};
	WalInfo("--------- %s ----------\n",__FUNCTION__);
    if ( ! component_id || ! path )
    {
        CcspTraceError((" !!! ssp_Mbi_MessageBusEngage: component_id or path is NULL !!!\n"));
		WalError(" !!! ssp_Mbi_MessageBusEngage: component_id or path is NULL !!!\n");
    }

    /* Connect to message bus */
    returnStatus = 
        CCSP_Message_Bus_Init
            (
                component_id,
                config_file,
                &bus_handle,
                Ansc_AllocateMemory_Callback,           /* mallocfc, use default */
                Ansc_FreeMemory_Callback                /* freefc,   use default */
            );

    if ( returnStatus != ANSC_STATUS_SUCCESS )
    {
        CcspTraceError((" !!! SSD Message Bus Init ERROR !!!\n"));
		WalError(" !!! SSD Message Bus Init ERROR !!!\n");
        return returnStatus;
    }

    CcspTraceInfo(("INFO: bus_handle: 0x%8x \n", bus_handle));
	WalInfo("INFO: bus_handle: 0x%8x \n", bus_handle);
    g_MessageBusHandle_Irep = bus_handle;
    AnscCopyString(g_SubSysPrefix_Irep, g_Subsystem);

    CCSP_Msg_SleepInMilliSeconds(1000);

    /* Base interface implementation that will be used cross components */
    cb.getParameterValues     = CcspCcMbi_GetParameterValues;
    cb.setParameterValues     = CcspCcMbi_SetParameterValues;
    cb.setCommit              = CcspCcMbi_SetCommit;
    cb.setParameterAttributes = CcspCcMbi_SetParameterAttributes;
    cb.getParameterAttributes = CcspCcMbi_GetParameterAttributes;
    cb.AddTblRow              = CcspCcMbi_AddTblRow;
    cb.DeleteTblRow           = CcspCcMbi_DeleteTblRow;
    cb.getParameterNames      = CcspCcMbi_GetParameterNames;
    cb.currentSessionIDSignal = CcspCcMbi_CurrentSessionIdSignal;

    /* Base interface implementation that will only be used by ssd */
    cb.initialize             = ssp_Mbi_Initialize;
    cb.finalize               = ssp_Mbi_Finalize;
    cb.freeResources          = ssp_Mbi_FreeResources;
    cb.busCheck               = ssp_Mbi_Buscheck;

    CcspBaseIf_SetCallback(bus_handle, &cb);

    /* Register service callback functions */
    returnStatus =
        CCSP_Message_Bus_Register_Path
            (
                bus_handle,
                path,
                CcspComp_path_message_func,
                bus_handle
            );

    if ( returnStatus != CCSP_Message_Bus_OK )
    {
        CcspTraceError((" !!! CCSP_Message_Bus_Register_Path ERROR returnStatus: %d\n!!!\n", returnStatus));
		WalError(" !!! CCSP_Message_Bus_Register_Path ERROR returnStatus: %d\n!!!\n", returnStatus);
        return returnStatus;
    }


    /* Register event/signal */
    returnStatus = 
        CcspBaseIf_Register_Event
            (
                bus_handle,
                0,
                "currentSessionIDSignal"
            );

    if ( returnStatus != CCSP_Message_Bus_OK )
    {
         CcspTraceError((" !!! CCSP_Message_Bus_Register_Event: CurrentSessionIDSignal ERROR returnStatus: %d!!!\n", returnStatus));
		WalError(" !!! CCSP_Message_Bus_Register_Event: CurrentSessionIDSignal ERROR returnStatus: %d!!!\n", returnStatus);
        return returnStatus;
    }

    return ANSC_STATUS_SUCCESS;

}

#endif

int ssp_Mbi_Initialize(void * user_data)
{
    ANSC_STATUS             returnStatus    = ANSC_STATUS_SUCCESS;

    return ( returnStatus == ANSC_STATUS_SUCCESS ) ? 0 : 1;
}


int ssp_Mbi_Finalize(void* user_data)
{
    ANSC_STATUS             returnStatus    = ANSC_STATUS_SUCCESS;

    returnStatus = ssp_cancel();

    return ( returnStatus == ANSC_STATUS_SUCCESS ) ? 0 : 1;
}


int ssp_Mbi_Buscheck(void* user_data)
{
    return 0;
}


int ssp_Mbi_FreeResources(int priority,void  * user_data)
{
    ANSC_STATUS             returnStatus    = ANSC_STATUS_SUCCESS;

    if ( priority == CCSP_COMMON_COMPONENT_FREERESOURCES_PRIORITY_Low )
    {
        /* Currently do nothing */
    }
    else if ( priority == CCSP_COMMON_COMPONENT_FREERESOURCES_PRIORITY_High )
    {
        returnStatus = ssp_cancel();
    }
    
    return ( returnStatus == ANSC_STATUS_SUCCESS ) ? 0 : 1;
}


