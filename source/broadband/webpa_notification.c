/**
 * @file webpa_notification.c
 *
 * @description This file describes the Webpa Abstraction Layer
 *
 * Copyright (c) 2015  Comcast
 */

#include <pthread.h>

#include "webpa_notification.h"
#include "webpa_internal.h"

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
/* Notify Macros */
#define WEBPA_SET_INITIAL_NOTIFY_RETRY_COUNT            5
#define WEBPA_SET_INITIAL_NOTIFY_RETRY_SEC              15
#define WEBPA_NOTIFY_EVENT_HANDLE_INTERVAL_MSEC         250
#define WEBPA_NOTIFY_EVENT_MAX_LENGTH                   256
#define MAX_REASON_LENGTH                               64
#define WEBPA_PARAM_HOSTS_NAME		        "Device.Hosts.Host."
#define WRP_TRANSACTION_ID			"transaction_uuid"
#define PARAM_HOSTS_VERSION	        "Device.Hosts.X_RDKCENTRAL-COM_HostVersionId"
#define PARAM_SYSTEM_TIME		        "Device.DeviceInfo.X_RDKCENTRAL-COM_SystemTime"
#define PARAM_FIRMWARE_VERSION		        "Device.DeviceInfo.X_CISCO_COM_FirmwareName"
#define WEBPA_CFG_FILE                     "/nvram/webpa_cfg.json"
#define WEBPA_CFG_FIRMWARE_VER		"oldFirmwareVersion"
#define DEVICE_BOOT_TIME                "Device.DeviceInfo.X_RDKCENTRAL-COM_BootTime"
/*----------------------------------------------------------------------------*/
/*                               Data Structures                              */
/*----------------------------------------------------------------------------*/

typedef struct NotifyMsg__
{
	NotifyData *notifyData;
	struct NotifyMsg__ *next;
} NotifyMsg;

typedef struct
{
    char oldFirmwareVersion[256];
} WebPaCfg;
/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
static NotifyMsg *notifyMsgQ = NULL;
void (*notifyCbFn)(NotifyData*) = NULL;
static WebPaCfg webPaCfg;
char deviceMAC[32]={'\0'};

pthread_mutex_t mut=PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t con=PTHREAD_COND_INITIALIZER;

const char * notifyparameters[]={
"Device.NotifyComponent.X_RDKCENTRAL-COM_Connected-Client",
"Device.Bridging.Bridge.1.Port.8.Enable",
"Device.Bridging.Bridge.2.Port.2.Enable",
"Device.DeviceInfo.X_COMCAST_COM_xfinitywifiEnable",
"Device.WiFi.AccessPoint.10001.Security.ModeEnabled",
"Device.WiFi.AccessPoint.10001.Security.X_COMCAST-COM_KeyPassphrase",
"Device.WiFi.AccessPoint.10001.SSIDAdvertisementEnabled",
"Device.WiFi.AccessPoint.10001.X_CISCO_COM_MACFilter.Enable",
"Device.WiFi.AccessPoint.10001.X_CISCO_COM_MACFilter.FilterAsBlackList",
"Device.WiFi.AccessPoint.10101.Security.ModeEnabled",
"Device.WiFi.AccessPoint.10101.Security.X_COMCAST-COM_KeyPassphrase",
"Device.WiFi.AccessPoint.10101.SSIDAdvertisementEnabled",
"Device.WiFi.AccessPoint.10101.X_CISCO_COM_MACFilter.Enable",
"Device.WiFi.AccessPoint.10101.X_CISCO_COM_MACFilter.FilterAsBlackList",
"Device.WiFi.Radio.10000.Enable",
"Device.WiFi.Radio.10100.Enable",
"Device.WiFi.SSID.10001.Enable",
"Device.WiFi.SSID.10001.SSID",
"Device.WiFi.SSID.10101.Enable",
"Device.WiFi.SSID.10101.SSID",
"Device.X_CISCO_COM_DeviceControl.LanManagementEntry.1.LanMode",
"Device.X_CISCO_COM_Security.Firewall.FilterAnonymousInternetRequests",
"Device.X_CISCO_COM_Security.Firewall.FilterHTTP",
"Device.X_CISCO_COM_Security.Firewall.FilterIdent",
"Device.X_CISCO_COM_Security.Firewall.FilterMulticast",
"Device.X_CISCO_COM_Security.Firewall.FilterP2P",
"Device.X_CISCO_COM_Security.Firewall.FirewallLevel",
// Commenting this parameter to avoid PSM crash
//"Device.DHCPv4.Server.Enable",
"Device.X_CISCO_COM_DeviceControl.LanManagementEntry.1.LanIPAddress",
"Device.X_CISCO_COM_DeviceControl.LanManagementEntry.1.LanSubnetMask",
"Device.DHCPv4.Server.Pool.1.MinAddress",
"Device.DHCPv4.Server.Pool.1.MaxAddress",
"Device.DHCPv4.Server.Pool.1.LeaseTime",
// Commenting as some boxes may not have this parameter
//"Device.Routing.Router.1.IPv4Forwarding.1.GatewayIPAddress",
"Device.NAT.X_CISCO_COM_DMZ.Enable",
"Device.NAT.X_CISCO_COM_DMZ.InternalIP",
"Device.NAT.X_CISCO_COM_DMZ.IPv6Host",
"Device.NAT.X_Comcast_com_EnablePortMapping"
};
/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/
static void loadCfgFile();
static void getDeviceMac();
static int writeToJson(char *data);
static PARAMVAL_CHANGE_SOURCE mapWriteID(unsigned int writeID);
static void *notifyTask(void *status);
static void notifyCallback(NotifyData *notifyData);
static void addNotifyMsgToQueue(NotifyData *notifyData);
static void handleNotificationEvents();
static void freeNotifyMessage(NotifyData *notifyData);
static void getNotifyParamList(const char ***paramList,int *size);
void processNotification(NotifyData *notifyData);
void sendNotificationForFactoryReset();
void sendNotificationForFirmwareUpgrade();
static WDMP_STATUS addOrUpdateFirmwareVerToConfigFile(char *value);
static WDMP_STATUS processParamNotification(ParamNotify *paramNotify, unsigned int *cmc, char **cid);
static void processConnectedClientNotification(NodeData *connectedNotify, char *deviceId, char **version, char ** nodeMacId, char **timeStamp, char **destination);
static WDMP_STATUS processFactoryResetNotification(ParamNotify *paramNotify, unsigned int *cmc, char **cid);
static WDMP_STATUS processFirmwareUpgradeNotification(ParamNotify *paramNotify, unsigned int *cmc, char **cid);
void processDeviceStatusNotification(int status);
static void mapComponentStatusToGetReason(COMPONENT_STATUS status, char *reason);

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/


