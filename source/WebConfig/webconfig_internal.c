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
#include "ansc_platform.h"
#include "ccsp_base_api.h"
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
#define MAX_HEADER_LEN			4096
#define MAX_PARAMETERNAME_LENGTH       512
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

typedef struct _notify_params
{
	char * url;
	long status_code;
	char * application_status;
	int application_details;
	char * previous_sync_time;
	char * version;
} notify_params_t;
/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
static char deviceMAC[32]={'\0'};
static char g_systemReadyTime[64]={'\0'};
static char g_interface[32]={'\0'};
//char *ETAG="NONE";
char serialNum[64]={'\0'};
int initURL=0;
char webpa_auth_token[4096]={'\0'};
pthread_mutex_t device_mac_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t periodicsync_mutex=PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t periodicsync_condition=PTHREAD_COND_INITIALIZER;
/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/
static void *WebConfigTask(void *status);
int processJsonDocument(char *jsonData, WDMP_STATUS *retStatus, char **docVersion);
int validateConfigFormat(cJSON *json, char **eTag);
int requestWebConfigData(char **configData, int r_count, int index, int status, long *code);
static void get_webCfg_interface(char **interface);
void createCurlheader(struct curl_slist *list, struct curl_slist **header_list, int status, int index);
int parseJsonData(char* jsonData, req_struct **req_obj, char **version);
size_t write_callback_fn(void *buffer, size_t size, size_t nmemb, struct token_data *data);
void getAuthToken();
void createNewAuthToken(char *newToken, size_t len, char *hw_mac, char* hw_serial_number);
int handleHttpResponse(long response_code, char *webConfigData, int retry_count, int index );
static char* generate_trans_uuid();
static void getDeviceMac();
static void macToLowerCase(char macValue[]);
static void loadInitURLFromFile(char **url);
void* processWebConfigNotification(void* pValue);
void Send_Notification_Task(char *url, long status_code, char *application_status, int application_details, char *previous_sync_time, char *version);
void free_notify_params_struct(notify_params_t *param);
/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/

pthread_cond_t *get_global_periodicsync_condition(void)
{
    return &periodicsync_condition;
}

pthread_mutex_t *get_global_periodicsync_mutex(void)
{
    return &periodicsync_mutex;
}

