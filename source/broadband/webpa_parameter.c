/**
 * @file webpa_parameter.c
 *
 * @description This file describes the Webpa Abstraction Layer
 *
 * Copyright (c) 2015  Comcast
 */
#include "webpa_internal.h"
#include "webpa_notification.h"

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
#define RDKB_WEBPA_FULL_COMPONENT_NAME      "eRT.com.cisco.spvtg.ccsp.webpaagent"
/*----------------------------------------------------------------------------*/
/*                               Data Structures                              */
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
BOOL bRadioRestartEn = FALSE;
BOOL bRestartRadio1 = FALSE;
BOOL bRestartRadio2 = FALSE;
pthread_mutex_t applySetting_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t applySetting_cond = PTHREAD_COND_INITIALIZER;
static char current_transaction_id[MAX_PARAMETERVALUE_LEN] = {'\0'};

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/

static int getParamValues(char *parameterNames[], int paramCount, char *CompName, char *dbusPath, money_trace_spans *timeSpan, int paramIndex,int startIndex, param_t ***paramArr,int *TotalParams);
static void free_set_param_values_memory(parameterValStruct_t* val, int paramCount, char * faultParam);
static void free_paramVal_memory(param_t ** val, int paramCount);
static int prepare_parameterValueStruct(parameterValStruct_t* val, param_t *paramVal, char *paramName);
static int setParamValues(param_t *paramVal, char *CompName, char *dbusPath, int paramCount,const WEBPA_SET_TYPE setType, char *transactionId);
static void *applyWiFiSettingsTask();
static void identifyRadioIndexToReset(int paramCount, parameterValStruct_t* val,BOOL *bRestartRadio1,BOOL *bRestartRadio2); 
BOOL applySettingsFlag;

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/

void getValues(const char *paramName[], const unsigned int paramCount, int index, money_trace_spans *timeSpan, param_t ***paramArr, int *retValCount, WDMP_STATUS *retStatus)
{
    int cnt1=0,cnt2=0, ret = -1, error = 0, compCount=0, count = 0, i = 0,retCount = 0, totalParams = 0;
    int startIndex = 0, isLargeWildCard = 0;
    char parameterName[MAX_PARAMETERNAME_LEN] = {'\0'};
    ParamCompList *ParamGroup = NULL;
    char **compName = NULL;
    char **dbusPath = NULL;
    WalPrint("------------- getValues -------------\n");
    for(cnt1 = 0; cnt1 < paramCount; cnt1++)
    {
        WalPrint("paramName[%d] : %s\n",cnt1,paramName[cnt1]);
        // Get the matching component index from cache
        walStrncpy(parameterName,paramName[cnt1],sizeof(parameterName));
        // To get list of component name and dbuspath
        ret = getComponentDetails(parameterName,&compName,&dbusPath,&error,&count);
        if(error == 1)
        {
            break;
        }
        WalPrint("parameterName: %s count: %d\n",parameterName,count);
        for(i = 0; i < count; i++)
        {
            WalPrint("compName[%d] : %s, dbusPath[%d] : %s\n", i,compName[i],i, dbusPath[i]);
            prepareParamGroups(&ParamGroup,paramCount,cnt1,parameterName,compName[i],dbusPath[i],&compCount);
        }
        free_componentDetails(compName,dbusPath,count);
    }//End of for loop

    WalPrint("Number of parameter groups : %d\n",compCount);

    if(error != 1)
    {
        isLargeWildCard = 0;
        WalPrint("compCount : %d paramCount: %d\n",compCount,paramCount);
        if(compCount > paramCount)
        {
            WalPrint("compCount is greater than paramCount\n");
            isLargeWildCard = 1;
        }

        for(cnt1 = 0; cnt1 < compCount; cnt1++)
        {
            WalPrint("------------- Parameter group -------------\n");
            WalPrint("ParamGroup[%d].comp_name :%s, ParamGroup[%d].dbus_path :%s, ParamGroup[%d].parameterCount :%d\n",cnt1,ParamGroup[cnt1].comp_name, cnt1,ParamGroup[cnt1].dbus_path, cnt1,ParamGroup[cnt1].parameterCount);

            for(cnt2 = 0; cnt2 < ParamGroup[cnt1].parameterCount; cnt2++)
            {
                WalPrint("ParamGroup[%d].parameterName :%s\n",cnt1,ParamGroup[cnt1].parameterName[cnt2]);
            }

            if(!strcmp(ParamGroup[cnt1].comp_name,RDKB_WIFI_FULL_COMPONENT_NAME) && applySettingsFlag == TRUE) 
            {
                ret = CCSP_ERR_WIFI_BUSY;
                WalError("WiFi component is busy\n");
                break;
            }
            WalPrint("index: %d startIndex: %d\n",index, startIndex);

            if(isLargeWildCard == 1)
            {
                ret = getParamValues(ParamGroup[cnt1].parameterName, ParamGroup[cnt1].parameterCount, ParamGroup[cnt1].comp_name, ParamGroup[cnt1].dbus_path, timeSpan, index, startIndex, paramArr,&retCount);
                startIndex = startIndex + retCount;
            }
            else
            {
                startIndex = 0;
                ret = getParamValues(ParamGroup[cnt1].parameterName, ParamGroup[cnt1].parameterCount, ParamGroup[cnt1].comp_name, ParamGroup[cnt1].dbus_path, timeSpan, index, startIndex, paramArr,&retCount);
                index = index + ParamGroup[cnt1].parameterCount;   
            }
            WalPrint("After getParamValues index = %d ,startIndex : %d retCount =  %d\n",index,startIndex,retCount);
            if(ret != CCSP_SUCCESS)
            {
                WalError("Get Atomic Values call failed for ParamGroup[%d]->comp_name :%s ret: %d\n",cnt1,ParamGroup[cnt1].comp_name,ret);
                break;
            }
            totalParams = totalParams + retCount;
            WalPrint("totalParams : %d\n",totalParams);
        }
    }
    retValCount[0] = totalParams;
    *retStatus = mapStatus(ret);
    free_ParamCompList(ParamGroup, compCount);		
}

