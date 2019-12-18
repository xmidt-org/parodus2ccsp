#ifndef _WEBCONFIG_LOG_H_
#define _WEBCONFIG_LOG_H_

#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <cimplog.h>

#define WEBCFG_LOGGING_MODULE                     "WEBCONFIG"
/**
 * @brief Enables or disables debug logs.
 */
#define WebConfigLog(...)       __cimplog_generic(LOGGING_MODULE, __VA_ARGS__)

#define WebcfgError(...)	cimplog_error(WEBCFG_LOGGING_MODULE, __VA_ARGS__)
#define WebcfgInfo(...)		cimplog_info(WEBCFG_LOGGING_MODULE, __VA_ARGS__)
#define WebcfgDebug(...)	cimplog_debug(WEBCFG_LOGGING_MODULE, __VA_ARGS__)

#endif /* _WEBCONFIG_LOG_H_ */
