/**
 * @file webconfig_internal.c
 *
 * @description This file describes the webconfig Abstraction Layer
 *
 * Copyright (c) 2019  Comcast
 */
#include <stdio.h>
#include <pthread.h>
#include "webpa_adapter.h"
#include "webpa_internal.h"
#include "webconfig_internal.h"
#include <curl/curl.h>
#include "cJSON.h"
#include "webconfig_log.h"
/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
/* Macros */
#define CURL_TIMEOUT_SEC	   25L
#define CLIENT_CERT_PATH  	   "/etc/clientcert.crt"
#define CA_CERT_PATH 		   "/etc/ssl/certs/ca-certificates.crt"
#define DEVICE_PROPS_FILE          "/etc/device.properties"
#define WEBCFG_INTERFACE_DEFAULT   "erouter0"
#define MAX_BUF_SIZE	           256
#define WEB_CFG_FILE		      "/nvram/webConfig.json"
#define MAX_PARAMETERNAME_LEN			4096
#define WEBPA_READ_HEADER             "/etc/parodus/parodus_read_file.sh"
#define WEBPA_CREATE_HEADER           "/etc/parodus/parodus_create_file.sh"

/*----------------------------------------------------------------------------*/
/*                               Data Structures                              */
/*----------------------------------------------------------------------------*/
struct token_data {
    size_t size;
    char* data;
};
/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
char deviceMac[32]={'\0'};
char *ETAG="NONE";

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/
static void *WebConfigTask(void *status);
int processJsonDocument(char *jsonData);
int validateConfigFormat(cJSON *json, char *etag);
int requestWebConfigData(char **configData, int r_count, int index, int status, long *code);
static void get_webCfg_interface(char **interface);
void createCurlheader(struct curl_slist *list, struct curl_slist **header_list, int status);
size_t write_callback_fn(void *buffer, size_t size, size_t nmemb, struct token_data *data);
WDMP_STATUS setConfigParamValues( param_t paramVal[], int paramCount );
void getAuthToken(char *webpa_auth_token);
/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/

void initWebConfigTask(int * status)
{
	int err = 0;
	pthread_t threadId;

	err = pthread_create(&threadId, NULL, WebConfigTask, (void *) status);
	if (err != 0) 
	{
		WebConfigLog("Error creating WebConfigTask thread :[%s]\n", strerror(err));
	}
	else
	{
		WebConfigLog("WebConfigTask Thread created Successfully\n");
	}
}

static void *WebConfigTask(void *status)
{
	pthread_detach(pthread_self());
	int configRet = -1;
	char *webConfigData = NULL;
	int r_count;
	long res_code;
	int index;
	int json_status=-1;
	int backoffRetryTime = 0;
	int backoff_max_time = 9;
        int max_retry_sleep;
    	int c=2;

	max_retry_sleep = (int) pow(2, backoff_max_time) -1;
        WebConfigLog("max_retry_sleep is %d\n", max_retry_sleep );

	while(1)
	{
		//TODO: iterate through all entries in Device.X_RDK_WebConfig.ConfigFile.[i].URL to check if the current stored version of each configuration document matches the latest version on the cloud. 

		if(backoffRetryTime < max_retry_sleep)
		{
		  backoffRetryTime = (int) pow(2, c) -1;
		}
		WebConfigLog("New backoffRetryTime value calculated as %d seconds\n", backoffRetryTime);

		configRet = requestWebConfigData(&webConfigData, r_count, index, *(int *)status, &res_code);
		WAL_FREE(status);

		if(configRet == 0)
		{
			if(res_code == 304)
			{
				WebConfigLog("webConfig is in sync with cloud. response_code:%d\n", res_code); //:TODO do sync check OK
				break;
			}
			else if(res_code == 200)
			{
				WebConfigLog("webConfig is not in sync with cloud. response_code:%d\n", res_code);

				if(webConfigData !=NULL)
				{
					WebConfigLog("webConfigData fetched successfully\n");
					json_status = processJsonDocument(webConfigData);
					if(json_status == 1)
					{
						WebConfigLog("processJsonDocument success\n");
					}
					else
					{
						WebConfigLog("Failure in processJsonDocument\n");
					}
				}
				break;
			}
			else if(res_code == 204)
			{
				WebConfigLog("No action required from client. response_code:%d\n", res_code);
				break;
			}
			else
			{
				WebConfigLog("Error code returned, need to retry. response_code:%d\n", res_code);
			}
		}
		else
		{
			WebConfigLog("Failed to get webConfigData from cloud\n");	
		}
		WebConfigLog("requestWebConfigData backoffRetryTime %d seconds\n", backoffRetryTime);
		sleep(backoffRetryTime);
		c++;
	}
	return NULL;
}