void setValues(const param_t paramVal[], const unsigned int paramCount, const WEBPA_SET_TYPE setType,char *transactionId, money_trace_spans *timeSpan, WDMP_STATUS *retStatus)
{
        int cnt = 0, ret = 0, cnt1 =0, i = 0, count = 0, error = 0, compCount = 0, cnt2= 0, j = 0;
        int index = 0,retCount = 0,checkSetstatus = 0,rev=0,indexWifi= -1,getFlag=0;
        char parameterName[MAX_PARAMETERNAME_LEN] = {'\0'};
        ParamCompList *ParamGroup = NULL;
        char **compName = NULL;
        char **dbusPath = NULL;
        param_t **val = NULL;
        param_t **rollbackVal = NULL;
        param_t **storeGetValue = NULL;// To store param values before failure occurs

        WalPrint("=============== Start of setValues =============\n");
        for(cnt1 = 0; cnt1 < paramCount; cnt1++)
        {
                walStrncpy(parameterName,paramVal[cnt1].name,sizeof(parameterName));
                // To get list of component name and dbuspath
                ret = getComponentDetails(parameterName,&compName,&dbusPath,&error,&count);
                if(error == 1)
                {
                        break;
                }
                WalPrint("parameterName: %s count: %d\n",parameterName,count);
                for(i = 0; i < count; i++)
                {
                        WalPrint("compName[%d] : %s, dbusPath[%d] : %s\n", i,compName[i],i, dbusPath[i]);
                        prepareParamGroups(&ParamGroup,paramCount,cnt1,parameterName,compName[i],dbusPath[i],&compCount);
                }
                free_componentDetails(compName,dbusPath,count);
        }

        if(error != 1)
        {
                WalPrint("Number of parameter groups : %d\n",compCount);

                val = (param_t **) malloc(sizeof(param_t *) * compCount);
                memset(val,0,(sizeof(param_t *) * compCount));

                rollbackVal = (param_t **) malloc(sizeof(param_t *) * compCount);
                memset(rollbackVal,0,(sizeof(param_t *) * compCount));

                storeGetValue = (param_t **)malloc(sizeof(param_t *) * paramCount);
                memset(storeGetValue,0,(sizeof(param_t *) * paramCount));
                
                for(j = 0; j < compCount ;j++)
                {
                        WalPrint("ParamGroup[%d].comp_name :%s, ParamGroup[%d].dbus_path :%s, ParamGroup[%d].parameterCount :%d\n",j,ParamGroup[j].comp_name, j,ParamGroup[j].dbus_path, j,ParamGroup[j].parameterCount);

                        val[j] = (param_t *) malloc(sizeof(param_t) * ParamGroup[j].parameterCount);
                        rollbackVal[j] = (param_t *) malloc(sizeof(param_t) * ParamGroup[j].parameterCount);
                }

                WalPrint("--------- Start of SET Atomic caching -------\n");
                for (i = 0; i < compCount; i++)
                {
                        if(!strcmp(ParamGroup[i].comp_name,RDKB_WIFI_FULL_COMPONENT_NAME) && applySettingsFlag == TRUE)
                        {
                                ret = CCSP_ERR_WIFI_BUSY;
                                WalError("WiFi component is busy\n");
                                getFlag = 1;
                                break;
                        }
                        
                        WalPrint("B4 getParamValues index = %d\n", index);
                        //GET values for rollback purpose
                        ret = getParamValues(ParamGroup[i].parameterName, ParamGroup[i].parameterCount, ParamGroup[i].comp_name, ParamGroup[i].dbus_path, timeSpan, index, 0, &storeGetValue,&retCount);
		  	WalPrint("After getParamValues index = %d , retCount =  %d\n",index,retCount);
                        if(ret != CCSP_SUCCESS)
                        {
                                WalError("Get Atomic Values call failed for ParamGroup[%d]->comp_name :%s ret: %d\n",i,ParamGroup[i].comp_name,ret);
                                getFlag = 1;

                                for(cnt1=index-1;cnt1>=0;cnt1--)
                                {
                                        WAL_FREE(storeGetValue[cnt1]->name);
                                        WAL_FREE(storeGetValue[cnt1]->value);
                                        WAL_FREE(storeGetValue[cnt1]);
                                }
                                break;
                        }
                        else
                        {		 
                                for(j = 0; j < retCount ; j++)
                                {
                                        WalPrint("storeGetValue[%d]->name : %s, storeGetValue[%d]->value : %s, storeGetValue[%d]->type : %d\n",index,storeGetValue[index]->name,index,storeGetValue[index]->value,index,storeGetValue[index]->type);

                                        rollbackVal[i][j].name=storeGetValue[index]->name;
                                        rollbackVal[i][j].value=storeGetValue[index]->value;
                                        rollbackVal[i][j].type=storeGetValue[index]->type;

                                        WalPrint("rollbackVal[%d][%d].name : %s, rollbackVal[%d][%d].value : %s, rollbackVal[%d][%d].type : %d\n",i,j,rollbackVal[i][j].name,i,j,rollbackVal[i][j].value,i,j,rollbackVal[i][j].type);
                                        index++;
                                }		  	    		  	    		  	    
                        }
                }
                WalPrint("--------- End of SET Atomic caching -------\n");
                if(getFlag !=1)
                {		    
                        WalPrint("---- Start of preparing val struct ------\n");
                        for(cnt1 = 0; cnt1 < paramCount; cnt1++)
                        {
                                for(j = 0; j < compCount ;j++)
                                {
                                        for(cnt2 = 0; cnt2 < ParamGroup[j].parameterCount; cnt2++)
                                        {
                                                WalPrint("ParamGroup[%d].parameterName[%d] :%s\n",j,cnt2,ParamGroup[j].parameterName[cnt2]);
                                                WalPrint("paramVal[%d].name : %s\n",cnt1,paramVal[cnt1].name);
                                                if(strcmp(paramVal[cnt1].name,ParamGroup[j].parameterName[cnt2]) == 0)
                                                {
                                                        WalPrint("paramVal[%d].name : %s \n",cnt1,paramVal[cnt1].name);
                                                        val[j][cnt2].name = paramVal[cnt1].name;
                                                        WalPrint("val[%d][%d].name : %s \n",j,cnt2,val[j][cnt2].name);
                                                        WalPrint("paramVal[%d].value : %s\n",cnt1,paramVal[cnt1].value);
                                                        val[j][cnt2].value = paramVal[cnt1].value;
                                                        WalPrint("val[%d][%d].value : %s \n",j,cnt2,val[j][cnt2].value);
                                                        val[j][cnt2].type = paramVal[cnt1].type;
                                                }
                                        }
                                }
                        }//End of for loop
                        WalPrint("---- End of preparing val struct ------\n");

                        for (i = 0; i < compCount; i++)
                        {
                                if(!strcmp(ParamGroup[i].comp_name,RDKB_WIFI_FULL_COMPONENT_NAME) && applySettingsFlag == TRUE)
                                {
                                        ret = CCSP_ERR_WIFI_BUSY;
                                        WalError("WiFi component is busy\n");
                                        break;
                                }			

                                // Skip and do SET for Wifi component at the end 
                                if(!strcmp(ParamGroup[i].comp_name,RDKB_WIFI_FULL_COMPONENT_NAME))
                                {
                                        WalPrint("skip wifi set and get the index %d\n",i);
                                        indexWifi = i;
                                }
                                else
                                {
                                        WalPrint("ParamGroup[%d].comp_name : %s\n",i,ParamGroup[i].comp_name);
                                        ret = setParamValues(val[i], ParamGroup[i].comp_name,ParamGroup[i].dbus_path,ParamGroup[i].parameterCount, setType, transactionId);
                                        WalPrint("ret : %d\n",ret);
                                        if(ret != CCSP_SUCCESS)
                                        {
                                                WalError("Failed to do atomic set hence rollbacking the changes. ret :%d\n",ret);
                                                WalPrint("------ Start of rollback ------\n");
                                                // Rollback data in failure case
                                                for(rev =i-1;rev>=0;rev--)
                                                {
                                                        WalPrint("rev value inside for loop is  %d\n",rev);
                                                        //skip for wifi rollback
                                                        if(indexWifi != rev)
                                                        {
                                                                WalPrint("ParamGroup[%d].comp_name : %s\n",rev,ParamGroup[rev].comp_name);
                                                                checkSetstatus = setParamValues(rollbackVal[rev],ParamGroup[rev].comp_name,ParamGroup[rev].dbus_path, ParamGroup[rev].parameterCount, setType, transactionId);
                                                                WalPrint("checkSetstatus is : %d\n",checkSetstatus);
                                                                if(checkSetstatus != CCSP_SUCCESS)
                                                                {
                                                                        WalError("While rollback Failed to do atomic set. checkSetstatus :%d\n",checkSetstatus);
                                                                }
                                                        }
                                                        else
                                                        {
                                                                WalPrint("Skip rollback for WiFi\n");
                                                                indexWifi = -1;
                                                        }
                                                }
                                                WalPrint("------ End of rollback ------\n");				
                                                break;
                                        }
                                }
                        }
                        //Got wifi index and do SET
                        if(indexWifi !=-1)
                        {
                                WalPrint("Wifi SET at end\n");
                                WalPrint("ParamGroup[%d].comp_name : %s\n",indexWifi,ParamGroup[indexWifi].comp_name);
                                ret = setParamValues(val[indexWifi], ParamGroup[indexWifi].comp_name,ParamGroup[indexWifi].dbus_path, ParamGroup[indexWifi].parameterCount, setType, transactionId);
                                if(ret != CCSP_SUCCESS)
                                {
                                        WalError("Failed atomic set for WIFI hence rollbacking the changes. ret :%d and i is %d\n",ret,i);

                                        // Rollback data in failure case
                                        for(rev =i-1;rev>=0;rev--)
                                        {
                                                WalPrint("rev value inside for loop is  %d\n",rev);
                                                //skip for wifi rollback
                                                if(indexWifi != rev)
                                                {
                                                        WalPrint("ParamGroup[%d].comp_name : %s\n",rev,ParamGroup[rev].comp_name);
                                                        checkSetstatus = setParamValues(rollbackVal[rev], ParamGroup[rev].comp_name,ParamGroup[rev].dbus_path, ParamGroup[rev].parameterCount, setType, transactionId);	
                                                        WalPrint("checkSetstatus is: %d\n",checkSetstatus);
                                                        if(checkSetstatus != CCSP_SUCCESS)
                                                        {
                                                                WalError("While rollback Failed to do atomic set. checkSetstatus :%d\n",checkSetstatus);
                                                        }
                                                }
                                                else
                                                {
                                                        WalPrint("Skip rollback for WiFi\n");
                                                }	

                                        }
                                        WalPrint("------ End of rollback ------\n");
                                }
                        }

                        for(i=0;i<paramCount;i++)
                        {
                                WAL_FREE(storeGetValue[i]->name);
                                WAL_FREE(storeGetValue[i]->value);
                                WAL_FREE(storeGetValue[i]);
                        }
                }

                free_paramVal_memory(val,compCount);
                free_paramVal_memory(rollbackVal,compCount);
                WAL_FREE(storeGetValue);
        }
        else
        {
                WalError("Failed to get Component details\n");
        }

        WalPrint("------ Free for ParamGroup ------\n");
        free_ParamCompList(ParamGroup, compCount);
        
        *retStatus = mapStatus(ret);

        WalPrint("=============== End of setValues =============\n");
}


