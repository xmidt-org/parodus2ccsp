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

#include <ccsp_base_api.h>
#include "../source/include/webpa_adapter.h"
#include "../source/broadband/include/webpa_internal.h"
#define UNUSED(x) (void )(x)
/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
extern int cachingStatus;
extern ComponentVal ComponentValArray[RDKB_TR181_OBJECT_LEVEL1_COUNT];
extern ComponentVal SubComponentValArray[RDKB_TR181_OBJECT_LEVEL2_COUNT];
extern int compCacheSuccessCnt;
extern int subCompCacheSuccessCnt;
int numLoops;
/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/

void getCompDetails()
{
    int i=0;
    int compSizeList[] = {1,1,1,2,1};
    char *compNameList[] = {RDKB_WIFI_FULL_COMPONENT_NAME,"eRT.com.cisco.spvtg.ccsp.webpaagent","com.ccsp.webpa","com.ccsp.pam","com.ccsp.nat"};
    char *dbusPathList[] = {RDKB_WIFI_DBUS_PATH,"/com/ccsp/webpa","/com/ccsp/pam","/com/ccsp/nat"};
    char *objList[]={"Device.WiFi.","Device.WebpaAgent.","Device.Webpa.","Device.DeviceInfo.","Device.NAT."};
    int subCompSizeList[] = {1,1};
    char *subCompNameList[] = {"com.ccsp.webpa","com.ccsp.nat"};
    char *subDbusPathList[] = {"/com/ccsp/webpa","/com/ccsp/nat"};
    char *subObjList[]={"Device.DeviceInfo.Webpa.","Device.NAT.PortMapping."};

    cachingStatus = 1;
    compCacheSuccessCnt = 5;
    subCompCacheSuccessCnt = 2;
    for(i=0; i<compCacheSuccessCnt; i++)
    {
        ComponentValArray[i].comp_id=i;
        ComponentValArray[i].comp_size=compSizeList[i];
        ComponentValArray[i].obj_name=objList[i];
        ComponentValArray[i].comp_name=compNameList[i];
        ComponentValArray[i].dbus_path=dbusPathList[i];
    }

    for(i=0; i<subCompCacheSuccessCnt; i++)
    {
        SubComponentValArray[i].comp_id=i;
        SubComponentValArray[i].comp_size=subCompSizeList[i];
        SubComponentValArray[i].obj_name=subObjList[i];
        SubComponentValArray[i].comp_name=subCompNameList[i];
        SubComponentValArray[i].dbus_path=subDbusPathList[i];
    }
}

componentStruct_t **getDeviceInfoCompDetails()
{
    int count = 2, i = 0;
    char *compNames[] = {"com.ccsp.pam","com.ccsp.webpa"};
    char *dbusPaths[] = {"/com/ccsp/pam","/com/ccsp/webpa"};
    componentStruct_t **list = (componentStruct_t **) malloc(sizeof(componentStruct_t *)*count);
    for(i=0; i<count; i++)
    {
        list[i] = (componentStruct_t *) malloc(sizeof(componentStruct_t));
        list[i]->componentName = strdup(compNames[i]);
        list[i]->dbusPath = strdup(dbusPaths[i]);
    }
    return list;
}