/*
* @brief Initialize curl object with required options. create configData using libcurl.
* @param[out] configData 
* @param[in] len total configData size
* @param[in] r_count Number of curl retries on ipv4 and ipv6 mode during failure
* @return returns 0 if success, otherwise failed to fetch auth token and will be retried.
*/
int requestWebConfigData(char **configData, int r_count, int index, int status, long *code)
{
	CURL *curl;
	CURLcode res;
	CURLcode time_res;
	struct curl_slist *list = NULL;
	struct curl_slist *headers_list = NULL;
	int i = index, rv=1;
	char *auth_header = NULL;
	char *version_header = NULL;
	double total;
	long response_code = 0;
	char *interface = NULL;
	char *ct = NULL;
	char *URL_param = NULL;
	char *webConfigURL= NULL;
	DATA_TYPE paramType;
	int content_res=0;
	struct token_data data;
	data.size = 0;

	curl = curl_easy_init();
	if(curl)
	{
		//this memory will be dynamically grown by write call back fn as required
		data.data = (char *) malloc(sizeof(char) * 1);
		if(NULL == data.data)
		{
			WebConfigLog("Failed to allocate memory.\n");
			return rv;
		}
		data.data[0] = '\0';
		createCurlheader(list, &headers_list, status);

		URL_param = (char *) malloc(sizeof(char)*MAX_BUF_SIZE);
		if(URL_param !=NULL)
		{
			//snprintf(URL_param, MAX_BUF_SIZE, "Device.X_RDK_WebConfig.ConfigFile.[%d].URL", i);//testing purpose.
			snprintf(URL_param, MAX_BUF_SIZE, "http://96.116.56.207:8080/api/v4/gateway-cpe/%s/config/voice", deviceMac);
			webConfigURL = strdup(URL_param); //testing. remove this.
			WebConfigLog("webConfigURL is %s\n", webConfigURL);
			//webConfigURL = getParameterValue(URL_param, &paramType);
			curl_easy_setopt(curl, CURLOPT_URL, webConfigURL );
		}
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, CURL_TIMEOUT_SEC);

		get_webCfg_interface(&interface);

		if(interface !=NULL && strlen(interface) >0)
		{
			curl_easy_setopt(curl, CURLOPT_INTERFACE, interface);
		}
		// set callback for writing received data 
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback_fn);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &data);

		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers_list);

		// setting curl resolve option as default mode.
		//If any failure, retry with v4 first and then v6 mode. 
		if(r_count == 1)
		{
			WebConfigLog("curl Ip resolve option set as V4 mode\n");
			curl_easy_setopt(curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);
		}
		else if(r_count == 2)
		{
			WebConfigLog("curl Ip resolve option set as V6 mode\n");
			curl_easy_setopt(curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V6);
		}
		else
		{
			WebConfigLog("curl Ip resolve option set as default mode\n");
			curl_easy_setopt(curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_WHATEVER);
		}

		curl_easy_setopt(curl, CURLOPT_CAINFO, CA_CERT_PATH);

		// disconnect if it is failed to validate server's cert 
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
		
		// Verify the certificate's name against host 
  		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);

		// To use TLS version 1.2 or later 
  		curl_easy_setopt(curl, CURLOPT_SSLVERSION, CURL_SSLVERSION_TLSv1_2);

		// To follow HTTP 3xx redirections
  		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

		// Perform the request, res will get the return code 
		res = curl_easy_perform(curl);

		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
		WebConfigLog("webConfig curl response %d http_code %d\n", res, response_code);
		*code = response_code;

		time_res = curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME, &total);
		if(time_res == 0)
		{
			WebConfigLog("curl response Time: %.1f seconds\n", total);
		}
		curl_slist_free_all(headers_list);
		WAL_FREE(URL_param);
		WAL_FREE(webConfigURL);
		if(res != 0)
		{
			WebConfigLog("curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
		}
		else
		{
			content_res = curl_easy_getinfo(curl, CURLINFO_CONTENT_TYPE, &ct);
			if(!content_res && ct)
			{
				if(strcmp(ct, "application/json") !=0)
				{
					WebConfigLog("Invalid Content-Type\n");
				}
				else
				{
					*configData = strdup(data.data);
					WebConfigLog("configData received from cloud is %s\n", *configData);
				}
			}
			
		}
		WAL_FREE(data.data);
		curl_easy_cleanup(curl);
		rv=0;
	}
	else
	{
		WebConfigLog("curl init failure\n");
	}
	return rv;
}

