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

#define WAL_FREE(__x__) if(__x__ != NULL) { free((void*)(__x__)); __x__ = NULL;} else {printf("Trying to free null pointer\n");}

#define WEBPA_COMPONENT_NAME               "com.cisco.spvtg.ccsp.webpaagent"
/**
 * @brief Enables or disables debug logs.
 */
#define WEBPA_LOG_ERROR                 0
#define WEBPA_LOG_INFO                  1
#define WEBPA_LOG_PRINT                 2
#define WalError(...)                   _WEBPA_LOG(WEBPA_LOG_ERROR, __VA_ARGS__)
#define WalInfo(...)                    _WEBPA_LOG(WEBPA_LOG_INFO, __VA_ARGS__)
#define WalPrint(...)                   _WEBPA_LOG(WEBPA_LOG_PRINT, __VA_ARGS__)

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
void waitForOperationalReadyCondition();

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
 * @brief LOGInit Initialize RDK Logger
 */
void LOGInit();

/**
 * @brief _WEBPA_LOG WEBPA RDK Logger API
 *
 * @param[in] level LOG Level
 * @param[in] msg Message to be logged 
 */
void _WEBPA_LOG(unsigned int level, const char *msg, ...)
    __attribute__((format (printf, 2, 3)));

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
void initNotifyTask();

void sendNotification(char *payload, char *source, char *destination);

#endif /* _WEBPA_ADAPTER_H_ */
