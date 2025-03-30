#ifndef MAIN_H
#define MAIN_H

// Build configuration
//#define R_UNIX
#define R_WIN32

#define R_DEBUG
#define RDEBUG_LOG
#define RDEBUG_TIME

// C_UNIX and C_WIN32 should only be passed by compiler
#if defined C_UNIX && defined R_WIN32
    #undef R_WIN32
    #ifndef R_UNIX
        #define R_UNIX
    #endif
#endif

#if defined C_WIN32 && defined R_UNIX
    #undef R_UNIX
    #ifndef R_WIN32
        #define R_WIN32
    #endif
#endif

// Build configuration check
#if defined R_WIN32 && defined R_UNIX
    #error 'Definition of both R_WIN32 and R_UNIX'
#endif

#if defined C_WIN32 && defined C_UNIX
    #error 'Definition of both C_WIN32 and C_UNIX'
#endif

#include <stdlib.h>

//
//  Constants
//

// HTTP headers
#define H_KEEPALIVE         0
#define H_CLOSE             1  
#define H_CTYEXF            2
#define H_CTYPMPT           3
#define H_FACESRQ           4
#define H_XMLHTTP           5
#define H_REFERER           6
#define H_HOST              7
#define H_ORIGIN            8
#define H_UPGRADE           9
#define H_STDACPT           10
#define H_XMLACPT           11
#define H_JSSN              12
#define H_JSSNMXRDR         13
#define H_JSSNMX            14

// Multipart request configs
#define CONF_CHTAB          0
#define CONF_ADDLN          1
#define CONF_SELPRF         2
#define CONF_CMTLN          3
#define CONF_SAVE           4

#define FILE_IN_BUF_SIZE    65535
#define RGET_BUF_SIZE       1024
#define RPOST_BUF_SIZE      16000
#define RCV_BUF_SIZE        65535
#define HDR_BUF_SIZE        2048

//
//  Typedefs
//
#ifdef R_UNIX
    #include <unistd.h>
    typedef __socklen_t socklen_t;
#endif

typedef signed long long i64;
typedef unsigned long long u64;

typedef unsigned char bool;

typedef struct import_s import_t;       // definition in fileio.h
typedef struct config_s config_t;       // definition in fileio.h
typedef struct strnode_s strnode_t;     // definition in fileio.h

typedef struct field_s {
    u64 idx;
    const char* val;
} field_t;

//
//  Functions
//
void addProfile(import_t* info, config_t* conf);

config_t* setupConfig(const char* filename);
import_t* setupImport(const char* filename);
void      setupLogFile(void);
void      setupFieldConfigs(field_t* selprf, field_t* cmtln, import_t* info, config_t* conf);

void login(int socket, char* txbuf, size_t size, char* ssnbuf);
int  openSocket(const char* url, const char* port);
int  sockWrite(int socket, const char* txbuf, size_t reqlen);
int  sockRead(int socket, char* rxbuf, size_t bufsize);

void sendRequest(int socket, const char* txbuf, size_t size, char* ssntbuf, char* vstbuf, int expected, bool bypassSSN);
void sendRequestIgnoreResponse(int socket, const char* txbuf, size_t size, int expected);
char** sendRequestParseFields(int socket, const char* txbuf, size_t size, const char** fields, size_t fldSize, int expected);

void configureFields(const char** fields, size_t fldSize, field_t* valbuf, unsigned config);
void error(const char* msg);
void assertFieldValues(char** flds, config_t* conf);
void assertStatusCode(int expected, char* rxbuf);

#endif