/* @brief callback function for writing libcurl received data
 * @param[in] buffer curl delivered data which need to be saved.
 * @param[in] size size is always 1
 * @param[in] nmemb size of delivered data
 * @param[out] data curl response data saved.
*/
size_t write_callback_fn(void *buffer, size_t size, size_t nmemb, struct token_data *data)
{
    size_t index = data->size;
    size_t n = (size * nmemb);
    char* tmp;

    data->size += (size * nmemb);

    tmp = realloc(data->data, data->size + 1); // +1 for '\0' 

    if(tmp) {
        data->data = tmp;
    } else {
        if(data->data) {
            free(data->data);
        }
        WebConfigLog("Failed to allocate memory for data\n");
        return 0;
    }

    memcpy((data->data + index), buffer, n);
    data->data[data->size] = '\0';

    return size * nmemb;
}

int processJsonDocument(char *jsonData)
{
	cJSON *paramArray = NULL;
	int parseStatus = 0;
	int rollbackRet=0;
	int i=0, item_size=0, getStatus =-1;
	int getRet=0, count =0, setRet2=0, rollbackRet2=0;
	req_struct *reqObj;
	const char *getParamList[MAX_PARAMETERNAME_LEN];
	int paramCount =0;
	param_t *getVal = NULL;
	param_t *storeGetvalArr = NULL;
	param_t *globalRollbackVal=NULL;
	WDMP_STATUS setRet = WDMP_FAILURE, valid_ret = WDMP_FAILURE;
	WDMP_STATUS ret = WDMP_FAILURE;

	parseStatus = parseJsonData(jsonData, &reqObj);
	if(parseStatus ==1)
	{
		WebConfigLog("Request:> Type : %d\n",reqObj->reqType);
		WebConfigLog("Request:> ParamCount = %zu\n",reqObj->u.setReq->paramCnt);
		paramCount = (int)reqObj->u.setReq->paramCnt;
		for (i = 0; i < paramCount; i++) 
		{
		        WebConfigLog("Request:> param[%d].name = %s\n",i,reqObj->u.setReq->param[i].name);
		        WebConfigLog("Request:> param[%d].value = %s\n",i,reqObj->u.setReq->param[i].value);
		        WebConfigLog("Request:> param[%d].type = %d\n",i,reqObj->u.setReq->param[i].type);

		}

		valid_ret = validate_parameter(reqObj->u.setReq->param, paramCount, reqObj->reqType);

		if(valid_ret == WDMP_SUCCESS)
		{
			setValues(reqObj->u.setReq->param, paramCount, WEBPA_SET, NULL, NULL, &ret);
			WebConfigLog("setValues success. ret : %d\n", ret);
			return 1;
		}
		else
		{
			WebConfigLog("validate_parameter failed. parseStatus is %d\n", valid_ret);
			return 0;
		}
	}
	else
	{
		WebConfigLog("parseJsonData failed. parseStatus is %d\n", parseStatus);
		return 0;
	}
	return 0;
}

