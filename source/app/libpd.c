#include <stdio.h>
#include <libparodus.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <cJSON.h>
#include "libpd.h"
#include "webpa_adapter.h"

#define CONTENT_TYPE_JSON       "application/json"
#define DEVICE_PROPS_FILE   "/etc/device.properties"
#define MAX_PARALLEL_THREADS    2
#define PARODUS_URL_DEFAULT      "tcp://127.0.0.1:6666"
#define CLIENT_URL_DEFAULT       "tcp://127.0.0.1:6667"
#define CLOUD_STATUS 		"cloud-status"
#define CLOUD_STATUS_ONLINE      "online"
#define MAX_STR_LENGTH      	100

static void connect_parodus();
static void get_parodus_url(char **parodus_url, char **client_url);
static void parodus_receive();
static void initParallelProcess();
static void generate_trans_uuid(char **transID);
libpd_instance_t current_instance;
static char *cloud_status = "offline";
pthread_mutex_t cloud_mut=PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cloud_con=PTHREAD_COND_INITIALIZER;

static void connect_parodus()
{
        int backoffRetryTime = 0;
        int backoff_max_time = 5;
        int max_retry_sleep;
        //Retry Backoff count shall start at c=2 & calculate 2^c - 1.
        int c =2;
        int retval=-1;
        int fd = 0;
        char *parodus_url = NULL;
        char *client_url = NULL;

        max_retry_sleep = (int) pow(2, backoff_max_time) -1;
        WalInfo("max_retry_sleep is %d\n", max_retry_sleep );

	get_parodus_url(&parodus_url, &client_url);
		
	if(parodus_url != NULL && client_url != NULL)
	{	
		libpd_cfg_t cfg1 = {.service_name = "config",
					.receive = true, .keepalive_timeout_secs = 64,
					.parodus_url = parodus_url,
					.client_url = client_url
				   };
	            
	    	WalPrint("libparodus_init with parodus url %s and client url %s\n",cfg1.parodus_url,cfg1.client_url);

	    	while(1)
	    	{
	            if(backoffRetryTime < max_retry_sleep)
	            {
	                  backoffRetryTime = (int) pow(2, c) -1;
	            }

	            WalPrint("New backoffRetryTime value calculated as %d seconds\n", backoffRetryTime);
	            int ret =libparodus_init (&current_instance, &cfg1);
	            WalPrint("ret is %d\n",ret);
	            if(ret ==0)
	            {
	                    WalInfo("Init for parodus Success..!!\n");
	                    WalInfo("WebPA is now ready to process requests\n");
#ifdef RDKB_BUILD
	                    if (access("/tmp/webpa_start", F_OK) == -1 && errno == ENOENT)
	                    {
	                            system("print_uptime \"boot_to_WEBPA_READY_uptime\" \"/rdklogs/logs/WEBPAlog.txt.0\"");

	                            if((fd = creat("/tmp/webpa_start", S_IRUSR | S_IWUSR)) == -1)
	                            {
	                                    fprintf(stderr, "File: /tmp/webpa_start creation failed with error:%d\n", errno);
	                            }
	                            else
	                            {
	                                    close(fd);
	                            }
	                    }
#endif
	                    break;
	            }
	            else
	            {
	                    WalError("Init for parodus failed: '%s'\n",libparodus_strerror(ret));
	                    sleep(backoffRetryTime);
	                    c++;

	                    if(backoffRetryTime == max_retry_sleep)
	                    {
	                    	c = 2;
	                    	backoffRetryTime = 0;
	                    	WalPrint("backoffRetryTime reached max value, reseting to initial value\n");
	                    }
	            }
	            retval = libparodus_shutdown(&current_instance);
	            WalPrint("libparodus_shutdown retval %d\n", retval);
	    	}
	}
}
	
