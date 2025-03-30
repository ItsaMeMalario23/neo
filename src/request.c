#include <stdarg.h>
#include <string.h>

#include <request.h>
#include <response.h>

//
//  Service dst
//
const char* s_url = NULL;
const char* s_port = NULL;

//
//  HTTP methods
//
const char* m_get = "GET";
const char* m_post = "POST";

//
//  Service endpoints
//
const char* e_login =   "/matrix-v4.1.1.83984/login.jspx";
const char* e_menu =    "/matrix-v4.1.1.83984/mainMenu.jsf";
const char* e_search =  "/matrix-v4.1.1.83984/views/basis/personalmanagement/searchPersRecord.jsf";
const char* e_edtprs =  "/matrix-v4.1.1.83984/views/basis/personalmanagement/editPersRecord.jsf";

//
//  HTTP request
//
const char* stdreq_fmt =
    // http header
    "%s %s HTTP/1.1\r\n"                            // method + endpoint
    // request headers
    "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/132.0.0.0 Safari/537.36\r\n"
    "Connection: keep-alive\r\n"
    "Cache-Control: no-cache\r\n"
    "Accept-Encoding: gzip, deflate\r\n"
    "%s"                                            // content-type header
    "%s"                                            // content-len header
    "%s"                                            // cookie header
    "%s"                                            // optional headers
    "Pragma: no-cache\r\n"
    "Referer: http://%s:%s%s\r\n"                   // url + port + endpoint
    "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,"
        "image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.7\r\n"
    "Accept-Language: en-US,en;q=0.9,de-DE;q=0.8,de;q=0.7\r\n"
    "Origin: http://%s:%s\r\n"                      // url + port
    "Host: %s:%s\r\n"                               // url + port
    "Upgrade-Insecure-Requests: 1\r\n\r\n"
    "%s";                                           // payload

const char* req_fmt =
    // http header
    "%s %s HTTP/1.1\r\n"                            // method + endpoint
    // request headers
    "Accept-Encoding: gzip, deflate\r\n"
    "Accept-Language: en-US,en;q=0.9,de-DE;q=0.8,de;q=0.7\r\n"
    "%s"                                            // content-len header
    "%s"                                            // optional headers
    "Pragma: no-cache\r\n"
    "Cache-Control: no-cache\r\n"
    "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/132.0.0.0 Safari/537.36\r\n\r\n"
    "%s";                                           // payload

//
//  HTTP headers
//
const char* h_ctypexf = "Content-Type: application/x-www-form-urlencoded\r\n";
const char* h_clen_fmt = "Content-Length: %d\r\n";

const char* h_jsession_fmt =
    "Cookie: JSESSIONID=%s; timezonename=Europe%%2FBerlin; timezoneoffset=%%2B01%%3A00; timezonedst=true; "     // session id
    "icarus_activemenuitem=menuform%%3AmainMenu_bas_root_1%%2Cmenuform%%3AmainMenu_bas_root_"
    "1_1%%2Cmenuform%%3AmainMenu_acc_root_0%%2Cmenuform%%3AmainMenu_acc_root_0_0\r\n";

const char* h_headers[15] = {
    "Connection: keep-alive",
    "Connection: close",
    "Content-Type: application/x-www-form-urlencoded",
    "Content-Type: multipart/form-data; boundary=----WebKitFormBoundaryxRKaq1BRumca55Nt",
    "Faces-Request: partial/ajax",
    "X-Requested-With: XMLHttpRequest",
    "Referer: http://%s:%s%s\r\n",
    "Host: %s:%s\r\n",
    "Origin: http://%s:%s\r\n",
    "Upgrade-Insecure-Requests: 1",
    "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,"
        "image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.7",
    "Accept: application/xml, text/xml, */*; q=0.01",
    //
    "Cookie: JSESSIONID=%s; timezonename=Europe%%2FBerlin; timezoneoffset=%%2B01%%3A00; timezonedst=true; "
        "icarus_activemenuitem=menuform%%3AmainMenu_acc_root_0%%2Cmenuform%%3AmainMenu_acc_root_0_0\r\n",
    //
    "Cookie: MATRIX_searchPersRecord=headArea; JSESSIONID=%s; oam.Flash.REDIRECT=true; "
        "timezonename=Europe%%2FBerlin; timezoneoffset=%%2B01%%3A00; timezonedst=true; "
        "icarus_activemenuitem=menuform%%3AmainMenu_acc_root_0%%2Cmenuform%%3AmainMenu_acc_root_0_0\r\n",
    //
    "Cookie: MATRIX_searchPersRecord=headArea; JSESSIONID=%s; "
        "timezonename=Europe%%2FBerlin; timezoneoffset=%%2B01%%3A00; timezonedst=true; "
        "icarus_activemenuitem=menuform%%3AmainMenu_acc_root_0%%2Cmenuform%%3AmainMenu_acc_root_0_0\r\n",
    //
};

