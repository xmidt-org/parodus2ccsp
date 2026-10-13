/**
 *  Copyright 2010-2023 Comcast Cable Communications Management, LLC
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <CUnit/Basic.h>
#include <rbus/rbus.h>
#include <trower-base64/base64.h>
#include <cJSON.h>
#include <wdmp-c.h>

#define UNUSED(x) (void)(x)

/*----------------------------------------------------------------------------*/
/*                            Mock Control Variables                           */
/*----------------------------------------------------------------------------*/
static rbusError_t mock_rbus_invoke_rc = RBUS_ERROR_SUCCESS;
static rbusObject_t mock_rbus_outParams = NULL;

/*----------------------------------------------------------------------------*/
/*                                   Mocks                                    */
/*----------------------------------------------------------------------------*/
rbusError_t webpaRbusMethodInvoke(const char *methodName, rbusObject_t inParams, rbusObject_t *outParams)
{
    UNUSED(methodName);
    UNUSED(inParams);
    *outParams = mock_rbus_outParams;
    return mock_rbus_invoke_rc;
}

/* Include the .c file directly to access static functions */
#include "../source/broadband/webpa_method.c"

/*----------------------------------------------------------------------------*/
/*                             Helper Functions                                */
/*----------------------------------------------------------------------------*/

/**
 * @brief Encode a plain string to base64 for building test payloads.
 */
static char *testBase64Encode(const char *in)
{
    size_t inLen = strlen(in);
    size_t encSize = b64_get_encoded_buffer_size(inLen);
    char *out = (char *)malloc(encSize + 1);
    if (out == NULL) return NULL;
    b64_encode((const uint8_t *)in, inLen, (uint8_t *)out);
    out[encSize] = '\0';
    return out;
}

/**
 * @brief Build a set_req_t with a single RDK.Operate parameter.
 */
static set_req_t *buildOperateRequest(const char *b64Value, int dataType)
{
    set_req_t *req = (set_req_t *)malloc(sizeof(set_req_t));
    memset(req, 0, sizeof(set_req_t));
    req->paramCnt = 1;
    req->param = (param_t *)malloc(sizeof(param_t));
    memset(req->param, 0, sizeof(param_t));
    req->param[0].name = strdup(RDK_OPERATE_PARAM);
    req->param[0].value = b64Value ? strdup(b64Value) : NULL;
    req->param[0].type = dataType;
    return req;
}

static void freeOperateRequest(set_req_t *req)
{
    if (req == NULL) return;
    if (req->param != NULL)
    {
        free(req->param[0].name);
        free(req->param[0].value);
        free(req->param);
    }
    free(req);
}

static res_struct *allocResObj(void)
{
    res_struct *resObj = (res_struct *)malloc(sizeof(res_struct));
    memset(resObj, 0, sizeof(res_struct));
    return resObj;
}

static void freeResObj(res_struct *resObj)
{
    if (resObj == NULL) return;
    if (resObj->retStatus != NULL) free(resObj->retStatus);
    if (resObj->u.paramRes != NULL)
    {
        if (resObj->u.paramRes->params != NULL)
        {
            free(resObj->u.paramRes->params[0].name);
            free(resObj->u.paramRes->params[0].value);
            free(resObj->u.paramRes->params);
        }
        free(resObj->u.paramRes);
    }
    free(resObj);
}

/*----------------------------------------------------------------------------*/
/*                      Tests for isMethodInvokeRequest                        */
/*----------------------------------------------------------------------------*/

void test_isMethodInvokeRequest_null_input()
{
    CU_ASSERT_FALSE(isMethodInvokeRequest(NULL));
}

void test_isMethodInvokeRequest_null_param()
{
    set_req_t req;
    memset(&req, 0, sizeof(req));
    req.param = NULL;
    CU_ASSERT_FALSE(isMethodInvokeRequest(&req));
}

void test_isMethodInvokeRequest_null_param_name()
{
    set_req_t req;
    param_t p;
    memset(&req, 0, sizeof(req));
    memset(&p, 0, sizeof(p));
    req.param = &p;
    p.name = NULL;
    CU_ASSERT_FALSE(isMethodInvokeRequest(&req));
}

void test_isMethodInvokeRequest_non_matching_name()
{
    set_req_t req;
    param_t p;
    memset(&req, 0, sizeof(req));
    memset(&p, 0, sizeof(p));
    req.param = &p;
    p.name = "Device.WiFi.SSID";
    CU_ASSERT_FALSE(isMethodInvokeRequest(&req));
}

void test_isMethodInvokeRequest_valid()
{
    set_req_t req;
    param_t p;
    memset(&req, 0, sizeof(req));
    memset(&p, 0, sizeof(p));
    req.param = &p;
    p.name = "RDK.Operate";
    CU_ASSERT_TRUE(isMethodInvokeRequest(&req));
}

/*----------------------------------------------------------------------------*/
/*                       Tests for handleMethodInvoke                          */
/*----------------------------------------------------------------------------*/

void test_handleMethodInvoke_invalid_paramCnt()
{
    /* paramCnt != 1 should produce an error response */
    set_req_t req;
    param_t params[2];
    memset(&req, 0, sizeof(req));
    memset(params, 0, sizeof(params));
    req.paramCnt = 2;
    req.param = params;
    params[0].name = strdup(RDK_OPERATE_PARAM);
    params[0].value = strdup("dGVzdA==");
    params[0].type = WDMP_BASE64;

    res_struct *resObj = allocResObj();
    handleMethodInvoke(&req, resObj);

    CU_ASSERT_PTR_NOT_NULL(resObj->retStatus);
    CU_ASSERT_EQUAL(resObj->retStatus[0], WDMP_FAILURE);
    CU_ASSERT_PTR_NOT_NULL(resObj->u.paramRes);
    CU_ASSERT_PTR_NOT_NULL(resObj->u.paramRes->params);
    /* Response name should be RDK.Operate (fallback) */
    CU_ASSERT_STRING_EQUAL(resObj->u.paramRes->params[0].name, RDK_OPERATE_PARAM);

    freeResObj(resObj);
    free(params[0].name);
    free(params[0].value);
}

void test_handleMethodInvoke_wrong_datatype()
{
    /* dataType != WDMP_BASE64 should produce an error response */
    char *b64 = testBase64Encode("{\"method\":\"Device.Test()\"}");
    set_req_t *req = buildOperateRequest(b64, WDMP_STRING);
    free(b64);

    res_struct *resObj = allocResObj();
    handleMethodInvoke(req, resObj);

    CU_ASSERT_PTR_NOT_NULL(resObj->retStatus);
    CU_ASSERT_EQUAL(resObj->retStatus[0], WDMP_FAILURE);
    CU_ASSERT_STRING_EQUAL(resObj->u.paramRes->params[0].name, RDK_OPERATE_PARAM);

    freeResObj(resObj);
    freeOperateRequest(req);
}

