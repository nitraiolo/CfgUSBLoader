#ifndef _VERSION_H 
#define _VERSION_H 

#define STR(X) #X
#define QUOTE(X) STR(X)

#ifndef VERSION
#define VERSION 00xy
#endif

#define VERSION_STR QUOTE(VERSION)

#if defined(BUILD_222) && (BUILD_222 == 1)

   #define CFG_VERSION_STR VERSION_STR"-222" 
   #define DEFAULT_IOS_IDX CFG_IOS_222_MLOAD 
   //#define CFG_DEFAULT_PARTITION "FAT1" 
   #define CFG_DEFAULT_PARTITION "auto" 
   #define CFG_HIDE_HDDINFO 1 

#elif defined(BUILD_DBG) && (BUILD_DBG > 0)

   #define CFG_VERSION_STR VERSION_STR"-dbg" 
   #define DEFAULT_IOS_IDX CFG_IOS_AUTO 
   #define CFG_DEFAULT_PARTITION "auto" 
   #define CFG_HIDE_HDDINFO 1 

#else

   #define CFG_VERSION_STR VERSION_STR"\0\0\0\0"
   #define DEFAULT_IOS_IDX CFG_IOS_AUTO 
   //#define CFG_DEFAULT_PARTITION "WBFS1" 
   #define CFG_DEFAULT_PARTITION "auto" 
   //#define CFG_HIDE_HDDINFO 0 
   #define CFG_HIDE_HDDINFO 1 

#endif 
#endif 

