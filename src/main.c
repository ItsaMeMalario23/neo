// Project includes
#include <main.h>
#include <memtrack.h>
#include <fileio.h>
#include <request.h>
#include <response.h>
#include <rdebug.h>

// Sys includes
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef R_WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <iphlpapi.h>
#else
    #include <netdb.h>
    #include <unistd.h>
    #include <arpa/inet.h>
    #include <netinet/in.h>
    #include <sys/socket.h>
#endif

// ========================================================================== //
//                                                                            //
//  GLOBALS                                                                   //
//                                                                            //
// ========================================================================== //

char ssn[33];
char ssnt[46];
char vstate[33];

config_t* g_conf = NULL;

// ========================================================================== //
//                                                                            //
//  KNOWN LIMITATIONS                                                         //
//                                                                            //
//  > Does not work if user has no access profile                             //
//  > Some badge numbers have a leading 0, some do not                        //
//      (badge number must match exactly)                                     //
//                                                                            //
// ========================================================================== //

int main(void)
{
    setupLogFile();

    config_t* conf = setupConfig("config.txt");

    import_t* info = setupImport(conf->filePath);

    for (import_t* i = info; i; i = i->next) {
        setupFieldConfigs(cnf_selprf, cnf_cmtln, i, conf);
        addProfile(i, conf);
    }

    moveImportFile(conf->filePath, 1);

    freeConfigData(conf);
    freeImportData(info);

    #ifdef R_WIN32
        WSACleanup();
    #endif

    rDebugString("Execution successful\n");

    return EXIT_SUCCESS;
}

