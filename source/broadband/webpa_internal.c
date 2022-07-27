/**
 * @file webpa_internal.c
 *
 * @description This file describes the Webpa Abstraction Layer
 *
 * Copyright (c) 2015  Comcast
 */

#include <pthread.h>

#include "webpa_internal.h"

#if defined(FEATURE_SUPPORT_WEBCONFIG)
#include <webcfg_log.h>
#include <webcfg.h>
#endif

#include <cJSON.h>
/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
ComponentVal ComponentValArray[RDKB_TR181_OBJECT_LEVEL1_COUNT] = {'\0'};
ComponentVal SubComponentValArray[RDKB_TR181_OBJECT_LEVEL2_COUNT] = {'\0'};
int compCacheSuccessCnt = 0, subCompCacheSuccessCnt = 0;
int compCacheFailedCnt = 0, subCompCacheFailedCnt = 0;
int cachingStatus = 0;
char *failedCompList[RDKB_TR181_OBJECT_LEVEL1_COUNT] = { '\0' };
char *failedSubCompList[RDKB_TR181_OBJECT_LEVEL2_COUNT] = { '\0' };
char *objectList[] ={
"Device.WiFi.",
"Device.DeviceInfo.",
"Device.GatewayInfo.",
"Device.Time.",
"Device.UserInterface.",
"Device.InterfaceStack.",
"Device.Ethernet.",
#if ! defined(_HUB4_PRODUCT_REQ_) && ! defined(_CBR_PRODUCT_REQ_)
"Device.MoCA.",
#endif
"Device.PPP.",
"Device.IP.",
"Device.Routing.",
"Device.DNS.",
"Device.Firewall.",
"Device.NAT.",
"Device.DHCPv4.",
"Device.DHCPv6.",
"Device.Users.",
"Device.UPnP.",
"Device.X_CISCO_COM_DDNS.",
"Device.X_CISCO_COM_Security.",
"Device.X_CISCO_COM_DeviceControl.",
"Device.Bridging.",
"Device.RouterAdvertisement.",
"Device.NeighborDiscovery.",
"Device.IPv6rd.",
"Device.X_CISCO_COM_MLD.",
#ifndef _HUB4_PRODUCT_REQ_
#if defined(_COSA_BCM_MIPS_)
"Device.DPoE.",
#else
"Device.X_CISCO_COM_CableModem.",
#endif
#endif
"Device.X_Comcast_com_ParentalControl.",
"Device.X_CISCO_COM_Diagnostics.",
"Device.X_CISCO_COM_MultiLAN.",
"Device.X_COMCAST_COM_GRE.",
"Device.X_CISCO_COM_GRE.",
"Device.Hosts.",
"Device.ManagementServer.",
"Device.XHosts.",
#ifndef _HUB4_PRODUCT_REQ_
"Device.X_CISCO_COM_MTA.",
#endif
"Device.X_RDKCENTRAL-COM_XDNS.",
"Device.X_RDKCENTRAL-COM_Report.",
"Device.SelfHeal.",
"Device.LogBackup.",
"Device.NotifyComponent.",
"Device.X_RDKCENTRAL-COM_Webpa.",
#if defined(FEATURE_SUPPORT_WEBCONFIG)
"Device.X_RDK_WebConfig.",
#endif
"Device.Webpa."
};
 
char *subObjectList[] = 
{
"Device.DeviceInfo.NetworkProperties.",
#if ! defined(_HUB4_PRODUCT_REQ_) && ! defined(_CBR_PRODUCT_REQ_)
"Device.MoCA.Interface.",
#endif
"Device.IP.Diagnostics.",
"Device.IP.Interface.",
"Device.DNS.Diagnostics.",
"Device.DNS.Client.",
"Device.DeviceInfo.VendorConfigFile.",
"Device.DeviceInfo.MemoryStatus.",
"Device.DeviceInfo.ProcessStatus.",
"Device.DeviceInfo.Webpa.",
"Device.DeviceInfo.SupportedDataModel.",
"Device.DeviceInfo.X_RDKCENTRAL-COM.",
"Device.DeviceInfo.X_RDKCENTRAL-COM_xOpsDeviceMgmt.",
"Device.X_RDKCENTRAL-COM_Report.InterfaceDevicesWifi.",
"Device.X_RDKCENTRAL-COM_Report.RadioInterfaceStatistics.",
"Device.X_RDKCENTRAL-COM_Report.NeighboringAP.",
"Device.X_RDKCENTRAL-COM_Report.NetworkDevicesStatus.",
"Device.X_RDKCENTRAL-COM_Report.NetworkDevicesTraffic.",
"Device.X_RDKCENTRAL-COM_Webpa.Server.",
"Device.X_RDKCENTRAL-COM_Webpa.TokenServer.",
"Device.X_RDKCENTRAL-COM_Webpa.DNSText."
}; 

char *CcspDmlName[WIFI_PARAM_MAP_SIZE] = {"Device.WiFi.Radio", "Device.WiFi.SSID", "Device.WiFi.AccessPoint"};
CpeWebpaIndexMap IndexMap[WIFI_INDEX_MAP_SIZE] = {
{10000, 1},
{10100, 2},
{10200, 3},
{10001, 1},
{10002, 3},
{10003, 5},
{10004, 7},
{10005, 9},
{10006, 11},
{10007, 13},
{10008, 15},
{10101, 2},
{10102, 4},
{10103, 6},
{10104, 8},
{10105, 10},
{10106, 12},
{10107, 14},
{10108, 16},
{10201, 17},
{10202, 18},
{10203, 19},
{10204, 20},
{10205, 21},
{10206, 22},
{10207, 23},
{10208, 24}
};
BOOL eth_wan_status = FALSE;

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/
static int getComponentInfoFromCache(char *parameterName, char *objectName, char *compName, char *dbusPath);
static int getMatchingComponentValArrayIndex(char *objectName);
static int getMatchingSubComponentValArrayIndex(char *objectName);
static void getObjectName(char *str, char *objectName, int objectLevel);
static int waitForComponentReady(char *compName, char *dbusPath);
static void checkComponentReady(char *compName, char *dbusPath);
static void checkComponentHealthStatus(char * compName, char * dbusPath, char *status, int *retStatus);
static void waitUntilSystemReady();
static void ccspSystemReadySignalCB(void* user_data);
static int checkIfSystemReady();
extern ANSC_HANDLE bus_handle;
extern char        g_Subsystem[32];
static void *WALInit(void *status);
static void retryFailedComponentCaching();

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/
void initComponentCaching(int status)
{
        int err = 0;
	pthread_t threadId;

	err = pthread_create(&threadId, NULL, WALInit, (void *) status);
	if (err != 0)
	{
		WalError("Error creating WALInit thread :[%s]\n", strerror(err));
	}
	else
	{
		WalPrint("WALInit Thread created Successfully\n");
	}
}

int waitForOperationalReadyCondition()
{
	// Wait till PAM, CM, PSM, WiFi components are ready on the stack.
	if(waitForComponentReady(RDKB_PAM_COMPONENT_NAME,RDKB_PAM_DBUS_PATH) != CCSP_SUCCESS)
	{
		return PAM_FAILED;
	}
#if defined(_ENABLE_EPON_SUPPORT_)
	if(waitForComponentReady(RDKB_EPON_COMPONENT_NAME,RDKB_EPON_DBUS_PATH) != CCSP_SUCCESS)
	{
		return EPON_FAILED;
	}
#elif !defined(PLATFORM_RASPBERRYPI) && !defined(RDKB_EMU)
    if(check_ethernet_wan_status() != WDMP_SUCCESS)
	{
#if !defined(_SKY_HUB_COMMON_PRODUCT_REQ_)
	    if(waitForComponentReady(RDKB_CM_COMPONENT_NAME,RDKB_CM_DBUS_PATH) != CCSP_SUCCESS)
	    {
		    return CM_FAILED;
	    }
#endif // _SKY_HUB_COMMON_PRODUCT_REQ_
	}
#endif
	if(waitForComponentReady(CCSP_DBUS_PSM,CCSP_DBUS_PATH_PSM) != CCSP_SUCCESS)
	{
		return PSM_FAILED;
	}
	if(waitForComponentReady(RDKB_WIFI_COMPONENT_NAME,RDKB_WIFI_DBUS_PATH) != CCSP_SUCCESS)
	{
		return WIFI_FAILED;
	}
	return 0;
}

