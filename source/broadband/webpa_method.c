/**
 * @file webpa_method.c
 *
 * @description Implements the WebPA cloud-to-CPE RPC method invocation path.
 *              A method request arrives as an ordinary WebPA PATCH/SET carrying
 *              a single reserved parameter named RDK.Operate (dataType 5,
 *              WDMP_BASE64) whose value is a Base64-encoded UTF-8 JSON operate
 *              payload: { "method": "...", "params": { ... },
 *              "rspDestination": "..." }. The target RDK
 *              method is invoked synchronously via rbusMethod_Invoke and the
 *              result (or error) is returned to the cloud using the method
 *              response shape.
 *
 * Copyright (c) 2026  Comcast
 */

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#include <cJSON.h>
#include <trower-base64/base64.h>

#include "webpa_method.h"
#include "webpa_adapter.h"
#include "webpa_rbus.h"

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/
static char *base64Decode(const char *in, size_t *outLen);
static char *base64Encode(const char *in);
static char *buildErrorObject(int code, const char *data);
static char *buildErrorObjectFromJson(int code, cJSON *dataObj);
static char *buildMethodResponse(const char *name, int statusCode, cJSON *messageObj);
static rbusValueType_t wdmpToRbusType(int wdmpType);
static int mapRbusErrorToMethodError(rbusError_t rc);
static int jsonScalarToRbusValue(cJSON *val, rbusValue_t *out);
static int jsonLeafToRbusValue(cJSON *val, int wdmpType, rbusValue_t *out);
static int jsonObjectToRbus(cJSON *jsonObj, rbusObject_t rbusObj);
static cJSON *rbusValueToJson(rbusValue_t val);
static int rbusObjectToJson(rbusObject_t obj, cJSON *jsonOut);
#if 0
static void dumpRbusObjectWithTimestamp(const char *path, const char *label,
                const char *responseName, rbusObject_t obj);
#endif                
static int parseOperatePayload(set_req_t *setReq,
                char **methodName,
                rbusObject_t *inParams, bool *isAsync, char **errorObj);
/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/

bool isMethodInvokeRequest(set_req_t *setReq)
{
    if(setReq == NULL || setReq->param == NULL)
        return false;

    return (setReq->param[0].name != NULL &&
            strcmp(setReq->param[0].name, RDK_OPERATE_PARAM) == 0);
}

