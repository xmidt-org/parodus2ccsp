
/**********************************************************************

    module: plugin_main.h

        For COSA Library development.

    ---------------------------------------------------------------

    copyright:

        Cisco Systems, Inc.
        All Rights Reserved.

    ---------------------------------------------------------------

    description:

        This header file defines the exported apis for COSA Library plugin.

    ---------------------------------------------------------------

    environment:

        platform independent

    ---------------------------------------------------------------

    author:

        Bin Zhu

    ---------------------------------------------------------------

    revision:

        12/12/2010    initial revision.

**********************************************************************/


#ifndef  _PLUGIN_MAIN_H
#define  _PLUGIN_MAIN_H


#if (defined _ANSC_WINDOWSNT) || (defined _ANSC_WINDOWS9X)

#ifdef _ALMIB_EXPORTS
#define ANSC_EXPORT_API                                __declspec(dllexport)
#else
#define ANSC_EXPORT_API                                __declspec(dllimport)
#endif

#endif

#ifdef _ANSC_LINUX
#define ANSC_EXPORT_API
#endif

#ifdef __cplusplus 
extern "C"{
#endif

/***************************************************************************
 *
 *  COSA stands for "Cisco Open Service Architecture"
 *
 ***************************************************************************/
int ANSC_EXPORT_API
COSA_Init
    (
        ULONG                       uMaxVersionSupported, 
        void*                       hCosaPlugInfo         /* PCOSA_PLUGIN_INFO passed in by the caller */
    );

BOOL ANSC_EXPORT_API
COSA_IsObjSupported
    (
        char*                        pObjName
    );

void ANSC_EXPORT_API
COSA_Unload
    (
        void
    );

#ifdef __cplusplus 
}
#endif

#endif
