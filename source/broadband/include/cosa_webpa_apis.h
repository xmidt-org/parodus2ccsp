/**
 * @file cosa_webpa_apis.h
 *
 * @description This file describes the Webpa DML layer 
 *
 * Copyright (c) 2016  Comcast
 */

#ifndef  _COSA_WEBPA_APIS_H
#define  _COSA_WEBPA_APIS_H

#include "slap_definitions.h"

/*
 *  Dynamic portion of WEBPA Configuration
 */
struct
_COSA_DML_WEBPA_CONFIG
{
  UINT  X_COMCAST_COM_CMC;
  CHAR  X_COMCAST_COM_CID[64];
  CHAR  X_COMCAST_COM_SyncProtocolVersion[64];
}_struct_pack_;

typedef  struct _COSA_DML_WEBPA_CONFIG COSA_DML_WEBPA_CONFIG,  *PCOSA_DML_WEBPA_CONFIG;

/**********************************************************************
                FUNCTION PROTOTYPES
**********************************************************************/

BOOL    
CosaDmlWEBPA_GetConfiguration( PCOSA_DML_WEBPA_CONFIG  pWebpaCfg );

BOOL    
CosaDmlWEBPA_SetConfiguration( PCOSA_DML_WEBPA_CONFIG  pWebpaCfg, char* ParamName, void* pvParamValue );

BOOL 
CosaDmlWEBPA_GetValueFromDB( char* ParamName, char* pString );

BOOL 
CosaDmlWEBPA_StoreValueIntoDB( char* ParamName, char* pString );

#endif /* _COSA_WEBPA_APIS_H */