void handleMethodInvoke(set_req_t *setReq, res_struct *resObj)
{
        const char *responseName = RDK_OPERATE_PARAM;
        char *methodName = NULL;
        char *errorObj = NULL;
        char *resultStr = NULL;
        char *b64message = NULL;
        cJSON *resultObj = NULL;
        rbusObject_t inParams = NULL;
        rbusObject_t outParams = NULL;
        WDMP_STATUS methodStatus = WDMP_FAILURE;
        rbusError_t rc = RBUS_ERROR_SUCCESS;
        bool isAsync = false;

        WalPrint("************** handleMethodInvoke *****************\n");

        /* Validate and parse the operate payload into method name and input params. */
        if(parseOperatePayload(setReq, &methodName,
                              &inParams, &isAsync, &errorObj) != 0)
        {
                goto respond;
        }
        responseName = methodName;

        if(isAsync)
        {
                WalError("Async method invocation is not supported\n");
                errorObj = buildErrorObject(METHOD_ERR_INVALID_REQUEST,
                        "The async request is not supported");
                goto respond;
        }

        /* Invoke the method synchronously (blocking) and return the result
         * directly in the response payload carried by the HTTP response. */
        rc = webpaRbusMethodInvoke(responseName, inParams, &outParams);
#if 0
        dumpRbusObjectWithTimestamp("/tmp/webpa_method_inParams.txt", "inParams", responseName, inParams);
        dumpRbusObjectWithTimestamp("/tmp/webpa_method_outParams.txt", "outParams", responseName, outParams);
#endif
        if((rc != RBUS_ERROR_SUCCESS))
        {
                int code = mapRbusErrorToMethodError(rc);
                const char *rbusDetail = rbusError_ToString(rc);
                char detail[256] = {'\0'};
                snprintf(detail, sizeof(detail), "RBUS error: %s", (rbusDetail != NULL && rbusDetail[0] != '\0') ? rbusDetail : "unknown");                
                WalError("rbusMethod_Invoke failed for method '%s' with error code %d: %s\n", responseName, rc, detail);
                if(outParams != NULL)
                {
                        resultObj = cJSON_CreateObject();
                        if(rbusObjectToJson(outParams, resultObj) == 0)
                        {
                                errorObj = buildErrorObjectFromJson(code, resultObj);                                
                        }
                        else
                        {
                                WalError("Failed to convert RBUS outParams to JSON\n");
                                errorObj = buildErrorObject(METHOD_ERR_INTERNAL, "Failed to convert RBUS result object to JSON");
                        }
                }
                else
                {
                        errorObj = buildErrorObject(code, detail);
                }
                goto respond;
        }

        /* Convert the RBUS result object back into JSON. */
        resultObj = cJSON_CreateObject();
        if(outParams != NULL)
        {       
                if(rbusObjectToJson(outParams, resultObj) != 0)
                {
                        WalError("Failed to convert RBUS result object to JSON\n");
                        cJSON_Delete(resultObj);
                        resultObj = NULL;
                        errorObj = buildErrorObject(METHOD_ERR_INTERNAL,
                                "Failed to convert RBUS result object to JSON");
                        goto respond;
                }
        }
        else
        {
                WalInfo("method outParams is empty\n");
        }

        cJSON *resultWrapper = cJSON_CreateObject();
        if(resultWrapper == NULL)
        {
                WalError("Failed to allocate result wrapper JSON object\n");
                errorObj = buildErrorObject(METHOD_ERR_INTERNAL,"Failed to serialize method result JSON");
                goto respond;
        }
        cJSON_AddItemToObject(resultWrapper, "result", resultObj);
        resultObj = resultWrapper;
        resultStr = cJSON_PrintUnformatted(resultObj);

        if(resultStr == NULL)
        {
                WalError("Failed to serialize method result JSON\n");
                errorObj = buildErrorObject(METHOD_ERR_INTERNAL,
                        "Failed to serialize method result JSON");
                goto respond;
        }
        WalInfo("Method response: %s\n", resultStr);

        methodStatus = WDMP_SUCCESS;
        WalPrint("Method %s invoked successfully\n", responseName);

respond:
        if(resObj != NULL)
        {
                resObj->reqType = METHOD;
                resObj->paramCnt = 1;

                if(resObj->retStatus == NULL)
                {
                        resObj->retStatus = (WDMP_STATUS *) malloc(sizeof(WDMP_STATUS));
                }
                if(resObj->retStatus != NULL)
                {
                        resObj->retStatus[0] = methodStatus;
                }

                if(resObj->u.paramRes == NULL)
                {
                        resObj->u.paramRes = (param_res_t *) malloc(sizeof(param_res_t));
                        if(resObj->u.paramRes != NULL)
                        {
                                memset(resObj->u.paramRes, 0, sizeof(param_res_t));
                        }
                }

                if(resObj->u.paramRes != NULL && resObj->u.paramRes->params == NULL)
                {
                        resObj->u.paramRes->params = (param_t *) malloc(sizeof(param_t));
                        if(resObj->u.paramRes->params != NULL)
                        {
                                memset(resObj->u.paramRes->params, 0, sizeof(param_t));
                        }
                }

                if(resObj->u.paramRes != NULL && resObj->u.paramRes->params != NULL)
                {
                        WAL_FREE(resObj->u.paramRes->params[0].name);
                        WAL_FREE(resObj->u.paramRes->params[0].value);

                        resObj->u.paramRes->params[0].name = strdup(responseName);
                        if(errorObj != NULL)
                        {
                                WalPrint("Encoding method error payload for %s: %s\n", responseName, errorObj);
                                b64message = base64Encode(errorObj);
                                if(b64message != NULL)
                                {
                                        resObj->u.paramRes->params[0].value = strdup(b64message);
                                        WAL_FREE(b64message);
                                }
                                else
                                {
                                        WalError("Failed to base64-encode method error payload for %s\n",
                                                responseName);
                                        resObj->u.paramRes->params[0].value = NULL;
                                }
                                resObj->u.paramRes->params[0].type = WDMP_BASE64;
                        }
                        else if(resultStr != NULL)
                        {
                                WalPrint("Encoding method result payload for %s: %s\n", responseName, resultStr);
                                b64message = base64Encode(resultStr);
                                if(b64message != NULL)
                                {
                                        resObj->u.paramRes->params[0].value = strdup(b64message);
                                        WAL_FREE(b64message);
                                }
                                else
                                {
                                        WalError("Failed to base64-encode method result payload for %s\n",
                                                responseName);
                                        resObj->u.paramRes->params[0].value = NULL;
                                }
                                resObj->u.paramRes->params[0].type = WDMP_BASE64;
                        }
                        else
                        {
                                WalError("No method result/error payload available for %s\n",
                                        responseName);
                                resObj->u.paramRes->params[0].value = NULL;
                                resObj->u.paramRes->params[0].type = WDMP_NONE;
                        }
                }
        }

        WAL_FREE(errorObj);
        WAL_FREE(methodName);
        WAL_FREE(resultStr);
        if(resultObj != NULL)
        {
                cJSON_Delete(resultObj);
        }
        WAL_FREE(b64message);
        if(inParams != NULL)
        {
                rbusObject_Release(inParams);
        }
        if(outParams != NULL)
        {
                rbusObject_Release(outParams);
        }
        WalPrint("************** handleMethodInvoke *****************\n");
}