void getCurrentTime(struct timespec *timer)
{
	clock_gettime(CLOCK_REALTIME, timer);
}

uint64_t getCurrentTimeInMicroSeconds(struct timespec *timer)
{
        uint64_t systime = 0;
	clock_gettime(CLOCK_REALTIME, timer);       
        WalPrint("timer->tv_sec : %lu\n",timer->tv_sec);
        WalPrint("timer->tv_nsec : %lu\n",timer->tv_nsec);
        systime = (uint64_t)timer->tv_sec * 1000000L + timer->tv_nsec/ 1000;
        return systime;	
}

long timeValDiff(struct timespec *starttime, struct timespec *finishtime)
{
	long msec;
	msec=(finishtime->tv_sec-starttime->tv_sec)*1000;
	msec+=(finishtime->tv_nsec-starttime->tv_nsec)/1000000;
	return msec;
}

void walStrncpy(char *destStr, const char *srcStr, size_t destSize)
{
    strncpy(destStr, srcStr, destSize-1);
    destStr[destSize-1] = '\0';
}

#ifdef FEATURE_SUPPORT_WEBCONFIG
int operationalStatus = 0;
void set_global_operationalStatus(int status)
{
    operationalStatus = status;
}

int get_global_operationalStatus(void)
{
    return operationalStatus;
}
#endif
static void *WALInit(void *status)
{
	char dst_pathname_cr[MAX_PATHNAME_CR_LEN] = { 0 };
	char l_Subsystem[MAX_DBUS_INTERFACE_LEN] = { 0 };
	int ret = 0, i = 0, size = 0, len = 0, cnt = 0, cnt1 = 0, count = 0, count1 = 0;
	char paramName[MAX_PARAMETERNAME_LEN] = { 0 };
	componentStruct_t ** ppComponents = NULL;
	cachingStatus = 0;

	WalPrint("------------ WALInit ----------\n");
	pthread_detach(pthread_self());
	waitUntilSystemReady();
	
#ifdef FEATURE_SUPPORT_WEBCONFIG
	//Function to start webConfig operation after system ready.
	WebcfgInfo("FEATURE_SUPPORT_WEBCONFIG is enabled, device status %d\n", (int)status);
	set_global_operationalStatus(status);
	char RfcEnable[64];
	memset(RfcEnable, 0, sizeof(RfcEnable));
#ifdef RDKB_BUILD
	char* strValue = NULL;
	if (CCSP_SUCCESS == PSM_Get_Record_Value2(bus_handle, g_Subsystem, "eRT.com.cisco.spvtg.ccsp.webpa.WebConfigRfcEnable", NULL, &strValue))
	{
		WebcfgDebug("strValue %s \n", strValue);
		if(strValue != NULL)
		{
			walStrncpy(RfcEnable, strValue, sizeof(RfcEnable));
			((CCSP_MESSAGE_BUS_INFO *)bus_handle)->freefunc( strValue );
		}
	}
#endif
	if(RfcEnable[0] != '\0' && strncmp(RfcEnable, "true", strlen("true")) == 0)
	{
	    if(get_global_mpThreadId() == NULL) 
	    {
	    	WebcfgInfo("WebConfig Rfc is enabled, starting WebConfigMultipartTask\n");
	    	initWebConfigMultipartTask((unsigned long) status);
 	    }
	    else
	    {
		WebcfgInfo("Webconfig is already started, so not starting after systemready\n");
	    }
	}
	else
	{
		WebcfgError("WebConfig Rfc Flag is not enabled\n");
	}
#endif
#if !defined(RDKB_EMU)
	strncpy(l_Subsystem, "eRT.",sizeof(l_Subsystem));
#endif
	snprintf(dst_pathname_cr, sizeof(dst_pathname_cr),"%s%s", l_Subsystem, CCSP_DBUS_INTERFACE_CR);

	WalPrint("-------- Start of populateComponentValArray -------\n");
	len = sizeof(objectList)/sizeof(objectList[0]);
	WalPrint("Length of object list : %d\n",len);
	for(i = 0; i < len ; i++)
	{
	    if(strncmp(objectList[i], "Device.X_CISCO_COM_CableModem.", strlen("Device.X_CISCO_COM_CableModem.")) == 0 && get_eth_wan_status() == TRUE )
	    {
	        WalInfo("Skipped caching of CM Agent parameter\n");
	    }
		else
		{
		    walStrncpy(paramName,objectList[i],sizeof(paramName));
		    ret = CcspBaseIf_discComponentSupportingNamespace(bus_handle,dst_pathname_cr, paramName,l_Subsystem, &ppComponents, &size);
			
		    if (ret == CCSP_SUCCESS)
		    {	    
			    WalPrint("WALInit(): %s Component caching is successful\n",objectList[i]);
			    // Allocate memory for ComponentVal obj_name, comp_name, dbus_path
			    ComponentValArray[cnt].obj_name = (char *)malloc(sizeof(char) * (MAX_PARAMETERNAME_LEN/2));
			    memset(ComponentValArray[cnt].obj_name, 0, sizeof(char) * (MAX_PARAMETERNAME_LEN/2));
			    ComponentValArray[cnt].comp_name = (char *)malloc(sizeof(char) * (MAX_PARAMETERNAME_LEN/2));
			    ComponentValArray[cnt].dbus_path = (char *)malloc(sizeof(char) * (MAX_PARAMETERNAME_LEN/2));

			    ComponentValArray[cnt].comp_id = cnt;
			    ComponentValArray[cnt].comp_size = size;
			    getObjectName(paramName,ComponentValArray[cnt].obj_name,1);
			    strncpy(ComponentValArray[cnt].comp_name,ppComponents[0]->componentName,MAX_PARAMETERNAME_LEN/2);
			    strncpy(ComponentValArray[cnt].dbus_path,ppComponents[0]->dbusPath,MAX_PARAMETERNAME_LEN/2);
					       
			    WalInfo("ComponentValArray[%d].comp_id = %d,ComponentValArray[cnt].comp_size = %d, ComponentValArray[%d].obj_name = %s, ComponentValArray[%d].comp_name = %s, ComponentValArray[%d].dbus_path = %s\n", cnt, ComponentValArray[cnt].comp_id,ComponentValArray[cnt].comp_size, cnt, ComponentValArray[cnt].obj_name, cnt, ComponentValArray[cnt].comp_name, cnt, ComponentValArray[cnt].dbus_path);  
				    cnt++;
		    }
		    else
		    {
			    WalPrint("Component caching is failed for %s\n",paramName);
			    WalError("---Failed to get component info for object %s---: ret = %d, size = %d, Adding into failedCompList....\n", objectList[i], ret, size);
			    failedCompList[count] = objectList[i];
			    WalInfo("failedCompList[%d] : %s\n", count, failedCompList[count]);
			    count++;
		    }
		    free_componentStruct_t(bus_handle, size, ppComponents);
		}
	}
	
	compCacheSuccessCnt = cnt;
	compCacheFailedCnt = count;
	WalPrint("compCacheSuccessCnt : %d\n", compCacheSuccessCnt);
	WalPrint("compCacheFailedCnt : %d\n", compCacheFailedCnt);
	
	WalPrint("Initializing sub component list\n");
	len = sizeof(subObjectList)/sizeof(subObjectList[0]);
	WalPrint("Length of sub object list : %d\n",len);

	for(i = 0; i < len; i++)
	{
		walStrncpy(paramName,subObjectList[i],sizeof(paramName));
		ret = CcspBaseIf_discComponentSupportingNamespace(bus_handle,
					dst_pathname_cr, paramName, l_Subsystem, &ppComponents, &size);
			
		if (ret == CCSP_SUCCESS)
		{	  
			WalPrint("WALInit(): %s Component caching is successful\n",subObjectList[i]);     
			// Allocate memory for ComponentVal obj_name, comp_name, dbus_path
			SubComponentValArray[cnt1].obj_name = (char *)malloc(sizeof(char) * (MAX_PARAMETERNAME_LEN/2));
			memset(SubComponentValArray[cnt1].obj_name, 0, sizeof(char) * (MAX_PARAMETERNAME_LEN/2));
			SubComponentValArray[cnt1].comp_name = (char *)malloc(sizeof(char) * (MAX_PARAMETERNAME_LEN/2));
			SubComponentValArray[cnt1].dbus_path = (char *)malloc(sizeof(char) * (MAX_PARAMETERNAME_LEN/2));

			SubComponentValArray[cnt1].comp_id = cnt1;
			SubComponentValArray[cnt1].comp_size = size;
			getObjectName(paramName,SubComponentValArray[cnt1].obj_name,2);
			WalPrint("in WALInit() SubComponentValArray[cnt].obj_name is %s",SubComponentValArray[cnt1].obj_name);
			strncpy(SubComponentValArray[cnt1].comp_name,ppComponents[0]->componentName,MAX_PARAMETERNAME_LEN/2);
			strncpy(SubComponentValArray[cnt1].dbus_path,ppComponents[0]->dbusPath,MAX_PARAMETERNAME_LEN/2);
					   
			WalInfo("SubComponentValArray[%d].comp_id = %d,SubComponentValArray[i].comp_size = %d, SubComponentValArray[%d].obj_name = %s, SubComponentValArray[%d].comp_name = %s, SubComponentValArray[%d].dbus_path = %s\n", cnt1, SubComponentValArray[cnt1].comp_id,SubComponentValArray[cnt1].comp_size, cnt1, SubComponentValArray[cnt1].obj_name, cnt1, SubComponentValArray[cnt1].comp_name, cnt1, SubComponentValArray[cnt1].dbus_path);  
				cnt1++;
		}
		else
		{
			WalPrint("SubComponent Caching is failed for %s\n",paramName);
			WalError("---Failed to get component info for object %s---: ret = %d, size = %d, Adding into failedSubCompList....\n", subObjectList[i], ret, size);
			failedSubCompList[count1] = subObjectList[i];
			WalInfo("failedSubCompList[%d] : %s\n", count1, failedSubCompList[count1]);
			count1++;
		}
		free_componentStruct_t(bus_handle, size, ppComponents);
	}

	subCompCacheSuccessCnt = cnt1;
	subCompCacheFailedCnt = count1;
	WalPrint("subCompCacheSuccessCnt : %d\n", subCompCacheSuccessCnt);
	WalPrint("subCompCacheFailedCnt : %d\n", subCompCacheFailedCnt);
	retryFailedComponentCaching();
	WalInfo("Component caching is completed. Hence setting cachingStatus to active\n");
	cachingStatus = 1;
	WalPrint("-------- End of populateComponentValArray -------\n");
	return NULL;
}