void initNotifyTask(int status)
{
	int err = 0;
	pthread_t threadId;
	notifyMsgQ = NULL;
	int *device_status = (int *) malloc(sizeof(int));
	*device_status = status;

	err = pthread_create(&threadId, NULL, notifyTask, (void *) device_status);
	if (err != 0) 
	{
		WalError("Error creating notifyTask thread :[%s]\n", strerror(err));
	}
	else
	{
		WalPrint("notifyTask Thread created Successfully\n");
	}
}

void ccspWebPaValueChangedCB(parameterSigStruct_t* val, int size, void* user_data)
{
	WalPrint("Inside CcspWebpaValueChangedCB\n");

	ParamNotify *paramNotify;

	if (NULL == notifyCbFn) 
	{
		WalError("Fatal: ccspWebPaValueChangedCB() notifyCbFn is NULL\n");
		return;
	}

	paramNotify= (ParamNotify *) malloc(sizeof(ParamNotify));
	paramNotify->paramName = val->parameterName;
	paramNotify->oldValue= val->oldValue;
	paramNotify->newValue = val->newValue;
	paramNotify->type = val->type;
	paramNotify->changeSource = mapWriteID(val->writeID);

	NotifyData *notifyDataPtr = (NotifyData *) malloc(sizeof(NotifyData) * 1);
	notifyDataPtr->type = PARAM_NOTIFY;
	notifyDataPtr->u.notify = paramNotify;

	WalInfo("Notification Event from stack: Parameter Name: %s, Data Type: %d, Change Source: %d\n", paramNotify->paramName, paramNotify->type, paramNotify->changeSource);

	(*notifyCbFn)(notifyDataPtr);
}

int RegisterNotifyCB(notifyCB cb)
{
	notifyCbFn = cb;
	return 1;
}

void * getNotifyCB()
{
	WalPrint("Inside getNotifyCB\n");
	return notifyCbFn;
}

void processTransactionNotification(char transId[])
{
	WalPrint("processTransactionNotification\n");
	if (NULL == notifyCbFn)
	{
		WalError("Fatal: notifyCbFn is NULL\n");
		return;
	}
	else
	{
		
		WalPrint("transId : %s\n",transId);
		WalPrint("Allocate memory to NotifyData \n");
		NotifyData *notifyDataPtr = (NotifyData *) malloc(sizeof(NotifyData) * 1);
		memset(notifyDataPtr,0,sizeof(NotifyData));

		notifyDataPtr->type = TRANS_STATUS;
		notifyDataPtr->u.status = (TransData*) malloc(sizeof(TransData));
		notifyDataPtr->u.status->transId = (char *)malloc(sizeof(char) * (strlen(transId)+1));
		walStrncpy(notifyDataPtr->u.status->transId, transId, (strlen(transId)+1));
		WalPrint("notifyDataPtr->u.status->transId : %s\n",notifyDataPtr->u.status->transId);
		(*notifyCbFn)(notifyDataPtr);
	}
}

void sendConnectedClientNotification(char * macId, char *status, char *interface, char *hostname)
{

	NotifyData *notifyDataPtr = (NotifyData *) malloc(sizeof(NotifyData) * 1);
	NodeData * node = NULL;

	notifyDataPtr->type = CONNECTED_CLIENT_NOTIFY;
	if(macId != NULL && status != NULL && interface != NULL && hostname != NULL)
	{
		node = (NodeData *) malloc(sizeof(NodeData) * 1);
		memset(node, 0, sizeof(NodeData));
		WalPrint("macId : %s status : %s interface : %s hostname :%s\n",macId,status, interface, hostname);
		node->nodeMacId = (char *)(malloc(sizeof(char) * strlen(macId) + 1));
		strncpy(node->nodeMacId, macId, strlen(macId) + 1);

		node->status = (char *)(malloc(sizeof(char) * strlen(status) + 1));
		strncpy(node->status, status, strlen(status) + 1);
		
		node->interface = (char *)(malloc(sizeof(char) * strlen(interface) + 1));
		strncpy(node->interface, interface, strlen(interface) + 1);
		
		node->hostname = (char *)(malloc(sizeof(char) * strlen(hostname) + 1));
		strncpy(node->hostname, hostname, strlen(hostname) + 1);
		
		WalPrint("node->nodeMacId : %s node->status: %s node->interface: %s node->hostname: %s\n",node->nodeMacId,node->status, node->interface, node->hostname);
	}

	notifyDataPtr->u.node = node;

	(*notifyCbFn)(notifyDataPtr);
}
/*----------------------------------------------------------------------------*/
/*                               Internal functions                              */
/*----------------------------------------------------------------------------*/

/*
 * @brief loadCfgFile To load the config file.
 */