/**
 * @brief parseOperatePayload validates the set request and parses the Base64-encoded
 *        operate JSON payload into the method name and RBUS input parameters.
 *
 * @param[in]  setReq      incoming set request
 * @param[out] methodName  extracted method name (caller frees)
 * @param[out] inParams    RBUS input object (caller releases)
 * @param[out] errorObj    error JSON string on failure (caller frees)
 * @return 0 on success, -1 on failure (errorObj is set)
 */
static int parseOperatePayload(set_req_t *setReq,
                char **methodName,
                rbusObject_t *inParams, bool *isAsync, char **errorObj)
{
        char *decoded = NULL;
        size_t decodedLen = 0;
        cJSON *operateJson = NULL;
        cJSON *methodItem = NULL;
        cJSON *paramsItem = NULL;
        cJSON *rspDestinationItem = NULL;
        const char *rspDestination = NULL;
        rbusObject_t tmpParams = NULL;
        param_t *p = NULL;

        /* A method request carries exactly one RDK.Operate parameter. */
        if(setReq->paramCnt != 1)
        {
                WalError("RDK.Operate method request must carry exactly one parameter\n");
                *errorObj = buildErrorObject(METHOD_ERR_INVALID_REQUEST,
                        "RDK.Operate method request must carry exactly one parameter");
                return -1;
        }

        p = &setReq->param[0];
        if(p->type != WDMP_BASE64)
        {
                WalError("RDK.Operate parameter must be Base64 (dataType 5)\n");
                *errorObj = buildErrorObject(METHOD_ERR_INVALID_REQUEST,
                        "RDK.Operate parameter must be Base64 (dataType 5)");
                return -1;
        }
        if(p->value == NULL)
        {
                WalError("RDK.Operate parameter value is missing\n");
                *errorObj = buildErrorObject(METHOD_ERR_INVALID_REQUEST,
                        "RDK.Operate parameter value is missing");
                return -1;
        }

        /* Base64-decode the operate payload. */
        decoded = base64Decode(p->value, &decodedLen);
        if(decoded == NULL)
        {
                WalError("Failed to Base64-decode operate payload\n");
                *errorObj = buildErrorObject(METHOD_ERR_PARSE,
                        "Failed to Base64-decode operate payload");
                return -1;
        }
        WalPrint("Base64-decoded operate payload (%zu bytes): %s\n", decodedLen, decoded);

        /* Parse the decoded operate payload JSON. */
        operateJson = cJSON_Parse(decoded);
        WAL_FREE(decoded);
        if(operateJson == NULL)
        {
                WalError("Operate payload is not valid JSON\n");
                *errorObj = buildErrorObject(METHOD_ERR_PARSE,
                        "Operate payload is not valid JSON");
                return -1;
        }

        /* Extract the mandatory method name. */
        methodItem = cJSON_GetObjectItem(operateJson, "method");
        if(methodItem == NULL || !cJSON_IsString(methodItem) ||
                methodItem->valuestring == NULL || methodItem->valuestring[0] == '\0')
        {
                WalError("Operate payload is missing the method name\n");
                *errorObj = buildErrorObject(METHOD_ERR_INVALID_REQUEST,
                        "Operate payload is missing the method name");
                cJSON_Delete(operateJson);
                return -1;
        }
        *methodName = strdup(methodItem->valuestring);
        if(*methodName == NULL)
        {
                WalError("Failed to allocate memory for method name\n");
                *errorObj = buildErrorObject(METHOD_ERR_INTERNAL,
                        "Failed to allocate memory for method name");
                cJSON_Delete(operateJson);
                return -1;
        }
        WalPrint("Method invocation target: %s\n", *methodName);

        /* Convert the optional params object into an RBUS input object. */
        rbusObject_Init(&tmpParams, NULL);
        paramsItem = cJSON_GetObjectItem(operateJson, "params");
        if(paramsItem != NULL && cJSON_IsObject(paramsItem))
        {
                if(jsonObjectToRbus(paramsItem, tmpParams) != 0)
                {
                        WalError("Failed to convert params to RBUS input object\n");
                        *errorObj = buildErrorObject(METHOD_ERR_INTERNAL,
                                "Failed to convert params to RBUS input object");
                        rbusObject_Release(tmpParams);
                        cJSON_Delete(operateJson);
                        return -1;
                }
        }

        /* Optional rspDestination controls invocation mode. When provided,
         * the request is asynchronous. */
        *isAsync = false;
        rspDestinationItem = cJSON_GetObjectItem(operateJson, "rspDestination");
        if(rspDestinationItem != NULL)
        {
                if(!cJSON_IsString(rspDestinationItem) || rspDestinationItem->valuestring == NULL)
                {
                        WalError("Operate payload has invalid rspDestination\n");
                        *errorObj = buildErrorObject(METHOD_ERR_INVALID_REQUEST,
                                "Operate payload has invalid rspDestination");
                        rbusObject_Release(tmpParams);
                        cJSON_Delete(operateJson);
                        return -1;
                }
                rspDestination = rspDestinationItem->valuestring;
                if(rspDestination[0] != '\0')
                {
                        *isAsync = true;
                }
        }

        *inParams = tmpParams;
        cJSON_Delete(operateJson);
        return 0;
}

