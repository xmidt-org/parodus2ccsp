/**
 * @file main.c
 *
 * @description This file defines WebPA's main function
 *
 * Copyright (c) 2015  Comcast
 */
#include "stdlib.h"
#include "signal.h"
#include "webpa_adapter.h"
#include "libpd.h"

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/

static void sig_handler(int sig);

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/

int main()
{

	LOGInit();
        
	signal(SIGTERM, sig_handler);
	signal(SIGINT, sig_handler);
	signal(SIGUSR1, sig_handler);
	signal(SIGUSR2, sig_handler);
	signal(SIGSEGV, sig_handler);
	signal(SIGBUS, sig_handler);
	signal(SIGKILL, sig_handler);
	signal(SIGFPE, sig_handler);
	signal(SIGILL, sig_handler);
	signal(SIGQUIT, sig_handler);
	signal(SIGHUP, sig_handler);
	signal(SIGALRM, sig_handler);

	const char *pComponentName = WEBPA_COMPONENT_NAME;
	WalInfo("********** Starting component: %s **********\n ", pComponentName); 

	msgBusInit(pComponentName);
	waitForOperationalReadyCondition();
	
	/* Backend Manager for Webpa Creation and Initilization */
	CosaWebpaBEManagerCreate( );
	libpd_client_mgr();
	initComponentCaching();
	// Initialize Apply WiFi Settings handler
	initApplyWiFiSettings();
	initNotifyTask();
	parodus_receive_wait();
 
	WalInfo("Exiting webpa main thread!!\n");
	return 1;
}

/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/

static void sig_handler(int sig)
{

	if ( sig == SIGINT ) 
	{
		signal(SIGINT, sig_handler); /* reset it to this function */
		WalInfo("SIGINT received!\n");
	}
	else if ( sig == SIGUSR1 ) 
	{
		signal(SIGUSR1, sig_handler); /* reset it to this function */
		WalInfo("SIGUSR1 received!\n");
	}
	else if ( sig == SIGUSR2 ) 
	{
		WalInfo("SIGUSR2 received!\n");
	}
	else if ( sig == SIGCHLD ) 
	{
		signal(SIGCHLD, sig_handler); /* reset it to this function */
		WalInfo("SIGHLD received!\n");
	}
	else if ( sig == SIGPIPE ) 
	{
		signal(SIGPIPE, sig_handler); /* reset it to this function */
		WalInfo("SIGPIPE received!\n");
	}
	else if ( sig == SIGALRM ) 
	{
		signal(SIGALRM, sig_handler); /* reset it to this function */
		WalInfo("SIGALRM received!\n");
	}
	else 
	{
		WalInfo("Signal %d received!\n", sig);
		exit(0);
	}
	
}
