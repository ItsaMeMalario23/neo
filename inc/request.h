#ifndef REQUEST_H
#define REQUEST_H

#include <stdio.h>
#include <stdlib.h>

#include <main.h>

//
//  Definitions in request.c
//

// Service dst
extern const char* s_url;
extern const char* s_port;

// HTTP methods
extern const char* m_get;
extern const char* m_post;

// Service endpoints
extern const char* e_login;
extern const char* e_menu;
extern const char* e_search;
extern const char* e_edtprs;

// HTTP request
extern const char* stdreq_fmt;
extern const char* req_fmt;

// HTTP headers
extern const char* h_ctypexf;
extern const char* h_clen_fmt;
extern const char* h_jsession_fmt;
extern const char* h_headers[15];

// Request payloads
extern const char* p_login;
extern const char* p_mtoprs_fmt;
extern const char* p_searchprs_fmt;
extern const char* p_stoprs_fmt;

// Misc
extern const char* cdisposh_fmt;
extern const char* mpartboundary;
extern const char* mpartrfields[82];
extern const char* searchfields[15];

// Include indices for multipart requests
extern const int inc_chtab[70];
extern const int inc_addln[63];
extern const int inc_selprf[70];
extern const int inc_cmtln[66];
extern const int inc_save[63];

// Field configurations for multipart requests
extern const unsigned fieldIndices[11];
extern field_t cnf_chtab[12];
extern field_t cnf_addln[4];
extern field_t cnf_selprf[10];
extern field_t cnf_cmtln[7];

//
//  Public functions
//
char* buildHeaders(const char* url, const char* port, const char* ref, const char* ssn, unsigned numHeaders, ...);

size_t loginRequest(char* txbuf, size_t txbufSize);
size_t redirectLoginRequest(char* txbuf, size_t txbufSize, const char* ssn);
size_t buildGetRequest(char* txbuf, size_t size, const char* endpt, char* headers);
size_t buildPostRequest(char* txbuf, size_t size, const char* endpt, char* headers, char* ssnt, char* vsta, const char* persNo, const char* payld);
size_t buildMPtPostRequest(char* txbuf, size_t size, const char* endpt, char* headers);

size_t buildMPartPayload(char* buf, size_t size, const int* incHdrs, size_t incLen, const field_t* fldVals, size_t fldLen);

//  Local functions
bool isIncluded(int idx, int* inc, size_t incLen);

//
//  Request sequence:
//
//  > POST login (r login) redirect -> GET mainMenu (r login)                                   (LOGIN)
//  > POST mainMenu (r mainMenu) redirect -> GET searchPersRecord (r mainMenu)                  (MENU to PERSONS)
//      > POST searchPersRecord (r searchPers) -> 200 OK                                        (SEARCH bID)
//  > POST searchPersRecord (r searchPers + eID + tab) -> GET editPersRecord (r searchPers)     (SEARCH to EDIT)
//      > POST editPersRecord (r editPers) + x-req chtab -> 200 OK                              (GOTO PERM TAB)
//      > POST editPersRecord (r editPers) -> 200 OK                                            (ADD LINE)
//      > POST editPersRecord (r editPers) + x-req accpr -> 200 OK                              (SELECT ACCPRF)
//      > POST editPersRecord (r editPers) -> 200 OK                                            (COMMIT LINE)
//  > POST editPersRecord (r editPers) -> GET editPersRecord (r editPers)                       (SAVE)
//

#endif