// TODO add badgeno cross check
void addProfile(import_t* info, config_t* conf)
{
    rReleaseAssert(s_url);
    rReleaseAssert(s_port);

    int socket = openSocket(s_url, s_port);

    char txbuf[RPOST_BUF_SIZE];

    // login
    login(socket, txbuf, RPOST_BUF_SIZE, ssn);

    // rdr login
    size_t len = redirectLoginRequest(txbuf, RPOST_BUF_SIZE, ssn);

    rReleaseAssert(len);

    sendRequest(socket, txbuf, len, ssnt, vstate, 200, 0);
    
    // mtoprs
    char* hdrs = buildHeaders(s_url, s_port, e_menu, ssn, 8, H_STDACPT, H_KEEPALIVE, H_CTYEXF, H_JSSN, H_HOST, H_ORIGIN, H_REFERER, H_UPGRADE);
    len = buildPostRequest(txbuf, RPOST_BUF_SIZE, e_menu, hdrs, ssnt, vstate, NULL, p_mtoprs_fmt);
    sendRequestIgnoreResponse(socket, txbuf, len, 302);

    // rdr mtoprs
    hdrs = buildHeaders(s_url, s_port, e_menu, ssn, 6, H_STDACPT, H_KEEPALIVE, H_JSSNMXRDR, H_HOST, H_REFERER, H_UPGRADE);
    len = buildGetRequest(txbuf, RPOST_BUF_SIZE, e_search, hdrs);
    sendRequest(socket, txbuf, len, ssnt, vstate, 200, 0);

    // search id
    hdrs = buildHeaders(s_url, s_port, e_search, ssn, 8, H_STDACPT, H_KEEPALIVE, H_CTYEXF, H_JSSNMX, H_HOST, H_ORIGIN, H_REFERER, H_UPGRADE);
    len = buildPostRequest(txbuf, RPOST_BUF_SIZE, e_search, hdrs, ssnt, vstate, info->persNo, p_searchprs_fmt);
    sendRequest(socket, txbuf, len, ssnt, vstate, 200, 0);

    // stoedt
    hdrs = buildHeaders(s_url, s_port, e_search, ssn, 8, H_STDACPT, H_KEEPALIVE, H_CTYEXF, H_JSSNMX, H_HOST, H_ORIGIN, H_REFERER, H_UPGRADE);
    len = buildPostRequest(txbuf, RPOST_BUF_SIZE, e_search, hdrs, ssnt, vstate, info->persNo, p_stoprs_fmt);
    sendRequestIgnoreResponse(socket, txbuf, len, 302);

    // rdr stoedt
    hdrs = buildHeaders(s_url, s_port, e_search, ssn, 6, H_STDACPT, H_KEEPALIVE, H_JSSNMXRDR, H_HOST, H_REFERER, H_UPGRADE);
    len = buildGetRequest(txbuf, RPOST_BUF_SIZE, e_edtprs, hdrs);
    char** flds = sendRequestParseFields(socket, txbuf, len, searchfields, 15, 200);
    
    printFields(flds, 15);

    assertFieldValues(flds, conf);

    field_t valbuf[28];

    // chtab
    rDebugString("CHTAB");
    configureFields((const char**) flds, 15, valbuf, CONF_CHTAB);
    hdrs = buildHeaders(s_url, s_port, e_edtprs, ssn, 9, H_XMLACPT, H_KEEPALIVE, H_CTYPMPT, H_JSSNMX, H_FACESRQ, H_HOST, H_ORIGIN, H_REFERER, H_XMLHTTP);
    len = buildMPartPayload(txbuf, RPOST_BUF_SIZE, inc_chtab, 70, valbuf, 24);
    len = buildMPtPostRequest(txbuf, RPOST_BUF_SIZE, e_edtprs, hdrs);
    sendRequest(socket, txbuf, len, ssnt, vstate, 200, 1);  // found vsta
    memcpy(flds[14], vstate, 32 * sizeof(char));    // update viewstate
    flds[14][32] = '\0';

    // addln
    rDebugString("ADDLN");
    configureFields((const char**) flds, 15, valbuf, CONF_ADDLN);
    hdrs = buildHeaders(s_url, s_port, e_edtprs, ssn, 8, H_STDACPT, H_KEEPALIVE, H_CTYPMPT, H_JSSNMX, H_HOST, H_ORIGIN, H_REFERER, H_UPGRADE);
    len = buildMPartPayload(txbuf, RPOST_BUF_SIZE, inc_addln, 63, valbuf, 16);
    len = buildMPtPostRequest(txbuf, RPOST_BUF_SIZE, e_edtprs, hdrs);
    sendRequest(socket, txbuf, len, ssnt, vstate, 200, 0);  // found vsta
    memcpy(flds[14], vstate, 32 * sizeof(char));    // update viewstate
    memcpy(flds[0], ssnt, 45 * sizeof(char));       // update token
    flds[14][32] = '\0';
    flds[0][45] = '\0';

    // selprf
    rDebugString("SELPRF");
    configureFields((const char**) flds, 15, valbuf, CONF_SELPRF);
    hdrs = buildHeaders(s_url, s_port, e_edtprs, ssn, 9, H_XMLACPT, H_KEEPALIVE, H_CTYPMPT, H_JSSNMX, H_FACESRQ, H_HOST, H_ORIGIN, H_REFERER, H_XMLHTTP);
    len = buildMPartPayload(txbuf, RPOST_BUF_SIZE, inc_selprf, 70, valbuf, 22);
    len = buildMPtPostRequest(txbuf, RPOST_BUF_SIZE, e_edtprs, hdrs);
    sendRequest(socket, txbuf, len, ssnt, vstate, 200, 1);  // vsta null

    // cmtln
    rDebugString("CMTLN");
    configureFields((const char**) flds, 15, valbuf, CONF_CMTLN);
    hdrs = buildHeaders(s_url, s_port, e_edtprs, ssn, 8, H_STDACPT, H_KEEPALIVE, H_CTYPMPT, H_JSSNMX, H_HOST, H_ORIGIN, H_REFERER, H_UPGRADE);
    len = buildMPartPayload(txbuf, RPOST_BUF_SIZE, inc_cmtln, 66, valbuf, 19);
    len = buildMPtPostRequest(txbuf, RPOST_BUF_SIZE, e_edtprs, hdrs);
    sendRequest(socket, txbuf, len, ssnt, vstate, 200, 0);  // vsta null ssnt null
    memcpy(flds[14], vstate, 32 * sizeof(char));    // update viewstate
    memcpy(flds[0], ssnt, 45 * sizeof(char));       // update token
    flds[14][32] = '\0';
    flds[0][45] = '\0';

    // save
    rDebugString("SAVE");
    configureFields((const char**) flds, 15, valbuf, CONF_SAVE);
    hdrs = buildHeaders(s_url, s_port, e_edtprs, ssn, 8, H_STDACPT, H_KEEPALIVE, H_CTYPMPT, H_JSSNMX, H_HOST, H_ORIGIN, H_REFERER, H_UPGRADE);
    len = buildMPartPayload(txbuf, RPOST_BUF_SIZE, inc_save, 63, valbuf, 16);
    len = buildMPtPostRequest(txbuf, RPOST_BUF_SIZE, e_edtprs, hdrs);
    sendRequestIgnoreResponse(socket, txbuf, len, 302);

    freeFields(flds, 15);

    // rdr save
    rDebugString("RDRSAVE");
    hdrs = buildHeaders(s_url, s_port, e_edtprs, ssn, 6, H_STDACPT, H_KEEPALIVE, H_JSSNMXRDR, H_HOST, H_REFERER, H_UPGRADE);
    len = buildGetRequest(txbuf, RPOST_BUF_SIZE, e_edtprs, hdrs);
    flds = sendRequestParseFields(socket, txbuf, len, searchfields, 15, 200);

    printFields(flds, 15);

    freeFields(flds, 15);

    #ifdef R_WIN32
        closesocket(socket);
    #else
        close(socket);
    #endif
}

