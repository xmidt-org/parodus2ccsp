/**
* @file ssp_internal.h
 *
 * @description This is the template file of ssp_internal.h for WebPA ccsp component.
 */

#ifndef  _SSP_INTERNAL_H_
#define  _SSP_INTERNAL_H_

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/

#define  CCSP_COMMON_COMPONENT_HEALTH_Red                   1
#define  CCSP_COMMON_COMPONENT_HEALTH_Yellow                2
#define  CCSP_COMMON_COMPONENT_HEALTH_Green                 3

#define  CCSP_COMMON_COMPONENT_STATE_Initializing           1
#define  CCSP_COMMON_COMPONENT_STATE_Running                2
#define  CCSP_COMMON_COMPONENT_STATE_Blocked                3
#define  CCSP_COMMON_COMPONENT_STATE_Paused                 3

#define  CCSP_COMMON_COMPONENT_FREERESOURCES_PRIORITY_High  1
#define  CCSP_COMMON_COMPONENT_FREERESOURCES_PRIORITY_Low   2

#define  CCSP_COMPONENT_ID_WEBPAAGENT                             "com.cisco.spvtg.ccsp.webpaagent"
#define  CCSP_COMPONENT_NAME_WEBPAAGENT                           "com.cisco.spvtg.ccsp.webpaagent"
#define  CCSP_COMPONENT_VERSION_WEBPAAGENT                       1
#define  CCSP_COMPONENT_PATH_WEBPAAGENT                           "/com/cisco/spvtg/ccsp/webpaagent"

#define  MESSAGE_BUS_CONFIG_FILE                            "msg_daemon.cfg"

/**
 * @brief Defines webpa agent component Structure
 */
typedef  struct
_COMPONENT_COMMON_WEBPAAGENT
{
    char*                           Name;
    ULONG                           Version;
    char*                           Author;
    ULONG                           Health;
    ULONG                           State;

    BOOL                            LogEnable;
    ULONG                           LogLevel;

    ULONG                           MemMaxUsage;
    ULONG                           MemMinUsage;
    ULONG                           MemConsumed;
}
COMPONENT_COMMON_WEBPAAGENT,  *PCOMPONENT_COMMON_WEBPAAGENT;

/**
 * @brief Initializes webpa agent component
 * param[in] component_com_webpaagent object
 */
#define ComponentCommonDmInit(component_com_webpaagent)                                          \
        {                                                                                  \
            AnscZeroMemory(component_com_webpaagent, sizeof(COMPONENT_COMMON_WEBPAAGENT));             \
            component_com_webpaagent->Name        = NULL;                                        \
            component_com_webpaagent->Version     = 1;                                           \
            component_com_webpaagent->Author      = NULL;                                        \
            component_com_webpaagent->Health      = CCSP_COMMON_COMPONENT_HEALTH_Red;            \
            component_com_webpaagent->State       = CCSP_COMMON_COMPONENT_STATE_Running;         \
            if(g_iTraceLevel >= CCSP_TRACE_LEVEL_EMERGENCY)                                \
                component_com_webpaagent->LogLevel = (ULONG) g_iTraceLevel;                      \
            component_com_webpaagent->LogEnable   = TRUE;                                        \
            component_com_webpaagent->MemMaxUsage = 0;                                           \
            component_com_webpaagent->MemMinUsage = 0;                                           \
            component_com_webpaagent->MemConsumed = 0;                                           \
        }

/**
 * @brief clears webpa agent component
 * param[in] component_com_webpaagent object
 */
#define  ComponentCommonDmClean(component_com_webpaagent)                                        \
         {                                                                                  \
            if ( component_com_webpaagent->Name )                                                \
            {                                                                               \
                AnscFreeMemory(component_com_webpaagent->Name);                                  \
            }                                                                               \
                                                                                            \
            if ( component_com_webpaagent->Author )                                              \
            {                                                                               \
                AnscFreeMemory(component_com_webpaagent->Author);                                \
            }                                                                               \
         }

/**
 * @brief free webpa agent component
 * param[in] component_com_webpaagent object
 */
#define  ComponentCommonDmFree(component_com_webpaagent)                                         \
         {                                                                                  \
            ComponentCommonDmClean(component_com_webpaagent);                                    \
            AnscFreeMemory(component_com_webpaagent);                                            \
         }

int  cmd_dispatch(int  command);

/**
 * @brief ssp_create function definition to create component
 */
ANSC_STATUS
ssp_create
(
);

/**
 * @brief ssp_engage function definition to engage component
 */
ANSC_STATUS
ssp_engage
(
);

/**
 * @brief ssp_cancel function definition to cancel component
 */
ANSC_STATUS
ssp_cancel
(
);



char*
ssp_CcdIfGetComponentName
    (
        ANSC_HANDLE                     hThisObject
    );

ULONG
ssp_CcdIfGetComponentVersion
    (
        ANSC_HANDLE                     hThisObject
    );

char*
ssp_CcdIfGetComponentAuthor
    (
        ANSC_HANDLE                     hThisObject
    );

ULONG
ssp_CcdIfGetComponentHealth
    (
        ANSC_HANDLE                     hThisObject
    );

ULONG
ssp_CcdIfGetComponentState
    (
        ANSC_HANDLE                     hThisObject
    );

BOOL
ssp_CcdIfGetLoggingEnabled
    (
        ANSC_HANDLE                     hThisObject
    );

ANSC_STATUS
ssp_CcdIfSetLoggingEnabled
    (
        ANSC_HANDLE                     hThisObject,
        BOOL                            bEnabled
    );

ULONG
ssp_CcdIfGetLoggingLevel
    (
        ANSC_HANDLE                     hThisObject
    );

ANSC_STATUS
ssp_CcdIfSetLoggingLevel
    (
        ANSC_HANDLE                     hThisObject,
        ULONG                           LogLevel
    );

ULONG
ssp_CcdIfGetMemMaxUsage
    (
        ANSC_HANDLE                     hThisObject
    );

ULONG
ssp_CcdIfGetMemMinUsage
    (
        ANSC_HANDLE                     hThisObject
    );

ULONG
ssp_CcdIfGetMemConsumed
    (
        ANSC_HANDLE                     hThisObject
    );

ANSC_STATUS
ssp_CcdIfApplyChanges
    (
        ANSC_HANDLE                     hThisObject
    );


#endif