static void loadCfgFile()
{
	FILE *fp;
	cJSON *webpa_cfg = NULL;
	char *cfg_file_content = NULL, *temp_ptr = NULL;
	int ch_count = 0;
	int flag = 0;
	fp = fopen(WEBPA_CFG_FILE, "r");
	if (fp == NULL)
	{
		WalError("Failed to open cfg file in read mode creating new file %s\n", WEBPA_CFG_FILE);
		fp = fopen(WEBPA_CFG_FILE, "w");
		if (fp == NULL)
		{
			WalError("Failed to create cfg file %s\n", WEBPA_CFG_FILE);
			return;
		}
		flag = 1;
		fprintf(fp, "{\n}");
	}

	fseek(fp, 0, SEEK_END);
	ch_count = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	cfg_file_content = (char *) malloc(sizeof(char) * (ch_count + 1));
	fread(cfg_file_content, 1, ch_count,fp);
	cfg_file_content[ch_count] ='\0';
	WalPrint("cfg_file_content : \n%s\n",cfg_file_content);
	fclose(fp);

	if(flag == 0)
	{
		webpa_cfg = cJSON_Parse(cfg_file_content);

		if(webpa_cfg)
		{
			WalPrint("**********Loading Webpa Config***********\n");

			if(cJSON_GetObjectItem(webpa_cfg, WEBPA_CFG_FIRMWARE_VER) != NULL)
			{
				temp_ptr = cJSON_GetObjectItem(webpa_cfg, WEBPA_CFG_FIRMWARE_VER)->valuestring;
				strncpy(webPaCfg.oldFirmwareVersion, temp_ptr, strlen(temp_ptr)+1);
				WalPrint("oldFirmwareVersion : %s\n", webPaCfg.oldFirmwareVersion);
			}
			else
			{
				strcpy(webPaCfg.oldFirmwareVersion,"");
			}
		}
		else
		{
			WalError("Error parsing WebPA config file\n");
		}
	}
	free(cfg_file_content);
}

/**
 * @brief getNotifyParamList Get notification parameters from intial NotifList
 *      returns notif parameter names and size of list
 *
 * @param[inout] paramList Initial Notif parameters list
 * @param[inout] size Notif List array size
 */
static void getNotifyParamList(const char ***paramList, int *size)
{
	*size = sizeof(notifyparameters)/sizeof(notifyparameters[0]);
	WalPrint("Notify param list size :%d\n", *size);
	*paramList = notifyparameters;
}

/**
 * @brief To turn on notification for the parameters extracted from the notifyList of the config file.
 */
static void setInitialNotify()
{
	WalPrint("***************Inside setInitialNotify*****************\n");
	int i = 0, isError = 0, retry = 0;
	char notif[20] = "";
	const char **notifyparameters = NULL;
	int notifyListSize = 0;

	int backoffRetryTime = 0;
	int backoff_max_time = 5;
        int max_retry_sleep;
	//Retry Backoff count will start at c=2 & calculate 2^c - 1.
	int c = 2;
	
	max_retry_sleep = (int) pow(2, backoff_max_time) -1;
        WalInfo("setInitialNotify max_retry_sleep is %d\n", max_retry_sleep );

	getNotifyParamList(&notifyparameters, &notifyListSize);

	//notifyparameters is empty for webpa-video
	if (notifyparameters != NULL)
	{
		int *setInitialNotifStatus = (int *) malloc(
				sizeof(int) * notifyListSize);

		WDMP_STATUS ret = WDMP_FAILURE;
		param_t *attArr = NULL;

		for (i = 0; i < notifyListSize; i++)
		{
			setInitialNotifStatus[i] = 0;
		}

		do
		{
			if(backoffRetryTime < max_retry_sleep)
                	{
				backoffRetryTime = (int) pow(2, c) - 1;
			}
			
			WalPrint("setInitialNotify backoffRetryTime calculated as %d seconds\n", backoffRetryTime);

			isError = 0;
			WalPrint("notify List Size: %d\n", notifyListSize);
			attArr = (param_t *) malloc(sizeof(param_t));
			for (i = 0; i < notifyListSize; i++)
			{
				if (setInitialNotifStatus[i] == 0)
				{
					snprintf(notif, sizeof(notif), "%d", 1);
					attArr[0].value = (char *) malloc(sizeof(char) * 20);
					walStrncpy(attArr[0].value, notif, 20);
					attArr[0].name = (char *) notifyparameters[i];
					attArr[0].type = WDMP_INT;
					WalPrint("notifyparameters[%d]: %s\n", i,notifyparameters[i]);
					setAttributes(attArr, 1, NULL, &ret);
					if (ret != WDMP_SUCCESS)
					{
						isError = 1;
						setInitialNotifStatus[i] = 0;
						WalError("Failed to turn notification ON for parameter : %s ret: %d Attempt Number: %d\n",
								notifyparameters[i], ret, retry + 1);
					}
					else
					{
						setInitialNotifStatus[i] = 1;
						WalInfo("Successfully set notification ON for parameter : %s ret: %d\n",notifyparameters[i], ret);
					}
					WAL_FREE(attArr[0].value);
				}
			}

			WAL_FREE(attArr);

			if (isError == 0)
			{
				WalInfo("Successfully set initial notifications\n");
				break;
			}

			WalInfo("setInitialNotify backoffRetryTime %d seconds, retry:%d\n",	backoffRetryTime, retry);
			sleep(backoffRetryTime);
			c++;
			
			if(backoffRetryTime == max_retry_sleep)
                        {
                        	c = 2;
                        	backoffRetryTime = 0;
                        	WalPrint("setInitialNotify backoffRetryTime reached max value, reseting to initial value\n");
                        }

		} while (retry++ < WEBPA_SET_INITIAL_NOTIFY_RETRY_COUNT);

		WAL_FREE(setInitialNotifStatus);

		WalPrint("**********************End of setInitial Notify************************\n");
	}
	else
	{
		WalError("Initial Notification list is empty\n");
	}
}