void test_handleMethodInvoke_null_value()
{
    /* NULL value should produce an error response */
    set_req_t *req = buildOperateRequest(NULL, WDMP_BASE64);

    res_struct *resObj = allocResObj();
    handleMethodInvoke(req, resObj);

    CU_ASSERT_PTR_NOT_NULL(resObj->retStatus);
    CU_ASSERT_EQUAL(resObj->retStatus[0], WDMP_FAILURE);

    freeResObj(resObj);
    freeOperateRequest(req);
}

void test_handleMethodInvoke_invalid_base64()
{
    /* Non-base64 value should produce a parse error */
    set_req_t *req = buildOperateRequest("!!!invalid-base64!!!", WDMP_BASE64);

    res_struct *resObj = allocResObj();
    handleMethodInvoke(req, resObj);

    CU_ASSERT_PTR_NOT_NULL(resObj->retStatus);
    CU_ASSERT_EQUAL(resObj->retStatus[0], WDMP_FAILURE);

    freeResObj(resObj);
    freeOperateRequest(req);
}

void test_handleMethodInvoke_invalid_json()
{
    /* Valid base64 but invalid JSON inside */
    char *b64 = testBase64Encode("this is not json");
    set_req_t *req = buildOperateRequest(b64, WDMP_BASE64);
    free(b64);

    res_struct *resObj = allocResObj();
    handleMethodInvoke(req, resObj);

    CU_ASSERT_PTR_NOT_NULL(resObj->retStatus);
    CU_ASSERT_EQUAL(resObj->retStatus[0], WDMP_FAILURE);

    freeResObj(resObj);
    freeOperateRequest(req);
}

void test_handleMethodInvoke_missing_method_name()
{
    /* Valid JSON but no "method" field */
    char *b64 = testBase64Encode("{\"params\":{}}");
    set_req_t *req = buildOperateRequest(b64, WDMP_BASE64);
    free(b64);

    res_struct *resObj = allocResObj();
    handleMethodInvoke(req, resObj);

    CU_ASSERT_PTR_NOT_NULL(resObj->retStatus);
    CU_ASSERT_EQUAL(resObj->retStatus[0], WDMP_FAILURE);

    freeResObj(resObj);
    freeOperateRequest(req);
}

void test_handleMethodInvoke_empty_method_name()
{
    /* Method name is empty string */
    char *b64 = testBase64Encode("{\"method\":\"\"}");
    set_req_t *req = buildOperateRequest(b64, WDMP_BASE64);
    free(b64);

    res_struct *resObj = allocResObj();
    handleMethodInvoke(req, resObj);

    CU_ASSERT_PTR_NOT_NULL(resObj->retStatus);
    CU_ASSERT_EQUAL(resObj->retStatus[0], WDMP_FAILURE);

    freeResObj(resObj);
    freeOperateRequest(req);
}

void test_handleMethodInvoke_async_not_supported()
{
    /* rspDestination present means async - should fail */
    char *b64 = testBase64Encode("{\"method\":\"Device.Test()\",\"rspDestination\":\"some-dest\"}");
    set_req_t *req = buildOperateRequest(b64, WDMP_BASE64);
    free(b64);

    res_struct *resObj = allocResObj();
    handleMethodInvoke(req, resObj);

    CU_ASSERT_PTR_NOT_NULL(resObj->retStatus);
    CU_ASSERT_EQUAL(resObj->retStatus[0], WDMP_FAILURE);
    /* responseName should be the method name since parsing succeeded */
    CU_ASSERT_STRING_EQUAL(resObj->u.paramRes->params[0].name, "Device.Test()");

    freeResObj(resObj);
    freeOperateRequest(req);
}

void test_handleMethodInvoke_rbus_invoke_failure()
{
    /* Simulate rbus method invoke failure */
    char *b64 = testBase64Encode("{\"method\":\"Device.FailMethod()\"}");
    set_req_t *req = buildOperateRequest(b64, WDMP_BASE64);
    free(b64);

    mock_rbus_invoke_rc = RBUS_ERROR_DESTINATION_NOT_FOUND;
    mock_rbus_outParams = NULL;

    res_struct *resObj = allocResObj();
    handleMethodInvoke(req, resObj);

    CU_ASSERT_PTR_NOT_NULL(resObj->retStatus);
    CU_ASSERT_EQUAL(resObj->retStatus[0], WDMP_FAILURE);
    CU_ASSERT_STRING_EQUAL(resObj->u.paramRes->params[0].name, "Device.FailMethod()");
    /* Should have a base64-encoded error payload */
    CU_ASSERT_EQUAL(resObj->u.paramRes->params[0].type, WDMP_BASE64);
    CU_ASSERT_PTR_NOT_NULL(resObj->u.paramRes->params[0].value);

    freeResObj(resObj);
    freeOperateRequest(req);

    /* Reset mock */
    mock_rbus_invoke_rc = RBUS_ERROR_SUCCESS;
}

void test_handleMethodInvoke_success_no_params()
{
    /* Successful method invoke with no params and no outParams */
    char *b64 = testBase64Encode("{\"method\":\"Device.Reboot()\"}");
    set_req_t *req = buildOperateRequest(b64, WDMP_BASE64);
    free(b64);

    mock_rbus_invoke_rc = RBUS_ERROR_SUCCESS;
    mock_rbus_outParams = NULL;

    res_struct *resObj = allocResObj();
    handleMethodInvoke(req, resObj);

    CU_ASSERT_PTR_NOT_NULL(resObj->retStatus);
    CU_ASSERT_EQUAL(resObj->retStatus[0], WDMP_SUCCESS);
    CU_ASSERT_STRING_EQUAL(resObj->u.paramRes->params[0].name, "Device.Reboot()");
    CU_ASSERT_EQUAL(resObj->u.paramRes->params[0].type, WDMP_BASE64);
    CU_ASSERT_PTR_NOT_NULL(resObj->u.paramRes->params[0].value);

    freeResObj(resObj);
    freeOperateRequest(req);
}

void test_handleMethodInvoke_success_with_params()
{
    /* Successful method invoke with input params */
    char *b64 = testBase64Encode("{\"method\":\"Device.Run()\",\"params\":{\"timeout\":\"30\"}}");
    set_req_t *req = buildOperateRequest(b64, WDMP_BASE64);
    free(b64);

    mock_rbus_invoke_rc = RBUS_ERROR_SUCCESS;
    mock_rbus_outParams = NULL;

    res_struct *resObj = allocResObj();
    handleMethodInvoke(req, resObj);

    CU_ASSERT_PTR_NOT_NULL(resObj->retStatus);
    CU_ASSERT_EQUAL(resObj->retStatus[0], WDMP_SUCCESS);
    CU_ASSERT_STRING_EQUAL(resObj->u.paramRes->params[0].name, "Device.Run()");

    freeResObj(resObj);
    freeOperateRequest(req);
}

