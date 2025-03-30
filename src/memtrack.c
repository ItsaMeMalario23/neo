#include <stdio.h>
#include <string.h>

#include <memtrack.h>

//
//  Globals
//
size_t g_memAllocated = 0;

mblock_t* g_memBuf = NULL;
size_t g_memBufSize = 0;

size_t g_mTrackWarning = 0;
size_t g_mTrackLimit = 0;

void** g_memFreeBuf = NULL;
size_t g_freeBufSize = 0;
size_t g_freeBufIdx = 0;

// Only used in debug law
char** g_freeInfoBuf = NULL;

// Control modes
control_law g_mTrackLaw = MTRACK_UNINITIALIZED;

bool g_mdebugAllocs = 0;
bool g_mdebugReallocs = 0;
bool g_mdebugFrees = 0;
bool g_mdebugMemtrack = 0;

// ========================================================================== //
//                                                                            //
//  Local functions                                                           //
//                                                                            //
// ========================================================================== //

// Return 0 if memory can be allocated, 1 if not
int __sizeOverflow(size_t size)
{
    rAssert(size);

    if (!g_memBuf || g_mTrackLaw < 3)
    {
        if (g_mTrackWarning || g_mTrackLimit)
        {
            rDebugString("[MEMTRACK]: Invalid memtrack law, limits cannot be checked");
        }

        return 0;
    }

    if (g_mTrackLimit && g_mTrackLimit < g_memAllocated + size)
    {
        rDebugPrintf("[MEMTRACK]: Allocating %lld bytes would exceed set limit of %lld bytes", size, g_mTrackLimit);
        return 1;
    }

    if (g_mTrackWarning && g_mTrackWarning < g_memAllocated + size)
    {
        rDebugPrintf("[MEMTRACK]: %lld bytes allocated", g_memAllocated + size);
    }

    return 0;
}

// Append ptr and size to allocated buffer and increase buffer if necessary
void __setAllocated(void* ptr, size_t size)
{
    rAssert(ptr);
    rAssert(size);

    if (!ptr || g_mTrackLaw < 2)
    {
        return;
    }

    if (!g_memBuf || g_mTrackLaw == MTRACK_DEGRADED_LAW)
    {
        g_memAllocated++;
        return;
    }

    g_memAllocated += size;

    for (size_t i = 0; i < g_memBufSize; i++)
    {
        if (!g_memBuf[i].ptr && !g_memBuf[i].size)
        {
            g_memBuf[i].ptr = ptr;
            g_memBuf[i].size = size;

            if (g_mTrackLaw == MTRACK_DEBUG_LAW && g_mdebugMemtrack)
            {
                printf("[MEMTRACK]: Memory block of size %lld has been appended to buffer\n", size);
            }

            return;
        }
    }

    mblock_t* tmp = (mblock_t*) realloc(g_memBuf, (g_memBufSize + 32) * sizeof(mblock_t));

    if (!tmp)
    {
        rDebugString("[MEMTRACK]: Buffer realloc failed, memtrack running in degraded law");

        free(g_memBuf);

        g_mTrackLaw = MTRACK_DEGRADED_LAW;
        g_memBuf = NULL;
        g_memBufSize = 0;

        return;
    }

    tmp[g_memBufSize].ptr = ptr;
    tmp[g_memBufSize].size = size;

    for (unsigned i = 1; i < 32; i++)
    {
        tmp[g_memBufSize + i].ptr = NULL;
        tmp[g_memBufSize + i].size = 0;
    }

    g_memBufSize += 32;
    g_memBuf = tmp;

    if (g_mTrackLaw == MTRACK_DEBUG_LAW && g_mdebugMemtrack)
    {
        printf("[MEMTRACK]: Buffer size increased by 32 elements to %lld, block of size %lld appended\n", g_memBufSize, size);
    }
}