/**
 * @brief mapWriteID maps write id and returns change source
 *
 * @param[in] writeID
 */
static PARAMVAL_CHANGE_SOURCE mapWriteID(unsigned int writeID)
{
	PARAMVAL_CHANGE_SOURCE source;
	WalPrint("WRITE ID is %d\n", writeID);

	switch(writeID)
	{
		case CCSP_COMPONENT_ID_ACS:
			source = CHANGED_BY_ACS;
			break;
		case CCSP_COMPONENT_ID_WebPA:
			source = CHANGED_BY_WEBPA;
			break;
		case CCSP_COMPONENT_ID_XPC:
			source = CHANGED_BY_XPC;
			break;
		case DSLH_MPA_ACCESS_CONTROL_CLIENTTOOL:
			source = CHANGED_BY_CLI;
			break;
		case CCSP_COMPONENT_ID_SNMP:
			source = CHANGED_BY_SNMP;
			break;
		case CCSP_COMPONENT_ID_WebUI:
			source = CHANGED_BY_WEBUI;
			break;
		default:
			source = CHANGED_BY_UNKNOWN;
			break;
	}

	WalInfo("CMC/component_writeID is: %d\n", source);
	return source;
}

/*
 * @brief To handle notification tasks
 */
static void *notifyTask(void *status)
{
	pthread_detach(pthread_self());
	getDeviceMac();
	loadCfgFile();
	processDeviceStatusNotification(*(int *)status);
	RegisterNotifyCB(&notifyCallback);
	sendNotificationForFactoryReset();
	sendNotificationForFirmwareUpgrade();
	setInitialNotify();
	handleNotificationEvents();
	WAL_FREE(status);
	WalPrint("notifyTask ended!\n");
	return NULL;
}

static void getDeviceMac()
{
        char *macID = NULL;
        char deviceMACValue[32] = { '\0' };
	int retryCount = 0;
	int backoffRetryTime = 0;  
	int c=2;
	
        if(strlen(deviceMAC) == 0)
        {
                do
                {
                        backoffRetryTime = (int) pow(2, c) -1;	
                        macID = getParameterValue(DEVICE_MAC);
	                if (macID != NULL)
	                {
		                strncpy(deviceMACValue, macID, strlen(macID)+1);
		                macToLower(deviceMACValue, deviceMAC);
		                WalInfo("deviceMAC: %s\n",deviceMAC);
		                WAL_FREE(macID);
	                }
	                else
	                {
	                        WalError("Failed to GetValue for MAC. Retrying...\n");
	                        WalInfo("backoffRetryTime %d seconds\n", backoffRetryTime);
			        sleep(backoffRetryTime);
                                c++;
			        retryCount++;
	                }
	        }while((retryCount >= 1) && (retryCount <= 5));
	        
	}

}
/*
 * @brief notifyCallback is to check if notification event needs to be sent
 *  @param[in] paramNotify parameters to be notified .
 */
static void notifyCallback(NotifyData *notifyData)
{
	addNotifyMsgToQueue(notifyData);
}

/*
 * @brief To add Notification message to queue 
 */
static void addNotifyMsgToQueue(NotifyData *notifyData)
{
	NotifyMsg *message;


	message = (NotifyMsg *)malloc(sizeof(NotifyMsg));

	if(message)
	{
		message->notifyData = notifyData;
		message->next = NULL;
		pthread_mutex_lock (&mut);
		WalPrint("addNotifyMsgToQueue :mutex lock in producer thread\n");
		if(notifyMsgQ == NULL)
		{
			notifyMsgQ = message;
			WalPrint("addNotifyMsgToQueue : Producer added message\n");
		 	pthread_cond_signal(&con);
			pthread_mutex_unlock (&mut);
			WalPrint("addNotifyMsgToQueue :mutex unlock in producer thread\n");
		}
		else
		{
			NotifyMsg *temp = notifyMsgQ;
			while(temp->next)
			{
				temp = temp->next;
			}
			temp->next = message;
                        pthread_mutex_unlock (&mut);
		}
	}
	else
	{
		//Memory allocation failed
		WalError("Memory allocation is failed\n");
	}
	WalPrint("*****Returned from addNotifyMsgToQueue*****\n");
}


/*
 * @brief To monitor notification events in Notification queue
 */
static void handleNotificationEvents()
{
	while(1)
	{
		pthread_mutex_lock (&mut);
		WalPrint("handleNotificationEvents : mutex lock in consumer thread\n");
		if(notifyMsgQ != NULL)
		{
			NotifyMsg *message = notifyMsgQ;
			notifyMsgQ = notifyMsgQ->next;
			pthread_mutex_unlock (&mut);
			WalPrint("handleNotificationEvents : mutex unlock in consumer thread\n");
			NotifyData *notifyData = message->notifyData;
			processNotification(notifyData);
			WAL_FREE(message);
		}
		else
		{		
			WalPrint("handleNotificationEvents : Before pthread cond wait in consumer thread\n");   
			pthread_cond_wait(&con, &mut);
			pthread_mutex_unlock (&mut);
			WalPrint("handleNotificationEvents : mutex unlock in consumer thread after cond wait\n");
		}
	}
}

/*
 * @brief To handle notification during Factory reset
 */
void sendNotificationForFactoryReset()
{
	WalPrint("sendNotificationForFactoryReset\n");
	NotifyData *notifyData = (NotifyData *)malloc(sizeof(NotifyData));
	memset(notifyData,0,sizeof(NotifyData));

	notifyData->type = FACTORY_RESET;
	processNotification(notifyData);

}

static int writeToJson(char *data)
{
	FILE *fp;
	fp = fopen(WEBPA_CFG_FILE, "w");
	if (fp == NULL)
	{
		WalPrint("Failed to open file %s\n", WEBPA_CFG_FILE);
		return -1;
	}

	fwrite(data, strlen(data), 1, fp);
	fclose(fp);
	return 0;
}