void test_handleMethodInvoke_success_with_outParams()
{
    /* Successful method invoke with outParams from rbus */
    char *b64 = testBase64Encode("{\"method\":\"Device.Query()\"}");
    set_req_t *req = buildOperateRequest(b64, WDMP_BASE64);
    free(b64);

    /* Create a simple outParams object */
    rbusObject_t outObj = NULL;
    rbusObject_Init(&outObj, NULL);
    rbusValue_t val = NULL;
    rbusValue_Init(&val);
    rbusValue_SetString(val, "result-value");
    rbusObject_SetValue(outObj, "output", val);
    rbusValue_Release(val);

    mock_rbus_invoke_rc = RBUS_ERROR_SUCCESS;
    mock_rbus_outParams = outObj;

    res_struct *resObj = allocResObj();
    handleMethodInvoke(req, resObj);

    CU_ASSERT_PTR_NOT_NULL(resObj->retStatus);
    CU_ASSERT_EQUAL(resObj->retStatus[0], WDMP_SUCCESS);
    CU_ASSERT_STRING_EQUAL(resObj->u.paramRes->params[0].name, "Device.Query()");
    CU_ASSERT_EQUAL(resObj->u.paramRes->params[0].type, WDMP_BASE64);
    CU_ASSERT_PTR_NOT_NULL(resObj->u.paramRes->params[0].value);

    freeResObj(resObj);
    freeOperateRequest(req);

    /* outParams is released inside handleMethodInvoke, don't double-release */
    mock_rbus_outParams = NULL;
}

void test_handleMethodInvoke_null_resObj()
{
    /* handleMethodInvoke should not crash with NULL resObj */
    char *b64 = testBase64Encode("{\"method\":\"Device.Test()\"}");
    set_req_t *req = buildOperateRequest(b64, WDMP_BASE64);
    free(b64);

    mock_rbus_invoke_rc = RBUS_ERROR_SUCCESS;
    mock_rbus_outParams = NULL;

    handleMethodInvoke(req, NULL);
    /* No crash = pass */
    CU_PASS("handleMethodInvoke with NULL resObj did not crash");

    freeOperateRequest(req);
}

void test_handleMethodInvoke_rspDestination_empty()
{
    /* Empty rspDestination should be treated as sync (not async) */
    char *b64 = testBase64Encode("{\"method\":\"Device.Sync()\",\"rspDestination\":\"\"}");
    set_req_t *req = buildOperateRequest(b64, WDMP_BASE64);
    free(b64);

    mock_rbus_invoke_rc = RBUS_ERROR_SUCCESS;
    mock_rbus_outParams = NULL;

    res_struct *resObj = allocResObj();
    handleMethodInvoke(req, resObj);

    CU_ASSERT_PTR_NOT_NULL(resObj->retStatus);
    CU_ASSERT_EQUAL(resObj->retStatus[0], WDMP_SUCCESS);
    CU_ASSERT_STRING_EQUAL(resObj->u.paramRes->params[0].name, "Device.Sync()");

    freeResObj(resObj);
    freeOperateRequest(req);
}

void test_handleMethodInvoke_invalid_rspDestination()
{
    /* rspDestination is not a string (number) - should fail */
    char *b64 = testBase64Encode("{\"method\":\"Device.Test()\",\"rspDestination\":123}");
    set_req_t *req = buildOperateRequest(b64, WDMP_BASE64);
    free(b64);

    res_struct *resObj = allocResObj();
    handleMethodInvoke(req, resObj);

    CU_ASSERT_PTR_NOT_NULL(resObj->retStatus);
    CU_ASSERT_EQUAL(resObj->retStatus[0], WDMP_FAILURE);

    freeResObj(resObj);
    freeOperateRequest(req);
}

void test_handleMethodInvoke_rbus_failure_with_outParams()
{
    /* rbus invoke fails but returns outParams (error details) */
    char *b64 = testBase64Encode("{\"method\":\"Device.ErrorMethod()\"}");
    set_req_t *req = buildOperateRequest(b64, WDMP_BASE64);
    free(b64);

    rbusObject_t outObj = NULL;
    rbusObject_Init(&outObj, NULL);
    rbusValue_t val = NULL;
    rbusValue_Init(&val);
    rbusValue_SetString(val, "detailed error info");
    rbusObject_SetValue(outObj, "errorDetail", val);
    rbusValue_Release(val);

    mock_rbus_invoke_rc = RBUS_ERROR_INVALID_INPUT;
    mock_rbus_outParams = outObj;

    res_struct *resObj = allocResObj();
    handleMethodInvoke(req, resObj);

    CU_ASSERT_PTR_NOT_NULL(resObj->retStatus);
    CU_ASSERT_EQUAL(resObj->retStatus[0], WDMP_FAILURE);
    CU_ASSERT_STRING_EQUAL(resObj->u.paramRes->params[0].name, "Device.ErrorMethod()");
    CU_ASSERT_EQUAL(resObj->u.paramRes->params[0].type, WDMP_BASE64);
    CU_ASSERT_PTR_NOT_NULL(resObj->u.paramRes->params[0].value);

    freeResObj(resObj);
    freeOperateRequest(req);
    mock_rbus_invoke_rc = RBUS_ERROR_SUCCESS;
    mock_rbus_outParams = NULL;
}

void test_handleMethodInvoke_rbus_generic_error()
{
    /* rbus invoke fails with generic error, no outParams */
    char *b64 = testBase64Encode("{\"method\":\"Device.GenericFail()\"}");
    set_req_t *req = buildOperateRequest(b64, WDMP_BASE64);
    free(b64);

    mock_rbus_invoke_rc = RBUS_ERROR_BUS_ERROR;
    mock_rbus_outParams = NULL;

    res_struct *resObj = allocResObj();
    handleMethodInvoke(req, resObj);

    CU_ASSERT_PTR_NOT_NULL(resObj->retStatus);
    CU_ASSERT_EQUAL(resObj->retStatus[0], WDMP_FAILURE);
    CU_ASSERT_EQUAL(resObj->u.paramRes->params[0].type, WDMP_BASE64);

    freeResObj(resObj);
    freeOperateRequest(req);
    mock_rbus_invoke_rc = RBUS_ERROR_SUCCESS;
}

void test_handleMethodInvoke_typed_params()
{
    /* Params with typed leaves: {"key":{"value":"42","dataType":1}} */
    char *b64 = testBase64Encode("{\"method\":\"Device.TypedParam()\",\"params\":{\"count\":{\"value\":\"42\",\"dataType\":1}}}");
    set_req_t *req = buildOperateRequest(b64, WDMP_BASE64);
    free(b64);

    mock_rbus_invoke_rc = RBUS_ERROR_SUCCESS;
    mock_rbus_outParams = NULL;

    res_struct *resObj = allocResObj();
    handleMethodInvoke(req, resObj);

    CU_ASSERT_PTR_NOT_NULL(resObj->retStatus);
    CU_ASSERT_EQUAL(resObj->retStatus[0], WDMP_SUCCESS);

    freeResObj(resObj);
    freeOperateRequest(req);
}