int parseJsonData(char* jsonData, req_struct **req_obj)
{
	cJSON *json = NULL;
	cJSON *paramData = NULL;
	cJSON *paramArray = NULL;
	int i=0, isValid =0;
	int rv =-1;
	req_struct *reqObj = NULL;
	int paramCount=0;
	WDMP_STATUS ret = WDMP_FAILURE, valid_ret = WDMP_FAILURE;
	int itemSize=0;

	if((jsonData !=NULL) && (strlen(jsonData)>0))
	{
		json = cJSON_Parse( jsonData );
		WAL_FREE(jsonData);

		if( json == NULL )
		{
			WebConfigLog("WebConfig Parse error\n");
			return rv;
		}
		else
		{
			isValid = validateConfigFormat(json, ETAG); //check eTAG value here :TODO
			if(isValid)// testing purpose. make it to !isValid
			{
				WebConfigLog("validateConfigFormat failed\n");
				return rv;
			}
			(reqObj) = (req_struct *) malloc(sizeof(req_struct));
                	memset((reqObj), 0, sizeof(req_struct));

			//testing purpose as json format is differnt in test server
			paramData = cJSON_GetObjectItem( json, "data" );
			//parse_set_request(json, &reqObj, WDMP_TR181); testing purpose.
			parse_set_request(paramData, &reqObj, WDMP_TR181);
			if(reqObj != NULL)
        		{
				*req_obj = reqObj;	
				rv = 1;		
			}
			else
			{
				WebConfigLog("Failed to parse set request\n");
			}
		}
	}
	else
	{
		WebConfigLog("jsonData is empty\n");
	}
	return rv;
}

int validateConfigFormat(cJSON *json, char *eTag)
{
	cJSON *versionObj =NULL;
	cJSON *paramArray = NULL;
	int itemSize=0;
	char *version=NULL;

	versionObj = cJSON_GetObjectItem( json, "version" );
	if(versionObj !=NULL)
	{
		if(cJSON_GetObjectItem( json, "version" )->type == cJSON_String)
		{
			version = cJSON_GetObjectItem( json, "version" )->valuestring;
			if(version !=NULL)
			{
				if(strcmp(version, eTag) == 0)
				{
					//check parameters
					paramArray = cJSON_GetObjectItem( json, "parameters" );
					if( paramArray != NULL )
					{
						itemSize = cJSON_GetArraySize( json );
						if(itemSize ==2)
						{
							return 1;
						}
						else
						{
							WebConfigLog("config contains fields other than version and parameters\n");
							return 0;
						}
					}
					else
					{
						WebConfigLog("Invalid config json, parameters field is not present\n");
						return 0;
					}
				}
				else
				{
					WebConfigLog("Invalid config json, version and ETAG are not same\n");
					return 0;
				}
			}
		}
	}
	else
	{
		WebConfigLog("Invalid config json, version field is not present\n");
		return 0;
	}

	return 0;
}

static void get_webCfg_interface(char **interface)
{

	FILE *fp = fopen(DEVICE_PROPS_FILE, "r");

	if (NULL != fp)
	{
		char str[255] = {'\0'};
		while(fscanf(fp,"%s", str) != EOF)
		{
		    char *value = NULL;

		    if(NULL != (value = strstr(str, "WEBCONFIG_INTERFACE=")))
		    {
			value = value + strlen("WEBCONFIG_INTERFACE=");
			*interface = strdup(value);
		    }

		}
		fclose(fp);
	}
	else
	{
		WebConfigLog("Failed to open device.properties file:%s\n", DEVICE_PROPS_FILE);
		WebConfigLog("Adding default values for webConfig interface\n");
		*interface = strdup(WEBCFG_INTERFACE_DEFAULT);
	}

	if (NULL == *interface)
	{
		WebConfigLog("WebConfig interface is not present in device.properties, adding default interface\n");
		
		*interface = strdup(WEBCFG_INTERFACE_DEFAULT);
	}
	else
	{
		WebConfigLog("interface fetched is %s\n", *interface);
	}
}

