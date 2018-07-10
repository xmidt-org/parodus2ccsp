/**
 * @file webpa_adapter.c
 *
 * @description This file describes the Webpa Abstraction Layer
 *
 * Copyright (c) 2015  Comcast
 */

#include "webpa_notification.h"
#include "webpa_internal.h"

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/

#define WEBPA_DEVICE_REBOOT_PARAM          "Device.X_CISCO_COM_DeviceControl.RebootDevice"
#define WEBPA_DEVICE_REBOOT_VALUE          "Device"
#define WEBPA_CLIENT					"webpa_client"

/*----------------------------------------------------------------------------*/
/*                               Data Structures                              */
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/
static WDMP_STATUS validate_cmc_and_cid(test_set_req_t *testSetReq, char *dbCMC, char *dbCID);
static WDMP_STATUS set_cmc_and_cid(char *dbCMC, char *cid, int isNew);
static WDMP_STATUS validate_parameter(param_t *param, int paramCount, REQ_TYPE type);
static WDMP_STATUS validate_table_object(table_req_t *tableObj);
static void setRebootReason(param_t param, WEBPA_SET_TYPE setType);
static void add_wildcard_timespan_to_response(money_trace_spans *timeSpan, money_trace_spans *wildcardSpan);
static void add_total_webpa_client_time(uint64_t startTime,uint32_t duration,money_trace_spans *timeSpan);

extern ANSC_HANDLE bus_handle;
/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/

