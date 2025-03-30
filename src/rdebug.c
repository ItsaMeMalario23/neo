#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <rdebug.h>

//
//  Globals
//
FILE* g_debugStream = NULL;

char g_debugLog = 0;
char g_debugTime = 0;
char g_haltOnAsserts = 1;

const char* g_errorLevels[4] = {"[INFO]:", "[WARNING]:", "[ERROR]:", "[FATAL]:"};

// Initialize output stream
void __init(void)
{
    #ifdef RDEBUG_TIME
        g_debugTime = 1;
    #else
        g_debugTime = 0;
    #endif

    g_debugStream = stdout;
    g_debugLog = 0;
}

// Close log file
void __cleanup(void)
{
    if (g_debugLog && g_debugStream)
    {
        fclose(g_debugStream);
        g_debugStream = NULL;
    }
}

// Enable debug break / execution halt on assert fail
void rDebugHaltOnAsserts_Implementation(char halt)
{
    g_haltOnAsserts = halt;
}

// Set output stream for debug messages
// Expects either filename or "stdout" / "stderr"
void rDebugOutputStream_Implementation(const char* stream)
{
    if (g_debugStream)
    {
        fprintf(g_debugStream, "[DEBUG]: Output stream already initialized\n");
        return;
    }

    __init();

    if (!stream || stream[0] == '\0')
    {
        fprintf(stdout, "[DEBUG]: Empty or null debug output stream indentifier, deferring to stdout\n");
        return;
    }

    FILE* fd = NULL;

    #ifndef RDEBUG_LOG

        if (strncmp(stream, "stdout", 6) == 0)
        {
            fd = stdout;
        }
        else if (strncmp(stream, "stderr", 6) == 0)
        {
            fd = stderr;
        }

    #endif

    if (!fd)
    {
        fd = fopen(stream, "a+");

        if (!fd)
        {
            fprintf(stdout, "[DEBUG]: Error opening log file \"%s\", deferring to stdout\n", rNullStringWrap(stream));

            fd = stdout;
        }
        else
        {
            g_debugLog = 1;

            atexit(__cleanup);

            fputs("\n\n", fd);
        }
    }

    g_debugStream = fd;
}

// Invoked if assert fails
void rAssertFail_Implementation(const char* condition, const char* filename, unsigned linenum)
{
    if (!g_debugStream)
    {
        __init();
    }

    if (g_debugTime)
    {
        time_t now = time(NULL);

        char* timestamp = ctime(&now);

        timestamp[strnlen(timestamp, 64) - 1] = '\0';

        fprintf(g_debugStream, "%s: ", &timestamp[4]);
    }

    fprintf
    (
        g_debugStream,
        "\n"
        " ASSERT FAILED: [ %s ]\n"
        "          Line: [ %d ]\n"
        "          File: [ %s ]\n",
        condition, linenum, filename
    );

    if (g_haltOnAsserts)
    {
        rReleaseBreak();
    }
}

// Invoked if warning fails
void rWarningFail_Implementation(const char* condition, const char* filename, unsigned linenum)
{
    if (!g_debugStream)
    {
        __init();
    }

    if (g_debugTime)
    {
        time_t now = time(NULL);

        char* timestamp = ctime(&now);

        timestamp[strnlen(timestamp, 64) - 1] = '\0';

        fprintf(g_debugStream, "%s: ", &timestamp[4]);
    }

    fprintf
    (
        g_debugStream,
        "\n"
        " Warning: [ %s ]\n"
        "    Line: [ %d ]\n"
        "    File: [ %s ]\n",
        condition, linenum, filename
    );
}

// Debug message with optional error level
void rDebugString_Implementation(unsigned errorlevel, const char* string)
{
    if (!g_debugStream)
    {
        __init();
    }

    if (g_debugTime)
    {
        time_t now = time(NULL);

        char* timestamp = ctime(&now);

        timestamp[strnlen(timestamp, 64) - 1] = '\0';

        fprintf(g_debugStream, "%s: ", &timestamp[4]);
    }

    if (errorlevel)
    {
        fprintf(g_debugStream, "%s %.*s\n", g_errorLevels[--errorlevel], RDEBUG_STRING_MAX_LEN, rNullStringWrap(string));
    }
    else
    {
        fprintf(g_debugStream, "[DEBUG]: %.*s\n", RDEBUG_STRING_MAX_LEN, rNullStringWrap(string));
    }

    if (g_debugLog && ferror(g_debugStream))
    {
        fclose(g_debugStream);

        __init();

        fprintf(g_debugStream, "[DEBUG]: Log file error, deferring to stdout\n");
    }
}

void rReleasePrintf(const char* fmt, ... )
{
    va_list args;

    va_start(args, fmt);

    char string[RDEBUG_STRING_MAX_LEN];

    vsnprintf(string, RDEBUG_STRING_MAX_LEN, rNullStringWrap(fmt), args);

    string[RDEBUG_STRING_MAX_LEN - 1] = '\0';

    va_end(args);

    rDebugString_Implementation(0, string);
}

#ifndef R_RELEASE

void rDebugPrintf(const char* fmt, ... )
{
    va_list args;

    va_start(args, fmt);

    char string[RDEBUG_STRING_MAX_LEN];

    vsnprintf(string, RDEBUG_STRING_MAX_LEN, rNullStringWrap(fmt), args);

    string[RDEBUG_STRING_MAX_LEN - 1] = '\0';

    va_end(args);

    rDebugString_Implementation(0, string);
}

#endif