#include<stdio.h>
#include <CUnit/Basic.h>

#include "../source/include/webpa_rbus.h"
rbusHandle_t handle;

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
	webpaRbus_Uninit();
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


