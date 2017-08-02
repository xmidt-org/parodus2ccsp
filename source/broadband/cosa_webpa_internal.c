/**
 * @file cosa_webpa_internal.c
 *
 * @description This file describes the Webpa intilization
 *
 * Copyright (c) 2016  Comcast
 */
#include "cosa_webpa_internal.h"

/* Global Pointer for WEBPA backend manager */
PCOSA_BACKEND_MANAGER_OBJECT g_pCosaBEManager;

/**********************************************************************

    caller:     owner of the object

    prototype:

        VOID
        CosaWebpaBEManagerCreate
            (
            		VOID
            );

    description:

        This function constructs cosa webpa object and return handle.

    argument:  

    return:     nothing.

**********************************************************************/

VOID
CosaWebpaBEManagerCreate
    (
        VOID
    )
{

	/*
	  * Need to create memory for WEBPA BACKEND manager pointer
	  */
	g_pCosaBEManager = (PCOSA_BACKEND_MANAGER_OBJECT)malloc(sizeof(COSA_BACKEND_MANAGER_OBJECT));
	
	if ( !g_pCosaBEManager )
	{
	   return;
	}

  /*
     * Initialize the common variables and functions for a container object.
     */
	g_pCosaBEManager->hWebpa = CosaWebpaCreate( );
}

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
        ANSC_STATUS                     returnStatus  = FALSE;
        PCOSA_DATAMODEL_WEBPA           pMyObject     = (PCOSA_DATAMODEL_WEBPA)hThisObject;
	PCOSA_DML_WEBPA 	        pWebpa	      = (PCOSA_DML_WEBPA )NULL;
	PCOSA_DML_WEBPA_CONFIG          pWebpaCfg     = (PCOSA_DML_WEBPA_CONFIG)NULL; 

	/*
	  * Need to create memory for WEBPA DML pointer
	  */
	pWebpa = (PCOSA_DML_WEBPA)malloc(sizeof(COSA_DML_WEBPA));
	
	if ( !pWebpa )
	{
	   return  (ANSC_HANDLE)NULL;
	}

	pWebpaCfg = (PCOSA_DML_WEBPA_CONFIG)malloc(sizeof(COSA_DML_WEBPA_CONFIG));
	
	if ( !pWebpaCfg )
	{
            free(pWebpa);
            return  (ANSC_HANDLE)NULL;
	}

	/* Get all webpa parameters values */
	CosaDmlWEBPA_GetConfiguration( pWebpaCfg );

	pWebpa->pWebpaCfg	  = pWebpaCfg;
	pMyObject->pWebpa     = pWebpa;

EXIT:
    return returnStatus;
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