void processRequest(char *reqPayload,char *transactionId, bool include_spans, char **resPayload, money_trace_spans *timeSpan)
{
        req_struct *reqObj = NULL;
        res_struct *resObj = NULL;
        char *payload = NULL;
        WDMP_STATUS ret = WDMP_FAILURE, setRet = WDMP_FAILURE;
        int paramCount = 0, i = 0, wildcardParamCount = 0,nonWildcardParamCount = 0, retCount=0, index = 0, error = 0;
        const char *getParamList[MAX_PARAMETERNAME_LEN];
        const char *wildcardGetParamList[MAX_PARAMETERNAME_LEN];
        WDMP_STATUS setCidStatus = WDMP_SUCCESS, setCmcStatus = WDMP_SUCCESS;
        const char *wildcardList[1];
        char *param = NULL;
        money_trace_spans *wildcardSpan = NULL;
        uint32_t duration = 0;
        uint64_t startTime = 0, endTime = 0;
        struct timespec start,end;
	char *dbCID = NULL;
	char *dbCMC = NULL;
	char newCMC[32]={'\0'};
	
        WalPrint("************** processRequest *****************\n");
        startTime = getCurrentTimeInMicroSeconds(&start);
        WalPrint("WEBPA start_time : %llu\n",startTime);
        wdmp_parse_request(reqPayload,&reqObj);
        WalInfo("transactionId in request: %s\n",transactionId);
        
        if(reqObj != NULL)
        {
                WalPrint("Request:> Type : %d\n",reqObj->reqType);
                
                resObj = (res_struct *) malloc(sizeof(res_struct));
                memset(resObj, 0, sizeof(res_struct));
                
                resObj->reqType = reqObj->reqType;
                WalPrint("Response:> type = %d\n", resObj->reqType);
                if(include_spans)
                {
                    resObj->timeSpan = (money_trace_spans *) malloc(sizeof(money_trace_spans));
                    memset(resObj->timeSpan,0,(sizeof(money_trace_spans)));
                }
                else
                {
                    WalPrint("include_spans is false\n");
                }
                switch( reqObj->reqType ) 
                {
                        case GET:
                        {
                                WalPrint("Request:> ParamCount = %zu\n",reqObj->u.getReq->paramCnt);
                                resObj->paramCnt = reqObj->u.getReq->paramCnt;
                                WalPrint("Response:> paramCnt = %zu\n", resObj->paramCnt);
                                resObj->retStatus = (WDMP_STATUS *) malloc(sizeof(WDMP_STATUS)*resObj->paramCnt);
                                paramCount = (int)reqObj->u.getReq->paramCnt;
                                
                                for (i = 0; i < paramCount; i++) 
                                {
                                        WalPrint("Request:> paramNames[%d] = %s\n",i,reqObj->u.getReq->paramNames[i]);
                                        param = reqObj->u.getReq->paramNames[i];
                                        if(strlen(param) >= MAX_PARAMETERNAME_LEN)
                                        {
                                                *resObj->retStatus = WDMP_ERR_INVALID_PARAM;
                                                error = 1;
                                                break;
                                        }

                                        if(param[(strlen(param)-1)] == '.')
                                        {
                                                wildcardGetParamList[wildcardParamCount] = param;
                                                wildcardParamCount++; 
                                        }
                                        else
                                        {
                                                getParamList[nonWildcardParamCount] = param;	
                                                nonWildcardParamCount++;
                                        }
                                }
                                
                                if(error != 1)
                                {
                                        resObj->u.getRes = (get_res_t *) malloc(sizeof(get_res_t));
                                        memset(resObj->u.getRes, 0, sizeof(get_res_t));
                
                                        resObj->u.getRes->paramCnt = reqObj->u.getReq->paramCnt;
                                        resObj->u.getRes->paramNames = (char **) malloc(sizeof(char *) * resObj->u.getRes->paramCnt);
                                        resObj->u.getRes->retParamCnt = (size_t *) malloc(sizeof(size_t)*paramCount);
                                        
                                        resObj->u.getRes->params = (param_t **) malloc(sizeof(param_t*)*paramCount);
                                        memset(resObj->u.getRes->params, 0, sizeof(param_t*)*paramCount);

                                    if (0 < nonWildcardParamCount) { 
                                        getValues(getParamList, nonWildcardParamCount, index, resObj->timeSpan, &resObj->u.getRes->params, &retCount, &ret);
                                        WalPrint("Non-Wildcard retCount : %d ret : %d\n",retCount, ret);
                                        for(i = 0; i < nonWildcardParamCount; i++)
                                        {       
                                                resObj->u.getRes->paramNames[i] = getParamList[i];
                                                WalPrint("Response:> paramNames[%d] = %s\n",i,resObj->u.getRes->paramNames[i]);
                                                resObj->u.getRes->retParamCnt[i] = 1;
                                                WalPrint("Response:> retParamCnt[%d] = %zu\n",i,resObj->u.getRes->retParamCnt[i]);
                                                resObj->retStatus[i] = ret;
                                                WalPrint("Response:> retStatus[%d] = %d\n",i,resObj->retStatus[i]);
                                        }
					if(ret != WDMP_SUCCESS)
                                        {
                                            WalPrint("Non-wildcard get failed. Hence returning...\n");
                                            break;
                                        }
                                     } else {
                                        WalPrint("Non-Wildcard count is zero!\n");
                                       }   

                                        if(wildcardParamCount > 0)
                                        {
                                                wildcardSpan=(money_trace_spans *) malloc(sizeof(money_trace_spans));
                                                memset(wildcardSpan,0,(sizeof(money_trace_spans)));
                                                index = index+nonWildcardParamCount;
                                                for(i = 0; i < wildcardParamCount; i++)
                                                {
                                                        wildcardList[0] = wildcardGetParamList[i];
                                                        resObj->u.getRes->paramNames[index] = wildcardGetParamList[i];
                                                        WalPrint("Response:> paramNames[%d] = %s\n",index,resObj->u.getRes->paramNames[index]);
                                                        getValues(wildcardList, 1, index, wildcardSpan, &resObj->u.getRes->params, &retCount, &ret);
                                                        WalPrint("Wildcard retCount : %d ret: %d\n",retCount, ret);
                                                        resObj->u.getRes->retParamCnt[index] = retCount;
                                                        WalPrint("Response:> retParamCnt[%d] = %zu\n",index,resObj->u.getRes->retParamCnt[index]);
                                                        resObj->retStatus[index] = ret;
                                                        WalPrint("Response:> retStatus[%d] = %d\n",index,resObj->retStatus[index]);
                                                        if(include_spans)
                                                        {
                                                            add_wildcard_timespan_to_response(resObj->timeSpan, wildcardSpan);
                                                        }
                                                        index++;
                                                }
                                        }
                                }
                        }
                        break;
                        
                        case GET_ATTRIBUTES:
                        {
                                WalPrint("Request:> ParamCount = %zu\n",reqObj->u.getReq->paramCnt);
                                resObj->paramCnt = reqObj->u.getReq->paramCnt;
                                WalPrint("Response:> paramCnt = %zu\n", resObj->paramCnt);
                                resObj->retStatus = (WDMP_STATUS *) malloc(sizeof(WDMP_STATUS)*resObj->paramCnt);
                                paramCount = (int)reqObj->u.getReq->paramCnt;
                                
                                for (i = 0; i < paramCount; i++) 
                                {
                                        WalPrint("Request:> paramNames[%d] = %s\n",i,reqObj->u.getReq->paramNames[i]);
                                        if(strlen(reqObj->u.getReq->paramNames[i]) >= MAX_PARAMETERNAME_LEN)
                                        {
                                                *resObj->retStatus = WDMP_ERR_INVALID_PARAM;
                                                error = 1;
                                                break;
                                        }

                                        if(reqObj->u.getReq->paramNames[i][(strlen(reqObj->u.getReq->paramNames[i])-1)] == '.')
                                        {
                                                *resObj->retStatus = WDMP_ERR_WILDCARD_NOT_SUPPORTED;
                                                error = 1;
                                                break;
                                        }
                                }
                                
                                if(error != 1)
                                {
                                        resObj->u.paramRes = (param_res_t *) malloc(sizeof(param_res_t));
                                        memset(resObj->u.paramRes, 0, sizeof(param_res_t));
                                        
                                        resObj->u.paramRes->params = (param_t *) malloc(sizeof(param_t)*paramCount);
                                        memset(resObj->u.paramRes->params, 0, sizeof(param_t)*paramCount);
                                        
                                        getAttributes(reqObj->u.getReq->paramNames, paramCount, resObj->timeSpan, &resObj->u.paramRes->params, &retCount, &ret);
                                        WalPrint("retCount : %d ret : %d\n",retCount, ret);
                                        
                                        for (i = 0; i < paramCount; i++) 
                                        {
                                                WalPrint("Response:> params[%d].name = %s\n",i,resObj->u.paramRes->params[i].name);
                                                WalPrint("Response:> params[%d].value = %s\n",i,resObj->u.paramRes->params[i].value);
                                                WalPrint("Response:> params[%d].type = %d\n",i,resObj->u.paramRes->params[i].type);
                                                
                                                resObj->retStatus[i] = ret;
                                                WalPrint("Response:> retStatus[%d] = %d\n",i,resObj->retStatus[i]);
                                        }
                                }
                        }
                        break;
                        
                        case SET:
                        case SET_ATTRIBUTES:
                        {
                                WalPrint("Request:> ParamCount = %zu\n",reqObj->u.setReq->paramCnt);
                                resObj->paramCnt = reqObj->u.setReq->paramCnt;
                                WalPrint("Response:> paramCnt = %zu\n", resObj->paramCnt);
                                resObj->retStatus = (WDMP_STATUS *) malloc(sizeof(WDMP_STATUS)*resObj->paramCnt);
                                paramCount = (int)reqObj->u.setReq->paramCnt;
                                resObj->u.paramRes = (param_res_t *) malloc(sizeof(param_res_t));
                                memset(resObj->u.paramRes, 0, sizeof(param_res_t));
                                
                                for (i = 0; i < paramCount; i++) 
                                {
                                        WalPrint("Request:> param[%d].name = %s\n",i,reqObj->u.setReq->param[i].name);
                                        WalPrint("Request:> param[%d].value = %s\n",i,reqObj->u.setReq->param[i].value);
                                        WalPrint("Request:> param[%d].type = %d\n",i,reqObj->u.setReq->param[i].type);
                                        setRebootReason(reqObj->u.setReq->param[i], WEBPA_SET);
                                }
                                
                                ret = validate_parameter(reqObj->u.setReq->param, paramCount, reqObj->reqType);
                                WalPrint("ret : %d\n",ret);
                                if(ret == WDMP_SUCCESS)
                                {
                                        if(reqObj->reqType == SET)
                                        {
                                                setValues(reqObj->u.setReq->param, paramCount, WEBPA_SET, transactionId, resObj->timeSpan, &ret);
                                        }
                                        else
                                        {
                                                setAttributes(reqObj->u.setReq->param, paramCount, resObj->timeSpan, &ret);
                                        }
                                        
                                        resObj->u.paramRes->params = (param_t *) malloc(sizeof(param_t)*paramCount);
                                        memset(resObj->u.paramRes->params, 0, sizeof(param_t)*paramCount);
                                        
                                        for (i = 0; i < paramCount; i++) 
                                        {
                                                resObj->u.paramRes->params[i].name = (char *) malloc(sizeof(char) * MAX_PARAMETERNAME_LEN);
                                                strcpy(resObj->u.paramRes->params[i].name, reqObj->u.setReq->param[i].name);
                                                WalPrint("Response:> params[%d].name = %s\n",i,resObj->u.paramRes->params[i].name);
                                                resObj->u.paramRes->params[i].value = NULL;
                                                resObj->u.paramRes->params[i].type = 0;
                                                
                                        }
                                        
                                        WalPrint("ret : %d\n",ret);
                                        for (i = 0; i < paramCount; i++) 
                                        {
                                                resObj->retStatus[i] = ret;
                                                WalPrint("Response:> retStatus[%d] = %d\n",i,resObj->retStatus[i]);
                                        }
                                }
                                else
                                {
                                        resObj->retStatus[0] = ret;
                                        WalPrint("Response:> resObj->retStatus[0] = %d\n",resObj->retStatus[0]);
                                }
                                
                        }
                        break;
                        
                        case TEST_AND_SET:
                        {
                                WalPrint("Request:> ParamCount = %zu\n",reqObj->u.testSetReq->paramCnt);
                                resObj->paramCnt = reqObj->u.testSetReq->paramCnt;
                                WalPrint("Response:> paramCnt = %zu\n", resObj->paramCnt);
                                resObj->retStatus = (WDMP_STATUS *) malloc(sizeof(WDMP_STATUS));
                                paramCount = (int)reqObj->u.testSetReq->paramCnt;
                                resObj->u.paramRes = (param_res_t *) malloc(sizeof(param_res_t));
                                memset(resObj->u.paramRes, 0, sizeof(param_res_t));
                                resObj->u.paramRes->params = NULL;
                                
                                WalInfo("Request:> newCid: %s oldCid: %s syncCmc: %s\n",reqObj->u.testSetReq->newCid, reqObj->u.testSetReq->oldCid, reqObj->u.testSetReq->syncCmc);
                                // Get CMC from device database
	                        dbCMC = getParameterValue(PARAM_CMC);
	                        WalInfo("dbCMC : %s\n",dbCMC);
	                        // Get CID from device database
	                        dbCID = getParameterValue(PARAM_CID);
	                        WalInfo("dbCID : %s\n",dbCID);
                                snprintf(newCMC, sizeof(newCMC),"%d", CHANGED_BY_XPC);
                                WalInfo("newCMC : %s\n",newCMC);
                                
                                for (i = 0; i < paramCount; i++) 
                                {
                                        WalPrint("Request:> param[%d].name = %s\n",i,reqObj->u.setReq->param[i].name);
                                        WalPrint("Request:> param[%d].value = %s\n",i,reqObj->u.setReq->param[i].value);
                                        WalPrint("Request:> param[%d].type = %d\n",i,reqObj->u.setReq->param[i].type);
                                        setRebootReason(reqObj->u.setReq->param[i], WEBPA_ATOMIC_SET_XPC);
                                }
                                
                                if(dbCMC != NULL && dbCID != NULL)
	                        {
	                                ret = validate_cmc_and_cid(reqObj->u.testSetReq, dbCMC, dbCID);
	                                WalPrint("ret : %d\n",ret);
	                                if(ret == WDMP_SUCCESS)
	                                {
	                                        ret = set_cmc_and_cid(dbCMC, reqObj->u.testSetReq->newCid, 1);
	                                        WalPrint("ret : %d\n",ret);
	                                        if(ret == WDMP_SUCCESS && paramCount > 0)
	                                        {
                                                        ret = validate_parameter(reqObj->u.testSetReq->param, paramCount, reqObj->reqType);
                                                        WalPrint("ret : %d\n",ret);
                                                        if(ret == WDMP_SUCCESS)
	                                                {
	                                                        setValues(reqObj->u.setReq->param, paramCount, WEBPA_ATOMIC_SET_XPC, transactionId, resObj->timeSpan, &ret);
	                                                        WalPrint("SPV ret : %d\n",ret);
	                                                        if(ret == WDMP_SUCCESS)
	                                                        {
	                                                             WalInfo("Atomic set is success\n");
	                                                        }
	                                                }
	                                                
	                                                if(ret != WDMP_SUCCESS)
                                                        {
                                                                WalPrint("Atomic set is failed. Hence reverting device CID ...\n");
                                                                setRet = set_cmc_and_cid(dbCMC, reqObj->u.testSetReq->oldCid, 0);
                                                                WalPrint("setRet : %d\n",setRet);
                                                                if(setRet == WDMP_SUCCESS)
                                                                {
                                                                        WalInfo("Reverted device CID to %s\n", reqObj->u.testSetReq->oldCid);
                                                                }
                                                                else
                                                                {
                                                                        WalError("Failed to revert CID\n");
                                                                }
                                                        }
	                                        }
                                        }
                                        
                                }
                                else
                                {
                                        WalError("Failed to Get CMC, CID value\n");
                                }   
                                
                                WalPrint("ret : %d\n",ret);
                                resObj->u.paramRes->syncCMC = (char *) malloc(sizeof(char) * MAX_PARAMETERNAME_LEN);
                                resObj->u.paramRes->syncCID = (char *) malloc(sizeof(char) * MAX_PARAMETERNAME_LEN);
        
                                if(ret == WDMP_SUCCESS)
                                {
                                        strcpy(resObj->u.paramRes->syncCMC, newCMC);
                                        strcpy(resObj->u.paramRes->syncCID, reqObj->u.testSetReq->newCid);
                                }
                                else
                                {
                                        strcpy(resObj->u.paramRes->syncCMC, dbCMC);
                                        strcpy(resObj->u.paramRes->syncCID, dbCID);
                                }
                                
                                WalPrint("Response:> CMC = %s\n",resObj->u.paramRes->syncCMC);
                                WalPrint("Response:> CID = %s\n",resObj->u.paramRes->syncCID);
                                resObj->retStatus[0] = ret;
                                WalPrint("Response:> retStatus = %d\n",resObj->retStatus[0]);
                                WAL_FREE(dbCMC);
                                WAL_FREE(dbCID);
                        }
                        break;
                        
                        case REPLACE_ROWS:
                        {
                                WalPrint("Request:> ParamCount = %zu\n",reqObj->u.tableReq->rowCnt);
                                resObj->paramCnt = reqObj->u.tableReq->rowCnt;
                                WalPrint("Response:> paramCnt = %zu\n", resObj->paramCnt);
                                resObj->retStatus = (WDMP_STATUS *) malloc(sizeof(WDMP_STATUS)*resObj->paramCnt);
                                WalPrint("Request:> Object Name = %s\n",reqObj->u.tableReq->objectName);

                                ret = validate_table_object(reqObj->u.tableReq);
                                if(ret == WDMP_SUCCESS)
                                {
                                        replaceTable(reqObj->u.tableReq->objectName,reqObj->u.tableReq->rows,reqObj->u.tableReq->rowCnt,&ret);
                                }
                                else
                                {
                                        WalError("Table object validations failed\n");
                                }
                                WalPrint("Response:> ret = %d\n",ret);
                                *resObj->retStatus = ret;
                                WalPrint("Response:> retStatus = %d\n", *resObj->retStatus);
                        }
                        break;
                        
                        case DELETE_ROW:
                        {
                                WalPrint("Request:> ParamCount = %zu\n",reqObj->u.tableReq->rowCnt);
                                resObj->paramCnt = reqObj->u.tableReq->rowCnt;
                                WalPrint("Response:> paramCnt = %zu\n", resObj->paramCnt);
                                resObj->retStatus = (WDMP_STATUS *) malloc(sizeof(WDMP_STATUS)*resObj->paramCnt);
                                WalPrint("Request:> Object Name = %s\n",reqObj->u.tableReq->objectName);

                                ret = validate_table_object(reqObj->u.tableReq);
                                if(ret == WDMP_SUCCESS)
                                {
                                        deleteRowTable(reqObj->u.tableReq->objectName,&ret);
                                }
                                else
                                {
                                        WalError("Table object validations failed\n");
                                }
                                WalPrint("Response:> ret = %d\n",ret);
                                *resObj->retStatus = ret;
                                WalPrint("Response:> retStatus = %d\n", *resObj->retStatus);
                        }
                        break;
                        
                        case ADD_ROWS:
                        {
                                WalPrint("Request:> ParamCount = %zu\n",reqObj->u.tableReq->rowCnt);
                                resObj->paramCnt = reqObj->u.tableReq->rowCnt;
                                WalPrint("Response:> paramCnt = %zu\n", resObj->paramCnt);
                                WalPrint("Request:> Object Name = %s\n",reqObj->u.tableReq->objectName);
                                resObj->retStatus = (WDMP_STATUS *) malloc(sizeof(WDMP_STATUS)*resObj->paramCnt);

                                ret = validate_table_object(reqObj->u.tableReq);
                                if(ret == WDMP_SUCCESS)
                                {
                                        resObj->u.tableRes = (table_res_t *) malloc(sizeof(table_res_t));
                                        memset(resObj->u.tableRes, 0, sizeof(table_res_t));

                                        if(reqObj->u.tableReq->rowCnt > 0)
                                        {
                                                resObj->u.tableRes->newObj = (char *) malloc(sizeof(char) * MAX_PARAMETERNAME_LEN);
                                                addRowTable(reqObj->u.tableReq->objectName, reqObj->u.tableReq->rows,&resObj->u.tableRes->newObj, &ret);
                                        }

                                        if(resObj->u.tableRes->newObj == NULL)
                                        {
                                                WAL_FREE(resObj->u.tableRes);
                                        }
                                        else
                                        {
                                                WalPrint("Response:> newObj = %s\n",resObj->u.tableRes->newObj);
                                        }
                                }
                                else
                                {
                                        WalError("Table object validations failed\n");
                                }
                                WalPrint("Response:> ret = %d\n",ret);
                                *resObj->retStatus = ret;
                                WalPrint("Response:> retStatus = %d\n", *resObj->retStatus);
                        }
                        break;
                }
        }
	else
	{
		WalError("Command is NULL\n");
	}

        endTime = getCurrentTimeInMicroSeconds(&end);
        duration = endTime - startTime;
        WalPrint("WEBPA duration : %lu\n", duration);
        if(include_spans)
        {
            add_total_webpa_client_time(startTime, duration, resObj->timeSpan);
            *timeSpan = *resObj->timeSpan;
            (*timeSpan).count = resObj->timeSpan->count;
            //*(struct money_trace_spans*)timeSpan.count = resObj->timeSpan->count;
            (*timeSpan).spans = ((money_trace_span *) malloc(sizeof(money_trace_span)* resObj->timeSpan->count));
            memset((*timeSpan).spans,0,(sizeof(money_trace_span)* resObj->timeSpan->count));
            for(i =0 ; i<resObj->timeSpan->count; i++)
            {
                WalPrint("name[%d] : %s \t start[%d]: %lu duration[%d] : %d\n",i,resObj->timeSpan->spans[i].name, i, resObj->timeSpan->spans[i].start, i, resObj->timeSpan->spans[i].duration);
                (*timeSpan).spans[i].name = strdup(resObj->timeSpan->spans[i].name);
                (*timeSpan).spans[i].start = resObj->timeSpan->spans[i].start;
                (*timeSpan).spans[i].duration = resObj->timeSpan->spans[i].duration;
            }
        }
        wdmp_form_response(resObj,&payload);
        WalPrint("payload : %s\n",payload);
        *resPayload = payload;
        
        WalPrint("Response:> Payload = %s\n", *resPayload);
        
        if(NULL != reqObj)
        {
                wdmp_free_req_struct(reqObj);
        }
        if(NULL != resObj)
        {
                wdmp_free_res_struct(resObj);
        }
        WalPrint("************** processRequest *****************\n");
}

