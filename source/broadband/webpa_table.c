/**
 * @file webpa_adapter.c
 *
 * @description This file describes the Webpa Abstraction Layer
 *
 * Copyright (c) 2015  Comcast
 */

#include "webpa_table.h"

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/*                               Data Structures                              */
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/

void addRowTable(char *objectName, TableData *list,char **retObject, money_trace_spans *timeSpan, WDMP_STATUS *retStatus)
{
    int ret = 0, index =0, status =0, retUpdate = 0, retDel = 0;
    char paramName[MAX_PARAMETERNAME_LEN] = { 0 };
    char compName[MAX_PARAMETERNAME_LEN/2] = { 0 };
    char dbusPath[MAX_PARAMETERNAME_LEN/2] = { 0 };
    char tempParamName[MAX_PARAMETERNAME_LEN] = { 0 };
    uint32_t duration = 0;
    money_trace_spans *deleteSpan = NULL;

    WalPrint("objectName : %s\n",objectName);
    strncpy(paramName,objectName,sizeof(paramName));
    WalPrint("paramName before mapping : %s\n",paramName);
    status=IndexMpa_WEBPAtoCPE(paramName);
    if(status == -1)
    {
        if(strstr(paramName, PARAM_RADIO_OBJECT) != NULL)
        {
            ret = CCSP_ERR_INVALID_RADIO_INDEX;
            WalError("%s has invalid Radio index, Valid indexes are 10000 and 10100. ret = %d\n", paramName,ret); 
        }
        else
        {
            ret = CCSP_ERR_INVALID_WIFI_INDEX;
            WalError("%s has invalid WiFi index, Valid range is between 10001-10008 and 10101-10108. ret = %d\n",paramName, ret);
        }
    }
    else
    {
        WalPrint("paramName after mapping : %s\n",paramName);
        ret = addRow(paramName,compName,dbusPath,timeSpan, &index);
        WalPrint("ret = %d index :%d\n",ret,index);
        WalPrint("parameterName: %s, CompName : %s, dbusPath : %s\n", paramName, compName, dbusPath);
        if(ret == CCSP_SUCCESS)
        {
            WalPrint("paramName : %s index : %d\n",paramName,index);
            snprintf(tempParamName,MAX_PARAMETERNAME_LEN,"%s%d.", paramName, index);
            WalPrint("tempParamName : %s\n",tempParamName);
            retUpdate = updateRow(tempParamName,list,compName,dbusPath, &duration);
            if(timeSpan)
            {
                timeSpan->spans[0].duration = timeSpan->spans[0].duration + duration;
                WalPrint("timeSpan->spans[0].duration = %d\n",timeSpan->spans[0].duration);
            }
            if(retUpdate == CCSP_SUCCESS)
            {
                strcpy(*retObject, tempParamName);
                WalPrint("retObject : %s\n",*retObject);
                WalPrint("Table is updated successfully\n");
                WalPrint("retObject before mapping :%s\n",*retObject);
                IndexMpa_CPEtoWEBPA(retObject);
                WalPrint("retObject after mapping :%s\n",*retObject);
            }
            else
            {
                ret = retUpdate;
                WalError("Failed to update row hence deleting the added row %s\n",tempParamName);
                if(timeSpan)
                {
                    deleteSpan = (money_trace_spans *) malloc(sizeof(money_trace_spans));
                    memset(deleteSpan,0,(sizeof(money_trace_spans)));
                }
                retDel = deleteRow(tempParamName, deleteSpan);
                if(timeSpan)
                {
                    if(deleteSpan->count > 0)
                    {
                        timeSpan->spans[0].duration = deleteSpan->spans[0].duration + duration;
                        WalPrint("timeSpan->spans[0].duration = %d\n",timeSpan->spans[0].duration);
                        WAL_FREE(deleteSpan->spans[0].name);
                        WAL_FREE(deleteSpan->spans);
                    }
                    WAL_FREE(deleteSpan);
                }
                if(retDel == CCSP_SUCCESS)
                {
                    WalInfo("Reverted the add row changes.\n");
                }
                else
                {
                    WalError("Failed to revert the add row changes\n");
                }
            }
        }
        else
        {
            WalError("Failed to add table\n");
        }
    }
    WalPrint("ret : %d\n",ret);
    *retStatus = mapStatus(ret);
    WalPrint("retStatus : %d\n",*retStatus);
}