//
//  Request payloads
//
const char* p_login = "userid=test&password=Supergehe%21m23&loginButton=";     // password is sent in plaintext (suboptimal)

const char* p_mtoprs_fmt =
    "uniqueToken=%s"
    "&menuform_SUBMIT=1"
    "&autoScroll="
    "&javax.faces.ViewState=%s"
    "&activateMenuItem=acc_personalRecords"
    "&menuform%%3AmainMenu_acc_root=menuform%%3AmainMenu_acc_root"
    "&data-matrix-treepath=acc_root.acc_personalManagement.acc_personalRecords"
    "&menuform%%3AmainMenu_acc_root_menuid=0_0";    // clen 350

const char* p_searchprs_fmt =
    "uniqueToken=%s"
    "&autoScroll="
    "&mainbody%%3AsearchPersRecord%%3AsearchHeadField1_Surname="
    "&mainbody%%3AsearchPersRecord%%3AsearchHeadField3_Firstname="
    "&mainbody%%3AsearchPersRecord%%3AsearchHeadField5_Division_inputHidden="
    "&mainbody%%3AsearchPersRecord%%3AsearchHeadField5_Division_inputText="
    "&mainbody%%3AsearchPersRecord%%3AsearchHeadField2_PersNr=%s"
    "&mainbody%%3AsearchPersRecord%%3AsearchHeadField4_BadgeNo="
    "&mainbody%%3AsearchPersRecord%%3AsearchHeadField6_BadgeDesc="
    "&mainbody%%3AsearchPersRecord%%3AbadgeNoKey=searchHeadField4"
    "&mainbody%%3AsearchPersRecord%%3AsearchStartButton="
    "&mainbody%%3AsearchPersRecord%%3AselectSearchProfile_inputHidden="
    "&mainbody%%3AsearchPersRecord%%3Asearchresulttable_reflowDD=5_0"
    "&mainbody%%3AsearchPersRecord_SUBMIT=1"
    "&javax.faces.ViewState=%s";                    // clen 813

const char* p_stoprs_fmt =
    "uniqueToken=%s"
    "&autoScroll="
    "&mainbody%%3AsearchPersRecord%%3AsearchHeadField1_Surname="
    "&mainbody%%3AsearchPersRecord%%3AsearchHeadField3_Firstname="
    "&mainbody%%3AsearchPersRecord%%3AsearchHeadField5_Division_inputHidden="
    "&mainbody%%3AsearchPersRecord%%3AsearchHeadField5_Division_inputText="
    "&mainbody%%3AsearchPersRecord%%3AsearchHeadField2_PersNr=%s"
    "&mainbody%%3AsearchPersRecord%%3AsearchHeadField4_BadgeNo="
    "&mainbody%%3AsearchPersRecord%%3AsearchHeadField6_BadgeDesc="
    "&mainbody%%3AsearchPersRecord%%3AbadgeNoKey=searchHeadField4"
    "&mainbody%%3AsearchPersRecord%%3AselectSearchProfile_inputHidden="
    "&mainbody%%3AsearchPersRecord%%3Asearchresulttable_reflowDD=5_0"
    "&mainbody%%3AsearchPersRecord_SUBMIT=1"
    "&javax.faces.ViewState=%s"
    "&mainbody%%3AsearchPersRecord%%3Asearchresulttable%%3A0%%3Acolumns%%3A4%%3Aj_id_8q="
    "mainbody%%3AsearchPersRecord%%3Asearchresulttable%%3A0%%3Acolumns%%3A4%%3Aj_id_8q";    // clen 916 (915-917)

