/**
 * @file webpa_internal.h
 *
 * @description This file describes the Webpa Abstraction Layer
 *
 * Copyright (c) 2015  Comcast
 */
#include <cJSON.h>
#include "stdlib.h"
#include "wdmp-c.h"
#include "webpa_adapter.h"


/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
#define FACTORY_RESET_NOTIFY_MAX_RETRY_COUNT			5
/*----------------------------------------------------------------------------*/
/*                               Data Structures                              */
/*----------------------------------------------------------------------------*/

/**
 * @brief Component or source that changed a param value.
 */
typedef enum
{
    CHANGED_BY_FACTORY_DEFAULT      = (1<<0), /**< Factory Defaults */
    CHANGED_BY_ACS                  = (1<<1), /**< ACS/TR-069 */
    CHANGED_BY_WEBPA                = (1<<2), /**< WebPA */
    CHANGED_BY_CLI                  = (1<<3), /**< Command Line Interface (CLI) */
    CHANGED_BY_SNMP                 = (1<<4), /**< SNMP */
    CHANGED_BY_FIRMWARE_UPGRADE     = (1<<5), /**< Firmware Upgrade */
    CHANGED_BY_WEBUI                = (1<<7), /**< Local Web UI (HTTP) */
    CHANGED_BY_UNKNOWN              = (1<<8), /**< Unknown */
    CHANGED_BY_XPC                  = (1<<9)  /**< xPC */
} PARAMVAL_CHANGE_SOURCE;

/**
 * @brief Structure to return Parameter info in Notification callback.
 */
typedef struct
{
    const char *paramName;
    const char *oldValue;
    const char *newValue;
    DATA_TYPE type;
    PARAMVAL_CHANGE_SOURCE changeSource;
} ParamNotify;

typedef struct
{
   char *transId;
}TransData;

typedef struct
{
   char *nodeMacId;
   char *status;
   char *interface;
   char *hostname;
}NodeData;

typedef struct
{
   int status;
}DeviceStatus;

typedef enum
{
    PARAM_NOTIFY = 0,
    TRANS_STATUS,
    CONNECTED_CLIENT_NOTIFY,
    DEVICE_STATUS,
    FACTORY_RESET,
    FIRMWARE_UPGRADE
} NOTIFY_TYPE;

typedef struct
{
    NOTIFY_TYPE type;
    union {
    	ParamNotify *notify;
    	TransData * status;
        NodeData * node;
        DeviceStatus *device;
    } u;
} NotifyData;

/**
 * @brief Function pointer for Notification callback
 */
typedef void (*notifyCB)(NotifyData *notifyDataPtr);

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/

/**
 * @brief sendConnectedClientNotification function to send Connected Client notification
 * for change to Device.Hosts.Host. dynamic table
 */
void sendConnectedClientNotification(char * macId, char *status, char *interface, char *hostname);

/**
 * @brief send_transaction_Notify function to send transaction status notification
 */
void processTransactionNotification(char transId[]);
/**
 * @brief Registers the notification callback function.
 *
 * @param[in] cb Notification callback function.
 * @return status.
 */
int RegisterNotifyCB(notifyCB cb);

void * getNotifyCB();

/**
 * @brief Validate connected client notification data based on max length.
 *
 * @param[in] notification data values.
 * @return status.
 */
WDMP_STATUS validate_conn_client_notify_data(char *notify_param_name, char* interface_name,char* mac_id,char* status,char* hostname);

/**
 * @brief Validate webpa notification data based on max length.
 *
 * @param[in] notification data values.
 * @return status.
 */
WDMP_STATUS validate_webpa_notification_data(char *notify_param_name, char *write_id);

