#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <fileio.h>
#include <rdebug.h>
#include <memtrack.h>

// returns linked list with all parsed lines
void readImportFile(const char* filename, import_t** data)
{
    rAssert(filename);
    rAssert(data);

    FILE* fd = fopen(filename, "r");

    if (!fd) {
        rDebugStringLevel(ERR_FTL, "Failed to open import file, aborting execution\n");
        exit(0);
    }

    char inputbuf[FILE_IN_BUF_SIZE];

    size_t len = fread(inputbuf, 1, FILE_IN_BUF_SIZE, fd);

    if (len >= FILE_IN_BUF_SIZE)
        error("Unexpected import file size ( > 65535b )");

    // cygwin implementation of stdio does not remove carriage returns so here we go
    for (size_t k = 0; k < len - 1; k++) {
        if (inputbuf[k] == '\r')
            memmove(&inputbuf[k], &inputbuf[k + 1], (len-- - k) * sizeof(char));
    }

    *data = NULL;

    import_t* current = NULL;

    size_t i = 0;

    while (i < len) {
        // clear leading whitespaces and newlines
        while ((inputbuf[i] == ' ' || inputbuf[i] == '\n') && ++i < len);

        // skip headline
        if (i + 18 < len && strncmp(&inputbuf[i], "MATRIX CSV IMPORT\n", 18) == 0)
            i += 18;

        // check if there is enough len for a valid line
        if (i + 28 >= len) {
            break;
        } else {
            import_t* tmp = (import_t*) memAlloc(sizeof(import_t));

            if (current)
                current->next = tmp;
            else
                *data = tmp;

            current = tmp;
        }

        if (!current)
            error("Memory allocation for file inpurt failed");

        // update flag
        i += parseVal(&inputbuf[i], len - i, NULL, 0);

        // employee id
        i += parseVal(&inputbuf[i], len - i, current->persNo, 8);

        // badge id
        i += parseVal(&inputbuf[i], len - i, current->badgeNo, 16);

        // valid from
        i += parseVal(&inputbuf[i], len - i, current->validFrom, 16);

        // valid to
        i += parseVal(&inputbuf[i], len - i, current->validTo, 16);

        // profile id
        i += parseVal(&inputbuf[i], len - i, current->profile, 8);
    }

    if (fd)
        fclose(fd);

    if (*data == NULL)
        error("Error parsing import file");

    if (current)
        current->next = NULL;
}

// free import info list
void freeImportData(import_t* data)
{
    while (data) {
        import_t* tmp = data->next;

        memFree(data);

        data = tmp;
    }
}

unsigned parseVal(const char* buf, int len, char* dst, size_t dstsize)
{
    rAssert(buf);

    if (len <= 0)
        error("Error parsing value from import file: unexpected eof");

    size_t i = 0;

    // find len
    while (buf[i] != ';' && buf[i] != '\n' && ++i < len);

    if (!i)
        error("Error parsing value from import file: unexpected zero-length value");

    if (dst) {
        if (i >= dstsize)
            error("Error parsing value from import file: unexpected value length");

        memcpy(dst, buf, i * sizeof(char));
        dst[i] = '\0';

    } else if (i != 1 || buf[i - 1] != 'U') {
        error("Error parsing value from import file: missing update identifier");
    }

    return ++i;
}

// ========================================================================== //
//                                                                            //
//  EXPECTED CONFIG FILE CONTENT:                                             //
//                                                                            //
//  allowNoFirstName: [0/1]                                                   //
//  allowNoLastName: [0/1]                                                    //
//  allowNonAlphaNumericName: [0/1]                                           //
//  allowNoDepartment: [0/1]                                                  //
//  allowAttendance: [val], [val], ...                                        //
//  allowBlockingReason: [0/1]                                                //
//  allowOfficeRelease: [val], [val], ...                                     //
//  allowAccessProfiles: [val], [val], ...      (max 34, range 0..255)        //
//                                                                            //
// ========================================================================== //