void test_handleMethodInvoke_bool_param()
{
    /* Params with boolean scalar */
    char *b64 = testBase64Encode("{\"method\":\"Device.BoolParam()\",\"params\":{\"enable\":true}}");
    set_req_t *req = buildOperateRequest(b64, WDMP_BASE64);
    free(b64);

    mock_rbus_invoke_rc = RBUS_ERROR_SUCCESS;
    mock_rbus_outParams = NULL;

    res_struct *resObj = allocResObj();
    handleMethodInvoke(req, resObj);

    CU_ASSERT_EQUAL(resObj->retStatus[0], WDMP_SUCCESS);

    freeResObj(resObj);
    freeOperateRequest(req);
}

void test_handleMethodInvoke_number_param()
{
    /* Params with numeric scalar */
    char *b64 = testBase64Encode("{\"method\":\"Device.NumParam()\",\"params\":{\"count\":42}}");
    set_req_t *req = buildOperateRequest(b64, WDMP_BASE64);
    free(b64);

    mock_rbus_invoke_rc = RBUS_ERROR_SUCCESS;
    mock_rbus_outParams = NULL;

    res_struct *resObj = allocResObj();
    handleMethodInvoke(req, resObj);

    CU_ASSERT_EQUAL(resObj->retStatus[0], WDMP_SUCCESS);

    freeResObj(resObj);
    freeOperateRequest(req);
}

void test_handleMethodInvoke_float_param()
{
    /* Params with floating-point numeric scalar */
    char *b64 = testBase64Encode("{\"method\":\"Device.FloatParam()\",\"params\":{\"ratio\":3.14}}");
    set_req_t *req = buildOperateRequest(b64, WDMP_BASE64);
    free(b64);

    mock_rbus_invoke_rc = RBUS_ERROR_SUCCESS;
    mock_rbus_outParams = NULL;

    res_struct *resObj = allocResObj();
    handleMethodInvoke(req, resObj);

    CU_ASSERT_EQUAL(resObj->retStatus[0], WDMP_SUCCESS);

    freeResObj(resObj);
    freeOperateRequest(req);
}

void test_handleMethodInvoke_null_scalar_param()
{
    /* Params with null scalar */
    char *b64 = testBase64Encode("{\"method\":\"Device.NullParam()\",\"params\":{\"val\":null}}");
    set_req_t *req = buildOperateRequest(b64, WDMP_BASE64);
    free(b64);

    mock_rbus_invoke_rc = RBUS_ERROR_SUCCESS;
    mock_rbus_outParams = NULL;

    res_struct *resObj = allocResObj();
    handleMethodInvoke(req, resObj);

    CU_ASSERT_EQUAL(resObj->retStatus[0], WDMP_SUCCESS);

    freeResObj(resObj);
    freeOperateRequest(req);
}

void test_handleMethodInvoke_outParams_various_types()
{
    /* Successful invoke with outParams containing various rbus types */
    char *b64 = testBase64Encode("{\"method\":\"Device.MultiOut()\"}");
    set_req_t *req = buildOperateRequest(b64, WDMP_BASE64);
    free(b64);

    rbusObject_t outObj = NULL;
    rbusObject_Init(&outObj, NULL);

    rbusValue_t vBool = NULL, vInt = NULL, vUint = NULL, vDouble = NULL;

    rbusValue_Init(&vBool);
    rbusValue_SetBoolean(vBool, true);
    rbusObject_SetValue(outObj, "boolProp", vBool);
    rbusValue_Release(vBool);

    rbusValue_Init(&vInt);
    rbusValue_SetInt32(vInt, -42);
    rbusObject_SetValue(outObj, "intProp", vInt);
    rbusValue_Release(vInt);

    rbusValue_Init(&vUint);
    rbusValue_SetUInt32(vUint, 100);
    rbusObject_SetValue(outObj, "uintProp", vUint);
    rbusValue_Release(vUint);

    rbusValue_Init(&vDouble);
    rbusValue_SetDouble(vDouble, 3.14);
    rbusObject_SetValue(outObj, "doubleProp", vDouble);
    rbusValue_Release(vDouble);

    mock_rbus_invoke_rc = RBUS_ERROR_SUCCESS;
    mock_rbus_outParams = outObj;

    res_struct *resObj = allocResObj();
    handleMethodInvoke(req, resObj);

    CU_ASSERT_EQUAL(resObj->retStatus[0], WDMP_SUCCESS);
    CU_ASSERT_PTR_NOT_NULL(resObj->u.paramRes->params[0].value);

    freeResObj(resObj);
    freeOperateRequest(req);
    mock_rbus_outParams = NULL;
}

/*----------------------------------------------------------------------------*/
/*                    Tests for static helper functions                         */
/*----------------------------------------------------------------------------*/

void test_buildErrorObject_basic()
{
    char *out = buildErrorObject(METHOD_ERR_INTERNAL, "something broke");
    CU_ASSERT_PTR_NOT_NULL(out);

    cJSON *root = cJSON_Parse(out);
    CU_ASSERT_PTR_NOT_NULL(root);
    cJSON *err = cJSON_GetObjectItem(root, "error");
    CU_ASSERT_PTR_NOT_NULL(err);
    CU_ASSERT_EQUAL(cJSON_GetObjectItem(err, "code")->valueint, METHOD_ERR_INTERNAL);
    CU_ASSERT_STRING_EQUAL(cJSON_GetObjectItem(err, "data")->valuestring, "something broke");

    cJSON_Delete(root);
    free(out);
}

void test_buildErrorObject_null_data()
{
    char *out = buildErrorObject(METHOD_ERR_PARSE, NULL);
    CU_ASSERT_PTR_NOT_NULL(out);

    cJSON *root = cJSON_Parse(out);
    CU_ASSERT_PTR_NOT_NULL(root);
    cJSON *err = cJSON_GetObjectItem(root, "error");
    CU_ASSERT_PTR_NOT_NULL(err);
    CU_ASSERT_EQUAL(cJSON_GetObjectItem(err, "code")->valueint, METHOD_ERR_PARSE);
    CU_ASSERT_PTR_NULL(cJSON_GetObjectItem(err, "data"));

    cJSON_Delete(root);
    free(out);
}