/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/

/**
 * @brief validate_cmc_and_cid validates cmc and cid values for TEST-AND-SET
 *
 * @param[in] testSetReq input test-and-set request
 * @param[in] dbCMC input cmc
 * @param[in] dbCID input cid
 */
static WDMP_STATUS validate_cmc_and_cid(test_set_req_t *testSetReq, char *dbCMC, char *dbCID)
{
	WalPrint("------------ validate_cmc_and_cid ----------\n");
        if(testSetReq->syncCmc != NULL)
        {
                if(strcmp(testSetReq->syncCmc, dbCMC) != 0) // Error WEBPA_STATUS CMC_TEST_FAILED
                {
                        WalError("CMC check failed in Test And Set request as CMC (%s) did not match device CMC (%s) \n",testSetReq->syncCmc,dbCMC);
                        return WDMP_ERR_CMC_TEST_FAILED;
                }
        }
        
        if (testSetReq->newCid == NULL)
        {
                WalError("New-Cid input parameter is not present in Test And Set request\n");
                return WDMP_ERR_NEW_CID_IS_MISSING;
        }
        else
        {
                if (testSetReq->oldCid != NULL && strcmp(testSetReq->oldCid, dbCID) != 0) 
                {
                        WalError("Test and Set Failed as old CID (%s) didn't match device CID (%s)\n", testSetReq->oldCid, dbCID);
                        return WDMP_ERR_CID_TEST_FAILED;
                }
        }
        
        return WDMP_SUCCESS;
}

