/**
 * @file webpa_internal.h
 *
 * @description This file describes the Webpa Abstraction Layer
 *
 * Copyright (c) 2015  Comcast
 */
#include "ssp_global.h"
#include <stdio.h>
#include "ccsp_dm_api.h"
#include <sys/time.h>
#include "webpa_adapter.h"

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/

#define WIFI_INDEX_MAP_SIZE                     18
#define WIFI_PARAM_MAP_SIZE			3
#define WIFI_MAX_STRING_LEN			4096
#define MAX_PARAMETERNAME_LEN			4096
#define MAX_PARAMETERVALUE_LEN			4096
#define MAX_DBUS_INTERFACE_LEN			32
#define MAX_PATHNAME_CR_LEN			64
#define CCSP_COMPONENT_ID_WebPA			0x0000000A
#define CCSP_COMPONENT_ID_XPC			0x0000000B
#if defined(FEATURE_SUPPORT_WEBCONFIG)
#define RDKB_PARAM_WEBCONFIG	"Device.X_RDK_WebConfig."
#define RDKB_TR181_OBJECT_LEVEL1_COUNT	        46
#define RDKB_TR181_OBJECT_LEVEL2_COUNT	        19
#else
#define RDKB_TR181_OBJECT_LEVEL1_COUNT	        45
#define RDKB_TR181_OBJECT_LEVEL2_COUNT	        18
#endif
#define WAL_COMPONENT_INIT_RETRY_COUNT          4
#define WAL_COMPONENT_INIT_RETRY_INTERVAL       10
#define CCSP_ERR_WIFI_BUSY			503
#define CCSP_ERR_INVALID_WIFI_INDEX             504
#define CCSP_ERR_INVALID_RADIO_INDEX            505
#define MAX_ROW_COUNT                           128
#define NAME_VALUE_COUNT                        2 	

#define RDKB_WEBPA_COMPONENT_NAME               "com.cisco.spvtg.ccsp.webpaagent"
#define RDKB_WIFI_COMPONENT_NAME	        "com.cisco.spvtg.ccsp.wifi"
#define RDKB_WIFI_DBUS_PATH		        "/com/cisco/spvtg/ccsp/wifi"
#define RDKB_WIFI_FULL_COMPONENT_NAME	        "eRT.com.cisco.spvtg.ccsp.wifi"
#define RDKB_PAM_COMPONENT_NAME              "com.cisco.spvtg.ccsp.pam"
#define RDKB_PAM_DBUS_PATH                   "/com/cisco/spvtg/ccsp/pam"
#define RDKB_CM_COMPONENT_NAME                  "com.cisco.spvtg.ccsp.cm"
#define RDKB_CM_DBUS_PATH                       "/com/cisco/spvtg/ccsp/cm"
#define RDKB_LM_COMPONENT_NAME              "com.cisco.spvtg.ccsp.lmlite"
#define RDKB_LM_DBUS_PATH                   "/com/cisco/spvtg/ccsp/lmlite"
#define PARAM_CID                      "Device.DeviceInfo.Webpa.X_COMCAST-COM_CID"
#define PARAM_CMC                      "Device.DeviceInfo.Webpa.X_COMCAST-COM_CMC"
#if defined(_COSA_BCM_MIPS_)
#define DEVICE_MAC                   "Device.DPoE.Mac_address"
#elif defined(PLATFORM_RASPBERRYPI)
#define DEVICE_MAC                   "Device.Ethernet.Interface.5.MACAddress"
#elif defined(RDKB_EMU)
#define DEVICE_MAC                   "Device.DeviceInfo.X_COMCAST-COM_WAN_MAC"
#else
#define DEVICE_MAC                   "Device.X_CISCO_COM_CableModem.MACAddress"
#endif
#define PARAM_REBOOT_REASON		     "Device.DeviceInfo.X_RDKCENTRAL-COM_LastRebootReason"
#define ALIAS_PARAM				"Alias"
#define PARAM_RADIO_OBJECT            "Device.WiFi.Radio."
#define WEBPA_PROTOCOL                "WEBPA-2.0"
#if defined(_ENABLE_EPON_SUPPORT_)
#define RDKB_EPON_COMPONENT_NAME                  "com.cisco.spvtg.ccsp.epon"
#define RDKB_EPON_DBUS_PATH                       "/com/cisco/spvtg/ccsp/epon"
#endif
#define ETH_WAN_STATUS_PARAM "Device.Ethernet.X_RDKCENTRAL-COM_WAN.Enabled"
#define RDKB_ETHAGENT_COMPONENT_NAME                  "com.cisco.spvtg.ccsp.ethagent"
#define RDKB_ETHAGENT_DBUS_PATH                       "/com/cisco/spvtg/ccsp/ethagent"

/* RDKB Logger defines */
#define LOG_FATAL       0
#define LOG_ERROR       1
#define LOG_WARN        2
#define LOG_NOTICE      3
#define LOG_INFO        4
#define LOG_DEBUG       5
/*----------------------------------------------------------------------------*/
/*                               Data Structures                              */
/*----------------------------------------------------------------------------*/