/*
 * @brief To add or update webpa config file with the "oldFirmwareVersion"
 */
static WDMP_STATUS addOrUpdateFirmwareVerToConfigFile(char *value)
{
	char *data = NULL;
	cJSON *cfgValObj = NULL;
	cJSON *json = NULL;
	FILE *fileRead;
	char *cJsonOut =NULL;
	int len;
	int configUpdateStatus = -1;
	fileRead = fopen( WEBPA_CFG_FILE, "r+" );    
	if( fileRead == NULL ) 
	{
		WalError( "Error opening file in read mode\n" );
		return WDMP_FAILURE;
	}

	fseek( fileRead, 0, SEEK_END );
	len = ftell( fileRead );
	fseek( fileRead, 0, SEEK_SET );
	data = ( char* )malloc( len + 1 );
	if (data != NULL) {
		fread( data, 1, len, fileRead );
	} else {
		WalError("malloc() failed\n");
	}

	fclose( fileRead );

	if( data != NULL ) 
	{
		json = cJSON_Parse( data );
		if( !json )
		{
			WalError( "json parse error: [%s]\n", cJSON_GetErrorPtr() );
		}
		else
		{
			cfgValObj = cJSON_GetObjectItem( json, WEBPA_CFG_FIRMWARE_VER );
			if( cfgValObj != NULL)
			{
				cJSON_ReplaceItemInObject(json, WEBPA_CFG_FIRMWARE_VER, cJSON_CreateString(value));
				WalPrint("Updated current firmware in config file %s\n",value);

			}
			else {
				cJSON_AddStringToObject(json, WEBPA_CFG_FIRMWARE_VER, value);
				WalPrint("Firmware version is not available in webpa_cfg.json, adding %s as %s\n",WEBPA_CFG_FIRMWARE_VER,value);
			}

			cJsonOut = cJSON_Print(json);
			configUpdateStatus = writeToJson(cJsonOut);
			if (configUpdateStatus == 0)
			{
				WalPrint("Updated current Firmware version to config file\n");
				WAL_FREE(cJsonOut);
				cJSON_Delete(json);
				WAL_FREE(data);
				return WDMP_SUCCESS;
			}
			else
			{
				WalPrint("Unable to update Firmware version to config file\n");
			}
			WAL_FREE(cJsonOut);
			cJSON_Delete(json);
		}
		WAL_FREE(data);
	}
	return WDMP_FAILURE;
}

/*
 * @brief To send notification during Firmware Upgrade
 * GET "oldFirmwareVersion" from the /nvram/webpa_cfg.json file and compare
 * with the current device firmware version. If they dont match then update "oldFirmwareVersion"
 * with the latest version, update CMC and send notification to XPC indicating Firmware Upgrade
 */
void sendNotificationForFirmwareUpgrade()
{
	WDMP_STATUS configUpdateStatus = WDMP_FAILURE;
	char *cur_firmware_ver = NULL;

	cur_firmware_ver = getParameterValue(PARAM_FIRMWARE_VERSION);

	if(cur_firmware_ver == NULL)
	{
		WalError("Could not GET the current device Firmware version\n");
		return;
	}
	WalPrint("cur_firmware_ver: %s\n", cur_firmware_ver);
	WalPrint("webPaCfg.oldFirmwareVersion : %s\n",webPaCfg.oldFirmwareVersion);

	if(strlen(webPaCfg.oldFirmwareVersion) == 0 ||
		(strcmp(webPaCfg.oldFirmwareVersion,cur_firmware_ver) != 0))
	{
		WalInfo("oldFirmwareVer :%s, cur_firmware_ver value :%s\n", webPaCfg.oldFirmwareVersion, cur_firmware_ver);
		configUpdateStatus = addOrUpdateFirmwareVerToConfigFile(cur_firmware_ver);
		if(configUpdateStatus == WDMP_SUCCESS)
		{
			WalInfo("Added/Updated Firmware version details to WebPA config file\n");
		}
		else
		{
			WalError("Error in adding/updating Firmware details to WebPa config file\n");
		}
		// Update config structure with the current device firmware version
		walStrncpy(webPaCfg.oldFirmwareVersion,cur_firmware_ver,sizeof(webPaCfg.oldFirmwareVersion));

		NotifyData *notifyData = (NotifyData *)malloc(sizeof(NotifyData));
		memset(notifyData,0,sizeof(NotifyData));

		notifyData->type = FIRMWARE_UPGRADE;
		processNotification(notifyData);
	}
	else if(strcmp(webPaCfg.oldFirmwareVersion,cur_firmware_ver) == 0)
	{
		WalInfo("Current device firmware version %s is same as old firmware version in config file %s\n",cur_firmware_ver,webPaCfg.oldFirmwareVersion);
	}
	WAL_FREE(cur_firmware_ver);
}

/*
 * @brief To handle notification for all notification types
 */