// Remove entry from buffer if size 0
// Find index of entry if size > 0
int __resetAllocated(void* ptr, size_t size)
{
    if (!ptr || g_mTrackLaw < 2)
    {
        return 0;
    }

    if (!g_memBuf || g_mTrackLaw == MTRACK_DEGRADED_LAW)
    {
        if (size == 0)
        {
            g_memAllocated--;
        }

        return 1;
    }

    for (unsigned i = 0; i < g_memBufSize; i++)
    {
        if (g_memBuf[i].ptr == ptr)
        {
            if (size)
            {
                long n = size - g_memBuf[i].size;

                if (n > 0 && __sizeOverflow(n))
                {
                    return 0;
                }

                return i;
            }

            g_memAllocated -= g_memBuf[i].size;

            g_memBuf[i].ptr = NULL;
            g_memBuf[i].size = 0;
            
            return 1;
        }
    }

    return 0;
}

// Append ptr to free buffer and increase buffer size if necessary
// Same for freeInfo buffer if memtrack is in debug law
void __appendFree(void* ptr, const char* filename, unsigned linenum)
{
    if (!ptr || !g_memFreeBuf || g_mTrackLaw < 3)
    {
        return;
    }

    if (g_freeBufIdx >= g_freeBufSize)
    {
        g_freeBufIdx = g_freeBufSize;
        g_freeBufSize += 32;

        void** tmp = realloc(g_memFreeBuf, g_freeBufSize * sizeof(void*));

        if (!tmp)
        {
            goto fail;
        }

        g_memFreeBuf = tmp;

        if (g_mTrackLaw == MTRACK_DEBUG_LAW)
        {
            char** info = realloc(g_freeInfoBuf, g_freeBufSize * sizeof(char*));

            if (!info)
            {
                goto fail;
            }

            g_freeInfoBuf = info;
        }
    }

    if (g_mTrackLaw == MTRACK_DEBUG_LAW)
    {
        char* str = malloc(MTRACK_MAX_STRING_LEN * sizeof(char));

        snprintf(str, MTRACK_MAX_STRING_LEN, "file: %s, line %d", filename, linenum);

        g_freeInfoBuf[g_freeBufIdx] = str;
    }
    
    g_memFreeBuf[g_freeBufIdx++] = ptr;

    return;

    fail:
        
    rDebugString("[MEMTRACK]: Buffer realloc failed, memtrack running in degraded law");

    free(g_memFreeBuf);
    free(g_freeInfoBuf);

    g_memFreeBuf = NULL;
    g_freeInfoBuf = NULL;
    g_freeBufSize = 0;
    g_mTrackLaw = MTRACK_DEGRADED_LAW;
}

// Return index if ptr is in free buffer
int __isFree(void* ptr)
{
    if (!ptr || !g_memFreeBuf || g_mTrackLaw < 3)
    {
        return -1;
    }

    for (size_t i = 0; i < g_freeBufSize; i++)
    {
        if (g_memFreeBuf[i] == ptr)
        {
            return i;
        }
    }

    return -1;
}

// ========================================================================== //
//                                                                            //
//  Memtrack implementation                                                   //
//                                                                            //
// ========================================================================== //