// ========================================================================== //
//                                                                            //
//  SETUP FUNCTIONS                                                           //
//                                                                            //
// ========================================================================== //

config_t* setupConfig(const char* filename)
{
    config_t* conf = readConfigFile(filename);

    if (!conf)
        error("Error reading configuration");

    rDebugString("Configuration");
    rDebugPrintf("allowNoFirstName:\t%d", conf->noFirstName);
    rDebugPrintf("allowNoLastName:\t\t%d", conf->noLastName);
    rDebugPrintf("allowNonAlphaNum:\t%d", conf->alphaNumName);
    rDebugPrintf("allowNoDepartment:\t%d", conf->noDepartment);
    rDebugPrintf("allowBlockingReason:\t%d", conf->blockingReason);

    for (unsigned i = 0; i < conf->numProfiles; i++)
        rDebugPrintf("profile %d:\t\t\t%d", i + 1, conf->profiles[i]);

    if (conf->attendance)
        for (strnode_t* i = conf->attendance; i; i = i->next)
            rDebugPrintf("attendance:\t\t\t%s", i->val);
    else
        error("No attendance value found");

    if (conf->officeRelease)
        for (strnode_t* i = conf->officeRelease; i; i = i->next)
            rDebugPrintf("office release:\t\t%s", i->val);
    else
        error("No office release value found");

    rDebugPrintf("url:\t\t\t\t\t%s", conf->serverUrl);
    rDebugPrintf("port:\t\t\t\t%s", conf->serverPort);
    rDebugPrintf("path:\t\t\t\t%s\n", conf->filePath);

    s_url = conf->serverUrl;
    s_port = conf->serverPort;

    g_conf = conf;

    return conf;
}

