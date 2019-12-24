
#include "ansc_platform.h"
#include "cosa_webpa_dml.h"
#include "ccsp_trace.h"
#include "cosa_webpa_internal.h"
#include "ccsp_base_api.h"
#include "plugin_main_apis.h"
#include "webpa_internal.h"
#include "webpa_notification.h"

#define WEBPA_PARAM_VERSION                 "Device.X_RDKCENTRAL-COM_Webpa.Version"
#define WEBPA_PARAM_PROTOCOL_VERSION        "Device.DeviceInfo.Webpa.X_COMCAST-COM_SyncProtocolVersion"
#define WiFi_FactoryResetRadioAndAp	    "Device.WiFi.X_CISCO_COM_FactoryResetRadioAndAp"

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
                char *tmpStr, *notifyStr;
                tmpStr = notifyStr = strdup(pString);

                p_notify_param_name = strsep(&notifyStr, ",");
                p_write_id = strsep(&notifyStr,",");
                p_new_val = strsep(&notifyStr,",");
                p_old_val = strsep(&notifyStr,",");
                p_val_type = strsep(&notifyStr, ",");

                if(p_val_type !=NULL && p_write_id !=NULL)
                {
                        value_type = atoi(p_val_type);
                        write_id = atoi(p_write_id);

                        WalPrint(" \n Notification : Parameter Name = %s \n", p_notify_param_name);
                        WalPrint(" \n Notification : Value Type = %d \n", value_type);
                        WalPrint(" \n Notification : Component ID = %d \n", write_id);
#if 0 /*Removing Logging of Password due to security requirement*/
                        WalPrint(" \n Notification : New Value = %s \n", p_new_val);
                        WalPrint(" \n Notification : Old Value = %s \n", p_old_val);
#endif

			if(NULL != p_notify_param_name && (strcmp(p_notify_param_name, WiFi_FactoryResetRadioAndAp)== 0))
			{
				// sleep for 90s to delay the notification and give wifi time to reset and apply to driver
				WalInfo("Delay wifi factory reset notification by 90s so that wifi is reset completely\n");
				sleep(90);
			}

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
                WAL_FREE(tmpStr);
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
		CosaDmlWEBPA_GetValueFromDB( "X_COMCAST-COM_CID", pWebpaCfg->X_COMCAST_COM_CID );
		AnscCopyString(pValue, pWebpaCfg->X_COMCAST_COM_CID);
		return 0;
        }
    	
        if( AnscEqualString(ParamName, "X_COMCAST-COM_SyncProtocolVersion", TRUE))
        {
                WalPrint("X_COMCAST-COM_SyncProtocolVersion\n");
		/* X_COMCAST-COM_SyncProtocolVersion */
		CosaDmlWEBPA_GetValueFromDB( "X_COMCAST-COM_SyncProtocolVersion", pWebpaCfg->X_COMCAST_COM_SyncProtocolVersion);
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
	char tmpchar[128] = { 0 };
	
        if( AnscEqualString(ParamName, "X_COMCAST-COM_CMC", TRUE))
        {
                WalPrint("X_COMCAST-COM_CMC\n");
				/* X_COMCAST-COM_CMC */
				CosaDmlWEBPA_GetValueFromDB( "X_COMCAST-COM_CMC", tmpchar );
				if(strlen(tmpchar)>0)
				{
					pWebpaCfg->X_COMCAST_COM_CMC = atoi(tmpchar);
		            *puLong = pWebpaCfg->X_COMCAST_COM_CMC;
					return TRUE;
				}
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

int getWebpaParameterValues(char **parameterNames, int paramCount, int *val_size, parameterValStruct_t ***val)
{
    char *webpaObjects[] ={"Device.DeviceInfo.Webpa.", "Device.X_RDKCENTRAL-COM_Webpa.","Device.Webpa.","Device.DeviceInfo."};
    int objSize = sizeof(webpaObjects)/sizeof(webpaObjects[0]);
    parameterValStruct_t **paramVal = NULL;
    paramVal = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t *)*paramCount);
    int i=0, j=0, k=0, isWildcard = 0, matchFound = 0;
    int localCount = paramCount;
    char tmpchar[128] = { 0 };
    WalPrint("*********** %s ***************\n",__FUNCTION__);

    PCOSA_DATAMODEL_WEBPA       hWebpa    = (PCOSA_DATAMODEL_WEBPA)g_pCosaBEManager->hWebpa;
    PCOSA_DML_WEBPA             pWebpa    = (PCOSA_DML_WEBPA) hWebpa->pWebpa;
    PCOSA_DML_WEBPA_CONFIG      pWebpaCfg = (PCOSA_DML_WEBPA_CONFIG)pWebpa->pWebpaCfg;

    WalPrint("paramCount = %d\n",paramCount);
    for(i=0; i<paramCount; i++)
    {
        if(parameterNames[i][strlen(parameterNames[i])-1] == '.')
        {
            isWildcard = 1;
        }
        else
        {
            isWildcard = 0;
        }
        for(j=0; j<objSize; j++)
        {
            if(strstr(parameterNames[i],webpaObjects[j]) != NULL)
            {
                matchFound = 1;
                switch(j)
                {
                    case 0:
                    case 3:
                    {
                        if(isWildcard == 0)
                        {
                            paramVal[k] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t));
                            if((strcmp(parameterNames[i], PARAM_CMC) == 0))
                            {
                                paramVal[k]->parameterName = strndup(PARAM_CMC, MAX_PARAMETERNAME_LEN);
                                paramVal[k]->parameterValue = (char *)malloc(sizeof(char)*MAX_PARAMETERVALUE_LEN);
				if(pWebpaCfg->X_COMCAST_COM_CMC == 0)
				{
					CosaDmlWEBPA_GetValueFromDB( "X_COMCAST-COM_CMC", tmpchar );
					pWebpaCfg->X_COMCAST_COM_CMC = atoi(tmpchar);
				}
                                snprintf(paramVal[k]->parameterValue,MAX_PARAMETERVALUE_LEN,"%d",pWebpaCfg->X_COMCAST_COM_CMC);
                                paramVal[k]->type = ccsp_unsignedInt;
                                k++;
                            }
                            else if(strcmp(parameterNames[i], PARAM_CID) == 0)
                            {
                                paramVal[k]->parameterName = strndup(PARAM_CID, MAX_PARAMETERNAME_LEN);
				if((strlen(pWebpaCfg->X_COMCAST_COM_CID) == 0) || (strcmp(pWebpaCfg->X_COMCAST_COM_CID, "0") == 0))
				{
					WalPrint("CID is empty or 0, get value from DB\n");
					CosaDmlWEBPA_GetValueFromDB( "X_COMCAST-COM_CID", pWebpaCfg->X_COMCAST_COM_CID );
				}
                                paramVal[k]->parameterValue = strndup(pWebpaCfg->X_COMCAST_COM_CID,MAX_PARAMETERVALUE_LEN);
                                paramVal[k]->type = ccsp_string;
                                k++;
                            }
                            else if(strcmp(parameterNames[i], WEBPA_PARAM_PROTOCOL_VERSION) == 0)
                            {
                                paramVal[k]->parameterName = strndup(WEBPA_PARAM_PROTOCOL_VERSION, MAX_PARAMETERNAME_LEN);
				if((strlen(pWebpaCfg->X_COMCAST_COM_SyncProtocolVersion) == 0) || (strcmp(pWebpaCfg->X_COMCAST_COM_SyncProtocolVersion, "0") == 0))
				{
					CosaDmlWEBPA_GetValueFromDB( "X_COMCAST-COM_SyncProtocolVersion", pWebpaCfg->X_COMCAST_COM_SyncProtocolVersion );
				}
                                paramVal[k]->parameterValue = strndup(pWebpaCfg->X_COMCAST_COM_SyncProtocolVersion,MAX_PARAMETERVALUE_LEN);
                                paramVal[k]->type = ccsp_string;
                                k++;
                            }
                            else
                            {
                                WAL_FREE(paramVal[k]);
                                matchFound = 0;
                            }
                        }
                        else
                        {
                            if(strcmp(parameterNames[i],webpaObjects[j]) == 0)
                            {
                                localCount= localCount+2;
                                paramVal = (parameterValStruct_t **) realloc(paramVal, sizeof(parameterValStruct_t *)*localCount);
                                paramVal[k] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t));
                                paramVal[k]->parameterName = strndup(PARAM_CMC, MAX_PARAMETERNAME_LEN);
                                paramVal[k]->parameterValue = (char *)malloc(sizeof(char)*MAX_PARAMETERVALUE_LEN);
				if(pWebpaCfg->X_COMCAST_COM_CMC == 0)
				{
					CosaDmlWEBPA_GetValueFromDB( "X_COMCAST-COM_CMC", tmpchar );
					pWebpaCfg->X_COMCAST_COM_CMC = atoi(tmpchar);

				}
                                snprintf(paramVal[k]->parameterValue,MAX_PARAMETERVALUE_LEN,"%d",pWebpaCfg->X_COMCAST_COM_CMC);
                                paramVal[k]->type = ccsp_unsignedInt;
                                k++;
                                paramVal[k] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t));
                                paramVal[k]->parameterName = strndup(PARAM_CID, MAX_PARAMETERNAME_LEN);
				if((strlen(pWebpaCfg->X_COMCAST_COM_CID) == 0) || (strcmp(pWebpaCfg->X_COMCAST_COM_CID, "0") == 0))
				{
					WalPrint("CID is empty or 0, get value from DB\n");
					CosaDmlWEBPA_GetValueFromDB( "X_COMCAST-COM_CID", pWebpaCfg->X_COMCAST_COM_CID );
				}
                                paramVal[k]->parameterValue = strndup(pWebpaCfg->X_COMCAST_COM_CID,MAX_PARAMETERVALUE_LEN);
                                paramVal[k]->type = ccsp_string;
                                k++;
                                paramVal[k] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t));
                                paramVal[k]->parameterName = strndup(WEBPA_PARAM_PROTOCOL_VERSION, MAX_PARAMETERNAME_LEN);
				if((strlen(pWebpaCfg->X_COMCAST_COM_SyncProtocolVersion) == 0) || (strcmp(pWebpaCfg->X_COMCAST_COM_SyncProtocolVersion, "0") == 0)) 
				{
					CosaDmlWEBPA_GetValueFromDB( "X_COMCAST-COM_SyncProtocolVersion",pWebpaCfg->X_COMCAST_COM_SyncProtocolVersion );
				}
                                paramVal[k]->parameterValue = strndup(pWebpaCfg->X_COMCAST_COM_SyncProtocolVersion,MAX_PARAMETERVALUE_LEN);
                                paramVal[k]->type = ccsp_string;
                                k++;
                            }
                            else
                            {
                                matchFound = 0;
                            }
                        }
                        break;
                    }
                    case 1:
                    {
                        paramVal[k] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t));
                        if((strcmp(parameterNames[i], WEBPA_PARAM_VERSION) == 0) || ((isWildcard == 1) && (strcmp(parameterNames[i],webpaObjects[j]) == 0)))
                        {
                            paramVal[k]->parameterName = strndup(WEBPA_PARAM_VERSION, MAX_PARAMETERNAME_LEN);
                            paramVal[k]->parameterValue = (char*) malloc(sizeof(char)*MAX_PARAMETERVALUE_LEN);
                            snprintf(paramVal[k]->parameterValue,sizeof(char)*MAX_PARAMETERVALUE_LEN,"%s-%s",WEBPA_PROTOCOL, WEBPA_GIT_VERSION);
                            paramVal[k]->type = ccsp_string;
                            k++;
                        }
                        else
                        {
                            WAL_FREE(paramVal[k]);
                            matchFound = 0;
                        }
                        break;
                    }
                    case 2:
                    {
                        WalError("%s parameter GET is not supported through webpa\n",parameterNames[i]);
                        OnboardLog("%s parameter GET is not supported through webpa\n",parameterNames[i]);
                        *val = NULL;
                        *val_size = 0;
                        for(k=k-1;k>=0;k--)
                        {
                            WAL_FREE(paramVal[k]->parameterName);
                            WAL_FREE(paramVal[k]->parameterValue);
                            WAL_FREE(paramVal[k]);
                        }
                        WAL_FREE(paramVal);
                        return CCSP_ERR_METHOD_NOT_SUPPORTED;
                        break;
                    }
                }
                break;
            }
        }
        if(matchFound == 0)
        {
            WalError("%s is invalid parameter\n",parameterNames[i]);
            OnboardLog("%s is invalid parameter\n",parameterNames[i]);
            *val = NULL;
            *val_size = 0;
            for(k=k-1;k>=0;k--)
            {
                WAL_FREE(paramVal[k]->parameterName);
                WAL_FREE(paramVal[k]->parameterValue);
                WAL_FREE(paramVal[k]);
            }
            WAL_FREE(paramVal);
            return CCSP_CR_ERR_UNSUPPORTED_NAMESPACE;
        }
    }
    *val = paramVal;
    for(i=0; i<k; i++)
    {
        WalPrint("Final-> %s %s %d\n",(*val)[i]->parameterName, (*val)[i]->parameterValue, (*val)[i]->type);
    }
    *val_size = k;
    WalPrint("Final count is %d\n",*val_size);
    WalPrint("*********** %s ***************\n",__FUNCTION__);
    return CCSP_SUCCESS;
}

