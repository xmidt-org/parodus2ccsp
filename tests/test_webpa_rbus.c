#include<stdio.h>
#include <CUnit/Basic.h>
#include <pthread.h>
#include "../source/include/webpa_rbus.h"

rbusHandle_t handle;
int cloud_online_subscribe = 0;
    
pthread_mutex_t sync_mutex=PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t sync_condition=PTHREAD_COND_INITIALIZER;

pthread_cond_t *get_global_sync_condition(void)
{
   return &sync_condition;
}

pthread_mutex_t *get_global_sync_mutex(void)
{
        return &sync_mutex;
}

void register_event(char * eventname, rbusDataElement_t* dataelement)
{
	int rc = RBUS_ERROR_SUCCESS;
	WalInfo("rbus_open for component %s\n", "component_webpa");
	rc = rbus_open(&handle, "component_webpa");
	if(rc != RBUS_ERROR_SUCCESS)
	{
		WalError("rbus_open failed for subscribe_to_event");
	}

	if(strncmp(eventname, CLOUD_CONN_ONLINE, strlen(CLOUD_CONN_ONLINE)) == 0)
	{
		WalInfo("Inside register_event for %s and eventname is %s\n", CLOUD_CONN_ONLINE, eventname);
		rc = rbus_regDataElements(handle, 1, dataelement);
	}

	if(rc != RBUS_ERROR_SUCCESS)
		WalError("register_event %s failed\n", eventname);
}
void Unregister_event(char * eventname, rbusDataElement_t* dataelement)
{
	int rc = RBUS_ERROR_SUCCESS;
	if(rc != RBUS_ERROR_SUCCESS)
	{
		WalError("rbus_open failed for subscribe_to_event");
	}

	if(strncmp(eventname, CLOUD_CONN_ONLINE, strlen(CLOUD_CONN_ONLINE)) == 0)
	{
		WalInfo("Inside Unregister_event for %s and eventname is %s\n", CLOUD_CONN_ONLINE, eventname);
		rc = rbus_unregDataElements(handle, 1, dataelement);
	}

	if(rc != RBUS_ERROR_SUCCESS)
		WalError("register_event %s failed\n", eventname);
		
	rbus_close(handle);
	handle = NULL;
}

rbusError_t CloudConnectionSubscribeHandler(rbusHandle_t handle, rbusEventSubAction_t action, const char* eventName, rbusFilter_t filter, int32_t interval, bool* autoPublish)
{
    (void)handle;
    (void)filter;
    (void)autoPublish;
    (void)interval;

    WalInfo("CloudConnectionSubscribeHandler: action=%s eventName=%s", action == RBUS_EVENT_ACTION_SUBSCRIBE ? "subscribe" : "unsubscribe", eventName);

    if(!strcmp(CLOUD_CONN_ONLINE, eventName))
    {
        cloud_online_subscribe = action == RBUS_EVENT_ACTION_SUBSCRIBE ? 1 : 0;
    }
    else
    {
        WalError("provider: CloudConnectionSubscribeHandler unexpected eventName %s\n", eventName);
    }

    return RBUS_ERROR_SUCCESS;
}

rbusError_t Publish_event()
{
	rbusError_t rc = RBUS_ERROR_BUS_ERROR;
	if(cloud_online_subscribe)
	{
		rbusEvent_t event = {0};
		rbusObject_t data;
		rbusValue_t value;

		WalInfo("publishing cloud connection online Event\n");

		rbusValue_Init(&value);
		rbusValue_SetInt32(value, 1);

		rbusObject_Init(&data, NULL);
		rbusObject_SetValue(data, "value", value);

		event.name = CLOUD_CONN_ONLINE;
		event.data = data;
		event.type = RBUS_EVENT_GENERAL;

		rc = rbusEvent_Publish(handle, &event);

		rbusValue_Release(value);
		rbusObject_Release(data);

		if(rc != RBUS_ERROR_SUCCESS)
			WalError("rbus event Publish cloud connection online event failed: %d\n", rc);
	}
	return rc;
}

// Test case for isRbusEnabled
void test_isRbusEnabled_success()
{
    WalInfo("\n**************************************************\n");
	bool result = isRbusEnabled();
	CU_ASSERT_TRUE(result);
}

// Test case for isRbusInitialized success
void test_isRbusInitialized_success()
{
    WalInfo("\n**************************************************\n");
	webpaRbusInit("componentName");
	bool result = isRbusInitialized();
        printf("The bool value is %d\n", result);

	CU_ASSERT_TRUE(result);
	webpaRbus_Uninit();
}

void test_webpaRbusInit_success()
{
    WalInfo("\n**************************************************\n");
    int result = webpaRbusInit("component");
    CU_ASSERT_EQUAL(result, 0);
}

//Successcase for setTraceContext
void test_setTraceContext_success()
{
    WalInfo("\n**************************************************\n");
    rbusError_t rc = RBUS_ERROR_BUS_ERROR;
    char* traceContext[2];

    //allocating moemory for each element
    traceContext[0] = strdup("randomvalueone");
    traceContext[1] = strdup("randomvaluetwo");
    rc = setTraceContext(traceContext);
    CU_ASSERT_EQUAL(0,rc);
    free(traceContext[0]);
    free(traceContext[1]);
}

//traceContext header NULL
void test_setTraceContext_header_NULL()
{   
    WalInfo("\n**************************************************\n");
    rbusError_t rc = RBUS_ERROR_BUS_ERROR;
    char* traceContext[2];
    traceContext[0] = NULL;
    traceContext[1] = NULL;
    rc = setTraceContext(traceContext);
    CU_ASSERT_EQUAL(1,rc);
}