/* @brief Function to create curl header options
 * @param[in] list temp curl header list
 * @param[in] device status value
 * @param[out] header_list output curl header list
*/
void createCurlheader( struct curl_slist *list, struct curl_slist **header_list, int status)
{
	char *version_header = NULL;
	char webpa_auth_token[4096];
	char *auth_header = NULL;
	char *status_header=NULL;
	char *bootTime = NULL, *bootTime_header = NULL;
	char *FwVersion = NULL, *FwVersion_header=NULL;
	char *systemReadyTime = NULL, *systemReadyTime_header=NULL;
	struct timespec cTime;
	char currentTime[32];
	char *currentTime_header=NULL;

	//Fetch auth JWT token from cloud.
	getAuthToken(webpa_auth_token); 

	auth_header = (char *) malloc(sizeof(char)*MAX_PARAMETERNAME_LEN);
	if(auth_header !=NULL)
	{
		snprintf(auth_header, MAX_PARAMETERNAME_LEN, "Authorization:Bearer %s", (0 < strlen(webpa_auth_token) ? webpa_auth_token : NULL));
		list = curl_slist_append(list, auth_header);
		WAL_FREE(auth_header);
	}

	version_header = (char *) malloc(sizeof(char)*MAX_BUF_SIZE);
	if(version_header !=NULL)
	{
		//cur_firmware_ver = getParameterValue(FIRMWARE_VERSION); :TODO get ETAG from dynamic table entry
		//snprintf(version_header, MAX_BUF_SIZE, "IF-NONE-MATCH:[%s]-[%d]", cur_firmware_ver, ETAG_version);
		if(ETAG !=NULL)
		{
			snprintf(version_header, MAX_BUF_SIZE, "XV-Version:%s", ETAG);
			WebConfigLog("version_header formed %s\n", version_header);
			list = curl_slist_append(list, version_header);
			WAL_FREE(version_header);
		}
		else
		{
			WebConfigLog("Failed to create version header\n");
		}
	}

	bootTime = getParameterValue(DEVICE_BOOT_TIME);
	if(bootTime !=NULL)
	{
		bootTime_header = (char *) malloc(sizeof(char)*MAX_BUF_SIZE);
		if(bootTime_header !=NULL)
		{
			snprintf(bootTime_header, MAX_BUF_SIZE, "X-System-Boot-Time: %s", bootTime);
			WebConfigLog("bootTime_header formed %s\n", bootTime_header);
			list = curl_slist_append(list, bootTime_header);
			WAL_FREE(bootTime_header);
		}
		WAL_FREE(bootTime);
	}
	else
	{
		WebConfigLog("Failed to get bootTime\n");
	}

	FwVersion = getParameterValue(FIRMWARE_VERSION);
	if(FwVersion !=NULL)
	{
		FwVersion_header = (char *) malloc(sizeof(char)*MAX_BUF_SIZE);
		if(FwVersion_header !=NULL)
		{
			snprintf(FwVersion_header, MAX_BUF_SIZE, "X-System-Firmware-Version: %s", FwVersion);
			WebConfigLog("FwVersion_header formed %s\n", FwVersion_header);
			list = curl_slist_append(list, FwVersion_header);
			WAL_FREE(FwVersion_header);
		}
		WAL_FREE(FwVersion);
	}
	else
	{
		WebConfigLog("Failed to get FwVersion\n");
	}

	status_header = (char *) malloc(sizeof(char)*MAX_BUF_SIZE);
	if(status_header !=NULL)
	{
		if(status !=0)
		{
			snprintf(status_header, MAX_BUF_SIZE, "X-System-Status: %s", "Non-Operational");
		}
		else
		{
			snprintf(status_header, MAX_BUF_SIZE, "X-System-Status: %s", "Operational");
		}
		WebConfigLog("status_header formed %s\n", status_header);
		list = curl_slist_append(list, status_header);
		WAL_FREE(status_header);
	}

	memset(currentTime, 0, sizeof(currentTime));
	getCurrentTime(&cTime);
	snprintf(currentTime,sizeof(currentTime),"%d",(int)cTime.tv_sec);
	currentTime_header = (char *) malloc(sizeof(char)*MAX_BUF_SIZE);
	if(currentTime_header !=NULL)
	{
		snprintf(currentTime_header, MAX_BUF_SIZE, "X-System-Current-Time: %s", currentTime);
		WebConfigLog("currentTime_header formed %s\n", currentTime_header);
		list = curl_slist_append(list, currentTime_header);
		WAL_FREE(currentTime_header);
	}

	systemReadyTime = get_global_systemReadyTime();
	if(systemReadyTime !=NULL)
	{
		systemReadyTime_header = (char *) malloc(sizeof(char)*MAX_BUF_SIZE);
		if(systemReadyTime_header !=NULL)
		{
			snprintf(systemReadyTime_header, MAX_BUF_SIZE, "X-System-Ready-Time: %s", systemReadyTime);
			WebConfigLog("systemReadyTime_header formed %s\n", systemReadyTime_header);
			list = curl_slist_append(list, systemReadyTime_header);
			WAL_FREE(systemReadyTime_header);
		}
		WAL_FREE(systemReadyTime);
	}
	else
	{
		WebConfigLog("Failed to get systemReadyTime\n");
	}
	*header_list = list;
}