/**
 * @brief set_cmc_and_cid sets cmc and cid values
 *
 * @param[in] dbCMC input cmc
 * @param[in] cid input cid
 * @param[in] isNew flag to identify the data
 */
static WDMP_STATUS set_cmc_and_cid(char *dbCMC, char *cid, int isNew)
{
        char newCMC[32]={'\0'};
        WalPrint("------------ set_cmc_and_cid ----------\n");
        WDMP_STATUS setCmcStatus = WDMP_SUCCESS, setCidStatus = WDMP_SUCCESS;
        snprintf(newCMC, sizeof(newCMC),"%d", CHANGED_BY_XPC);
        WalPrint("newCMC : %s\n",newCMC);
        setCidStatus = setParameterValue(PARAM_CID, cid, WDMP_STRING);
        if(strcmp(dbCMC, newCMC) != 0)
        {
                if(isNew)
                {
                        setCmcStatus = setParameterValue(PARAM_CMC, newCMC,WDMP_UINT);
                }
                else
                {
                        setCmcStatus = setParameterValue(PARAM_CMC, dbCMC,WDMP_UINT);
                }
        }
        
        if((setCidStatus != WDMP_SUCCESS) || (setCmcStatus != WDMP_SUCCESS))
        {
                WalError("Error setting CID/CMC\n");
                return WDMP_ERR_SETTING_CMC_OR_CID;
        }
        
        return WDMP_SUCCESS;
}