//traceContext header empty
void test_setTraceContext_header_empty()
{
    WalInfo("\n**************************************************\n");
    rbusError_t rc = RBUS_ERROR_BUS_ERROR;
    char* traceContext[2];
    traceContext[0] = "";
    traceContext[1] = "";
    rc = setTraceContext(traceContext);
    CU_ASSERT_EQUAL(1,rc);
}

void test_getTraceContext_success()
{
    WalInfo("\n**************************************************\n");
    rbusError_t rc = RBUS_ERROR_BUS_ERROR;
    char* traceContext[2];

    //allocating moemory for each element
    traceContext[0] = strdup("randomvalueone");
    traceContext[1] = strdup("randomvaluetwo");
    rc = setTraceContext(traceContext);
    CU_ASSERT_EQUAL(0,rc);

    getTraceContext(traceContext);
    CU_ASSERT_EQUAL(0,rc);
    clearTraceContext();
}

void test_getTraceContext_empty()
{
    WalInfo("\n**************************************************\n");
    rbusError_t rc = RBUS_ERROR_BUS_ERROR;
    char* traceContext[2];

    getTraceContext(traceContext);
    CU_ASSERT_EQUAL(1,rc);
    webpaRbus_Uninit();
}

void test_SubscribeCloudConnOnlineEvent_Rbushandle_empty()
{
    WalInfo("\n**************************************************\n");
    int rc = RBUS_ERROR_SUCCESS; 
    rc = SubscribeCloudConnOnlineEvent();
    CU_ASSERT_NOT_EQUAL(0,rc); 
}

void test_SubscribeCloudConnOnlineEvent_success()
{
    WalInfo("\n**************************************************\n");
    rbusDataElement_t SyncRetryElements[1] = {{CLOUD_CONN_ONLINE, RBUS_ELEMENT_TYPE_EVENT, {NULL, NULL, NULL, NULL, CloudConnectionSubscribeHandler, NULL}}};
    register_event(CLOUD_CONN_ONLINE, SyncRetryElements);
    webpaRbusInit("consumer");  
    int rc = RBUS_ERROR_SUCCESS;
    rc = SubscribeCloudConnOnlineEvent();
    CU_ASSERT_EQUAL(0,rc); 
    Unregister_event(CLOUD_CONN_ONLINE, SyncRetryElements);
}

void test_SubscribeCloudConnOnlineEvent_failure()
{
    WalInfo("\n**************************************************\n");
    int rc = RBUS_ERROR_SUCCESS; 
    rc = SubscribeCloudConnOnlineEvent();
    CU_ASSERT_NOT_EQUAL(0,rc); 
}

void test_cloudConnEventHandler_success()
{
    WalInfo("\n**************************************************\n");
    struct timespec ts;
    int backoffdelay = 2;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += backoffdelay;
    rbusDataElement_t SyncRetryElements[1] = {{CLOUD_CONN_ONLINE, RBUS_ELEMENT_TYPE_EVENT, {NULL, NULL, NULL, NULL, CloudConnectionSubscribeHandler, NULL}}};
    register_event(CLOUD_CONN_ONLINE,SyncRetryElements);
    Publish_event();
    int rv = pthread_cond_timedwait(&sync_condition, &sync_mutex, &ts);
    CU_ASSERT_EQUAL(0,rv); 
    Unregister_event(CLOUD_CONN_ONLINE,SyncRetryElements);
}

void add_suites( CU_pSuite *suite )
{
	*suite = CU_add_suite( "tests", NULL, NULL );
    CU_add_test( *suite, "test isRbusEnabled_success", test_isRbusEnabled_success);
    CU_add_test( *suite, "test isRbusInitialized_success", test_isRbusInitialized_success);
    CU_add_test( *suite, "test webpaRbusInit_success", test_webpaRbusInit_success);
    CU_add_test( *suite, "test setTraceContext_success", test_setTraceContext_success);
    CU_add_test( *suite, "test setTraceContext_header_NULL", test_setTraceContext_header_NULL);
    CU_add_test( *suite, "test setTraceContext_header_empty", test_setTraceContext_header_empty);
    CU_add_test( *suite, "test getTraceContext_success", test_getTraceContext_success);
    CU_add_test( *suite, "test getTraceContext_empty", test_getTraceContext_empty);
    CU_add_test( *suite, "test SubscribeCloudConnOnlineEvent_Rbushandle_empty", test_SubscribeCloudConnOnlineEvent_Rbushandle_empty);
    CU_add_test( *suite, "test SubscribeCloudConnOnlineEvent_success", test_SubscribeCloudConnOnlineEvent_success);
    CU_add_test( *suite, "test SubscribeCloudConnOnlineEvent_failure", test_SubscribeCloudConnOnlineEvent_failure);
    CU_add_test( *suite, "test cloudConnEventHandler_success", test_cloudConnEventHandler_success);  
}


int main( int argc, char *argv[] )
{
	unsigned rv = 1;
    CU_pSuite suite = NULL;
 
    (void ) argc;
    (void ) argv;
    
    if( CUE_SUCCESS == CU_initialize_registry() ) {
        add_suites( &suite );

        if( NULL != suite ) {
            CU_basic_set_mode( CU_BRM_VERBOSE );
            CU_basic_run_tests();
            printf( "\n" );
            CU_basic_show_failures( CU_get_failure_list() );
            printf( "\n\n" );
            rv = CU_get_number_of_tests_failed();
        }
        CU_cleanup_registry();
    }
    return rv;
}