void test_buildErrorObjectFromJson_with_data()
{
    cJSON *dataObj = cJSON_CreateObject();
    cJSON_AddStringToObject(dataObj, "detail", "test error detail");
    cJSON_AddNumberToObject(dataObj, "count", 5);

    char *out = buildErrorObjectFromJson(METHOD_ERR_INVALID_REQUEST, dataObj);
    CU_ASSERT_PTR_NOT_NULL(out);

    cJSON *root = cJSON_Parse(out);
    CU_ASSERT_PTR_NOT_NULL(root);
    cJSON *err = cJSON_GetObjectItem(root, "error");
    CU_ASSERT_PTR_NOT_NULL(err);
    CU_ASSERT_EQUAL(cJSON_GetObjectItem(err, "code")->valueint, METHOD_ERR_INVALID_REQUEST);
    cJSON *data = cJSON_GetObjectItem(err, "data");
    CU_ASSERT_PTR_NOT_NULL(data);
    CU_ASSERT_STRING_EQUAL(cJSON_GetObjectItem(data, "detail")->valuestring, "test error detail");
    CU_ASSERT_EQUAL(cJSON_GetObjectItem(data, "count")->valueint, 5);

    cJSON_Delete(root);
    cJSON_Delete(dataObj);
    free(out);
}

void test_buildErrorObjectFromJson_null_data()
{
    char *out = buildErrorObjectFromJson(METHOD_ERR_METHOD_NOT_FOUND, NULL);
    CU_ASSERT_PTR_NOT_NULL(out);

    cJSON *root = cJSON_Parse(out);
    CU_ASSERT_PTR_NOT_NULL(root);
    cJSON *err = cJSON_GetObjectItem(root, "error");
    CU_ASSERT_PTR_NOT_NULL(err);
    CU_ASSERT_EQUAL(cJSON_GetObjectItem(err, "code")->valueint, METHOD_ERR_METHOD_NOT_FOUND);

    cJSON_Delete(root);
    free(out);
}

void test_buildMethodResponse_with_message()
{
    cJSON *msgObj = cJSON_CreateObject();
    cJSON_AddStringToObject(msgObj, "result", "ok");

    char *out = buildMethodResponse("Device.TestMethod()", 200, msgObj);
    CU_ASSERT_PTR_NOT_NULL(out);

    cJSON *root = cJSON_Parse(out);
    CU_ASSERT_PTR_NOT_NULL(root);
    CU_ASSERT_EQUAL(cJSON_GetObjectItem(root, "statusCode")->valueint, 200);
    cJSON *params = cJSON_GetObjectItem(root, "parameters");
    CU_ASSERT_PTR_NOT_NULL(params);
    CU_ASSERT_EQUAL(cJSON_GetArraySize(params), 1);
    cJSON *entry = cJSON_GetArrayItem(params, 0);
    CU_ASSERT_STRING_EQUAL(cJSON_GetObjectItem(entry, "name")->valuestring, "Device.TestMethod()");
    /* message should be non-empty base64 */
    CU_ASSERT_PTR_NOT_NULL(cJSON_GetObjectItem(entry, "message")->valuestring);
    CU_ASSERT_TRUE(strlen(cJSON_GetObjectItem(entry, "message")->valuestring) > 0);

    cJSON_Delete(root);
    cJSON_Delete(msgObj);
    free(out);
}

void test_buildMethodResponse_null_name()
{
    cJSON *msgObj = cJSON_CreateObject();
    cJSON_AddStringToObject(msgObj, "status", "done");

    char *out = buildMethodResponse(NULL, 500, msgObj);
    CU_ASSERT_PTR_NOT_NULL(out);

    cJSON *root = cJSON_Parse(out);
    CU_ASSERT_PTR_NOT_NULL(root);
    cJSON *params = cJSON_GetObjectItem(root, "parameters");
    cJSON *entry = cJSON_GetArrayItem(params, 0);
    /* Should fallback to RDK.Operate */
    CU_ASSERT_STRING_EQUAL(cJSON_GetObjectItem(entry, "name")->valuestring, RDK_OPERATE_PARAM);

    cJSON_Delete(root);
    cJSON_Delete(msgObj);
    free(out);
}

void test_buildMethodResponse_null_message()
{
    char *out = buildMethodResponse("Device.NoMsg()", 200, NULL);
    CU_ASSERT_PTR_NOT_NULL(out);

    cJSON *root = cJSON_Parse(out);
    CU_ASSERT_PTR_NOT_NULL(root);
    cJSON *params = cJSON_GetObjectItem(root, "parameters");
    cJSON *entry = cJSON_GetArrayItem(params, 0);
    CU_ASSERT_STRING_EQUAL(cJSON_GetObjectItem(entry, "name")->valuestring, "Device.NoMsg()");
    /* message should be empty string when no messageObj */
    CU_ASSERT_STRING_EQUAL(cJSON_GetObjectItem(entry, "message")->valuestring, "");

    cJSON_Delete(root);
    free(out);
}

void test_wdmpToRbusType_mappings()
{
    CU_ASSERT_EQUAL(wdmpToRbusType(WDMP_INT), RBUS_INT32);
    CU_ASSERT_EQUAL(wdmpToRbusType(WDMP_UINT), RBUS_UINT32);
    CU_ASSERT_EQUAL(wdmpToRbusType(WDMP_BOOLEAN), RBUS_BOOLEAN);
    CU_ASSERT_EQUAL(wdmpToRbusType(WDMP_LONG), RBUS_INT64);
    CU_ASSERT_EQUAL(wdmpToRbusType(WDMP_ULONG), RBUS_UINT64);
    CU_ASSERT_EQUAL(wdmpToRbusType(WDMP_FLOAT), RBUS_SINGLE);
    CU_ASSERT_EQUAL(wdmpToRbusType(WDMP_DOUBLE), RBUS_DOUBLE);
    CU_ASSERT_EQUAL(wdmpToRbusType(WDMP_DATETIME), RBUS_DATETIME);
    CU_ASSERT_EQUAL(wdmpToRbusType(WDMP_BASE64), RBUS_BYTES);
    CU_ASSERT_EQUAL(wdmpToRbusType(WDMP_BYTE), RBUS_BYTES);
    CU_ASSERT_EQUAL(wdmpToRbusType(WDMP_NONE), RBUS_NONE);
    CU_ASSERT_EQUAL(wdmpToRbusType(WDMP_STRING), RBUS_STRING);
    /* Unknown type falls to default STRING */
    CU_ASSERT_EQUAL(wdmpToRbusType(9999), RBUS_STRING);
}

void test_mapRbusErrorToMethodError_mappings()
{
    CU_ASSERT_EQUAL(mapRbusErrorToMethodError(RBUS_ERROR_DESTINATION_NOT_FOUND), METHOD_ERR_METHOD_NOT_FOUND);
    CU_ASSERT_EQUAL(mapRbusErrorToMethodError(RBUS_ERROR_INVALID_INPUT), METHOD_ERR_INVALID_PARAMS);
    CU_ASSERT_EQUAL(mapRbusErrorToMethodError(RBUS_ERROR_BUS_ERROR), METHOD_ERR_INTERNAL);
    CU_ASSERT_EQUAL(mapRbusErrorToMethodError(RBUS_ERROR_TIMEOUT), METHOD_ERR_INTERNAL);
}