const char* mpartboundary = "----WebKitFormBoundaryxRKaq1BRumca55Nt";

const char* cdisposh_fmt = "Content-Disposition: form-data; name=\"%s\"\r\n\r\n";

const char* mpartrfields[82] = {    
    "javax.faces.partial.ajax",
    "javax.faces.source",
    "javax.faces.partial.execute",
    "javax.faces.partial.render",
    "javax.faces.behavior.event",
    "javax.faces.partial.event",
    "mainbody:editPersRecord:persRecordTabs_newTab",
    "mainbody:editPersRecord:persRecordTabs_tabindex",
    //
    "javax.faces.partial.ajax",
    "javax.faces.source",
    "javax.faces.partial.execute",          // 10
    "javax.faces.partial.render",
    "mainbody:editPersRecord:j_id_5c",
    //
    "uniqueToken",
    //
    "mainbody:editPersRecord:j_id_4x",
    "mainbody:editPersRecord:j_id_4y",
    //
    "mainbody:editPersRecord:saveButton",
    //
    "autoScroll",
    "mainbody:editPersRecord:badgeNoKey",
    "mainbody:editPersRecord:editHeadField1_Surname",
    "mainbody:editPersRecord:editHeadField3_Firstname",         // 20
    "mainbody:editPersRecord:editHeadField5_Division_inputHidden",
    "mainbody:editPersRecord:editHeadField5_Division_inputText",
    "mainbody:editPersRecord:editHeadField2_PersNr",
    "mainbody:editPersRecord:correctionToCancel",
    "mainbody:editPersRecord:persRecordTabs:tabid1:tab1Field1_AccessState",
    "mainbody:editPersRecord:persRecordTabs:tabid1:tab1Field2_LockingReason_inputHidden",
    "mainbody:editPersRecord:persRecordTabs:tabid1:tab1Field2_LockingReason_inputText",
    "mainbody:editPersRecord:persRecordTabs:tabid1:tab1Field3_BocAuthorisation_inputHidden",
    "mainbody:editPersRecord:persRecordTabs:tabid1:tab1Field3_BocAuthorisation_inputText",
    "mainbody:editPersRecord:persRecordTabs:tabid1:tab1Field4_PinCode",             // 30
    "mainbody:editPersRecord:persRecordTabs:tabid1:logTable:actDay_input",
    "mainbody:editPersRecord:persRecordTabs:tabid2:accessCalendar_inputHidden",
    "mainbody:editPersRecord:persRecordTabs:tabid2:accessCalendar_inputText",
    "mainbody:editPersRecord:persRecordTabs:tabid2:officeRelease_inputHidden",
    "mainbody:editPersRecord:persRecordTabs:tabid2:officeRelease_inputText",
    "mainbody:editPersRecord:persRecordTabs:tabid2:authorisationLevel_inputHidden",
    "mainbody:editPersRecord:persRecordTabs:tabid2:authorisationLevel_inputText",
    "mainbody:editPersRecord:persRecordTabs:tabid2:accessGrantedFromCal_input",
    "mainbody:editPersRecord:persRecordTabs:tabid2:fromHour",
    "mainbody:editPersRecord:persRecordTabs:tabid2:accessGrantedUntilCal_input",        // 40
    "mainbody:editPersRecord:persRecordTabs:tabid2:tilHour",
    //
    "mainbody:editPersRecord:persRecordTabs:tabid2:tableAccProfiles:accessProfileNumber_inputHidden",
    "mainbody:editPersRecord:persRecordTabs:tabid2:tableAccProfiles:accessProfileNumber_inputText",
    "mainbody:editPersRecord:persRecordTabs:tabid2:tableAccProfiles:validFrom_input",
    "mainbody:editPersRecord:persRecordTabs:tabid2:tableAccProfiles:validTo_input",
    //
    "mainbody:editPersRecord:persRecordTabs:tabid2:tableAccProfiles:acceptAdd_j_id__v_71",
    //
    "mainbody:editPersRecord:persRecordTabs:tabid2:selectOnlyActual_input",
    //
    "mainbody:editPersRecord:persRecordTabs:tabid2:tableAccProfiles:addRow_j_id__v_76",
    //
    "mainbody:editPersRecord:persRecordTabs:tabid2:lockSchedulesPersonGroups_inputHidden",
    "mainbody:editPersRecord:persRecordTabs:tabid2:lockSchedulesPersonGroups_inputText",    // 50
    "mainbody:editPersRecord:persRecordTabs:tabid3:aocValidityPeriodId",
    "mainbody:editPersRecord:persRecordTabs:tab4Field1_Street",
    "mainbody:editPersRecord:persRecordTabs:tab4Field3_ZipCity",
    "mainbody:editPersRecord:persRecordTabs:tab4Field5_PrivatePhone",
    "mainbody:editPersRecord:persRecordTabs:tab4Field7_PrivateMobilePhone",
    "mainbody:editPersRecord:persRecordTabs:tab4Field9_PrivateEMail",
    "mainbody:editPersRecord:persRecordTabs:tab4Field2_Title_inputHidden",
    "mainbody:editPersRecord:persRecordTabs:tab4Field2_Title_inputText",
    "mainbody:editPersRecord:persRecordTabs:tab4Field4_DayOfBirth_input",
    "mainbody:editPersRecord:persRecordTabs:tab4Field6_Gender_inputHidden",         // 60
    "mainbody:editPersRecord:persRecordTabs:tab4Field6_Gender_inputText",
    "mainbody:editPersRecord:persRecordTabs:tab4Field8_MaritalStatus_inputHidden",
    "mainbody:editPersRecord:persRecordTabs:tab4Field8_MaritalStatus_inputText",
    "mainbody:editPersRecord:persRecordTabs:tab4Field10_PrivateComment",
    "mainbody:editPersRecord:persRecordTabs:tab5Field1_DayOfJoining_input",
    "mainbody:editPersRecord:persRecordTabs:tab5Field3_PlaceOfEmployment",
    "mainbody:editPersRecord:persRecordTabs:tab5Field5_Telephone",
    "mainbody:editPersRecord:persRecordTabs:tab5Field7_MobilePhone",
    "mainbody:editPersRecord:persRecordTabs:tab5Field9_EMail",
    "mainbody:editPersRecord:persRecordTabs:tab5Field2_DayOfSeparation_input",      // 70
    "mainbody:editPersRecord:persRecordTabs:tab5Field4_Role",
    "mainbody:editPersRecord:persRecordTabs:tab5Field6_Attribute",
    "mainbody:editPersRecord:persRecordTabs:tab5Field8_OfficialComment",
    "mainbody:editPersRecord:persRecordTabs:tabid6:bemerkungenArea",
    "mainbody:editPersRecord:persRecordTabs:tabid6:zusatzinformationenArea",
    "mainbody:editPersRecord:persRecordTabs:tabid10:user",
    "mainbody:editPersRecord:persRecordTabs:tabid10:ExtendedUsrSelection_inputHidden",
    "mainbody:editPersRecord:persRecordTabs:tabid10:ExtendedUsrSelection_inputText",
    "mainbody:editPersRecord:persRecordTabs_activeIndex",
    "mainbody:editPersRecord_SUBMIT",                                   // 80
    "javax.faces.ViewState"
};

