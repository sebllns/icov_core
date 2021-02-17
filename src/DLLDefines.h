// Contents of DLLDefines.h
#ifndef _IcovCoreLib_DLLDEFINES_H_
#define _IcovCoreLib_DLLDEFINES_H_

/* Cmake will define IcovCoreLib_EXPORTS on Windows when it
configures to build a shared library. If you are going to use
another build system on windows or create the visual studio
projects by hand you need to define IcovCoreLib_EXPORTS when
building a DLL on windows.
*/
// We are using the Visual Studio Compiler and building Shared libraries

#if defined (_WIN32)
  #if defined(IcovCoreLib_EXPORTS)
    #define  ICOV_EXPORT __declspec(dllexport)
  #else
    #define  ICOV_EXPORT __declspec(dllimport)
  #endif /* IcovCoreLib_EXPORTS */
#else /* defined (_WIN32) */
 #define ICOV_EXPORT
#endif

#endif /* _IcovCoreLib_DLLDEFINES_H_ */
