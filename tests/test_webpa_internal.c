#include<stdio.h>
#include <CUnit/Basic.h>

#include "../source/include/webpa_rbus.h"
#include "../source/include/webpa_adapter.h"
#include "../source/broadband/include/webpa_internal.h"
#include "../source/app/libpd.h"

#define TEST_COMPONENT_NAME "TestComp"
#define UNUSED(x) (void )(x)

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
rbusHandle_t handle;
ANSC_HANDLE bus_handle;
int numLoops;
//To control the checkIfSystemReady() state
static int cr_ready_state = 0;

// Extern variable from webpa_internal.c to check caching completion
extern int cachingStatus;

/*----------------------------------------------------------------------------*/
/*                             Helper Functions                               */
/*----------------------------------------------------------------------------*/
/**
 * @brief Wait for component caching to complete by polling cachingStatus
 * @param timeout_seconds Maximum time to wait in seconds
 * @return 0 on success, -1 on timeout
 */
static int waitForComponentCachingComplete(int timeout_seconds)
{
    int elapsed = 0;

    WalPrint("Waiting for component caching to complete (timeout: %d seconds)\\n", timeout_seconds);

    while (elapsed < timeout_seconds) {
        if (cachingStatus == 1) {
            WalPrint("Component caching completed after %d seconds\\n", elapsed);
            return 0;  // Success
        }
        sleep(10);
        elapsed += 10;
    }

    return -1;  // Timeout
}

/*----------------------------------------------------------------------------*/
/*                                   Mocks                                    */
/*----------------------------------------------------------------------------*/
void free_parameterValStruct_t (void* bus_handle, int size,parameterValStruct_t **val)
{
    UNUSED(bus_handle); UNUSED(size); UNUSED(val);
}

int getWebpaParameterValues(char **parameterNames, int paramCount, int *val_size, parameterValStruct_t ***val)
{
    UNUSED(parameterNames); UNUSED(paramCount); UNUSED(val_size); UNUSED(val);
    return 0;
}

int setWebpaParameterValues(parameterValStruct_t *val, int paramCount, char **faultParam )
{
    UNUSED(faultParam); UNUSED(paramCount); UNUSED(val);
    return 0;
}

void setAttributes(param_t *attArr, const unsigned int paramCount, money_trace_spans *timeSpan, WDMP_STATUS *retStatus)
{
    UNUSED(attArr); UNUSED(paramCount); UNUSED(timeSpan); UNUSED(retStatus);
    return;
}

void addRowTable(char *objectName, TableData *list,char **retObject, WDMP_STATUS *retStatus)
{
    UNUSED(objectName); UNUSED(list); UNUSED(retObject); UNUSED(retStatus);
    return;
}
void deleteRowTable(char *object,WDMP_STATUS *retStatus)
{
    UNUSED(object); UNUSED(retStatus);
    return;
}

void replaceTable(char *objectName,TableData * list,unsigned int paramcount,WDMP_STATUS *retStatus)
{
    UNUSED(objectName); UNUSED(list); UNUSED(paramcount); UNUSED(retStatus);
    return;
}
void getAttributes(const char *paramName[], const unsigned int paramCount, money_trace_spans *timeSpan, param_t **attr, int *retAttrCount, WDMP_STATUS *retStatus)
{
    UNUSED(paramName); UNUSED(paramCount); UNUSED(timeSpan); UNUSED(attr); UNUSED(retAttrCount); UNUSED(retStatus);
    return;
}

int CcspBaseIf_discComponentSupportingNamespace ( void* bus_handle, const char* dst_component_id, const char *name_space, const char *subsystem_prefix, componentStruct_t ***components, int *size)
{
    UNUSED(bus_handle); UNUSED(dst_component_id); UNUSED(name_space); UNUSED(subsystem_prefix); UNUSED(components); UNUSED(size);
    return 0;
}

int CcspBaseIf_isSystemReady (void* bus_handle, const char* dst_component_id, dbus_bool *val)
{
    UNUSED(bus_handle); UNUSED(dst_component_id);
    if(cr_ready_state)
    {
        *val = 1;
    }
    else
    {
        *val = 0;
    }
    return 1;
}

