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
static int getParamAttributes(char *parameterNames[], int paramCount, char *CompName, char *dbusPath, money_trace_spans *timeSpan, param_t **attr, int index);
static int setParamAttributes(param_t *attArr,int paramCount, money_trace_spans *timeSpan);

extern BOOL applySettingsFlag;
/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/


void getAttributes(const char *paramName[], const unsigned int paramCount, money_trace_spans *timeSpan, param_t **attr, int *retAttrCount, WDMP_STATUS *retStatus)
{
	unsigned int cnt1=0, compCount=0;
        int cnt2=0, ret = -1, index = 0,error = 0, count =0, i= 0;
	char parameterName[MAX_PARAMETERNAME_LEN] = {'\0'};
	ParamCompList *ParamGroup = NULL;
	char **compName = NULL;
	char **dbusPath = NULL;
	
	for(cnt1 = 0; cnt1 < paramCount; cnt1++)
	{
		// Get the matching component index from cache
		walStrncpy(parameterName,paramName[cnt1],sizeof(parameterName));
		// To get list of component name and dbuspath
		ret = getComponentDetails(parameterName,&compName,&dbusPath,&error,&count);
		if(error == 1)
		{
			break;
		}
		WalPrint("parameterName: %s count : %d\n",parameterName,count);
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
		for(cnt1 = 0; cnt1 < compCount; cnt1++)
		{
			WalPrint("********** Parameter group ****************\n");
		  	WalPrint("ParamGroup[%d].comp_name :%s, ParamGroup[%d].dbus_path :%s, ParamGroup[%d].parameterCount :%d\n",cnt1,ParamGroup[cnt1].comp_name, cnt1,ParamGroup[cnt1].dbus_path, cnt1,ParamGroup[cnt1].parameterCount);
		  	
		  	for(cnt2 = 0; cnt2 < ParamGroup[cnt1].parameterCount; cnt2++)
		  	{
		 		WalPrint("ParamGroup[%d].parameterName :%s\n",cnt1,ParamGroup[cnt1].parameterName[cnt2]);
		  	}
			if(!strcmp(ParamGroup[cnt1].comp_name,RDKB_WIFI_FULL_COMPONENT_NAME)&& applySettingsFlag == TRUE) 
			{
				ret = CCSP_ERR_WIFI_BUSY;
				WalError("Wifi busy\n");
				break;
			}
		  	// GET atomic value call
			WalPrint("index %d\n",index);
		  	ret = getParamAttributes(ParamGroup[cnt1].parameterName, ParamGroup[cnt1].parameterCount, ParamGroup[cnt1].comp_name, ParamGroup[cnt1].dbus_path, timeSpan, attr, index);
		  	if(ret != CCSP_SUCCESS)
		  	{
				WalError("Get attributes call failed for ParamGroup[%d]->comp_name :%s ret: %d\n",cnt1,ParamGroup[cnt1].comp_name,ret);
				break;
		  	}
			index = index + ParamGroup[cnt1].parameterCount;
		}
	}
	
	*retStatus = mapStatus(ret);
	*retAttrCount = paramCount;	
	
	free_ParamCompList(ParamGroup, compCount);
}

void setAttributes(param_t *attArr, const unsigned int paramCount, money_trace_spans *timeSpan, WDMP_STATUS *retStatus)
{
	int ret = 0;
	ret = setParamAttributes(attArr,paramCount, timeSpan);
	*retStatus = mapStatus(ret);
}

/*----------------------------------------------------------------------------*/
/*                               Internal functions                              */
/*----------------------------------------------------------------------------*/

/**
 * @brief getParamAttributes Returns the parameter Attributes from stack for GET-ATTRIBUTES request
 *
 * @param[in] parameterNames parameter Name
 * @param[in] paramCount Number of parameters
 * @param[in] CompName Component Name of parameters
 * @param[in] dbusPath Dbus Path of component
 * @param[out] timeSpan timing_values for each component.
 * @param[out] attr parameter attribute Array
 * @param[in] index parameter attribute Array index
 */
