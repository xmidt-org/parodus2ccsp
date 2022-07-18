/**
 * @file webpa_adapter.c
 *
 * @description This file describes the Webpa Abstraction Layer
 *
 * Copyright (c) 2015  Comcast
 */

#include "webpa_internal.h"

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
/*                            Extern Scoped Variables                         */
/*----------------------------------------------------------------------------*/
extern pthread_cond_t applySetting_cond;
extern BOOL bRestartRadio1;
extern BOOL bRestartRadio2;

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/

void addRowTable(char *objectName, TableData *list,char **retObject, WDMP_STATUS *retStatus)
{
    int ret = 0, index =0, status =0, retUpdate = 0, retDel = 0;
    char paramName[MAX_PARAMETERNAME_LEN] = { 0 };
    char compName[MAX_PARAMETERNAME_LEN/2] = { 0 };
    char dbusPath[MAX_PARAMETERNAME_LEN/2] = { 0 };
    char tempParamName[MAX_PARAMETERNAME_LEN] = { 0 };

    WalPrint("objectName : %s\n",objectName);
    walStrncpy(paramName,objectName,sizeof(paramName));
    WalPrint("paramName before mapping : %s\n",paramName);
    status=IndexMpa_WEBPAtoCPE(paramName);
    if(status == -1)
    {
        if(strstr(paramName, PARAM_RADIO_OBJECT) != NULL)
        {
            ret = CCSP_ERR_INVALID_RADIO_INDEX;
            WalError("%s has invalid Radio index, Valid indexes are 10000, 10100 and 10200. ret = %d\n", paramName,ret); 
        }
        else
        {
            ret = CCSP_ERR_INVALID_WIFI_INDEX;
            WalError("%s has invalid WiFi index, Valid range is between 10001-10008, 10101-10108 and 10201-10208. ret = %d\n",paramName, ret);
        }
    }
    else
    {
        WalPrint("paramName after mapping : %s\n",paramName);
        ret = addRow(paramName,compName,dbusPath,&index);
        WalPrint("ret = %d index :%d\n",ret,index);
        WalPrint("parameterName: %s, CompName : %s, dbusPath : %s\n", paramName, compName, dbusPath);
        if(ret == CCSP_SUCCESS)
        {
            WalPrint("paramName : %s index : %d\n",paramName,index);
            snprintf(tempParamName,MAX_PARAMETERNAME_LEN,"%s%d.", paramName, index);
            WalPrint("tempParamName : %s\n",tempParamName);
            retUpdate = updateRow(tempParamName,list,compName,dbusPath);
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
                OnboardLog("Failed to update row hence deleting the added row %s\n",tempParamName);
                retDel = deleteRow(tempParamName);
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

void deleteRowTable(char *object,WDMP_STATUS *retStatus)
{
    int ret = 0,status = 0;
    char paramName[MAX_PARAMETERNAME_LEN] = { 0 };
    WalPrint("object : %s\n",object);
    walStrncpy(paramName,object,sizeof(paramName));
    WalPrint("paramName before mapping : %s\n",paramName);
    status=IndexMpa_WEBPAtoCPE(paramName);
    if(status == -1)
    {
        if(strstr(paramName, PARAM_RADIO_OBJECT) != NULL)
        {
            ret = CCSP_ERR_INVALID_RADIO_INDEX;
            WalError("%s has invalid Radio index, Valid indexes are 10000, 10100 and 10200. ret = %d\n", paramName,ret);
            OnboardLog("%s has invalid Radio index, Valid indexes are 10000, 10100 and 10200. ret = %d\n", paramName,ret);
        }
        else
        {
            ret = CCSP_ERR_INVALID_WIFI_INDEX;
            WalError("%s has invalid WiFi index, Valid range is between 10001-10008, 10101-10108 and 10201-10208. ret = %d\n",paramName, ret);
            OnboardLog("%s has invalid WiFi index, Valid range is between 10001-10008, 10101-10108 and 10201-10208. ret = %d\n",paramName, ret);
        }
    }
    else
    {
        WalPrint("paramName after mapping : %s\n",paramName);
        ret = deleteRow(paramName);
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

int addRow(char *object,char *compName,char *dbusPath,int *retIndex)
{
    int ret = 0, size = 0, index = 0;
    char dst_pathname_cr[MAX_PATHNAME_CR_LEN] = { 0 };
    char l_Subsystem[MAX_DBUS_INTERFACE_LEN] = { 0 };	
    componentStruct_t ** ppComponents = NULL;
#if !defined(RDKB_EMU)
    strncpy(l_Subsystem, "eRT.",sizeof(l_Subsystem));
#endif
    snprintf(dst_pathname_cr, sizeof(dst_pathname_cr),"%s%s", l_Subsystem, CCSP_DBUS_INTERFACE_CR);	

    WalPrint("<==========start of addRow ========>\n ");

    ret = CcspBaseIf_discComponentSupportingNamespace(bus_handle, dst_pathname_cr, object, l_Subsystem, &ppComponents, &size);

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
        OnboardLog("Parameter name %s is not supported. ret = %d\n", object, ret);
        free_componentStruct_t(bus_handle, size, ppComponents);
        return ret;
    }
    WalInfo("parameterName: %s, CompName : %s, dbusPath : %s\n", object, compName, dbusPath);
    ret = CcspBaseIf_AddTblRow(bus_handle, compName, dbusPath, 0, object, &index);
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

int updateRow(char *objectName,TableData *list,char *compName,char *dbusPath)
{
    int i=0, ret = -1,numParam =0, val_size = 0, retGet = -1;
    char **parameterNamesLocal = NULL; 
    char *faultParam = NULL;
    unsigned int writeID = CCSP_COMPONENT_ID_WebPA;
    parameterValStruct_t *val= NULL;
    parameterValStruct_t **parameterval = NULL;

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

    // To get dataType of parameter do bulk GET for all the input parameters in the requests
    retGet = CcspBaseIf_getParameterValues(bus_handle, compName, dbusPath, parameterNamesLocal, numParam, &val_size, &parameterval);
    WalPrint("After GPV ret: %d, val_size: %d\n",retGet,val_size);
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

        ret = CcspBaseIf_setParameterValues(bus_handle, compName, dbusPath, 0, writeID, val, numParam, TRUE, &faultParam);
        WalPrint("ret : %d\n",ret);
        if((ret != CCSP_SUCCESS) && (faultParam != NULL))
        {
            WAL_FREE(faultParam);
        }
        if(!strcmp(compName,RDKB_WIFI_FULL_COMPONENT_NAME))
        {
            identifyRadioIndexToReset(numParam,val,&bRestartRadio1,&bRestartRadio2);
            pthread_cond_signal(&applySetting_cond);
        }
    }
    else
    {
        ret = retGet;
    }
    if(ret != CCSP_SUCCESS)
    {
        WalError("Failed to update row %d\n",ret);
        OnboardLog("Failed to update row %d\n",ret);
    }

    for(i =0; i<numParam; i++)
    {
        WAL_FREE(parameterNamesLocal[i]);
    }
    WAL_FREE(parameterNamesLocal);
    WAL_FREE(val);
    WalPrint("<==========End of updateRow ========>\n ");
    return ret;
}

int deleteRow(char *object)
{
    int ret = 0, size =0;
    char compName[MAX_PARAMETERNAME_LEN/2] = { 0 };
    char dbusPath[MAX_PARAMETERNAME_LEN/2] = { 0 };
    char dst_pathname_cr[MAX_PATHNAME_CR_LEN] = { 0 };
    char l_Subsystem[MAX_DBUS_INTERFACE_LEN] = { 0 };
    componentStruct_t ** ppComponents = NULL;
#if !defined(RDKB_EMU)
    strncpy(l_Subsystem, "eRT.",sizeof(l_Subsystem));
#endif
    snprintf(dst_pathname_cr, sizeof(dst_pathname_cr),"%s%s", l_Subsystem, CCSP_DBUS_INTERFACE_CR);

    WalPrint("<==========Start of deleteRow ========>\n ");

    ret = CcspBaseIf_discComponentSupportingNamespace(bus_handle, dst_pathname_cr, object, l_Subsystem, &ppComponents, &size);
    WalPrint("size : %d, ret : %d\n",size,ret);

    if (ret == CCSP_SUCCESS && size == 1)
    {
        walStrncpy(compName,ppComponents[0]->componentName,sizeof(compName));
        walStrncpy(dbusPath,ppComponents[0]->dbusPath,sizeof(dbusPath));
        free_componentStruct_t(bus_handle, size, ppComponents);
    }
    else
    {
        WalError("Parameter name %s is not supported. ret = %d\n", object, ret);
        OnboardLog("Parameter name %s is not supported. ret = %d\n", object, ret);
        free_componentStruct_t(bus_handle, size, ppComponents);
        return ret;
    }
    WalInfo("parameterName: %s, CompName : %s, dbusPath : %s\n", object, compName, dbusPath);
    ret = CcspBaseIf_DeleteTblRow(bus_handle, compName, dbusPath, 0, object);
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