void deleteRowTable(char *object, money_trace_spans *timeSpan, WDMP_STATUS *retStatus)
{
    int ret = 0,status = 0;
    char paramName[MAX_PARAMETERNAME_LEN] = { 0 };

    WalPrint("object : %s\n",object);
    strncpy(paramName,object,sizeof(paramName));
    WalPrint("paramName before mapping : %s\n",paramName);
    status=IndexMpa_WEBPAtoCPE(paramName);
    if(status == -1)
    {
        if(strstr(paramName, PARAM_RADIO_OBJECT) != NULL)
        {
            ret = CCSP_ERR_INVALID_RADIO_INDEX;
            WalError("%s has invalid Radio index, Valid indexes are 10000 and 10100. ret = %d\n", paramName,ret); 
        }
        else
        {
            ret = CCSP_ERR_INVALID_WIFI_INDEX;
            WalError("%s has invalid WiFi index, Valid range is between 10001-10008 and 10101-10108. ret = %d\n",paramName, ret);
        }
    }
    else
    {
        WalPrint("paramName after mapping : %s\n",paramName);
        ret = deleteRow(paramName, timeSpan);
        if(ret == CCSP_SUCCESS)
        {
            WalPrint("%s is deleted Successfully.\n", paramName);
        }
        else
        {
            WalError("%s could not be deleted ret %d\n", paramName, ret);
        }
    }
    *retStatus = mapStatus(ret);
}

int addRow(char *object,char *compName,char *dbusPath, money_trace_spans *timeSpan,int *retIndex)
{
    int ret = 0, size = 0, index = 0;
    char dst_pathname_cr[MAX_PATHNAME_CR_LEN] = { 0 };
    char l_Subsystem[MAX_DBUS_INTERFACE_LEN] = { 0 };	
    componentStruct_t ** ppComponents = NULL;
    uint64_t startTime = 0, endTime = 0;
    struct timespec start, end;
    uint64_t start_time = 0;
    uint32_t timeDuration = 0;
#if !defined(RDKB_EMU)
    strncpy(l_Subsystem, "eRT.",sizeof(l_Subsystem));
#endif
    snprintf(dst_pathname_cr, sizeof(dst_pathname_cr),"%s%s", l_Subsystem, CCSP_DBUS_INTERFACE_CR);	

    WalPrint("<==========start of addRow ========>\n ");
    if(timeSpan)
    {
        startTime = getCurrentTimeInMicroSeconds(&start);
    }
    ret = CcspBaseIf_discComponentSupportingNamespace(bus_handle,
    dst_pathname_cr, object, l_Subsystem, &ppComponents, &size);
    if(timeSpan)
    {
        endTime = getCurrentTimeInMicroSeconds(&end);
        timeDuration += endTime - startTime;
    }
    WalPrint("size : %d, ret : %d\n",size,ret);

    if (ret == CCSP_SUCCESS && size == 1)
    {
        strcpy(compName,ppComponents[0]->componentName);
        strcpy(dbusPath,ppComponents[0]->dbusPath);
        free_componentStruct_t(bus_handle, size, ppComponents);
    }
    else
    {
        WalError("Parameter name %s is not supported. ret = %d\n", object, ret);
        free_componentStruct_t(bus_handle, size, ppComponents);
        return ret;
    }
    WalInfo("parameterName: %s, CompName : %s, dbusPath : %s\n", object, compName, dbusPath);
    if(timeSpan)
    {
        timeSpan->spans = (money_trace_span *) malloc(sizeof(money_trace_span));
        memset(timeSpan->spans,0,(sizeof(money_trace_span)));
        timeSpan->count = 1;
        WalPrint("timeSpan->count : %d\n",timeSpan->count);
        startTime = getCurrentTimeInMicroSeconds(&start);
        start_time = startTime;
        WalPrint("component start_time: %llu\n",start_time);
    }
    ret = CcspBaseIf_AddTblRow(
        bus_handle,
        compName,
        dbusPath,
        0,
        object,
        &index
    );
    if(timeSpan)
    {
        endTime = getCurrentTimeInMicroSeconds(&end);
        timeDuration += endTime - startTime;

        timeSpan->spans[0].name = strdup(compName);
        WalPrint("timeSpan->spans[0].name : %s\n",timeSpan->spans[0].name);
        WalPrint("start_time : %llu\n",start_time);
        timeSpan->spans[0].start = start_time;
        WalPrint("timeSpan->spans[0].start : %llu\n",timeSpan->spans[0].start);
        WalPrint("timeDuration : %lu\n",timeDuration);
        timeSpan->spans[0].duration = timeDuration;
        WalPrint("timeSpan->spans[0].duration : %lu\n",timeSpan->spans[0].duration);
    }
    WalPrint("ret = %d index : %d\n",ret,index);    
    if ( ret == CCSP_SUCCESS )
    {
        WalPrint("Execution succeed.\n");
        WalInfo("%s%d. is added.\n", object, index);               
        *retIndex = index;
        WalPrint("retIndex : %d\n",*retIndex);               
    }
    else
    {
        WalError("Execution fail ret :%d\n", ret);
    }
    WalPrint("<==========End of addRow ========>\n ");
    return ret;
}

