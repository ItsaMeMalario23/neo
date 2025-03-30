#ifndef RDEBUG_H
#define RDEBUG_H

#include <stdlib.h>

#include <main.h>       /* necessary for debug mode defs */

// Build configuration check
#if !defined R_DEBUG && !defined R_RELEASE
    #error 'Requires definition of R_DEBUG or R_RELEASE'
#endif

#if defined R_DEBUG && defined R_RELEASE
    #warning 'Both R_DEBUG and R_RELEASE defined, assuming release mode'
    #undef R_DEBUG
#endif

// ========================================================================== //
//                                                                            //
//  LINUS' UNIVERSAL DEBUG LIBRARY (only partially stolen from radical)       //
//                                                                            //
//  rDebug modes:                                                             //
//                                                                            //
//  R_DEBUG                 All debug functions will be compiled              //
//  R_RELEASE               Only release functions will be compiled           //
//                                                                            //
//  RDEBUG_TIME             Show timestamps on debug messages                 //
//                                                                            //
//  Debug break modes:                                                        //  
//                                                                            //
//  RDEBUG_BREAK_DBREAK     Enable breaks through compiler intrinsics         //
//  RDEBUG_BREAK_GCC        Enable breaks through AT&T asm (GCC)              //
//  RDEBUG_BREAK_INTEL      Enable breaks through Intel asm (MSVC)            //
//  RDEBUG_BREAK_EXIT       Enable breaks throught exit function              //
//                                                                            //  
// ========================================================================== //

//
//  Error levels
//
#define RDEBUG_LVL_NONE     0
#define RDEBUG_LVL_INFO     1
#define RDEBUG_LVL_WARN     2
#define RDEBUG_LVL_ERROR    3
#define RDEBUG_LVL_FATAL    4
// Shorter names for convinience
#define ERR_NONE            0
#define ERR_INFO            1
#define ERR_WARN            2
#define ERR_ERR             3
#define ERR_FTL             4

// Maximum length for debug messages
#define RDEBUG_STRING_MAX_LEN   256

//
//  Dont use these functions
//
void rDebugHaltOnAsserts_Implementation(char halt);
void rDebugOutputStream_Implementation(const char* stream);
void rAssertFail_Implementation(const char* condition, const char* filename, unsigned linenum);
void rWarningFail_Implementation(const char* condition, const char* filename, unsigned linenum);
void rDebugString_Implementation(unsigned errorlevel, const char* string);

//
//  Use these functions and macros
//
#define rNullStringWrap( x ) ((x) == NULL ? "<NULL>" : x)

//  Debug breaks
#if defined RDEBUG_BREAK_DBREAK
    #define rReleaseBreak() {__debugbreak();}
#elif defined RDEBUG_BREAK_GCC
    // Issue int 3 if debugbreak() does not work
    #define rReleaseBreak() {__asm__( "int $3" );}
#elif defined RDEBUG_BREAK_INTEL
    #define rReleaseBreak() {__asm { int 3 }}
#elif defined RDEBUG_BREAK_EXIT
    // Halt execution on assert fail
    #define rReleaseBreak() { exit(-1); }
#else
    #define rReleaseBreak() ((void)0)
#endif

//
//  Always compile release functions
//
#define rReleaseString( x ) rDebugString_Implementation(0, x)
#define rReleaseStringLevel( lvl, x ) rDebugString_Implementation(lvl, x)

void rReleasePrintf(const char* fmt, ... );

#define rReleaseAssert( x ) if (!(x)) rAssertFail_Implementation(#x,__FILE__,__LINE__)
#define rReleaseAssertMsg( x, msg ) if (!(x)) rAssertFail_Implementation(msg,__FILE__,__LINE__)

#define rDebugOutputStream( x ) rDebugOutputStream_Implementation(x)

//
//  Debug functions are not compiled in release mode
//
#ifdef R_RELEASE

    #define rBreak() ((void)0)
    #define rHaltOnAsserts( x ) ((void)0)
    #define rAssert( x ) ((void)0)      	    /* Assert x true, print condition on fail */
    #define rAssertMsg( x, msg ) ((void)0)      /* Assert x true, print msg on fail */
    #define rWarning( x ) ((void)0)
    #define rWarningMsg( x, msg ) ((void)0)

    #define rDebugString( x ) ((void)0)
    #define rDebugStringLevel( lvl, x ) ((void)0)

    inline void rDebugPrintf(const char* fmt, ... ) { }

#else
    
    // Compiled only if debug mode is defined
    #define rBreak() rReleaseBreak()
    #define rAssert( x ) if (!(x)) rAssertFail_Implementation(#x,__FILE__,__LINE__)
    #define rAssertMsg( x, msg ) if (!(x)) rAssertFail_Implementation(msg,__FILE__,__LINE__)
    #define rWarning( x ) if (!(x)) rWarningFail_Implementation(#x,__FILE__,__LINE__)
    #define rWarningMsg( x, msg ) if (!(x)) rWarningFail_Implementation(msg,__FILE__,__LINE__)
    #define rHaltOnAsserts( x ) rDebugHaltOnAsserts_Implementation(x)

    #define rDebugString( x ) rDebugString_Implementation(0, x)
    #define rDebugStringLevel( lvl, x ) rDebugString_Implementation(lvl, x)

    void rDebugPrintf(const char* fmt, ... );

#endif

#endif