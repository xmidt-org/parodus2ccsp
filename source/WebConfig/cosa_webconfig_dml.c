/*
 * If not stated otherwise in this file or this component's Licenses.txt file the
 * following copyright and licenses apply:
 *
 * Copyright 2016 RDK Management
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
#include "ansc_platform.h"
#include "cosa_webconfig_apis.h"
#include "cosa_webconfig_dml.h"
#include "cosa_webconfig_internal.h"
#include "plugin_main_apis.h"
#include "webconfig_log.h"

//bool g_shutdown  = false;
/***********************************************************************

 APIs for Object:

    X_RDK_WebConfig.

    *  X_RDK_WebConfig_GetParamBoolValue
    *  X_RDK_WebConfig_SetParamBoolValue

***********************************************************************/
BOOL
X_RDK_WebConfig_GetParamBoolValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        BOOL*                       pBool
    )
{
	WebcfgDebug("------- %s ----- ENTER ----\n",__FUNCTION__);
	if( AnscEqualString(ParamName, "RfcEnable", TRUE))
	{
		/* collect value */
		*pBool = Get_RfcEnable();
		return TRUE;
	}
	WebcfgDebug("------- %s ----- EXIT ----\n",__FUNCTION__);
	return FALSE;
}

BOOL
X_RDK_WebConfig_SetParamBoolValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        BOOL                        bValue
    )
{
	WebcfgDebug("------- %s ----- ENTER ----\n",__FUNCTION__);
	/* check the parameter name and set the corresponding value */
	if( AnscEqualString(ParamName, "RfcEnable", TRUE))
	{
		if(setRfcEnable(bValue) == 0)
		{
			return TRUE;
		}
	}
	WebcfgDebug("------- %s ----- EXIT ----\n",__FUNCTION__);
	return FALSE;
}


BOOL
X_RDK_WebConfig_SetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       strValue
    )
{
	WebConfigLog(" %s : ENTER \n", __FUNCTION__ );
	RFC_ENABLE=Get_RfcEnable();
	if(!RFC_ENABLE)
	{
		WebConfigLog("%s RfcEnable is disabled so, %s SET failed\n",__FUNCTION__,ParamName);
		return FALSE;
	}
	if( AnscEqualString(ParamName, "ForceSync", TRUE))
	{
		/* save update to backup */
		if(setForceSync(strValue, "", 0) == 1)
		{
			return TRUE;
		}
		else
		{
			WebConfigLog("setForceSync failed\n");
		}
	}

        if( AnscEqualString(ParamName, "URL", TRUE))
        {
                if(Set_Webconfig_URL(strValue))
                {
                        return TRUE;
                }
                else
                {
                        WebConfigLog("Set_Webconfig_URL failed\n");
                }
        }
		if( AnscEqualString(ParamName, "Data", TRUE))
	    {
			WebConfigLog("Data set is Not supported\n");
			return 0;
		}
	WebConfigLog(" %s : EXIT \n", __FUNCTION__ );

	return FALSE;
}

ULONG
X_RDK_WebConfig_GetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pValue,
        ULONG*                      pUlSize
    )
{
	WebConfigLog("------- %s ----- ENTER ----\n",__FUNCTION__);
	RFC_ENABLE=Get_RfcEnable();
	if(!RFC_ENABLE)
	{
		WebConfigLog("------- %s ----- RfcEnable is disabled so, %s Get from DB failed\n",__FUNCTION__,ParamName);
		return 0;
	}
	/* check the parameter name and return the corresponding value */
	if( AnscEqualString(ParamName, "ForceSync", TRUE))
	{
		WebConfigLog("ForceSync Get Not supported\n");
		return 0;
	}
        if( AnscEqualString(ParamName, "URL", TRUE))
        {
                if(Get_Webconfig_URL(pValue))
                {

			
			WebConfigLog("URL fetched : pValue %s\n", pValue);
				return 0;
                }
        }
        if( AnscEqualString(ParamName, "Data", TRUE))
	    {
                if (Get_Webconfig_Blob(pValue))
                {
					WebConfigLog("Blob Data fetched : pValue %s\n", pValue);
					*pUlSize = AnscSizeOfString(pValue)+1;
					WebConfigLog("*pUlSize %lu\n", *pUlSize);
					return 1;
                }
                else
                {
			WebConfigLog("Failed to get Blob Data \n");
			return 0;
                }
        }

	WebConfigLog("------- %s ----- EXIT ----\n",__FUNCTION__);
 	CcspTraceWarning(("Unsupported parameter '%s'\n", ParamName));
	return -1;
}

BOOL isValidUrl
    (
        PCHAR                       pUrl
    )
{
	if(strstr(pUrl, "https") == NULL)
	{
		WalError("Invalid URL\n");
		return FALSE;
	}
	return TRUE;
}