int updateRow(char *objectName,TableData *list,char *compName,char *dbusPath, uint32_t *duration)
{
    int i=0, ret = -1,numParam =0, val_size = 0, retGet = -1;
    char **parameterNamesLocal = NULL; 
    char *faultParam = NULL;
    unsigned int writeID = CCSP_COMPONENT_ID_WebPA;	
    parameterValStruct_t *val= NULL;
    parameterValStruct_t **parameterval = NULL;
    uint64_t startTime = 0, endTime = 0;
    struct timespec start, end;
    uint32_t timeDuration = 0;

    WalPrint("<==========Start of updateRow ========>\n ");
    numParam = list->paramCnt;
    WalPrint("numParam : %d\n",numParam);
    parameterNamesLocal = (char **) malloc(sizeof(char *) * numParam);
    memset(parameterNamesLocal,0,(sizeof(char *) * numParam));        
    val = (parameterValStruct_t*) malloc(sizeof(parameterValStruct_t) * numParam);
    memset(val,0,(sizeof(parameterValStruct_t) * numParam));
    for(i =0; i<numParam; i++)
    {
        parameterNamesLocal[i] = (char *) malloc(sizeof(char ) * MAX_PARAMETERNAME_LEN);
        WalPrint("list->names[%d] : %s\n",i,list->names[i]);
        snprintf(parameterNamesLocal[i],MAX_PARAMETERNAME_LEN,"%s%s", objectName,list->names[i]);
        WalPrint("parameterNamesLocal[%d] : %s\n",i,parameterNamesLocal[i]);
    }

    WalInfo("parameterName: %s, CompName : %s, dbusPath : %s\n", parameterNamesLocal[0], compName, dbusPath);
    startTime = getCurrentTimeInMicroSeconds(&start);
    // To get dataType of parameter do bulk GET for all the input parameters in the requests
    retGet = CcspBaseIf_getParameterValues(bus_handle, compName, dbusPath, parameterNamesLocal,
         numParam, &val_size, &parameterval);
    WalPrint("After GPV ret: %d, val_size: %d\n",retGet,val_size);
    endTime = getCurrentTimeInMicroSeconds(&end);
    timeDuration += endTime - startTime;
    if(retGet == CCSP_SUCCESS && val_size > 0)
    {
        WalPrint("val_size : %d, numParam %d\n",val_size, numParam);
        for(i =0; i<numParam; i++)
        {
            WalPrint("parameterval[i]->parameterName %s, parameterval[i]->parameterValue %s, parameterval[i]->type %d\n",parameterval[i]->parameterName, parameterval[i]->parameterValue, parameterval[i]->type);
            val[i].parameterName = parameterNamesLocal[i];
            WalPrint("list->values[%d] : %s\n",i,list->values[i]);
            val[i].parameterValue = list->values[i];
            val[i].type = parameterval[i]->type;	
        }
        free_parameterValStruct_t (bus_handle, val_size, parameterval);
        startTime = getCurrentTimeInMicroSeconds(&start);
        ret = CcspBaseIf_setParameterValues(bus_handle, compName, dbusPath, 0, writeID, val, numParam,
             TRUE, &faultParam);
        endTime = getCurrentTimeInMicroSeconds(&end);
        timeDuration += endTime - startTime;
        WalPrint("ret : %d\n",ret);
        if((ret != CCSP_SUCCESS) && (faultParam != NULL))
        {
            WAL_FREE(faultParam);
        }
    }
    else
    {
        ret = retGet;
    }

    if(ret != CCSP_SUCCESS)
    {
        WalError("Failed to update row %d\n",ret);
    }

    for(i =0; i<numParam; i++)
    {
        WAL_FREE(parameterNamesLocal[i]);
    }
    WAL_FREE(parameterNamesLocal);
    WAL_FREE(val);
    WalPrint("timeDuration : %lu\n",timeDuration);
    *duration = timeDuration;
    WalPrint("*duration : %lu\n",*duration);
    WalPrint("<==========End of updateRow ========>\n ");
    return ret;
}