import_t* setupImport(const char* filename)
{
    import_t* info = NULL;

    char c = 1;

    readImportFile(filename, &info);

    if (!info)
        error("Error reading import file");

    for (import_t* i = info; i; i = i->next) {
        rDebugPrintf("Import configuration %d", c++);
        rDebugPrintf("BadgeNo:\t\t%s", i->badgeNo);
        rDebugPrintf("PersNo:\t\t%s", i->persNo);
        rDebugPrintf("ValidFrom:\t%s", i->validFrom);
        rDebugPrintf("ValidTo:\t\t%s", i->validTo);
        rDebugPrintf("Profile:\t\t%s\n", i->profile);
    
        size_t dtlen = strnlen(i->validFrom, 11);

        char dttmp[10];
    
        if (dtlen > 10 || dtlen < 10)
            error("Invalid from date length");
    
        memcpy(dttmp, i->validFrom, 10 * sizeof(char));
    
        i->validFrom[0] = dttmp[3];
        i->validFrom[1] = dttmp[4];
        i->validFrom[3] = dttmp[0];
        i->validFrom[4] = dttmp[1];
        i->validFrom[2] = '/';
        i->validFrom[5] = '/';
    
        dtlen = strnlen(i->validTo, 11);
    
        if (dtlen > 10 || dtlen < 10)
            error("Error parsing validTo date");
        
        memcpy(dttmp, i->validTo, 10 * sizeof(char));
    
        i->validTo[0] = dttmp[3];
        i->validTo[1] = dttmp[4];
        i->validTo[3] = dttmp[0];
        i->validTo[4] = dttmp[1];
        i->validTo[2] = '/';
        i->validTo[5] = '/';
    }

    return info;
}

void setupLogFile(void)
{
    time_t t = time(NULL);
    struct tm ts;

    localtime_s(&ts, &t);

    char filename[128];

    snprintf(filename, 128, "log/log-%d-%d-%d.txt", ts.tm_year + 1900, ts.tm_mon + 1, ts.tm_mday);

    #ifndef RDEBUG_LOG
        rAssert(0);
    #endif

    rDebugOutputStream(filename);

    rDebugString("Execution started\n");
}

void setupFieldConfigs(field_t* selprf, field_t* cmtln, import_t* info, config_t* conf)
{
    char f = 0;
    
    // check if profile is allowed
    for (unsigned i = 0; i < conf->numProfiles; i++) {
        if (conf->profiles[i] == atoi(info->profile)) {
            f = 1;
            break;
        }
    }

    if (!f)
        error("Profile specified in import file is not allowed");

    cnf_selprf[5].val = info->profile;
    cnf_selprf[6].val = info->profile;

    cnf_cmtln[0].val = info->profile;
    cnf_cmtln[1].val = info->profile;
    cnf_cmtln[2].val = info->validFrom;
    cnf_cmtln[3].val = info->validTo;
    // cnf_cmtln[2].val = "\0";
    // cnf_cmtln[3].val = "\0";
}

// open tcp connection to service
int openSocket(const char* url, const char* port)
{
    #ifdef R_WIN32

        WSADATA wsa;

        if (WSAStartup(MAKEWORD(2, 2), &wsa))
            error("Winsock init failed");

        struct addrinfo* info = NULL;

        void* addr;

        if (getaddrinfo(url, port, NULL, &info))
            error(gai_strerror(WSAGetLastError()));

        char addrbuf[46];
        int len, fa = 0;

        for (struct addrinfo* i = info; i; i = i->ai_next) {
            fa = i->ai_family;
            addr = i->ai_addr;
            len = i->ai_addrlen;

            if (fa == AF_INET)
                break;

            if (fa != AF_INET6)
                fa = 0;
        }

        if (!fa)
            error("Could not resolve hostname");

        rDebugPrintf("Connecting to %s:%s (IPv%d)\n",
            inet_ntop(fa, addr, addrbuf, 46), port, fa == AF_INET ? 4 : 6);

        int sockfd = socket(fa, SOCK_STREAM, IPPROTO_TCP);

        if (sockfd == INVALID_SOCKET)
            error("Failed to open socket");

        if (connect(sockfd, (struct sockaddr*) addr, len))
            error(gai_strerror(WSAGetLastError()));

        freeaddrinfo(info);

        return sockfd;

    #else

        struct addrinfo* info = NULL;

        void* addr;

        getaddrinfo(url, port, NULL, &info);

        if (!info)
            error("Could not resolve hostname");

        char addrbuf[INET6_ADDRSTRLEN];
        char fa = 0;

        for (struct addrinfo* i = info; i; i = i->ai_next) {
            fa = i->ai_addr->sa_family;
            addr = i->ai_addr;

            if (fa == AF_INET)
                break;

            if (fa != AF_INET6)
                fa = 0;
        }

        socklen_t len;

        if (fa == AF_INET)
            len = sizeof(struct sockaddr_in);
        else if (fa == AF_INET6)
            len = sizeof(struct sockaddr_in6);
        else
            return -1;

        rDebugPrintf("Connecting to %s:%s (IPv%d)\n",
            inet_ntop(fa, addr, addrbuf, sizeof(addrbuf)), port, fa == AF_INET ? 4 : 6);

        int sockfd = socket(fa, SOCK_STREAM, 0);

        if (sockfd < 0)
            error("Failed to open socket");

        if (connect(sockfd, (struct sockaddr*) addr, len) < 0)
            error(strerror(errno));

        freeaddrinfo(info);

        return sockfd;

    #endif
}