int setWebpaParameterValues(parameterValStruct_t *val, int paramCount, char **faultParam )
{
    int i=0;
    char *object = "Device.DeviceInfo.Webpa.";
    char *subStr = NULL;
    WalPrint("*********** %s ***************\n",__FUNCTION__);

    PCOSA_DATAMODEL_WEBPA       hWebpa    = (PCOSA_DATAMODEL_WEBPA)g_pCosaBEManager->hWebpa;
    PCOSA_DML_WEBPA             pWebpa    = (PCOSA_DML_WEBPA) hWebpa->pWebpa;
    PCOSA_DML_WEBPA_CONFIG      pWebpaCfg = (PCOSA_DML_WEBPA_CONFIG)pWebpa->pWebpaCfg;

    WalPrint("paramCount = %d\n",paramCount);
    for(i=0; i<paramCount; i++)
    {
        if(strstr(val[i].parameterName, object) != NULL)
        {
            subStr = val[i].parameterName+strlen(object);
            WalPrint("subStr = %s\n",subStr);
            if(strcmp(subStr,"X_COMCAST-COM_CID") == 0)
            {
                if(TRUE == CosaDmlWEBPA_StoreValueIntoDB( subStr, val[i].parameterValue ))
                {
                    memset( pWebpaCfg->X_COMCAST_COM_CID, 0, sizeof(pWebpaCfg->X_COMCAST_COM_CID));
                    AnscCopyString(pWebpaCfg->X_COMCAST_COM_CID, val[i].parameterValue);
		        }
		        else
		        {
		            return CCSP_FAILURE;
		        }
            }
            else if(strcmp(subStr, "X_COMCAST-COM_CMC") == 0)
            {
                if(TRUE == CosaDmlWEBPA_StoreValueIntoDB( subStr, val[i].parameterValue ))
                {
                    pWebpaCfg->X_COMCAST_COM_CMC = atoi(val[i].parameterValue);
                }
                else
                {
                    return CCSP_FAILURE;
                }
            }
            else if(strcmp(subStr, "X_COMCAST-COM_SyncProtocolVersion")== 0)
            {
                if(TRUE == CosaDmlWEBPA_StoreValueIntoDB( subStr, val[i].parameterValue ))
                {
                    memset( pWebpaCfg->X_COMCAST_COM_SyncProtocolVersion, 0, sizeof(pWebpaCfg->X_COMCAST_COM_SyncProtocolVersion));
                    AnscCopyString(pWebpaCfg->X_COMCAST_COM_SyncProtocolVersion, val[i].parameterValue);
	            }
	            else
	            {
	                return CCSP_FAILURE;
	            }
            }
            else
            {
                WalError("%s parameter SET is not supported through webpa\n",val[i].parameterName);
                OnboardLog("%s parameter SET is not supported through webpa\n",val[i].parameterName);
                *faultParam = strdup(val[i].parameterName);
                return CCSP_ERR_METHOD_NOT_SUPPORTED;
            }
        }
        else
        {
            WalError("%s is not writable\n",val[i].parameterName);
            OnboardLog("%s is not writable\n",val[i].parameterName);
            *faultParam = strdup(val[i].parameterName);
            return CCSP_ERR_NOT_WRITABLE;
        }
    }
    WalPrint("*********** %s ***************\n",__FUNCTION__);
    return CCSP_SUCCESS;
}
