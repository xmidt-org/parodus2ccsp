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
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/
static void getTableRows(char *objectName,parameterValStruct_t **parameterval, int paramCount, int *numRows,char ***rowObjects);
static void contructRollbackTableData(parameterValStruct_t **parameterval,int paramCount,char ***rowList,int rowCount, int *numParam,TableData ** getList);
static void getWritableParams(char *paramName, char ***writableParams, int *paramCount);
static int getComponentInfoFromCache(char *parameterName, char *objectName, char *compName, char *dbusPath);
static int addRow(char *object,char *compName,char *dbusPath,int *retIndex);
static int updateRow(char *objectName,TableData *list,char *compName,char *dbusPath);
static int deleteRow(char *object);
static int cacheTableData(char *objectName,int paramcount,char ***rowList,int *numRows,int *params,TableData ** list);
static int addNewData(char *objectName,TableData * list,int paramcount);
static void deleteAllTableData(char **deleteList,int rowCount);
static void addCachedData(char *objectName,TableData * addList,int rowCount);

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

void replaceTable(char *objectName,TableData * list,unsigned int paramcount,WDMP_STATUS *retStatus)
{
	int cnt = 0, ret = 0,numParams = 0,retIndex =0,addRet =0, isWalStatus = 0, cnt1 =0, rowCount = 0;
	char **deleteList = NULL;
	TableData * addList = NULL;
	char paramName[MAX_PARAMETERNAME_LEN] = {'\0'};
	WalPrint("<==========Start of replaceTable ========>\n ");
	strncpy(paramName,objectName,sizeof(paramName));
	WalPrint("paramName before Mapping : %s\n",paramName);
	// Index mapping 
	retIndex=IndexMpa_WEBPAtoCPE(paramName);
	if(retIndex == -1)
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
		ret = cacheTableData(paramName,paramcount,&deleteList,&rowCount,&numParams,&addList);
		WalPrint("ret : %d rowCount %d numParams: %d\n",ret,rowCount,numParams);
		if(ret == CCSP_SUCCESS)
		{
			WalInfo("Table (%s) has %d rows",paramName,rowCount);
			if(rowCount > 0)
			{
				WalPrint("-------- Printing table data ----------\n");
				for(cnt =0; cnt < rowCount; cnt++)
				{	
					WalPrint("deleteList[%d] : %s\n",cnt,deleteList[cnt]);
					if(paramcount != 0)
					{
					WalPrint("addList[%d].paramCnt : %d\n",cnt,addList[cnt].paramCnt);
						for (cnt1 = 0; cnt1 < addList[cnt].paramCnt; cnt1++)
						{
							WalPrint("addList[%d].names[%d] : %s,addList[%d].values[%d] : %s\n ",cnt,cnt1,addList[cnt].names[cnt1],cnt,cnt1,addList[cnt].values[cnt1]);
						}
					}
				}
				WalPrint("-------- Printed %d rows----------\n",rowCount);
			}
			deleteAllTableData(deleteList,rowCount);
			if(paramcount != 0)
			{
				addRet = addNewData(objectName,list,paramcount);
				ret = addRet;
				isWalStatus = 1;
				if(addRet != WDMP_SUCCESS && rowCount > 0)
				{
					WalError("Failed to replace table, hence reverting the changes\n");
					addCachedData(objectName,addList,rowCount);
				}	
				for ( cnt = 0 ; cnt < rowCount ; cnt++)
				{
					for(cnt1 = 0; cnt1 < numParams; cnt1++)
					{
						WAL_FREE(addList[cnt].names[cnt1]);
						WAL_FREE(addList[cnt].values[cnt1]);
					}
					WAL_FREE(addList[cnt].names);
					WAL_FREE(addList[cnt].values);
				}
				WAL_FREE(addList);
			}	
		}
	}
	if(isWalStatus == 1)
	{
		*retStatus = ret;
	}
	else
	{
		*retStatus = mapStatus(ret);
	}
	WalPrint("Finally ----> ret: %d retStatus : %d\n",ret,*retStatus);
    WalPrint("<==========End of replaceTable ========>\n ");	
}

/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/

/**
 * @brief addRow adds new row to the table
 *
 * param[in] object table name
 * @param[in] CompName Component Name of parameters
 * @param[in] dbusPath dbus path of parameters
 * param[out] retIndex return new row added
 */
static int addRow(char *object,char *compName,char *dbusPath,int *retIndex)
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
	
	ret = CcspBaseIf_discComponentSupportingNamespace(bus_handle,
			dst_pathname_cr, object, l_Subsystem, &ppComponents, &size);
			
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
	ret = CcspBaseIf_AddTblRow(
                bus_handle,
                compName,
                dbusPath,
                0,
                object,
                &index
            );
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