static void parodus_receive()
{
        int rtn;
        wrp_msg_t *wrp_msg;
        wrp_msg_t *res_wrp_msg ;

        struct timespec start,end,*startPtr,*endPtr;
        startPtr = &start;
        endPtr = &end;
        char *contentType = NULL;
        char *sourceService, *sourceApplication =NULL;
        char *status=NULL;

        rtn = libparodus_receive (current_instance, &wrp_msg, 2000);
        if (rtn == 1)
        {
                return;
        }

        if (rtn != 0)
        {
                WalError ("Libparodus failed to recieve message: '%s'\n",libparodus_strerror(rtn));
                sleep(5);
                return;
        }

        if(wrp_msg != NULL)
        {
            if (wrp_msg->msg_type == WRP_MSG_TYPE__REQ)
            {
                    res_wrp_msg = (wrp_msg_t *)malloc(sizeof(wrp_msg_t));
                    

                    if(res_wrp_msg != NULL)
                    {
						memset(res_wrp_msg, 0, sizeof(wrp_msg_t));
                        getCurrentTime(startPtr);
                        processRequest((char *)wrp_msg->u.req.payload, wrp_msg->u.req.transaction_uuid, ((char **)(&(res_wrp_msg->u.req.payload))));

                        
                        if(res_wrp_msg->u.req.payload !=NULL)
                        {   
                                WalPrint("Response payload is %s\n",(char *)(res_wrp_msg->u.req.payload));
                                res_wrp_msg->u.req.payload_size = strlen(res_wrp_msg->u.req.payload);
                        }
                        res_wrp_msg->msg_type = wrp_msg->msg_type;
						if(wrp_msg->u.req.dest != NULL){
                        	res_wrp_msg->u.req.source = wrp_msg->u.req.dest;
						}	
						if(wrp_msg->u.req.source != NULL){
                        	res_wrp_msg->u.req.dest = wrp_msg->u.req.source;
						}
						if(wrp_msg->u.req.transaction_uuid != NULL){
	                        res_wrp_msg->u.req.transaction_uuid = wrp_msg->u.req.transaction_uuid;
						}
                        contentType = strdup(CONTENT_TYPE_JSON);
                        if(contentType != NULL)
                        {
                            res_wrp_msg->u.req.content_type = contentType;
                        }
                        int sendStatus = libparodus_send(current_instance, res_wrp_msg);
                        WalPrint("sendStatus is %d\n",sendStatus);
                        if(sendStatus == 0)
                        {
                                WalInfo("Sent message successfully to parodus\n");
                        }
                        else
                        {
                                WalError("Failed to send message: '%s'\n",libparodus_strerror(sendStatus));
                        }
                        getCurrentTime(endPtr);
                        WalInfo("Elapsed time : %ld ms\n", timeValDiff(startPtr, endPtr));
                        wrp_free_struct (res_wrp_msg);
                    }
            }

            //handle cloud-status retrieve response received from parodus
            if (wrp_msg->msg_type == WRP_MSG_TYPE__RETREIVE)
            {
				sourceService = wrp_get_msg_element(WRP_ID_ELEMENT__SERVICE, wrp_msg, SOURCE);
				sourceApplication = wrp_get_msg_element(WRP_ID_ELEMENT__APPLICATION, wrp_msg, SOURCE);

				if(sourceService != NULL && sourceApplication != NULL && strcmp(sourceService,"parodus")== 0 && strcmp(sourceApplication,"cloud-status")== 0)
				{
					WalInfo("cloud-status Retrieve response received from parodus : %s len %lu\n",(char *)wrp_msg->u.crud.payload, strlen(wrp_msg->u.crud.payload) );

					parsePayloadForStatus(wrp_msg->u.crud.payload, &status);
					if(status !=NULL)
					{
						//set this as global conn status. add lock before update it.
						set_global_cloud_status(status);
						WalInfo("set cloud-status value as %s\n", status);

						pthread_mutex_lock (&cloud_mut);
						WalPrint("mutex lock in producer thread\n");
						pthread_cond_signal(&cloud_con);
						pthread_mutex_unlock (&cloud_mut);
						WalPrint("mutex unlock in producer thread\n");
					}
				}
            }
            free(wrp_msg);
        }
}

void *parallelProcessTask()
{
        pthread_detach(pthread_self());
        while( FOREVER() )
        {
                parodus_receive();
        }
        return NULL;
}

static void initParallelProcess()
{
        int err = 0, i = 0;
        pthread_t threadId[MAX_PARALLEL_THREADS-1];
        WalPrint("============ initParallelProcess ==============\n");
        for(i=0; i<MAX_PARALLEL_THREADS-1; i++)
        {
                err = pthread_create(&threadId[i], NULL, parallelProcessTask, NULL);
                if (err != 0)
                {
                        WalError("Error creating parallelProcessTask thread %d :[%s]\n", i, strerror(err));
                }
                else
                {
                        WalPrint("parallelProcessTask thread %d created Successfully\n",i);
                }
        }
}

void parodus_receive_wait()
{
        if(MAX_PARALLEL_THREADS > 1)
        {
                WalInfo("Parallel request processing is enabled\n");
        }
        initParallelProcess();
        parallelProcessTask();
        libparodus_shutdown(&current_instance);
        WalPrint ("End of parodus_upstream\n");
}

char *get_global_cloud_status()
{
	char *temp = NULL;
	pthread_mutex_lock (&cloud_mut);
	temp = cloud_status;
	pthread_mutex_unlock (&cloud_mut);
    return temp;
}

void set_global_cloud_status(char *status)
{
	pthread_mutex_lock (&cloud_mut);
    cloud_status = status;
    pthread_mutex_unlock (&cloud_mut);
}

