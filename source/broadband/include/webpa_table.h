/**
 * @file webpa_internal.h
 *
 * @description This file describes the Webpa Abstraction Layer
 *
 * Copyright (c) 2015  Comcast
 */
#include "ssp_global.h"
#include <stdio.h>
#include "ccsp_dm_api.h"
#include <sys/time.h>
#include "webpa_adapter.h"
#include "webpa_internal.h"

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/*                               Data Structures                              */
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/

/**
 * @brief addRow adds new row to the table
 *
 * param[in] object table name
 * @param[in] CompName Component Name of parameters
 * @param[in] dbusPath dbus path of parameters
 * param[out] retIndex return new row added
 */
int addRow(char *object,char *compName,char *dbusPath,int *retIndex);

/**
 * @brief updateRow updates row data
 *
 * param[in] objectName table name
 * param[in] list Parameter name/value pairs
 * param[out] retObject return new row added
 * param[out] retStatus Returns status
 */
int updateRow(char *objectName,TableData *list,char *compName,char *dbusPath);

/**
 * @brief deleteRow deletes table row
 *
 * param[in] object table name to delete
 */
int deleteRow(char *object);