int sockWrite(int socket, const char* txbuf, size_t reqlen)
{
    rAssert(txbuf);
    rAssert(reqlen);

    #ifdef R_WIN32
        int len = send(socket, txbuf, reqlen, 0);
    #else
        int len = write(socket, txbuf, reqlen);
    #endif

    if (len < 0)
        error("Could not write request to socket");

    rDebugPrintf("Transmitted %d bytes", len);

    return len;
}

int sockRead(int socket, char* rxbuf, size_t bufsize)
{
    rAssert(rxbuf);
    rAssert(bufsize);

    #ifdef R_WIN32
        int len = recv(socket, rxbuf, bufsize, 0);
    #else
        int len = read(socket, rxbuf, bufsize);
    #endif

    if (len < 0)
        error("Could not read response from socket");

    rDebugPrintf("Received %d bytes", len);

    return len;
}

void login(int socket, char* txbuf, size_t size, char* ssnbuf)
{
    rAssert(txbuf);
    rAssert(ssnbuf);

    size_t len = loginRequest(txbuf, size);

    if (!len)
        error("Building login request failed");

    rDebugString("Sending request...");

    (void) sockWrite(socket, txbuf, strnlen(txbuf, size));

    int rlen = sockRead(socket, txbuf, size);

    if (rlen == 0)
        error("Connection reset");

    assertStatusCode(302, txbuf);

    if (rlen >= size)
        error("Unexpected response to login request");

    ssnbuf[0] = '\0';

    parseJSession(txbuf, rlen, ssnbuf);

    if (ssnbuf[0])
        rDebugPrintf("Found JSESSION key: %s", ssnbuf);
    else
        error("No JSESSION key found");
}

void sendRequest(int socket, const char* txbuf, size_t size, char* ssntbuf, char* vstbuf, int expected, bool partial)
{
    rAssert(txbuf);
    rAssert(size);
    rAssert(ssntbuf);
    rAssert(vstbuf);

    rDebugString("Sending request...");

    (void) sockWrite(socket, txbuf, size);

    char rxbuf[RCV_BUF_SIZE];

    size_t rlen, recv = 0;
    int f = 0;
    char c = 1;

    // bypass ssn because partial response contains no ssn token
    if (!partial)
        ssntbuf[0] = '\0';
    
    vstbuf[0] = '\0';

    for (;;) {
        rlen = sockRead(socket, &rxbuf[recv], RCV_BUF_SIZE - recv);

        if (partial) {
            recv += rlen;

            if (rlen == 8192) {
                while (rlen > 4 && strncmp(&rxbuf[recv - 5], "0\r\n\r\n", 5) != 0) {
                    rlen = sockRead(socket, &rxbuf[recv], RCV_BUF_SIZE - recv);
                    recv += rlen;

                    if (recv >= RCV_BUF_SIZE)
                        break;
                }
            }

            parseViewstate(rxbuf, recv, vstbuf);

            f = 2;
            break;
        }

        if (c) {
            assertStatusCode(expected, rxbuf);
            c = 0;
        }
        
        recv += rlen;

        if (rlen > 4 && strncmp(&rxbuf[recv - 5], "0\r\n\r\n", 5) == 0)
            break;

        if (recv >= RCV_BUF_SIZE) {
            rDebugString("RCV buffer reset");

            if (f < 2)
                f = parseSSNTokenViewstate(rxbuf, recv, ssntbuf, vstbuf);

            recv = 0;
            continue;
        }

        if (!rlen)
            error("Connection reset");

        if (rlen > 9 && strncmp(&rxbuf[recv - 10], "Server\r\n\r\n", 10) == 0) {
            rxbuf[recv] = '\0';
            rDebugPrintf("Unexpected response, authentication may have failed:\n\n%s\n\n", rxbuf);
            break;
        }
    }

    if (f < 2)
        f = parseSSNTokenViewstate(rxbuf, recv, ssntbuf, vstbuf);

    /*
    if (f < 2)
        error("No viewstate or session ID found in response");
    */

    rDebugPrintf("ssnt: %s vsta: %s", ssntbuf, vstbuf);
}