/**
 * @brief validate_parameter validates parameter values
 *
 * @param[in] param arry if parameters
 * @param[in] paramCount input cid
 */
static WDMP_STATUS validate_parameter(param_t *param, int paramCount, REQ_TYPE type)
{
        int i = 0;
        WalPrint("------------ validate_parameter ----------\n");
        for (i = 0; i < paramCount; i++) 
        {
                if(param[i].name == NULL || param[i].value == NULL)
                {
                        WalError("Parameter value is null\n");
                        return WDMP_ERR_VALUE_IS_NULL;
                }

	        if(strlen(param[i].name) >= MAX_PARAMETERNAME_LEN)
                {
                        return WDMP_ERR_INVALID_PARAM;
                }

                if(strlen(param[i].value) >= MAX_PARAMETERVALUE_LEN)
                {
                        return WDMP_ERR_INVALID_PARAM;
                }

                // If input parameter is wildcard ending with "." then send error as wildcard is not supported for TEST_AND_SET
                if(param[i].name[(strlen(param[i].name)-1)] == '.')
                {
                        WalError("Wildcard SET/SET-ATTRIBUTES is not supported \n");
                        return WDMP_ERR_WILDCARD_NOT_SUPPORTED;		
                }
                // Prevent SET of CMC/CID through WebPA
                if(strcmp(param[i].name, PARAM_CID) == 0 || strcmp(param[i].name, PARAM_CMC) == 0)
                {
                        WalError("Invalid Input parameter - CID/CMC value cannot be set \n");
                        return WDMP_ERR_SET_OF_CMC_OR_CID_NOT_SUPPORTED;
                }

                if((type == SET_ATTRIBUTES) && (atoi(param[i].value) != 0) && (atoi(param[i].value) != 1))
                {
                        WalError("Notify value should be either 0 or 1\n");
                        return WDMP_ERR_INVALID_ATTRIBUTES;
                }
        }
        return WDMP_SUCCESS;
}