typedef struct
{
	ULONG WebPaInstanceNumber;
	ULONG CcspInstanceNumber;
}CpeWebpaIndexMap;

typedef struct 
{
  int comp_id;   //Unique id for the component
  int comp_size;
  char *obj_name;
  char *comp_name;
  char *dbus_path;
}ComponentVal;

typedef struct 
{
  int parameterCount;   
  char **parameterName;
  char *comp_name;
  char *dbus_path;
}ParamCompList;

extern ANSC_HANDLE bus_handle;

typedef enum
{
    SUCCESS = 0,
    PAM_FAILED,
    EPON_FAILED,
    CM_FAILED,
    PSM_FAILED,
    WIFI_FAILED
} COMPONENT_STATUS;
/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/

/**
* @brief walStrncpy WAL String copy function that copies the content of source string into destination string 
* and null terminates the destination string
*
* @param[in] destStr Destination String
* @param[in] srcStr Source String
* @param[in] destSize size of destination string
*/
void walStrncpy(char *destStr, const char *srcStr, size_t destSize);

/*
* @brief mapStatus Defines WDMP status values from corresponding ccsp values
* @param[in] ret ccsp status values from stack
*/
WDMP_STATUS mapStatus(int ret);

/**
 * @brief IndexMpa_WEBPAtoCPE maps to CPE index
 * @param[in] pParameterName parameter name
 */
int IndexMpa_WEBPAtoCPE(char *pParameterName);

/**
 * @brief IndexMpa_CPEtoWEBPA maps to WEBPA index
 * @param[in] pParameterName parameter name
 */
void IndexMpa_CPEtoWEBPA(char **ppParameterName);

/**
 * @brief getComponentDetails Returns the compName list and dbusPath list
 *
 * @param[in] parameterName parameter Name
 * @param[out] compName component name array
 * @param[out] dbusPath dbuspath array
 * @param[out] error 
 * @param[out] retCount count of components	
 */
int getComponentDetails(char *parameterName,char ***compName,char ***dbusPath, int * error, int *retCount);

/**
 * @brief To free allocated memory of get component details
 *
 * @param[in] compName component name list 
 * @param[in] dbusPath dbuspath list 
 * @param[in] size number of components  
 */
void free_componentDetails(char **compName,char **dbusPath,int size);

 /**
 * @brief prepareParamGroups groups parameters based on component 
 *
 * @param[in] ParamGroup ParamCompList formed during GET request to group parameters based on components
 * @param[in] paramCount count of parameters
 * @param[in] cnt1 ParamGroup index
 * @param[in] paramName parameter Name
 * @param[out] compName component name
 * @param[out] dbusPath dbuspath
 * @param[out] compCount returns number of components  
 */
void prepareParamGroups(ParamCompList **ParamGroup,int paramCount,int cnt1,char *paramName,char *compName,char *dbusPath, int * compCount );

/**
 * @brief To free allocated memory for ParamCompList
 *
 * @param[in] ParamGroup ParamCompList formed during GET request to group parameters based on components
 * @param[in] compCount number of components  
 */
void free_ParamCompList(ParamCompList *ParamGroup, int compCount);

/**
 * @brief getParameterValue interface handles GET parameter requests.
 * Returns the parameter value from stack
 *
 * @param[in] paramName
 * @return char*  parameter value
 */
char * getParameterValue(char *paramName);

/**
 * @brief setParameterValue interface handles SET parameter requests in non-atomic way.
 * Sets the parameter value to stack and returns the status
 *
 * @param[in] setParameterValue
 * @param[in] value parameter value string
 * @param[in] type data type
 * @return WDMP_STATUS success or failure status
 */
WDMP_STATUS setParameterValue(char *paramName, char* value, DATA_TYPE type);

/**
 * @brief ccspWebPaValueChangedCB callback function for set notification
 *
 * @param[in] val parameterSigStruct_t notification struct got from stack
 * @param[in] size 
 * @param[in] user_data
 */
void ccspWebPaValueChangedCB(parameterSigStruct_t* val, int size,void* user_data);

/*
 * @brief To convert MAC to lower case without colon
 * assuming max MAC size as 32
 */
void macToLower(char macValue[],char macConverted[]);

int getWebpaParameterValues(char **parameterNames, int paramCount, int *val_size, parameterValStruct_t ***val);
int setWebpaParameterValues(parameterValStruct_t *val, int paramCount, char **faultParam );

/**
 * @brief validate_parameter validates parameter values
 *
 * @param[in] param arry if parameters
 * @param[in] paramCount input cid
 */
WDMP_STATUS validate_parameter(param_t *param, int paramCount, REQ_TYPE type);

#ifdef FEATURE_SUPPORT_WEBCONFIG
/*
 * @brief webConfig getter function to get systemReadyTime from webpa
 * @return systemReadyTime in UTC format.
 */
char *get_global_systemReadyTime();

/*
 * @brief To initiate webConfig Task handling
 */
void initWebConfigTask();
#endif
BOOL get_eth_wan_status();

WDMP_STATUS check_ethernet_wan_status();

void processDeviceManageableNotification();