void sendRequestIgnoreResponse(int socket, const char* txbuf, size_t size, int expected)
{
    if (!txbuf || !size)
        return;

    rDebugString("Sending request...");

    (void) sockWrite(socket, txbuf, size);

    char rxbuf[RPOST_BUF_SIZE];

    size_t rlen = sockRead(socket, rxbuf, RPOST_BUF_SIZE);

    if (rlen >= RPOST_BUF_SIZE)
        error("Unexpected response length");

    assertStatusCode(expected, rxbuf);

    // rxbuf[rlen == 16000 ? rlen - 1 : rlen] = '\0';
    // printf("send ign req res:\n%s\n\n", rxbuf);
}

char** sendRequestParseFields(int socket, const char* txbuf, size_t size, const char** fields, size_t fldsize, int expected)
{
    if (!txbuf || !size || !fields || !fldsize)
        return NULL;

    rDebugString("Sending request...");

    (void) sockWrite(socket, txbuf, size);

    char** fldbuf = (char**) memAlloc(fldsize * sizeof(char*));
    char** fldtmp = (char**) memAlloc(fldsize * sizeof(char*));

    if (!fldbuf || !fldtmp)
        error("Parse response: memory allocation failed");

    memset(fldbuf, 0, fldsize * sizeof(char*));
    memcpy(fldtmp, fields, fldsize * sizeof(char*));

    char rxbuf[RCV_BUF_SIZE];

    size_t rlen, recv = 0;
    char f = 1;

    for (;;) {
        rlen = sockRead(socket, &rxbuf[recv], RCV_BUF_SIZE - recv);

        if (f) {
            assertStatusCode(expected, rxbuf);
            f = 0;
        }

        recv += rlen;

        // check for terminating block
        if (rlen > 4 && strncmp(&rxbuf[recv - 5], "0\r\n\r\n", 5) == 0)
            break;

        if (rlen > 9 && strncmp(&rxbuf[recv - 10], "Server\r\n\r\n", 10) == 0) {
            rxbuf[recv] = '\0';
            rDebugPrintf("Unexpected response, authentication may have failed:\n\n%s\n\n", rxbuf);
            return NULL;
        }

        if (!rlen)
            error("Connection reset");

        if (recv >= RCV_BUF_SIZE) {
            rDebugString("RCV buffer reset");
            parseFieldVals(rxbuf, recv, fldtmp, fldsize, fldbuf);
            recv = 0;
        }
    }

    parseFieldVals(rxbuf, recv, fldtmp, fldsize, fldbuf);

    memFree(fldtmp);

    return fldbuf;
}

