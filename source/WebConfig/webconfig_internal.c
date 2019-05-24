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
#include <uuid/uuid.h>
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
#define BACKOFF_SLEEP_DELAY_SEC 	    10
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
char serialNum[64]={'\0'};
char webpa_auth_token[4096]={'\0'};
/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/
static void *WebConfigTask();
int processJsonDocument(char *jsonData);
int validateConfigFormat(cJSON *json, char *etag);
int requestWebConfigData(char **configData, int r_count, int index, int status, long *code);
static void get_webCfg_interface(char **interface);
void createCurlheader(struct curl_slist *list, struct curl_slist **header_list, int status);
size_t write_callback_fn(void *buffer, size_t size, size_t nmemb, struct token_data *data);
WDMP_STATUS setConfigParamValues( param_t paramVal[], int paramCount );
void getAuthToken();
void createNewAuthToken(char *newToken, size_t len, char *hw_mac, char* hw_serial_number);
int handleHttpResponse(long response_code, char *webConfigData );
static char* generate_trans_uuid();
/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/

void initWebConfigTask(int status)
{
	int err = 0;
	pthread_t threadId;

	err = pthread_create(&threadId, NULL, WebConfigTask, (void *) device_status);
	if (err != 0) 
	{
		WalError("Error creating WebConfigTask thread :[%s]\n", strerror(err));
	}
	else
	{
		WalInfo("WebConfigTask Thread created Successfully\n");
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
	int retry_count=0, rv=0;

	while(1)
	{
		//TODO: iterate through all entries in Device.X_RDK_WebConfig.ConfigFile.[i].URL to check if the current stored version of each configuration document matches the latest version on the cloud. 

		if(retry_count >=3)
		{
			WalError("retry_count has reached max limit %d. Exiting.\n",retry_count);
			break;
		}
		WalInfo("calling requestWebConfigData\n");
		configRet = requestWebConfigData(&webConfigData, r_count, index, *(int *)status, &res_code);
		WalInfo("requestWebConfigData done\n");
		WAL_FREE(status);
		WalInfo("free for status done\n");
		if(configRet == 0)
		{
			WalInfo("B4 handleHttpResponse\n");
			rv = handleHttpResponse(res_code, webConfigData);
			if(rv ==1)
			{
				WalInfo("No retries are required. Exiting..\n");
				break;
			}
		}
		else
		{
			WalError("Failed to get webConfigData from cloud\n");
		}
		WalInfo("requestWebConfigData BACKOFF_SLEEP_DELAY_SEC is %d seconds\n", BACKOFF_SLEEP_DELAY_SEC);
		sleep(BACKOFF_SLEEP_DELAY_SEC);
		retry_count++;
	}
	return NULL;
}

int handleHttpResponse(long response_code, char *webConfigData)
{
	int first_digit=0;
	int json_status=0;

	if(response_code == 304)
	{
		WalInfo("webConfig is in sync with cloud. response_code:%d\n", response_code); //:TODO do sync check OK
		return 1;
	}
	else if(response_code == 200)
	{
		WalInfo("webConfig is not in sync with cloud. response_code:%d\n", response_code);

		if(webConfigData !=NULL)
		{
			WalInfo("webConfigData fetched successfully\n");
			json_status = processJsonDocument(webConfigData);
			//WalInfo("freeing webConfigData\n");
			//WAL_FREE(webConfigData);
			//WalInfo("free for webConfigData done\n");
			if(json_status == 1)
			{
				WalInfo("processJsonDocument success\n");
				return 1;
			}
			else
			{
				WalError("Failure in processJsonDocument\n");
			}
		}
		else
		{
			WalError("webConfigData is empty, need to retry\n");
		}
	}
	else if(response_code == 204)
	{
		WalError("No configuration available for this device. response_code:%d\n", response_code);
		return 1;
	}
	else if(response_code == 403)
	{
		WalError("Token is expired, fetch new token. response_code:%d\n", response_code);
		WalInfo("serialNum is %s\n",serialNum);
		createNewAuthToken(webpa_auth_token, sizeof(webpa_auth_token), deviceMac, serialNum );
		WalInfo("createNewAuthToken done in 403 case\n");
	}
	else if(response_code == 429)
	{
		WalInfo("No action required from client. response_code:%d\n", response_code);
		return 1;
	}
	WalInfo("B4 first_digit\n");
	first_digit = (int)(response_code / pow(10, (int)log10(response_code)));
	printf("First digit is %d\n", first_digit);
	if(first_digit == 4) //4xx
	{
		WalError("action not supported. response_code:%d\n", response_code);
		return 1;
	}
	else //5xx & all other errors
	{
		WalError("Error code returned, need to retry. response_code:%d\n", response_code);
	}
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

	WalInfo("-----------start of requestWebConfigData --------\n");

	curl = curl_easy_init();
	if(curl)
	{
		//this memory will be dynamically grown by write call back fn as required
		data.data = (char *) malloc(sizeof(char) * 1);
		if(NULL == data.data)
		{
			WalError("Failed to allocate memory.\n");
			return rv;
		}
		data.data[0] = '\0';
		createCurlheader(list, &headers_list, status);
		WalInfo("createCurlheader done\n");
		URL_param = (char *) malloc(sizeof(char)*MAX_BUF_SIZE);
		if(URL_param !=NULL)
		{
			//snprintf(URL_param, MAX_BUF_SIZE, "Device.X_RDK_WebConfig.ConfigFile.[%d].URL", i);//testing purpose.
			snprintf(URL_param, MAX_BUF_SIZE, "http://96.116.56.207:8080/api/v4/gateway-cpe/%s/config/voice", deviceMac);
			webConfigURL = strdup(URL_param); //testing. remove this.
			WalInfo("webConfigURL is %s\n", webConfigURL);
			//webConfigURL = getParameterValue(URL_param, &paramType);
			curl_easy_setopt(curl, CURLOPT_URL, webConfigURL );
		}
		WalInfo("setting CURLOPT_TIMEOUT\n");
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, CURL_TIMEOUT_SEC);
		WalInfo("B4 get interface\n");
		get_webCfg_interface(&interface);
		WalInfo("After get_webCfg_interface\n");
		if(interface !=NULL && strlen(interface) >0)
		{
			curl_easy_setopt(curl, CURLOPT_INTERFACE, interface);
		}
		WalInfo("setting CURLOPT_WRITEFUNCTION\n");
		// set callback for writing received data 
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback_fn);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &data);

		WalInfo("setting headers_list\n");
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers_list);

		// setting curl resolve option as default mode.
		//If any failure, retry with v4 first and then v6 mode. 
		if(r_count == 1)
		{
			WalInfo("curl Ip resolve option set as V4 mode\n");
			curl_easy_setopt(curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);
		}
		else if(r_count == 2)
		{
			WalInfo("curl Ip resolve option set as V6 mode\n");
			curl_easy_setopt(curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V6);
		}
		else
		{
			WalInfo("curl Ip resolve option set as default mode\n");
			curl_easy_setopt(curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_WHATEVER);
		}
		WalInfo("setting CAINFO\n");
		curl_easy_setopt(curl, CURLOPT_CAINFO, CA_CERT_PATH);
		WalInfo("setting CURLOPT_SSL_VERIFYPEER\n");
		// disconnect if it is failed to validate server's cert 
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
		WalInfo("setting CAINFO\n");
		// Verify the certificate's name against host 
  		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
		WalInfo("setting VERIFYHOST\n");
		// To use TLS version 1.2 or later 
  		curl_easy_setopt(curl, CURLOPT_SSLVERSION, CURL_SSLVERSION_TLSv1_2);
		WalInfo("setting SSLVERSION\n");
		// To follow HTTP 3xx redirections
  		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
		WalInfo("setting FOLLOWLOCATION\n");
		// Perform the request, res will get the return code 
		res = curl_easy_perform(curl);
		WalInfo("After curl_easy_perform\n");
		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
		WalInfo("webConfig curl response %d http_code %d\n", res, response_code);
		*code = response_code;
		WalInfo("B4 CURLINFO_TOTAL_TIME\n");
		time_res = curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME, &total);
		if(time_res == 0)
		{
			WalInfo("curl response Time: %.1f seconds\n", total);
		}
		WalInfo("free headers_list\n");
		curl_slist_free_all(headers_list);
		WalInfo("free URL_param\n");
		WAL_FREE(URL_param);
		WalInfo("free webConfigURL\n");
		WAL_FREE(webConfigURL);
		WalInfo("free done\n");
		if(res != 0)
		{
			WalError("curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
		}
		else
		{
			//content_res = curl_easy_getinfo(curl, CURLINFO_CONTENT_TYPE, &ct);
			//if(!content_res && ct)
			//{
			//	if(strcmp(ct, "application/json") !=0)
			//	{
			//		WalError("Invalid Content-Type\n");
		//		}
			//	else
			//	{
				if(response_code == 200)
				{
					WalInfo("copying to configData\n");
					*configData = strdup(data.data);
					WalInfo("configData received from cloud is %s\n", *configData);
				}
			//}
			
		}
		WalInfo("free data.data\n");
		WAL_FREE(data.data);
		WalInfo("B4 curl_easy_cleanup\n");
		curl_easy_cleanup(curl);
		rv=0;
		WalInfo("curl_easy_cleanup done\n");
	}
	else
	{
		WalError("curl init failure\n");
	}
	WalInfo("-----------End of requestWebConfigData --------\n");
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
        WalError("Failed to allocate memory for data\n");
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
		WalInfo("Request:> Type : %d\n",reqObj->reqType);
		WalInfo("Request:> ParamCount = %zu\n",reqObj->u.setReq->paramCnt);
		paramCount = (int)reqObj->u.setReq->paramCnt;
		for (i = 0; i < paramCount; i++) 
		{
		        WalPrint("Request:> param[%d].name = %s\n",i,reqObj->u.setReq->param[i].name);
		        WalPrint("Request:> param[%d].value = %s\n",i,reqObj->u.setReq->param[i].value);
		        WalPrint("Request:> param[%d].type = %d\n",i,reqObj->u.setReq->param[i].type);

		}

		valid_ret = validate_parameter(reqObj->u.setReq->param, paramCount, reqObj->reqType);

		if(valid_ret == WDMP_SUCCESS)
		{
			setValues(reqObj->u.setReq->param, paramCount, WEBPA_SET, NULL, NULL, &ret);
			WalInfo("setValues success. ret : %d\n", ret);
			return 1;
		}
		else
		{
			WalError("validate_parameter failed. parseStatus is %d\n", valid_ret);
			return 0;
		}
	}
	else
	{
		WalError("parseJsonData failed. parseStatus is %d\n", parseStatus);
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
			WalError("WebConfig Parse error\n");
			return rv;
		}
		else
		{
			isValid = validateConfigFormat(json, ETAG); //check eTAG value here :TODO
			if(isValid)// testing purpose. make it to !isValid
			{
				WalError("validateConfigFormat failed\n");
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
				WalError("Failed to parse set request\n");
			}
		}
	}
	else
	{
		WalError("jsonData is empty\n");
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
							WalError("config contains fields other than version and parameters\n");
							return 0;
						}
					}
					else
					{
						WalError("Invalid config json, parameters field is not present\n");
						return 0;
					}
				}
				else
				{
					WalError("Invalid config json, version and ETAG are not same\n");
					return 0;
				}
			}
		}
	}
	else
	{
		WalError("Invalid config json, version field is not present\n");
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
		WalError("Failed to open device.properties file:%s\n", DEVICE_PROPS_FILE);
		WalInfo("Adding default values for webConfig interface\n");
		*interface = strdup(WEBCFG_INTERFACE_DEFAULT);
	}

	if (NULL == *interface)
	{
		WalError("WebConfig interface is not present in device.properties, adding default interface\n");
		
		*interface = strdup(WEBCFG_INTERFACE_DEFAULT);
	}
	else
	{
		WalPrint("interface fetched is %s\n", *interface);
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
	//char webpa_auth_token[4096];
	char *auth_header = NULL;
	char *status_header=NULL;
	char *schema_header=NULL;
	char *bootTime = NULL, *bootTime_header = NULL;
	char *FwVersion = NULL, *FwVersion_header=NULL;
	char *systemReadyTime = NULL, *systemReadyTime_header=NULL;
	struct timespec cTime;
	char currentTime[32];
	char *currentTime_header=NULL;
	char *uuid_header = NULL;
	char *transaction_uuid = NULL;

	WalInfo("Start of createCurlheader\n");
	//Fetch auth JWT token from cloud.
	getAuthToken();
	WalInfo("webpa_auth_token is %s\n", webpa_auth_token);

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
			snprintf(version_header, MAX_BUF_SIZE, "IF-NONE-MATCH:%s", ETAG);
			WalInfo("version_header formed %s\n", version_header);
			list = curl_slist_append(list, version_header);
			WAL_FREE(version_header);
		}
		else
		{
			WalError("Failed to create version header\n");
		}
	}

	schema_header = (char *) malloc(sizeof(char)*MAX_BUF_SIZE);
	if(schema_header !=NULL)
	{
		snprintf(schema_header, MAX_BUF_SIZE, "Schema-Version: %s", "v1.0");
		WalInfo("schema_header formed %s\n", schema_header);
		list = curl_slist_append(list, schema_header);
		WAL_FREE(schema_header);
	}
	WalInfo("schema_header done\n");
	bootTime = getParameterValue(DEVICE_BOOT_TIME);
	if(bootTime !=NULL)
	{
		bootTime_header = (char *) malloc(sizeof(char)*MAX_BUF_SIZE);
		if(bootTime_header !=NULL)
		{
			snprintf(bootTime_header, MAX_BUF_SIZE, "X-System-Boot-Time: %s", bootTime);
			WalInfo("bootTime_header formed %s\n", bootTime_header);
			list = curl_slist_append(list, bootTime_header);
			WAL_FREE(bootTime_header);
		}
		WAL_FREE(bootTime);
	}
	else
	{
		WalError("Failed to get bootTime\n");
	}

	FwVersion = getParameterValue(FIRMWARE_VERSION);
	if(FwVersion !=NULL)
	{
		FwVersion_header = (char *) malloc(sizeof(char)*MAX_BUF_SIZE);
		if(FwVersion_header !=NULL)
		{
			snprintf(FwVersion_header, MAX_BUF_SIZE, "X-System-Firmware-Version: %s", FwVersion);
			WalInfo("FwVersion_header formed %s\n", FwVersion_header);
			list = curl_slist_append(list, FwVersion_header);
			WAL_FREE(FwVersion_header);
		}
		WAL_FREE(FwVersion);
	}
	else
	{
		WalError("Failed to get FwVersion\n");
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
		WalInfo("status_header formed %s\n", status_header);
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
		WalInfo("currentTime_header formed %s\n", currentTime_header);
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
			WalInfo("systemReadyTime_header formed %s\n", systemReadyTime_header);
			list = curl_slist_append(list, systemReadyTime_header);
			WAL_FREE(systemReadyTime_header);
		}
		WAL_FREE(systemReadyTime);
	}
	else
	{
		WalError("Failed to get systemReadyTime\n");
	}

	WalInfo("calculating transaction id\n");
	transaction_uuid = generate_trans_uuid();
	WalInfo("transaction_uuid is %s\n", transaction_uuid);
	if(transaction_uuid !=NULL)
	{
		uuid_header = (char *) malloc(sizeof(char)*MAX_BUF_SIZE);
		if(uuid_header !=NULL)
		{
			snprintf(uuid_header, MAX_BUF_SIZE, "Transaction-ID: %s", transaction_uuid);
			WalInfo("uuid_header formed %s\n", uuid_header);
			list = curl_slist_append(list, uuid_header);
			WAL_FREE(transaction_uuid);
			WAL_FREE(uuid_header);
		}
	}
	else
	{
		WalError("Failed to generate transaction_uuid\n");
	}
	WalInfo("uuid_header done\n");
	*header_list = list;
	WalInfo("End of createCurlheader\n");
}