void test_base64Decode_null_input()
{
    size_t len = 0;
    CU_ASSERT_PTR_NULL(base64Decode(NULL, &len));
}

void test_base64Decode_empty_input()
{
    size_t len = 0;
    CU_ASSERT_PTR_NULL(base64Decode("", &len));
}

void test_base64Decode_valid()
{
    size_t len = 0;
    char *out = base64Decode("aGVsbG8=", &len);  /* "hello" */
    CU_ASSERT_PTR_NOT_NULL(out);
    CU_ASSERT_EQUAL(len, 5);
    CU_ASSERT_STRING_EQUAL(out, "hello");
    free(out);
}

void test_base64Encode_null_input()
{
    CU_ASSERT_PTR_NULL(base64Encode(NULL));
}

void test_base64Encode_valid()
{
    char *out = base64Encode("hello");
    CU_ASSERT_PTR_NOT_NULL(out);
    CU_ASSERT_STRING_EQUAL(out, "aGVsbG8=");
    free(out);
}

void test_rbusObjectToJson_null_inputs()
{
    CU_ASSERT_EQUAL(rbusObjectToJson(NULL, NULL), -1);

    cJSON *json = cJSON_CreateObject();
    CU_ASSERT_EQUAL(rbusObjectToJson(NULL, json), -1);
    cJSON_Delete(json);
}

void test_rbusObjectToJson_empty_object()
{
    rbusObject_t obj = NULL;
    rbusObject_Init(&obj, NULL);

    cJSON *json = cJSON_CreateObject();
    CU_ASSERT_EQUAL(rbusObjectToJson(obj, json), 0);

    cJSON_Delete(json);
    rbusObject_Release(obj);
}

void test_rbusObjectToJson_with_properties()
{
    rbusObject_t obj = NULL;
    rbusObject_Init(&obj, NULL);

    rbusValue_t vStr = NULL;
    rbusValue_Init(&vStr);
    rbusValue_SetString(vStr, "test-value");
    rbusObject_SetValue(obj, "key1", vStr);
    rbusValue_Release(vStr);

    rbusValue_t vInt = NULL;
    rbusValue_Init(&vInt);
    rbusValue_SetInt32(vInt, 42);
    rbusObject_SetValue(obj, "key2", vInt);
    rbusValue_Release(vInt);

    cJSON *json = cJSON_CreateObject();
    CU_ASSERT_EQUAL(rbusObjectToJson(obj, json), 0);
    CU_ASSERT_STRING_EQUAL(cJSON_GetObjectItem(json, "key1")->valuestring, "test-value");
    CU_ASSERT_EQUAL(cJSON_GetObjectItem(json, "key2")->valueint, 42);

    cJSON_Delete(json);
    rbusObject_Release(obj);
}

void test_rbusValueToJson_boolean()
{
    rbusValue_t val = NULL;
    rbusValue_Init(&val);
    rbusValue_SetBoolean(val, true);
    cJSON *j = rbusValueToJson(val);
    CU_ASSERT_PTR_NOT_NULL(j);
    CU_ASSERT_TRUE(cJSON_IsTrue(j));
    cJSON_Delete(j);
    rbusValue_Release(val);
}

void test_rbusValueToJson_int_types()
{
    rbusValue_t val = NULL;

    /* INT8 */
    rbusValue_Init(&val);
    rbusValue_SetInt8(val, -5);
    cJSON *j = rbusValueToJson(val);
    CU_ASSERT_PTR_NOT_NULL(j);
    CU_ASSERT_EQUAL(j->valueint, -5);
    cJSON_Delete(j);
    rbusValue_Release(val);

    /* UINT8 */
    rbusValue_Init(&val);
    rbusValue_SetUInt8(val, 200);
    j = rbusValueToJson(val);
    CU_ASSERT_PTR_NOT_NULL(j);
    CU_ASSERT_EQUAL(j->valueint, 200);
    cJSON_Delete(j);
    rbusValue_Release(val);

    /* INT16 */
    rbusValue_Init(&val);
    rbusValue_SetInt16(val, -1000);
    j = rbusValueToJson(val);
    CU_ASSERT_PTR_NOT_NULL(j);
    CU_ASSERT_EQUAL(j->valueint, -1000);
    cJSON_Delete(j);
    rbusValue_Release(val);

    /* UINT16 */
    rbusValue_Init(&val);
    rbusValue_SetUInt16(val, 50000);
    j = rbusValueToJson(val);
    CU_ASSERT_PTR_NOT_NULL(j);
    CU_ASSERT_EQUAL(j->valueint, 50000);
    cJSON_Delete(j);
    rbusValue_Release(val);

    /* INT64 */
    rbusValue_Init(&val);
    rbusValue_SetInt64(val, -123456789LL);
    j = rbusValueToJson(val);
    CU_ASSERT_PTR_NOT_NULL(j);
    cJSON_Delete(j);
    rbusValue_Release(val);

    /* UINT64 */
    rbusValue_Init(&val);
    rbusValue_SetUInt64(val, 123456789ULL);
    j = rbusValueToJson(val);
    CU_ASSERT_PTR_NOT_NULL(j);
    cJSON_Delete(j);
    rbusValue_Release(val);
}

void test_rbusValueToJson_single()
{
    rbusValue_t val = NULL;
    rbusValue_Init(&val);
    rbusValue_SetSingle(val, 2.5f);
    cJSON *j = rbusValueToJson(val);
    CU_ASSERT_PTR_NOT_NULL(j);
    CU_ASSERT_TRUE(cJSON_IsNumber(j));
    cJSON_Delete(j);
    rbusValue_Release(val);
}

void test_rbusValueToJson_string_json()
{
    /* String value that is valid JSON should be parsed */
    rbusValue_t val = NULL;
    rbusValue_Init(&val);
    rbusValue_SetString(val, "{\"nested\":\"value\"}");
    cJSON *j = rbusValueToJson(val);
    CU_ASSERT_PTR_NOT_NULL(j);
    CU_ASSERT_TRUE(cJSON_IsObject(j));
    CU_ASSERT_STRING_EQUAL(cJSON_GetObjectItem(j, "nested")->valuestring, "value");
    cJSON_Delete(j);
    rbusValue_Release(val);
}

void test_rbusValueToJson_string_plain()
{
    /* Plain string (not JSON) */
    rbusValue_t val = NULL;
    rbusValue_Init(&val);
    rbusValue_SetString(val, "plain text");
    cJSON *j = rbusValueToJson(val);
    CU_ASSERT_PTR_NOT_NULL(j);
    CU_ASSERT_TRUE(cJSON_IsString(j));
    CU_ASSERT_STRING_EQUAL(j->valuestring, "plain text");
    cJSON_Delete(j);
    rbusValue_Release(val);
}