void execute_token_script(char *token, char *name, size_t len, char *mac, char *serNum)
{
    FILE* out = NULL, *file = NULL;
    char command[MAX_BUF_SIZE] = {'\0'};
    if(strlen(name)>0)
    {
        file = fopen(name, "r");
        if(file)
        {
            snprintf(command,sizeof(command),"%s %s %s",name,serNum,mac);
            out = popen(command, "r");
            if(out)
            {
                fgets(token, len, out);
                pclose(out);
            }
            fclose(file);
        }
        else
        {
            WebConfigLog ("File %s open error\n", name);
        }
    }
}

/*
* call parodus create/acquisition script to create new auth token, if success then calls
* execute_token_script func with args as parodus read script.
*/

void createNewAuthToken(char *newToken, size_t len, char *hw_mac, char* hw_serial_number)
{
	//Call create script
	char output[12] = {'\0'};
	execute_token_script(output,WEBPA_CREATE_HEADER,sizeof(output),hw_mac,hw_serial_number);
	if (strlen(output)>0  && strcmp(output,"SUCCESS")==0)
	{
		//Call read script
		execute_token_script(newToken,WEBPA_READ_HEADER,len,hw_mac,hw_serial_number);
	}
	else
	{
		WebConfigLog("Failed to create new token\n");
	}
}

/*
* Fetches authorization token from the output of read script. If read script returns "ERROR"
* it will call createNewAuthToken to create and read new token
*/

void getAuthToken(char *webpa_auth_token)
{
	//local var to update webpa_auth_token only in success case
	char output[4069] = {'\0'} ;
	char *macID = NULL;
	char deviceMACValue[32] = { '\0' };
	char *hw_serial_number=NULL;

	if( strlen(WEBPA_READ_HEADER) !=0 && strlen(WEBPA_CREATE_HEADER) !=0)
	{
		macID = getParameterValue(DEVICE_MAC);
		if (macID != NULL)
		{
		    strncpy(deviceMACValue, macID, strlen(macID)+1);
		    macToLower(deviceMACValue, deviceMac);
		    WebConfigLog("deviceMAC: %s\n", deviceMac);
		    WAL_FREE(macID);
		}
		if( deviceMac != NULL && strlen(deviceMac) !=0 )
		{
			hw_serial_number = getParameterValue(SERIAL_NUMBER);
			WebConfigLog("hw_serial_number: %s\n", hw_serial_number);

			if( hw_serial_number != NULL && strlen(hw_serial_number) !=0 )
			{
				execute_token_script(output, WEBPA_READ_HEADER, sizeof(output), deviceMac, hw_serial_number);
				if ((strlen(output) == 0))
				{
					WebConfigLog("Unable to get auth token\n");
				}
				else if(strcmp(output,"ERROR")==0)
				{
					WebConfigLog("Failed to read token from %s. Proceeding to create new token.\n",WEBPA_READ_HEADER);
					//Call create/acquisition script
					createNewAuthToken(webpa_auth_token, sizeof(webpa_auth_token), deviceMac, hw_serial_number );
				}
				else
				{
					WebConfigLog("update webpa_auth_token in success case\n");
					walStrncpy(webpa_auth_token, output, sizeof(webpa_auth_token));
				}
			}
			else
			{
				WebConfigLog("hw_serial_number is NULL, failed to fetch auth token\n");
			}
		}
		else
		{
			WebConfigLog("deviceMAC is NULL, failed to fetch auth token\n");
		}
	}
	else
	{
		WebConfigLog("Both read and write file are NULL \n");
	}
}