int getComponentDetails(char *parameterName,char ***compName,char ***dbusPath, int *error, int *retCount)
{
	char objectName[MAX_PARAMETERNAME_LEN] = {'\0'};
	int index = -1,retIndex = 0,ret= -1, size = 0, i = 0;
	char dst_pathname_cr[MAX_PATHNAME_CR_LEN] = { 0 };
	char l_Subsystem[MAX_DBUS_INTERFACE_LEN] = { 0 };
	char **localCompName = NULL;
	char **localDbusPath = NULL;
	char tempParamName[MAX_PARAMETERNAME_LEN] = {'\0'};
	char tempCompName[MAX_PARAMETERNAME_LEN/2] = {'\0'};
	char tempDbusPath[MAX_PARAMETERNAME_LEN/2] = {'\0'};
	
	componentStruct_t ** ppComponents = NULL;
#if !defined(RDKB_EMU)
	strncpy(l_Subsystem, "eRT.",sizeof(l_Subsystem));
#endif
	snprintf(dst_pathname_cr, sizeof(dst_pathname_cr),"%s%s", l_Subsystem, CCSP_DBUS_INTERFACE_CR);
	walStrncpy(tempParamName, parameterName,sizeof(tempParamName));
	WalPrint("======= start of getComponentDetails ========\n");
	if(cachingStatus == 1)
	{
	        WalPrint("Component caching is ready, fetch component details from cache\n");
	        index = getComponentInfoFromCache(tempParamName, objectName, tempCompName, tempDbusPath);
        }
        else
        {
                WalPrint("Component caching is not yet ready, fetch component details from stack\n");
                index = -1;
        }
	WalPrint("index : %d\n",index);
	// Cannot identify the component from cache, make DBUS call to fetch component
	if(index == -1 || ComponentValArray[index].comp_size > 2 || SubComponentValArray[index].comp_size >= 2) //comp size anything > 2 and sub comp size >1 . TCCBR-5475 allows dbus calls when sub comp size>1.
	{
		WalPrint("in if for size >2\n");
		// GET Component for parameter from stack
		if(index > 0 && ComponentValArray[index].comp_size > 2)
		{
		        WalPrint("ComponentValArray[index].comp_size : %d\n",ComponentValArray[index].comp_size);
		}
		else if(index > 0 && SubComponentValArray[index].comp_size >= 2)
		{
		        WalPrint("SubComponentValArray[index].comp_size : %d\n",SubComponentValArray[index].comp_size);
		}
		retIndex = IndexMpa_WEBPAtoCPE(tempParamName);
		if(retIndex == -1)
		{
			if(strstr(tempParamName, PARAM_RADIO_OBJECT) != NULL)
		 	{
		 	       ret = CCSP_ERR_INVALID_RADIO_INDEX;
		 	       WalError("%s has invalid Radio index, Valid indexes are 10000, 10100 and 10200. ret = %d\n", tempParamName,ret);
		 	       OnboardLog("%s has invalid Radio index, Valid indexes are 10000, 10100 and 10200. ret = %d\n", tempParamName,ret);
		 	}
		 	else
		 	{
		         	ret = CCSP_ERR_INVALID_WIFI_INDEX;
		         	WalError("%s has invalid WiFi index, Valid range is between 10001-10008, 10101-10108 and 10201-10208. ret = %d\n",tempParamName, ret);
		         	OnboardLog("%s has invalid WiFi index, Valid range is between 10001-10008, 10101-10108 and 10201-10208. ret = %d\n",tempParamName, ret);
		 	}					
            		*error = 1;
			return ret;
		}
		WalPrint("Get component for parameterName : %s from stack\n",tempParamName);

		ret = CcspBaseIf_discComponentSupportingNamespace(bus_handle,
			dst_pathname_cr, tempParamName, l_Subsystem, &ppComponents, &size);
		WalPrint("size : %d, ret : %d\n",size,ret);

		if (ret == CCSP_SUCCESS)
		{
			localCompName = (char **) malloc (sizeof(char*) * size);
			localDbusPath = (char **) malloc (sizeof(char*) * size);
			for(i = 0; i < size; i++)
			{
				localCompName[i] = (char *) malloc (sizeof(char) * MAX_PARAMETERNAME_LEN);
				localDbusPath[i] = (char *) malloc (sizeof(char) * MAX_PARAMETERNAME_LEN);
				strcpy(localCompName[i],ppComponents[i]->componentName);
				strcpy(localDbusPath[i],ppComponents[i]->dbusPath);
				WalPrint("localCompName[%d] : %s, localDbusPath[%d] : %s\n",i,localCompName[i],i, localDbusPath[i]);
			}
			
			*retCount = size;
			free_componentStruct_t(bus_handle, size, ppComponents);
		}
		else
		{
			WalError("Parameter name %s is not supported. ret = %d\n", tempParamName, ret);
			OnboardLog("Parameter name %s is not supported. ret = %d\n", tempParamName, ret);
			free_componentStruct_t(bus_handle, size, ppComponents);
			*error = 1;
			return ret;
		}
	}
	else
	{
		localCompName = (char **) malloc (sizeof(char*)*1);
		localDbusPath = (char **) malloc (sizeof(char*)*1);
		localCompName[0] = (char *) malloc (sizeof(char) * MAX_PARAMETERNAME_LEN);
		localDbusPath[0] = (char *) malloc (sizeof(char) * MAX_PARAMETERNAME_LEN);
		strcpy(localCompName[0],tempCompName);
		strcpy(localDbusPath[0],tempDbusPath);
		*retCount = 1;
		size = 1;
		WalPrint("localCompName[0] : %s, localDbusPath[0] : %s\n",localCompName[0], localDbusPath[0]);
	}
	
	*compName = localCompName;
	*dbusPath = localDbusPath;
	for(i = 0; i < size; i++)
	{
		WalPrint("(*compName)[%d] : %s, (*dbusPath)[%d] : %s\n",i,(*compName)[i],i, (*dbusPath)[i]);
	}
	WalPrint("======= End of getComponentDetails ret =%d ========\n",ret);
	return CCSP_SUCCESS;
}