void initApplyWiFiSettings()
{
	int err = 0;
	pthread_t applySettingsThreadId;
	WalPrint("============ initApplySettings ==============\n");
	err = pthread_create(&applySettingsThreadId, NULL, applyWiFiSettingsTask, NULL);
	if (err != 0) 
	{
		WalError("Error creating applyWiFiSettings thread :[%s]\n", strerror(err));
	}
	else
	{
		WalPrint("applyWiFiSettings thread created Successfully\n");
	}
}

/*----------------------------------------------------------------------------*/
/*                             Internal Functions                             */
/*----------------------------------------------------------------------------*/

/**
 * @brief getParamValues Returns the parameter Values from stack for GET request
 *
 * @param[in] paramName parameter Name
 * @param[in] paramCount count of paramter nanmes
 * @param[in] CompName Component Name of parameters
 * @param[in] dbusPath Dbus Path of component
 * @param[out] timeSpan timing_values for each component.
 * @param[in] index parameter value Array
 * @param[out] paramArr parameter value Array
 * @param[out] TotalParams Number of parameters returned from stack
 */
 
static int getParamValues(char *parameterNames[], int paramCount, char *CompName, char *dbusPath, money_trace_spans *timeSpan, int paramIndex,int startIndex, param_t ***paramArr,int *TotalParams)
{
    int ret = 0, val_size = 0, cnt=0, retIndex=0, error=0;
    char **parameterNamesLocal = NULL;
    parameterValStruct_t **parameterval = NULL;
    WalPrint(" ------ Start of getParamValues ----\n");
    parameterNamesLocal = (char **) malloc(sizeof(char *) * paramCount);
    memset(parameterNamesLocal,0,(sizeof(char *) * paramCount));

    // Initialize names array with converted index	
    for (cnt = 0; cnt < paramCount; cnt++)
    {
        WalPrint("Before Mapping parameterNames[%d] : %s\n",cnt,parameterNames[cnt]);

        parameterNamesLocal[cnt] = (char *) malloc(sizeof(char) * (strlen(parameterNames[cnt]) + 1));
        strcpy(parameterNamesLocal[cnt],parameterNames[cnt]);

        retIndex=IndexMpa_WEBPAtoCPE(parameterNamesLocal[cnt]);
        if(retIndex == -1)
        {
            if(strstr(parameterNamesLocal[cnt], PARAM_RADIO_OBJECT) != NULL)
            {
                ret = CCSP_ERR_INVALID_RADIO_INDEX;
                WalError("%s has invalid Radio index, Valid indexes are 10000 and 10100. ret = %d\n", parameterNamesLocal[cnt],ret); 
            }
            else
            {
                ret = CCSP_ERR_INVALID_WIFI_INDEX;
                WalError("%s has invalid WiFi index, Valid range is between 10001-10008 and 10101-10108. ret = %d\n",parameterNamesLocal[cnt], ret);
            }
            error = 1;
            break;
        }

        WalPrint("After mapping parameterNamesLocal[%d] : %s\n",cnt,parameterNamesLocal[cnt]);
    }

    if(error != 1)
    {
        WalInfo("CompName = %s, dbusPath : %s, paramCount = %d\n", CompName, dbusPath, paramCount);
        if(strcmp(CompName, RDKB_WEBPA_FULL_COMPONENT_NAME) == 0)
        {
            ret = getWebpaParameterValues(parameterNamesLocal, paramCount, &val_size, &parameterval);
        }
        else
        {
            ret = CcspBaseIf_getParameterValues(bus_handle,CompName,dbusPath,parameterNamesLocal,paramCount, &val_size, &parameterval);
        }
        WalPrint("----- After GPV ret = %d------\n",ret);
        if (ret != CCSP_SUCCESS)
        {
            WalError("Error:Failed to GetValue for parameters ret: %d\n", ret);
        }
        else
        {
            WalPrint("val_size : %d\n",val_size);
            if (val_size > 0)
            {
		if((paramCount == val_size) && (parameterNamesLocal[0][strlen(parameterNamesLocal[0])-1] != '.'))
                {
                    for (cnt = 0; cnt < val_size; cnt++)
                    {
                        (*paramArr)[paramIndex] = (param_t *) malloc(sizeof(param_t));
                        WalPrint("Stack:> success: %s %s %d \n",parameterval[cnt][0].parameterName,parameterval[cnt][0].parameterValue, parameterval[cnt][0].type);
                        IndexMpa_CPEtoWEBPA(&parameterval[cnt][0].parameterName);

                        IndexMpa_CPEtoWEBPA(&parameterval[cnt][0].parameterValue);

                        WalPrint("B4 assignment\n");
                        (*paramArr)[paramIndex][0].name = parameterval[cnt][0].parameterName;
                        (*paramArr)[paramIndex][0].value = parameterval[cnt][0].parameterValue;
                        (*paramArr)[paramIndex][0].type = parameterval[cnt][0].type;
                        WalPrint("success: %s %s %d \n",(*paramArr)[paramIndex][0].name,(*paramArr)[paramIndex][0].value, (*paramArr)[paramIndex][0].type);
                        paramIndex++;
                    }
                }
                else
                {
                    if(startIndex == 0)
                    {
                        (*paramArr)[paramIndex] = (param_t *) malloc(sizeof(param_t)*val_size);
                    }
                    else
                    {
                        (*paramArr)[paramIndex] = (param_t *) realloc((*paramArr)[paramIndex], sizeof(param_t)*(startIndex + val_size));
                    }

                    for (cnt = 0; cnt < val_size; cnt++)
                    {
                        WalPrint("Stack:> success: %s %s %d \n",parameterval[cnt][0].parameterName,parameterval[cnt][0].parameterValue, parameterval[cnt][0].type);
                        IndexMpa_CPEtoWEBPA(&parameterval[cnt][0].parameterName);
                        IndexMpa_CPEtoWEBPA(&parameterval[cnt][0].parameterValue);
                        WalPrint("B4 assignment\n");
                        (*paramArr)[paramIndex][cnt+startIndex].name = parameterval[cnt][0].parameterName;
                        (*paramArr)[paramIndex][cnt+startIndex].value = parameterval[cnt][0].parameterValue;
                        (*paramArr)[paramIndex][cnt+startIndex].type = parameterval[cnt][0].type;
                        WalPrint("success: %s %s %d \n",(*paramArr)[paramIndex][cnt+startIndex].name,(*paramArr)[paramIndex][cnt+startIndex].value, (*paramArr)[paramIndex][cnt+startIndex].type);
                    }
                }
            }
            else if(val_size == 0 && ret == CCSP_SUCCESS)
            {
                WalPrint("No child elements found\n");
            }

            *TotalParams = val_size;
	    for(cnt=0; cnt<val_size; cnt++)
	    {
		WAL_FREE(parameterval[cnt]);
	    }
            WAL_FREE(parameterval);
        }	
    }

    for (cnt = 0; cnt < paramCount; cnt++)
    {
        WAL_FREE(parameterNamesLocal[cnt]);
    }
    WAL_FREE(parameterNamesLocal);
    return ret;
}

