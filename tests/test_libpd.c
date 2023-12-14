/**
 *  Copyright 2010-2016 Comcast Cable Communications Management, LLC
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
#include <setjmp.h>
#include <cmocka.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <CUnit/Basic.h>

#include "../source/include/webpa_adapter.h"
#include "../source/app/libpd.h"
#include <cimplog/cimplog.h>
#include <libparodus.h>

#define UNUSED(x) (void )(x)
#ifdef BUILD_YOCTO
#define DEVICE_PROPS_FILE       "/etc/device.properties"
#else
#define DEVICE_PROPS_FILE       "/tmp/device.properties"
#endif
/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
extern libpd_instance_t current_instance;
extern int wakeUpFlag;
int numLoops=1;
/*----------------------------------------------------------------------------*/
/*                                   Mocks                                    */
/*----------------------------------------------------------------------------*/
int writeToDBFile(char *db_file_path, char *data, size_t size)
{
	int file_descriptor1 = open(db_file_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (file_descriptor1 != -1) {
        if (data == NULL)
        {
       	    close(file_descriptor1);              
            return 0;
        }
        else
        {
            write(file_descriptor1, data, strlen(data));    
            close(file_descriptor1);      
            return 1;       
        }
    }
    return 0;
}

void clearTraceContext()
{

}

void getCurrentTime(struct timespec *timer)
{
    clock_gettime(CLOCK_REALTIME, timer); 
}

long timeValDiff(struct timespec *starttime, struct timespec *finishtime)
{
    long msec;
    msec=(finishtime->tv_sec-starttime->tv_sec)*1000;
    msec+=(finishtime->tv_nsec-starttime->tv_nsec)/1000000;
    return msec;
}

void processRequest(char *reqPayload, char *transactionId, char **resPayload, headers_t *req_headers, headers_t *res_headers)
{
    UNUSED(reqPayload);
    UNUSED(transactionId);
    if(reqPayload == NULL)
        UNUSED(resPayload);
    else
        *resPayload = strdup("{\"parameters\":[{\"name\":\"Device.DeviceInfo.TestApp\",\"value\":\"ACTIVE\",\"dataType\":0,\"parameterCount\":1,\"message\":\"Success\"}],\"statusCode\":200}");
    UNUSED(req_headers);
    if(req_headers == NULL)
    {
        UNUSED(res_headers);
    }
    else
    {
        res_headers->count = req_headers->count;
        res_headers->headers[0] = strdup(req_headers->headers[0]);
        res_headers->headers[1] = strdup(req_headers->headers[1]);                            
    }
}

int libparodus_init (libpd_instance_t *instance, libpd_cfg_t *libpd_cfg)
{
    UNUSED(instance);
    UNUSED(libpd_cfg);
    function_called();
    return (int) mock();
}
int libparodus_send (libpd_instance_t instance, wrp_msg_t *msg)
{
    UNUSED(instance);
    UNUSED(msg);
    function_called();
    return (int) mock();
}
wrp_msg_t *msg_tmp = NULL;
int libparodus_receive (libpd_instance_t instance, wrp_msg_t **msg, uint32_t ms)
{
    UNUSED(instance);
    if(msg_tmp == NULL)
        UNUSED(msg);  
    else
        *msg = msg_tmp;
    UNUSED(ms);        
    function_called();
    return (int) mock();    
}

unsigned int sleep(unsigned int seconds)
{
    struct timespec delay;

    delay.tv_sec = seconds / 1000;
    delay.tv_nsec = seconds % 1000 * 1000000;

    nanosleep( &delay, NULL );

    return seconds;
}
/*----------------------------------------------------------------------------*/
/*                                   Tests                                    */
/*----------------------------------------------------------------------------*/

void test_libpd_client_mgr()
{    
        will_return(libparodus_init, (intptr_t)1);
        expect_function_call(libparodus_init);
        will_return(libparodus_init, (intptr_t)1);        
        expect_function_call(libparodus_init);        
        will_return(libparodus_init, (intptr_t)1);        
        expect_function_call(libparodus_init);     
        will_return(libparodus_init, (intptr_t)1);        
        expect_function_call(libparodus_init);                     
        will_return(libparodus_init, (intptr_t)0);
        expect_function_call(libparodus_init);
        libpd_client_mgr();           
}

void err_libpd_client_mgr()
{
        char *parodus_url = "PARODUS_URL=tcp://127.0.0.1:6666\nWEBPA_CLIENT_URL=tcp://127.0.0.1:6667\n";
        writeToDBFile(DEVICE_PROPS_FILE,NULL,0);    
        will_return(libparodus_init, (intptr_t)0);
        expect_function_call(libparodus_init);
        libpd_client_mgr();  

        writeToDBFile(DEVICE_PROPS_FILE,parodus_url,strlen(parodus_url));          
        will_return(libparodus_init, (intptr_t)0);
        expect_function_call(libparodus_init);
        libpd_client_mgr();
        remove(DEVICE_PROPS_FILE);    
}

void test_sendNotification()
{
    char *source =  (char *) malloc(sizeof(char)*16);
    char *payload =  (char *) malloc(sizeof(char)*100);
    char destination[16] = "destination";
    strcpy(source, "source");
    strcpy(payload, "Hello Webpa!");

    will_return(libparodus_send, (intptr_t)0);
    expect_function_call(libparodus_send);
    sendNotification(payload, source, destination);
}

void err_sendNotification()
{
    char *source =  (char *) malloc(sizeof(char)*16);
    char *payload =  (char *) malloc(sizeof(char)*100);
    char destination[16] = "destination";
    strcpy(source, "source");
    strcpy(payload, "Hello Webpa!");

    will_return(libparodus_send, (intptr_t)-404);
    will_return(libparodus_send, (intptr_t)-404);
    will_return(libparodus_send, (intptr_t)-404);
    will_return(libparodus_send, (intptr_t)-404);
    expect_function_calls(libparodus_send, 4);
    sendNotification(payload, source, destination);
}

void test_getConnCloudStatus()
{
	char *status= strdup("online");
	set_global_cloud_status(status);
	will_return(libparodus_send, (intptr_t)0);
    expect_function_call(libparodus_send);
	int ret = getConnCloudStatus("14cxxxxxxx44");
	assert_int_equal(1,ret);
}

void test_getConnCloudStatusOffline()
{
	char *status= strdup("offline");
	set_global_cloud_status(status);
	int ret = getConnCloudStatus("14cxxxxxxx44");
	assert_int_equal(-1,ret);
	will_return(libparodus_send, (intptr_t)0);
    expect_function_call(libparodus_send);
	will_return(libparodus_send, (intptr_t)1);
    expect_function_call(libparodus_send);
	ret = getConnCloudStatus("14cxxxxxxx44");
	assert_int_equal(-1,ret);
}

void err_getConnCloudStatus()
{
	int ret = getConnCloudStatus(NULL);
	assert_int_equal(-1,ret);
	ret = getConnCloudStatus("");
	assert_int_equal(-1,ret);
}

void test_parallelProcessTask()
{
    numLoops = 1;
    msg_tmp = (wrp_msg_t *)malloc(sizeof(wrp_msg_t));
    memset(msg_tmp, 0, sizeof(wrp_msg_t));
    msg_tmp->msg_type = WRP_MSG_TYPE__REQ;
    msg_tmp->u.req.headers = (headers_t *)malloc(sizeof(headers_t));
    msg_tmp->u.req.headers->count = 2; 
    msg_tmp->u.req.payload  = strdup("{\"parameters\":[{\"name\":\"Device.DeviceInfo.TestingApp\",\"value\":\"ACTIVE\",\"dataType\":0,\"parameterCount\":1,\"message\":\"Success\"}],\"statusCode\":200}");
    msg_tmp->u.req.transaction_uuid = strdup("RYjWZTaP4TTTkZ6HhwjGLA");
    msg_tmp->u.req.headers->headers[0] = strdup("123");
    msg_tmp->u.req.headers->headers[1] = strdup("xyz");  
    msg_tmp->u.req.source = strdup("dns:uvxyz.webpa.comcast.net");
    msg_tmp->u.req.dest = strdup("mac:dcebxxxxxxxx/config");
    will_return(libparodus_receive, (intptr_t)0);
    expect_function_call(libparodus_receive);  
    will_return(libparodus_send, (intptr_t)0);
    expect_function_call(libparodus_send);
    parallelProcessTask(NULL);             
}

void test_cloudstatus_parallelProcessTask()
{
    numLoops = 1;
    msg_tmp = (wrp_msg_t *)malloc(sizeof(wrp_msg_t));
    memset(msg_tmp, 0, sizeof(wrp_msg_t));
    msg_tmp->msg_type = WRP_MSG_TYPE__RETREIVE;
    msg_tmp->u.crud.source  = strdup("mac:5896xxxxxxxx/parodus/cloud-status");
    msg_tmp->u.crud.transaction_uuid = strdup("f804ba0d-fd7e-42b0-a00e-27c669da4b6e"); 
    msg_tmp->u.crud.status = 0;
    msg_tmp->u.crud.payload = strdup("{\"cloud-status\":\"online\"}");
    will_return(libparodus_receive, (intptr_t)0);
    expect_function_call(libparodus_receive);   
    parallelProcessTask(NULL);        
}
void err_parallelProcessTask()
{
    numLoops = 2;
    will_return(libparodus_receive, (intptr_t)1);
    expect_function_call(libparodus_receive);    
    will_return(libparodus_receive, (intptr_t)2);
    expect_function_call(libparodus_receive);       
    parallelProcessTask(NULL); 
}

void test_rdk_logger_module_fetch()
{
    rdk_logger_module_fetch();
}

void test_parsePayloadForStatus()
{
    char *status = NULL;
    status = parsePayloadForStatus(NULL);
    assert_int_equal(status,NULL);
    status = parsePayloadForStatus("{\"cloudstatus\":\"online\"}");
    assert_int_equal(status,NULL);
    status = parsePayloadForStatus("{\"cloud-status\":\"\"}");
    assert_int_equal(status,NULL);
}    

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_libpd_client_mgr),
        cmocka_unit_test(err_libpd_client_mgr),
        cmocka_unit_test(test_sendNotification),
        cmocka_unit_test(err_sendNotification),
		cmocka_unit_test(test_getConnCloudStatus),
		cmocka_unit_test(test_getConnCloudStatusOffline),
		cmocka_unit_test(err_getConnCloudStatus),
        cmocka_unit_test(test_parallelProcessTask),
        cmocka_unit_test(test_cloudstatus_parallelProcessTask),
        cmocka_unit_test(err_parallelProcessTask),
        cmocka_unit_test(test_rdk_logger_module_fetch),
        cmocka_unit_test(test_parsePayloadForStatus) 
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