config_t* readConfigFile(const char* filename)
{
    FILE* fd = fopen(filename, "r");

    if (!fd)
        error("Failed to open config file");

    char inputbuf[FILE_IN_BUF_SIZE];

    size_t len = fread(inputbuf, 1, FILE_IN_BUF_SIZE, fd);

    if (len >= FILE_IN_BUF_SIZE)
        error("Unexpected config file size ( > 65535b )");

    config_t* data = (config_t*) memAlloc(sizeof(config_t));

    if (!data)
        error("Failed to allocate memory for config data");

    // parse allowNoFirstName
    size_t i = parseByteVal(inputbuf, len, &data->noFirstName);

    // parse allowNoLastName
    i += parseByteVal(inputbuf + i, len - i, &data->noLastName);

    // parse allowNonAlphaNumericName
    i += parseByteVal(inputbuf + i, len - i, &data->alphaNumName);

    // parse allowNoDepartment
    i += parseByteVal(inputbuf + i, len - i, &data->noDepartment);

    // parse allowAttendance
    i += parseStrList(inputbuf + i, len - i, &data->attendance);

    // parse allowBlockingReason
    i += parseByteVal(inputbuf + i, len - i, &data->blockingReason);

    // parse allowOfficeRelease
    i += parseStrList(inputbuf + i, len - i, &data->officeRelease);

    // parse allowAccessProfiles
    i += parseByteList(inputbuf + i, len - i, data->profiles, &data->numProfiles, 34);

    // parse serverUrl
    i += parseStr(inputbuf + i, len - i, &data->serverUrl);

    // parse serverPort
    i += parseStr(inputbuf + i, len -i, &data->serverPort);

    // parse importFilePath
    i += parseStr(inputbuf + i, len - i, &data->filePath);

    return data;
}

size_t parseByteVal(const char* buf, i64 len, unsigned char* dst)
{
    if (len < 2 || !dst || !buf)
        error("Error parsing config file: bval null ref");

    size_t i = readUntilVal(buf, len);

    // parse value
    if (buf[i] == '0')
        *dst = 0;
    else if (buf[i] == '1')
        *dst = 1;
    else
        error("Error parsing config file: bval unexpected value");

    i++;

    // clear trailing whitespaces
    while (buf[i] == ' ' && ++i < len);

    // expect newline or eof
    if (i >= len || buf[i] == '\n')
        return ++i;
    else
        error("Error parsing config file: bval unexpected token");
}

size_t parseByteList(const char* buf, i64 len, unsigned char* dst, unsigned char* dstlen, unsigned char dstmaxlen)
{
    if (len < 2 || !dst || !dstlen || !dstmaxlen || !buf)
        error("Error parsing config file: blist null ref");

    size_t i = readUntilVal(buf, len);
    size_t vln;

    unsigned dstidx = 0;

    for (;;) {
        // clear leading whitespaces
        while (buf[i] == ' ')
            if (++i >= len)
                break;

        if (buf[i] > 57 || buf[i] < 48)
            error("Error parsing config file: unexpected character in profile ID");

        vln = 0;

        // read number
        while (buf[i + vln] > 47 && buf[i + vln] < 58 && ++vln + i < len);

        if (!vln)
            error("Error parsing config file: zero-length value");
        else if (vln > 3)
            error("Error parsing config file: profile ID > 255");

        char numbuf[4];

        memcpy(numbuf, &buf[i], vln * sizeof(char));

        numbuf[vln] = '\0';

        int val = atoi(numbuf);

        if (val > 255 || val < 0)
            error("Error parsing config file: invalid profile ID");

        if (dstidx >= dstmaxlen)
            error("Error parsing config file: too many profile IDs");

        dst[dstidx++] = val;

        i += vln;

        // read until delimiter or eof
        while ((buf[i] != ',' && buf[i] != '\n') && ++i < len);

        i++;

        if (i >= len || buf[i - 1] == '\n')
            break;
    }

    if (!dstidx)
        error("Error parsing config file: no profile IDs found");

    *dstlen = dstidx;

    return i;
}

size_t parseStr(const char* buf, i64 len, char** dst)
{
    if (len < 2 || !buf || !dst)
        error("Error parsing config file: str null ref");

    size_t i = readUntilVal(buf, len);
    size_t strln = 0;

    if (buf[i] != '\"')
        error("Error parsing string: expeced \"");

    if (len - i < 2 || buf[i + 1] == '\"')
        error("Error parsing string: unexpected string length");

    while (++strln + i < len && buf[strln + i] != '\"');

    if (buf[strln + i] != '\"')
        error("Error parsing string: expected \"");

    *dst = (char*) memAlloc(strln * sizeof(char));

    rReleaseAssert(*dst);

    memcpy(*dst, buf + i + 1, (strln - 1) * sizeof(char));

    (*dst)[strln - 1] = '\0';

    // read until LF or EOF
    while (++strln + i < len && buf[strln + i] != '\n');

    return ++strln + i;
}