// setup field buf for multipart requests
void configureFields(const char** fields, size_t fldSize, field_t* valbuf, unsigned config)
{
    if (config == CONF_CHTAB)
    {
        memcpy(valbuf, cnf_chtab, 8 * sizeof(field_t));

        for (unsigned i = 0; i < 11; i++) {
            valbuf[i + 8].idx = fieldIndices[i];
            valbuf[i + 8].val = fields[i];
        }

        memcpy(&valbuf[19], &cnf_chtab[8], 4 * sizeof(field_t));

        valbuf[23].idx = 81;
        valbuf[23].val = fields[14];
    }
    else if (config == CONF_ADDLN || config == CONF_SAVE)
    {
        for (unsigned i = 0; i < 11; i++) {
            valbuf[i].idx = fieldIndices[i];
            valbuf[i].val = fields[i];
        }

        memcpy(&valbuf[11], cnf_addln, 4 * sizeof(field_t));

        valbuf[15].idx = 81;
        valbuf[15].val = fields[14];
    }
    else if (config == CONF_SELPRF)
    {
        memcpy(valbuf, cnf_selprf, 5 * sizeof(field_t));

        for (unsigned i = 0; i < 11; i++) {
            valbuf[i + 5].idx = fieldIndices[i];
            valbuf[i + 5].val = fields[i];
        }

        memcpy(&valbuf[16], &cnf_selprf[5], 5 * sizeof(field_t));

        valbuf[21].idx = 81;
        valbuf[21].val = fields[14];
    }
    else if (config == CONF_CMTLN)
    {
        for (unsigned i = 0; i < 11; i++) {
            valbuf[i].idx = fieldIndices[i];
            valbuf[i].val = fields[i];
        }

        memcpy(&valbuf[11], cnf_cmtln, 7 * sizeof(field_t));

        valbuf[18].idx = 81;
        valbuf[18].val = fields[14];
    }
    else
    {
        error("Invalid field config");
    }
}

void error(const char* msg)
{
    rReleaseStringLevel(ERR_FTL, msg);

    if (g_conf && g_conf->filePath)
        moveImportFile(g_conf->filePath, 0);

    #ifdef R_WIN32
        WSACleanup();
    #endif

    rDebugStringLevel(ERR_FTL, "Execution failed\n");

    exit(EXIT_FAILURE);
}

void assertFieldValues(char** flds, config_t* conf)
{
    if (!flds || !conf)
        error("Assert fields null ref");

    if (!conf->noLastName && (!flds[1] || flds[1][0] == '\0'))
        error("Empty last name not allowed");

    if (!conf->noFirstName && (!flds[2] || flds[2][0] == '\0'))
        error("Empty first name not allowed");

    if (!conf->noDepartment && (!flds[3] || !flds[4] || flds[3][0] == '\0' || flds[4][0] == '\0'))
        error("Empty department not allowed");

    if (!flds[5] || flds[5][0] == '\0')
        error("Empty employee ID in response");

    if ((!conf->blockingReason && flds[7][0] != '0') || !flds[7] || flds[7][0] == '\0')
        error("Invalid blocking reason in response");

    char f = 0;

    for (strnode_t* i = conf->attendance; i; i = i->next) {
        size_t len = strnlen(flds[6], 16);

        if (strncmp(flds[6], i->val, len) == 0) {
            f = 1;
            break;
        }
    }

    if (!f)
        error("Invalid attendance");

    f = 0;

    for (strnode_t* i = conf->officeRelease; i; i = i->next) {
        size_t len = strnlen(flds[9], 16);

        if (strncmp(flds[9], i->val, len) == 0) {
            f = 1;
            break;
        }
    }

    if (!f)
        error("Invalid office release");
}

// assert expected http response code, exit on failure
void assertStatusCode(int expected, char* rxbuf)
{
    if (strncmp(rxbuf, "HTTP/1.1", 8)) {
        rDebugString("Assert status code: no HTTP header in rxbuf");
        return;
    }

    char nbuf[4];

    memcpy(nbuf, &rxbuf[9], 3 * sizeof(char));

    nbuf[3] = '\0';

    if (expected != atoi(nbuf)) {
        char msg[40];

        snprintf(msg, 40, "Expected status code %d, got %s\n", expected, nbuf);

        error(msg);
    }
}