int deleteRow(char *object, money_trace_spans *timeSpan)
{
    int ret = 0, size =0;
    char compName[MAX_PARAMETERNAME_LEN/2] = { 0 };
    char dbusPath[MAX_PARAMETERNAME_LEN/2] = { 0 };
    char dst_pathname_cr[MAX_PATHNAME_CR_LEN] = { 0 };
    char l_Subsystem[MAX_DBUS_INTERFACE_LEN] = { 0 };
    componentStruct_t ** ppComponents = NULL;
    uint64_t startTime = 0, endTime = 0;
    struct timespec start, end;
    uint64_t start_time = 0;
    uint32_t timeDuration = 0;
#if !defined(RDKB_EMU)
    strncpy(l_Subsystem, "eRT.",sizeof(l_Subsystem));
#endif
    snprintf(dst_pathname_cr, sizeof(dst_pathname_cr),"%s%s", l_Subsystem, CCSP_DBUS_INTERFACE_CR);

    WalPrint("<==========Start of deleteRow ========>\n ");
    if(timeSpan)
    {
        startTime = getCurrentTimeInMicroSeconds(&start);
    }
    ret = CcspBaseIf_discComponentSupportingNamespace(
        bus_handle,
        dst_pathname_cr,
        object,
        l_Subsystem,
        &ppComponents,
        &size
    );
    WalPrint("size : %d, ret : %d\n",size,ret);
    if(timeSpan)
    {
        endTime = getCurrentTimeInMicroSeconds(&end);
        timeDuration += endTime - startTime;
    }

    if (ret == CCSP_SUCCESS && size == 1)
    {
        strcpy(compName,ppComponents[0]->componentName);
        strcpy(dbusPath,ppComponents[0]->dbusPath);
        free_componentStruct_t(bus_handle, size, ppComponents);
    }
    else
    {
        WalError("Parameter name %s is not supported. ret = %d\n", object, ret);
        free_componentStruct_t(bus_handle, size, ppComponents);
        return ret;
    }
    WalInfo("parameterName: %s, CompName : %s, dbusPath : %s\n", object, compName, dbusPath);
    if(timeSpan)
    {
        timeSpan->spans = (money_trace_span *) malloc(sizeof(money_trace_span));
        memset(timeSpan->spans,0,(sizeof(money_trace_span)));
        timeSpan->count = 1;
        WalPrint("timeSpan->count : %d\n",timeSpan->count);
        startTime = getCurrentTimeInMicroSeconds(&start);
        start_time = startTime;
        WalPrint("component start_time: %llu\n",start_time);
    }
    ret = CcspBaseIf_DeleteTblRow(
        bus_handle,
        compName,
        dbusPath,
        0,
        object
    );
    if(timeSpan)
    {
        endTime = getCurrentTimeInMicroSeconds(&end);
        timeDuration += endTime - startTime;

        timeSpan->spans[0].name = strdup(compName);
        WalPrint("timeSpan->spans[0].name : %s\n",timeSpan->spans[0].name);
        WalPrint("start_time : %llu\n",start_time);
        timeSpan->spans[0].start = start_time;
        WalPrint("timeSpan->spans[0].start : %llu\n",timeSpan->spans[0].start);
        WalPrint("timeDuration : %lu\n",timeDuration);
        timeSpan->spans[0].duration = timeDuration;
        WalPrint("timeSpan->spans[0].duration : %lu\n",timeSpan->spans[0].duration);
    }
    WalPrint("ret = %d\n",ret);    
    if ( ret == CCSP_SUCCESS )
    {
        WalPrint("Execution succeed.\n");
        WalInfo("%s is deleted.\n", object);
    }
    else
    {
        WalError("Execution fail ret :%d\n", ret);
    }
    WalPrint("<==========End of deleteRow ========>\n ");
    return ret;
}
