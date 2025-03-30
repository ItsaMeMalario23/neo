#ifndef FILEIO_H
#define FILEIO_H

#include <main.h>

struct import_s {
    char        badgeNo[16];        // TODO check max len
    char        persNo[8];
    char        validFrom[16];
    char        validTo[16];
    char        profile[8];
    import_t*   next;
};

struct config_s {
    unsigned char   noFirstName;
    unsigned char   noLastName;
    unsigned char   alphaNumName;
    unsigned char   noDepartment;
    unsigned char   blockingReason;
    unsigned char   numProfiles;
    unsigned char   profiles[34];     // range 0..255
    strnode_t*      attendance;       // linked list for allowed attendance values
    strnode_t*      officeRelease;    // linked list for allowed office release values
    char*           serverUrl;
    char*           serverPort;
    char*           filePath;
};

struct strnode_s {
    char* val;
    strnode_t* next;
};

//
//  Functions for import CSV file
//
void readImportFile(const char* filename, import_t** data);
void freeImportData(import_t* data);

unsigned parseVal(const char* buf, int len, char* dst, size_t dstsize);

//
//  Functions for config
//
config_t* readConfigFile(const char* filename);

size_t parseByteVal(const char* buf, i64 len, unsigned char* dst);
size_t parseByteList(const char* buf, i64 len, unsigned char* dst, unsigned char* dstlen, unsigned char dstmaxlen);
size_t parseStr(const char* buf, i64 len, char** dst);
size_t parseStrList(const char* buf, i64 len, strnode_t** dst);

void validImportPath(const char* path);
void freeConfigData(config_t* data);

void moveImportFile(const char* filename, bool success);

// local functions
size_t readUntilVal(const char* buf, i64 len);
size_t readUntilNewline(const char* buf, i64 len);

#endif