const char* searchfields[15] = {
    "uniqueToken",
    "mainbody:editPersRecord:editHeadField1_Surname",
    "mainbody:editPersRecord:editHeadField3_Firstname",
    "mainbody:editPersRecord:editHeadField5_Division_inputHidden",
    "mainbody:editPersRecord:editHeadField5_Division_inputText",
    "mainbody:editPersRecord:editHeadField2_PersNr",
    "mainbody:editPersRecord:persRecordTabs:tabid1:tab1Field1_AccessState",
    "mainbody:editPersRecord:persRecordTabs:tabid1:tab1Field2_LockingReason_inputHidden",
    "mainbody:editPersRecord:persRecordTabs:tabid1:logTable:actDay_input",
    "mainbody:editPersRecord:persRecordTabs:tabid2:officeRelease_inputHidden",
    "mainbody:editPersRecord:persRecordTabs:tabid2:officeRelease_inputText",
    "mainbody:editPersRecord:persRecordTabs:tabid2:selectOnlyActual_input",
    "mainbody:editPersRecord:persRecordTabs:tabid2:lockSchedulesPersonGroups_inputHidden",
    "mainbody:editPersRecord:persRecordTabs_activeIndex",
    "javax.faces.ViewState"
};

const unsigned fieldIndices[11] = {13, 19, 20, 21, 22, 23, 25, 26, 31, 34, 35};