/**
 * @brief free_set_param_values_memory to free memory allocated in setParamValues function
 *
 * @param[in] val parameter value Array
 * @param[in] paramCount parameter count
 * @param[in] faultParam fault Param
 */
static void free_set_param_values_memory(parameterValStruct_t* val, int paramCount, char * faultParam)
{
	int cnt1 = 0;
	WAL_FREE(faultParam);

	for (cnt1 = 0; cnt1 < paramCount; cnt1++) 
	{
		WAL_FREE(val[cnt1].parameterName);
	}
	WAL_FREE(val);
}

/**
 * @brief free_paramVal_memory to free memory allocated to param struct 
 *
 * @param[in] val parameter value Array
 * @param[in] paramCount parameter count
 */
static void free_paramVal_memory(param_t ** val, int paramCount)
{
	int cnt1 = 0;
	for (cnt1 = 0; cnt1 < paramCount; cnt1++)
	{
		WAL_FREE(val[cnt1]);
	}
	WAL_FREE(val);
}

/**
 * @brief prepare_parameterValueStruct returns parameter values
 *
 * @param[in] val parameter value Array
 * @param[in] paramVal parameter value Array
 * @param[in] paramName parameter name
 */
 
static int prepare_parameterValueStruct(parameterValStruct_t* val, param_t *paramVal, char *paramName)
{
	val->parameterName = malloc( sizeof(char) * MAX_PARAMETERNAME_LEN);

	if(val->parameterName == NULL)
	{
		return WDMP_FAILURE;
	}
	strcpy(val->parameterName,paramName);

	val->parameterValue = paramVal->value;
		
	switch(paramVal->type)
	{ 
		case 0:
				val->type = ccsp_string;
				break;
		case 1:
				val->type = ccsp_int;
				break;
		case 2:
				val->type = ccsp_unsignedInt;
				break;
		case 3:
				val->type = ccsp_boolean;
				break;
		case 4:
				val->type = ccsp_dateTime;
				break;
		case 5:
				val->type = ccsp_base64;
				break;
		case 6:
				val->type = ccsp_long;
				break;
		case 7:
				val->type = ccsp_unsignedLong;
				break;
		case 8:
				val->type = ccsp_float;
				break;
		case 9:
				val->type = ccsp_double;
				break;
		case 10:
				val->type = ccsp_byte;
				break;
		default:
				val->type = ccsp_none;
				break;
	}
	return WDMP_SUCCESS;
}

