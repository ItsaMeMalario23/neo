#ifndef MEMTRACK_H
#define MEMTRACK_H

#include <stdlib.h>

#include <main.h>       /* necessary for debug mode defs */
#include <rdebug.h>

//#define MTRACK_DEBUG
//#define MTRACK_MANUAL_CLEANUP

// Build configuration check
#if !defined R_DEBUG && !defined R_RELEASE
    #error 'Memtrack requires definition of R_DEBUG or R_RELEASE'
#endif

#if defined MTRACK_DEBUG && defined R_RELEASE
    #warning 'Memtrack debug and release mode defined at the same time'
#endif

// ========================================================================== //
//                                                                            //
//  LINUS' UNIVERSAL MEMORY TRACKING LIBRARY                                  //
//                                                                            //
//  MTRACK_BUF_SIZE:                                                          //
//  Initial size of memtrack buffers                                          //
//  Memtrack uses up to 3 buffers depending on operation mode                 //
//  Buffer sizes will be increased automatically if necessary                 //
//                                                                            //
//  Memtrack control modes:                                                   //
//                                                                            //
//  DIRECT_LAW:                                                               //
//  Bypass all memory allocation functions                                    //
//  Memtrack functions will not be compiled                                   //
//                                                                            //
//  DEGRADED_LAW:                                                             //
//  Track only the amount of allocations / deallocations                      //
//  Allocated memory cannot be freed automatically                            //
//                                                                            //
//  NORMAL_LAW:                                                               //
//  Track all allocated memory blocks including sizes                         //
//  Pointers to freed memory blocks are tracked to prevent double freeing     //
//  Allocated memory can be freed automatically                               //
//                                                                            //
//  DEBUG_LAW:                                                                //
//  Also tracks information about where memory was allocated / deallocated    //
//  memSetup() configures which information should be output                  //
//                                                                            //
//  If R_DEBUG is defined, memtrack will compile assuming NORMAL_LAW          //
//                                                                            //
//  Definition of R_RELEASE will impose DIRECT_LAW, memory allocation         //
//  functions will be bypassed, memtrack functions will not be compiled       //
//                                                                            //
//  Unless MTRACK_MANUAL_CLEANUP is defined, allocated memory will be         //
//  freed automatically at program exit                                       //
//                                                                            //
// ========================================================================== //

//
//  Constants
//
#define MTRACK_BUF_SIZE         256

// Max string len used to store information about freed pointers in debug mode
#define MTRACK_MAX_STRING_LEN   64

// Memtrack control laws / modes
#define MTRACK_UNINITIALIZED    0
#define MTRACK_DIRECT_LAW       1
#define MTRACK_DEGRADED_LAW     2
#define MTRACK_NORMAL_LAW       3
#define MTRACK_DEBUG_LAW        4

// Typedefs
typedef unsigned char control_law;

typedef struct mblock_s {
    void* ptr;
    size_t size;
} mblock_t;

//
//  Dont use these functions
//
void  memtrackInitialize_Implementation(control_law law);
void  memtrackSetup_Implementation(control_law law, bool dAlloc, bool dRealloc, bool dFree, bool dMtrack);
void  memtrackChangeLaw_Implementation(control_law law);
void  memtrackCleanup_Implementation(void);
void  memtrackSetWarning_Implementation(size_t size);
void  memtrackSetLimit_Implementation(size_t size);
void* memtrackAllocate_Implementation(size_t size);
void* memtrackAllocateInitialize_Implementation(size_t size, size_t count);
void* memtrackAllocateSet_Implementation(size_t size, unsigned char value);
void* memtrackReallocate_Implementation(void* ptr, size_t size);
int   memtrackFree_Implementation(void* ptr, const char* filename, unsigned linenum);

void* memtrackAllocate_Debug(size_t size, const char* filename, unsigned linenum);
void* memtrackAllocateInitialize_Debug(size_t size, size_t count, const char* filename, unsigned linenum);
void* memtrackAllocateSet_Debug(size_t size, unsigned char value, const char* filename, unsigned linenum);
void* memtrackReallocate_Debug(void* ptr, size_t size, const char* filename, unsigned linenum);
void  memtrackFree_Debug(void* ptr, const char* filename, unsigned linenum);

