/**
 *  Copyright 2017 Comcast Cable Communications Management, LLC
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */
#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <malloc.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdbool.h>
#include <setjmp.h>
#include <pthread.h>
#include <cmocka.h>
#include <CUnit/Basic.h>

#include "mock_dbus.h"
/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
/* None */

/*----------------------------------------------------------------------------*/
/*                                   Mocks                                    */
/*----------------------------------------------------------------------------*/
typedef void* ANSC_HANDLE;

int CCSP_Message_Bus_Init(
    char *component_id,
    char *config_file,
    void **bus_handle,
    CCSP_MESSAGE_BUS_MALLOC mallocfunc,
    CCSP_MESSAGE_BUS_FREE   freefunc
)
{
    (void) component_id;
    (void) config_file;
    (void) bus_handle;
    (void) mallocfunc;
    (void) freefunc;
    function_called();
    return (int) mock();
}

int CCSP_Message_Bus_Register_Path(
    void *bus_handle,
    const char* path,
    DBusObjectPathMessageFunction funcptr,
    void *user_data
)
{
   (void) bus_handle;
   (void) path;
   (void) funcptr;
   (void) user_data;
   function_called();
   return (int) mock();
}

char *pComponentName = NULL;

int g_iTraceLevel = 0;

unsigned long g_ulAllocatedSizePeak = 0;

ANSC_HANDLE g_MessageBusHandle_Irep = NULL;

char g_SubSysPrefix_Irep[32] = {0};

/*----------------------------------------------------------------------------*/
/*                                   Tests                                    */
/*----------------------------------------------------------------------------*/
void test_msgBusInit()
{
    FILE *fh = fopen("/tmp/ccsp_msg.cfg", "w");
    if( NULL == fh ) {
        return;
    }
    fprintf(fh, "tcp:host=localhost,port=12368");
    fclose(fh);

    msgBusInit("parodus2ccsp");

    remove("/tmp/ccsp_msg.cfg");
}

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/
int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_msgBusInit)
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