int  CcspBaseIf_Register_Event(void* bus_handle, const char* sender, const char* event_name)
{
   UNUSED(bus_handle); UNUSED(sender); UNUSED(event_name);
   return 0;
}

void CcspBaseIf_SetCallback2(void* bus_handle, char *name, void*  func, void * user_data)
{
    UNUSED(bus_handle); UNUSED(name); UNUSED(func); UNUSED(user_data);
    return;
}

int CcspBaseIf_getParameterValues(void* bus_handle, const char* dst_component_id, char* dbus_path, char * parameterNames[], int size, int *val_size, parameterValStruct_t ***val)
{
    UNUSED(bus_handle); UNUSED(dst_component_id); UNUSED(dbus_path); UNUSED(parameterNames);
    size = 1;
    parameterValStruct_t **temp = (parameterValStruct_t**)malloc(sizeof(parameterValStruct_t));

    temp[0] = (parameterValStruct_t *)malloc(sizeof(parameterValStruct_t));
    temp[0]->parameterName = strdup("Testname");
    temp[0]->parameterValue = strdup("Green");
    *val = temp;
    *val_size = 1;
    return CCSP_SUCCESS;
}

/*----------------------------------------------------------------------------*/
/*                                   Tests                                    */
/*----------------------------------------------------------------------------*/
// Test case for component cache with component fail but CR ready
void test_initComponentCaching_cr_ready_failure()
{
    WalInfo("\n**************************************************\n");
    int ret = -1;
    const char *pComponentName = TEST_COMPONENT_NAME;
	WalInfo("********** Starting component: %s **********\n ", pComponentName);
        drop_root_privilege();
	/* Backend Manager for Webpa Creation and Initilization
    CosaWebpaBEManagerCreate( );*/
	if(isRbusEnabled())
	{
	        webpaRbusInit(pComponentName);
    }
    ret = 0;
    cr_ready_state = 1;
    initComponentCaching(ret);
    // Wait for component caching to complete (with retries for all failed components)
    waitForComponentCachingComplete(200);
	webpaRbus_Uninit();
}

// Test case for component cache file available but component fail and CR not ready
void test_initComponentCaching_cr_notready_cache_present_failure()
{
    WalInfo("\n**************************************************\n");
    int ret = -1;
    const char *pComponentName = TEST_COMPONENT_NAME;
	WalInfo("********** Starting component: %s **********\n ", pComponentName);
    drop_root_privilege();
	if(isRbusEnabled())
	{
	        webpaRbusInit(pComponentName);
    }
    ret = 0;
    cr_ready_state = 0;
    initComponentCaching(ret);
    // Wait for component caching to complete (CR not ready scenario)
    waitForComponentCachingComplete(200);
    system("rm /var/tmp/cacheready");
	webpaRbus_Uninit();
}

// Test case for component cache with component fail and CR not ready
void test_initComponentCaching_cr_notready_failure()
{
    WalInfo("\n**************************************************\n");
    int ret = -1;
    const char *pComponentName = TEST_COMPONENT_NAME;
	WalInfo("********** Starting component: %s **********\n ", pComponentName); 
    drop_root_privilege();
	if(isRbusEnabled())
	{
	        webpaRbusInit(pComponentName);
    }
    ret = 0;
    cr_ready_state = 0;
    initComponentCaching(ret);
    // Wait for component caching to complete (CR not ready, no cache scenario)
    waitForComponentCachingComplete(200);  // Allow up to 200 seconds
	webpaRbus_Uninit();
}

//Test case for getCurrentTimeInMicroSeconds
void test_getCurrentTimeInMicroSeconds()
{
    struct timespec timer;
    uint64_t res = getCurrentTimeInMicroSeconds(&timer);
    CU_ASSERT_NOT_EQUAL(res, 0);
}