/**
 * @brief updateRow updates row data
 *
 * param[in] objectName table name
 * param[in] list Parameter name/value pairs
 * param[out] retObject return new row added
 * param[out] retStatus Returns status
 */
static int updateRow(char *objectName,TableData *list,char *compName,char *dbusPath)
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
	retGet = CcspBaseIf_getParameterValues(bus_handle,
				compName, dbusPath,
				parameterNamesLocal,
				numParam, &val_size, &parameterval);
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
        WalPrint("<==========End of updateRow ========>\n ");
        return ret;
         
}

/**
 * @brief deleteRow deletes table row
 *
 * param[in] object table name to delete
 */
static int deleteRow(char *object)
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
	
	ret = CcspBaseIf_discComponentSupportingNamespace(bus_handle,
			dst_pathname_cr, object, l_Subsystem, &ppComponents, &size);
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
	ret = CcspBaseIf_DeleteTblRow(
                bus_handle,
                compName,
                dbusPath,
                0,
                object
            );
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

/**
 * @brief cacheTableData stores current data in the table into array of TableData
 *
 * param[in] objectName table name
 * param[in] paramcount no.of rows in the given data
 * param[out] rowList list of rowObjects in the array  
 * param[out] numRows return no.of rows cached
 * param[out] params return no.of params in each row 
 * param[out] list return rows with data
 */
static int cacheTableData(char *objectName,int paramcount,char ***rowList,int *numRows,int *params,TableData ** list)
{
	int cnt =0,val_size = 0,ret = 0,size =0,rowCount = 0, cnt1=0;
	char dst_pathname_cr[MAX_PATHNAME_CR_LEN] = {'\0'};
	char l_Subsystem[MAX_DBUS_INTERFACE_LEN] = {'\0'};
	componentStruct_t ** ppComponents = NULL;
#if !defined(RDKB_EMU)
	strncpy(l_Subsystem, "eRT.",sizeof(l_Subsystem));
#endif
	char *parameterNames[1];
	char **rows = NULL;
	char paramName[MAX_PARAMETERNAME_LEN] = {'\0'};
	char *p = NULL;
	p = &paramName;
	TableData * getList = NULL;
	snprintf(dst_pathname_cr, sizeof(dst_pathname_cr),"%s%s", l_Subsystem, CCSP_DBUS_INTERFACE_CR);
	parameterValStruct_t **parameterval = NULL;
	WalPrint("<================ Start of cacheTableData =============>\n ");
	ret = CcspBaseIf_discComponentSupportingNamespace(bus_handle,dst_pathname_cr, objectName, l_Subsystem, &ppComponents, &size);
	WalPrint("size : %d, ret : %d\n",size,ret);
	if (ret == CCSP_SUCCESS && size == 1)
	{
		WalInfo("parameterName: %s, CompName : %s, dbusPath : %s\n", objectName, ppComponents[0]->componentName, ppComponents[0]->dbusPath);
		strncpy(paramName, objectName, sizeof(paramName));
		parameterNames[0] = p;
		ret = CcspBaseIf_getParameterValues(bus_handle,	ppComponents[0]->componentName, ppComponents[0]->dbusPath,  parameterNames,	1, &val_size, &parameterval);
		WalPrint("ret = %d val_size = %d\n",ret,val_size);
		if(ret == CCSP_SUCCESS && val_size > 0)
		{
			for (cnt = 0; cnt < val_size; cnt++)
			{
				WalPrint("parameterval[%d]->parameterName : %s,parameterval[%d]->parameterValue : %s\n ",cnt,parameterval[cnt]->parameterName,cnt,parameterval[cnt]->parameterValue);    
			}
			getTableRows(objectName,parameterval,val_size,&rowCount,&rows);
			WalPrint("rowCount : %d\n",rowCount);
			*rowList = rows;
			*numRows = rowCount;
			for(cnt = 0; cnt < rowCount; cnt++)
			{
				WalPrint("(*rowList)[%d] %s\n",cnt,(*rowList)[cnt]);
			}
			if(rowCount > 0 && paramcount > 0)
			{
				contructRollbackTableData(parameterval,val_size,rowList,rowCount,params,&getList);
				*list = getList;
				for(cnt =0; cnt < rowCount; cnt++)
				{	
					WalPrint("(*list)[%d].paramCnt : %d\n",cnt,(*list)[cnt].paramCnt);
					for (cnt1 = 0; cnt1 < (*list)[cnt].paramCnt; cnt1++)
					{
						WalPrint("(*list)[%d].names[%d] : %s,(*list)[%d].values[%d] : %s\n ",cnt,cnt1,(*list)[cnt].names[cnt1],cnt,cnt1,(*list)[cnt].values[cnt1]);
					}
				}
			}	
		}
		else
		{
			if(val_size == 0)
			{
				WalInfo("Table %s is EMPTY\n",objectName);
				*numRows = 0;
				*params = 0;
				rowList = NULL;
				list = NULL;
			}
		}
		free_componentStruct_t(bus_handle, size, ppComponents);
		free_parameterValStruct_t (bus_handle,val_size,parameterval);
	}
	else
	{
		WalError("Parameter name %s is not supported. ret = %d\n", objectName, ret);
		free_componentStruct_t(bus_handle, size, ppComponents);
		return ret;
	}
	WalPrint("<================ End of cacheTableData =============>\n ");
	return ret;
}