/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/

/**
 * @brief base64Decode decodes a Base64 string into a NUL-terminated buffer.
 *
 * @param[in]  in     Base64-encoded NUL-terminated string
 * @param[out] outLen number of decoded bytes
 * @return newly-allocated decoded buffer (caller frees) or NULL on failure
 */
static char *base64Decode(const char *in, size_t *outLen)
{
        size_t inLen = 0;
        size_t decSize = 0;
        size_t n = 0;
        uint8_t *out = NULL;

        if(in == NULL || in[0] == '\0')
        {
                return NULL;
        }
        inLen = strlen(in);
        decSize = b64_get_decoded_buffer_size(inLen);
        out = (uint8_t *) malloc(decSize + 1);
        if(out == NULL)
        {
                return NULL;
        }
        n = b64_decode((const uint8_t *) in, inLen, out);
        if(n == 0)
        {
                free(out);
                return NULL;
        }
        out[n] = '\0';
        if(outLen != NULL)
        {
                *outLen = n;
        }
        return (char *) out;
}

/**
 * @brief base64Encode Base64-encodes a NUL-terminated string.
 *
 * @param[in] in NUL-terminated input string
 * @return newly-allocated Base64 string (caller frees) or NULL on failure
 */
static char *base64Encode(const char *in)
{
        size_t inLen = 0;
        size_t encSize = 0;
        char *out = NULL;

        if(in == NULL)
        {
                return NULL;
        }
        inLen = strlen(in);
        encSize = b64_get_encoded_buffer_size(inLen);
        out = (char *) malloc(encSize + 1);
        if(out == NULL)
        {
                return NULL;
        }
        b64_encode((const uint8_t *) in, inLen, (uint8_t *) out);
        out[encSize] = '\0';
        return out;
}