//Test case for timeValDiff
void test_timeValDiff()
{
	struct timespec start, finish;
	start.tv_sec = 1735187400;
	start.tv_nsec = 0;

	finish.tv_sec = 1735187800; //400sec diff
	finish.tv_nsec = 0;

	long diff = timeValDiff(&start, &finish);
	CU_ASSERT_EQUAL(400000, diff); //400000msec = 400sec
}

//Test case for waitForOperationalReadyCondition
void test_waitForOperationalReadyCondition_success()
{
    WalInfo("\n**************************************************\n");
    int ret = -1;
    const char *pComponentName = TEST_COMPONENT_NAME;
	WalInfo("********** Starting component: %s **********\n ", pComponentName);
    drop_root_privilege();
	if(isRbusEnabled())
	{
	        webpaRbusInit(pComponentName);
    }
	ret = waitForOperationalReadyCondition();
    webpaRbus_Uninit();
}

//Test Case to check mapstatus CCSP_ERR_TIMEOUT
void test_ccsp_err_timeout()
{
    int input = CCSP_ERR_TIMEOUT;
    WDMP_STATUS ret = mapStatus(input);
    CU_ASSERT_EQUAL(WDMP_ERR_TIMEOUT, ret);
}

//Test Case to check mapstatus CCSP_ERR_NOT_EXIST
void test_ccsp_err_not_exist()
{
    int input = CCSP_ERR_NOT_EXIST;
    WDMP_STATUS ret = mapStatus(input);
    CU_ASSERT_EQUAL(WDMP_ERR_NOT_EXIST, ret);
}

//Test Case to check mapstatus CCSP_ERR_INVALID_PARAMETER_VALUE
void test_ccsp_err_invalid_parameter_value()
{
    int input = CCSP_ERR_INVALID_PARAMETER_VALUE;
    WDMP_STATUS ret = mapStatus(input);
    CU_ASSERT_EQUAL(WDMP_ERR_INVALID_PARAMETER_VALUE, ret);
}

//Test Case to check mapstatus CCSP_ERR_SETATTRIBUTE_REJECTED
void test_ccsp_err_setattribute_rejected()
{
    int input = CCSP_ERR_SETATTRIBUTE_REJECTED;
    WDMP_STATUS ret = mapStatus(input);
    CU_ASSERT_EQUAL(WDMP_ERR_SETATTRIBUTE_REJECTED, ret);
}

//Test Case to check mapstatus CCSP_ERR_REQUEST_REJECTED
void test_ccsp_err_request_rejected()
{
    int input = CCSP_ERR_REQUEST_REJECTED;
    WDMP_STATUS ret = mapStatus(input);
    CU_ASSERT_EQUAL(WDMP_ERR_REQUEST_REJECTED, ret);
}

//Test Case to check mapstatus CCSP_CR_ERR_NAMESPACE_OVERLAP
void test_ccsp_err_namespace_overlap()
{
    int input = CCSP_CR_ERR_NAMESPACE_OVERLAP;
    WDMP_STATUS ret = mapStatus(input);
    CU_ASSERT_EQUAL(WDMP_ERR_NAMESPACE_OVERLAP, ret);
}

//Test Case to check mapstatus CCSP_ERR_TIMEOUT
void test_ccsp_err_unknown_component()
{
    int input = CCSP_CR_ERR_UNKNOWN_COMPONENT;
    WDMP_STATUS ret = mapStatus(input);
    CU_ASSERT_EQUAL(WDMP_ERR_UNKNOWN_COMPONENT, ret);
}

//Test Case to check mapstatus CCSP_CR_ERR_NAMESPACE_MISMATCH
void test_ccsp_err_namespace_mismatch()
{
    int input = CCSP_CR_ERR_NAMESPACE_MISMATCH;
    WDMP_STATUS ret = mapStatus(input);
    CU_ASSERT_EQUAL(WDMP_ERR_NAMESPACE_MISMATCH, ret);
}

//Test Case to check mapstatus CCSP_CR_ERR_DP_COMPONENT_VERSION_MISMATCH
void test_ccsp_err_dp_component_version_mismatch()
{
    int input = CCSP_CR_ERR_DP_COMPONENT_VERSION_MISMATCH;
    WDMP_STATUS ret = mapStatus(input);
    CU_ASSERT_EQUAL(WDMP_ERR_DP_COMPONENT_VERSION_MISMATCH, ret);
}