//send cloud-status upstream RETRIEVE request to parodus to check connectivity
int getConnCloudStatus(char *device_mac)
{
	char *source = NULL, *dest = NULL;
	wrp_msg_t *req_wrp_msg = NULL;
	char *contentType = NULL;
	int rv = -1;
	int sendStatus = -1;
    int backoffRetryTime = 0;
    int c=2;
    char *transaction_uuid = NULL;

	if(device_mac == NULL)
	{
		WalError("device_mac is NULL, unable to get cloud_status\n");
		return rv;
	}
	else
	{
		req_wrp_msg = (wrp_msg_t *)malloc(sizeof(wrp_msg_t));
		if(req_wrp_msg != NULL)
		{
			memset(req_wrp_msg, 0, sizeof(wrp_msg_t));
			req_wrp_msg->msg_type = WRP_MSG_TYPE__RETREIVE;

			source = (char *) malloc(sizeof(char)*MAX_STR_LENGTH);
			dest   = (char *) malloc(sizeof(char)*MAX_STR_LENGTH);

			if(source !=NULL)
			{
				snprintf(source, MAX_STR_LENGTH, "mac:%s/config", device_mac);
				req_wrp_msg->u.crud.source = source;
				WalPrint("req_wrp_msg->u.crud.source is %s\n", req_wrp_msg->u.crud.source);
			}

			if(dest !=NULL)
			{
				snprintf(dest, MAX_STR_LENGTH, "mac:%s/parodus/cloud-status", device_mac);
				req_wrp_msg->u.crud.dest = dest;
				WalPrint("req_wrp_msg->u.crud.dest is %s\n", req_wrp_msg->u.crud.dest);
			}

			generate_trans_uuid(&transaction_uuid);
			if(transaction_uuid !=NULL)
			{
				req_wrp_msg->u.crud.transaction_uuid = transaction_uuid;
				WalInfo("transaction_uuid generated is %s\n", req_wrp_msg->u.crud.transaction_uuid);
			}

			contentType = strdup(CONTENT_TYPE_JSON);
			if(contentType != NULL)
			{
				req_wrp_msg->u.crud.content_type = contentType;
				WalPrint("retrieve content_type is %s\n",req_wrp_msg->u.crud.content_type);
			}

			while( FOREVER() )
			{
				backoffRetryTime = (int) pow(2, c) -1;
				WalPrint("Backoff calculated is %d\n", backoffRetryTime);

				sendStatus = libparodus_send(current_instance, req_wrp_msg);
				WalPrint("sendStatus is %d\n",sendStatus);
				if(sendStatus == 0)
				{
					WalInfo("Sent retrieve request successfully to parodus\n");
				}
				else
				{
					WalError("Failed to send retrieve req: '%s'\n",libparodus_strerror(sendStatus));
					break;
				}

				//waiting to get response from parodus. add lock here while reading
				pthread_mutex_lock (&cloud_mut);
				WalPrint("mutex lock in consumer thread\n");
				WalPrint("Before pthread cond wait in consumer thread\n");
				pthread_cond_wait(&cloud_con, &cloud_mut);
				pthread_mutex_unlock (&cloud_mut);
				WalPrint("mutex unlock in consumer thread after cond wait\n");

				if ((get_global_cloud_status() !=NULL) && (strcmp(get_global_cloud_status(), CLOUD_STATUS_ONLINE) == 0))
				{
					WalPrint("Received cloud_status as online, returning ..\n");
					rv = 1;
					free(get_global_cloud_status());
					break;
				}
				else
				{
					WalError("cloud_status is not online, sending RETRIEVE request again and retrying\n" );
					WalInfo("cloud status retry backoffRetryTime '%d' seconds\n", backoffRetryTime);
					sleep(backoffRetryTime);
					c++;
					if(get_global_cloud_status() !=NULL)
					{
						free(get_global_cloud_status());
					}
				}
			}
			wrp_free_struct (req_wrp_msg);
		}
	}
	return rv;
}

