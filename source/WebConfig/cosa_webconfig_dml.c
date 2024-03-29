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
#include <webcfg_log.h>

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
	WebcfgDebug(" %s : ENTER \n", __FUNCTION__ );
	RFC_ENABLE=Get_RfcEnable();
	if(!RFC_ENABLE)
	{
		WebcfgError("%s RfcEnable is disabled so, %s SET failed\n",__FUNCTION__,ParamName);
		return FALSE;
	}
	if( AnscEqualString(ParamName, "ForceSync", TRUE))
	{
		int session_status = 0;
		if(AnscEqualString(strValue, "telemetry", TRUE))
        	{
			char telemetryUrl[256] = {0};
			Get_Supplementary_URL("Telemetry", telemetryUrl);
			if(strncmp(telemetryUrl,"NULL",strlen("NULL")) == 0)
			{
				WebcfgError("%s Telemetry url is null so, %s SET failed\n",__FUNCTION__,ParamName);
				return FALSE;
			}		
		}	
		/* save update to backup */
		if(setForceSync(strValue, "", &session_status) == 1)
		{
			return TRUE;
		}
		else
		{
			WebcfgError("setForceSync failed\n");
		}
	}

        if( AnscEqualString(ParamName, "URL", TRUE))
        {
		if(isValidUrl(strValue) == TRUE)
		{
			if(Set_Webconfig_URL(strValue))
			{
				return TRUE;
			}
			else
			{
				WebcfgError("Set_Webconfig_URL failed\n");
			}	
		}
		else
		{
			WebcfgError("Webcfg URL validation failed\n");
		}
        }
		if( AnscEqualString(ParamName, "Data", TRUE))
	        {
			WebcfgDebug("Data set is Not supported\n");
            return TRUE;
		}
		if( AnscEqualString(ParamName, "SupportedDocs", TRUE))
                {
			WebcfgDebug("SupportedDocs set is Not supported\n");
                        return TRUE;
                }
                if( AnscEqualString(ParamName, "SupportedSchemaVersion", TRUE))
                {
			WebcfgDebug("SupportedSchemaVersion set is Not supported\n");
                        return TRUE;
                }

	WebcfgDebug(" %s : EXIT \n", __FUNCTION__ );

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
	WebcfgDebug("------- %s ----- ENTER ----\n",__FUNCTION__);
	RFC_ENABLE=Get_RfcEnable();
	if(!RFC_ENABLE)
	{
		WebcfgError("------- %s ----- RfcEnable is disabled so, %s Get from DB failed\n",__FUNCTION__,ParamName);
		return 0;
	}
	/* check the parameter name and return the corresponding value */
	if( AnscEqualString(ParamName, "ForceSync", TRUE))
	{
		WebcfgDebug("ForceSync Get Not supported\n");
		return 0;
	}
        if( AnscEqualString(ParamName, "URL", TRUE))
        {
                if(Get_Webconfig_URL(pValue))
                {
			WebcfgInfo("URL fetched : pValue %s\n", pValue);
			return 0;
                }
        }
        if( AnscEqualString(ParamName, "Data", TRUE))
	    {
                WebcfgDebug("[%s] at [%d]parameter '%s'\n",__FUNCTION__,__LINE__, ParamName);
                char * blobData = NULL;

                blobData = get_DB_BLOB_base64();

                if (blobData != NULL)
                {
                         WebcfgDebug("The Blob fetched is %s size %zu\n",blobData, strlen(blobData));

			 if(*pUlSize <= strlen(blobData))
                         {
			     *pUlSize = strlen(blobData) + 1;
			     return 1;
		         }
		         /* collect value */
		         AnscCopyString(pValue, blobData);
                         WebcfgDebug("The pValue is %s\n",pValue);
		         if(blobData != NULL)
		         {
			     free(blobData);
			     blobData = NULL;
		         }
	        }
                return 0;
        }
        if( AnscEqualString(ParamName, "SupportedDocs", TRUE))
	    {
                WebcfgDebug("[%s] at [%d]parameter '%s'\n",__FUNCTION__,__LINE__, ParamName);
                char * docValue = NULL;

                docValue = getsupportedDocs();

                if (docValue != NULL)
                {
                         WebcfgDebug("The docValue fetched is %s size %zu\n",docValue, strlen(docValue));

			 if(*pUlSize <= strlen(docValue))
                         {
			     *pUlSize = strlen(docValue) + 1;
			     return 1;
		         }
		         /* collect value */
		         AnscCopyString(pValue, docValue);
                         WebcfgDebug("The pValue is %s\n",pValue);
	        }
                return 0;
        }
        if( AnscEqualString(ParamName, "SupportedSchemaVersion", TRUE))
	    {
                WebcfgDebug("[%s] at [%d]parameter '%s'\n",__FUNCTION__,__LINE__, ParamName);
                char * versionValue = NULL;

                versionValue = getsupportedVersion();

                if (versionValue != NULL)
                {
                         WebcfgDebug("The versionValue fetched is %s size %zu\n",versionValue, strlen(versionValue));

			 if(*pUlSize <= strlen(versionValue))
                         {
			     *pUlSize = strlen(versionValue) + 1;
			     return 1;
		         }
		         /* collect value */
		         AnscCopyString(pValue, versionValue);
                         WebcfgDebug("The pValue is %s\n",pValue);
	        }
		return 0;
        }

	WebcfgDebug("------- %s ----- EXIT ----\n",__FUNCTION__);
 	WebcfgError("Unsupported parameter '%s'\n", ParamName);
	return -1;
}