void initWebConfigTask(int status)
{
	int err = 0;
	pthread_t threadId;

	err = pthread_create(&threadId, NULL, WebConfigTask, (void *) status);
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
	int r_count=0;
	long res_code;
	int index=0, i=0;
	int json_status=-1;
	int retry_count=0, rv=0, rc=0;
        struct timeval tp;
        struct timespec ts;
        time_t t;
	int wait_flag=0;
        int value =Get_PeriodicSyncCheckInterval();
	//index = getConfigNumberOfEntries(); //:TODO local api implementation

       while(1)
      {
	if(!wait_flag)
	{
	while(1)
	{
		//iterate through all entries in Device.X_RDK_WebConfig.ConfigFile.[i].URL to check if the current stored version of each configuration document matches the latest version on the cloud. 
		//for(i = 0; i < index; i++)
		//{
			if(retry_count >3)
			{
				WalError("retry_count has reached max limit. Exiting.\n");
				break;
			}
			WalInfo("calling requestWebConfigData\n");
			configRet = requestWebConfigData(&webConfigData, r_count, index,(int)status, &res_code);
			WalInfo("requestWebConfigData done\n");
			//WAL_FREE(status);
			WalInfo("configRet is %d\n", configRet);
			if(configRet == 0)
			{
				rv = handleHttpResponse(res_code, webConfigData, retry_count, index);
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
			WalInfo("Webconfig retry_count is %d\n", retry_count);
		//}
	}
      }

                 pthread_mutex_lock (&periodicsync_mutex);
                gettimeofday(&tp, NULL);
                ts.tv_sec = tp.tv_sec;
                ts.tv_nsec = tp.tv_usec * 1000;
                ts.tv_sec += value;
                rv = pthread_cond_timedwait(&periodicsync_condition, &periodicsync_mutex, &ts);
		value=Get_PeriodicSyncCheckInterval();
                if(!rv)
                {
                        time(&t);
			wait_flag=1;
                        WalInfo("Recieved signal interput to change the sync interval to %d\n",value);
                }
                else if (rv == ETIMEDOUT)
                {
                        time(&t);
			wait_flag=0;
                        WalInfo("Periodic Sync Interval %d sec and syncing at %s\n",value,ctime(&t));
                }
			pthread_mutex_unlock(&periodicsync_mutex);

     }
	return NULL;
}

int handleHttpResponse(long response_code, char *webConfigData, int retry_count, int index)
{
	int first_digit=0;
	int json_status=0;
	WDMP_STATUS setRet = WDMP_FAILURE;
	int ret=0, getRet = 0;
	char *configURL = NULL;
	char *version = NULL;
	char *PreviousSyncDateTime=NULL;
	char *newDocVersion = NULL;

	if(response_code == 304)
	{
		WalInfo("webConfig is in sync with cloud. response_code:%d\n", response_code); //do sync check OK
		setSyncCheckOK(1, TRUE); //TODO: local api implementation
		return 1;
	}
	else if(response_code == 200)
	{
		WalInfo("webConfig is not in sync with cloud. response_code:%d\n", response_code);

		if(webConfigData !=NULL)
		{
			WalInfo("webConfigData fetched successfully\n");
			json_status = processJsonDocument(webConfigData, &setRet, &newDocVersion);
			WalInfo("setRet after process Json is %d\n", setRet);
			WalInfo("newDocVersion is %s\n", newDocVersion);

			//get common items for success and failure cases to send notification.

			//for loop with i 0 to index value for future cases when more config entries.

			getRet = getConfigURL(1, &configURL);
			WalInfo("getConfigURL .. getRet is %d\n", getRet);
			if(getRet)
			{
				WalInfo("After processJsonDocument: configURL is %s\n", configURL);
			}
			else
			{
				WalError("getConfigURL failed\n");
			}

			/*getRet = getConfigVersion(1, &version);
			WalInfo("getConfigVersion . getRet is %d\n", getRet);
			if(getRet)
			{
				WalInfo("After processJsonDocument: version is %s\n", version);
			}
			else
			{
				WalError("getConfigVersion failed\n");
			}*/

			ret = setPreviousSyncDateTime(1);
			WalInfo("setPreviousSyncDateTime ret is %d\n", ret);
			if(ret == 0)
			{
				WalInfo("PreviousSyncDateTime set successfully for index 1\n");
			}
			else
			{
				WalError("Failed to set PreviousSyncDateTime for index 1\n");
			}
	
			if(json_status == 1)
			{
				WalInfo("processJsonDocument success\n");
				if(configURL!=NULL && newDocVersion !=NULL)
				{
					WalInfo("Configuration settings from %s version %s were applied successfully\n", configURL, newDocVersion );
				}

				WalInfo("set version and syncCheckOK for success\n");
				ret = setConfigVersion(1, newDocVersion);//get new version from json
				WalInfo("setConfigVersion ret is %d\n", ret);
				if(ret == 0)
				{
					WalInfo("Config Version %s set successfully for index 1\n", newDocVersion);
				}
				else
				{
					WalError("Failed to set Config version %s for index 1\n", newDocVersion);
				}

				ret = setSyncCheckOK(1, TRUE );
				WalInfo("setSyncCheckOK ret is %d\n", ret);
				if(ret == 0)
				{
					WalInfo("SyncCheckOK set successfully for index 1\n");
				}
				else
				{
					WalError("Failed to set SyncCheckOK for index 1\n");
				}

				getRet = getPreviousSyncDateTime(1, &PreviousSyncDateTime);
				WalInfo("getPreviousSyncDateTime getRet is %d\n", getRet);
				if(getRet)
				{
					WalInfo("After processJsonDocument: PreviousSyncDateTime is %s\n", PreviousSyncDateTime);
				}
				else
				{
					WalError("PreviousSyncDateTime get failed\n");
				}

				WalInfo("B4 Send_Notification_Task success case\n");
				Send_Notification_Task(configURL, response_code, "success", setRet, PreviousSyncDateTime , newDocVersion);
				WalInfo("After Send_Notification_Task for success case\n");
				return 1;
			}
			else
			{
				WalError("Failure in processJsonDocument\n");
				ret = setSyncCheckOK(1, FALSE); //TODO: local api implementation
				WalInfo("setSyncCheckOK ret is %d\n", ret);
				if(ret == 0)
				{
					WalInfo("SyncCheckOK set to FALSE successfully for index 1\n");
				}
				else
				{
					WalError("Failed to set SyncCheckOK to FALSE for 1\n");
				}

				if(retry_count == 3)
				{
					WalInfo("retry_count is %d\n", retry_count);
					getRet = getPreviousSyncDateTime(1, &PreviousSyncDateTime);
					WalInfo("getPreviousSyncDateTime getRet is %d\n", getRet);
					if(getRet)
					{
						WalInfo("After processJsonDocument failure: PreviousSyncDateTime is %s\n", PreviousSyncDateTime);
					}
					else
					{
						WalError("PreviousSyncDateTime get failed\n");
					}

					WalError("Sending Failure Notification after 3 retry attempts\n");
					Send_Notification_Task(configURL, response_code, "failed", setRet, PreviousSyncDateTime , newDocVersion);
					WalInfo("After Send_Notification_Task failure case\n");
				}
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
		createNewAuthToken(webpa_auth_token, sizeof(webpa_auth_token), deviceMAC, serialNum );
		WalInfo("createNewAuthToken done in 403 case\n");
	}
	else if(response_code == 429)
	{
		WalInfo("No action required from client. response_code:%d\n", response_code);
		return 1;
	}
	first_digit = (int)(response_code / pow(10, (int)log10(response_code)));
	if((response_code !=403) && (first_digit == 4)) //4xx
	{
		WalError("Action not supported. response_code:%d\n", response_code);
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
	int rv=1;
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
	char * configURL = NULL;
	int ret =0;
	char *url = NULL;

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
		createCurlheader(list, &headers_list, status, index);
		if(initURL==0)
		{
			WalInfo("loadInitURLFromFile first case\n");
			loadInitURLFromFile(&url);
			if(url !=NULL)
			{
				WalInfo("Init url fetched from device.properties file is %s\n", url);
				configURL = (char *) malloc(sizeof(char)*MAX_BUF_SIZE); //free this.
				if(configURL !=NULL)
				{
					snprintf(configURL, MAX_BUF_SIZE, url, deviceMAC);
					WalInfo("configURL is %s\n", configURL);
					ret = setConfigURL(1, configURL); //TODO: local api implementation
					if(ret == 0)
					{
						WalInfo("setConfigURL done\n");
					}
					initURL=1;
				}
			}
		}
		else
		{
			WalInfo("getConfigURL second case\n");
			getConfigURL(1, &configURL); //TODO: local api implementation
			WalInfo("configURL fetched is %s\n", configURL);
		}
		webConfigURL = (char *) malloc(sizeof(char)*MAX_BUF_SIZE);
		if(configURL !=NULL)
		{
			WalInfo("forming webConfigURL\n");
			snprintf(webConfigURL, MAX_BUF_SIZE, configURL , deviceMAC);
			WalInfo("webConfigURL is %s\n", webConfigURL);
			//webConfigURL = getParameterValue(URL_param, &paramType);
			curl_easy_setopt(curl, CURLOPT_URL, webConfigURL );
			WalInfo("free ing configURL\n");
			WAL_FREE(configURL);
			WalInfo("free done for configURL\n");
		}
		else
		{
			WalError("Failed to get configURL\n");
		}
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, CURL_TIMEOUT_SEC);
		WalInfo("fetching interface from device.properties\n");
		if(strlen(g_interface) == 0)
		{
			get_webCfg_interface(&interface);
			if(interface !=NULL)
		        {
		               strncpy(g_interface, interface, sizeof(g_interface));
		               WalInfo("g_interface copied is %s\n", g_interface);
		               WAL_FREE(interface);
		        }
		}
		WalInfo("g_interface fetched is %s\n", g_interface);
		if(strlen(g_interface) > 0)
		{
			WalInfo("setting interface %s\n", g_interface);
			curl_easy_setopt(curl, CURLOPT_INTERFACE, g_interface);
		}

		// set callback for writing received data
		WalInfo("setting CURLOPT_WRITEFUNCTION\n");
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
		WalInfo("webConfig curl response %d http_code %d\n", res, response_code);
		*code = response_code;
		time_res = curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME, &total);
		if(time_res == 0)
		{
			WalInfo("curl response Time: %.1f seconds\n", total);
		}
		WalInfo("free headers_list\n");
		curl_slist_free_all(headers_list);
		//WalInfo("free URL_param\n");
		//WAL_FREE(URL_param);
		WalInfo("free webConfigURL\n");
		WAL_FREE(webConfigURL);
		WalInfo("free done\n");
		if(res != 0)
		{
			WalError("curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
		}
		else
		{
                        WalInfo("checking content type\n");
			content_res = curl_easy_getinfo(curl, CURLINFO_CONTENT_TYPE, &ct);
			if(!content_res && ct)
			{
				if(strcmp(ct, "application/json") !=0)
				{
					WalError("Invalid Content-Type\n");
				}
				else
				{
                                        if(response_code == 200)
                                        {
                                                *configData = strdup(data.data);
                                                WalInfo("configData received from cloud is %s\n", *configData);
                                        }
                                }
			}
		}
		WAL_FREE(data.data);
		curl_easy_cleanup(curl);
		rv=0;
	}
	else
	{
		WalError("curl init failure\n");
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
        WalError("Failed to allocate memory for data\n");
        return 0;
    }

    memcpy((data->data + index), buffer, n);
    data->data[data->size] = '\0';

    return size * nmemb;
}

int processJsonDocument(char *jsonData, WDMP_STATUS *retStatus, char **docVersion)
{
	cJSON *paramArray = NULL;
	int parseStatus = 0;
	int i=0, rv=0;
	req_struct *reqObj;
	int paramCount =0;
	WDMP_STATUS valid_ret = WDMP_FAILURE;
	WDMP_STATUS ret = WDMP_FAILURE;
	char *version = NULL;

	parseStatus = parseJsonData(jsonData, &reqObj, &version);
	WalInfo("After parseJsonData version is %s\n", version);
	if(version!=NULL)
	{
		WalInfo("copying to docVersion\n");
		*docVersion = strdup(version);
		WalInfo("docVersion is %s\n", *docVersion);
		WAL_FREE(version);
		WalInfo("free for version done\n");
	}
	WalInfo("checking parseStatus\n");
	if(parseStatus ==1)
	{
		WalInfo("Request:> Type : %d\n",reqObj->reqType);
		WalInfo("Request:> ParamCount = %zu\n",reqObj->u.setReq->paramCnt);
		paramCount = (int)reqObj->u.setReq->paramCnt;
		for (i = 0; i < paramCount; i++) 
		{
		        WalInfo("Request:> param[%d].name = %s\n",i,reqObj->u.setReq->param[i].name);
		        WalInfo("Request:> param[%d].value = %s\n",i,reqObj->u.setReq->param[i].value);
		        WalInfo("Request:> param[%d].type = %d\n",i,reqObj->u.setReq->param[i].type);
		}

		valid_ret = validate_parameter(reqObj->u.setReq->param, paramCount, reqObj->reqType);

		if(valid_ret == WDMP_SUCCESS)
		{
			setValues(reqObj->u.setReq->param, paramCount, WEBPA_SET, NULL, NULL, &ret);
			WalInfo("Assigning retStatus\n");
			*retStatus = ret;
			WalInfo("*retStatus is %d\n", *retStatus);
                        if(ret == WDMP_SUCCESS)
                        {
                                WalInfo("setValues success. ret : %d\n", ret);
                                rv= 1;
                        }
                        else
                        {
                              WalError("setValues Failed. ret : %d\n", ret);
                        }
		}
		else
		{
			WalError("validate_parameter failed. parseStatus is %d\n", valid_ret);
		}
		if(NULL != reqObj)
		{
			WalInfo("free for reqObj\n");
		        wdmp_free_req_struct(reqObj);
			WalInfo("free for reqObj done.\n");
		}
		WalInfo("checked for reqObj free\n");
	}
	else
	{
		WalError("parseJsonData failed. parseStatus is %d\n", parseStatus);
	}
	return rv;
}

int parseJsonData(char* jsonData, req_struct **req_obj, char **version)
{
	cJSON *json = NULL;
	cJSON *paramArray = NULL;
	int i=0, isValid =0;
	int rv =-1;
	req_struct *reqObj = NULL;
	int paramCount=0;
	WDMP_STATUS ret = WDMP_FAILURE, valid_ret = WDMP_FAILURE;
	int itemSize=0;
	char *ETAG= NULL;

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
			isValid = validateConfigFormat(json, &ETAG); //check eTAG value here :TODO
			WalInfo("ETAG is %s\n", ETAG);
			if(ETAG !=NULL)
			{
				*version = strdup(ETAG);
				WalInfo("version copied from ETAG is %s\n", *version);
				WAL_FREE(ETAG);
				WalInfo("After ETAG free\n");
			}
			if(!isValid)// testing purpose. make it to !isValid
			{
				WalError("validateConfigFormat failed\n");
				return rv;
			}
			(reqObj) = (req_struct *) malloc(sizeof(req_struct));
                	memset((reqObj), 0, sizeof(req_struct));

			parse_set_request(json, &reqObj, WDMP_TR181);
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

int validateConfigFormat(cJSON *json, char **eTag)
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
				//version & eTag validation is not doing for phase 1 (Assuming both are matching)
				//if(strcmp(version, eTag) == 0) 
				//{
				WalInfo("Copying version to eTag value\n"); //free eTag
					*eTag = strdup(version);
					WalInfo("eTag is %s\n", *eTag);
					//WAL_FREE(version);
					//WalInfo("free version done\n");
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
				//}
				//else
				//{
				//	WalError("Invalid config json, version and ETAG are not same\n");
				//	return 0;
				//}
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
//loadInitURLFromFile
static void loadInitURLFromFile(char **url)
{
	FILE *fp = fopen(DEVICE_PROPS_FILE, "r");

	if (NULL != fp)
	{
		char str[255] = {'\0'};
		while(fscanf(fp,"%s", str) != EOF)
		{
		    char *value = NULL;

		    if(NULL != (value = strstr(str, "WEBCONFIG_INIT_URL=")))
		    {
			value = value + strlen("WEBCONFIG_INIT_URL=");
			*url = strdup(value);
		    }

		}
		fclose(fp);
	}
	else
	{
		WalError("Failed to open device.properties file:%s\n", DEVICE_PROPS_FILE);
	}

	if (NULL == *url)
	{
		WalError("WebConfig url is not present in device.properties\n");
	}
	else
	{
		WalInfo("url fetched is %s\n", *url);
	}
}

/* @brief Function to create curl header options
 * @param[in] list temp curl header list
 * @param[in] device status value
 * @param[out] header_list output curl header list
*/
void createCurlheader( struct curl_slist *list, struct curl_slist **header_list, int status, int index)
{
	char *version_header = NULL;
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
	char *version = NULL;

	WalInfo("Start of createCurlheader\n");
	//Fetch auth JWT token from cloud.
	getAuthToken();

	auth_header = (char *) malloc(sizeof(char)*MAX_HEADER_LEN);
	if(auth_header !=NULL)
	{
		snprintf(auth_header, MAX_HEADER_LEN, "Authorization:Bearer %s", (0 < strlen(webpa_auth_token) ? webpa_auth_token : NULL));
		list = curl_slist_append(list, auth_header);
		WAL_FREE(auth_header);
	}

	version_header = (char *) malloc(sizeof(char)*MAX_BUF_SIZE);
	if(version_header !=NULL)
	{
		//snprintf(version_header, MAX_BUF_SIZE, "IF-NONE-MATCH:%s", ETAG);
		//WalInfo("calling getConfigVersion for index %d\n", index);
		getConfigVersion(1, &version); //TODO: local api implementation
		WalInfo("createCurlheader version fetched is %s\n", version);
		snprintf(version_header, MAX_BUF_SIZE, "IF-NONE-MATCH:%s", ((NULL != version) ? version : "V1.0-NONE"));
		WalInfo("version_header formed %s\n", version_header);
		list = curl_slist_append(list, version_header);
		WAL_FREE(version_header);
	}

	schema_header = (char *) malloc(sizeof(char)*MAX_BUF_SIZE);
	if(schema_header !=NULL)
	{
		snprintf(schema_header, MAX_BUF_SIZE, "Schema-Version: %s", "v1.0");
		WalInfo("schema_header formed %s\n", schema_header);
		list = curl_slist_append(list, schema_header);
		WAL_FREE(schema_header);
	}
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

        if(strlen(g_systemReadyTime) ==0)
        {
                systemReadyTime = get_global_systemReadyTime();
                if(systemReadyTime !=NULL)
                {
                       strncpy(g_systemReadyTime, systemReadyTime, sizeof(g_systemReadyTime));
                       WalInfo("g_systemReadyTime fetched is %s\n", g_systemReadyTime);
                       WAL_FREE(systemReadyTime);
                }
        }

        if(strlen(g_systemReadyTime))
        {
                systemReadyTime_header = (char *) malloc(sizeof(char)*MAX_BUF_SIZE);
                if(systemReadyTime_header !=NULL)
                {
	                snprintf(systemReadyTime_header, MAX_BUF_SIZE, "X-System-Ready-Time: %s", g_systemReadyTime);
	                WalInfo("systemReadyTime_header formed %s\n", systemReadyTime_header);
	                list = curl_slist_append(list, systemReadyTime_header);
	                WAL_FREE(systemReadyTime_header);
                }
        }
        else
        {
                WalError("Failed to get systemReadyTime\n");
        }

	transaction_uuid = generate_trans_uuid();
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
	char *serial_number=NULL;
	memset (webpa_auth_token, 0, sizeof(webpa_auth_token));

	if( strlen(WEBPA_READ_HEADER) !=0 && strlen(WEBPA_CREATE_HEADER) !=0)
	{
                getDeviceMac();
                WalInfo("deviceMAC: %s\n",deviceMAC);

		if( deviceMAC != NULL && strlen(deviceMAC) !=0 )
		{
			serial_number = getParameterValue(SERIAL_NUMBER);
                        if(serial_number !=NULL)
                        {
			        strncpy(serialNum ,serial_number, sizeof(serialNum));
			        WalInfo("serialNum: %s\n", serialNum);
			        WAL_FREE(serial_number);
                        }

			if( serialNum != NULL && strlen(serialNum)>0 )
			{
				execute_token_script(output, WEBPA_READ_HEADER, sizeof(output), deviceMAC, serialNum);
				if ((strlen(output) == 0))
				{
					WalError("Unable to get auth token\n");
				}
				else if(strcmp(output,"ERROR")==0)
				{
					WalInfo("Failed to read token from %s. Proceeding to create new token.\n",WEBPA_READ_HEADER);
					//Call create/acquisition script
					createNewAuthToken(webpa_auth_token, sizeof(webpa_auth_token), deviceMAC, serialNum );
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


static void getDeviceMac()
{
    int retryCount = 0;

    while(!strlen(deviceMAC))
    {
	pthread_mutex_lock(&device_mac_mutex);

        int ret = -1, size =0, val_size =0,cnt =0;
        char compName[MAX_PARAMETERNAME_LENGTH/2] = { '\0' };
        char dbusPath[MAX_PARAMETERNAME_LENGTH/2] = { '\0' };
        parameterValStruct_t **parameterval = NULL;
        char *getList[] = {DEVICE_MAC};
        componentStruct_t **        ppComponents = NULL;
        char dst_pathname_cr[256] = {0};

	if (strlen(deviceMAC))
	{
	        pthread_mutex_unlock(&device_mac_mutex);
	        break;
	}
        sprintf(dst_pathname_cr, "%s%s", "eRT.", CCSP_DBUS_INTERFACE_CR);
        ret = CcspBaseIf_discComponentSupportingNamespace(bus_handle, dst_pathname_cr, DEVICE_MAC, "", &ppComponents, &size);
        if ( ret == CCSP_SUCCESS && size >= 1)
        {
                strncpy(compName, ppComponents[0]->componentName, sizeof(compName)-1);
                strncpy(dbusPath, ppComponents[0]->dbusPath, sizeof(compName)-1);
        }
        else
        {
                WalError("Failed to get component for %s ret: %d\n",DEVICE_MAC,ret);
                retryCount++;
        }
        free_componentStruct_t(bus_handle, size, ppComponents);
        if(strlen(compName) != 0 && strlen(dbusPath) != 0)
        {
                ret = CcspBaseIf_getParameterValues(bus_handle,
                        compName, dbusPath,
                        getList,
                        1, &val_size, &parameterval);
                if(ret == CCSP_SUCCESS)
                {
                    for (cnt = 0; cnt < val_size; cnt++)
                    {
                        WalInfo("parameterval[%d]->parameterName : %s\n",cnt,parameterval[cnt]->parameterName);
                        WalInfo("parameterval[%d]->parameterValue : %s\n",cnt,parameterval[cnt]->parameterValue);
                        WalInfo("parameterval[%d]->type :%d\n",cnt,parameterval[cnt]->type);
                    }
                    macToLowerCase(parameterval[0]->parameterValue);
                    retryCount = 0;
                }
                else
                {
                        WalError("Failed to get values for %s ret: %d\n",getList[0],ret);
                        retryCount++;
                }
                free_parameterValStruct_t(bus_handle, val_size, parameterval);
        }
        if(retryCount == 0)
        {
                WalInfo("deviceMAC is %s\n",deviceMAC);
                pthread_mutex_unlock(&device_mac_mutex);
                break;
        }
        else
        {
                if(retryCount > 5 )
                {
                        WalError("Unable to get CM Mac after %d retry attempts..\n", retryCount);
                        pthread_mutex_unlock(&device_mac_mutex);
                        break;
                }
                else
                {
                        WalError("Failed to GetValue for MAC. Retrying...retryCount %d\n", retryCount);
                        pthread_mutex_unlock(&device_mac_mutex);
                        sleep(10);
                }
        }
    }
}

static void macToLowerCase(char macValue[])
{
    int i = 0;
    int j;
    char *token[32]={'\0'};
    char tmp[32]={'\0'};
    strncpy(tmp, macValue,sizeof(tmp)-1);
    token[i] = strtok(tmp, ":");
    if(token[i]!=NULL)
    {
        strncpy(deviceMAC, token[i],sizeof(deviceMAC)-1);
        deviceMAC[31]='\0';
        i++;
    }
    while ((token[i] = strtok(NULL, ":")) != NULL)
    {
        strncat(deviceMAC, token[i],sizeof(deviceMAC)-1);
        deviceMAC[31]='\0';
        i++;
    }
    deviceMAC[31]='\0';
    for(j = 0; deviceMAC[j]; j++)
    {
        deviceMAC[j] = tolower(deviceMAC[j]);
    }
}

void Send_Notification_Task(char *url, long status_code, char *application_status, int application_details, char *previous_sync_time, char *version)
{
	int err = 0;
	pthread_t NotificationThreadId;
	notify_params_t *args = NULL;

	args = (notify_params_t *)malloc(sizeof(notify_params_t));

	if(args != NULL)
	{
		memset(args, 0, sizeof(notify_params_t));
		WalInfo("Send_Notification_Task: start processing\n");
		if(url != NULL)
		{
			args->url = strdup(url);
			WalInfo("args->url: %s\n", args->url);
		}
		args->status_code = status_code;
		WalInfo("args->status_code: %d\n", args->status_code);
		if(application_status != NULL)
		{
			args->application_status = strdup(application_status);
			WalInfo("args->application_status: %s\n", args->application_status);
		}
		args->application_details = application_details;
		WalInfo("args->application_details: %d\n", args->application_details);
		if(previous_sync_time != NULL)
		{
			args->previous_sync_time = strdup(previous_sync_time);
			WalInfo("args->previous_sync_time: %s\n", args->previous_sync_time);
		}
		if(version != NULL)
		{
			args->version = strdup(version);
			WalInfo("args->version: %s\n", args->version);
		}
		WalInfo("args values are printing\n");
		WalInfo("args->url:%s args->status_code:%d args->application_status:%s args->application_details:%d args->previous_sync_time:%s args->version:%s\n", args->url, args->status_code, args->application_status, args->application_details, args->previous_sync_time, args->version );
		WalInfo("creating processWebConfigNotification thread\n");
		err = pthread_create(&NotificationThreadId, NULL, processWebConfigNotification, (void *) args);
		if (err != 0)
		{
			WalError("Error creating Webconfig Notification thread :[%s]\n", strerror(err));
		}
		else
		{
			WalInfo("Webconfig Notification thread created Successfully\n");
		}
	}
}

void* processWebConfigNotification(void* pValue)
{
    char device_id[32] = { '\0' };
    cJSON *notifyPayload = cJSON_CreateObject();
    char  * stringifiedNotifyPayload;
    notify_params_t *msg;
    char dest[512] = {'\0'};
    char *source = NULL;
    cJSON * reports, *one_report;

    pthread_detach(pthread_self());
    if(pValue)
    {
		msg = (notify_params_t *) pValue;
    }
    if(strlen(deviceMAC) == 0)
    {
		WalError("deviceMAC is NULL, failed to send Webconfig Notification\n");
    }
    else
    {
	snprintf(device_id, sizeof(device_id), "mac:%s", deviceMAC);
	WalInfo("webconfig Device_id %s\n", device_id);

	if(notifyPayload != NULL)
	{
		cJSON_AddStringToObject(notifyPayload,"device_id", device_id);

		if(msg)
		{
			cJSON_AddItemToObject(notifyPayload, "reports", reports = cJSON_CreateArray());
			cJSON_AddItemToArray(reports, one_report = cJSON_CreateObject());
			cJSON_AddStringToObject(one_report, "url", (NULL != msg->url) ? msg->url : "unknown");
			cJSON_AddNumberToObject(one_report,"http_status_code", msg->status_code);
			cJSON_AddStringToObject(one_report,"document_application_status", (NULL != msg->application_status) ? msg->application_status : "unknown");
			cJSON_AddNumberToObject(one_report,"document_application_details", msg->application_details);
			cJSON_AddNumberToObject(one_report, "previous_sync_time", (NULL != msg->previous_sync_time) ? atoi(msg->previous_sync_time) : 0);
			cJSON_AddStringToObject(one_report,"version", (NULL != msg->version) ? msg->version : "V1.0-NONE");
		}
		stringifiedNotifyPayload = cJSON_PrintUnformatted(notifyPayload);
		cJSON_Delete(notifyPayload);
	}

	snprintf(dest,sizeof(dest),"event:config-version-report/%s",device_id);
	WalInfo("dest is %s\n", dest);

	if (stringifiedNotifyPayload != NULL && strlen(device_id) != 0)
        {
		source = (char*) malloc(sizeof(char) * sizeof(device_id));
		strncpy(source, device_id, sizeof(device_id));
		WalInfo("source is %s\n", source);

		sendNotification(stringifiedNotifyPayload, source, dest);
	}
	WalInfo("After sendNotification\n");
	if(msg != NULL)
	{
		free_notify_params_struct(msg);
	}
   }
}

void free_notify_params_struct(notify_params_t *param)
{
    WalInfo("Inside free_notify_params_struct\n");
    if(param != NULL)
    {
        if(param->url != NULL)
        {
            WAL_FREE(param->url);
        }
        if(param->application_status != NULL)
        {
            WAL_FREE(param->application_status);
        }
        if(param->previous_sync_time != NULL)
        {
            WAL_FREE(param->previous_sync_time);
        }
        if(param->version != NULL)
        {
            WAL_FREE(param->version);
        }
        WAL_FREE(param);
    }
    WalInfo("End of free_notify_params_struct\n");
}
