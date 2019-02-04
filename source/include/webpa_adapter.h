/**
 * @file wal.h
 *
 * @description This header defines the WebPA Abstraction APIs
 *
 * Copyright (c) 2015  Comcast
 */

#ifndef _WEBPA_ADAPTER_H_
#define _WEBPA_ADAPTER_H_

#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <wdmp-c.h>
#include <cimplog.h>

#define WAL_FREE(__x__) if(__x__ != NULL) { free((void*)(__x__)); __x__ = NULL;} else {printf("Trying to free null pointer\n");}

#ifndef TEST
#define FOREVER()   1
#else
extern int numLoops;
#define FOREVER()   numLoops--
#endif

#define WEBPA_COMPONENT_NAME               "com.cisco.spvtg.ccsp.webpaagent"
#define LOGGING_MODULE                     "WEBPA"
/**
 * @brief Enables or disables debug logs.
 */
#define WalError(...)                   cimplog_error(LOGGING_MODULE, __VA_ARGS__)
#define WalInfo(...)                    cimplog_info(LOGGING_MODULE, __VA_ARGS__)
#define WalPrint(...)                   cimplog_debug(LOGGING_MODULE, __VA_ARGS__)

#define OnboardLog(...)                 onboarding_log(LOGGING_MODULE, __VA_ARGS__)

/**
 * @brief Set operations supported by WebPA.
 */
typedef enum
{
    WEBPA_SET = 0,
    WEBPA_ATOMIC_SET,
    WEBPA_ATOMIC_SET_XPC
} WEBPA_SET_TYPE;

/**
 * @brief Initializes the Message Bus and registers WebPA component with the stack.
 *
 * @param[in] name WebPA Component Name.
 */
WDMP_STATUS msgBusInit(const char *name);

/**
 * @brief waitForOperationalReadyCondition wait till all dependent components
 * required for being operational are initialized
 */
int waitForOperationalReadyCondition();

/**
 * @brief initComponentCaching Initalize component caching
 */
void initComponentCaching();

/*
* @brief initApplyWiFiSettings intializes apply wify settings thread
*/
void initApplyWiFiSettings();

/**
 * @brief Function to keep listening for parodus downstream messages
 */
void parodus_receive_wait();

/**
 * @brief displays the current time.
 * @param[in] timer current time.
 */
void getCurrentTime(struct timespec *timer);

/*
 * @brief displays the current time in microseconds.
 * @param[in] timer current time.
 */
uint64_t getCurrentTimeInMicroSeconds(struct timespec *timer);

/*
 * @brief Returns the time difference between start and end time of request processed.
 * @param[in] starttime starting time of request processed.
 * @param[in] finishtime ending time of request processed.
 * @return msec.
 */
long timeValDiff(struct timespec *starttime, struct timespec *finishtime);

/**
 * @brief processRequest processes the request and returns response payload
 *
 * @param[in] reqPayload input request to process
 * @param[in] resPayload retuns response payload
 */
void processRequest(char *reqPayload, char *transactionId, char **resPayload);

/**
 * @brief getValues Returns the parameter values from stack for GET request
 *
 * @param[in] paramName parameter Name
 * @param[in] paramCount Number of parameters
 * @param[in] index parameter vallue array index
 * @param[out] timeSpan timing_values for each component.
 * @param[out] paramArr parameter value Array
 * @param[out] retValCount Number of parameters returned from stack
 * @param[out] retStatus Returns status
 */
void getValues(const char *paramName[], const unsigned int paramCount, int index, money_trace_spans *timeSpan, param_t ***paramArr, int *retValCount, WDMP_STATUS *retStatus);

/**
 * @brief getAttributes Returns the parameter Attributes from stack for GET-ATTRIBUTES request
 *
 * @param[in] paramName parameter Name
 * @param[in] paramCount Number of parameters
 * @param[out] timeSpan timing_values for each component.
 * @param[out] attr parameter attribute Array
 * @param[out] retValCount Number of parameters returned from stack
 * @param[out] retStatus Returns status
 */
void getAttributes(const char *paramName[], const unsigned int paramCount, money_trace_spans *timeSpan, param_t **attr, int *retAttrCount, WDMP_STATUS *retStatus);

/**
 * @brief setValues interface sets the parameter value.
 *
 * @param[in] paramVal List of Parameter name/value pairs.
 * @param[in] paramCount Number of parameters.
 * @param[in] setType Flag to specify the type of set operation.
 * @param[out] timeSpan timing_values for each component. 
 * @param[out] retStatus Returns status
 */
void setValues(const param_t paramVal[], const unsigned int paramCount, const WEBPA_SET_TYPE setType, char *transactionId, money_trace_spans *timeSpan, WDMP_STATUS *retStatus);

/**
 * @brief setAttributes Returns the status of parameter from stack for SET-ATTRIBUTES request
 *
 * @param[in] attArr parameter attributes Array
 * @param[in] paramCount Number of parameters
 * @param[out] timeSpan timing_values for each component.
 * @param[out] retStatus Returns status
 */
void setAttributes(param_t *attArr, const unsigned int paramCount, money_trace_spans *timeSpan, WDMP_STATUS *retStatus);

/**
 * @brief addRowTable adds data to the tables and returns new row added on success
 *
 * param[in] objectName table name
 * param[in] list Parameter name/value pairs
 * param[out] retObject return new row added
 * param[out] retStatus Returns status
 */
void addRowTable(char *objectName, TableData *list,char **retObject, WDMP_STATUS *retStatus);

/**
 * @brief deleteRowTable deletes row from a dynamic table
 *
 * param[in] objectName row to delete
 * param[out] retStatus Returns status
 */
void deleteRowTable(char *object,WDMP_STATUS *retStatus);

/**
 * @brief replaceTable replaces existing data of table with provided data
 *
 * param[in] objectName table name
 * param[in] list Parameter name/value pairs
 * param[in] paramcount count of rows
 * param[out] retStatus Returns status
 */
void replaceTable(char *objectName,TableData * list,unsigned int paramcount,WDMP_STATUS *retStatus);

/*
 * @brief To initiate notification Task handling
 */
void initNotifyTask(int status);

void sendNotification(char *payload, char *source, char *destination);

char* parsePayloadForStatus(char *payload);

#endif /* _WEBPA_ADAPTER_H_ */