/**
 * @brief buildErrorObject builds a { "error": { "code", "data" } } cJSON object.
 */
static char *buildErrorObject(int code, const char *data)
{
        cJSON *root = cJSON_CreateObject();
        cJSON *err = cJSON_CreateObject();
        char *out = NULL;

        cJSON_AddNumberToObject(err, "code", code);
        if(data != NULL)
        {
                cJSON_AddStringToObject(err, "data", data);
        }
        cJSON_AddItemToObject(root, "error", err);
        out = cJSON_PrintUnformatted(root);
        cJSON_Delete(root);
        return out;
}

static char *buildErrorObjectFromJson(int code, cJSON *dataObj)
{
        cJSON *root = cJSON_CreateObject();
        cJSON *err = cJSON_CreateObject();
        char *out = NULL;

        cJSON_AddNumberToObject(err, "code", code);
        if(dataObj != NULL)
        {
                cJSON_AddItemToObject(err, "data", cJSON_Duplicate(dataObj, 1));
        }
        cJSON_AddItemToObject(root, "error", err);
        out = cJSON_PrintUnformatted(root);
        cJSON_Delete(root);
        return out;
}

/**
 * @brief buildMethodResponse builds the method response payload
 *        { "statusCode", "parameters": [ { "name", "message" } ] } where
 *        message is the Base64-encoded result/error object.
 *
 * @param[in] name       invoked method name (or RDK.Operate if unknown)
 * @param[in] statusCode WDMP_SUCCESS on success, WDMP_FAILURE on failure
 * @param[in] messageObj result/error cJSON object (not consumed)
 * @return newly-allocated payload string (caller frees)
 */
static char *buildMethodResponse(const char *name, int statusCode, cJSON *messageObj)
{
        char *messageStr = NULL;
        char *b64message = NULL;
        char *payload = NULL;
        cJSON *root = NULL;
        cJSON *paramsArr = NULL;
        cJSON *entry = NULL;

        WalInfo("buildMethodResponse: name='%s', statusCode=%d\n",
                name != NULL ? name : RDK_OPERATE_PARAM, statusCode);

        if(messageObj != NULL)
        {
                messageStr = cJSON_PrintUnformatted(messageObj);
                if(messageStr == NULL)
                {
                        WalError("buildMethodResponse: failed to serialize message object\n");
                }
                else
                {
                        WalInfo("buildMethodResponse: message JSON: %s\n", messageStr);
                }
        }
        if(messageStr != NULL)
        {
                b64message = base64Encode(messageStr);
                if(b64message == NULL)
                {
                        WalError("buildMethodResponse: failed to base64-encode message\n");
                }
                else
                {
                        WalInfo("buildMethodResponse: message base64: %s\n", b64message);
                }
                free(messageStr);
        }

        root = cJSON_CreateObject();
        cJSON_AddNumberToObject(root, "statusCode", statusCode);
        paramsArr = cJSON_CreateArray();
        entry = cJSON_CreateObject();
        cJSON_AddStringToObject(entry, "name", name != NULL ? name : RDK_OPERATE_PARAM);
        cJSON_AddStringToObject(entry, "message", b64message != NULL ? b64message : "");
        cJSON_AddItemToArray(paramsArr, entry);
        cJSON_AddItemToObject(root, "parameters", paramsArr);

        payload = cJSON_PrintUnformatted(root);
        if(payload == NULL)
        {
                WalError("buildMethodResponse: failed to serialize response payload\n");
        }
        else
        {
                WalInfo("buildMethodResponse: payload generated (%zu bytes)\n", strlen(payload));
                WalInfo("buildMethodResponse: payload: %s\n", payload);
        }
        cJSON_Delete(root);
        WAL_FREE(b64message);
        return payload;
}

