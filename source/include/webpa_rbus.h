#ifndef _WEBPA_RBUS_H_
#define _WEBPA_RBUS_H_

#include <stdio.h>
#include <rbus/rbus.h>
#include <rbus/rbus_object.h>
#include <rbus/rbus_property.h>
#include <rbus/rbus_value.h>

#include "webpa_adapter.h"
#include <wdmp-c.h>
#include <cimplog.h>


bool isRbusEnabled();
bool isRbusInitialized();
WDMP_STATUS webpaRbusInit(const char *pComponentName);
void webpaRbus_Uninit();
rbusError_t setTraceContext(char* traceContext[]);
rbusError_t getTraceContext(char* traceContext[]);
rbusError_t clearTraceContext();
static void SubscribeCloudConnOnlineEventCallbackHandler(rbusHandle_t handle,rbusEvent_t const* event,rbusEventSubscription_t* subscription);
void SubscribeCloudConnOnlineEvent();
#endif