static char* generate_trans_uuid()
{
	char *transID = NULL;
	uuid_t transaction_Id;
	char *trans_id = NULL;
	trans_id = (char *)malloc(37);
	uuid_generate_random(transaction_Id);
	uuid_unparse(transaction_Id, trans_id);

	if(trans_id !=NULL)
	{
		transID = trans_id;
	}
	WalInfo("returning transID:%s\n", transID);
	return transID;
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
            WalError ("File %s open error\n", name);
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
	WalInfo("output is %s. strlen(output):%lu\n", output, strlen(output));
	if (strlen(output)>0  && strcmp(output,"SUCCESS")==0)
	{
		//Call read script
		WalInfo("calling read script\n");
		execute_token_script(newToken,WEBPA_READ_HEADER,len,hw_mac,hw_serial_number);
	}
	else
	{
		WalError("Failed to create new token\n");
	}
}

/*
* Fetches authorization token from the output of read script. If read script returns "ERROR"
* it will call createNewAuthToken to create and read new token
*/

void getAuthToken()
{
	//local var to update webpa_auth_token only in success case
	char output[4069] = {'\0'} ;
	char *macID = NULL;
	char deviceMACValue[32] = { '\0' };
	char *serial_number=NULL;
	WalInfo("----------start of getAuthToken-------\n");
	memset (webpa_auth_token, 0, sizeof(webpa_auth_token));

	WalInfo("after memset of webpa_auth_token\n");
	if( strlen(WEBPA_READ_HEADER) !=0 && strlen(WEBPA_CREATE_HEADER) !=0)
	{
		macID = getParameterValue(DEVICE_MAC);
		if (macID != NULL)
		{
		    strncpy(deviceMACValue, macID, strlen(macID)+1);
		    macToLower(deviceMACValue, deviceMac);
		    WalInfo("deviceMAC: %s\n", deviceMac);
		    WAL_FREE(macID);
		}
		if( deviceMac != NULL && strlen(deviceMac) !=0 )
		{
			serial_number = getParameterValue(SERIAL_NUMBER);
			WalInfo("serial_number fetched: %s\n", serial_number);
			serialNum = strdup(serial_number);
			WalInfo("serialNum: %s\n", serialNum);
			WAL_FREE(serial_number);

			if( serialNum != NULL && strlen(serialNum) !=0 )
			{
				//set_global_hw_serial_number(hw_serial_number);
				execute_token_script(output, WEBPA_READ_HEADER, sizeof(output), deviceMac, serialNum);
				if ((strlen(output) == 0))
				{
					WalError("Unable to get auth token\n");
				}
				else if(strcmp(output,"ERROR")==0)
				{
					WalInfo("Failed to read token from %s. Proceeding to create new token.\n",WEBPA_READ_HEADER);
					//Call create/acquisition script
					createNewAuthToken(webpa_auth_token, sizeof(webpa_auth_token), deviceMac, serialNum );
				}
				else
				{
					WalInfo("update webpa_auth_token in success case\n");
					walStrncpy(webpa_auth_token, output, sizeof(webpa_auth_token));
				}
			}
			else
			{
				WalError("serialNum is NULL, failed to fetch auth token\n");
			}
		}
		else
		{
			WalError("deviceMAC is NULL, failed to fetch auth token\n");
		}
	}
	else
	{
		WalInfo("Both read and write file are NULL \n");
	}
}