void processNotification(NotifyData *notifyData)
{
	WDMP_STATUS ret = WDMP_FAILURE;
	char device_id[32] = { '\0' };
	char *cid = NULL, *dest = NULL, *version = NULL, *timeStamp =
			NULL, *nodeMacId = NULL, *source = NULL;
	cJSON * notifyPayload;
	cJSON * nodes, *one_node;
	char *stringifiedNotifyPayload = NULL;
	notifyPayload = cJSON_CreateObject();
	unsigned int cmc;
	char *strBootTime = NULL;
	char *reason = NULL;

	snprintf(device_id, sizeof(device_id), "mac:%s", deviceMAC);
	WalPrint("Device_id %s\n", device_id);

	cJSON_AddStringToObject(notifyPayload, "device_id", device_id);

	dest = (char*) malloc(sizeof(char) * WEBPA_NOTIFY_EVENT_MAX_LENGTH);

        if (dest != NULL)
        {
            if (notifyData != NULL)
            {
	        switch (notifyData->type)
	        {
	        	case PARAM_NOTIFY:
	        	{
	        		strcpy(dest, "event:SYNC_NOTIFICATION");

	        		ret = processParamNotification(notifyData->u.notify, &cmc, &cid);

	        		if (ret != WDMP_SUCCESS)
	        		{
	        			free(dest);
	        			return;
	        		}
	        		cJSON_AddNumberToObject(notifyPayload, "cmc", cmc);
	        		cJSON_AddStringToObject(notifyPayload, "cid", cid);
	        	}
	        		break;

	        	case FACTORY_RESET:
	        	{
	        		WalPrint("----- Inside FACTORY_RESET type -----\n");

	        		strcpy(dest, "event:SYNC_NOTIFICATION");

	        		ret = processFactoryResetNotification(notifyData->u.notify, &cmc, &cid);

	        		if (ret != WDMP_SUCCESS)
	        		{
	        			free(dest);
	        			return;
	        		}
	        		WalPrint("Framing notifyPayload for Factory reset\n");
	        		cJSON_AddNumberToObject(notifyPayload, "cmc", cmc);
	        		cJSON_AddStringToObject(notifyPayload, "cid", cid);
	        	}
	        		break;

	        	case FIRMWARE_UPGRADE:
	        		{
	        			WalPrint("----- Inside FIRMWARE_UPGRADE type -----\n");

	        			strcpy(dest, "event:SYNC_NOTIFICATION");

	        			ret = processFirmwareUpgradeNotification(notifyData->u.notify, &cmc, &cid);

	        			if (ret != WDMP_SUCCESS)
	        			{
	        				free(dest);
	        				return;
	        			}
	        			WalPrint("Framing notifyPayload for Firmware upgrade\n");
	        			cJSON_AddNumberToObject(notifyPayload, "cmc", cmc);
	        			cJSON_AddStringToObject(notifyPayload, "cid", cid);
	        		}
	        			break;

	        	case CONNECTED_CLIENT_NOTIFY:
	        	{
	        		WalPrint("Processing connected client notification\n");
	        		processConnectedClientNotification(notifyData->u.node, device_id,
	        				&version, &nodeMacId, &timeStamp, &dest);

	        		cJSON_AddStringToObject(notifyPayload, "timestamp",
	        				(NULL != timeStamp) ? timeStamp : "unknown");
	        		cJSON_AddItemToObject(notifyPayload, "nodes", nodes =
	        				cJSON_CreateArray());
	        		cJSON_AddItemToArray(nodes, one_node = cJSON_CreateObject());
	        		cJSON_AddStringToObject(one_node, "name", WEBPA_PARAM_HOSTS_NAME);
	        		cJSON_AddNumberToObject(one_node, "version",
	        				(NULL != version) ? atoi(version) : 0);
	        		cJSON_AddStringToObject(one_node, "node-mac",
	        				(NULL != nodeMacId) ? nodeMacId : "unknown");
	        		cJSON_AddStringToObject(one_node, "interface",
	        				(NULL != notifyData->u.node->interface) ? notifyData->u.node->interface : "unknown");
	        		cJSON_AddStringToObject(one_node, "hostname",
	        				(NULL != notifyData->u.node->hostname) ? notifyData->u.node->hostname : "unknown");
	        		cJSON_AddStringToObject(one_node, "status",
	        				(NULL != notifyData->u.node->status) ? notifyData->u.node->status : "unknown");
	        		if (NULL != nodeMacId) {
	        			free(nodeMacId);
	        		}

	        		if (NULL != version) {
	        			free(version);
	        		}
	        	}
	        		break;

	        	case TRANS_STATUS:
	        	{
	        		strcpy(dest, "event:transaction-status");

	        		cJSON_AddStringToObject(notifyPayload, "state", "complete");
	        		if (notifyData->u.status != NULL)
	        		{
	        			cJSON_AddStringToObject(notifyPayload, WRP_TRANSACTION_ID,
	        					(NULL != notifyData->u.status->transId)? notifyData->u.status->transId : "unknown");
	        		}
	        		else
	        		{
	        			free(dest);
	        			return;
	        		}
	        	}
	        		break;

	        	case DEVICE_STATUS:
	        	{
				strBootTime = getParameterValue(DEVICE_BOOT_TIME);
                                if(notifyData->u.device->status != 0)
                                {
                                        reason = (char *)malloc(sizeof(char)*MAX_REASON_LENGTH);
				        mapComponentStatusToGetReason(notifyData->u.device->status, reason);
                                        snprintf(dest, WEBPA_NOTIFY_EVENT_MAX_LENGTH, "event:device-status/%s/non-operational/%s/%s", device_id,(NULL != strBootTime)?strBootTime:"unknown",reason);
                                        cJSON_AddStringToObject(notifyPayload, "status", "non-operational");
                                        cJSON_AddStringToObject(notifyPayload, "reason", reason);
                                        WAL_FREE(reason);
                                }
                                else
                                {
                                        snprintf(dest, WEBPA_NOTIFY_EVENT_MAX_LENGTH, "event:device-status/%s/operational/%s", device_id,(NULL != strBootTime)?strBootTime:"unknown");
                                        cJSON_AddStringToObject(notifyPayload, "status", "operational");
                                }
				WalPrint("dest: %s\n",dest);
				cJSON_AddStringToObject(notifyPayload,"boot-time", (NULL != strBootTime)?strBootTime:"unknown");
				if (strBootTime != NULL)
				{
					WAL_FREE(strBootTime);
				}
			}
	        		break;

	        	default:
	        		break;
	        }

	        stringifiedNotifyPayload = cJSON_PrintUnformatted(notifyPayload);
	        WalPrint("stringifiedNotifyPayload %s\n", stringifiedNotifyPayload);

	        if (stringifiedNotifyPayload != NULL
	        		&& strlen(device_id) != 0)
	        {
	        	source = (char*) malloc(sizeof(char) * sizeof(device_id));
	        	walStrncpy(source, device_id, sizeof(device_id));
	        	sendNotification(stringifiedNotifyPayload, source, dest);
	        	WalPrint("After sendNotification\n");
	        }

	        WalPrint("Freeing notifyData ....\n");
	        freeNotifyMessage(notifyData);
	        WalPrint("notifyData is freed.\n");
	    }

	    free(dest);
        }
}