BOOL isValidUrl
    (
        char *	pUrl
    )
{
	WebcfgDebug("Validate URL %s\n", pUrl);
	if(strstr(pUrl, "https") == NULL)
	{
		if(1 == get_supplementary_flag() && strncmp(pUrl,"NULL", strlen("NULL")) == 0)
		{
			return TRUE;
		}
		WebcfgError("Invalid URL, HTTPS is only allowed\n");
		return FALSE;
	}
	return TRUE;
}

/***********************************************************************

 APIs for Object:

    X_RDK_WebConfig.SupplementaryServiceUrls.

    *  Supplementary_Service_Urls_GetParamStringValue
    *  Supplementary_Service_Urls_SetParamStringValue

***********************************************************************/

BOOL
Supplementary_Service_Urls_SetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       strValue
    )
{
	WebcfgDebug("------- %s ----- ENTER ----\n", __FUNCTION__ );
	RFC_ENABLE=Get_RfcEnable();
	if(!RFC_ENABLE)
	{
		WebcfgError("%s RfcEnable is disabled so, %s SET failed\n",__FUNCTION__,ParamName);
		return FALSE;
	}

	if( AnscEqualString(ParamName, "Telemetry", TRUE))
        {
		WebcfgDebug("strValue is%s-url\n", strValue);
		set_supplementary_flag(1);
		if(isValidUrl(strValue) == TRUE )
		{

			set_supplementary_flag(0);
			if(Set_Supplementary_URL(ParamName, strValue))
			{
				return TRUE;
			}
			else
			{
				WebcfgError("Set_Supplementary_URL failed\n");
			}
		}
		else
		{
			WebcfgError("Supplementary URL validation failed\n");
		}
        }

	WebcfgDebug(" %s : EXIT \n", __FUNCTION__ );

	return FALSE;
}

ULONG
Supplementary_Service_Urls_GetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pValue,
        ULONG*                      pUlSize
    )
{
	WebcfgDebug("------- %s ----- ENTER ----\n",__FUNCTION__);
	RFC_ENABLE=Get_RfcEnable();
	if(!RFC_ENABLE)
	{
		WebcfgError("------- %s ----- RfcEnable is disabled so, %s Get from DB failed\n",__FUNCTION__,ParamName);
		return 0;
	}

	if( AnscEqualString(ParamName, "Telemetry", TRUE))
        {
                if(Get_Supplementary_URL(ParamName, pValue))
                {
			WebcfgInfo("URL fetched : pValue %s\n", pValue);
			return 0;
                }
        }

	WebcfgDebug("------- %s ----- EXIT ----\n",__FUNCTION__);
	WebcfgError("Unsupported parameter '%s'\n", ParamName);
	return -1;

}