/**
 * @brief deleteAllTableData deletes all table data 
 *
 * param[in] deleteList array of rows from cached data
 * param[in] rowCount no.of rows to delete
 */
static void deleteAllTableData(char **deleteList,int rowCount)
{
	int cnt =0, delRet = 0; 
	WalPrint("---------- Start of deleteAllTableData -----------\n");
	for(cnt =0; cnt < rowCount; cnt++)
	{	
		delRet = deleteRow(deleteList[cnt]);
		WalPrint("delRet: %d\n",delRet);
		if(delRet != CCSP_SUCCESS)
		{
			WalError("deleteList[%d] :%s failed to delete\n",cnt,deleteList[cnt]);
		}
		WAL_FREE(deleteList[cnt]);
	}
	WAL_FREE(deleteList); 
	WalPrint("---------- End of deleteAllTableData -----------\n");
}

/**
 * @brief addNewData adds new data to the table 
 * param[in] objectName table name 
 * param[in] list table data to add
 * param[in] paramcount no.of rows to add
 */
static int addNewData(char *objectName,TableData * list,int paramcount)
{
	int cnt = 0,addRet = 0, i =0, delRet = 0;
	char paramName[MAX_PARAMETERNAME_LEN] = {'\0'};
char **retObject = NULL;
	retObject = (char **)malloc(sizeof(char*) * paramcount);
	memset(retObject,0,(sizeof(char *) * paramcount));
	WalPrint("---------- Start of addNewData -----------\n");
	for(cnt =0; cnt < paramcount; cnt++)
	{				
		strncpy(paramName,objectName,sizeof(paramName));
		retObject[cnt] = (char *)malloc(sizeof(char) * MAX_PARAMETERNAME_LEN);
		addRowTable(paramName,&list[cnt],&retObject[cnt],&addRet);
		WalPrint("addRet : %d\n",addRet);
		if(addRet != WDMP_SUCCESS)
		{
			WalError("Failed to add/update row to %s table, addRet : %d, hence deleting the already added rows\n", objectName, addRet);
			for(i= cnt-1; i >= 0; i--)
			{
				strncpy(paramName,retObject[i],sizeof(paramName));
				deleteRowTable(paramName, &delRet);
				WalPrint("delRet : %d\n",delRet);
				if(delRet != WDMP_SUCCESS)
				{
					WalError("retObject[%d] :%s failed to delete, delRet %d\n",i,retObject[i], delRet);
					break;
				}		   
			}
			break;
		}						
	}
	for(cnt =0; cnt < paramcount; cnt++)
	{
		WAL_FREE(retObject[cnt]);
	}
	WAL_FREE(retObject);
	WalPrint("---------- End of addNewData -----------\n");
	return addRet;
}

/**
 * @brief addCachedData adds stored data to the table on roll back
 * param[in] objectName table name
 * param[in] addList list of table data to add
 * param[in] rowCount no.of rows to add
 */
static void addCachedData(char *objectName,TableData * addList,int rowCount)
{
	int cnt = 0, addRet = 0;
	char paramName[MAX_PARAMETERNAME_LEN] = {'\0'};
	char **retRows = NULL;
	retRows = (char **)malloc(sizeof(char*) * rowCount);
	memset(retRows,0,(sizeof(char *) * rowCount));
	WalPrint("---------- Start of addCachedData -----------\n");
	for(cnt =0; cnt < rowCount; cnt++)
	{
		retRows[cnt] = (char *)malloc(sizeof(char) * MAX_PARAMETERNAME_LEN);
		strncpy(paramName,objectName,sizeof(paramName));
		addRowTable(paramName,&addList[cnt],&retRows[cnt],&addRet);
		WalPrint("addRet : %d\n",addRet);
		if(addRet == WDMP_SUCCESS)
		{
			WalInfo("%s row is successfully added\n",retRows[cnt]);
		}
		WAL_FREE(retRows[cnt]);
	}					
	WAL_FREE(retRows);
	WalPrint("---------- End of addCachedData -----------\n");
}