// Optional explicit initialization
void memtrackInitialize_Implementation(control_law law)
{
    if (g_mTrackLaw != MTRACK_UNINITIALIZED)
    {
        rDebugString("[MEMTRACK]: Already initialized");
        return;
    }

    if (law == MTRACK_UNINITIALIZED)
    {
        rDebugString("[MEMTRACK]: Initialization failed, invalid control law");
        return;
    }

    #ifndef MTRACK_BUF_SIZE
        #define MTRACK_BUF_SIZE 256
        rDebugString("[MEMTRACK]: MTRACK_BUF_SIZE not defined, defaulting to 256");
    #endif

    if (MTRACK_BUF_SIZE <= 0)
    {
        rDebugString("[MEMTRACK]: Memtrack initialization failed, invalid MTRACK_BUF_SIZE definition");
        return;
    }

    #ifndef MTRACK_MANUAL_CLEANUP
        atexit(memtrackCleanup_Implementation);
    #endif

    if (law > 4)
    {
        rDebugString("[MEMTRACK]: Invalid memtrack mode on initialization, memtrack running in normal law");

        law = MTRACK_NORMAL_LAW;
    }

    g_mTrackLaw = law;

    if (law < 3)
    {
        rAssert(law);
        return;
    }

    g_memBuf = malloc(MTRACK_BUF_SIZE * sizeof(mblock_t));
    g_memFreeBuf = malloc(MTRACK_BUF_SIZE * sizeof(void*));

    g_memBufSize = MTRACK_BUF_SIZE;
    g_freeBufSize = MTRACK_BUF_SIZE;
    g_memAllocated = 0;

    if (g_mTrackLaw == MTRACK_DEBUG_LAW)
    {
        g_freeInfoBuf = malloc(MTRACK_BUF_SIZE * sizeof(char*));
    }

    if (g_memBuf && g_memFreeBuf && (g_mTrackLaw != MTRACK_DEBUG_LAW || g_freeInfoBuf))
    {
        for (unsigned i = 0; i < g_memBufSize; i++)
        {
            g_memBuf[i].ptr = NULL;
            g_memBuf[i].size = 0;

            g_memFreeBuf[i] = NULL;

            if (g_freeInfoBuf)
            {
                g_freeInfoBuf[i] = NULL;
            }
        }

        g_memBuf[0].ptr = g_memBuf;
        g_memBuf[0].size = g_memBufSize * sizeof(mblock_t);
    }
    else
    {
        rDebugString("Memtrack buffer initialization error, memtrack running in degraded law");

        g_mTrackLaw = MTRACK_DEGRADED_LAW;
        g_memBuf = NULL;
        g_memFreeBuf = NULL;
        g_freeInfoBuf = NULL;
        g_memBufSize = 0;
        g_freeBufSize = 0;
    }

    if (g_mTrackLaw == MTRACK_DEBUG_LAW && g_mdebugMemtrack && g_memBuf && g_memFreeBuf)
    {
        printf("[MEMTRACK]: Initialization successful, memtrack buffers have been allocated: %lld bytes\n", g_memBufSize);
    }
}

void memtrackChangeLaw_Implementation(control_law law)
{
    if (g_mTrackLaw == MTRACK_UNINITIALIZED)
    {
        memtrackInitialize_Implementation(law);
        return;
    }

    if (law < 1 || law > 4)
    {
        rDebugString("[MEMTRACK]: Invalid memtrack control law");
        return;
    }

    if (law > 2 && (g_memBuf == NULL || g_memFreeBuf == NULL))
    {
        rDebugString("[MEMTRACK]: Buffers not initialized, control law could not be changed");
        return;
    }

    if ((g_mTrackLaw < 3 && law > 2) || (g_mTrackLaw > 2 && law < 3))
    {
        rDebugString("[MEMTRACK]: Switching between normal/debug and degraded/direct law is not recommended");
    }

    g_mTrackLaw = law;
}

void memtrackSetup_Implementation(control_law law, bool dAlloc, bool dRealloc, bool dFree, bool dMtrack)
{
    if (g_mTrackLaw == MTRACK_UNINITIALIZED)
    {
        memtrackInitialize_Implementation(law);
    }
    else if (law && law <= 4)
    {
        rWarningMsg(law == MTRACK_DEBUG_LAW, "Memtrack setup only takes effect if law is debug");

        g_mTrackLaw = law;
    }
    else
    {
        rDebugString("[MEMTRACK]: Setup failed, invalid control law");
        return;
    }
    
    g_mdebugAllocs = dAlloc;
    g_mdebugReallocs = dRealloc;
    g_mdebugFrees = dFree;
    g_mdebugMemtrack = dMtrack;
}

void* memtrackAllocate_Implementation(size_t size)
{
    if (g_mTrackLaw == MTRACK_UNINITIALIZED)
    {
        memInit();
    }

    if (size <= 0 || __sizeOverflow(size))
    {
        rDebugPrintf("[MEMTRACK]: Memory not allocated, size: %lld\n", size);
        return NULL;
    }

    void* ptr = malloc(size);

    rAssert(ptr);

    if (ptr)
    {
        __setAllocated(ptr, size);
    }

    return ptr;
}