/**
 * @brief setParamValues sets the parameter value.
 *
 * @param[in] paramVal List of Parameter name/value pairs.
 * @param[in] CompName Component Name of parameters
 * @param[in] dbusPath Dbus Path of component
 * @param[in] paramCount count of paramter nanmes
 * @param[in] setType Flag to specify the type of set operation.
 */
static int setParamValues(param_t *paramVal, char *CompName, char *dbusPath, int paramCount,const WEBPA_SET_TYPE setType, char *transactionId)
{
        char* faultParam = NULL;
        int ret=0, cnt = 0, retIndex=0;
        char paramName[MAX_PARAMETERNAME_LEN] = { 0 };
        char objectName[MAX_PARAMETERNAME_LEN] = { 0 };
        unsigned int writeID = CCSP_COMPONENT_ID_WebPA;

        WalPrint("------------------ start of setParamValues ----------------\n");
        parameterValStruct_t* val = (parameterValStruct_t*) malloc(sizeof(parameterValStruct_t) * paramCount);
        memset(val,0,(sizeof(parameterValStruct_t) * paramCount));

        for (cnt = 0; cnt < paramCount; cnt++)
        {
                retIndex=0;
                walStrncpy(paramName, paramVal[cnt].name,sizeof(paramName));
                WalPrint("Inside setParamValues paramName is %s \n",paramName);
                retIndex = IndexMpa_WEBPAtoCPE(paramName);
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
			free_set_param_values_memory(val,paramCount,faultParam);
                        return ret;
                }
                WalPrint("B4 prepare_parameterValueStruct\n");
                ret = prepare_parameterValueStruct(&val[cnt], &paramVal[cnt], paramName);
                if(ret)
                {
                        WalError("Preparing parameter value struct is Failed \n");
                        free_set_param_values_memory(val,paramCount,faultParam);
                        return ret;
                }
        }

        writeID = (setType == WEBPA_ATOMIC_SET_XPC)? CCSP_COMPONENT_ID_XPC: CCSP_COMPONENT_ID_WebPA;

        if(!strcmp(CompName,RDKB_WIFI_FULL_COMPONENT_NAME))
        {
                identifyRadioIndexToReset(paramCount,val,&bRestartRadio1,&bRestartRadio2);
                bRadioRestartEn = TRUE;
        }

        if(strcmp(CompName, RDKB_WEBPA_FULL_COMPONENT_NAME) == 0)
        {
            ret = setWebpaParameterValues(val, paramCount,&faultParam);
        }
        else
        {
            ret = CcspBaseIf_setParameterValues(bus_handle, CompName, dbusPath, 0, writeID, val, paramCount, TRUE, &faultParam);
        }

        if(!strcmp(CompName,RDKB_WIFI_FULL_COMPONENT_NAME))
        {
                if(ret == CCSP_SUCCESS) //signal apply settings thread only when set is success
                {
                        if( transactionId!= NULL)
                        {
	                        WalPrint("transactionId :%s \n",transactionId);
	                        walStrncpy(current_transaction_id, transactionId,sizeof(current_transaction_id));
	                        WalPrint("current_transaction_id %s\n",current_transaction_id);
                        }
                        else
                        {
	                        WalError("transaction_id in request is NULL\n");
	                        memset(current_transaction_id,0,sizeof(current_transaction_id));
                        }
                        pthread_cond_signal(&applySetting_cond);
                        WalPrint("condition signalling in setParamValues\n");
                }
        }
        
        if (ret != CCSP_SUCCESS && faultParam)
        {
                WalError("Failed to SetAtomicValue for param  '%s' ret : %d \n", faultParam, ret);
                free_set_param_values_memory(val,paramCount,faultParam);
                return ret;
        }
        
        free_set_param_values_memory(val,paramCount,faultParam);
        WalPrint("------------------ End of setParamValues ----------------\n");
        return ret;
}

