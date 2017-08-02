
#include "ansc_platform.h"
#include "cosa_webpa_dml.h"
#include "ccsp_trace.h"
#include "cosa_webpa_internal.h"
#include "ccsp_base_api.h"
#include "webpa_internal.h"
#include "webpa_notification.h"

extern PCOSA_BACKEND_MANAGER_OBJECT g_pCosaBEManager;

void (*notifyCbFnPtr)(NotifyData*) = NULL;

BOOL
Webpa_SetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
	char*                       pString
    )
{

	PCOSA_DATAMODEL_WEBPA       hWebpa    = (PCOSA_DATAMODEL_WEBPA)g_pCosaBEManager->hWebpa;
        PCOSA_DML_WEBPA             pWebpa    = (PCOSA_DML_WEBPA) hWebpa->pWebpa;
	PCOSA_DML_WEBPA_CONFIG      pWebpaCfg = (PCOSA_DML_WEBPA_CONFIG)pWebpa->pWebpaCfg;
	
#ifdef USE_NOTIFY_COMPONENT
	char* p_write_id;
	char* p_new_val;
	char* p_old_val;
	char* p_notify_param_name;
	char* st;
	char* p_interface_name = NULL;
	char* p_mac_id = NULL;
	char* p_status = NULL;
	char* p_hostname = NULL;
	char* p_val_type;
	UINT value_type,write_id;
	parameterSigStruct_t param = {0};
#endif
	WalPrint("<========= Start of Webpa_SetParamStringValue ========>\n");
	WalInfo("Received data ParamName %s,data length: %d bytes\n",ParamName, strlen(pString));
	
        if( AnscEqualString(ParamName, "X_RDKCENTRAL-COM_WebPA_Notification", TRUE))
        {
        #ifdef USE_NOTIFY_COMPONENT

                WalPrint(" \n WebPA : Notification Received \n");

                p_notify_param_name = strtok_r(pString, ",", &st);
                p_write_id = strtok_r(NULL, ",", &st);
                p_new_val = strtok_r(NULL, ",", &st);
                p_old_val = strtok_r(NULL, ",", &st);
                p_val_type = strtok_r(NULL, ",", &st);

                if(p_val_type !=NULL && p_write_id !=NULL)
                {
                        value_type = atoi(p_val_type);
                        write_id = atoi(p_write_id);

                        WalPrint(" \n Notification : Parameter Name = %s \n", p_notify_param_name);
                        WalPrint(" \n Notification : New Value = %s \n", p_new_val);
                        WalPrint(" \n Notification : Old Value = %s \n", p_old_val);
                        WalPrint(" \n Notification : Value Type = %d \n", value_type);
                        WalPrint(" \n Notification : Component ID = %d \n", write_id);

                        param.parameterName = p_notify_param_name;
                        param.oldValue = p_old_val;
                        param.newValue = p_new_val;
                        param.type = value_type;
                        param.writeID = write_id;

                        ccspWebPaValueChangedCB(&param,0,NULL);
                }
                else
                {
                        WalError("Received insufficient data to process notification\n");
                }

        #endif
                return TRUE;
        }    

        if( AnscEqualString(ParamName, "X_RDKCENTRAL-COM_Connected-Client", TRUE))
        	{
        	#ifdef USE_NOTIFY_COMPONENT
        		WalInfo("...Connected client notification..\n");
                        WalPrint(" \n WebPA : Connected-Client Received \n");
                        p_notify_param_name = strtok_r(pString, ",", &st);
                        WalPrint("PString value for X_RDKCENTRAL-COM_Connected-Client:%s\n", pString);

                        p_interface_name = strtok_r(NULL, ",", &st);
                        p_mac_id = strtok_r(NULL, ",", &st);
                        p_status = strtok_r(NULL, ",", &st);
                        p_hostname = strtok_r(NULL, ",", &st);

                        WalPrint(" \n Notification : Parameter Name = %s \n", p_notify_param_name);
                        WalPrint(" \n Notification : Interface = %s \n", p_interface_name);
                        WalPrint(" \n Notification : MAC = %s \n", p_mac_id);
                        WalPrint(" \n Notification : Status = %s \n", p_status);
                        WalPrint(" \n Notification : HostName = %s \n", p_hostname);

                        notifyCbFnPtr = getNotifyCB();

                        if (NULL == notifyCbFnPtr)
                        {
                                WalError("Fatal: notifyCbFnPtr is NULL\n");
                                return FALSE;
                        }
                        else
                        {
                                // Data received from stack is not sent upstream to server for Connected Client
                                sendConnectedClientNotification(p_mac_id, p_status, p_interface_name, p_hostname);
                        }

        #endif
        		return TRUE;
        	}


	/* Required for xPC sync */
        if( AnscEqualString(ParamName, "X_COMCAST-COM_CID", TRUE))
        {
                WalPrint("X_COMCAST-COM_CID\n");
	        CosaDmlWEBPA_SetConfiguration( pWebpaCfg, ParamName, (PVOID)pString );
	        return TRUE;
        }


        if( AnscEqualString(ParamName, "X_COMCAST-COM_SyncProtocolVersion", TRUE))
        {
                WalPrint("X_COMCAST-COM_SyncProtocolVersion\n");
	        CosaDmlWEBPA_SetConfiguration( pWebpaCfg, ParamName, (PVOID)pString );
	        return TRUE;
        }
	
	WalPrint("<=========== End of Webpa_SetParamStringValue ========\n");

    return FALSE;
}