const int inc_chtab[70] = {
    0, 1, 2, 3, 4, 5, 6, 7, 13, 14, 15, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41,
    47, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81
};

const int inc_addln[63] = {
    13, 14, 15, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 47, 48, 49, 50, 51, 52,
    53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81
};

const int inc_selprf[70] = {
    0, 1, 2, 3, 12, 13, 14, 15, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44,
    45, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81
};

const int inc_cmtln[66] = {
    13, 14, 15, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 49,
    50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81
};

const int inc_save[63] = {
    13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 47, 49, 50, 51, 52,
    53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81
};

field_t cnf_chtab[12] = {
    {0, "true"}, {1, "mainbody:editPersRecord:persRecordTabs"}, {2, "mainbody:editPersRecord:persRecordTabs"},
    {3, "@none"}, {4, "tabChange"}, {5, "tabChange"}, {6, "mainbody:editPersRecord:persRecordTabs:tabAccessAuthorisations"},
    {7, "1"}, {47, "on"}, {49, "0"}, {79, "1"}, {80, "1"}
};

field_t cnf_addln[4] = {
    {47, "on"}, {49, "0"}, {79, "1"}, {80, "1"}
};

field_t cnf_selprf[10] = {
    {0, "true"}, {1, "mainbody:editPersRecord:j_id_5c"}, {2, "mainbody:editPersRecord:j_id_5c"},
    {3, "mainbody:editPersRecord:toolbarDiv"}, {12, "mainbody:editPersRecord:j_id_5c"},
    {42, ""}, {43, ""}, {49, "0"}, {79, "1"}, {80, "1"}
};

field_t cnf_cmtln[7] = {
    {42, ""}, {43, ""}, {44, ""}, {45, ""}, {49, "0"}, {79, "1"}, {80, "1"}
};

char hdrbuf[HDR_BUF_SIZE];

char* buildHeaders(const char* url, const char* port, const char* ref, const char* ssn, unsigned numHeaders, ...)
{
    if (!url || !port || !ref || !numHeaders)
        return "\0";

    va_list args;

    va_start(args, numHeaders);

    size_t len = 0;

    for (unsigned i = 0; i < numHeaders; i++) {
        unsigned char h = va_arg(args, int);

        switch (h) {
        case 0:
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 9:
        case 10:
        case 11:
            len += snprintf(&hdrbuf[len], HDR_BUF_SIZE - len, "%s\r\n", h_headers[h]);
            break;

        case 6:
            len += snprintf(&hdrbuf[len], HDR_BUF_SIZE - len, h_headers[h], url, port, ref);
            break;

        case 7:
        case 8:
            len += snprintf(&hdrbuf[len], HDR_BUF_SIZE - len, h_headers[h], url, port);
            break;

        case 12:
        case 13:
        case 14:
            if (!ssn)
                error("SSN buf null ref");

            len += snprintf(&hdrbuf[len], HDR_BUF_SIZE - len, h_headers[h], ssn);
            break;

        default:
            error("Invalid headers idx");
            continue;
        }

        if (len >= HDR_BUF_SIZE)
            error("Header buffer full");
    }

    va_end(args);

    return hdrbuf;
}

size_t loginRequest(char* txbuf, size_t txbufSize)
{
    char clbuf[32];

    unsigned len = snprintf(clbuf, 32, h_clen_fmt, 49);

    if (len >= 32)
        error("Login request: content-len buffer full");

    len = snprintf
    (
        txbuf, txbufSize, stdreq_fmt,
        m_post, e_login, h_ctypexf,
        clbuf, "\0", "\0", s_url, s_port,
        e_login, s_url, s_port,
        s_url, s_port, p_login
    );

    if (len >= txbufSize)
        error("Login request: request buffer full");

    return len;
}