void* memtrackAllocateInitialize_Implementation(size_t size, size_t count)
{
    if (g_mTrackLaw == MTRACK_UNINITIALIZED)
    {
        memInit();
    }

    if (size <= 0 || count <= 0 || __sizeOverflow(size * count))
    {
        rDebugPrintf("[MEMTRACK]: Memory not allocated, size: %lld\n", size * count);

        return NULL;
    }

    void* ptr = calloc(count, size);

    rAssert(ptr);

    if (ptr)
    {
        __setAllocated(ptr, size * count);
    }

    return ptr;
}

void* memtrackAllocateSet_Implementation(size_t size, unsigned char value)
{
    void* ptr = memtrackAllocate_Implementation(size);

    if (ptr == NULL)
    {
        return NULL;
    }

    memset(ptr, value, size);

    return ptr;
}

void* memtrackReallocate_Implementation(void* ptr, size_t size)
{
    rAssert(ptr);
    rAssert(size);

    if (g_mTrackLaw == MTRACK_UNINITIALIZED)
    {
        memInit();
    }

    int i = __resetAllocated(ptr, size);

    if (i == 0)
    {
        goto fail;
    }

    if (size == 0)
    {
        memtrackFree_Implementation(ptr, NULL, 0);
        
        rDebugString("[MEMTRACK]: Realloc called with size 0, memory block freed");

        return NULL;
    }

    if (g_mTrackLaw > 2 && g_memBuf && g_memBuf[i].size == size)
    {
        rDebugString("[MEMTRACK]: No realloc necessary");

        return ptr;
    }

    void* tmp = realloc(ptr, size);

    if (tmp)
    {
        if (g_mTrackLaw > 2 && g_memBuf)
        {
            g_memAllocated += size - g_memBuf[i].size;

            g_memBuf[i].size = size;
            g_memBuf[i].ptr = tmp;
        }

        return tmp;
    }

    fail:
    
    rDebugString("[MEMTRACK]: Realloc failed");

    return NULL;
}

int memtrackFree_Implementation(void* ptr, const char* filename, unsigned linenum)
{
    if (g_mTrackLaw == MTRACK_UNINITIALIZED)
    {
        memInit();
    }

    if (!ptr)
    {
        return 1;
    }

    if (__resetAllocated(ptr, 0))
    {
        __appendFree(ptr, NULL, 0);

        free(ptr);

        return 2;
    }
    else
    {
        int i = __isFree(ptr);

        if (i >= 0 && g_mTrackLaw == MTRACK_DEBUG_LAW && g_freeInfoBuf)
        {
            printf
            (
                "[FATAL]: Pointer freed in file %s, line %d has already been freed\n"
                "         Pointer was first freed in %s\n",
                rNullStringWrap(filename), linenum, rNullStringWrap(g_freeInfoBuf[i])
            );

            return 0;
        }
        else if (i >= 0)
        {
            rDebugPrintf("[MEMTRACK]: Pointer %p has already been freed", ptr);
            return 0;
        }
    }
    
    rDebugString("[MEMTRACK]: Memory block not found in memtrack buffer, block not freed");

    return 0;
}

// Display warning if set amount of memory is exceeded
void memtrackSetWarning_Implementation(size_t size)
{
    if (g_mTrackLaw == MTRACK_UNINITIALIZED)
    {
        memInit();
    }

    rAssert(g_memBuf);

    rWarningMsg(g_mTrackLaw < 3, "Memtrack warning only takes effect in normal or debug law");

    g_mTrackWarning = size;

    if (g_mTrackLimit)
    {
        rWarningMsg(g_mTrackLimit <= size, "Memtrack warning size bigger than set limit");
    }
}

// Prevent memory allocation beyond set limit
void memtrackSetLimit_Implementation(size_t size)
{
    if (g_mTrackLaw == MTRACK_UNINITIALIZED)
    {
        memInit();
    }

    rAssert(g_memBuf);

    rWarningMsg(g_mTrackLaw < 3, "Memtrack limit only takes effect in normal or debug law");

    g_mTrackLimit = size;
}