void test_rbusValueToJson_nested_object()
{
    /* RBUS_OBJECT type */
    rbusObject_t child = NULL;
    rbusObject_Init(&child, NULL);
    rbusValue_t sv = NULL;
    rbusValue_Init(&sv);
    rbusValue_SetString(sv, "inner");
    rbusObject_SetValue(child, "key", sv);
    rbusValue_Release(sv);

    rbusValue_t val = NULL;
    rbusValue_Init(&val);
    rbusValue_SetObject(val, child);

    cJSON *j = rbusValueToJson(val);
    CU_ASSERT_PTR_NOT_NULL(j);
    CU_ASSERT_TRUE(cJSON_IsObject(j));
    CU_ASSERT_STRING_EQUAL(cJSON_GetObjectItem(j, "key")->valuestring, "inner");

    cJSON_Delete(j);
    rbusValue_Release(val);
    rbusObject_Release(child);
}

void test_jsonScalarToRbusValue_string()
{
    cJSON *val = cJSON_CreateString("test");
    rbusValue_t rv = NULL;
    CU_ASSERT_EQUAL(jsonScalarToRbusValue(val, &rv), 0);
    CU_ASSERT_PTR_NOT_NULL(rv);
    CU_ASSERT_EQUAL(rbusValue_GetType(rv), RBUS_STRING);
    rbusValue_Release(rv);
    cJSON_Delete(val);
}

void test_jsonScalarToRbusValue_bool()
{
    cJSON *val = cJSON_CreateTrue();
    rbusValue_t rv = NULL;
    CU_ASSERT_EQUAL(jsonScalarToRbusValue(val, &rv), 0);
    CU_ASSERT_PTR_NOT_NULL(rv);
    CU_ASSERT_EQUAL(rbusValue_GetType(rv), RBUS_BOOLEAN);
    CU_ASSERT_TRUE(rbusValue_GetBoolean(rv));
    rbusValue_Release(rv);
    cJSON_Delete(val);
}

void test_jsonScalarToRbusValue_integer()
{
    cJSON *val = cJSON_CreateNumber(42);
    rbusValue_t rv = NULL;
    CU_ASSERT_EQUAL(jsonScalarToRbusValue(val, &rv), 0);
    CU_ASSERT_PTR_NOT_NULL(rv);
    CU_ASSERT_EQUAL(rbusValue_GetInt32(rv), 42);
    rbusValue_Release(rv);
    cJSON_Delete(val);
}

void test_jsonScalarToRbusValue_double()
{
    cJSON *val = cJSON_CreateNumber(3.14);
    rbusValue_t rv = NULL;
    CU_ASSERT_EQUAL(jsonScalarToRbusValue(val, &rv), 0);
    CU_ASSERT_PTR_NOT_NULL(rv);
    rbusValue_Release(rv);
    cJSON_Delete(val);
}

void test_jsonScalarToRbusValue_null()
{
    cJSON *val = cJSON_CreateNull();
    rbusValue_t rv = NULL;
    CU_ASSERT_EQUAL(jsonScalarToRbusValue(val, &rv), 0);
    CU_ASSERT_PTR_NOT_NULL(rv);
    rbusValue_Release(rv);
    cJSON_Delete(val);
}

void test_jsonScalarToRbusValue_array_fails()
{
    /* Array type should fail */
    cJSON *val = cJSON_CreateArray();
    rbusValue_t rv = NULL;
    CU_ASSERT_EQUAL(jsonScalarToRbusValue(val, &rv), -1);
    CU_ASSERT_PTR_NULL(rv);
    cJSON_Delete(val);
}

void test_handleMethodInvoke_typed_param_conversion_fails()
{
    /* Typed leaf with dataType=int but a non-numeric value -> rbusValue_SetFromString fails */
    char *b64 = testBase64Encode("{\"method\":\"Device.BadInt()\",\"params\":{\"count\":{\"value\":\"notanumber\",\"dataType\":1}}}");
    set_req_t *req = buildOperateRequest(b64, WDMP_BASE64);
    free(b64);

    res_struct *resObj = allocResObj();
    handleMethodInvoke(req, resObj);

    CU_ASSERT_PTR_NOT_NULL(resObj->retStatus);
    CU_ASSERT_EQUAL(resObj->retStatus[0], WDMP_FAILURE);
    CU_ASSERT_STRING_EQUAL(resObj->u.paramRes->params[0].name, RDK_OPERATE_PARAM);

    freeResObj(resObj);
    freeOperateRequest(req);
}

void test_handleMethodInvoke_typed_param_non_string_value()
{
    /* Typed leaf where "value" itself is a JSON number, not a string -> jsonLeafToRbusValue rejects it */
    char *b64 = testBase64Encode("{\"method\":\"Device.BadValue()\",\"params\":{\"count\":{\"value\":123,\"dataType\":0}}}");
    set_req_t *req = buildOperateRequest(b64, WDMP_BASE64);
    free(b64);

    res_struct *resObj = allocResObj();
    handleMethodInvoke(req, resObj);

    CU_ASSERT_PTR_NOT_NULL(resObj->retStatus);
    CU_ASSERT_EQUAL(resObj->retStatus[0], WDMP_FAILURE);

    freeResObj(resObj);
    freeOperateRequest(req);
}

/*----------------------------------------------------------------------------*/
/*                              Suite Setup                                    */
/*----------------------------------------------------------------------------*/