/**
 * @brief validate_table_object validates table object values
 *
 * @param[in] table object
 */
static WDMP_STATUS validate_table_object(table_req_t *tableObj)
{
        int i = 0, j = 0;
        WalPrint("------------ validate_table_object ----------\n");
        if(strlen(tableObj->objectName) >= MAX_PARAMETERNAME_LEN)
        {
                return WDMP_ERR_INVALID_PARAM;
        }

        if(tableObj->rows != NULL)
        {
                for(i=0; i< (int)(tableObj->rowCnt); i++)
                {
                        for(j=0; j< (int)(tableObj->rows[i].paramCnt); j++)
                        {
                                if(strlen(tableObj->rows[i].names[j]) >= MAX_PARAMETERNAME_LEN)
                                {
                                        return WDMP_ERR_INVALID_PARAM;
                                }

                                if(strlen(tableObj->rows[i].values[j]) >= MAX_PARAMETERVALUE_LEN)
                                {
                                        return WDMP_ERR_INVALID_PARAM;
                                }
                        }
                }
        }
        return WDMP_SUCCESS;
}

static void setRebootReason(param_t param, WEBPA_SET_TYPE setType)
{
	
	WDMP_STATUS retReason = WDMP_FAILURE;
	param_t *rebootParam = NULL;
	// Detect device reboot through WEBPA and log message for device reboot through webpa
	if(param.name != NULL && strcmp(param.name, WEBPA_DEVICE_REBOOT_PARAM) == 0 && param.value != NULL && strcmp(param.value, WEBPA_DEVICE_REBOOT_VALUE) == 0)
	{
		rebootParam = (param_t *) malloc(sizeof(param_t));

		// Using printf to log message to ArmConsolelog.txt.0 on XB3
		printf("RDKB_REBOOT : Reboot triggered through WEBPA\n");
		WalInfo("RDKB_REBOOT : Reboot triggered through WEBPA\n");
		rebootParam[0].name = "Device.DeviceInfo.X_RDKCENTRAL-COM_LastRebootReason";
		rebootParam[0].value = "webpa-reboot";
		rebootParam[0].type = WDMP_STRING;

		setValues(rebootParam, 1, setType, NULL, NULL, &retReason);

		if(retReason != WDMP_SUCCESS)
		{
			WalError("Failed to set Reason with status %d\n",retReason);
		}
		else
		{
			WalPrint("Successfully set Reason with status %d\n",retReason);
		}

		WAL_FREE(rebootParam);
	}
	
}

