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
#include "webconfig_log.h"

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
/* Macros */
#define CURL_TIMEOUT_SEC	   25L
#define CLIENT_CERT_PATH  	   "/etc/clientcert.crt"
#define CA_CERT_PATH 		   "/etc/ssl/certs/ca-certificates.crt"
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
static char g_systemReadyTime[64]={'\0'};
static char g_interface[32]={'\0'};
char serialNum[64]={'\0'};
char webpa_auth_token[4096]={'\0'};
pthread_mutex_t device_mac_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t periodicsync_mutex=PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t periodicsync_condition=PTHREAD_COND_INITIALIZER;
static pthread_t NotificationThreadId=0;
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
static void macToLowerCase(char macValue[]);
static void loadInitURLFromFile(char **url);
void* processWebConfigNotification(void* pValue);
void Send_Notification_Task(char *url, long status_code, char *application_status, int application_details, char *previous_sync_time, char *version);
void free_notify_params_struct(notify_params_t *param);
char *replaceMacWord(const char *s, const char *macW, const char *deviceMACW);
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
	int r_count=0;
	long res_code;
	int index=0, i=0;
	int ret = 0;
	int json_status=-1;
	int retry_count=0, rv=0, rc=0;
        struct timeval tp;
        struct timespec ts;
        time_t t;
	int count=0;
	int wait_flag=0;
        int value =Get_PeriodicSyncCheckInterval();

	count = getConfigNumberOfEntries();
	WebConfigLog("count returned from getConfigNumberOfEntries:%d\n", count);

       while(1)
      {
	if(!wait_flag)
	{
        for(i = 0; i < count; i++)
        {
		index = getInstanceNumberAtIndex(i);
		WebConfigLog("index returned from getInstanceNumberAtIndex:%d\n", index);
		if(index != 0)
		{
			while(1)
			{
				//iterate through all entries in Device.X_RDK_WebConfig.ConfigFile.[i].URL to check if the current stored version of each configuration document matches the latest version on the cloud. 
				if(retry_count >3)
				{
					WebConfigLog("retry_count has reached max limit. Exiting.\n");
					retry_count=0;
					break;
				}
				configRet = requestWebConfigData(&webConfigData, r_count, index,(int)status, &res_code);
				if(configRet == 0)
				{
					rv = handleHttpResponse(res_code, webConfigData, retry_count, index);
					if(rv ==1)
					{
						WebcfgDebug("No retries are required. Exiting..\n");
						break;
					}
				}
				else
				{
					WebConfigLog("Failed to get webConfigData from cloud\n");
				}
				WebConfigLog("requestWebConfigData BACKOFF_SLEEP_DELAY_SEC is %d seconds\n", BACKOFF_SLEEP_DELAY_SEC);
				sleep(BACKOFF_SLEEP_DELAY_SEC);
				retry_count++;
				WebcfgDebug("Webconfig retry_count is %d\n", retry_count);
			}
		}
        }
      }

                 pthread_mutex_lock (&periodicsync_mutex);
                gettimeofday(&tp, NULL);
                ts.tv_sec = tp.tv_sec;
                ts.tv_nsec = tp.tv_usec * 1000;
                ts.tv_sec += value;

		if (g_shutdown)
		{
			WebConfigLog("g_shutdown is %d, proceeding to kill webconfig thread\n", g_shutdown);
			pthread_mutex_unlock (&periodicsync_mutex);
			break;
		}

                rv = pthread_cond_timedwait(&periodicsync_condition, &periodicsync_mutex, &ts);
		value=Get_PeriodicSyncCheckInterval();
                if(!rv && !g_shutdown)
		{
                        time(&t);
			BOOL ForceSyncEnable;
			getForceSyncCheck(1,&ForceSyncEnable); //TODO: Iterate through all indexes and check which index needs ForceSync
                        if(ForceSyncEnable)
                        {
                                wait_flag=0;
                                WebConfigLog("Recieved signal interput to getForceSyncCheck at %s\n",ctime(&t));
                                setForceSyncCheck(1,false);
                        }
                        else
                        {
                                wait_flag=1;
                                WebConfigLog("Recieved signal interput to change the sync interval to %d\n",value);
                        }
                }
                else if (rv == ETIMEDOUT && !g_shutdown)
                {
                        time(&t);
			wait_flag=0;
                        WebConfigLog("Periodic Sync Interval %d sec and syncing at %s\n",value,ctime(&t));
                }
		else if(g_shutdown)
		{
			WebConfigLog("Received signal interupt to RFC disable. g_shutdown is %d, proceeding to kill webconfig thread\n", g_shutdown);
			pthread_mutex_unlock (&periodicsync_mutex);
			break;
		}
			pthread_mutex_unlock(&periodicsync_mutex);

     }
	if(NotificationThreadId)
	{
		ret = pthread_join (NotificationThreadId, NULL);
		if(ret ==0)
		{
			WebConfigLog("pthread_join returns success\n");
		}
		else
		{
			WebConfigLog("Error joining thread\n");
		}
	}
	WebConfigLog("B4 pthread_exit\n");
	pthread_exit(0);
	WebcfgDebug("After pthread_exit\n");
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
		WebConfigLog("webConfig is in sync with cloud. response_code:%d\n", response_code); //do sync check OK
		setSyncCheckOK(index, TRUE);
		return 1;
	}
	else if(response_code == 200)
	{
		WebConfigLog("webConfig is not in sync with cloud. response_code:%d\n", response_code);

		if(webConfigData !=NULL)
		{
			WebcfgDebug("webConfigData fetched successfully\n");
			json_status = processJsonDocument(webConfigData, &setRet, &newDocVersion);
			WebConfigLog("setRet after process Json is %d\n", setRet);
			WebConfigLog("newDocVersion is %s\n", newDocVersion);

			//get common items for success and failure cases to send notification.

			getRet = getConfigURL(index, &configURL);
			if(getRet)
			{
				WebcfgDebug("After processJsonDocument: configURL is %s\n", configURL);
			}
			else
			{
				WebConfigLog("getConfigURL failed\n");
			}

			ret = setPreviousSyncDateTime(index);
			WebcfgDebug("setPreviousSyncDateTime ret is %d\n", ret);
			if(ret == 0)
			{
				WebcfgDebug("PreviousSyncDateTime set successfully for index %d\n", index);
			}
			else
			{
				WebConfigLog("Failed to set PreviousSyncDateTime for index %d\n", index);
			}
	
			if(json_status == 1)
			{
				WebcfgDebug("processJsonDocument success\n");
				if(configURL!=NULL && newDocVersion !=NULL)
				{
					WebConfigLog("Configuration settings from %s version %s were applied successfully\n", configURL, newDocVersion );
				}

				WebcfgDebug("set version and syncCheckOK for success\n");
				ret = setConfigVersion(index, newDocVersion);//get new version from json
				WebcfgDebug("setConfigVersion ret is %d\n", ret);
				if(ret == 0)
				{
					WebcfgDebug("Config Version %s set successfully for index %d\n", newDocVersion, index);
				}
				else
				{
					WebConfigLog("Failed to set Config version %s for index %d\n", newDocVersion, index);
				}

				ret = setSyncCheckOK(index, TRUE );
				WebcfgDebug("setSyncCheckOK ret is %d\n", ret);
				if(ret == 0)
				{
					WebcfgDebug("SyncCheckOK set successfully for index %d\n", index);
				}
				else
				{
					WebConfigLog("Failed to set SyncCheckOK for index %d\n", index);
				}

				getRet = getPreviousSyncDateTime(index, &PreviousSyncDateTime);
				WebcfgDebug("getPreviousSyncDateTime getRet is %d\n", getRet);
				if(getRet)
				{
					WebConfigLog("After processJsonDocument: PreviousSyncDateTime is %s for index %d\n", PreviousSyncDateTime, index);
				}
				else
				{
					WebConfigLog("PreviousSyncDateTime get failed for index %d\n", index);
				}

				WebcfgDebug("B4 Send_Notification_Task success case\n");
				Send_Notification_Task(configURL, response_code, "success", setRet, PreviousSyncDateTime , newDocVersion);
				WebcfgDebug("After Send_Notification_Task for success case\n");
				return 1;
			}
			else
			{
				WebConfigLog("Failure in processJsonDocument\n");
				ret = setSyncCheckOK(index, FALSE);
				WebcfgDebug("setSyncCheckOK ret is %d\n", ret);
				if(ret == 0)
				{
					WebcfgDebug("SyncCheckOK set to FALSE for index %d\n", index);
				}
				else
				{
					WebConfigLog("Failed to set SyncCheckOK to FALSE for index %d\n", index);
				}

				if(retry_count == 3)
				{
					WebcfgDebug("retry_count is %d\n", retry_count);
					getRet = getPreviousSyncDateTime(index, &PreviousSyncDateTime);
					WebcfgDebug("getPreviousSyncDateTime getRet is %d\n", getRet);
					if(getRet)
					{
						WebConfigLog("After processJsonDocument failure: PreviousSyncDateTime is %s for index %d\n", PreviousSyncDateTime, index);
					}
					else
					{
						WebConfigLog("PreviousSyncDateTime get failed for index %d\n", index);
					}
					WebConfigLog("Configuration settings from %s version %s FAILED\n", configURL, newDocVersion );
					WebConfigLog("Sending Failure Notification after 3 retry attempts\n");
					Send_Notification_Task(configURL, response_code, "failed", setRet, PreviousSyncDateTime , newDocVersion);
					WebcfgDebug("After Send_Notification_Task failure case\n");
				}
			}
		}
		else
		{
			WebConfigLog("webConfigData is empty, need to retry\n");
		}
	}
	else if(response_code == 204)
	{
		WebConfigLog("No configuration available for this device. response_code:%d\n", response_code);
		return 1;
	}
	else if(response_code == 403)
	{
		WebConfigLog("Token is expired, fetch new token. response_code:%d\n", response_code);
		createNewAuthToken(webpa_auth_token, sizeof(webpa_auth_token), get_global_deviceMAC(), serialNum );
		WebcfgDebug("createNewAuthToken done in 403 case\n");
	}
	else if(response_code == 429)
	{
		WebConfigLog("No action required from client. response_code:%d\n", response_code);
		return 1;
	}
	first_digit = (int)(response_code / pow(10, (int)log10(response_code)));
	if((response_code !=403) && (first_digit == 4)) //4xx
	{
		WebConfigLog("Action not supported. response_code:%d\n", response_code);
		return 1;
	}
	else //5xx & all other errors
	{
		WebConfigLog("Error code returned, need to retry. response_code:%d\n", response_code);
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
	int ret =0, count = 0;
	char *url = NULL;
	char c[] = "{mac}";

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
		createCurlheader(list, &headers_list, status, index);
		
		getConfigURL(index, &configURL);
		WebConfigLog("configURL fetched is %s\n", configURL);

		if(configURL !=NULL)
		{
			WebcfgDebug("forming webConfigURL\n");
			//Replace {mac} string from default init url with actual deviceMAC
			webConfigURL = replaceMacWord(configURL, c, get_global_deviceMAC());
			WebConfigLog("webConfigURL is %s\n", webConfigURL);
			// Store {mac} replaced/updated config URL to DB
			setConfigURL(index, webConfigURL);
			WebcfgDebug("setConfigURL done\n");
			curl_easy_setopt(curl, CURLOPT_URL, webConfigURL );
		}
		else
		{
			WebConfigLog("Failed to get configURL\n");
		}
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, CURL_TIMEOUT_SEC);
		WebcfgDebug("fetching interface from device.properties\n");
		if(strlen(g_interface) == 0)
		{
			get_webCfg_interface(&interface);
			if(interface !=NULL)
		        {
		               strncpy(g_interface, interface, sizeof(g_interface));
		               WebcfgDebug("g_interface copied is %s\n", g_interface);
		               WAL_FREE(interface);
		        }
		}
		WebConfigLog("g_interface fetched is %s\n", g_interface);
		if(strlen(g_interface) > 0)
		{
			WebcfgDebug("setting interface %s\n", g_interface);
			curl_easy_setopt(curl, CURLOPT_INTERFACE, g_interface);
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
		WAL_FREE(webConfigURL);
		if(res != 0)
		{
			WebConfigLog("curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
		}
		else
		{
                        WebcfgDebug("checking content type\n");
			content_res = curl_easy_getinfo(curl, CURLINFO_CONTENT_TYPE, &ct);
			if(!content_res && ct)
			{
				if(strcmp(ct, "application/json") !=0)
				{
					WebConfigLog("Invalid Content-Type\n");
				}
				else
				{
                                        if(response_code == 200)
                                        {
                                                *configData = strdup(data.data);
                                                WebConfigLog("configData received from cloud is %s\n", *configData);
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
	WebConfigLog("After parseJsonData version is %s\n", version);
	if(version!=NULL)
	{
		WebcfgDebug("copying to docVersion\n");
		*docVersion = strdup(version);
		WebcfgDebug("docVersion is %s\n", *docVersion);
		WAL_FREE(version);
	}
	if(parseStatus ==1)
	{
		WebcfgDebug("Request:> Type : %d\n",reqObj->reqType);
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
			WebcfgDebug("Assigning retStatus\n");
			*retStatus = ret;
			WebcfgDebug("*retStatus is %d\n", *retStatus);
                        if(ret == WDMP_SUCCESS)
                        {
                                WebConfigLog("setValues success. ret : %d\n", ret);
                                rv= 1;
                        }
                        else
                        {
                              WebConfigLog("setValues Failed. ret : %d\n", ret);
                        }
		}
		else
		{
			WebConfigLog("validate_parameter failed. parseStatus is %d\n", valid_ret);
		}
		if(NULL != reqObj)
		{
		        wdmp_free_req_struct(reqObj);
		}
	}
	else
	{
		WebConfigLog("parseJsonData failed. parseStatus is %d\n", parseStatus);
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
			WebConfigLog("WebConfig Parse error\n");
			return rv;
		}
		else
		{
			isValid = validateConfigFormat(json, &ETAG); //check eTAG value here :TODO
			WebConfigLog("ETAG is %s\n", ETAG);
			if(ETAG !=NULL)
			{
				*version = strdup(ETAG);
				WebcfgDebug("version copied from ETAG is %s\n", *version);
				WAL_FREE(ETAG);
			}
			if(!isValid)// testing purpose. make it to !isValid
			{
				WebConfigLog("validateConfigFormat failed\n");
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
				//version & eTag header validation is not done for phase 1 (Assuming both are matching)
				//if(strcmp(version, eTag) == 0) 
				//{
				WebcfgDebug("Copying version to eTag value\n"); //free eTag
					*eTag = strdup(version);
					WebcfgDebug("eTag is %s\n", *eTag);
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
				//}
				//else
				//{
				//	WebConfigLog("Invalid config json, version and ETAG are not same\n");
				//	return 0;
				//}
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
		WalPrint("interface fetched is %s\n", *interface);
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

	WebConfigLog("Start of createCurlheader\n");
	//Fetch auth JWT token from cloud.
	getAuthToken();
	WebcfgDebug("After getAuthToken\n");
	
	auth_header = (char *) malloc(sizeof(char)*MAX_HEADER_LEN);
	if(auth_header !=NULL)
	{
		WebcfgDebug("forming auth_header\n");
		snprintf(auth_header, MAX_HEADER_LEN, "Authorization:Bearer %s", (0 < strlen(webpa_auth_token) ? webpa_auth_token : NULL));
		list = curl_slist_append(list, auth_header);
		WAL_FREE(auth_header);
	}
	WebcfgDebug("B4 version header\n");
	version_header = (char *) malloc(sizeof(char)*MAX_BUF_SIZE);
	if(version_header !=NULL)
	{
		WebcfgDebug("calling getConfigVersion\n");
		getConfigVersion(index, &version);
		WebcfgDebug("createCurlheader version fetched is %s\n", version);
		snprintf(version_header, MAX_BUF_SIZE, "IF-NONE-MATCH:%s", ((NULL != version) ? version : "V1.0-NONE"));
		WebConfigLog("version_header formed %s\n", version_header);
		list = curl_slist_append(list, version_header);
		WAL_FREE(version_header);
	}

	schema_header = (char *) malloc(sizeof(char)*MAX_BUF_SIZE);
	if(schema_header !=NULL)
	{
		snprintf(schema_header, MAX_BUF_SIZE, "Schema-Version: %s", "v1.0");
		WebConfigLog("schema_header formed %s\n", schema_header);
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

        if(strlen(g_systemReadyTime) ==0)
        {
                systemReadyTime = get_global_systemReadyTime();
                if(systemReadyTime !=NULL)
                {
                       strncpy(g_systemReadyTime, systemReadyTime, sizeof(g_systemReadyTime));
                       WebcfgDebug("g_systemReadyTime fetched is %s\n", g_systemReadyTime);
                       WAL_FREE(systemReadyTime);
                }
        }

        if(strlen(g_systemReadyTime))
        {
                systemReadyTime_header = (char *) malloc(sizeof(char)*MAX_BUF_SIZE);
                if(systemReadyTime_header !=NULL)
                {
	                snprintf(systemReadyTime_header, MAX_BUF_SIZE, "X-System-Ready-Time: %s", g_systemReadyTime);
	                WebConfigLog("systemReadyTime_header formed %s\n", systemReadyTime_header);
	                list = curl_slist_append(list, systemReadyTime_header);
	                WAL_FREE(systemReadyTime_header);
                }
        }
        else
        {
                WebConfigLog("Failed to get systemReadyTime\n");
        }

	transaction_uuid = generate_trans_uuid();
	if(transaction_uuid !=NULL)
	{
		uuid_header = (char *) malloc(sizeof(char)*MAX_BUF_SIZE);
		if(uuid_header !=NULL)
		{
			snprintf(uuid_header, MAX_BUF_SIZE, "Transaction-ID: %s", transaction_uuid);
			WebConfigLog("uuid_header formed %s\n", uuid_header);
			list = curl_slist_append(list, uuid_header);
			WAL_FREE(transaction_uuid);
			WAL_FREE(uuid_header);
		}
	}
	else
	{
		WebConfigLog("Failed to generate transaction_uuid\n");
	}
	*header_list = list;
	WebcfgDebug("End of createCurlheader\n");
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
		WebcfgDebug("calling read script\n");
		execute_token_script(newToken,WEBPA_READ_HEADER,len,hw_mac,hw_serial_number);
		WebcfgDebug("after read script\n");
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

void getAuthToken()
{
	//local var to update webpa_auth_token only in success case
	char output[4069] = {'\0'} ;
	char *serial_number=NULL;
	memset (webpa_auth_token, 0, sizeof(webpa_auth_token));

	if( strlen(WEBPA_READ_HEADER) !=0 && strlen(WEBPA_CREATE_HEADER) !=0)
	{
                getDeviceMac();
                WebConfigLog("get_global_deviceMAC(). deviceMAC: %s\n",get_global_deviceMAC());

		if( get_global_deviceMAC() != NULL && strlen(get_global_deviceMAC()) !=0 )
		{
			serial_number = getParameterValue(SERIAL_NUMBER);
                        if(serial_number !=NULL)
                        {
			        strncpy(serialNum ,serial_number, sizeof(serialNum));
			        WebConfigLog("serialNum: %s\n", serialNum);
			        WAL_FREE(serial_number);
                        }

			if( serialNum != NULL && strlen(serialNum)>0 )
			{
				execute_token_script(output, WEBPA_READ_HEADER, sizeof(output), get_global_deviceMAC(), serialNum);
				if ((strlen(output) == 0))
				{
					WebConfigLog("Unable to get auth token\n");
				}
				else if(strcmp(output,"ERROR")==0)
				{
					WebConfigLog("Failed to read token from %s. Proceeding to create new token.\n",WEBPA_READ_HEADER);
					//Call create/acquisition script
					createNewAuthToken(webpa_auth_token, sizeof(webpa_auth_token), get_global_deviceMAC(), serialNum );
					WebcfgDebug("After createNewAuthToken\n");
				}
				else
				{
					WebcfgDebug("update webpa_auth_token in success case\n");
					walStrncpy(webpa_auth_token, output, sizeof(webpa_auth_token));
				}
			}
			else
			{
				WebConfigLog("serialNum is NULL, failed to fetch auth token\n");
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



void Send_Notification_Task(char *url, long status_code, char *application_status, int application_details, char *previous_sync_time, char *version)
{
	int err = 0;
	//pthread_t NotificationThreadId;
	notify_params_t *args = NULL;

	args = (notify_params_t *)malloc(sizeof(notify_params_t));

	if(args != NULL)
	{
		memset(args, 0, sizeof(notify_params_t));
		WebcfgDebug("Send_Notification_Task: start processing\n");
		if(url != NULL)
		{
			args->url = strdup(url);
			WebcfgDebug("args->url: %s\n", args->url);
		}
		args->status_code = status_code;
		WebcfgDebug("args->status_code: %d\n", args->status_code);
		if(application_status != NULL)
		{
			args->application_status = strdup(application_status);
			WebcfgDebug("args->application_status: %s\n", args->application_status);
		}
		args->application_details = application_details;
		WebcfgDebug("args->application_details: %d\n", args->application_details);
		if(previous_sync_time != NULL)
		{
			args->previous_sync_time = strdup(previous_sync_time);
			WebcfgDebug("args->previous_sync_time: %s\n", args->previous_sync_time);
		}
		if(version != NULL)
		{
			args->version = strdup(version);
			WebcfgDebug("args->version: %s\n", args->version);
		}
		WebcfgDebug("args values are printing\n");
		WebConfigLog("args->url:%s args->status_code:%d args->application_status:%s args->application_details:%d args->previous_sync_time:%s args->version:%s\n", args->url, args->status_code, args->application_status, args->application_details, args->previous_sync_time, args->version );
		WebcfgDebug("creating processWebConfigNotification thread\n");
		err = pthread_create(&NotificationThreadId, NULL, processWebConfigNotification, (void *) args);
		if (err != 0)
		{
			WebConfigLog("Error creating Webconfig Notification thread :[%s]\n", strerror(err));
		}
		else
		{
			WebConfigLog("Webconfig Notification thread created Successfully\n");
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

    //pthread_detach(pthread_self());
    if(pValue)
    {
		msg = (notify_params_t *) pValue;
    }
    if(strlen(get_global_deviceMAC()) == 0)
    {
		WebConfigLog("deviceMAC is NULL, failed to send Webconfig Notification\n");
    }
    else
    {
	snprintf(device_id, sizeof(device_id), "mac:%s", get_global_deviceMAC());
	WebcfgDebug("webconfig Device_id %s\n", device_id);

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
	WebcfgDebug("dest is %s\n", dest);

	if (stringifiedNotifyPayload != NULL && strlen(device_id) != 0)
        {
		source = (char*) malloc(sizeof(char) * sizeof(device_id));
		strncpy(source, device_id, sizeof(device_id));
		WebcfgDebug("source is %s\n", source);

		sendNotification(stringifiedNotifyPayload, source, dest);
	}
	WebConfigLog("After sendNotification\n");
	if(msg != NULL)
	{
		free_notify_params_struct(msg);
	}
   }
   return NULL;
}

void free_notify_params_struct(notify_params_t *param)
{
    WebcfgDebug("Inside free_notify_params_struct\n");
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
    WebcfgDebug("End of free_notify_params_struct\n");
}

char *replaceMacWord(const char *s, const char *macW, const char *deviceMACW)
{
	char *result;
	int i, cnt = 0;
	int deviceMACWlen = strlen(deviceMACW);
	int macWlen = strlen(macW);
	WebcfgDebug("Inside replaceMacWord\n");
	WebcfgDebug("deviceMACWlen:%d macWlen:%d\n", deviceMACWlen, macWlen);
	// Counting the number of times mac word occur in the string
	for (i = 0; s[i] != '\0'; i++)
	{
		if (strstr(&s[i], macW) == &s[i])
		{
		    cnt++;
		    // Jumping to index after the mac word.
		    i += macWlen - 1;
		}
	}

	result = (char *)malloc(i + cnt * (deviceMACWlen - macWlen) + 1);
	i = 0;
	while (*s)
	{
		if (strstr(s, macW) == s)
		{
			strcpy(&result[i], deviceMACW);
			i += deviceMACWlen;
			s += macWlen;
		}
		else
		    result[i++] = *s++;
	}
	result[i] = '\0';
	WebcfgDebug("End replaceMacWord. result is %s\n", result);
	return result;
}
