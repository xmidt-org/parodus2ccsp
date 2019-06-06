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

int getConfigNumberOfEntries();
BOOL getConfigURL(int index,char **configURL);
int setConfigURL(int index, char *configURL);
BOOL getPreviousSyncDateTime(int index,char **PreviousSyncDateTime);
int setPreviousSyncDateTime(int index);
BOOL getConfigVersion(int index, char **version);
int setConfigVersion(int index, char *version);
BOOL getSyncCheckOK(int index,BOOL *pvalue );
int setSyncCheckOK(int index, BOOL status);
BOOL getForceSyncCheck(int index,BOOL *pvalue );
BOOL setForceSyncCheck(int index, BOOL pvalue);
int getWebConfigParameterValues(char **parameterNames, int paramCount, int *val_size, parameterValStruct_t ***val);

int setWebConfigParameterValues(parameterValStruct_t *val, int paramCount, char **faultParam );

#endif
