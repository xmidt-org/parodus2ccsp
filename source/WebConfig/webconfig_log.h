#ifndef _WEBCONFIG_LOG_H_
#define _WEBCONFIG_LOG_H_

#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <cimplog.h>

#define LOGGING_MODULE                     "WEBCONFIG"
/**
 * @brief Enables or disables debug logs.
 */
#define WebConfigLog(...)                   __cimplog_generic(LOGGING_MODULE, __VA_ARGS__)

#endif /* _WEBCONFIG_LOG_H_ */