//
//  Use these functions
//
#if defined MTRACK_DEBUG || defined R_DEBUG
    // Memtrack functions will only be compiled if debug mode is defined
    #define memSetup( lw, alc, rlc, fr, mem ) memtrackSetup_Implementation(lw,alc,rlc,fr,mem)
    #define memChLaw( x ) memtrackChangeLaw_Implementation(x)
    #define memCleanup() memtrackCleanup_Implementation()
    #define memWarn( x ) memtrackSetWarning_Implementation(x)
    #define memLimit( x ) memtrackSetLimit_Implementation(x)

#endif

#if defined MTRACK_DEBUG

    #define memInit() memtrackInitialize_Implementation(MTRACK_DEBUG_LAW)
    #define memInitLaw( x ) memtrackInitialize_Implementation(MTRACK_DEBUG_LAW)

    #define memAlloc( x ) memtrackAllocate_Debug(x,__FILE__,__LINE__);
    #define memAllocCount( x, y ) memtrackAllocate_Debug(x*y,__FILE__,__LINE__)
    #define memAllocType( x, y ) memtrackAllocate_Debug(sizeof(x)*y,__FILE__,__LINE__)
    #define memAllocInit( size, cnt ) memtrackAllocateInitialize_Debug(size,cnt,__FILE__,__LINE__)
    #define memAllocInitType( type, cnt ) memtrackAllocateInitialize_Debug(sizeof(type),cnt,__FILE__,__LINE__)
    #define memAllocSet( x, y ) memtrackAllocateSet_Debug(x,y,__FILE__,__LINE__)
    #define memAllocSetCount( x, y, z) memtrackAllocateSet_Debug(x*y,z,__FILE__,__LINE__)
    #define memRealloc( ptr, x ) memtrackReallocate_Debug(ptr,x,__FILE__,__LINE__)

    #define memFree( x ) memtrackFree_Debug(x,__FILE__,__LINE__)

#elif defined R_DEBUG

    #define memInit() memtrackInitialize_Implementation(MTRACK_NORMAL_LAW)
    #define memInitLaw( x ) memtrackInitialize_Implementation(x)

    #define memAlloc( x ) memtrackAllocate_Implementation(x)
    #define memAllocCount( x, y ) memtrackAllocate_Implementation(x * y)
    #define memAllocType( x, y ) memtrackAllocate_Implementation(sizeof(x) * y)
    #define memAllocInit( size, cnt ) memtrackAllocateInitialize_Implementation(size, cnt)
    #define memAllocInitType( type, cnt ) memtrackAllocateInitialize_Implementation(sizeof(type), cnt)
    #define memAllocSet( x, y ) memtrackAllocateSet_Implementation(x, y)
    #define memAllocSetCount( x, y, z) memtrackAllocateSet_Implementation(x * y, z)
    #define memRealloc( ptr, x ) memtrackReallocate_Implementation(ptr, x)

    #define memFree( x ) memtrackFree_Implementation(x,NULL,0)

#else
    // This will be compiled in release
    #define memInit() ((void)0)
    #define memInitLaw( x ) ((void)0)
    #define memSetup( a, b, c, d, e ) ((void)0)
    #define memChLaw( x ) ((void)0)
    #define memCleanup() ((void)0)
    #define memWarn( x ) ((void)0)
    #define memLimit( x ) ((void)0)

    #define memAlloc( x ) malloc(x)
    #define memAllocCount( x, y ) malloc(x * y)
    #define memAllocType( x, y ) malloc(sizeof(x) * y)
    #define memAllocInit( x, y ) calloc(y, x)
    #define memAllocInitType( x, y ) calloc(y, sizeof(x))
    #define memAllocSet( x, y ) calloc(1, x)
    #define memAllocSetCount( x, y, z) calloc(y, x)
    #define memRealloc( ptr, x ) realloc(ptr, x)

    #define memFree( x ) free(x)

#endif

#endif