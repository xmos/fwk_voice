// Copyright 2019-2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <stdio.h>

// Ripped from lib_logging

#ifndef DEBUG_UNIT
#define DEBUG_UNIT APPLICATION
#endif

#ifndef DEBUG_PRINT_ENABLE_ALL
#define DEBUG_PRINT_ENABLE_ALL 0
#endif

#ifndef DEBUG_PRINT_ENABLE
#define DEBUG_PRINT_ENABLE 0
#endif

#if !defined(DEBUG_PRINT_ENABLE_APPLICATION) && !defined(DEBUG_PRINT_DISABLE_APPLICATION)
#define DEBUG_PRINT_ENABLE_APPLICATION DEBUG_PRINT_ENABLE
#endif

#define DEBUG_UTILS_JOIN0(x,y) x ## y
#define DEBUG_UTILS_JOIN(x,y) DEBUG_UTILS_JOIN0(x,y)

#if DEBUG_UTILS_JOIN(DEBUG_PRINT_ENABLE_,DEBUG_UNIT)
#define DEBUG_PRINT_ENABLE0 1
#endif

#if DEBUG_UTILS_JOIN(DEBUG_PRINT_DISABLE_,DEBUG_UNIT)
#define DEBUG_PRINT_ENABLE0 0
#endif

#if !defined(DEBUG_PRINT_ENABLE0)
#define DEBUG_PRINT_ENABLE0 DEBUG_PRINT_ENABLE_ALL
#endif


#if DEBUG_PRINT_ENABLE0
#define debug_printf(...) printf(__VA_ARGS__)
#else
#define debug_printf(...)
#endif