/*
 * @brief To process notification during value change
 */
static WDMP_STATUS processParamNotification(ParamNotify *paramNotify,
		unsigned int *cmc, char **cid)
{
	char *strCMC = NULL, *strCID = NULL;
	char strNewCMC[20] = { '\0' };
	unsigned int oldCMC, newCMC;
	WDMP_STATUS status = WDMP_FAILURE;

	strCMC = getParameterValue(PARAM_CMC);
	if (strCMC != NULL)
	{
		oldCMC = atoi(strCMC);
		newCMC = oldCMC | paramNotify->changeSource;
		WalInfo("Notification received from stack Old CMC: %s (%d), newCMC: %d\n",
				strCMC, oldCMC, newCMC);

		WAL_FREE(strCMC);
		if (newCMC != oldCMC)
		{
			WalPrint("NewCMC and OldCMC not equal.\n");
			sprintf(strNewCMC, "%d", newCMC);
			status = setParameterValue(PARAM_CMC, strNewCMC, WDMP_UINT);

			if (status == WDMP_SUCCESS)
			{
				WalPrint("Successfully set newCMC value %s\n", strNewCMC);
				strCID = getParameterValue(PARAM_CID);
				if (strCID != NULL)
				{
					WalPrint("ConfigID string is : %s\n", strCID);
					(*cid) = strCID;
					(*cmc) = newCMC;

					return WDMP_SUCCESS;
				}
				else
				{
					WalError("Failed to Get CID value\n");
				}
			}
			else
			{
				WalError("Error in setting new CMC value\n");
			}
		}
		else
		{
			WalInfo("No change in CMC value, ignoring value change event\n");
		}
	}
	else
	{
		WalError("Failed to Get CMC Value, hence ignoring the notification\n");
	}
	return status;
}

/*
 * @brief To process notification during device status
 */
void processDeviceStatusNotification(int status)
{
	WalPrint("processDeviceStatusNotification\n");
	NotifyData *notifyData = (NotifyData *)malloc(sizeof(NotifyData));
	memset(notifyData,0,sizeof(NotifyData));

	notifyData->type = DEVICE_STATUS;
	notifyData->u.device = (DeviceStatus*) malloc(sizeof(DeviceStatus));
	notifyData->u.device->status = status;
	WalPrint("notifyData->u.device->status : %d\n",notifyData->u.device->status);
	processNotification(notifyData);
}

/*
 * @brief To process notification during factory reset
 */
static WDMP_STATUS processFactoryResetNotification(ParamNotify *paramNotify, unsigned int *cmc, char **cid)
{
	char *dbCID = NULL;
	char newCMC[32]={'\0'};
	char *reboot_reason = NULL;
	WDMP_STATUS status = WDMP_FAILURE;
	
	WalPrint("Inside processFactoryResetNotification ..\n");
	snprintf(newCMC, sizeof(newCMC), "%d", CHANGED_BY_FACTORY_DEFAULT);
	WalPrint("newCMC value is %s\n", newCMC);

	dbCID = getParameterValue(PARAM_CID);
	WalPrint("dbCID value is %s\n", dbCID);

	reboot_reason = getParameterValue(PARAM_REBOOT_REASON);
	WalInfo("Received reboot_reason as:%s\n", reboot_reason ? reboot_reason : "reboot_reason is NULL");

	if((NULL != dbCID) && (NULL != reboot_reason) && (strcmp(reboot_reason,"factory-reset")==0) )
	{
		WalInfo("Send notification to server as reboot reason indicates factory reset\n");
		// set CMC to the new value
		status = setParameterValue(PARAM_CMC,newCMC, WDMP_UINT);
		if(status == WDMP_SUCCESS)
		{
			WalInfo("Successfully Set CMC to %d\n", atoi(newCMC));
			(*cid) = dbCID;
			(*cmc) = atoi(newCMC);
			WalPrint("Returning success status from processFactoryResetNotification..\n");
			WAL_FREE(reboot_reason);
			return WDMP_SUCCESS;
			
		}
		else
		{
			WalError("Error setting CMC value for factory reset\n");
		}
	}

	if (NULL != dbCID) {
		WAL_FREE(dbCID);
	}
	if (NULL != reboot_reason) {
		WAL_FREE(reboot_reason);
	}

	WalPrint("Returned from processFactoryResetNotification with status %d\n", status);
	return status;

}

/*
 * @brief To process notification during firmware upgrade
 */