static void add_wildcard_timespan_to_response(money_trace_spans *timeSpan, money_trace_spans *wildcardSpan)
{
    unsigned int cnt = 0, count = 0;
    int matchFlag = 0;
    WalPrint("----------------- start of add_wildcard_timespan_to_response -------\n");
    count = timeSpan->count;
    WalPrint("count : %d\n",count);
    if(count == 0)
    {
        for(cnt = 0; cnt < wildcardSpan->count; cnt++)
        {
            WalPrint("wildcardSpan->spans[%d].name : %s, wildcardSpan->spans[%d].start : %llu, wildcardSpan->spans[%d].duration : %lu \n ",cnt,wildcardSpan->spans[cnt].name,cnt,wildcardSpan->spans[cnt].start,cnt,wildcardSpan->spans[cnt].duration);
        }

        timeSpan->spans = wildcardSpan->spans;
        timeSpan->count = wildcardSpan->count;

        for(cnt = 0; cnt < timeSpan->count; cnt++)
        {
            WalPrint("timeSpan->spans[%d].name : %s, timeSpan->spans[%d].start : %llu, timeSpan->spans[%d].duration : %lu \n ",cnt,timeSpan->spans[cnt].name,cnt,timeSpan->spans[cnt].start,cnt,timeSpan->spans[cnt].duration);
        }
    }
    else
    {
        if(wildcardSpan->spans != NULL)
        {
            for(cnt = 0; cnt < count; cnt++)
            {
                WalPrint("B4 compare\n");
                WalPrint("timeSpan->spans[%d].name : %s, wildcardSpan->spans[0].name : %s\n",cnt,timeSpan->spans[cnt].name,wildcardSpan->spans[0].name);
                if(0 != strcmp(timeSpan->spans[cnt].name, wildcardSpan->spans[0].name))
                {
                    timeSpan->spans[cnt].duration = timeSpan->spans[cnt].duration + wildcardSpan->spans[0].duration;
                    WalPrint("timeSpan->spans[%d].duration : %lu\n",cnt,timeSpan->spans[cnt].duration);
                    WAL_FREE(wildcardSpan->spans[0].name);
                    matchFlag = 1;
                    break;
                }
            }
            if(matchFlag == 0)
            {
                timeSpan->count = timeSpan->count +1;
                WalPrint("timeSpan->count : %d\n",timeSpan->count);
                timeSpan->spans = (money_trace_span *) realloc(timeSpan->spans,sizeof(money_trace_span)* timeSpan->count);
                timeSpan->spans[timeSpan->count - 1].name = strdup(wildcardSpan->spans[0].name);
                WalPrint("timeSpan->spans[%d].name : %s\n",timeSpan->count - 1,timeSpan->spans[timeSpan->count - 1].name);
                WalPrint("wildcardSpan->spans[0].start : %llu\n",wildcardSpan->spans[0].start);
                timeSpan->spans[timeSpan->count - 1].start = wildcardSpan->spans[0].start;
                WalPrint("timeSpan->spans[%d].start : %llu\n",timeSpan->count - 1,timeSpan->spans[timeSpan->count - 1].start);
                WalPrint("wildcardSpan->spans[0].duration : %lu\n",wildcardSpan->spans[0].duration);
                timeSpan->spans[timeSpan->count - 1].duration = wildcardSpan->spans[0].duration;
                WalPrint("timeSpan->spans[%d].duration : %lu\n",timeSpan->count - 1,timeSpan->spans[timeSpan->count - 1].duration);
                WAL_FREE(wildcardSpan->spans[0].name);
            }
            WAL_FREE(wildcardSpan->spans);
        }
        WAL_FREE(wildcardSpan);
    }
    WalPrint("----------------- End of add_wildcard_timespan_to_response -------\n");
}