void prepareParamGroups(ParamCompList **ParamGroup,int paramCount,int cnt1,char *paramName,char *compName,char *dbusPath, int * compCount )
{
        int cnt2 =0, subParamCount =0,matchFlag = 0, tempCount=0;
        tempCount =*compCount;
        ParamCompList *localParamGroup = *ParamGroup;
        WalPrint("============ start of prepareParamGroups ===========\n");
        if(*ParamGroup == NULL)
        {
                WalPrint("ParamCompList is null initializing\n");
                localParamGroup = (ParamCompList *) malloc(sizeof(ParamCompList));
           	localParamGroup[0].parameterCount = 1;
           	localParamGroup[0].comp_name = (char *) malloc(MAX_PARAMETERNAME_LEN/2);
                strncpy(localParamGroup[0].comp_name, compName,MAX_PARAMETERNAME_LEN/2);
                WalPrint("localParamGroup[0].comp_name : %s\n",localParamGroup[0].comp_name);
           	localParamGroup[0].dbus_path = (char *) malloc(MAX_PARAMETERNAME_LEN/2);
                strncpy(localParamGroup[0].dbus_path, dbusPath,MAX_PARAMETERNAME_LEN/2);
                WalPrint("localParamGroup[0].dbus_path :%s\n",localParamGroup[0].dbus_path);
                // max number of parameter will be equal to the remaining parameters to be iterated (i.e. paramCount - cnt1)
                localParamGroup[0].parameterName = (char **) malloc(sizeof(char *) * (paramCount - cnt1));
           	localParamGroup[0].parameterName[0] = (char *) malloc(MAX_PARAMETERNAME_LEN);
           	strncpy(localParamGroup[0].parameterName[0],paramName,MAX_PARAMETERNAME_LEN);
                WalPrint("localParamGroup[0].parameterName[0] : %s\n",localParamGroup[0].parameterName[0]);
           	tempCount++;
        }
        else
        {
           	WalPrint("ParamCompList exists checking if parameter belongs to existing group\n");		
                WalPrint("compName %s\n",compName);
                for(cnt2 = 0; cnt2 < tempCount; cnt2++)
                {
                       WalPrint("localParamGroup[cnt2].comp_name %s \n",localParamGroup[cnt2].comp_name);
	                if(!strcmp(localParamGroup[cnt2].comp_name,compName))
	                {
		                WalPrint("Match found to already existing component group in ParamCompList, adding parameter to it\n");
		                localParamGroup[cnt2].parameterCount = localParamGroup[cnt2].parameterCount + 1;
		                subParamCount =  localParamGroup[cnt2].parameterCount;
		                WalPrint("subParamCount :%d\n",subParamCount);

		                localParamGroup[cnt2].parameterName[subParamCount-1] = (char *) malloc(MAX_PARAMETERNAME_LEN);

		                strncpy(localParamGroup[cnt2].parameterName[subParamCount-1],paramName,MAX_PARAMETERNAME_LEN);
		                WalPrint("localParamGroup[%d].parameterName :%s\n",cnt2,localParamGroup[cnt2].parameterName[subParamCount-1]);

		                matchFlag=1;
		                break;
	                }
                }
            	if(matchFlag != 1)
            	{
	                WalPrint("Parameter does not belong to existing component group, creating new group \n");

                      	localParamGroup =  (ParamCompList *) realloc(localParamGroup,sizeof(ParamCompList) * (tempCount + 1));
                      	localParamGroup[tempCount].parameterCount = 1;
                      	localParamGroup[tempCount].comp_name = (char *) malloc(MAX_PARAMETERNAME_LEN/2);
	                strncpy(localParamGroup[tempCount].comp_name, compName,MAX_PARAMETERNAME_LEN/2);
                      	localParamGroup[tempCount].dbus_path = (char *) malloc(MAX_PARAMETERNAME_LEN/2);
	                strncpy(localParamGroup[tempCount].dbus_path, dbusPath,MAX_PARAMETERNAME_LEN/2);

	                // max number of parameter will be equal to the remaining parameters to be iterated (i.e. paramCount - cnt1)
                      	localParamGroup[tempCount].parameterName = (char **) malloc(sizeof(char *) * (paramCount - cnt1));
	                localParamGroup[tempCount].parameterName[0] = (char *) malloc(MAX_PARAMETERNAME_LEN);
                      	strncpy(localParamGroup[tempCount].parameterName[0],paramName,MAX_PARAMETERNAME_LEN);

                       	WalPrint("localParamGroup[%d].comp_name :%s\n",tempCount,localParamGroup[tempCount].comp_name);
                      	WalPrint("localParamGroup[%d].parameterName :%s\n",tempCount,localParamGroup[tempCount].parameterName[0]);
	                tempCount++;
            	}
        }
        *compCount = tempCount;
        *ParamGroup = localParamGroup; 
        WalPrint("============ End of prepareParamGroups compCount =%d===========\n",*compCount);
 	
 }
 

void free_ParamCompList(ParamCompList *ParamGroup, int compCount)
{
	int cnt1 = 0, cnt2 = 0;
	for(cnt1 = 0; cnt1 < compCount; cnt1++)
	{
	  	for(cnt2 = 0; cnt2 < ParamGroup[cnt1].parameterCount; cnt2++)
	  	{
	     	        WAL_FREE(ParamGroup[cnt1].parameterName[cnt2]);
	  	}
		WAL_FREE(ParamGroup[cnt1].parameterName);
		WAL_FREE(ParamGroup[cnt1].comp_name);
		WAL_FREE(ParamGroup[cnt1].dbus_path);
	}
	WAL_FREE(ParamGroup);
}

void free_componentDetails(char **compName,char **dbusPath,int size)
{
	int i;
	for(i = 0; i < size; i++)
	{
		WAL_FREE(compName[i]);
		WAL_FREE(dbusPath[i]);	
	}

	WAL_FREE(compName);
	WAL_FREE(dbusPath);
}