/**
 * @brief wdmpToRbusType maps a WDMP DATA_TYPE to the closest rbusValueType_t.
 */
static rbusValueType_t wdmpToRbusType(int wdmpType)
{
        switch(wdmpType)
        {
                case WDMP_INT:     return RBUS_INT32;
                case WDMP_UINT:    return RBUS_UINT32;
                case WDMP_BOOLEAN: return RBUS_BOOLEAN;
                case WDMP_LONG:    return RBUS_INT64;
                case WDMP_ULONG:   return RBUS_UINT64;
                case WDMP_FLOAT:   return RBUS_SINGLE;
                case WDMP_DOUBLE:  return RBUS_DOUBLE;
                case WDMP_DATETIME:return RBUS_DATETIME;
                case WDMP_BASE64:  return RBUS_BYTES;
                case WDMP_BYTE:    return RBUS_BYTES;
                case WDMP_NONE:    return RBUS_NONE;
                case WDMP_STRING:
                default:           return RBUS_STRING;
        }
}

static int mapRbusErrorToMethodError(rbusError_t rc)
{
        switch(rc)
        {
                case RBUS_ERROR_DESTINATION_NOT_FOUND:
                        return METHOD_ERR_METHOD_NOT_FOUND;
                case RBUS_ERROR_INVALID_INPUT:
                        return METHOD_ERR_INVALID_PARAMS;
                default:
                        return METHOD_ERR_INTERNAL;
        }
}

/**
 * @brief jsonScalarToRbusValue converts an untyped JSON scalar to an rbusValue.
 */
static int jsonScalarToRbusValue(cJSON *val, rbusValue_t *out)
{
        rbusValue_t tmp = NULL;

        rbusValue_Init(&tmp);
        if(cJSON_IsString(val))
        {
                rbusValue_SetString(tmp, val->valuestring != NULL ? val->valuestring : "");
        }
        else if(cJSON_IsBool(val))
        {
                rbusValue_SetBoolean(tmp, cJSON_IsTrue(val) ? true : false);
        }
        else if(cJSON_IsNumber(val))
        {
                if(val->valuedouble == (double) val->valueint)
                {
                        rbusValue_SetInt32(tmp, val->valueint);
                }
                else
                {
                        rbusValue_SetDouble(tmp, val->valuedouble);
                }
        }
        else if(cJSON_IsNull(val))
        {
                rbusValue_SetString(tmp, "");
        }
        else
        {
                rbusValue_Release(tmp);
                return -1;
        }
        *out = tmp;
        return 0;
}

/**
 * @brief jsonLeafToRbusValue converts a typed JSON leaf { "value", "dataType" }
 *        into an rbusValue of the mapped type.
 */
static int jsonLeafToRbusValue(cJSON *val, int wdmpType, rbusValue_t *out)
{
        rbusValueType_t rt = wdmpToRbusType(wdmpType);
        rbusValue_t tmp = NULL;
        const char *str = NULL;

        if(cJSON_IsString(val))
        {
                str = val->valuestring != NULL ? val->valuestring : "";
        }
        else
        {
                /* The operate payload always encodes parameter values as JSON strings
                 * (e.g. "value": "<string>"), regardless of the target dataType.
                 * If the value is not a string, it is invalid; return -1 for error. */
                return -1;
        }

        rbusValue_Init(&tmp);
        if(!rbusValue_SetFromString(tmp, rt, str))
        {
                /* rbusValue_SetFromString failed to parse the string into the
                 * requested rbus type; release and return -1 for error. */
                rbusValue_Release(tmp);
                return -1;
        }
        *out = tmp;
        return 0;
}