static void add_total_webpa_client_time(uint64_t startTime,uint32_t duration,money_trace_spans *timeSpan)
{
    WalPrint("---------------- Start of add_total_webpa_client_time ----------------\n");
    if(timeSpan->count == 0)
    {
        timeSpan->count = timeSpan->count +1;
        WalPrint("timeSpan->count : %d\n",timeSpan->count);
        timeSpan->spans = (money_trace_span *) malloc(sizeof(money_trace_span)* timeSpan->count);
    }
    else
    {
        timeSpan->count = timeSpan->count +1;
        WalPrint("timeSpan->count : %d\n",timeSpan->count);
        timeSpan->spans = (money_trace_span *) realloc(timeSpan->spans,sizeof(money_trace_span)* timeSpan->count);
    }

    timeSpan->spans[timeSpan->count - 1].name = strdup(WEBPA_CLIENT);
    WalPrint("timeSpan->spans[%d].name : %s\n",timeSpan->count - 1,timeSpan->spans[timeSpan->count - 1].name);
    WalPrint("startTime : %llu\n",startTime);
    timeSpan->spans[timeSpan->count - 1].start = startTime;
    WalPrint("timeSpan->spans[%d].start : %llu\n",timeSpan->count - 1,timeSpan->spans[timeSpan->count - 1].start);
    WalPrint("timeDuration : %lu\n",duration);
    timeSpan->spans[timeSpan->count - 1].duration = duration;
    WalPrint("timeSpan->spans[%d].duration : %lu\n",timeSpan->count - 1,timeSpan->spans[timeSpan->count - 1].duration);
    WalPrint("---------------- End of add_total_webpa_client_time ----------------\n");
}