/**
 * @brief getTableRows dynamically gets list of rowObjects and row count
 *
 * param[in] objectName table name
 * param[in] parameterval arry of parameterValStruct 
 * param[in] paramCount total parameters in table  
 * param[out] numRows return no.of rows in table
 * param[out] rowObjects return array of rowObjects
 */
static void getTableRows(char *objectName,parameterValStruct_t **parameterval, int paramCount, int *numRows,char ***rowObjects)
{
	int rows[MAX_ROW_COUNT] = {'\0'};
	int len = 0, cnt = 0, cnt1=0, objLen = 0, index = 0, rowCount = 0;
	char subStr[MAX_PARAMETERNAME_LEN] = {'\0'};
	char tempName[MAX_PARAMETERNAME_LEN] = {'\0'};
	WalPrint("---------- Start of getTableRows -----------\n");
	objLen = strlen(objectName);
	for (cnt = 0; cnt < paramCount; cnt++)
	{
		len = strlen(parameterval[cnt]->parameterName);
		strncpy (subStr, parameterval[cnt]->parameterName + objLen, len-objLen);
		subStr[len-objLen] = '\0';
		sscanf(subStr,"%d.%s", &index, tempName);
		WalPrint("index : %d tempName : %s\n",index,tempName);
		if(cnt == 0)
		{
			rows[cnt1] = index;
		}
		else if(rows[cnt1] != index)
		{
			cnt1++;
			rows[cnt1] = index;
		} 		
	}
	rowCount = cnt1+1;
	WalPrint("rowCount : %d\n",rowCount);
	if(rowCount > 0)
	{
		*numRows = rowCount;
		*rowObjects = (char **)malloc(sizeof(char *) * rowCount);
		for(cnt = 0; cnt < rowCount; cnt++)
		{
			(*rowObjects)[cnt] = (char *)malloc(sizeof(char) * MAX_PARAMETERNAME_LEN);
			WalPrint("rows[%d] %d\n",cnt,rows[cnt]);
			snprintf((*rowObjects)[cnt],MAX_PARAMETERNAME_LEN,"%s%d.", objectName, rows[cnt]);
			WalPrint("(*rowObjects)[%d] %s\n",cnt,(*rowObjects)[cnt]);
		}
	}
	else
	{
		*numRows = 0;
		rowObjects = NULL;
	}
	WalPrint("---------- End of getTableRows -----------\n");
}

/**
 * @brief contructRollbackTableData constructs table data with name and value for roll back
 *
 * param[in] parameterval array of parameter values in table 
 * param[in] paramCount total parameters in table
 * param[in] rowList list of rows
 * param[in] rowCount no.of rows 
 * param[out] numParam return no.of paramters for each row
 * param[out] getList return list of table data with name and value
 */
static void contructRollbackTableData(parameterValStruct_t **parameterval,int paramCount,char ***rowList,int rowCount, int *numParam,TableData ** getList)
{
	int writableParamCount = 0, cnt = 0, cnt1 = 0, i = 0, params = 0;
	char **writableList = NULL;
	WalPrint("---------- Start of contructRollbackTableData -----------\n");
	getWritableParams((*rowList[0]), &writableList, &writableParamCount);
	WalInfo("writableParamCount : %d\n",writableParamCount);
	*numParam = writableParamCount;
	*getList = (TableData *) malloc(sizeof(TableData) * rowCount);
	params = paramCount / rowCount;
	for(cnt = 0; cnt < rowCount; cnt++)
	{
		(*getList)[cnt].paramCnt = writableParamCount;
		(*getList)[cnt].names = (char **)malloc(sizeof(char *) * writableParamCount);
		(*getList)[cnt].values = (char **)malloc(sizeof(char *) * writableParamCount);
		i = 0;
		cnt1 = cnt * params;
		for(; cnt1 < paramCount; cnt1++)
		{
			if(strstr(parameterval[cnt1]->parameterName,(*rowList)[cnt]) != NULL &&
					strstr(parameterval[cnt1]->parameterName,writableList[i]) != NULL)
			{
				WalPrint("parameterval[%d]->parameterName : %s,parameterval[%d]->parameterValue : %s\n ",cnt1,parameterval[cnt1]->parameterName,cnt1,parameterval[cnt1]->parameterValue);
				(*getList)[cnt].names[i] = (char *)malloc(sizeof(char) * MAX_PARAMETERNAME_LEN);
				(*getList)[cnt].values[i] = (char *)malloc(sizeof(char) * MAX_PARAMETERNAME_LEN);
				strcpy((*getList)[cnt].names[i],writableList[i]);
				WalPrint("(*getList)[%d].names[%d] : %s\n",cnt, i, (*getList)[cnt].names[i]);
				strcpy((*getList)[cnt].values[i],parameterval[cnt1]->parameterValue);
				WalPrint("(*getList)[%d].values[%d] : %s\n",cnt, i, (*getList)[cnt].values[i]);
				i++;
			}
		}
	}
	
	for(cnt = 0; cnt < writableParamCount; cnt++)
	{
		WAL_FREE(writableList[cnt]);
	}
	WAL_FREE(writableList);	
	WalPrint("---------- End of contructRollbackTableData -----------\n");
}

