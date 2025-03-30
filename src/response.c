#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <response.h>
#include <memtrack.h>

// parse JSESSION id from set-cookie header
void parseJSession(const char* rxbuf, size_t size, char* ssnbuf)
{
    if (!rxbuf || !size || !ssnbuf)
        error("parseJSession: null ref");

    for (size_t i = 0; i < size; i++) {
        if (rxbuf[i] == 'J' && i + 11 < size && strncmp(&rxbuf[i], "JSESSIONID=", 11) == 0) {
            if (i + 43 < size) {
                memcpy(ssnbuf, &rxbuf[i + 11], 32 * sizeof(char));

                ssnbuf[32] = '\0';

                return;
            } else {
                error("JSESSION id out of bounds");
            }
        }
    }
}

// parse JSESSION id with timestamp and viewstate from html response
int parseSSNTokenViewstate(const char* rxbuf, size_t size, char* ssntbuf, char* vstabuf)
{
    char fssn = ssntbuf[0] ? 1 : 0;
    char fvst = vstabuf[0] ? 1 : 0;

    for (size_t i = 0; i < size; i++) {
        if (!fssn && rxbuf[i] == 'u' && i + 10 < size && strncmp(&rxbuf[i], "uniqueToken", 11) == 0) {
            if (i + 64 < size) {
                memcpy(ssntbuf, &rxbuf[i + 20], 45 * sizeof(char));
                ssntbuf[45] = '\0';
                fssn = 1;

                rDebugPrintf("Copied ssnt from %d", i);
            } else {
                error("Session token out of bounds");
            }
        } else  if (!fvst && rxbuf[i] == 'n' && i + 26 < size && strncmp(&rxbuf[i], "name=\"javax.faces.ViewState", 27) == 0) {
            if (i + 106 < size) {
                if (rxbuf[i + 75] == '"')
                    i++;

                memcpy(vstabuf, &rxbuf[i + 75], 32 * sizeof(char));
                vstabuf[32] = '\0';
                fvst = 1;

                rDebugPrintf("Copied vsta from %d", i);
            } else {
                error("Viewstate out of bounds");
            }
        }

        if (fssn && fvst)
            break;
    }

    return fssn + fvst;
}

// parse viewstate from partial xml response
void parseViewstate(const char* rxbuf, size_t size, char* vstabuf)
{
    for (size_t i = 0; i < size; i++) {
        if (rxbuf[i] == '1' && i + 12 < size && strncmp(&rxbuf[i], "1\"><![CDATA[", 12) == 0) {
            if (i + 43 < size) {
                memcpy(vstabuf, &rxbuf[i + 12], 32 * sizeof(char));
                vstabuf[32] = '\0';
                return;
            } else {
                error("parseViewstate: unexpected response length");
            }
        }
    }
}

// parse values specified in fields into fldbuf from html response
void parseFieldVals(const char* rxbuf, size_t size, char** fields, size_t fldsize, char** fldbuf)
{
    static char f = 0;

    if (f)
        return;

    for (size_t i = 0; i < size; i++)
    {
        if (i + 6 < size && rxbuf[i] == 'n' && strncmp(&rxbuf[i], "name=\"", 6) == 0)
        {
            i += 6;

            for (size_t k = 0; k < fldsize; k++)
            {
                if (!fields[k])
                    continue;

                size_t len = strnlen(fields[k], 128);

                if (i + len < size && strncmp(fields[k], &rxbuf[i], len) == 0)
                {
                    size_t offset = 1;

                    while (i + len + offset < size)
                    {
                        if (rxbuf[i + len + offset] == '>') {
                            i += len + offset;
                            break;
                        }

                        if (i + len + offset + 7 < size && rxbuf[i + len + offset] == 'v'
                            && strncmp(&rxbuf[i + len + offset], "value=\"", 7) == 0) {
                            size_t flen = 0;

                            while (i + len + offset + 6 + ++flen < size && rxbuf[i + len + offset + 6 + flen] != '"');

                            if (flen == 1) {
                                i += len + 7;
                                break; 
                            }

                            fldbuf[k] = (char*) memAlloc(flen * sizeof(char));

                            memcpy(fldbuf[k], &rxbuf[i + len + offset + 7], (flen - 1) * sizeof(char));

                            fldbuf[k][flen - 1] = '\0';
                            fields[k] = NULL;

                            i += len + offset + flen + 6;

                            break;
                        }

                        offset++;
                    }
                }
            }
        }
    }

    f = 1;

    for (size_t i = 0; i < fldsize; i++) {
        if (fields[i]) {
            f = 0;
            break;
        }
    }
}

// print field values
void printFields(char** fields, size_t fldsize)
{
    if (!fields || !fldsize) {
        error("Print fields: null ref");
        return;
    }

    rDebugString("Parsed fields from response:");

    for (int i = 0; i < fldsize; i++)
        rDebugPrintf(" > %d:\t%s", i, fields[i] ? fields[i] : "<null>");
}

// free fields array
void freeFields(char** fields, size_t fldsize)
{
    if (!fields || !fldsize)
        return;

    for (size_t i = 0; i < fldsize; i++) {
        memFree(fields[i]);
        fields[i] = NULL;
    }

    memFree(fields);
}