/**
 * @file cosa_webpa_apis.c
 *
 * @description This file describes the Webpa layer
 *
 * Copyright (c) 2016  Comcast
 */
#include "ansc_platform.h"
#include "cosa_webpa_internal.h"
#include "ccsp_trace.h"
#include "ccsp_base_api.h"
#include "webpa_adapter.h"
#include "ccsp_psm_helper.h"
#include "webpa_internal.h"

/* 
 * To enable when all webpa params getting from syscfg.db file otherwise keep 
 * it disable.
 */
//#define WEBPA_PARAMS_VIA_SYSCFG

extern ANSC_HANDLE bus_handle;
extern char        g_Subsystem[32];

static char *PSMPrefixString  = "eRT.com.cisco.spvtg.ccsp.webpa.";

BOOL    
CosaDmlWEBPA_GetConfiguration( PCOSA_DML_WEBPA_CONFIG  pWebpaCfg )
{
	BOOL returnstatus = TRUE;
	char tmpchar[128] = { 0 };

	memset(pWebpaCfg, 0, sizeof(COSA_DML_WEBPA_CONFIG));
	
	/* X_COMCAST-COM_CID */
	CosaDmlWEBPA_GetValueFromDB( "X_COMCAST-COM_CID", pWebpaCfg->X_COMCAST_COM_CID );

	/* X_COMCAST-COM_SyncProtocolVersion */
	CosaDmlWEBPA_GetValueFromDB( "X_COMCAST-COM_SyncProtocolVersion", pWebpaCfg->X_COMCAST_COM_SyncProtocolVersion );

	/* X_COMCAST-COM_CMC */
	CosaDmlWEBPA_GetValueFromDB( "X_COMCAST-COM_CMC", tmpchar );
	pWebpaCfg->X_COMCAST_COM_CMC = atoi(tmpchar);

	return returnstatus;
}

BOOL    
CosaDmlWEBPA_SetConfiguration( PCOSA_DML_WEBPA_CONFIG  pWebpaCfg, char* ParamName, void* pvParamValue )
{
	BOOL returnstatus = TRUE;
	
	if( AnscEqualString(ParamName, "X_COMCAST-COM_CID", TRUE))
	{
		returnstatus = CosaDmlWEBPA_StoreValueIntoDB( ParamName, (char*)pvParamValue );
		if ( returnstatus )
		{
			memset( pWebpaCfg->X_COMCAST_COM_CID, 0, sizeof(pWebpaCfg->X_COMCAST_COM_CID));
			AnscCopyString(pWebpaCfg->X_COMCAST_COM_CID, (char*)pvParamValue);
		}
	}
	else if( AnscEqualString(ParamName, "X_COMCAST-COM_SyncProtocolVersion", TRUE))
	{
		returnstatus = CosaDmlWEBPA_StoreValueIntoDB( ParamName, (char*)pvParamValue );
		if ( returnstatus )
		{
			memset( pWebpaCfg->X_COMCAST_COM_SyncProtocolVersion, 0, sizeof(pWebpaCfg->X_COMCAST_COM_SyncProtocolVersion));
			AnscCopyString(pWebpaCfg->X_COMCAST_COM_SyncProtocolVersion, (char*)pvParamValue);
		}
	}
	else if( AnscEqualString(ParamName, "X_COMCAST-COM_CMC", TRUE))
	{
		char tmpchar[8];

		snprintf(tmpchar,sizeof(tmpchar),"%d",*((ULONG*)pvParamValue));

		returnstatus = CosaDmlWEBPA_StoreValueIntoDB( ParamName, tmpchar );
		if ( returnstatus )
		{
			pWebpaCfg->X_COMCAST_COM_CMC = *((ULONG*)pvParamValue);
		}
	}

	return returnstatus;
}

BOOL 
CosaDmlWEBPA_GetValueFromDB( char* ParamName, char* pString )
{
/* Value getting/retriving from syscfg.db or CCSPSM */
#ifdef WEBPA_PARAMS_VIA_SYSCFG
    extern int syscfg_get (const char *ns, const char *name, char *out_val, int outbufsz);
	CHAR tmpbuf[ 128 ] = { 0 };

	if( 0 != syscfg_get( NULL, ParamName, tmpbuf, sizeof(tmpbuf)))
	{
		WalError("syscfg_get failed\n");
		return FALSE;
	}
	
	AnscCopyString( pString, tmpbuf );
#else
        CHAR  tmpbuf[ 256 ] = { 0 };
        	
	sprintf(tmpbuf, "%s%s",PSMPrefixString,ParamName );
	
        return CosaDml_GetValueFromPSMDB( tmpbuf, pString );
#endif /* WEBPA_PARAMS_VIA_SYSCFG */
	return TRUE;
}

BOOL 
CosaDmlWEBPA_StoreValueIntoDB( char*  	ParamName,
										   char* 	pString )
{
/* Value getting/retriving from syscfg.db and CCSPSM */
#ifdef WEBPA_PARAMS_VIA_SYSCFG
	if ( 0 != syscfg_set(NULL, ParamName , pString ) ) 
	{
		WalError("syscfg_set failed\n");
		return FALSE;
	}
	else 
	{
		if ( 0 != syscfg_commit( ) ) 
		{
			WalError("syscfg_commit failed\n");
			return FALSE;
		}
	}
#else
	CHAR tmpbuf[ 256 ] = { 0 };
	int  retPsmSet;

	sprintf(tmpbuf, "%s%s",PSMPrefixString,ParamName );
	retPsmSet = PSM_Set_Record_Value2(bus_handle,g_Subsystem, tmpbuf, ccsp_string, pString);
	if (retPsmSet != CCSP_SUCCESS) 
	{
		WalError("psm_set failed ret %d for parameter %s and value %s\n", retPsmSet, ParamName, pString);
		return FALSE;
	}
	else
		WalInfo("psm_set success ret %d for parameter %s and value %s\n", retPsmSet, ParamName, pString);
#endif /* WEBPA_PARAMS_VIA_SYSCFG */

	return TRUE;
}

BOOL
CosaDml_GetValueFromPSMDB ( char* ParamName, char* pString )
{
	char *tempDBBuffer = NULL;
        int   retPsmGet    = CCSP_SUCCESS;
        retPsmGet = PSM_Get_Record_Value2(bus_handle,g_Subsystem, ParamName, NULL, &tempDBBuffer);
        if (retPsmGet == CCSP_SUCCESS)
        {
                snprintf(pString,64,"%s",tempDBBuffer);
                ((CCSP_MESSAGE_BUS_INFO *)bus_handle)->freefunc(tempDBBuffer);
        }
        else
        {
                WalError("psm_get failed ret %d for parameter %s\n", retPsmGet, ParamName);
                return FALSE;
        }
        return TRUE;
}	


