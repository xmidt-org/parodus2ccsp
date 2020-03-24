/*
 * If not stated otherwise in this file or this component's Licenses.txt file the
 * following copyright and licenses apply:
 *
 * Copyright 2015 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/

/**********************************************************************
   Copyright [2014] [Cisco Systems, Inc.]
 
   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at
 
       http://www.apache.org/licenses/LICENSE-2.0
 
   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
**********************************************************************/


/**************************************************************************

    module: cosa_webconfig_internal.h

        For COSA Data Model Library Development

    -------------------------------------------------------------------

    copyright:

        Cisco Systems, Inc.
        All Rights Reserved.

    -------------------------------------------------------------------

    description:

        This file defines the apis for objects to support Data Model Library.

    -------------------------------------------------------------------


    author:

        COSA XML TOOL CODE GENERATOR 1.0

    -------------------------------------------------------------------

    revision:

        01/14/2011    initial revision.

**************************************************************************/


#ifndef  _COSA_WEBCONFIG_INTERNAL_H
#define  _COSA_WEBCONFIG_INTERNAL_H

#include "webpa_internal.h"

BOOL Get_RfcEnable();
int setRfcEnable(BOOL bValue);
int Get_Webconfig_URL( char **pString);
int Set_Webconfig_URL( char *pString);
int getConfigNumberOfEntries();
int Get_PeriodicSyncCheckInterval();
int setPeriodicSyncCheckInterval(int iValue);
BOOL getConfigURL(int index,char **configURL);
int setConfigURL(int index, char *configURL);
BOOL getRequestTimeStamp(int index,char **RequestTimeStamp);
int setRequestTimeStamp(int index);
BOOL getConfigVersion(int index, char **version);
int setConfigVersion(int index, char *version);
BOOL getSyncCheckOK(int index,BOOL *pvalue );
int setSyncCheckOK(int index, BOOL status);
BOOL getForceSyncCheck(int index,BOOL *pvalue, char** transactionID );
BOOL setForceSyncCheck(int index, BOOL pvalue, char *transactionId,int *session_status);
BOOL isValidInstanceNumber(int instNum);

BOOL getConfigURLFromWebConfigCtx(ANSC_HANDLE hInsContext, char *pValue);
BOOL setConfigURLWithWebConfigCtx(ANSC_HANDLE hInsContext, char *pValue);
BOOL getRequestTimeStampFromWebConfigCtx(ANSC_HANDLE hInsContext, char *pValue);
BOOL setRequestTimeStampWithWebConfigCtx(ANSC_HANDLE hInsContext, char *pValue);
BOOL getConfigVersionFromWebConfigCtx(ANSC_HANDLE hInsContext, char *pValue);
BOOL setConfigVersionWithWebConfigCtx(ANSC_HANDLE hInsContext, char *pValue);
BOOL getSyncCheckOKFromWebConfigCtx(ANSC_HANDLE hInsContext, BOOL *pBool );
BOOL setSyncCheckOKWithWebConfigCtx(ANSC_HANDLE hInsContext, BOOL status);
BOOL getForceSyncCheckFromWebConfigCtx(ANSC_HANDLE hInsContext, BOOL *pBool, char *pTransValue );
BOOL setForceSyncCheckWithWebConfigCtx(ANSC_HANDLE hInsContext, BOOL bValue, char *transactionId,int *pStatus);

int getWebConfigParameterValues(char **parameterNames, int paramCount, int *val_size, parameterValStruct_t ***val);

int setWebConfigParameterValues(parameterValStruct_t *val, int paramCount, char **faultParam, char *transactionId );

#endif