WDMP_STATUS mapStatus(int ret)
{
	switch (ret) 
	{
		case CCSP_SUCCESS:
			return WDMP_SUCCESS;
		case CCSP_FAILURE:
			return WDMP_FAILURE;
		case CCSP_ERR_TIMEOUT:
			return WDMP_ERR_TIMEOUT;
		case CCSP_ERR_NOT_EXIST:
			return WDMP_ERR_NOT_EXIST;
		case CCSP_ERR_INVALID_PARAMETER_NAME:
			return WDMP_ERR_INVALID_PARAMETER_NAME;
		case CCSP_ERR_INVALID_PARAMETER_TYPE:
			return WDMP_ERR_INVALID_PARAMETER_TYPE;
		case CCSP_ERR_INVALID_PARAMETER_VALUE:
			return WDMP_ERR_INVALID_PARAMETER_VALUE;
		case CCSP_ERR_NOT_WRITABLE:
			return WDMP_ERR_NOT_WRITABLE;
		case CCSP_ERR_SETATTRIBUTE_REJECTED:
			return WDMP_ERR_SETATTRIBUTE_REJECTED;
		case CCSP_ERR_REQUEST_REJECTED:
                        return WDMP_ERR_REQUEST_REJECTED;
		case CCSP_CR_ERR_NAMESPACE_OVERLAP:
			return WDMP_ERR_NAMESPACE_OVERLAP;
		case CCSP_CR_ERR_UNKNOWN_COMPONENT:
			return WDMP_ERR_UNKNOWN_COMPONENT;
		case CCSP_CR_ERR_NAMESPACE_MISMATCH:
			return WDMP_ERR_NAMESPACE_MISMATCH;
		case CCSP_CR_ERR_UNSUPPORTED_NAMESPACE:
			return WDMP_ERR_UNSUPPORTED_NAMESPACE;
		case CCSP_CR_ERR_DP_COMPONENT_VERSION_MISMATCH:
			return WDMP_ERR_DP_COMPONENT_VERSION_MISMATCH;
		case CCSP_CR_ERR_INVALID_PARAM:
			return WDMP_ERR_INVALID_PARAM;
		case CCSP_CR_ERR_UNSUPPORTED_DATATYPE:
			return WDMP_ERR_UNSUPPORTED_DATATYPE;
		case CCSP_ERR_WIFI_BUSY:
			return WDMP_ERR_WIFI_BUSY;
		case CCSP_ERR_INVALID_WIFI_INDEX:
			return WDMP_ERR_INVALID_WIFI_INDEX;
		case CCSP_ERR_INVALID_RADIO_INDEX:
			return WDMP_ERR_INVALID_RADIO_INDEX;
		case CCSP_ERR_METHOD_NOT_SUPPORTED:
		    return WDMP_ERR_METHOD_NOT_SUPPORTED;
		case CCSP_CR_ERR_SESSION_IN_PROGRESS:
		    return WDMP_ERR_SESSION_IN_PROGRESS;
		default:
			return WDMP_FAILURE;
	}
}

int IndexMpa_WEBPAtoCPE(char *pParameterName)
{
	int i = 0, j = 0, dmlNameLen = 0, instNum = 0, len = 0, matchFlag = -1;
	char pDmIntString[WIFI_MAX_STRING_LEN];
	char* instNumStart = NULL;
	char restDmlString[WIFI_MAX_STRING_LEN] = {'\0'};
	for (i = 0; i < WIFI_PARAM_MAP_SIZE; i++)
	{
		dmlNameLen = strlen(CcspDmlName[i]);
		if (strncmp(pParameterName, CcspDmlName[i], dmlNameLen) == 0)
		{
			instNumStart = pParameterName + dmlNameLen;
			//To match complete wildcard, including . 
			if (strlen(pParameterName) <= dmlNameLen + 1)
			{
				// Found match on table, but there is no instance number
				break;
			}
			else
			{
				if (instNumStart[0] == '.')
				{
					instNumStart++;
				}
				else
				{ 
				  WalPrint("No matching index as instNumStart[0] : %c\n",instNumStart[0]);
				  break;
				}
				sscanf(instNumStart, "%d%s", &instNum, restDmlString);
				WalPrint("instNum : %d restDmlString : %s\n",instNum,restDmlString);

				// Find instance match and translate
				if (i == 0)
				{
					// For Device.WiFI.Radio.
					j = 0;
					len=3;
				}
				else
				{
					// For other than Device.WiFI.Radio.
					j = 3;
					len =WIFI_INDEX_MAP_SIZE;
				}
				for (; j < len; j++)
				{
					if (IndexMap[j].WebPaInstanceNumber == instNum)
					{
						snprintf(pDmIntString, sizeof(pDmIntString),"%s.%d%s", CcspDmlName[i], IndexMap[j].CcspInstanceNumber, restDmlString);
						strcpy(pParameterName, pDmIntString);
						matchFlag = 1;
						break;
					}
				}
				WalPrint("matchFlag %d\n",matchFlag);
				if(matchFlag == -1)
				{
					WalInfo("Invalid index for : %s\n",pParameterName);
					return matchFlag;
				}
			}
			break;
		}
	}
	return 0;
}

void IndexMpa_CPEtoWEBPA(char **ppParameterName)
{
	int i = 0, j = 0, dmlNameLen = 0, instNum =0;
	char *pDmIntString = NULL;
	char* instNumStart = NULL;
	char restDmlString[WIFI_MAX_STRING_LEN]= {'\0'};
	char *pParameterName = *ppParameterName;

	for (i = 0; i < WIFI_PARAM_MAP_SIZE; i++) 
	{
		dmlNameLen = strlen(CcspDmlName[i]);
		if (strncmp(pParameterName, CcspDmlName[i], dmlNameLen) == 0)
		{
			instNumStart = pParameterName + dmlNameLen;
			if (strlen(pParameterName) < dmlNameLen + 1)
			{
				// Found match on table, but there is no instance number
				break;
			}
			else
			{
				if (instNumStart[0] == '.')
				{
					instNumStart++;
				}
				sscanf(instNumStart, "%d%s", &instNum, restDmlString);
				// Find instance match and translate
				if (i == 0)
				{
					// For Device.WiFI.Radio.
					j = 0;
				} else {
					// For other than Device.WiFI.Radio.
					j = 3;
				}
				for (j; j < WIFI_INDEX_MAP_SIZE; j++)
				{
					if (IndexMap[j].CcspInstanceNumber == instNum)
					{
						pDmIntString = (char *) malloc(
								sizeof(char) * (dmlNameLen + MAX_PARAMETERNAME_LEN));
						if (pDmIntString)
						{
							snprintf(pDmIntString, dmlNameLen + MAX_PARAMETERNAME_LEN ,"%s.%d%s", CcspDmlName[i],
									IndexMap[j].WebPaInstanceNumber,
									restDmlString);
							WAL_FREE(pParameterName);
							WalPrint("pDmIntString : %s\n",pDmIntString);
							*ppParameterName = pDmIntString;
							return;
						}

						break;
					}
				}
			}
			break;
		}
	}
	return;
}

char * getParameterValue(char *paramName)
{
	int paramCount=0;
	WDMP_STATUS ret = WDMP_FAILURE;
	int count=0;
	const char *getParamList[1];
	getParamList[0] = paramName;

	char *paramValue = (char *) malloc(sizeof(char)*64);
	paramCount = sizeof(getParamList)/sizeof(getParamList[0]);
	param_t **parametervalArr = (param_t **) malloc(sizeof(param_t *) * paramCount);
	
	getValues(getParamList, paramCount, 0, NULL,&parametervalArr, &count, &ret);
	
	if (ret == WDMP_SUCCESS )
	{
		walStrncpy(paramValue, parametervalArr[0]->value,64);
		WAL_FREE(parametervalArr[0]->name);
		WAL_FREE(parametervalArr[0]->value);
		WAL_FREE(parametervalArr[0]);
	}
	else
	{
		WalError("Failed to GetValue for %s\n", getParamList[0]);
		OnboardLog("Failed to GetValue for %s\n", getParamList[0]);
		WAL_FREE(paramValue);
	}
	
	WAL_FREE(parametervalArr);

	return paramValue;
}

