/**
 * @file webpa_logg.c
 *
 * @description This file describes the Webpa log API's
 *
 * Copyright (c) 2015  Comcast
 */

#include "webpa_internal.h"
#include <cimplog/cimplog.h>

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/*                               Data Structures                              */
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/
void webpa_logger(unsigned int level, const char *module,
        const char *format, char *msg);

#ifndef FEATURE_SUPPORT_RDKLOG
#define RDK_LOG webpa_logger
#define LOGGER_MODULE "WEBPA"
#else
#define LOGGER_MODULE "LOG.RDK.WEBPA"
#endif
/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/

void LOGInit()
{
	#ifdef FEATURE_SUPPORT_RDKLOG
		rdk_logger_init("/etc/debug.ini");    /* RDK logger initialization*/
	#endif
}

void _WEBPA_LOG(unsigned int level, const char *msg, ...)
{
	va_list arg;
	char *pTempChar = NULL;
	int ret = 0;
	unsigned int rdkLogLevel = LOG_DEBUG;
	switch(level)
	{
		case WEBPA_LOG_ERROR:
			rdkLogLevel = LOG_ERROR;
			break;

		case WEBPA_LOG_INFO:
			rdkLogLevel = LOG_INFO;
			break;

		case WEBPA_LOG_PRINT:
			rdkLogLevel = LOG_DEBUG;
			break;
	}

	if( rdkLogLevel <= LOG_INFO )
	{
		pTempChar = (char *)malloc(4096);
		if(pTempChar)
		{
			va_start(arg, msg);
			ret = vsnprintf(pTempChar, 4096, msg,arg);
			if(ret < 0)
			{
				perror(pTempChar);
			}
			va_end(arg);
			RDK_LOG(rdkLogLevel, LOGGER_MODULE, "%s", pTempChar);
			WAL_FREE(pTempChar);
		}
	}
}

/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/

void webpa_logger(unsigned int level, const char *module,
        const char *format, char *msg)
{
        switch(level)
	{
		case LOG_ERROR:
			cimplog_error(module, msg);
			break;

		case LOG_INFO:
			cimplog_info(module, msg);
			break;

		case LOG_DEBUG:
			cimplog_debug(module, msg);
			break;
	}
}
