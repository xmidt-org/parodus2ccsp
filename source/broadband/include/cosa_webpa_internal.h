/**
 * @file cosa_webpa_internal.h
 *
 * @description This file describes the Webpa intilization
 *
 * Copyright (c) 2016  Comcast
 */
 
#ifndef  _COSA_WEBPA_INTERNAL_H
#define  _COSA_WEBPA_INTERNAL_H
#include "cosa_webpa_apis.h"

/* Collection */
typedef  struct
_COSA_DML_WEBPA
{
	PCOSA_DML_WEBPA_CONFIG           pWebpaCfg; 
}
COSA_DML_WEBPA, *PCOSA_DML_WEBPA;

#define  COSA_DATAMODEL_WEBPA_CLASS_CONTENT                                                  \
    /* start of WEBPA object class content */                                                \
	PCOSA_DML_WEBPA					pWebpa;

typedef  struct
_COSA_DATAMODEL_WEBPA                                               
{
	COSA_DATAMODEL_WEBPA_CLASS_CONTENT
}
COSA_DATAMODEL_WEBPA,  *PCOSA_DATAMODEL_WEBPA;

/*
    Standard function declaration 
*/

ANSC_HANDLE
CosaWebpaCreate
    (
        VOID
    );

ANSC_STATUS
CosaWebpaInitialize
    (
        ANSC_HANDLE                 hThisObject
    );

VOID
CosaWebpaSyncDB
    (
        VOID
    );

ANSC_STATUS
CosaWebpaRemove
    (
        ANSC_HANDLE                 hThisObject
    );
#endif /* _COSA_WEBPA_INTERNAL_H */