WDMP_STATUS setParameterValue(char *paramName, char* value, DATA_TYPE type)
{
	char paramValue[64]={'\0'};
	int paramCount = 1;
	WDMP_STATUS ret = WDMP_FAILURE;
	param_t *parametervalArr = (param_t *) malloc(sizeof(param_t) * paramCount);
	bool error = false;
	int ccspStatus = 0;

	parametervalArr[0].name = paramName;
	parametervalArr[0].type = type;

	walStrncpy(paramValue, value,sizeof(paramValue));
	parametervalArr[0].value = paramValue;

	setValues(parametervalArr, paramCount, WEBPA_SET, NULL, NULL, &ret, &ccspStatus);

	if (ret == WDMP_SUCCESS)
	{
		WalPrint("Successfully SetValue for %s\n", parametervalArr[0].name);
	}
	else
	{
		WalError("Failed to SetValue for %s\n", parametervalArr[0].name);
		OnboardLog("Failed to SetValue for %s\n", parametervalArr[0].name);
	}

	WAL_FREE(parametervalArr);

	return ret;
}

void macToLower(char macValue[],char macConverted[])
{
	int i = 0;
	int j;
	char *token[32];
	char tmp[32];
	walStrncpy(tmp, macValue,sizeof(tmp));
	token[i] = strtok(tmp, ":");
	if(token[i]!=NULL)
	{
	    strncpy(macConverted, token[i],31);
	    macConverted[31]='\0';
	    i++;
	}
	while ((token[i] = strtok(NULL, ":")) != NULL) 
	{
	    strncat(macConverted, token[i],31);
	    macConverted[31]='\0';
	    i++;
	}
	macConverted[31]='\0';
	for(j = 0; macConverted[j]; j++)
	{
	    macConverted[j] = tolower(macConverted[j]);
	}
}

BOOL get_eth_wan_status()
{
    return eth_wan_status;
}
/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/

/**
 * @brief getMatchingComponentValArrayIndex Compare objectName with the pre-populated ComponentValArray and return matching index
 *
 * param[in] objectName 
 * @return matching ComponentValArray index
 */
static int getMatchingComponentValArrayIndex(char *objectName)
{
	int i =0,index = -1;

	for(i = 0; i < compCacheSuccessCnt ; i++)
	{
		if(ComponentValArray[i].obj_name != NULL && !strcmp(objectName,ComponentValArray[i].obj_name))
		{
	      		index = ComponentValArray[i].comp_id;
			WalPrint("Matching Component Val Array index for object %s : %d\n",objectName, index);
			break;
		}	    
	}
	return index;
}


/**
 * @brief getMatchingSubComponentValArrayIndex Compare objectName with the pre-populated SubComponentValArray and return matching index
 *
 * param[in] objectName 
 * @return matching ComponentValArray index
 */
static int getMatchingSubComponentValArrayIndex(char *objectName)
{
	int i =0,index = -1;
	
	for(i = 0; i < subCompCacheSuccessCnt ; i++)
	{
		if(SubComponentValArray[i].obj_name != NULL && !strcmp(objectName,SubComponentValArray[i].obj_name))
		{
		      	index = SubComponentValArray[i].comp_id;
			WalPrint("Matching Sub-Component Val Array index for object %s : %d\n",objectName, index);
			break;
		}	    
	}	
	return index;
}
static int getObjectLevelFromParameter(char *parameterName)
{
	int count = 0, i = 0;
	while(parameterName[i] != '\0')
	{
		if(parameterName[i] == '.')
		{
			count++;
			if(count > 2)
			{
				return 2;
			}
		}
		i++;
	}
	return count-1;
}
/**
 * @brief getComponentInfoFromCache Returns the component information from cache
 *
 * @param[in] parameterName parameter Name
 * @param[in] objectName object Name
 * @param[out] compName component name array
 * @param[out] dbusPath dbuspath array
 */
static int getComponentInfoFromCache(char *parameterName, char *objectName, char *compName, char *dbusPath)
{   
	int index = -1, level = 0;

	if(NULL == objectName)
		return index; 	
	level = getObjectLevelFromParameter(parameterName);
	if(level > 1)
	{
		getObjectName(parameterName, objectName, 2);
		index = getMatchingSubComponentValArrayIndex(objectName);
		WalPrint("objectLevel: 2, parameterName: %s, objectName: %s, matching index=%d\n",parameterName,objectName,index);
	
		if(index != -1 )
		{
			strcpy(compName,SubComponentValArray[index].comp_name);
			strcpy(dbusPath,SubComponentValArray[index].dbus_path); 
		}
	}
	if(index < 0)
	{
		getObjectName(parameterName, objectName, 1);
		index = getMatchingComponentValArrayIndex(objectName);
		WalPrint("objectLevel: 1, parameterName: %s, objectName: %s, matching index: %d\n",parameterName,objectName,index);
		 
		if((index != -1) && (ComponentValArray[index].comp_size == 1))
		{
			strcpy(compName,ComponentValArray[index].comp_name);
			strcpy(dbusPath,ComponentValArray[index].dbus_path);
		}
		else
		{
			index = -1;
		}
	}
/*
	index = getMatchingComponentValArrayIndex(objectName);
	WalPrint("objectLevel: 1, parameterName: %s, objectName: %s, matching index: %d\n",parameterName,objectName,index);
	 
	if((index != -1) && (ComponentValArray[index].comp_size == 1))
	{
		strcpy(compName,ComponentValArray[index].comp_name);
		strcpy(dbusPath,ComponentValArray[index].dbus_path);
	}
	else if((index != -1) && (ComponentValArray[index].comp_size == 2))
	{
		getObjectName(parameterName, objectName, 2);
		index = getMatchingSubComponentValArrayIndex(objectName);
		WalPrint("objectLevel: 2, parameterName: %s, objectName: %s, matching index=%d\n",parameterName,objectName,index);
	
		if(index != -1 )
		{
			strcpy(compName,SubComponentValArray[index].comp_name);
			strcpy(dbusPath,SubComponentValArray[index].dbus_path); 
		}
    	}
*/
	return index;	
}

/**
 * @brief getObjectName Get object name from parameter name. Example WiFi from "Device.WiFi.SSID."
 * objectName parameter should be initialized with null terminating characters to handle error scenarios
 *
 * @param[in] str Parameter Name
 * param[out] objectName Set with the object name
 * param[in] objectLevel Level of object 1, 2. Example 1 for WiFi and 2 for SSID
 */
static void getObjectName(char *str, char *objectName, int objectLevel)
{
        char *tmpStr;
        char localStr[MAX_PARAMETERNAME_LEN]={'\0'};
        walStrncpy(localStr,str,sizeof(localStr));
        int count = 1,len;

        if(localStr[0] != '\0')
        {	
                tmpStr = strchr(localStr,'.');
                while (tmpStr != NULL)
                {
                        tmpStr=strchr(tmpStr+1,'.');
                        len = tmpStr-localStr+1;
                        WalPrint ("found at %d\n",len);
                        if(tmpStr && count >= objectLevel)
                        {
                                strncpy(objectName,localStr,len);
				objectName[len] = '\0';
                                WalPrint("_________ objectName %s__________ \n",objectName);
                                break;
                        }
                        count++;
                }
        }
}

static void waitUntilSystemReady()
{
    if(checkIfSystemReady())
	{
		WalInfo("Checked CR - System is ready, proceed with component caching\n");
		system("touch /var/tmp/cacheready");
		processDeviceManageableNotification();
	}
	else
	{
	    CcspBaseIf_Register_Event(bus_handle, NULL, "systemReadySignal");

            CcspBaseIf_SetCallback2
	    (
		    bus_handle,
		    "systemReadySignal",
		    ccspSystemReadySignalCB,
		    NULL
	    );

	    FILE *file;
	    int wait_time = 0;
	    int total_wait_time = 0;

	    // Wait till Call back touches the indicator to proceed further
	    while((file = fopen("/var/tmp/cacheready", "r")) == NULL)
	    {
		    WalInfo("Waiting for system ready signal\n");
		    // After waiting for 24 * 5 = 120s (2mins) send dbus message to CR to query for system ready
		    if(wait_time == 24)
		    {
			    wait_time = 0;
			    if(checkIfSystemReady())
			    {
				    WalInfo("Checked CR - System is ready, proceed with component caching\n");
				    system("touch /var/tmp/cacheready");
				    processDeviceManageableNotification();
				    break;
				    //Break out, System ready signal already delivered
			    }
			    else
			    {
				    WalInfo("Queried CR for system ready after waiting for 2 mins, it is still not ready\n");
				    if(total_wait_time >= 84)
				    {
					    WalInfo("Queried CR for system ready after waiting for 7 mins, it is still not ready. Proceeding ...\n");
					    OnboardLog("Queried CR for system ready after waiting for 7 mins, it is still not ready. Proceeding ...\n");
					    break;
				    }
			    }
		    }
		    sleep(5);
		    wait_time++;
		    total_wait_time++;
	    };
	    // In case of Web PA restart, we should be having cacheready already touched.
	    // In normal boot up we will reach here only when system ready is received.
	    if(file != NULL)
	    {
		    WalInfo("/var/tmp/cacheready file exists, hence can proceed with component caching\n");
		    fclose(file);
	    }
    }
}