/**
 * @brief identifyRadioIndexToReset identifies which radio to restart 
 *
 * @param[in] paramCount count of parameters
 * @param[in] paramVal parameter value Array
 * @param[out] bRestartRadio1
 * @param[out] bRestartRadio2
 */
static void identifyRadioIndexToReset(int paramCount, parameterValStruct_t* val,BOOL *bRestartRadio1,BOOL *bRestartRadio2) 
{
	int x =0 ,index =0, SSID =0,apply_rf =0;
	for (x = 0; x < paramCount; x++)
	{
		WalPrint("val[%d].parameterName : %s\n",x,val[x].parameterName);
		if (!strncmp(val[x].parameterName, "Device.WiFi.Radio.1.", 20))
		{
			*bRestartRadio1 = TRUE;
		}
		else if (!strncmp(val[x].parameterName, "Device.WiFi.Radio.2.", 20))
		{
			*bRestartRadio2 = TRUE;
		}
		else
		{
			if ((!strncmp(val[x].parameterName, "Device.WiFi.SSID.", 17)))
			{
				sscanf(val[x].parameterName, "Device.WiFi.SSID.%d", &index);
				WalPrint("SSID index = %d\n", index);
				SSID = (1 << ((index) - 1));
				apply_rf = (2 - ((index) % 2));
				WalPrint("apply_rf = %d\n", apply_rf);

				if (apply_rf == 1)
				{
					*bRestartRadio1 = TRUE;
				}
				else if (apply_rf == 2)
				{
					*bRestartRadio2 = TRUE;
				}
			}
			else if (!strncmp(val[x].parameterName, "Device.WiFi.AccessPoint.",24))
			{
				sscanf(val[x].parameterName, "Device.WiFi.AccessPoint.%d", &index);
				WalPrint("AccessPoint index = %d\n", index);
				SSID = (1 << ((index) - 1));
				apply_rf = (2 - ((index) % 2));
				WalPrint("apply_rf = %d\n", apply_rf);

				if (apply_rf == 1)
				{
					*bRestartRadio1 = TRUE;
				}
				else if (apply_rf == 2)
				{
					*bRestartRadio2 = TRUE;
				}
			}
		}
	}
}