// Unless MTRACK_MANUAL_CLEANUP is defined, calling cleanup is optional
void memtrackCleanup_Implementation(void)
{
    if (g_mTrackLaw != MTRACK_UNINITIALIZED || !g_memBuf)
    {
        rWarning(g_memAllocated);
        return;
    }

    size_t allocated = 0;

    if (g_memAllocated)
    {
        for (size_t i = 1; i < g_memBufSize; i++)
        {
            if (g_memBuf[i].ptr && g_memBuf[i].size)
            {
                allocated += g_memBuf[i].size;

                free(g_memBuf[i].ptr);

                rDebugPrintf("[MEMTRACK]: Memory block with %lld bytes was freed automatically", g_memBuf[i].size);
            }
        }

        rWarningMsg(g_memAllocated == allocated, "Allocated mem sizes disagree");

        if (allocated)
        {
            rDebugPrintf("[MEMTRACK]: Total bytes freed: %lld", allocated);
        }
    }

    if (g_freeInfoBuf)
    {
        for (size_t i = 0; i < g_freeBufSize; i++)
        {
            free(g_freeInfoBuf[i]);
        }
    }

    free(g_memBuf);
    free(g_memFreeBuf);
    free(g_freeInfoBuf);

    g_memBuf = NULL;
    g_memFreeBuf = NULL;
    g_freeInfoBuf = NULL;
    g_memAllocated = 0;
    g_memBufSize = 0;
    g_freeBufSize = 0;
    g_mTrackLaw = MTRACK_UNINITIALIZED;
}

// ========================================================================== //
//                                                                            //
//  Memtrack debug law implementation                                         //
//                                                                            //
// ========================================================================== //

void* memtrackAllocate_Debug(size_t size, const char* filename, unsigned linenum)
{
    void* tmp = memtrackAllocate_Implementation(size);

    if (tmp && g_mTrackLaw == MTRACK_DEBUG_LAW && g_mdebugAllocs)
    {
        printf("[MEMTRACK]: Memory block of size %lld allocated. File %s - Line %d\n", size, filename, linenum);
    }

    return tmp;
}

void* memtrackAllocateInitialize_Debug(size_t size, size_t count, const char* filename, unsigned linenum)
{
    void* tmp = memtrackAllocateInitialize_Implementation(size, count);

    if (tmp && g_mTrackLaw == MTRACK_DEBUG_LAW && g_mdebugAllocs)
    {
        printf("[MEMTRACK]: Memory block of size %lld allocated and initialized. File %s - Line %d\n", size * count, filename, linenum);
    }

    return tmp;
}

void* memtrackAllocateSet_Debug(size_t size, unsigned char value, const char* filename, unsigned linenum)
{
    void* tmp = memtrackAllocateSet_Implementation(size, value);

    if (tmp && g_mTrackLaw == MTRACK_DEBUG_LAW && g_mdebugAllocs)
    {
        printf("[MEMTRACK]: Memory block of size %lld allocated and initialized. File %s - Line %d\n", size, filename, linenum);
    }

    return tmp;
}

void* memtrackReallocate_Debug(void* ptr, size_t size, const char* filename, unsigned linenum)
{
    void* tmp = memtrackReallocate_Implementation(ptr, size);

    if (tmp && g_mTrackLaw == MTRACK_DEBUG_LAW && g_mdebugReallocs)
    {
        printf("[MEMTRACK]: Memory block reallocated to size %lld. File %s - Line %d\n", size, filename, linenum);
    }

    return tmp;
}

void memtrackFree_Debug(void* ptr, const char* filename, unsigned linenum)
{
    int ret = memtrackFree_Implementation(ptr, filename, linenum);

    if (g_mTrackLaw == MTRACK_DEBUG_LAW && g_mdebugFrees)
    {
        if (ret == 1)
        {
            printf("[MEMTRACK]: Null pointer not freed. File %s - Line %d\n", filename, linenum);
        }
        else if (ret == 2)
        {
            printf("[MEMTRACK]: Memory block freed. File %s - Line %d\n", filename, linenum);
        }
        else
        {
            printf("[MEMTRACK]: Error freeing memory address %p. File %s - Line %d\n", ptr, filename, linenum);
        }
    }
}