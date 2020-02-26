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
#include "cosa_webconfig_apis.h"
#include <webcfg_generic.h>
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
#define ETAG_HEADER 		    "ETag:"
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
	char * request_timestamp;
	char * version;
	char * transaction_uuid;
} notify_params_t;
/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
static char g_systemReadyTime[64]={'\0'};
static char g_interface[32]={'\0'};
static char serialNum[64]={'\0'};
static char g_FirmwareVersion[64]={'\0'};
static char g_bootTime[64]={'\0'};
char webpa_auth_token[4096]={'\0'};
static char g_ETAG[64]={'\0'};
pthread_mutex_t periodicsync_mutex=PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t periodicsync_condition=PTHREAD_COND_INITIALIZER;
pthread_mutex_t notify_mut=PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t notify_con=PTHREAD_COND_INITIALIZER;
static pthread_t NotificationThreadId=0;
notify_params_t *notifyMsg = NULL;
/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/
static void *WebConfigTask(void *status);
int processJsonDocument(char *jsonData, int *retStatus, char **docVersion);
int validateConfigFormat(cJSON *json, char **eTag);
int requestWebConfigData(char **configData, int r_count, int index, int status, long *code,char **transaction_id);
static void get_webCfg_interface(char **interface);
void createCurlheader(struct curl_slist *list, struct curl_slist **header_list, int status, int index, char ** trans_uuid);
int parseJsonData(char* jsonData, req_struct **req_obj, char **version);
size_t write_callback_fn(void *buffer, size_t size, size_t nmemb, struct token_data *data);
void getAuthToken();
void createNewAuthToken(char *newToken, size_t len, char *hw_mac, char* hw_serial_number);
int handleHttpResponse(long response_code, char *webConfigData, int retry_count, int index,char* transaction_uuid);
static char* generate_trans_uuid();
static void macToLowerCase(char macValue[]);
static void loadInitURLFromFile(char **url);
void* processWebConfigNotification();
void addWebConfigNotifyMsg(char *url, long status_code, char *application_status, int application_details, char *request_timestamp, char *version, char *transaction_uuid);
void free_notify_params_struct(notify_params_t *param);
char *replaceMacWord(const char *s, const char *macW, const char *deviceMACW);
void processWebconfigSync(int index, int status);
static void initWebConfigNotifyTask();
size_t header_callback(char *buffer, size_t size, size_t nitems);
void stripSpaces(char *str, char **final_str);
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
	int index=0, i=0;
	int ret = 0;
	int json_status=-1;
	int rv=0, rc=0;
        struct timeval tp;
        struct timespec ts;
        time_t t;
	int count=0;
	int wait_flag=0;
	int forced_sync=0, syncIndex = 0;
        int value =Get_PeriodicSyncCheckInterval();

	//start webconfig notification thread.
	initWebConfigNotifyTask();

	while(1)
	{
		if(forced_sync)
		{
			//trigger sync only for particular index
			WebConfigLog("Trigger Forced sync for index %d\n", syncIndex);
			processWebconfigSync(syncIndex, (int)status);
			WebConfigLog("reset forced_sync and syncIndex after sync\n");
			forced_sync = 0;
			syncIndex = 0;
			WebConfigLog("reset ForceSyncCheck after sync\n");
			setForceSyncCheck(index, false, "", 0);
		}
		else if(!wait_flag)
		{
			//iterate through all entries in Device.X_RDK_WebConfig.ConfigFile.[i].URL to check if the current stored version of each configuration document matches the latest version on the cloud.

			count = getConfigNumberOfEntries();
			WebConfigLog("count returned from getConfigNumberOfEntries:%d\n", count);

			for(i = 0; i < count; i++)
			{
				index = getInstanceNumberAtIndex(i);
				WebConfigLog("index returned from getInstanceNumberAtIndex:%d\n", index);
				if(index != 0)
				{
					processWebconfigSync(index, (int)status);
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
		value=Get_PeriodicSyncCheckInterval();
		WebConfigLog("PeriodicSyncCheckInterval value is %d\n",value);
		if(value == 0)
		{
			WebcfgDebug("B4 periodicsync_condition pthread_cond_wait\n");
			pthread_cond_wait(&periodicsync_condition, &periodicsync_mutex);
			rv =0;
		}
		else
		{
			WebcfgDebug("B4 periodicsync_condition pthread_cond_timedwait\n");
			rv = pthread_cond_timedwait(&periodicsync_condition, &periodicsync_mutex, &ts);
		}

		if(!rv && !g_shutdown)
		{
			time(&t);
			BOOL ForceSyncEnable;
			char* ForceSyncTransID = NULL;

			// Iterate through all indexes to check which index needs ForceSync
			count = getConfigNumberOfEntries();
			WebConfigLog("count returned is:%d\n", count);

			for(i = 0; i < count; i++)
			{
				WebcfgDebug("B4 getInstanceNumberAtIndex for i %d\n", i);
				index = getInstanceNumberAtIndex(i);
				WebcfgDebug("getForceSyncCheck for index %d\n", index);
				getForceSyncCheck(index,&ForceSyncEnable, &ForceSyncTransID);
				WebcfgDebug("ForceSyncEnable is %d\n", ForceSyncEnable);
				if(ForceSyncTransID !=NULL)
				{
					if(ForceSyncEnable)
					{
						wait_flag=0;
						forced_sync = 1;
						syncIndex = index;
						WebConfigLog("Received signal interrupt to getForceSyncCheck at %s\n",ctime(&t));
						WAL_FREE(ForceSyncTransID);
						break;
					}
					WebConfigLog("ForceSyncEnable is false\n");
					WAL_FREE(ForceSyncTransID);
				}
			}
			WebConfigLog("forced_sync is %d\n", forced_sync);
			if(!forced_sync)
			{
				wait_flag=1;
				value=Get_PeriodicSyncCheckInterval();
				WebConfigLog("Received signal interrupt to change the sync interval to %d\n",value);
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
			WebConfigLog("Received signal interrupt to RFC disable. g_shutdown is %d, proceeding to kill webconfig thread\n", g_shutdown);
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

void processWebconfigSync(int index, int status)
{
	int retry_count=0;
	int r_count=0;
	int configRet = -1;
	char *webConfigData = NULL;
	long res_code;
	int rv=0;
	char *transaction_uuid =NULL;

	WebcfgDebug("========= Start of processWebconfigSync =============\n");
	while(1)
	{
		if(retry_count >3)
		{
			WebConfigLog("retry_count has reached max limit. Exiting.\n");
			retry_count=0;
			break;
		}
		configRet = requestWebConfigData(&webConfigData, r_count, index, status, &res_code, &transaction_uuid);
		if(configRet == 0)
		{
			rv = handleHttpResponse(res_code, webConfigData, retry_count, index, transaction_uuid);
			if(rv ==1)
			{
				WebConfigLog("No retries are required. Exiting..\n");
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
		WebConfigLog("Webconfig retry_count is %d\n", retry_count);
	}
	WebcfgDebug("========= End of processWebconfigSync =============\n");
	return;
}

int handleHttpResponse(long response_code, char *webConfigData, int retry_count, int index, char* transaction_uuid)
{
	int first_digit=0;
	int json_status=0;
	int setRet = 0;
	int ret=0, getRet = 0;
	char *configURL = NULL;
	char *configVersion = NULL;
	char *RequestTimeStamp=NULL;
	char *newDocVersion = NULL;
	int err = 0;

        //get common items for all status codes and send notification.
        getRet = getConfigURL(index, &configURL);
	if(getRet)
	{
		WebcfgDebug("configURL for index %d is %s\n", index, configURL);
	}
	else
	{
		WebConfigLog("getConfigURL failed for index %d\n", index);
	}

	getRet = getConfigVersion(index, &configVersion);
	if(getRet)
	{
		WebConfigLog("configVersion for index %d is %s\n", index, configVersion);
	}
	else
	{
		WebConfigLog("getConfigVersion failed for index %d\n", index);
	}

        ret = setRequestTimeStamp(index);
	if(ret == 0)
	{
		WebcfgDebug("RequestTimeStamp set successfully for index %d\n", index);
	}
	else
	{
		WebConfigLog("Failed to set RequestTimeStamp for index %d\n", index);
	}

        getRet = getRequestTimeStamp(index, &RequestTimeStamp);
	if(getRet)
	{
		WebcfgDebug("RequestTimeStamp for index %d is %s\n", index, RequestTimeStamp);
	}
	else
	{
		WebConfigLog("RequestTimeStamp get failed for index %d\n", index);
	}

	if(response_code == 304)
	{
		WebConfigLog("webConfig is in sync with cloud. response_code:%d\n", response_code);
		setSyncCheckOK(index, TRUE);
		addWebConfigNotifyMsg(configURL, response_code, NULL, 0, RequestTimeStamp , configVersion, transaction_uuid);
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
			WebcfgDebug("newDocVersion is %s\n", newDocVersion);

			if(json_status == 1)
			{
				WebcfgDebug("processJsonDocument success\n");
				if(configURL!=NULL && newDocVersion !=NULL)
				{
					WebConfigLog("Configuration settings from %s version %s were applied successfully\n", configURL, newDocVersion );
				}

				WebcfgDebug("set version and syncCheckOK for success\n");
				ret = setConfigVersion(index, newDocVersion);//get new version from json
				if(ret == 0)
				{
					WebcfgDebug("Config Version %s set successfully for index %d\n", newDocVersion, index);
				}
				else
				{
					WebConfigLog("Failed to set Config version %s for index %d\n", newDocVersion, index);
				}

				ret = setSyncCheckOK(index, TRUE );
				if(ret == 0)
				{
					WebcfgDebug("SyncCheckOK set successfully for index %d\n", index);
				}
				else
				{
					WebConfigLog("Failed to set SyncCheckOK for index %d\n", index);
				}

				addWebConfigNotifyMsg(configURL, response_code, "success", setRet, RequestTimeStamp , newDocVersion, transaction_uuid);
				return 1;
			}
			else
			{
				WebConfigLog("Failure in processJsonDocument\n");
				ret = setSyncCheckOK(index, FALSE);
				if(ret == 0)
				{
					WebcfgDebug("SyncCheckOK set to FALSE for index %d\n", index);
				}
				else
				{
					WebConfigLog("Failed to set SyncCheckOK to FALSE for index %d\n", index);
				}

				WebConfigLog("Configuration settings from %s version %s FAILED\n", configURL, newDocVersion );
				WebConfigLog("Sending Webconfig apply Failure Notification\n");
				addWebConfigNotifyMsg(configURL, response_code, "failed", setRet, RequestTimeStamp , newDocVersion, transaction_uuid);
				return 1;
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
		addWebConfigNotifyMsg(configURL, response_code, NULL, 0, RequestTimeStamp , configVersion, transaction_uuid);
		return 1;
	}
	else if(response_code == 403)
	{
		WebConfigLog("Token is expired, fetch new token. response_code:%d\n", response_code);
		createNewAuthToken(webpa_auth_token, sizeof(webpa_auth_token), get_global_deviceMAC(), serialNum );
		WebcfgDebug("createNewAuthToken done in 403 case\n");
		err = 1;
	}
	else if(response_code == 429)
	{
		WebConfigLog("No action required from client. response_code:%d\n", response_code);
		WAL_FREE(configURL);
		WAL_FREE(configVersion);
		WAL_FREE(RequestTimeStamp);
		WAL_FREE(transaction_uuid);
		return 1;
	}
	first_digit = (int)(response_code / pow(10, (int)log10(response_code)));
	if((response_code !=403) && (first_digit == 4)) //4xx
	{
		WebConfigLog("Action not supported. response_code:%d\n", response_code);
		addWebConfigNotifyMsg(configURL, response_code, NULL, 0, RequestTimeStamp , configVersion, transaction_uuid);
		return 1;
	}
	else //5xx & all other errors
	{
		WebConfigLog("Error code returned, need to retry. response_code:%d\n", response_code);
		if(retry_count == 3 && !err)
		{
			WebcfgDebug("Sending Notification after 3 retry attempts\n");
			addWebConfigNotifyMsg(configURL, response_code, NULL, 0, RequestTimeStamp , configVersion, transaction_uuid);
			return 0;
		}
		WAL_FREE(configURL);
		WAL_FREE(configVersion);
		WAL_FREE(RequestTimeStamp);
		WAL_FREE(transaction_uuid);
	}
	return 0;
}


/*
* @brief Initialize curl object with required options. create configData using libcurl.
* @param[out] configData 
* @param[in] len total configData size
* @param[in] r_count Number of curl retries on ipv4 and ipv6 mode during failure
* @return returns 0 if success, otherwise failed to fetch auth token and will be retried.
*/
int requestWebConfigData(char **configData, int r_count, int index, int status, long *code, char **transaction_id)
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
	char *transID = NULL;
	DATA_TYPE paramType;
	int content_res=0;
	struct token_data data;
	data.size = 0;
	char * configURL = NULL;
	int ret =0, count = 0;
	char *url = NULL;
	char c[] = "{mac}";
	void * dataVal = NULL;

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
		createCurlheader(list, &headers_list, status, index, &transID);
		if(transID !=NULL)
		{
			*transaction_id = strdup(transID);
			WAL_FREE(transID);
		}
		getConfigURL(index, &configURL);
		WebConfigLog("configURL fetched is %s\n", configURL);

		if(configURL !=NULL)
		{
			//Replace {mac} string from default init url with actual deviceMAC
			webConfigURL = replaceMacWord(configURL, c, get_global_deviceMAC());
			WebConfigLog("webConfigURL is %s\n", webConfigURL);
			// Store {mac} replaced/updated config URL to DB
			setConfigURL(index, webConfigURL);
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
		               strncpy(g_interface, interface, sizeof(g_interface)-1);
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
		dataVal = &data;
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback_fn);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, dataVal);

		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers_list);

		WebcfgDebug("Set CURLOPT_HEADERFUNCTION option\n");
		curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_callback);

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

/* @brief callback function to extract response header data.
*/
size_t header_callback(char *buffer, size_t size, size_t nitems)
{
	size_t etag_len = 0;
	char* header_value = NULL;
	char* final_header = NULL;
	char header_str[64] = {'\0'};
	int i=0, j=0;

	etag_len = strlen(ETAG_HEADER);
	if( nitems > etag_len )
	{
		if( strncasecmp(ETAG_HEADER, buffer, etag_len) == 0 )
		{
			header_value = strtok(buffer, ":");
			while( header_value != NULL )
			{
				header_value = strtok(NULL, ":");
				if(header_value !=NULL)
				{
					strncpy(header_str, header_value, sizeof(header_str)-1);
					WebcfgDebug("header_str is %s\n", header_str);
					stripSpaces(header_str, &final_header);

					WebcfgDebug("final_header is %s len %lu\n", final_header, strlen(final_header));
					strncpy(g_ETAG, final_header, sizeof(g_ETAG)-1);
				}
			}
		}
	}
	return nitems;
}

//To strip all spaces , new line & carriage return characters from header output
void stripSpaces(char *str, char **final_str)
{
	int i=0, j=0;

	for(i=0;str[i]!='\0';++i)
	{
		if(str[i]!=' ')
		{
			if(str[i]!='\n')
			{
				if(str[i]!='\r')
				{
					str[j++]=str[i];
				}
			}
		}
	}
	str[j]='\0';
	*final_str = str;
}

int processJsonDocument(char *jsonData, int *retStatus, char **docVersion)
{
	cJSON *paramArray = NULL;
	int parseStatus = 0;
	int i=0, rv=0;
	req_struct *reqObj;
	int paramCount =0;
	WDMP_STATUS valid_ret = WDMP_FAILURE;
	WDMP_STATUS ret = WDMP_FAILURE;
	int ccspStatus=0;
	char *version = NULL;
	int retry_count=0;

	parseStatus = parseJsonData(jsonData, &reqObj, &version);
	WebcfgDebug("After parseJsonData version is %s\n", version);
	if(version!=NULL)
	{
		*docVersion = strdup(version);
		WebcfgDebug("docVersion is %s\n", *docVersion);
		WAL_FREE(version);
	}
	if(parseStatus ==1)
	{
		if(NULL == reqObj)
		{
			WebConfigLog("parseJsonData failed. reqObj is empty\n");
			return rv;
		}
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
			while(1)
			{
				if(retry_count >3)
				{
					WebConfigLog("Config apply retry_count has reached max limit\n");
					retry_count=0;
					break;
				}

				WebcfgInfo("WebConfig SET Request\n");
				setValues(reqObj->u.setReq->param, paramCount, WEBPA_SET, NULL, NULL, &ret, &ccspStatus);
				WebcfgInfo("Processed WebConfig SET Request\n");
				*retStatus = ccspStatus;
				WebcfgDebug("*retStatus is %d\n", *retStatus);
		                if(ret == WDMP_SUCCESS)
		                {
		                        WebConfigLog("setValues success. ccspStatus : %d\n", ccspStatus);
		                        rv= 1;
					break;
		                }
		                else
		                {
		                      WebConfigLog("setValues Failed. ccspStatus : %d\n", ccspStatus);
		                }
				WebConfigLog("processJsonDocument BACKOFF_SLEEP_DELAY_SEC is %d seconds\n", BACKOFF_SLEEP_DELAY_SEC);
				sleep(BACKOFF_SLEEP_DELAY_SEC);
				retry_count++;
				WebConfigLog("Config apply retry_count is %d\n", retry_count);
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
	char *configVersion= NULL;

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
			isValid = validateConfigFormat(json, &configVersion);
			WebConfigLog("configVersion is %s\n", configVersion);
			if(configVersion !=NULL)
			{
				*version = strdup(configVersion);
				WebcfgDebug("version copied from configVersion is %s\n", *version);
				WAL_FREE(configVersion);
			}
			if(!isValid)
			{
				WebConfigLog("validateConfigFormat failed\n");
				return rv;
			}
			(reqObj) = (req_struct *) malloc(sizeof(req_struct));
			if(reqObj !=NULL)
			{
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
	char *jsonversion=NULL;

	versionObj = cJSON_GetObjectItem( json, "version" );
	if(versionObj !=NULL)
	{
		if(cJSON_GetObjectItem( json, "version" )->type == cJSON_String)
		{
			jsonversion = cJSON_GetObjectItem( json, "version" )->valuestring;
			if(jsonversion !=NULL)
			{
				//version & eTag header validation
				if(strlen(g_ETAG)>0)
				{
					WebcfgDebug("jsonversion :%s len %lu\n", jsonversion, strlen(jsonversion));
					if(strncmp(jsonversion, g_ETAG, strlen(g_ETAG)) == 0)
					{
						WebConfigLog("Config Version and ETAG header are matching\n");
						*eTag = strdup(jsonversion);
						WebConfigLog("eTag is %s\n", *eTag);
						//check parameters
						paramArray = cJSON_GetObjectItem( json, "parameters" );
						if( paramArray != NULL )
						{
							itemSize = cJSON_GetArraySize( json );
							if(itemSize ==2)
							{
								WebConfigLog("Config document format is valid\n");
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
						WebConfigLog("Invalid config json, version & ETAG are not same\n");
						return 0;
					}
				}
				else
				{
					WebConfigLog("Failed to fetch ETAG header from config response\n");
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
		while (fgets(str, sizeof(str), fp) != NULL)
		{
		    char *value = NULL;

		    if(NULL != (value = strstr(str, "WEBCONFIG_INTERFACE=")))
		    {
			value = value + strlen("WEBCONFIG_INTERFACE=");
			value[strlen(value)-1] = '\0';
			*interface = strdup(value);
			break;
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
void createCurlheader( struct curl_slist *list, struct curl_slist **header_list, int status, int index, char ** trans_uuid)
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
	char* syncTransID = NULL;
	BOOL ForceSyncEnable;

	WebConfigLog("Start of createCurlheader\n");
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
		getConfigVersion(index, &version);
		snprintf(version_header, MAX_BUF_SIZE, "IF-NONE-MATCH:%s", ((NULL != version && (strlen(version)!=0)) ? version : "NONE"));
		WebConfigLog("version_header formed %s\n", version_header);
		list = curl_slist_append(list, version_header);
		WAL_FREE(version_header);
		if(version !=NULL)
		{
			WAL_FREE(version);
		}
	}

	schema_header = (char *) malloc(sizeof(char)*MAX_BUF_SIZE);
	if(schema_header !=NULL)
	{
		snprintf(schema_header, MAX_BUF_SIZE, "Schema-Version: %s", "v1.0");
		WebConfigLog("schema_header formed %s\n", schema_header);
		list = curl_slist_append(list, schema_header);
		WAL_FREE(schema_header);
	}

	if(strlen(g_bootTime) ==0)
	{
		bootTime = getParameterValue(DEVICE_BOOT_TIME);
		if(bootTime !=NULL)
		{
		       strncpy(g_bootTime, bootTime, sizeof(g_bootTime)-1);
		       WebcfgDebug("g_bootTime fetched is %s\n", g_bootTime);
		       WAL_FREE(bootTime);
		}
	}

	if(strlen(g_bootTime))
	{
		bootTime_header = (char *) malloc(sizeof(char)*MAX_BUF_SIZE);
		if(bootTime_header !=NULL)
		{
			snprintf(bootTime_header, MAX_BUF_SIZE, "X-System-Boot-Time: %s", g_bootTime);
			WebConfigLog("bootTime_header formed %s\n", bootTime_header);
			list = curl_slist_append(list, bootTime_header);
			WAL_FREE(bootTime_header);
		}
	}
	else
	{
		WebConfigLog("Failed to get bootTime\n");
	}

	if(strlen(g_FirmwareVersion) ==0)
	{
		FwVersion = getParameterValue(FIRMWARE_VERSION);
		if(FwVersion !=NULL)
		{
		       strncpy(g_FirmwareVersion, FwVersion, sizeof(g_FirmwareVersion)-1);
		       WebcfgDebug("g_FirmwareVersion fetched is %s\n", g_FirmwareVersion);
		       WAL_FREE(FwVersion);
		}
	}

	if(strlen(g_FirmwareVersion))
	{
		FwVersion_header = (char *) malloc(sizeof(char)*MAX_BUF_SIZE);
		if(FwVersion_header !=NULL)
		{
			snprintf(FwVersion_header, MAX_BUF_SIZE, "X-System-Firmware-Version: %s", g_FirmwareVersion);
			WebConfigLog("FwVersion_header formed %s\n", FwVersion_header);
			list = curl_slist_append(list, FwVersion_header);
			WAL_FREE(FwVersion_header);
		}
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
                       strncpy(g_systemReadyTime, systemReadyTime, sizeof(g_systemReadyTime)-1);
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

	getForceSyncCheck(index,&ForceSyncEnable, &syncTransID);
	if(syncTransID !=NULL)
	{
		if(ForceSyncEnable && strlen(syncTransID)>0)
		{
			WebConfigLog("updating transaction_uuid with force syncTransID\n");
			transaction_uuid = strdup(syncTransID);
		}
		WAL_FREE(syncTransID);
	}

	if(transaction_uuid == NULL)
	{
		transaction_uuid = generate_trans_uuid();
	}

	if(transaction_uuid !=NULL)
	{
		uuid_header = (char *) malloc(sizeof(char)*MAX_BUF_SIZE);
		if(uuid_header !=NULL)
		{
			snprintf(uuid_header, MAX_BUF_SIZE, "Transaction-ID: %s", transaction_uuid);
			WebConfigLog("uuid_header formed %s\n", uuid_header);
			list = curl_slist_append(list, uuid_header);
			*trans_uuid = strdup(transaction_uuid);
			WAL_FREE(transaction_uuid);
			WAL_FREE(uuid_header);
		}
	}
	else
	{
		WebConfigLog("Failed to generate transaction_uuid\n");
	}
	*header_list = list;
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

void getAuthToken()
{
	//local var to update webpa_auth_token only in success case
	char output[4069] = {'\0'} ;
	char *serial_number=NULL;
	memset (webpa_auth_token, 0, sizeof(webpa_auth_token));

	if( strlen(WEBPA_READ_HEADER) !=0 && strlen(WEBPA_CREATE_HEADER) !=0)
	{
                getDeviceMac();
                WebConfigLog("deviceMAC: %s\n",get_global_deviceMAC());

		if( get_global_deviceMAC() != NULL && strlen(get_global_deviceMAC()) !=0 )
		{
			if(strlen(serialNum) ==0)
			{
				serial_number = getParameterValue(SERIAL_NUMBER);
		                if(serial_number !=NULL)
		                {
					strncpy(serialNum ,serial_number, sizeof(serialNum)-1);
					WebConfigLog("serialNum: %s\n", serialNum);
					WAL_FREE(serial_number);
		                }
			}

			if( strlen(serialNum)>0 )
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
				}
				else
				{
					WebConfigLog("update webpa_auth_token in success case\n");
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


//To handle webconfig notification tasks
static void initWebConfigNotifyTask()
{
	int err = 0;

	err = pthread_create(&NotificationThreadId, NULL, processWebConfigNotification, NULL);
	if (err != 0)
	{
		WebConfigLog("Error creating Webconfig Notification thread :[%s]\n", strerror(err));
	}
	else
	{
		WebConfigLog("Webconfig Notification thread created Successfully\n");
	}

}

void addWebConfigNotifyMsg(char *url, long status_code, char *application_status, int application_details, char *request_timestamp, char *version, char *transaction_uuid)
{
	notify_params_t *args = NULL;
	args = (notify_params_t *)malloc(sizeof(notify_params_t));

	if(args != NULL)
	{
                WebcfgDebug("pthread mutex lock\n");
		pthread_mutex_lock (&notify_mut);
		memset(args, 0, sizeof(notify_params_t));
		if(url != NULL)
		{
			args->url = strdup(url);
			WAL_FREE(url);
		}
		args->status_code = status_code;
		if(application_status != NULL)
		{
			args->application_status = strdup(application_status);
		}
		args->application_details = application_details;
		if(request_timestamp != NULL)
		{
			args->request_timestamp = strdup(request_timestamp);
			WAL_FREE(request_timestamp);
		}
		if(version != NULL)
		{
			args->version = strdup(version);
			WAL_FREE(version);
		}
		if(transaction_uuid != NULL)
		{
			args->transaction_uuid = strdup(transaction_uuid);
			WAL_FREE(transaction_uuid);
		}
		WebcfgDebug("args->url:%s args->status_code:%d args->application_status:%s args->application_details:%d args->request_timestamp:%s args->version:%s args->transaction_uuid:%s\n", args->url, args->status_code, args->application_status, args->application_details, args->request_timestamp, args->version, args->transaction_uuid );

		notifyMsg = args;

                WebcfgDebug("Before notify pthread cond signal\n");
		pthread_cond_signal(&notify_con);
                WebcfgDebug("After notify pthread cond signal\n");
		pthread_mutex_unlock (&notify_mut);
                WebcfgDebug("pthread mutex unlock\n");
	}
}

//Notify thread function waiting for notify msgs
void* processWebConfigNotification()
{
	char device_id[32] = { '\0' };
	cJSON *notifyPayload = NULL;
	char  * stringifiedNotifyPayload = NULL;
	notify_params_t *msg = NULL;
	char dest[512] = {'\0'};
	char *source = NULL;
	cJSON * reports, *one_report;

	while(1)
	{
                WebcfgDebug("processWebConfigNotification Inside while\n");
		pthread_mutex_lock (&notify_mut);
		WebcfgDebug("processWebConfigNotification mutex lock\n");
		msg = notifyMsg;
		if(msg !=NULL)
		{
                        WebcfgDebug("Processing msg\n");
			if(strlen(get_global_deviceMAC()) == 0)
			{
				WebConfigLog("deviceMAC is NULL, failed to send Webconfig Notification\n");
			}
			else
			{
				snprintf(device_id, sizeof(device_id), "mac:%s", get_global_deviceMAC());
				WebConfigLog("webconfig Device_id %s\n", device_id);

				notifyPayload = cJSON_CreateObject();

				if(notifyPayload != NULL)
				{
					cJSON_AddStringToObject(notifyPayload,"device_id", device_id);

					if(msg)
					{
						cJSON_AddItemToObject(notifyPayload, "reports", reports = cJSON_CreateArray());
						cJSON_AddItemToArray(reports, one_report = cJSON_CreateObject());
						cJSON_AddStringToObject(one_report, "url", (NULL != msg->url) ? msg->url : "unknown");
						cJSON_AddNumberToObject(one_report,"http_status_code", msg->status_code);
                                                if(msg->status_code == 200)
                                                {
						        cJSON_AddStringToObject(one_report,"document_application_status", (NULL != msg->application_status) ? msg->application_status : "unknown");
						        cJSON_AddNumberToObject(one_report,"document_application_details", msg->application_details);
                                                }
						cJSON_AddNumberToObject(one_report, "request_timestamp", (NULL != msg->request_timestamp) ? atoi(msg->request_timestamp) : 0);
						cJSON_AddStringToObject(one_report,"version", (NULL != msg->version && (strlen(msg->version)!=0)) ? msg->version : "NONE");
						cJSON_AddStringToObject(one_report,"transaction_uuid", (NULL != msg->transaction_uuid && (strlen(msg->transaction_uuid)!=0)) ? msg->transaction_uuid : "unknown");
					}
					stringifiedNotifyPayload = cJSON_PrintUnformatted(notifyPayload);
					cJSON_Delete(notifyPayload);
				}

				snprintf(dest,sizeof(dest),"event:config-version-report/%s",device_id);
				WebConfigLog("dest is %s\n", dest);

				if (stringifiedNotifyPayload != NULL && strlen(device_id) != 0)
				{
					source = (char*) malloc(sizeof(char) * sizeof(device_id));
					strncpy(source, device_id, sizeof(device_id));
					WebConfigLog("source is %s\n", source);
					WebConfigLog("stringifiedNotifyPayload is %s\n", stringifiedNotifyPayload);
					sendNotification(stringifiedNotifyPayload, source, dest);
				}
				if(msg != NULL)
				{
					free_notify_params_struct(msg);
					notifyMsg = NULL;
				}
			}
			pthread_mutex_unlock (&notify_mut);
			WebcfgDebug("processWebConfigNotification mutex unlock\n");
		}
		else
		{
			WebcfgDebug("Before pthread cond wait in notify thread\n");
			pthread_cond_wait(&notify_con, &notify_mut);
			pthread_mutex_unlock (&notify_mut);
			WebcfgDebug("mutex unlock in notify thread after cond wait\n");
		}
	}
	return NULL;
}

void free_notify_params_struct(notify_params_t *param)
{
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
        if(param->request_timestamp != NULL)
        {
            WAL_FREE(param->request_timestamp);
        }
        if(param->version != NULL)
        {
            WAL_FREE(param->version);
        }
	if(param->transaction_uuid != NULL)
        {
	    WAL_FREE(param->transaction_uuid);
        }
        WAL_FREE(param);
    }
}

char *replaceMacWord(const char *s, const char *macW, const char *deviceMACW)
{
	char *result;
	int i, cnt = 0;
	int deviceMACWlen = strlen(deviceMACW);
	int macWlen = strlen(macW);
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
	return result;
}
