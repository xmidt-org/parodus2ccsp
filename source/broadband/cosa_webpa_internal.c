/**
 * @file cosa_webpa_internal.c
 *
 * @description This file describes the Webpa intilization
 *
 * Copyright (c) 2016  Comcast
 */
#include "webpa_adapter.h"
#include "cosa_webpa_internal.h"
#include "plugin_main_apis.h"
#include "plugin_main.h"

/**********************************************************************

    caller:     owner of the object

    prototype:

        ANSC_HANDLE
        CosaWebpaCreate
            (
			VOID
            );

    description:

        This function constructs cosa webpa object and return handle.

    argument:  

    return:     newly created webpa object.

**********************************************************************/

ANSC_HANDLE
CosaWebpaCreate
    (
        VOID
    )
{
    PCOSA_DATAMODEL_WEBPA          pMyObject    = (PCOSA_DATAMODEL_WEBPA)NULL;

  /*
     * We create object by first allocating memory for holding the variables and member functions.
     */
    pMyObject = (PCOSA_DATAMODEL_WEBPA)malloc(sizeof(COSA_DATAMODEL_WEBPA));

    if ( !pMyObject )
    {
        return  (ANSC_HANDLE)NULL;
    }

  /*
     * Initialize the common variables and functions for a container object.
     */
    CosaWebpaInitialize((ANSC_HANDLE)pMyObject);

    return  (ANSC_HANDLE)pMyObject;
}

/**********************************************************************

    caller:     self

    prototype:

        ANSC_STATUS
        CosaWebpaInitialize
            (
                ANSC_HANDLE                 hThisObject
            );

    description:

        This function initiate  cosa webpa object and return handle.

    argument:    ANSC_HANDLE                 hThisObject
            This handle is actually the pointer of this object
            itself.

    return:     operation status.

**********************************************************************/

ANSC_STATUS
CosaWebpaInitialize
    (
        ANSC_HANDLE                 hThisObject
    )
{
    PCOSA_DATAMODEL_WEBPA   pMyObject   = (PCOSA_DATAMODEL_WEBPA)hThisObject;
    PCOSA_DML_WEBPA         pWebpa	    = (PCOSA_DML_WEBPA )NULL;
    PCOSA_DML_WEBPA_CONFIG  pWebpaCfg   = (PCOSA_DML_WEBPA_CONFIG)NULL; 

	/*
	  * Need to create memory for WEBPA DML pointer
	  */
	pWebpa = (PCOSA_DML_WEBPA)malloc(sizeof(COSA_DML_WEBPA));
	
	if ( !pWebpa )
	{
        return ANSC_STATUS_FAILURE;
	}

	pWebpaCfg = (PCOSA_DML_WEBPA_CONFIG)malloc(sizeof(COSA_DML_WEBPA_CONFIG));
	
	if ( !pWebpaCfg )
	{
        free(pWebpa);
        return ANSC_STATUS_FAILURE;
	}

	memset(pWebpaCfg, 0, sizeof(COSA_DML_WEBPA_CONFIG));
	pWebpa->pWebpaCfg	  = pWebpaCfg;
	pMyObject->pWebpa     = pWebpa;
    return ANSC_STATUS_SUCCESS;
}

/**********************************************************************
    caller:     owner of the object
    prototype:
        VOID
        CosaWebpaSyncDB
            (
		VOID
            );
    description:
        This function syncs WEBPA BACKEND manager with DB values.
    argument:
    return:     nothing.
**********************************************************************/
VOID
CosaWebpaSyncDB
    (
        VOID
    )
{
	PCOSA_DATAMODEL_WEBPA       hWebpa    = (PCOSA_DATAMODEL_WEBPA)g_pCosaBEManager->hWebpa;
	PCOSA_DML_WEBPA             pWebpa    = (PCOSA_DML_WEBPA) hWebpa->pWebpa;
	PCOSA_DML_WEBPA_CONFIG      pWebpaCfg = (PCOSA_DML_WEBPA_CONFIG)pWebpa->pWebpaCfg;

	/* Get all webpa parameters values */
	CosaDmlWEBPA_GetConfiguration( pWebpaCfg );
	WalInfo("CID = %s CMC = %d Version = %s\n",pWebpaCfg->X_COMCAST_COM_CID,pWebpaCfg->X_COMCAST_COM_CMC,pWebpaCfg->X_COMCAST_COM_SyncProtocolVersion);
	pWebpa->pWebpaCfg	  = pWebpaCfg;
}

/**********************************************************************

    caller:     self

    prototype:

        ANSC_STATUS
        CosaWebpaRemove
            (
                ANSC_HANDLE                 hThisObject
            );

    description:

        This function remove cosa webpa object and return status.

    argument:    ANSC_HANDLE                 hThisObject
            This handle is actually the pointer of this object
            itself.

    return:     operation status.

**********************************************************************/
ANSC_STATUS
CosaWebpaRemove
    (
        ANSC_HANDLE                 hThisObject
    )
{
    ANSC_STATUS                     returnStatus  = FALSE;
    PCOSA_DATAMODEL_WEBPA           pMyObject     = (PCOSA_DATAMODEL_WEBPA)hThisObject;
    PCOSA_DML_WEBPA 		    pWebpa        = (PCOSA_DML_WEBPA)NULL;

    /* Free Allocated Memory for WEBPA object */
    pWebpa = pMyObject->pWebpa;

    if( NULL != pWebpa )
    {
        free((ANSC_HANDLE)pWebpa->pWebpaCfg);
        free((ANSC_HANDLE)pWebpa);
    }

    /* Remove self */
    free((ANSC_HANDLE)pMyObject);

    return returnStatus;
}