void sendNotification(char *payload, char *source, char *destination)
{
    wrp_msg_t *notif_wrp_msg = NULL;
    int retry_count = 0;
    int sendStatus = -1;
    int backoffRetryTime = 0;
    int c=2;
    char *contentType = NULL;

    if(source != NULL && destination != NULL)
    {
        notif_wrp_msg = (wrp_msg_t *)malloc(sizeof(wrp_msg_t));
        

        if(notif_wrp_msg != NULL)
        {
	    memset(notif_wrp_msg, 0, sizeof(wrp_msg_t));
	    notif_wrp_msg ->msg_type = WRP_MSG_TYPE__EVENT;
            WalPrint("source: %s\n",source);
            notif_wrp_msg ->u.event.source = source;
            WalPrint("destination: %s\n", destination);
            notif_wrp_msg ->u.event.dest = strdup(destination);
            contentType = strdup(CONTENT_TYPE_JSON);
            if(contentType != NULL)
            {
                notif_wrp_msg->u.event.content_type = contentType;
                WalPrint("content_type is %s\n",notif_wrp_msg->u.event.content_type);
            }
            

	    if(payload != NULL)
            {
	        WalInfo("Notification payload: %s\n",payload);
		notif_wrp_msg ->u.event.payload = (void *)payload;
                notif_wrp_msg ->u.event.payload_size = strlen(notif_wrp_msg ->u.event.payload);
            }

            while(retry_count<=3)
            {
                backoffRetryTime = (int) pow(2, c) -1;

                sendStatus = libparodus_send(current_instance, notif_wrp_msg );
                if(sendStatus == 0)
                {
                    retry_count = 0;
                    WalInfo("Notification successfully sent to parodus\n");
                    break;
                }
                else
                {
                    WalError("Failed to send Notification: '%s', retrying ....\n",libparodus_strerror(sendStatus));
                    WalPrint("sendNotification backoffRetryTime %d seconds\n", backoffRetryTime);
                    sleep(backoffRetryTime);
                    c++;
                    retry_count++;
                }
            }

            WalPrint("sendStatus is %d\n",sendStatus);
            wrp_free_struct (notif_wrp_msg );
            WalPrint("Freed notif_wrp_msg struct.\n");
        }
    }
}

void libpd_client_mgr()
{
	WalPrint("Connect parodus \n");
        connect_parodus();
}

static void get_parodus_url(char **parodus_url, char **client_url)
{

	FILE *fp = fopen(DEVICE_PROPS_FILE, "r");

	if (NULL != fp)
	{
		char str[255] = {'\0'};
		while(fscanf(fp,"%s", str) != EOF)
		{
		    char *value = NULL;

		    if(NULL != (value = strstr(str, "PARODUS_URL=")))
		    {
			value = value + strlen("PARODUS_URL=");
			*parodus_url = strdup(value);
		    }

		    if(NULL != (value = strstr(str, "WEBPA_CLIENT_URL=")))
		    {
			value = value + strlen("WEBPA_CLIENT_URL=");
			*client_url = strdup(value);
		    }

		}
		fclose(fp);
	}
	else
	{
		WalError("Failed to open device.properties file:%s\n", DEVICE_PROPS_FILE);
		WalInfo("Adding default values for parodus_url and client_url\n");
		*parodus_url = strdup(PARODUS_URL_DEFAULT);
		*client_url = strdup(CLIENT_URL_DEFAULT);
	}

	if (NULL == *parodus_url)
	{
		WalError("parodus_url is not present in device.properties, adding default parodus_url\n");
		
		*parodus_url = strdup(PARODUS_URL_DEFAULT);
	}
	else
	{
		WalPrint("parodus_url formed is %s\n", *parodus_url);
	}

	if (NULL == *client_url)
	{
		WalError("client_url is not present in device.properties, adding default client_url\n");
		*client_url = strdup(CLIENT_URL_DEFAULT);
	}
	else
	{
		WalPrint("client_url formed is %s\n", *client_url);
    	}
}

void parsePayloadForStatus(char *payload, char **cloudStatus)
{
	cJSON *json = NULL;
	cJSON *cloudStatusObj = NULL;
	char *cloud_status_str = NULL;

	json = cJSON_Parse( payload );
	if( !json )
	{
		WalError( "json parse error: [%s]\n", cJSON_GetErrorPtr() );
	}
	else
	{
		cloudStatusObj = cJSON_GetObjectItem( json, CLOUD_STATUS );
		if( cloudStatusObj != NULL)
		{
			cloud_status_str = cJSON_GetObjectItem( json, CLOUD_STATUS )->valuestring;
			if ((cloud_status_str != NULL) && strlen(cloud_status_str) > 0)
			{
				*cloudStatus = strdup(cloud_status_str);
				WalPrint(" cloudStatus value parsed from payload is %s\n", *cloudStatus);
			}
			else
			{
				WalError("cloud status string is empty\n");
			}
		}
		else
		{
			WalError("Failed to get cloudStatus from payload\n");
		}
		cJSON_Delete(json);
	}
}

static void generate_trans_uuid(char **transID)
{
	FILE *file = NULL;
	char buffer [ 64 ] = { 0 };
	char *command = NULL;

	command = strdup("uuidgen -r");
	if(command !=NULL)
	{
		file = popen ( command, "r" );
		if(file)
		{
		   fgets ( buffer, 64, file );
		   pclose ( file );
		   file = NULL;
		   *transID = strdup(buffer);
		}
		else
		{
			WalError("Error in opening File to generate transaction uuid\n");
		}
		free(command);
	}
}

const char *rdk_logger_module_fetch(void)
{
    return "LOG.RDK.WEBPA";
}

