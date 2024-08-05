#include <stdbool.h>
#include <string.h>

#include <stdlib.h>
#include <wdmp-c.h>
#include <cimplog.h>
#include <pthread.h>
#include "webpa_rbus.h"
#include "webpa_notification.h"

#define CLOUD_CONN_ONLINE "cloud_conn_online_event"

static rbusHandle_t rbus_handle;
static bool isRbus = false;
int RetrySync = 0;

bool isRbusEnabled()
{
        if(RBUS_ENABLED == rbus_checkStatus())
        {
                isRbus = true;
        }
        else
        {
                isRbus = false;
        }
        WalInfo("Webpa RBUS mode active status = %s\n", isRbus ? "true":"false");
        return isRbus;
}

bool isRbusInitialized()
{
    return rbus_handle != NULL ? true : false;
}

WDMP_STATUS webpaRbusInit(const char *pComponentName)
{
        int ret = RBUS_ERROR_SUCCESS;

        WalInfo("rbus_open for component %s\n", pComponentName);
        ret = rbus_open(&rbus_handle, pComponentName);
        if(ret != RBUS_ERROR_SUCCESS)
        {
                WalError("webpaRbusInit failed with error code %d\n", ret);
                return WDMP_FAILURE;
        }
        WalInfo("webpaRbusInit is success. ret is %d\n", ret);
        return WDMP_SUCCESS;
}

void webpaRbus_Uninit()
{
    rbus_close(rbus_handle);
}

rbusError_t setTraceContext(char* traceContext[])
{
        rbusError_t ret = RBUS_ERROR_BUS_ERROR;
        if(isRbusInitialized)
        {
                if(traceContext[0] != NULL && traceContext[1] != NULL) {
                       if(strlen(traceContext[0]) > 0 && strlen(traceContext[1]) > 0) {
			    WalInfo("Invoked setTraceContext function with value traceParent - %s, traceState - %s\n", traceContext[0], traceContext[1]);    
                            ret = rbusHandle_SetTraceContextFromString(rbus_handle, traceContext[0], traceContext[1]);
                            if(ret == RBUS_ERROR_SUCCESS) {
                                  WalPrint("SetTraceContext request success\n");
                            }
                             else {
                                   WalError("SetTraceContext request failed with error code - %d\n", ret);
                             }
                        }
                        else {
                              WalError("Header is empty\n");
                        }
                  }
                  else {
                        WalError("Header is NULL\n");
                  }
        }
        else {
                WalError("Rbus not initialzed in setTraceContext function\n");
        }	
        return ret;
}

rbusError_t getTraceContext(char* traceContext[])
{
        rbusError_t ret = RBUS_ERROR_BUS_ERROR;
        char traceParent[512] = {'\0'};
        char traceState[512] = {'\0'};
	if(isRbusInitialized)
        {
	      ret =  rbusHandle_GetTraceContextAsString(rbus_handle, traceParent, sizeof(traceParent), traceState, sizeof(traceState));
	      if( ret == RBUS_ERROR_SUCCESS) {
		      if(strlen(traceParent) > 0 && strlen(traceState) > 0) {
			      WalPrint("GetTraceContext request success\n");
		              traceContext[0] = strdup(traceParent);
	                      traceContext[1] = strdup(traceState);
			      WalInfo("traceContext value, traceParent - %s, traceState - %s\n", traceContext[0], traceContext[1]);
	               }
		       else {
			       WalPrint("traceParent & traceState are empty\n");
		       }	       
	      }
	      else {
		      WalError("GetTraceContext request failed with error code - %d\n", ret);
	      }	      
	}
        else { 
              WalError("Rbus not initialzed in getTraceContext function\n");
	}
        return ret;
}

rbusError_t clearTraceContext()
{
	rbusError_t ret = RBUS_ERROR_BUS_ERROR;
	if(isRbusInitialized)
	{
		ret = rbusHandle_ClearTraceContext(rbus_handle);
		if(ret == RBUS_ERROR_SUCCESS) {
			WalInfo("ClearTraceContext request success\n");
		}
		else {
			WalError("ClearTraceContext request failed with error code - %d\n", ret);
		}
	}
	else {
		WalError("Rbus not initialized in clearTraceContext funcion\n");
        }
}

static void SubscribeCloudConnOnlineEventCallbackHandler(
    rbusHandle_t handle,
    rbusEvent_t const* event,
    rbusEventSubscription_t* subscription)
{

    rbusValue_t newValue = rbusObject_GetValue(event->data, "value");
    rbusValue_t oldValue = rbusObject_GetValue(event->data, "oldValue");
    
    int incoming_value = rbusValue_GetInt32(newValue);

    WalInfo("Received on subscribe callback event %s\n", event->name);

    if(newValue)
        WalInfo("  New Value: %d\n", incoming_value);

    if(oldValue)
        WalInfo("  Old Value: %d\n", rbusValue_GetInt32(oldValue));

    if(incoming_value)
    {
    	WalInfo("subscribe callback incoming_value: %d\n", newValue);
	WalInfo("Received cloud connection online event\n");
	pthread_mutex_lock (get_global_sync_mutex());
	//Triggering cloud connection online event for retrying sync notification.
	pthread_cond_signal(get_global_sync_condition());
	pthread_mutex_unlock(get_global_sync_mutex());
    }
    (void)handle;
}

void SubscribeCloudConnOnlineEvent()
{
	int rc = RBUS_ERROR_SUCCESS;
	WalPrint("================= CloudConnOnlineEventSubscribeHandler ==========\n");	
    	WalInfo("rbus event subscribe to cloud connection online subscribe callback\n");
    	if(isRbusInitialized)
    	{
    		rc = rbusEvent_Subscribe(rbus_handle, CLOUD_CONN_ONLINE, SubscribeCloudConnOnlineEventCallbackHandler, NULL, 0);
    		if(rc != RBUS_ERROR_SUCCESS)
		{
			WalError("consumer: rbusEvent_Subscribe for %s failed: %d\n", CLOUD_CONN_ONLINE, rc);
			return NULL;
		}
		else
		{
			WalInfo("consumer: rbusEvent_Subscribe for %s success\n", CLOUD_CONN_ONLINE);
		}
	}
	else 
	{
		WalError("Rbus not initialized in CloudConnOnlineEventSubscribeHandler funcion\n");
        }
}