size_t parseStrList(const char* buf, i64 len, strnode_t** dst)
{
    if (len < 2 || !dst || !buf)
        error("Error parsing config file: strlist null ref");

    size_t i = readUntilVal(buf, len);
    size_t vlen, trlen;

    strnode_t* prev = NULL;

    *dst = NULL;

    for (;;) {
        strnode_t* node = (strnode_t*) memAlloc(sizeof(strnode_t));

        if (!node)
            error("Memory allocation failed");

        node->val = NULL;
        node->next = NULL;

        if (*dst == NULL)
            *dst = node;

        if (prev)
            prev->next = node;

        // clear leading whitespaces
        while (buf[i] == ' ')
            if (++i >= len)
                break;

        vlen = 0;

        // read value
        while (buf[i + vlen] != ',' && buf[i + vlen] != '\n' && ++vlen + i < len);

        trlen = vlen;

        // clear trailing whitespaces
        while (buf[i + vlen - 1] == ' ' && --vlen > 0);

        if (!vlen)
            error("Error parsing config file: strlist zero-length value");

        node->val = (char*) memAlloc((vlen + 1) * sizeof(char));

        if (!node->val)
            error("Memory allocation failed");

        memcpy(node->val, &buf[i], vlen * sizeof(char));

        node->val[vlen] = '\0';

        i += ++trlen;

        if (i >= len || buf[i - 1] == '\n')
            break;

        prev = node;
    }

    if (*dst == NULL || !(*dst)->val)
        error("Error parsing config file: strlist no values found");

    return i;
}

void validImportPath(const char* path)
{
    size_t len = strnlen(path, 256);

    if (len < 5)
        error("Error in config file: invalid import path");

    if (strncmp(path + len - 4, ".csv", len))
        error("Error in config file: invalid import path file extension");
}

// free config object
void freeConfigData(config_t* data)
{
    if (!data)
        return;

    strnode_t* curr = data->attendance;
    strnode_t* tmp;

    while (curr) {
        tmp = curr->next;

        memFree(curr->val);
        memFree(curr);

        curr = tmp;
    }

    curr = data->officeRelease;

    while (curr) {
        tmp = curr->next;

        memFree(curr->val);
        memFree(curr);

        curr = tmp;
    }

    memFree(data);
}

void moveImportFile(const char* filename, bool success)
{
    char* dstpath = NULL;

    if (success)
        dstpath = "log/importfiles/";
    else
        dstpath = "log/failedimports/";

    #ifdef R_WIN32

    unsigned len = strnlen(filename, 256);
    int idx = -1;

    for (unsigned i = 0; i < len; i++) {
        if (filename[i] == '/' || filename[i] == '\\')
            idx = i;
    }

    if (++idx + 4 >= len)
        error("Error moving import file, unexpected filepath");

    char cmdbuf[512];

    time_t t = time(NULL);
    struct tm ts;

    localtime_s(&ts, &t);

    len = snprintf
    (
        cmdbuf, 512,
        "move \"%s\" \"%s%d-%d-%d-%d-%d-%d-%s\"",
        filename, dstpath, ts.tm_year + 1900, ts.tm_mon + 1, ts.tm_mday, ts.tm_hour, ts.tm_min, ts.tm_sec, filename + idx
    );

    if (len > 512)
        error("Error moving import file, command buffer full");

    FILE* pipe = _popen(cmdbuf, "r");

    if (!pipe)
        error("Failed to open pipe");

    while (fgets(cmdbuf, 512, pipe))
        rDebugPrintf("Piped output: %s", cmdbuf);

    idx = _pclose(pipe);

    rDebugPrintf("Pipe retval: %d", idx);

    #endif
}

size_t readUntilVal(const char* buf, i64 len)
{
    if (len < 2)
        error("Error parsing config file: readuntil null ref");

    size_t i = 0;

    // clear leading whitespaces and newlines, skip comments
    while (buf[i] == ' ' || buf[i] == '\n' || buf[i] == '#') {
        if (buf[i] == '#') {
            i += readUntilNewline(buf + i, len - i);
            continue;
        }

        if (++i >= len - 2)
            error("Error parsing config file: readuntil eof before identifier");
    }

    // read until value
    while (buf[i] != ':') {
        if (++i >= len - 1)
            error("Error parsing config file: readuntil eof before dlm");
    }

    i++;

    // clear leading whitespaces
    while (buf[i] == ' ')
        if (++i >= len - 1)
            error("Error parsing config file: readuntil eof before val");

    return i;
}

// skip commented line
size_t readUntilNewline(const char* buf, i64 len)
{
    if (len < 2)
        error("Error parsing config file: readuntil LF null ref");

    rAssert(buf[0] == '#');

    size_t i = 0;

    while (++i < len && buf[i] != '\n');

    return i;
}