static int getParamAttributes(char *parameterNames[], int paramCount, char *CompName, char *dbusPath, money_trace_spans *timeSpan, param_t **attr, int index)
{
	int ret = 0, sizeAttrArr = 0, cnt=0, retIndex=0, error=0;
	char **parameterNamesLocal = NULL;
	parameterAttributeStruct_t** ppAttrArray = NULL;
	WalPrint(" ------ Start of getParamAttributes ----\n");
	parameterNamesLocal = (char **) malloc(sizeof(char *) * paramCount);
	memset(parameterNamesLocal,0,(sizeof(char *) * paramCount));

	// Initialize names array with converted index	
	for (cnt = 0; cnt < paramCount; cnt++)
	{
		WalPrint("Before mapping parameterNames[%d] : %s\n",cnt,parameterNames[cnt]);
	
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
		ret = CcspBaseIf_getParameterAttributes(bus_handle,CompName,dbusPath,parameterNamesLocal,paramCount, &sizeAttrArr, &ppAttrArray);
		WalPrint("----- After GPA ret = %d------\n",ret);
		if (ret != CCSP_SUCCESS)
		{
			WalError("Error:Failed to GetAttributes for parameters ret: %d\n", ret);
		}
		else
		{
			WalPrint("sizeAttrArr : %d\n",sizeAttrArr);
			for (cnt = 0; cnt < sizeAttrArr; cnt++)
			{
			        
		                WalPrint("Stack:> success: %s %d \n",ppAttrArray[cnt]->parameterName,ppAttrArray[cnt]->notification);
		                
				(*attr)[index].name = (char *) malloc(sizeof(char) * MAX_PARAMETERNAME_LEN);
				(*attr)[index].value = (char *) malloc(sizeof(char) * MAX_PARAMETERVALUE_LEN);

				IndexMpa_CPEtoWEBPA(&ppAttrArray[cnt]->parameterName);
				WalPrint("ppAttrArray[cnt]->parameterName : %s\n",ppAttrArray[cnt]->parameterName);
				walStrncpy((*attr)[index].name, ppAttrArray[cnt]->parameterName,MAX_PARAMETERNAME_LEN);
				sprintf((*attr)[index].value, "%d", ppAttrArray[cnt]->notification);
				(*attr)[index].type = WDMP_INT;
				WalPrint("success: %s %s %d \n",(*attr)[index].name,(*attr)[index].value,(*attr)[index].type);
				index++;
			}

			free_parameterAttributeStruct_t(bus_handle, sizeAttrArr, ppAttrArray);
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
 * @brief setParamAttributes Returns the status of parameter from stack for SET-ATTRIBUTES request
 *
 * @param[in] attArr parameter attributes Array
 * @param[in] paramCount Number of parameters
 * @param[out] timeSpan timing_values for each component.
 */
static int setParamAttributes(param_t *attArr,int paramCount, money_trace_spans *timeSpan)
{
	int ret = 0, cnt = 0, notificationType = 0, error = 0,retIndex = 0, count = 0, i = 0, count1;
	char paramName[MAX_PARAMETERNAME_LEN] = { 0 };
	char **compName = NULL;
	char **tempCompName = NULL;
	char **dbusPath = NULL;
	char **tempDbusPath = NULL;
	
	parameterAttributeStruct_t *attriStruct =(parameterAttributeStruct_t*) malloc(sizeof(parameterAttributeStruct_t) * paramCount);
	memset(attriStruct,0,(sizeof(parameterAttributeStruct_t) * paramCount));
	
	WalPrint("==========setParamAttributes ========\n ");
	
	walStrncpy(paramName,attArr[0].name,sizeof(paramName));
	// To get list of component name and dbuspath
	ret = getComponentDetails(paramName,&compName,&dbusPath,&error,&count);
	if(error == 1)
	{
		WalError("Component name is not supported ret : %d\n", ret);
		WAL_FREE(attriStruct);
		return ret;
	}
	WalPrint("paramName: %s count: %d\n",paramName,count);
	for(i = 0; i < count; i++)
	{
		WalPrint("compName[%d] : %s, dbusPath[%d] : %s\n", i,compName[i],i, dbusPath[i]);
	}
	
	for (cnt = 0; cnt < paramCount; cnt++) 
	{
		retIndex = 0;
		walStrncpy(paramName,attArr[cnt].name,sizeof(paramName));
		// To get list of component name and dbuspath	
		ret = getComponentDetails(paramName,&tempCompName,&tempDbusPath,&error,&count1);
		if(error == 1)
		{
			WalError("Component name is not supported ret : %d\n", ret);
                        free_componentDetails(compName, dbusPath, count);
			WAL_FREE(attriStruct);
			return ret;
		}			
		WalPrint("paramName: %s count: %d\n",paramName,count);
		for(i = 0; i < count1; i++)
		{
			WalPrint("tempCompName[%d] : %s, tempDbusPath[%d] : %s\n", i,tempCompName[i],i, tempDbusPath[i]);
		}
		if (strcmp(compName[0], tempCompName[0]) != 0)
		{
			WalError("Error: Parameters does not belong to the same component\n");
                        free_componentDetails(compName, dbusPath, count);
                        free_componentDetails(tempCompName,tempDbusPath,count1);
			WAL_FREE(attriStruct);
			return CCSP_FAILURE;
		}		
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
                        free_componentDetails(compName, dbusPath, count);
                        free_componentDetails(tempCompName,tempDbusPath,count1);
			WAL_FREE(attriStruct);	
			return ret;
		}

		attriStruct[cnt].parameterName = NULL;
		attriStruct[cnt].notificationChanged = 1;
		attriStruct[cnt].accessControlChanged = 0;	
		notificationType = atoi(attArr[cnt].value);
		WalPrint("notificationType : %d\n",notificationType);
		if(notificationType == 1)
		{
#ifndef USE_NOTIFY_COMPONENT
			ret = CcspBaseIf_Register_Event(bus_handle, compName[0], "parameterValueChangeSignal");
			if (CCSP_SUCCESS != ret)
			{
				WalError("WebPa: CcspBaseIf_Register_Event failed!!!\n");
			}
			CcspBaseIf_SetCallback2(bus_handle, "parameterValueChangeSignal", ccspWebPaValueChangedCB, NULL);
#endif
		}
		attriStruct[cnt].parameterName = malloc( sizeof(char) * MAX_PARAMETERNAME_LEN);
		walStrncpy(attriStruct[cnt].parameterName,paramName,MAX_PARAMETERNAME_LEN);
		WalPrint("attriStruct[%d].parameterName : %s\n",cnt,attriStruct[cnt].parameterName);
		
		attriStruct[cnt].notification = notificationType;
		WalPrint("attriStruct[%d].notification : %d\n",cnt,attriStruct[cnt].notification );	
		free_componentDetails(tempCompName,tempDbusPath,count1);
	}
	
	if(error != 1)
	{
		ret = CcspBaseIf_setParameterAttributes(bus_handle,compName[0], dbusPath[0], 0, attriStruct, paramCount);
		WalPrint("=== After SPA == ret = %d\n",ret);
		if (CCSP_SUCCESS != ret)
		{
			WalError("Failed to set attributes for SetParamAttr ret : %d \n", ret);
		}
		for (cnt = 0; cnt < paramCount; cnt++) 
		{
			WAL_FREE(attriStruct[cnt].parameterName);
		}
	}
	free_componentDetails(compName,dbusPath,count);
	WAL_FREE(attriStruct);
	return ret;
}