size_t redirectLoginRequest(char* txbuf, size_t txbufSize, const char* ssn)
{
    char ssnbuf[300];

    unsigned len = snprintf(ssnbuf, 300, h_jsession_fmt, ssn);

    if (len >= 300)
        error("Redirect login request: cookie buffer full");

    len = snprintf
    (
        txbuf, txbufSize, stdreq_fmt,
        m_get, e_menu, "\0", "\0", ssnbuf,
        "\0", s_url, s_port, e_login, s_url,
        s_port, s_url, s_port, "\0"
    );

    if (len >= txbufSize)
        error("Redirect login request: request buffer full");

    return len;
}

size_t buildGetRequest(char* txbuf, size_t size, const char* endpt, char* headers)
{
    size_t len = snprintf(txbuf, size, req_fmt, m_get, endpt, "\0", headers, "\0");

    if (len >= size)
        error("Get: Request buffer full");

    return len;
}

size_t buildPostRequest(char* txbuf, size_t size, const char* endpt, char* headers, char* ssnt, char* vsta, const char* persNo, const char* payld)
{
    char payldbuf[1024];

    size_t len;

    if (persNo)
        len = snprintf(payldbuf, 1024, payld, ssnt, persNo, vsta);
    else
        len = snprintf(payldbuf, 1024, payld, ssnt, vsta);

    if (len >= 1024)
        error("Post: Payload buffer full");

    char clenbuf[32];

    len = snprintf(clenbuf, 32, h_clen_fmt, len);
    
    if (len >= 32)
        error("Post: Content len buffer full");

    len = snprintf(txbuf, size, req_fmt, m_post, endpt, clenbuf, headers, payldbuf);
    
    if (len >= size)
        error("Post: Request buffer full");

    return len;
}

// copies payload from txbuf to local buf, writes request in txbuf
// returns new length
size_t buildMPtPostRequest(char* txbuf, size_t size, const char* endpt, char* headers)
{
    char tmp[12000];

    size_t len = strnlen(txbuf, 12000);

    if (len == 12000)
        error("MPartreq: Payload too long");

    memcpy(tmp, txbuf, len * sizeof(char));
    tmp[len] = '\0';

    char clenbuf[32];

    len = snprintf(clenbuf, 32, h_clen_fmt, len);

    if (len >= 32)
        error("MPartreq: Content len buffer full");

    len = snprintf(txbuf, size, req_fmt, m_post, endpt, clenbuf, headers, tmp);

    if (len >= size)
        error("MPartreq: Request buffer full");

    // printf("len %d\n\n%s\n\n", len, txbuf);

    return len;
}

// fieldVals must contain global indices of fields
// fieldVals must be in order
size_t buildMPartPayload(char* buf, size_t size, const int* incHdrs, size_t incLen, const field_t* fldVals, size_t fldLen)
{
    char hbuf[200];
    const char* tmp;

    unsigned hlen, len = 2, fidx = 0;

    buf[0] = '\r';
    buf[1] = '\n';

    for (unsigned i = 0; i < incLen; i++) {
        hlen = snprintf(hbuf, 200, cdisposh_fmt, mpartrfields[incHdrs[i]]);

        if (hlen >= 200)
            error("MPartpld: Header buffer full");

        if (fidx < fldLen && fldVals[fidx].idx == incHdrs[i])
            tmp = fldVals[fidx++].val ? fldVals[fidx - 1].val : "\0";
        else
            tmp = "\0";
        
        len += snprintf(&buf[len], size - len, "--%s\r\n%s%s\r\n", mpartboundary, hbuf, tmp);

        if (len >= size)
            error("MPartpld: Request buffer full");
    }

    len += snprintf(&buf[len], size - len, "--%s--", mpartboundary);

    if (len >= size)
        error("MPartpld: Request buffer full");

    return len;
}

bool isIncluded(int idx, int* inc, size_t incLen)
{
    if (!inc || !incLen)
        return 0;

    for (size_t i = 0; i < incLen; i++) {
        if (inc[i] == idx)
            return i + 1;
    }

    return 0;
}