static WDMP_STATUS processFirmwareUpgradeNotification(ParamNotify *paramNotify, unsigned int *cmc, char **cid)
{
	char *dbCID = NULL, *dbCMC = NULL;
	char newCMC[32]={'\0'};
	WDMP_STATUS status = WDMP_FAILURE;

	WalPrint("processFirmwareUpgradeNotification........\n");

        dbCMC = getParameterValue(PARAM_CMC);
        if (NULL != dbCMC) {
                dbCID = getParameterValue(PARAM_CID);
                if (NULL == dbCID) {
                WAL_FREE(dbCMC);
                WalError("Error dbCID is NULL!\n");
                return status;
                }
        } else {
                WalError("Error dbCMC is NULL!\n");
                return status;
        }


	snprintf(newCMC, sizeof(newCMC), "%d", (atoi(dbCMC) | CHANGED_BY_FIRMWARE_UPGRADE));
	WalPrint("newCMC value after firmware upgrade: %s\n", newCMC);

	if(atoi(dbCMC) != atoi(newCMC))
	{
		status = setParameterValue(PARAM_CMC, newCMC,WDMP_UINT);
		if(status == WDMP_SUCCESS)
		{
			WalInfo("Successfully Set CMC to %d\n", atoi(newCMC));
			(*cmc) = atoi(newCMC);
			(*cid) = dbCID;
		}
		else
		{
			WAL_FREE(dbCID);
			WalError("Error setting CMC value for firmware upgrade\n");
		}
	}
	else
	{
		WalInfo("newCMC %s is same as dbCMC %s, hence firmware upgrade notification was not sent\n",newCMC,dbCMC);
		WAL_FREE(dbCID);
	}

	WAL_FREE(dbCMC);
	WalPrint("Returned from processFirmwareUpgradeNotification with status %d\n", status);
	return status;

}

/*
 * @brief To process connected client notification
 */
static void processConnectedClientNotification(NodeData *connectedNotify, char *deviceId, char **version, char ** nodeMacId, char **timeStamp, char **destination)
{
	char *nodeData=NULL;
	int len = 0;
	char nodeMAC[32]={'\0'};
	struct timespec sysTime;
	char sbuf[32] = {'\0'};

	if(connectedNotify != NULL)
	{
		if(connectedNotify->status != NULL)
		{
			WalPrint("Framing status\n");
			len += strlen(connectedNotify->status);
		}

		if(connectedNotify->nodeMacId != NULL)
		{
			macToLower(connectedNotify->nodeMacId, nodeMAC);
			WalPrint("nodeMAC %s, connectedNotify->nodeMacId %s\n",nodeMAC,connectedNotify->nodeMacId);
			len += strlen(nodeMAC);
			WalPrint("Framing nodeMacId\n");
			(*nodeMacId) = malloc(sizeof(char)* (strlen(nodeMAC) + 1));
			strncpy((*nodeMacId), nodeMAC, (strlen(nodeMAC) + 1));
			WalPrint("(*nodeMacId) :%s\n",(*nodeMacId));
		}

		if(len > 0)
		{
			len += strlen("/unknown/");
			nodeData = (char *)(malloc(sizeof(char) * (len + 1)));
			//E.g. connected/unknown/112233445566
			snprintf(nodeData, len + 1, "%s/unknown/%s",((NULL != connectedNotify->status) ? connectedNotify->status : "unknown"), ((NULL != connectedNotify->nodeMacId) ? nodeMAC : "unknown"));
			WalPrint("nodeData : %s\n",nodeData);
		}
	}

	WalPrint("nodeData is : %s\n",nodeData);

	sprintf(*destination,"event:node-change/%s/%s",deviceId,((NULL != nodeData) ? nodeData : "unknown"));

	WalPrint("(*destination) : %s\n",(*destination));

	*version = getParameterValue(PARAM_HOSTS_VERSION);
	WalPrint("*version : %s\n",*version);

	*timeStamp = getParameterValue(PARAM_SYSTEM_TIME);
	if(*timeStamp == NULL)
	{
		clock_gettime(CLOCK_REALTIME, &sysTime);
		if( sysTime.tv_nsec > 999999999L)
		{
			sysTime.tv_nsec = sysTime.tv_nsec - 1000000000L;
		}

		sprintf(sbuf, "%ld.%09ld", sysTime.tv_sec, sysTime.tv_nsec);
		*timeStamp = (char *) malloc (sizeof(char) * 64);
		strcpy(*timeStamp, sbuf);
		WalPrint("*timeStamp : %s\n",*timeStamp);
	}

	WalPrint("End of processConnectedClientNotification\n");

}

/*
 * @brief To free notification structure
 */
static void freeNotifyMessage(NotifyData *notifyData)
{
	WalPrint("Inside freeNotifyMessage\n");

	if(notifyData->type == PARAM_NOTIFY)
	{
		WalPrint("Free notifyData->u.notify\n");
		WAL_FREE(notifyData->u.notify);
	}
	else if(notifyData->type == TRANS_STATUS)
	{
		WalPrint("Free notifyData->u.status\n");
		WAL_FREE(notifyData->u.status);
	}
	else if(notifyData->type == CONNECTED_CLIENT_NOTIFY)
	{
		WalPrint("Free notifyData->u.node\n");
		WAL_FREE(notifyData->u.node);
	}
	else if(notifyData->type == DEVICE_STATUS)
	{
		WalPrint("Free notifyData->u.device\n");
		WAL_FREE(notifyData->u.device);
	}

	WalPrint("Free notifyData\n");
	WAL_FREE(notifyData);

	WalPrint("free done from freeNotifyMessage\n");
}

static void mapComponentStatusToGetReason(COMPONENT_STATUS status, char *reason)
{
        if (status == SUCCESS)
	{
		walStrncpy(reason,"Success",MAX_REASON_LENGTH);
	}
	else if (status == PAM_FAILED)
	{
		walStrncpy(reason, "PAM health timeout",MAX_REASON_LENGTH);
	}
	else if (status == EPON_FAILED)
	{
		walStrncpy(reason,"EPON health timeout",MAX_REASON_LENGTH);
	}
	else if (status == CM_FAILED)
	{
		walStrncpy(reason,"CM Agent health timeout",MAX_REASON_LENGTH);
	}
	else if (status == PSM_FAILED)
	{
		walStrncpy(reason,"PSM health timeout",MAX_REASON_LENGTH);
	}
	else if (status == WIFI_FAILED)
	{
	        walStrncpy(reason, "WiFi health timeout",MAX_REASON_LENGTH);
	}
	else
	{
	        walStrncpy(reason, "Failed",MAX_REASON_LENGTH);
	}
}