void add_suites(CU_pSuite *suite)
{
    *suite = CU_add_suite("webpa_method_tests", NULL, NULL);

    /* isMethodInvokeRequest tests */
    CU_add_test(*suite, "test_isMethodInvokeRequest_null_input", test_isMethodInvokeRequest_null_input);
    CU_add_test(*suite, "test_isMethodInvokeRequest_null_param", test_isMethodInvokeRequest_null_param);
    CU_add_test(*suite, "test_isMethodInvokeRequest_null_param_name", test_isMethodInvokeRequest_null_param_name);
    CU_add_test(*suite, "test_isMethodInvokeRequest_non_matching_name", test_isMethodInvokeRequest_non_matching_name);
    CU_add_test(*suite, "test_isMethodInvokeRequest_valid", test_isMethodInvokeRequest_valid);

    /* handleMethodInvoke tests */
    CU_add_test(*suite, "test_handleMethodInvoke_invalid_paramCnt", test_handleMethodInvoke_invalid_paramCnt);
    CU_add_test(*suite, "test_handleMethodInvoke_wrong_datatype", test_handleMethodInvoke_wrong_datatype);
    CU_add_test(*suite, "test_handleMethodInvoke_null_value", test_handleMethodInvoke_null_value);
    CU_add_test(*suite, "test_handleMethodInvoke_invalid_base64", test_handleMethodInvoke_invalid_base64);
    CU_add_test(*suite, "test_handleMethodInvoke_invalid_json", test_handleMethodInvoke_invalid_json);
    CU_add_test(*suite, "test_handleMethodInvoke_missing_method_name", test_handleMethodInvoke_missing_method_name);
    CU_add_test(*suite, "test_handleMethodInvoke_empty_method_name", test_handleMethodInvoke_empty_method_name);
    CU_add_test(*suite, "test_handleMethodInvoke_async_not_supported", test_handleMethodInvoke_async_not_supported);
    CU_add_test(*suite, "test_handleMethodInvoke_rbus_invoke_failure", test_handleMethodInvoke_rbus_invoke_failure);
    CU_add_test(*suite, "test_handleMethodInvoke_success_no_params", test_handleMethodInvoke_success_no_params);
    CU_add_test(*suite, "test_handleMethodInvoke_success_with_params", test_handleMethodInvoke_success_with_params);
    CU_add_test(*suite, "test_handleMethodInvoke_success_with_outParams", test_handleMethodInvoke_success_with_outParams);
    CU_add_test(*suite, "test_handleMethodInvoke_null_resObj", test_handleMethodInvoke_null_resObj);
    CU_add_test(*suite, "test_handleMethodInvoke_rspDestination_empty", test_handleMethodInvoke_rspDestination_empty);
    CU_add_test(*suite, "test_handleMethodInvoke_invalid_rspDestination", test_handleMethodInvoke_invalid_rspDestination);
    CU_add_test(*suite, "test_handleMethodInvoke_rbus_failure_with_outParams", test_handleMethodInvoke_rbus_failure_with_outParams);
    CU_add_test(*suite, "test_handleMethodInvoke_rbus_generic_error", test_handleMethodInvoke_rbus_generic_error);
    CU_add_test(*suite, "test_handleMethodInvoke_typed_params", test_handleMethodInvoke_typed_params);
    CU_add_test(*suite, "test_handleMethodInvoke_bool_param", test_handleMethodInvoke_bool_param);
    CU_add_test(*suite, "test_handleMethodInvoke_number_param", test_handleMethodInvoke_number_param);
    CU_add_test(*suite, "test_handleMethodInvoke_float_param", test_handleMethodInvoke_float_param);
    CU_add_test(*suite, "test_handleMethodInvoke_null_scalar_param", test_handleMethodInvoke_null_scalar_param);
    CU_add_test(*suite, "test_handleMethodInvoke_outParams_various_types", test_handleMethodInvoke_outParams_various_types);

    /* Static helper function tests */
    CU_add_test(*suite, "test_buildErrorObject_basic", test_buildErrorObject_basic);
    CU_add_test(*suite, "test_buildErrorObject_null_data", test_buildErrorObject_null_data);
    CU_add_test(*suite, "test_buildErrorObjectFromJson_with_data", test_buildErrorObjectFromJson_with_data);
    CU_add_test(*suite, "test_buildErrorObjectFromJson_null_data", test_buildErrorObjectFromJson_null_data);
    CU_add_test(*suite, "test_buildMethodResponse_with_message", test_buildMethodResponse_with_message);
    CU_add_test(*suite, "test_buildMethodResponse_null_name", test_buildMethodResponse_null_name);
    CU_add_test(*suite, "test_buildMethodResponse_null_message", test_buildMethodResponse_null_message);
    CU_add_test(*suite, "test_wdmpToRbusType_mappings", test_wdmpToRbusType_mappings);
    CU_add_test(*suite, "test_mapRbusErrorToMethodError_mappings", test_mapRbusErrorToMethodError_mappings);
    CU_add_test(*suite, "test_base64Decode_null_input", test_base64Decode_null_input);
    CU_add_test(*suite, "test_base64Decode_empty_input", test_base64Decode_empty_input);
    CU_add_test(*suite, "test_base64Decode_valid", test_base64Decode_valid);
    CU_add_test(*suite, "test_base64Encode_null_input", test_base64Encode_null_input);
    CU_add_test(*suite, "test_base64Encode_valid", test_base64Encode_valid);
    CU_add_test(*suite, "test_rbusObjectToJson_null_inputs", test_rbusObjectToJson_null_inputs);
    CU_add_test(*suite, "test_rbusObjectToJson_empty_object", test_rbusObjectToJson_empty_object);
    CU_add_test(*suite, "test_rbusObjectToJson_with_properties", test_rbusObjectToJson_with_properties);
    CU_add_test(*suite, "test_rbusValueToJson_boolean", test_rbusValueToJson_boolean);
    CU_add_test(*suite, "test_rbusValueToJson_int_types", test_rbusValueToJson_int_types);
    CU_add_test(*suite, "test_rbusValueToJson_single", test_rbusValueToJson_single);
    CU_add_test(*suite, "test_rbusValueToJson_string_json", test_rbusValueToJson_string_json);
    CU_add_test(*suite, "test_rbusValueToJson_string_plain", test_rbusValueToJson_string_plain);
    CU_add_test(*suite, "test_rbusValueToJson_nested_object", test_rbusValueToJson_nested_object);
    CU_add_test(*suite, "test_jsonScalarToRbusValue_string", test_jsonScalarToRbusValue_string);
    CU_add_test(*suite, "test_jsonScalarToRbusValue_bool", test_jsonScalarToRbusValue_bool);
    CU_add_test(*suite, "test_jsonScalarToRbusValue_integer", test_jsonScalarToRbusValue_integer);
    CU_add_test(*suite, "test_jsonScalarToRbusValue_double", test_jsonScalarToRbusValue_double);
    CU_add_test(*suite, "test_jsonScalarToRbusValue_null", test_jsonScalarToRbusValue_null);
    CU_add_test(*suite, "test_jsonScalarToRbusValue_array_fails", test_jsonScalarToRbusValue_array_fails);
    CU_add_test(*suite, "test_handleMethodInvoke_typed_param_conversion_fails", test_handleMethodInvoke_typed_param_conversion_fails);
    CU_add_test(*suite, "test_handleMethodInvoke_typed_param_non_string_value", test_handleMethodInvoke_typed_param_non_string_value);
}

int main(int argc, char *argv[])
{
    unsigned rv = 1;
    CU_pSuite suite = NULL;

    (void)argc;
    (void)argv;

    if (CUE_SUCCESS == CU_initialize_registry())
    {
        add_suites(&suite);

        if (NULL != suite)
        {
            CU_basic_set_mode(CU_BRM_VERBOSE);
            CU_basic_run_tests();
            printf("\n");
            CU_basic_show_failures(CU_get_failure_list());
            printf("\n\n");
            rv = CU_get_number_of_tests_failed();
        }
        CU_cleanup_registry();
    }
    return rv;
}