/**
 * @brief jsonObjectToRbus converts a JSON params object into an rbusObject,
 *        mapping typed leaves and nested objects recursively.
 *
 * @return 0 on success, -1 on failure
 */
static int jsonObjectToRbus(cJSON *jsonObj, rbusObject_t rbusObj)
{
        cJSON *child = NULL;

        for(child = jsonObj->child; child != NULL; child = child->next)
        {
                const char *key = child->string;
                rbusValue_t rv = NULL;

                if(key == NULL)
                {
                        return -1;
                }

                if(cJSON_IsObject(child))
                {
                        cJSON *dt = cJSON_GetObjectItem(child, "dataType");
                        cJSON *leafVal = cJSON_GetObjectItem(child, "value");

                        if(dt != NULL && cJSON_IsNumber(dt) && leafVal != NULL)
                        {
                                /* Typed leaf: { "value": ..., "dataType": <n> }. */
                                if(jsonLeafToRbusValue(leafVal, dt->valueint, &rv) != 0)
                                {
                                        return -1;
                                }
                                rbusObject_SetValue(rbusObj, key, rv);
                                rbusValue_Release(rv);
                        }
                        else
                        {
                                /* Nested params object. */
                                rbusObject_t nested = NULL;
                                rbusObject_Init(&nested, NULL);
                                if(jsonObjectToRbus(child, nested) != 0)
                                {
                                        rbusObject_Release(nested);
                                        return -1;
                                }
                                rbusValue_Init(&rv);
                                rbusValue_SetObject(rv, nested);
                                rbusObject_SetValue(rbusObj, key, rv);
                                rbusValue_Release(rv);
                                rbusObject_Release(nested);
                        }
                }
                else
                {
                        /* Untyped scalar leaf. */
                        if(jsonScalarToRbusValue(child, &rv) != 0)
                        {
                                return -1;
                        }
                        rbusObject_SetValue(rbusObj, key, rv);
                        rbusValue_Release(rv);
                }
        }
        return 0;
}

/**
 * @brief rbusValueToJson converts a single rbusValue into a JSON scalar
 *        or nested object for RBUS_OBJECT.
 *
 * @return newly-allocated cJSON node or NULL on failure
 */
static cJSON *rbusValueToJson(rbusValue_t val)
{
        rbusValueType_t rt = rbusValue_GetType(val);
        cJSON *leaf = NULL;

        if(rt == RBUS_OBJECT)
        {
                rbusObject_t child = rbusValue_GetObject(val);
                cJSON *nested = cJSON_CreateObject();
                if(child != NULL && rbusObjectToJson(child, nested) != 0)
                {
                        cJSON_Delete(nested);
                        return NULL;
                }
                return nested;
        }

        switch(rt)
        {
                case RBUS_BOOLEAN:
                        leaf = cJSON_CreateBool(rbusValue_GetBoolean(val));
                        break;
                case RBUS_INT8:
                        leaf = cJSON_CreateNumber(rbusValue_GetInt8(val));
                        break;
                case RBUS_UINT8:
                        leaf = cJSON_CreateNumber(rbusValue_GetUInt8(val));
                        break;
                case RBUS_INT16:
                        leaf = cJSON_CreateNumber(rbusValue_GetInt16(val));
                        break;
                case RBUS_UINT16:
                        leaf = cJSON_CreateNumber(rbusValue_GetUInt16(val));
                        break;
                case RBUS_INT32:
                        leaf = cJSON_CreateNumber(rbusValue_GetInt32(val));
                        break;
                case RBUS_UINT32:
                        leaf = cJSON_CreateNumber((double) rbusValue_GetUInt32(val));
                        break;
                case RBUS_INT64:
                        leaf = cJSON_CreateNumber((double) rbusValue_GetInt64(val));
                        break;
                case RBUS_UINT64:
                        leaf = cJSON_CreateNumber((double) rbusValue_GetUInt64(val));
                        break;
                case RBUS_SINGLE:
                        leaf = cJSON_CreateNumber(rbusValue_GetSingle(val));
                        break;
                case RBUS_DOUBLE:
                        leaf = cJSON_CreateNumber(rbusValue_GetDouble(val));
                        break;
                case RBUS_STRING:
                {
                        int len = 0;
                        const char *s = rbusValue_GetString(val, &len);
                        if(s != NULL && (s[0] == '{' || s[0] == '[' || s[0] == '"'))
                        {
                                cJSON *parsed = cJSON_Parse(s);
                                if(parsed != NULL)
                                {
                                        leaf = parsed;
                                        break;
                                }
                        }
                        leaf = cJSON_CreateString(s != NULL ? s : "");
                        break;
                }
                default:
                {
                        char buf[512] = {'\0'};
                        rbusValue_ToString(val, buf, sizeof(buf));
                        leaf = cJSON_CreateString(buf);
                        break;
                }
        }
        return leaf;
}