/**
 * @brief getWritableParams gets writable parameters from stack
 * param[in] paramName row object
 * param[out] writableParams return list of writable params
 * param[out] paramCount count of writable params
 */
static void getWritableParams(char *paramName, char ***writableParams, int *paramCount)
{
	int cnt =0,val_size = 0,ret = 0, size = 0, cnt1 = 0, writableCount = 0, len = 0;
	char dst_pathname_cr[MAX_PATHNAME_CR_LEN] = { 0 };
	char l_Subsystem[MAX_DBUS_INTERFACE_LEN] = { 0 };
	componentStruct_t ** ppComponents = NULL;
	char *tempStr = NULL;
	char temp[MAX_PARAMETERNAME_LEN] = { 0 };
#if !defined(RDKB_EMU)
	strncpy(l_Subsystem, "eRT.",sizeof(l_Subsystem));
#endif
	parameterInfoStruct_t **parameterInfo = NULL;
	WalPrint("==================== Start of getWritableParams ==================\n");
	snprintf(dst_pathname_cr, sizeof(dst_pathname_cr),"%s%s", l_Subsystem, CCSP_DBUS_INTERFACE_CR);
	ret = CcspBaseIf_discComponentSupportingNamespace(bus_handle,dst_pathname_cr, paramName, l_Subsystem, &ppComponents, &size);
	WalPrint("size : %d, ret : %d\n",size,ret);
	if (ret == CCSP_SUCCESS && size == 1)
	{
		WalInfo("parameterName: %s, CompName : %s, dbusPath : %s\n", paramName, ppComponents[0]->componentName, ppComponents[0]->dbusPath);
		ret = CcspBaseIf_getParameterNames(bus_handle,ppComponents[0]->componentName, ppComponents[0]->dbusPath, paramName,1,&val_size,&parameterInfo);
		WalPrint("val_size : %d, ret : %d\n",val_size,ret);
		if(ret == CCSP_SUCCESS && val_size > 0)
		{
			*writableParams = (char **)malloc(sizeof(char *) * val_size);
			cnt1 = 0;
			for(cnt = 0; cnt < val_size; cnt++)
			{
				
				len = strlen(paramName);
				if(parameterInfo[cnt]->writable == 1)
				{
					strcpy(temp, parameterInfo[cnt]->parameterName);
					tempStr =temp + len;
					WalPrint("tempStr : %s\n",tempStr);
					if(strcmp(tempStr ,ALIAS_PARAM) == 0)
					{
						WalInfo("Skipping Alias parameter \n");
					}
					else
					{
						(*writableParams)[cnt1] = (char *)malloc(sizeof(char) * MAX_PARAMETERNAME_LEN);
						strcpy((*writableParams)[cnt1],tempStr);	
						cnt1++;
					}	
				}
			}
			*paramCount = writableCount = cnt1;
			WalPrint("writableCount %d\n",writableCount);
		}
		else
		{
			if(val_size == 0)
			{
				WalInfo("Table %s is EMPTY\n",paramName);
				*paramCount = 0;
				writableParams = NULL;
			}
		}
		free_componentStruct_t(bus_handle, size, ppComponents);
		free_parameterInfoStruct_t (bus_handle, val_size, parameterInfo);
	}
	else
	{
		WalError("Parameter name %s is not supported. ret = %d\n", paramName, ret);
		free_componentStruct_t(bus_handle, size, ppComponents);
	}
	WalPrint("==================== End of getWritableParams ==================\n");
}