/**
 * @brief ccspSystemReadySignalCB Call back function to be executed once we receive system ready signal from CR.
 * This is to make sure that Web PA will do component caching only when system is completely UP
 */
static void ccspSystemReadySignalCB(void* user_data)
{
	// Touch a file to indicate that Web PA can proceed with
	// component caching.
	system("touch /var/tmp/cacheready");
	WalInfo("Received system ready signal, created /var/tmp/cacheready file\n");
	processDeviceManageableNotification();
}

/**
 * @brief checkIfSystemReady Function to query CR and check if system is ready.
 * This is just in case webpa registers for the systemReadySignal event late.
 * If SystemReadySignal is already sent then this will return 1 indicating system is ready.
 */
static int checkIfSystemReady()
{
	char str[MAX_PARAMETERNAME_LEN/2];
	int val, ret;
	snprintf(str, sizeof(str), "eRT.%s", CCSP_DBUS_INTERFACE_CR);
	// Query CR for system ready
	ret = CcspBaseIf_isSystemReady(bus_handle, str, &val);
	WalInfo("checkIfSystemReady(): ret %d, val %d\n", ret, val);
	return val;
}


/**
 * @brief waitForComponentReady Wait till the given component is ready, its health is green
 * @param[in] compName RDKB Component Name
 * @param[in] dbusPath RDKB Dbus Path name
 */
static int waitForComponentReady(char *compName, char *dbusPath)
{
	char status[32] = {'\0'};
	int ret = -1;
	int count = 0;
	while(1)
	{
		checkComponentHealthStatus(compName, dbusPath, status,&ret);
		if(ret == CCSP_SUCCESS && (strcmp(status, "Green") == 0))
		{
                        WalInfo("%s component health is %s, continue\n", compName, status);
			break;
		}
		else
		{
                        count++;
                        if(count > 60)
                        {
                                WalError("%s component Health check failed (ret:%d), continue\n",compName, ret);
                                break;
                        }
			if(count%5 == 0)
			{
				WalError("%s component Health, ret:%d, waiting\n", compName, ret);
			}
			sleep(5);
		}
	}
	return ret;
}

/**
 * @brief checkComponentHealthStatus Query the health of the given component
 * @param[in] compName RDKB Component Name
 * @param[in] dbusPath RDKB Dbus Path name
 * @param[out] status describes component health Red/Green
 * @param[out] retStatus error or status code returned by stack call
 */
static void checkComponentHealthStatus(char * compName, char * dbusPath, char *status, int *retStatus)
{
	int ret = 0, val_size = 0;
	parameterValStruct_t **parameterval = NULL;
	char *parameterNames[1] = {};
	char tmp[MAX_PARAMETERNAME_LEN];
	char str[MAX_PARAMETERNAME_LEN/2];     
	char l_Subsystem[MAX_DBUS_INTERFACE_LEN] = { 0 };
	
	sprintf(tmp,"%s.%s",compName, "Health");
	parameterNames[0] = tmp;
#if !defined(RDKB_EMU)
	walStrncpy(l_Subsystem, "eRT.",sizeof(l_Subsystem));
#endif
	snprintf(str, sizeof(str), "%s%s", l_Subsystem, compName);
	WalPrint("str is:%s\n", str);
		
	ret = CcspBaseIf_getParameterValues(bus_handle, str, dbusPath,  parameterNames, 1, &val_size, &parameterval);
	WalPrint("ret = %d val_size = %d\n",ret,val_size);
	if(ret == CCSP_SUCCESS)
	{
		WalPrint("parameterval[0]->parameterName : %s parameterval[0]->parameterValue : %s\n",parameterval[0]->parameterName,parameterval[0]->parameterValue);
		strcpy(status, parameterval[0]->parameterValue);
		WalPrint("status of component:%s\n", status);
	}
	free_parameterValStruct_t (bus_handle, val_size, parameterval);
	
	*retStatus = ret;
}

/**
 * @brief checkComponentReady Checks if the given component is ready, its health is green
   if not green, retry for 4 times and proceed .
 * @param[in] compName RDKB Component Name
 * @param[in] dbusPath RDKB Dbus Path name
 */
static void checkComponentReady(char *compName, char *dbusPath)
{
	char status[32] = {'\0'};
	int ret = -1;
	int retrycount = 0;
	
	while(retrycount <= 3)
	{
		checkComponentHealthStatus(compName, dbusPath, status,&ret);
		if(ret == CCSP_SUCCESS && (strcmp(status, "Green") == 0))
		{
			WalInfo("checkComponentReady: %s component health is %s, continue\n", compName, status);
			break;
		}
		else
		{
			retrycount++;
			WalError("%s component Health, ret:%d, retrying ...%d...\n", compName, ret, retrycount);
			sleep(5);
			
			if(retrycount > 3)
			{
				WalError("Proceeding as component %s is not up even after retry\n", compName);
				OnboardLog("Proceeding as component %s is not up even after retry\n", compName);
			}
		}
	} 
}

/*
 * @brief To retry component caching for failed objects
 */