//Test Case to check mapstatus CCSP_CR_ERR_UNSUPPORTED_DATATYPE
void test_ccsp_err_unsupported_datatype()
{
    int input = CCSP_CR_ERR_UNSUPPORTED_DATATYPE;
    WDMP_STATUS ret = mapStatus(input);
    CU_ASSERT_EQUAL(WDMP_ERR_UNSUPPORTED_DATATYPE, ret);
}

//Test Case to check mapstatus CCSP_ERR_METHOD_NOT_SUPPORTED
void test_ccsp_err_method_not_supported()
{
    int input = CCSP_ERR_METHOD_NOT_SUPPORTED;
    WDMP_STATUS ret = mapStatus(input);
    CU_ASSERT_EQUAL(WDMP_ERR_METHOD_NOT_SUPPORTED, ret);
}

//Test Case to check mapstatus CCSP_CR_ERR_SESSION_IN_PROGRESS
void test_ccsp_err_session_in_progress()
{
    int input = CCSP_CR_ERR_SESSION_IN_PROGRESS;
    WDMP_STATUS ret = mapStatus(input);
    CU_ASSERT_EQUAL(WDMP_ERR_SESSION_IN_PROGRESS, ret);
}

//Test Case to check mapstatus CCSP_Message_Bus_OOM
void test_ccsp_message_bus_oom()
{
    int input = CCSP_Message_Bus_OOM;
    WDMP_STATUS ret = mapStatus(input);
    CU_ASSERT_EQUAL(WDMP_ERR_MAX_REQUEST, ret);
}

void add_suites( CU_pSuite *suite )
{
	*suite = CU_add_suite( "tests", NULL, NULL );
    CU_add_test( *suite, "test initComponentCaching_cr_ready_failure", test_initComponentCaching_cr_ready_failure);
    CU_add_test( *suite, "test initComponentCaching_cr_notready_cache_present_failure", test_initComponentCaching_cr_notready_cache_present_failure);
    CU_add_test( *suite, "test initComponentCaching_cr_notready_failure", test_initComponentCaching_cr_notready_failure);
    CU_add_test( *suite, "test getCurrentTimeInMicroSeconds", test_getCurrentTimeInMicroSeconds);
    CU_add_test( *suite, "test timeValDiff", test_timeValDiff);
    CU_add_test( *suite, "test waitForOperationalReadyCondition_success", test_waitForOperationalReadyCondition_success);
    CU_add_test( *suite, "test ccsp_err_timeout", test_ccsp_err_timeout);
    CU_add_test( *suite, "test ccsp_err_not_exist", test_ccsp_err_not_exist);
    CU_add_test( *suite, "test ccsp_err_invalid_parameter_value", test_ccsp_err_invalid_parameter_value);
    CU_add_test( *suite, "test ccsp_err_setattribute_rejected", test_ccsp_err_setattribute_rejected);
    CU_add_test( *suite, "test ccsp_err_request_rejected", test_ccsp_err_request_rejected);
    CU_add_test( *suite, "test ccsp_err_namespace_overlap", test_ccsp_err_namespace_overlap);
    CU_add_test( *suite, "test ccsp_err_unknown_component", test_ccsp_err_unknown_component);
    CU_add_test( *suite, "test ccsp_err_namespace_mismatch", test_ccsp_err_namespace_mismatch);
    CU_add_test( *suite, "test ccsp_err_dp_component_version_mismatch", test_ccsp_err_dp_component_version_mismatch);
    CU_add_test( *suite, "test ccsp_err_unsupported_datatype", test_ccsp_err_unsupported_datatype);
    CU_add_test( *suite, "test ccsp_err_method_not_supported", test_ccsp_err_method_not_supported);
    CU_add_test( *suite, "test ccsp_err_session_in_progress", test_ccsp_err_session_in_progress);
    CU_add_test( *suite, "test ccsp_message_bus_oom", test_ccsp_message_bus_oom);
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