/**
 * @brief applyWiFiSettingsTask applys settings on WiFi component
 */
static void *applyWiFiSettingsTask()
{
	parameterValStruct_t *RadApplyParam = NULL;
	char* faultParam = NULL;
	unsigned int writeID = CCSP_COMPONENT_ID_WebPA;
	struct timespec start,end,*startPtr,*endPtr;
	startPtr = &start;
	endPtr = &end;
	int nreq = 0,ret=0;
	WalPrint("================= applyWiFiSettings ==========\n");
	
    pthread_detach(pthread_self());
        
	parameterValStruct_t val_set[4] = { 
					{"Device.WiFi.Radio.1.X_CISCO_COM_ApplySettingSSID","1", ccsp_int},
					{"Device.WiFi.Radio.1.X_CISCO_COM_ApplySetting", "true", ccsp_boolean},
					{"Device.WiFi.Radio.2.X_CISCO_COM_ApplySettingSSID","2", ccsp_int},
					{"Device.WiFi.Radio.2.X_CISCO_COM_ApplySetting", "true", ccsp_boolean} };
	
	//Identify the radio and apply settings
	while(1)
	{
		WalPrint("Before cond wait in applyWiFiSettings\n");
		pthread_cond_wait(&applySetting_cond, &applySetting_mutex);
		applySettingsFlag = TRUE;
		WalPrint("applySettingsFlag is set to TRUE\n");
		getCurrentTime(startPtr);
		WalPrint("After cond wait in applyWiFiSettings\n");
		if(bRadioRestartEn)
		{
			bRadioRestartEn = FALSE;
		
			if((bRestartRadio1 == TRUE) && (bRestartRadio2 == TRUE)) 
			{
				WalPrint("Need to restart both the Radios\n");
				RadApplyParam = val_set;
				nreq = 4;
			}

			else if(bRestartRadio1) 
			{
				WalPrint("Need to restart Radio 1\n");
				RadApplyParam = val_set;
				nreq = 2;
			}
			else if(bRestartRadio2) 
			{
				WalPrint("Need to restart Radio 2\n");
				RadApplyParam = &val_set[2];
				nreq = 2;
			}
		
			// Reset radio flags
			bRestartRadio1 = FALSE;
			bRestartRadio2 = FALSE;
		
			WalPrint("nreq : %d writeID : %d\n",nreq,writeID);
			ret = CcspBaseIf_setParameterValues(bus_handle, RDKB_WIFI_FULL_COMPONENT_NAME, RDKB_WIFI_DBUS_PATH, 0, writeID, RadApplyParam, nreq, TRUE,&faultParam);
			WalInfo("After SPV in applyWiFiSettings ret = %d\n",ret);
			if ((ret != CCSP_SUCCESS) && (faultParam != NULL))
			{
				WalError("Failed to Set Apply Settings\n");
				WAL_FREE(faultParam);
			}	
			
			// Send transcation event notify to server
			if(strlen(current_transaction_id) != 0)
			{
				processTransactionNotification(current_transaction_id);
			}

			applySettingsFlag = FALSE;
			WalPrint("applySettingsFlag is set to FALSE\n");
		}

		getCurrentTime(endPtr);
		WalInfo("Elapsed time for apply setting : %ld ms\n", timeValDiff(startPtr, endPtr));
	}
	WalPrint("============ End =============\n");
        return NULL;
}