ULONG
Webpa_GetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pValue,
        ULONG*                      pUlSize
    )
{
	PCOSA_DATAMODEL_WEBPA       hWebpa    = (PCOSA_DATAMODEL_WEBPA)g_pCosaBEManager->hWebpa;
        PCOSA_DML_WEBPA             pWebpa    = (PCOSA_DML_WEBPA) hWebpa->pWebpa;
	PCOSA_DML_WEBPA_CONFIG      pWebpaCfg = (PCOSA_DML_WEBPA_CONFIG)pWebpa->pWebpaCfg;
	char buf[32] ={'\0'};

	/* Required for xPC sync */
        if( AnscEqualString(ParamName, "X_COMCAST-COM_CID", TRUE))
        {
                WalPrint("X_COMCAST-COM_CID\n");
	        AnscCopyString(pValue, pWebpaCfg->X_COMCAST_COM_CID);
		return 0;
        }
    	
        if( AnscEqualString(ParamName, "X_COMCAST-COM_SyncProtocolVersion", TRUE))
        {
                WalPrint("X_COMCAST-COM_SyncProtocolVersion\n");
	        AnscCopyString(pValue, pWebpaCfg->X_COMCAST_COM_SyncProtocolVersion);
		return 0;
        }
	
	if( AnscEqualString(ParamName, "Version", TRUE))
        {
                WalPrint("Version\n");
                snprintf(buf, sizeof(buf), "%s-%s", WEBPA_PROTOCOL, WEBPA_GIT_VERSION);
                AnscCopyString(pValue, (char *)buf);
                return 0;
        }

	WalError("Unsupported parameter '%s'\n", ParamName);
    return -1;
  
}

BOOL
Webpa_GetParamUlongValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        ULONG*                      puLong
    )
{
        PCOSA_DATAMODEL_WEBPA       hWebpa    = (PCOSA_DATAMODEL_WEBPA)g_pCosaBEManager->hWebpa;
        PCOSA_DML_WEBPA             pWebpa    = (PCOSA_DML_WEBPA) hWebpa->pWebpa;
	PCOSA_DML_WEBPA_CONFIG      pWebpaCfg = (PCOSA_DML_WEBPA_CONFIG)pWebpa->pWebpaCfg;
	
        if( AnscEqualString(ParamName, "X_COMCAST-COM_CMC", TRUE))
        {
                WalPrint("X_COMCAST-COM_CMC\n");
                *puLong = pWebpaCfg->X_COMCAST_COM_CMC;
		return TRUE;
        }
        
        return FALSE;
}

BOOL
Webpa_SetParamUlongValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        ULONG                       uValue
    )
{
        PCOSA_DATAMODEL_WEBPA       hWebpa    = (PCOSA_DATAMODEL_WEBPA)g_pCosaBEManager->hWebpa;
        PCOSA_DML_WEBPA             pWebpa    = (PCOSA_DML_WEBPA) hWebpa->pWebpa;
	PCOSA_DML_WEBPA_CONFIG      pWebpaCfg = (PCOSA_DML_WEBPA_CONFIG)pWebpa->pWebpaCfg;
	
	if( AnscEqualString(ParamName, "X_COMCAST-COM_CMC", TRUE))
        {
                WalPrint("X_COMCAST-COM_CMC\n");
	        if ( TRUE != CosaDmlWEBPA_SetConfiguration( pWebpaCfg, ParamName, (PVOID)&uValue ) )
		{
			return FALSE;
		}
		
		return TRUE;
        }
	
    return FALSE;
}

