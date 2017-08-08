/**
 * @file ssp_messagebus_interface.h
 *
 * @description This header defines the Message Bus initalization and component path registration methods.
 */

/**
 * @brief SSP messagebus specific defines.
 */
#ifndef  _SSP_MESSAGEBUS_INTERFACE_
#define  _SSP_MESSAGEBUS_INTERFACE_

ANSC_STATUS
ssp_Mbi_MessageBusEngage
    (
        char * component_id,
        char * config_file,
        char * path
    );

int
ssp_Mbi_Initialize
    (
        void * user_data
    );

int
ssp_Mbi_Finalize
    (
        void * user_data
    );

int
ssp_Mbi_Buscheck
    (
        void * user_data
    );

int
ssp_Mbi_FreeResources
    (
        int priority,
        void * user_data
    );

#endif