/**
 * @brief rbusObjectToJson converts an rbusObject's properties into JSON.
 *
 * @return 0 on success, -1 on failure
 */
static int rbusObjectToJson(rbusObject_t obj, cJSON *jsonOut)
{
        if(obj == NULL || jsonOut == NULL)
        {
                WalError("rbusObjectToJson: invalid input (obj=%p, jsonOut=%p)\n", obj, jsonOut);
                return -1;
        }

        WalPrint("rbusObjectToJson: start converting RBUS object to JSON\n");
        rbusProperty_t prop = rbusObject_GetProperties(obj);

        if(prop == NULL)
        {
                WalInfo("rbusObjectToJson: RBUS object has no properties\n");
        }

        while(prop != NULL)
        {
                const char *name = rbusProperty_GetName(prop);
                rbusValue_t val = rbusProperty_GetValue(prop);

                if(name != NULL && val != NULL)
                {
                        WalPrint("rbusObjectToJson: converting property '%s'\n", name);
                        cJSON *leaf = rbusValueToJson(val);
                        if(leaf == NULL)
                        {
                                WalError("rbusObjectToJson: failed converting property '%s'\n", name);
                                return -1;
                        }
                        cJSON_AddItemToObject(jsonOut, name, leaf);
                }
                else
                {
                        WalError("rbusObjectToJson: skipping property with missing name/value (name=%s, value=%p)\n",
                                name != NULL ? name : "<null>", val);
                }                             
                prop = rbusProperty_GetNext(prop);
        }
        WalPrint("rbusObjectToJson: conversion completed successfully\n");
        return 0;
}
#if 0
static void dumpRbusObjectWithTimestamp(const char *path, const char *label,
                const char *responseName, rbusObject_t obj)
{
        FILE *tmpOut = NULL;
        time_t now = 0;
        struct tm localTm;
        char timestamp[32] = {'\0'};

        if(path == NULL || label == NULL || responseName == NULL || obj == NULL)
        {
                return;
        }

        tmpOut = fopen(path, "a+");
        if(tmpOut == NULL)
        {
                WalError("Failed to open %s for writing %s\n", path, label);
                return;
        }

        now = time(NULL);
        if(now != (time_t) -1 && localtime_r(&now, &localTm) != NULL &&
                strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", &localTm) > 0)
        {
                fprintf(tmpOut, "[%s] method=%s %s\n", timestamp, responseName, label);
        }
        else
        {
                fprintf(tmpOut, "[timestamp unavailable] method=%s %s\n", responseName, label);
        }

        rbusObject_fwrite(obj, 1, tmpOut);
        fputc('\n', tmpOut);
        fclose(tmpOut);

        if(timestamp[0] != '\0')
        {
                WalPrint("Dumped method %s to %s at %s\n", label, path, timestamp);
        }
        else
        {
                WalPrint("Dumped method %s to %s\n", label, path);
        }
}
#endif