static void retryFailedComponentCaching()
{
	int i = 0, ret = 0, size = 0, cnt = 0, cnt1 = 0, retryCount = 0, count = 0, count1 = 0;
	char dst_pathname_cr[MAX_PATHNAME_CR_LEN] = { 0 };
	char l_Subsystem[MAX_DBUS_INTERFACE_LEN] = { 0 };
	char paramName[MAX_PARAMETERNAME_LEN] = { 0 };
	componentStruct_t ** ppComponents = NULL;

	if(compCacheFailedCnt > 0 || subCompCacheFailedCnt > 0)
	{
		WalInfo("-------- retrying failed component caching -------\n");
		strncpy(l_Subsystem, "eRT.",sizeof(l_Subsystem));
		snprintf(dst_pathname_cr, sizeof(dst_pathname_cr),"%s%s", l_Subsystem, CCSP_DBUS_INTERFACE_CR);
		cnt = compCacheSuccessCnt;
		cnt1 = subCompCacheSuccessCnt;
		count = compCacheFailedCnt;
		count1 = subCompCacheFailedCnt;

		for(i = 0; i < count ; i++)
		{
			walStrncpy(paramName,failedCompList[i],sizeof(paramName));
			WalPrint("Retrying for component %s\n",paramName);
			retryCount = 1;
			do
			{
				ret = CcspBaseIf_discComponentSupportingNamespace(bus_handle,
						dst_pathname_cr, paramName, l_Subsystem, &ppComponents, &size);

				if (ret == CCSP_SUCCESS)
				{
					retryCount = 1;
					// Allocate memory for ComponentVal obj_name, comp_name, dbus_path
					ComponentValArray[cnt].obj_name = (char *)malloc(sizeof(char) * (MAX_PARAMETERNAME_LEN/2));
					memset(ComponentValArray[cnt].obj_name, 0, sizeof(char) * (MAX_PARAMETERNAME_LEN/2));
					ComponentValArray[cnt].comp_name = (char *)malloc(sizeof(char) * (MAX_PARAMETERNAME_LEN/2));
					ComponentValArray[cnt].dbus_path = (char *)malloc(sizeof(char) * (MAX_PARAMETERNAME_LEN/2));

					ComponentValArray[cnt].comp_id = cnt;
					ComponentValArray[cnt].comp_size = size;
					getObjectName(paramName,ComponentValArray[cnt].obj_name,1);
					strncpy(ComponentValArray[cnt].comp_name,ppComponents[0]->componentName,MAX_PARAMETERNAME_LEN/2);
					strncpy(ComponentValArray[cnt].dbus_path,ppComponents[0]->dbusPath,MAX_PARAMETERNAME_LEN/2);

					WalInfo("ComponentValArray[%d].comp_id = %d,ComponentValArray[cnt].comp_size = %d, ComponentValArray[%d].obj_name = %s, ComponentValArray[%d].comp_name = %s, ComponentValArray[%d].dbus_path = %s\n", cnt, ComponentValArray[cnt].comp_id,ComponentValArray[cnt].comp_size, cnt, ComponentValArray[cnt].obj_name, cnt, ComponentValArray[cnt].comp_name, cnt, ComponentValArray[cnt].dbus_path);
					cnt++;
				}
				else
				{
					retryCount++;
					WalError("------------Failed to get component info for object %s----------: ret = %d, size = %d, retrying .... %d ...\n", failedCompList[i], ret, size, retryCount);
					if(retryCount == WAL_COMPONENT_INIT_RETRY_COUNT)
					{
						WalError("Unable to get component for object %s\n", failedCompList[i]);
						OnboardLog("Unable to get component for object %s\n", failedCompList[i]);
					}
					else
					{
						sleep(WAL_COMPONENT_INIT_RETRY_INTERVAL);
					}
				}
				free_componentStruct_t(bus_handle, size, ppComponents);
			}while((retryCount > 1) && (retryCount <= 4));
		}

		for(i = 0; i < count1 ; i++)
		{
			walStrncpy(paramName,failedSubCompList[i],sizeof(paramName));
			WalPrint("Retrying for sub-component %s\n",paramName);
			retryCount = 1;
			do
			{
				ret = CcspBaseIf_discComponentSupportingNamespace(bus_handle,
						dst_pathname_cr, paramName, l_Subsystem, &ppComponents, &size);

				if (ret == CCSP_SUCCESS)
				{
					retryCount = 1;
					// Allocate memory for ComponentVal obj_name, comp_name, dbus_path
					SubComponentValArray[cnt1].obj_name = (char *)malloc(sizeof(char) * (MAX_PARAMETERNAME_LEN/2));
					memset(SubComponentValArray[cnt1].obj_name, 0, sizeof(char) * (MAX_PARAMETERNAME_LEN/2));
					SubComponentValArray[cnt1].comp_name = (char *)malloc(sizeof(char) * (MAX_PARAMETERNAME_LEN/2));
					SubComponentValArray[cnt1].dbus_path = (char *)malloc(sizeof(char) * (MAX_PARAMETERNAME_LEN/2));

					SubComponentValArray[cnt1].comp_id = cnt1;
					SubComponentValArray[cnt1].comp_size = size;
					getObjectName(paramName,SubComponentValArray[cnt1].obj_name,2);
					WalPrint("in WALInit() SubComponentValArray[cnt].obj_name is %s",SubComponentValArray[cnt1].obj_name);
					strncpy(SubComponentValArray[cnt1].comp_name,ppComponents[0]->componentName,MAX_PARAMETERNAME_LEN/2);
					strncpy(SubComponentValArray[cnt1].dbus_path,ppComponents[0]->dbusPath,MAX_PARAMETERNAME_LEN/2);

					WalInfo("SubComponentValArray[%d].comp_id = %d,SubComponentValArray[i].comp_size = %d, SubComponentValArray[%d].obj_name = %s, SubComponentValArray[%d].comp_name = %s, SubComponentValArray[%d].dbus_path = %s\n", cnt1, SubComponentValArray[cnt1].comp_id,SubComponentValArray[cnt1].comp_size, cnt1, SubComponentValArray[cnt1].obj_name, cnt1, SubComponentValArray[cnt1].comp_name, cnt1, SubComponentValArray[cnt1].dbus_path);
					cnt1++;
				}
				else
				{
					retryCount++;
					WalError("------------Failed to get component info for object %s----------: ret = %d, size = %d, retrying .... %d ...\n", failedSubCompList[i], ret, size, retryCount);
					if(retryCount == WAL_COMPONENT_INIT_RETRY_COUNT)
					{
						WalError("Unable to get component for object %s\n", failedSubCompList[i]);
						OnboardLog("Unable to get component for object %s\n", failedSubCompList[i]);
					}
					else
					{
						sleep(WAL_COMPONENT_INIT_RETRY_INTERVAL);
					}
				}
				free_componentStruct_t(bus_handle, size, ppComponents);

			}while((retryCount > 1) && (retryCount <= 4));
		}

		compCacheSuccessCnt = cnt;
		WalPrint("compCacheSuccessCnt : %d after retry...\n", compCacheSuccessCnt);
		subCompCacheSuccessCnt = cnt1;
		WalPrint("subCompCacheSuccessCnt : %d after retry...\n", subCompCacheSuccessCnt);
	}
}

WDMP_STATUS check_ethernet_wan_status()
{
    char *status = NULL;
    char isEthEnabled[64]={'\0'};
#ifdef RDKB_BUILD
    if(0 == syscfg_init())
    {
        if( 0 == syscfg_get( NULL, "eth_wan_enabled", isEthEnabled, sizeof(isEthEnabled)) && (isEthEnabled[0] != '\0' && strncmp(isEthEnabled, "true", strlen("true")) == 0))
        {
            WalInfo("Ethernet WAN is enabled\n");
            OnboardLog("Ethernet WAN is enabled\n");
            eth_wan_status = TRUE;
            return WDMP_SUCCESS;
        }
    }
    else
#endif
    {
        if(waitForComponentReady(RDKB_ETHAGENT_COMPONENT_NAME,RDKB_ETHAGENT_DBUS_PATH) != CCSP_SUCCESS)
		{
			return ETH_FAILED;
		}
        status = getParameterValue(ETH_WAN_STATUS_PARAM);
        if(status != NULL && strncmp(status, "true", strlen("true")) == 0)
        {
            WalInfo("Ethernet WAN is enabled\n");
            OnboardLog("Ethernet WAN is enabled\n");
            eth_wan_status = TRUE;
            WAL_FREE(status);
            return WDMP_SUCCESS;
        }
		WAL_FREE(status);
    }
    return WDMP_FAILURE;
}

#ifdef WEBCONFIG_BIN_SUPPORT
WDMP_STATUS createForceSyncJsonSchema(char *value, char *transactionId, char** stringifiedJson)
{
	if( value ==NULL || transactionId == NULL)
	{
		WalError("createForceSyncJsonSchema input values are empty\n");
		return WDMP_FAILURE;
	}

	char forcesyncVal[32] = { '\0' };
	char forcesynctransID[32] = { '\0' };
	cJSON *jsonresponse = NULL;

	walStrncpy(forcesyncVal , value, sizeof(forcesyncVal));
	walStrncpy(forcesynctransID , transactionId, sizeof(forcesynctransID));

	WalPrint("forcesyncVal %s forcesynctransID %s\n", forcesyncVal, forcesynctransID);
	jsonresponse = cJSON_CreateObject();

	if (jsonresponse !=NULL)
	{
		cJSON_AddStringToObject(jsonresponse,"value", forcesyncVal);
		cJSON_AddStringToObject(jsonresponse,"transaction_id", forcesynctransID);

		*stringifiedJson = cJSON_PrintUnformatted(jsonresponse);
		WalPrint("*stringifiedJson is %s\n", *stringifiedJson);
		cJSON_Delete(jsonresponse);
		return WDMP_SUCCESS;
	}
	return WDMP_FAILURE;
}
#endif
