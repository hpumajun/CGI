/*
 ============================================================================
 Name        : http_server.c
 Author      : junma
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */


#define _XOPEN_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>

#include "error.h"
#include "http_get.h"
#include "http_post.h"
#include "http_server.h"

#define DECODE_ERROR 0xffffffff

#define PORT 8888
#define HOST_NAME "127.0.0.1"
#define MAX_CONNECTIONS 5
#define MAX_TIMESTAMP_LENGTH 64


#define MAX_RETRIES 1


/************************************Variable*************************************/

int sockfd; // Listening socket
int client_sockfd; // Connected socket
int child;


time_t seconds;
struct tm *timestamp;
char timestamp_str[MAX_TIMESTAMP_LENGTH];

// Value specific to client connections
int keep_alive = TRUE; // Default in HTTP/1.1
int content_length = -1;
int cookie = FALSE;
int header_err_flag = FALSE;
struct tm *if_modified_since;
int time_is_valid = TRUE;
char *content = NULL;
int not_eng = FALSE;
int acceptable_text = TRUE;
int acceptable_charset = TRUE;
int acceptable_encoding = TRUE;


char user_agent[512];


CGIUSER astuser[] = {
        {"admin","hik12345"},
        {"junma","nsn@2014"},
};


static char base64_chars[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/********************************Private Function*********************************/

/**************************************************************************
*FuncName    : pos
*Author        : majun11
*Description :
*CreateDate  :2017/1/9 10:35:19
*Parameter IN:
*           OUT:
*Return      :
**************************************************************************/
static int pos(char c)
{
    char *p;
    for (p = base64_chars; *p; p++)
    if (*p == c)
        return p - base64_chars;
    return -1;
}

/**************************************************************************
*FuncName    : token_decode
*Author        : majun11
*Description :
*CreateDate  :2017/1/9 10:35:33
*Parameter IN:
*           OUT:
*Return      :
**************************************************************************/
static unsigned int token_decode(const char *token)
{
    int i;
    unsigned int val = 0;
    int marker = 0;
    if (!token[0] || !token[1] || !token[2] || !token[3])
    return DECODE_ERROR;
    for (i = 0; i < 4; i++) {
    val *= 64;
    if (token[i] == '=')
        marker++;
    else if (marker > 0)
        return DECODE_ERROR;
    else
        val += pos(token[i]);
    }
    if (marker > 2)
    return DECODE_ERROR;
    return (marker << 24) | val;
}

/**************************************************************************
*FuncName    : get_auth_info
*Author        : majun11
*Description :
*CreateDate  :2017/1/9 10:29:55
*Parameter IN:
*           OUT:
*Return      :
**************************************************************************/
/*
static char * get_auth_info(char * pszstr)
{
    ASSERT(NULL != pszstr);
    char pauthmethod[16] = {0};


    char * pszciphertext;
    int authmethod_len;
    int  uname_len;
    pszciphertext = strchr(pszstr, ' ');
    msg(M_INFO,"ciphertext is %s",pszciphertext);
    authmethod_len = pszciphertext - pszstr;
    pszciphertext++;
    msg(M_INFO,"%d",authmethod_len);
    memcpy(pauthmethod,pszstr,authmethod_len);
    pauthmethod[authmethod_len] = '\0';
    msg(M_INFO,"crypto method is %s",pauthmethod);
//    msg(M_INFO,"ciphertext is %s",pszciphertext);
    return pszciphertext;

}
*/
static int gethttpmethod(char *pszmethod)
{
    int type = 0;
    if(0 == strcmp(pszmethod,"GET"))
    {
        type = GET;
    }
    else if (0 == strcmp(pszmethod,"POST"))
    {
        type = POST;
    }
    else if (0 == strcmp(pszmethod,"HEAD"))
    {
        type = HEAD;
    }
    else
    {
        type = INVALID;
    }
    return type;
}

/********************************Public Function *********************************/
int http_base64_decode(const char *str, void *data, int size)
{
    const char *p;
    unsigned char *q;
    unsigned char *e = NULL;

    q = data;
    if (size >= 0)
      e = q + size;
    for (p = str; *p && (*p == '=' || strchr(base64_chars, *p)); p += 4) {
    unsigned int val = token_decode(p);
    unsigned int marker = (val >> 24) & 0xff;
    if (val == DECODE_ERROR)
        return -1;
    if (e && q >= e)
      return -1;
    *q++ = (val >> 16) & 0xff;
    if (marker < 2)
      {
        if (e && q >= e)
          return -1;
        *q++ = (val >> 8) & 0xff;
      }
    if (marker < 1)
      {
        if (e && q >= e)
          return -1;
        *q++ = val & 0xff;
      }
    }
    return q - (unsigned char *) data;
}


/**************************************************************************
*FuncName    : auth_user
*Author        : majun11
*Description :
*CreateDate  :2017/1/9 10:30:10
*Parameter IN:
*           OUT:
*Return      :
**************************************************************************/
int auth_user(char *usr,char *pwd)
{
    CGIUSER *pstusr;
    pstusr = astuser;
    int i ;
    int size = sizeof(astuser)/sizeof(struct user);
    for (i = 0; i < size; i ++)
    {
        if (0 == strcmp(astuser[i].user,usr) && 0 == strcmp(astuser[i].pwd,pwd))
            return TRUE;
    }
    return FALSE;
}

/**************************************************************************
*FuncName    : check_authorization
*Author        : majun11
*Description :
*CreateDate  :2017/1/9 10:30:14
*Parameter IN:
*           OUT:
*Return      :
**************************************************************************/
int check_authorization(char *pszciphertext)
{
    printf("cipher is %s\n", pszciphertext);
    char *user;
    char *pwd;
    int  uname_len;
    char * pcipher;
    char szplaintext[256]       = {0};
//    pcipher = get_auth_info(pszciphertext);
    http_base64_decode(pszciphertext,szplaintext,sizeof(szplaintext));
    pwd = strchr(szplaintext,':');
    uname_len = (pwd++) - szplaintext;
    szplaintext[uname_len] = '\0';
    user = szplaintext;
    printf("user is %s,pwd is **%s**",user,pwd);

    return auth_user(user,pwd);
}
/**************************************************************************
*FuncName    : checkurl
*Author        : majun11
*Description :
*CreateDate  :2017/1/9 10:30:14
*Parameter IN: url
*           OUT:
*Return      : return the cmd index, so that we can know which cmd user want to
*              execute
**************************************************************************/
int checkurl(char *pszurl,const char (*aaszurl)[128],int urlcnt)
{

    int i = 0;
    if (NULL == pszurl)
    {
        printf("url is NULL\n");
        return FALSE;
    }
    for ( i = 0 ; i < urlcnt ; i++)
    {
        printf("aaszurl is %s\n", aaszurl[i]);
        if ( 0 == strcmp(pszurl,aaszurl[i]))
        {
            return i;
        }
    }
    return INVALID;
}

/**************************************************************************
*FuncName    : read_line
*Author        : majun11
*Description :
*CreateDate  :2017/1/9 10:30:19
*Parameter IN:
*           OUT:
*Return      :
**************************************************************************/
int read_line(int fd, char *buffer, int size)
{
    char next = '\0';
    char err;
    int i = 0;
    while ((i < (size - 1)) && (next != '\n'))
    {
        err = read(fd, &next, 1);

        if (err <= 0)
            break;

        if (next == '\r')
        {
            err = recv(fd, &next, 1, MSG_PEEK);
            if (err > 0 && next == '\n')
            {
                read(fd, &next, 1);
            }
            else
            {
                next = '\n';
            }
        }
        buffer[i] = next;
        i++;
    }

    buffer[i] = '\0';
    return i;
}

int read_socket(int fd, char *buffer, int size)
{
    int bytes_recvd = 0;
    int retries = 0;
    int total_recvd = 0;

    while (retries < MAX_RETRIES && size > 0 && strstr(buffer, ">") == NULL)
    {
        bytes_recvd = read(fd, buffer, size);
 //       printf("bytes_recvd is %d\n",bytes_recvd);
        if (bytes_recvd > 0) {
            buffer += bytes_recvd;
            size -= bytes_recvd;
            total_recvd += bytes_recvd;
        }
        else
        {
            retries++;
        }
    }

    if (bytes_recvd >= 0) {
        // Last read was not an error, return how many bytes were recvd
        return total_recvd;
    }
    // Last read was an error, return error code
    return -1;
}

int write_socket(int fd, char *msg, int size) {
    int bytes_sent = 0;
    int retries = 0;
    int total_sent = 0;

    while (retries < MAX_RETRIES && size > 0) {
        bytes_sent = write(fd, msg, size);

        if (bytes_sent > 0) {
            msg += bytes_sent;
            size -= bytes_sent;
            total_sent += bytes_sent;
        } else {
            retries++;
        }
    }

    if (bytes_sent >= 0) {
        // Last write was not an error, return how many bytes were sent
        return total_sent;
    }
    // Last write was an error, return error code
    return -1;
}

void bad_request(int client_sockfd)
{
    // 400 Error
    char buffer[8096];
    char body[8096];
    sprintf(buffer, "HTTP/1.1 400 Bad Request\r\n");
    write_socket(client_sockfd, buffer, strlen(buffer));
    sprintf(buffer, SERVER_STRING);
    write_socket(client_sockfd, buffer, strlen(buffer));
    sprintf(buffer, "Content-Type: text/html\r\n");
    write_socket(client_sockfd, buffer, strlen(buffer));

    sprintf(body, "<HTML><HEAD><TITLE>Bad Request</TITLE></HEAD>\r\n");
    sprintf(body + strlen(body), "<BODY><P>The request cannot be fulfilled due to bad syntax.</P></BODY></HTML>\r\n");

    sprintf(buffer, "Content-Length: %ld\r\n", strlen(body));
    write_socket(client_sockfd, buffer, strlen(buffer));
    write_socket(client_sockfd, "\r\n", strlen("\r\n"));

    write_socket(client_sockfd, body, strlen(body));
}

void Unauthorized_response(int client_sockfd)
{
    char buffer[8096];
    char body[8096];
    //fill <version> <statucode> <reason-phrase>
    sprintf(buffer, "HTTP/1.1 401 Unauthorization\r\n");
    write_socket(client_sockfd, buffer, strlen(buffer));
    sprintf(buffer, SERVER_STRING);

    //fill <header>
    write_socket(client_sockfd, buffer, strlen(buffer));
    sprintf(buffer, "WWW-Authenticate: Basic realm=""\r\n");
    write_socket(client_sockfd, buffer, strlen(buffer));

    // fill body
    sprintf(body, "<HTML><HEAD><TITLE>Unauthorization Request</TITLE></HEAD>\r\n");
    sprintf(body + strlen(body), "<BODY><P>Client did't authorized by server,please check your usr and pwd.</P></BODY></HTML>\r\n");

    write_socket(client_sockfd, "\r\n", strlen("\r\n"));
    write_socket(client_sockfd, body, strlen(body));
}

void Authorizedfailed_response(int client_sockfd)
{
    char buffer[8096];
    char body[8096];
    //fill <version> <statucode> <reason-phrase>
    sprintf(buffer, "HTTP/1.1 401 Unauthorization\r\n");
    write_socket(client_sockfd, buffer, strlen(buffer));
    sprintf(buffer, SERVER_STRING);

    //fill <header>
    write_socket(client_sockfd, buffer, strlen(buffer));
    sprintf(buffer, "Content-Type: user/pwd\r\n");
    write_socket(client_sockfd, buffer, strlen(buffer));

    // fill body
    sprintf(body, "<HTML><HEAD><TITLE>Authorization Failed</TITLE></HEAD>\r\n");
    sprintf(body + strlen(body), "<BODY><P>Client Authorized failed by server,please check your usr and pwd.</P></BODY></HTML>\r\n");

    write_socket(client_sockfd, "\r\n", strlen("\r\n"));
    write_socket(client_sockfd, body, strlen(body));
}

void not_found(int client_sockfd) {
    // 404 Error
    char buffer[8096];
    char body[8096];
    sprintf(buffer, "HTTP/1.1 404 File Not Found\r\n");
    write_socket(client_sockfd, buffer, strlen(buffer));
    sprintf(buffer, SERVER_STRING);
    write_socket(client_sockfd, buffer, strlen(buffer));
    sprintf(buffer, "Content-Type: text/html\r\n");
    write_socket(client_sockfd, buffer, strlen(buffer));

    sprintf(body, "<HTML><HEAD><TITLE>File Not Found</TITLE></HEAD>\r\n");
    sprintf(body + strlen(body), "<BODY><P>File not found.</P></BODY></HTML>\r\n");

    sprintf(buffer, "Content-Length: %ld\r\n", strlen(body));
    write_socket(client_sockfd, buffer, strlen(buffer));
    write_socket(client_sockfd, "\r\n", strlen("\r\n"));

    write_socket(client_sockfd, body, strlen(body));
}

static void method_not_allowed() {
    // 405 Error
    char buffer[8096];
    char body[8096];
    sprintf(buffer, "HTTP/1.1 501 Method Not Allowed\r\n");
    write_socket(client_sockfd, buffer, strlen(buffer));
    sprintf(buffer, SERVER_STRING);
    write_socket(client_sockfd, buffer, strlen(buffer));
    sprintf(buffer, "Allow: GET\r\n");
    write_socket(client_sockfd, buffer, strlen(buffer));
    sprintf(buffer, "Content-Type: text/html\r\n");
    write_socket(client_sockfd, buffer, strlen(buffer));

    sprintf(body, "<HTML><HEAD><TITLE>Method Not Implemented</TITLE></HEAD>\r\n");
    sprintf(body + strlen(body), "<BODY><P>HTTP request method not supported.</P></BODY></HTML>\r\n");

    sprintf(buffer, "Content-Length: %ld\r\n", strlen(body));
    write_socket(client_sockfd, buffer, strlen(buffer));
    sprintf(buffer, "\r\n");
    write_socket(client_sockfd, buffer, strlen(buffer));

    write_socket(client_sockfd, body, strlen(body));
}

static void server_error() {
    // 500 Error
    char buffer[8096];
    char body[8096];
    sprintf(buffer, "HTTP/1.1 500 Internal Server Error\r\n");
    write_socket(client_sockfd, buffer, strlen(buffer));
    sprintf(buffer, SERVER_STRING);
    write_socket(client_sockfd, buffer, strlen(buffer));
    sprintf(buffer, "Content-Type: text/html\r\n");
    write_socket(client_sockfd, buffer, strlen(buffer));

    sprintf(body, "<HTML><HEAD><TITLE>Internal Server Error</TITLE></HEAD>\r\n");
    sprintf(body + strlen(body), "<BODY><P>The server encountered an unexpected condition which prevented it from fulfilling the request. </P></BODY></HTML>\r\n");

    sprintf(buffer, "Content-Length: %ld\r\n", strlen(body));
    write_socket(client_sockfd, buffer, strlen(buffer));
    write_socket(client_sockfd, "\r\n", strlen("\r\n"));

    write_socket(client_sockfd, body, strlen(body));
}

void not_implemented(int reason)
{
    // 501 Error
    char buffer[8096];
    char body[8096];
    sprintf(buffer, "HTTP/1.1 501 Not Implemented\r\n");
    write_socket(client_sockfd, buffer, strlen(buffer));
    sprintf(buffer, SERVER_STRING);
    write_socket(client_sockfd, buffer, strlen(buffer));
    sprintf(buffer, "Content-Type: text/html\r\n");
    write_socket(client_sockfd, buffer, strlen(buffer));

    sprintf(body, "<HTML><HEAD><TITLE>Not Implemented</TITLE></HEAD>\r\n");
    switch (reason)
    {
        case 0:
            sprintf(body + strlen(body),"<BODY><P> Unsupport cookies. </p><BODY></HTML>\r\n");
            break;
        case 1:
            sprintf(body + strlen(body),"<BODY><P> Unsupport Languagy. </p><BODY></HTML>\r\n");
            break;
        case 2:
            sprintf(body + strlen(body),"<BODY><P> Unsupport plain text. </p><BODY></HTML>\r\n");
            break;
        case 3:
            sprintf(body + strlen(body),"<BODY><P> Unsupport Non-ASCII charset. </p><BODY></HTML>\r\n");
            break;
        default:
            break;
    }
    sprintf(body + strlen(body), "<BODY><P>The server does not support the functionality required to fulfill the request. </P></BODY></HTML>\r\n");

    sprintf(buffer, "Content-Length: %ld\r\n", strlen(body));
    write_socket(client_sockfd, buffer, strlen(buffer));
    write_socket(client_sockfd, "\r\n", strlen("\r\n"));

    write_socket(client_sockfd, body, strlen(body));
}

static void analysis_request(int buffer_len,char * request,st_http_session *pst_session)
{
    int i = 0;
    int j = 0;

    while (!isspace(request[i]))
    {
        pst_session->szmethod[i] = request[i];
        i++;
    }
    pst_session->szmethod[i] = '\0';


    fprintf(stderr, "method: %s\n", pst_session->szmethod);

    // Skip over spaces
    while (i < buffer_len && isspace(request[i])) {
        i++;
    }

    // Get URL
    j = 0;
    while (i < buffer_len && (j < (strlen(pst_session->szurl) - 1)) && !isspace(request[i]))
    {
        pst_session->szurl[j] = request[i];
        printf("url[%d] is %s", j, pst_session->szurl[j]);
        i++;
        j++;
    }
    pst_session->szurl[j] = '\0';

    fprintf(stderr, "url: %s\n", pst_session->szurl);

    // Skip over spaces
    while (i < buffer_len && isspace(request[i])) {
        i++;
    }

    j = 0;
    while (j < sizeof(pst_session->szversion) - 1 && !isspace(request[i])) {
        pst_session->szversion[j] = request[i];
        i++;
        j++;
    }
    pst_session->szversion[j] = '\0';
}


void read_headers(int fd, St_header_info *pstreq_header_info)
{
    char *szencryptmethod;
    char *pszciphertext;
    // fprintf(stderr, "\n--READ HEADERS--\n\n");
    while(1)
    {

        char header[8096];
        int len;
        char next;
        int err;
        char *header_value_start;
        len = read_line(fd, header, sizeof(header));
        if (len <= 0)
        {
            // Error in reading from socket
            header_err_flag = TRUE;
            continue;
        }

        msg(M_INFO, "**%s**", header);

        if (strcmp(header, "\n") == 0) {
            // Empty line signals end of HTTP Headers
            return;
        }

        // If the next line begins with a space or tab, it is a continuation of the previous line.
        err = recv(fd, &next, 1, MSG_PEEK);
        while (isspace(next) && next != '\n' && next != '\r') {
            if (err) {
                fprintf(stderr, "header space/tab continuation check err\n");
                // Not sure what to do in this scenario
            }
            // Read the space/tab and get rid of it
            read(fd, &next, 1);

            // Concatenate the next line to the current running header line
            len = len + read_line(fd, header + len, sizeof(header) - len);
            err = recv(fd, &next, 1, MSG_PEEK);
        }

        // Find first occurence of colon, to split by header type and value
        header_value_start = strchr(header, ':');
        if (header_value_start == NULL) {
            // Invalid header, not sure what to do in this scenario
            fprintf(stderr, "invalid header\n");
            header_err_flag = TRUE;
            continue;
        }
        int header_type_len = header_value_start - header;

        // Increment header value start past colon
        header_value_start++;
        // Increment header value start to first non-space character
        while (isspace(*header_value_start) && (*header_value_start != '\n') && (*header_value_start != '\r')) {
            header_value_start++;
        }
//        int header_value_len = len - (header_value_start - header);


        if (strncasecmp(header, "Connection", header_type_len) == 0)
        {
            // We care about the connection type "keep-alive"
            if (strncasecmp(header_value_start, "keep-alive", strlen("keep-alive")) == 0)
            {
                keep_alive = TRUE;
            }
            else if (strncasecmp(header_value_start, "close", strlen("close")) == 0)
            {
                keep_alive = FALSE;

            }
        }
        else if (strncasecmp(header, "Content-Length", header_type_len) == 0)
        {
            content_length = atoi(header_value_start);
        }
        else if (strncasecmp(header, "Cookie", header_type_len) == 0)
        {
            cookie = TRUE;
        }
        else if (strncasecmp(header, "If-Modified-Since", header_type_len) == 0)
        {
            // Copy everything but trailing newline to buffer
            char *modified_since_buffer = (char*) malloc(sizeof(char) * strlen(header_value_start));
            strncpy(modified_since_buffer, header_value_start, strlen(header_value_start) - 1);

            // Get actual time structure for received time
            strptime(modified_since_buffer, "%a, %d %b %Y %T %Z", if_modified_since);
            free(modified_since_buffer);

            if (if_modified_since == NULL)
            {
                time_is_valid = FALSE;
            }
        }
        else if (strncasecmp(header, "Accept-Language", header_type_len) == 0)
        {
            char *traverse = header_value_start;
            char *temporary;
//            acceptable_text = FALSE;
            while (1)
            {
                if (strncasecmp(traverse, "text/plain", strlen("text/plain")) == 0)
                {
                    acceptable_text = TRUE;
                    break;
                }
                if (strncasecmp(traverse, "en-US", strlen("en-US")) == 0) {
                    acceptable_text = TRUE;
                    break;
                }
                if (strncasecmp(traverse, "en", strlen("en")) == 0) {
                    acceptable_text = TRUE;
                    break;
                }
                temporary = strchr(traverse, ',');
                if (temporary == NULL)
                    break;
                // Skip past comma
                temporary++;
                while(isspace(*temporary))
                    temporary++;
                traverse = temporary;
            }
        }
        else if (strncasecmp(header, "Accept", header_type_len) == 0)
        {
            char *traverse = header_value_start;
            char *temporary;
            acceptable_text = FALSE;
            while (1)
            {
                if (strncasecmp(traverse, "text/plain", strlen("text/plain")) == 0)
                {
                    acceptable_text = TRUE;
                    break;
                }
                if (strncasecmp(traverse, "text/html", strlen("text/html")) == 0)
                {
                    acceptable_text = TRUE;
                    break;
                }
                temporary = strchr(traverse, ',');
                if (temporary == NULL)
                    break;
                // Skip past comma
                temporary++;
                while(isspace(*temporary))
                    temporary++;
                traverse = temporary;
            }
        }
        else if (strncasecmp(header, "Accept-Charset", header_type_len) == 0)
        {
            char *traverse = header_value_start;
            char *temporary;
            acceptable_charset = FALSE;
            while (1)
            {
                if (strncasecmp(traverse, "ISO-8859-1", strlen("ISO-8859-1")) == 0)
                {
                    acceptable_charset = TRUE;
                    break;
                }
                temporary = strchr(traverse, ',');
                if(temporary == NULL)
                    break;
                // Skip past comma
                temporary++;
                while(isspace(*temporary))
                    temporary++;
                traverse = temporary;
            }
        }
        else if (strncasecmp(header, "Accept-Encoding", header_type_len) == 0)
        {
            acceptable_encoding = FALSE;
        }

        else if (strncasecmp(header, "User-Agent", header_type_len) == 0)
        {
            strcpy(user_agent, header_value_start);
        }
        else if (strncasecmp(header, "Authorization", header_type_len) == 0)
        {
            pstreq_header_info->stauth.ucflag = TRUE;
            printf("%s", header_value_start);
            szencryptmethod = strchr(header_value_start, ' ');
            len = szencryptmethod - header_value_start ;
            memcpy(pstreq_header_info->stauth.szauth_method,header_value_start,len);
            pstreq_header_info->stauth.szauth_method[len+1] = '\0';

            szencryptmethod++;
            pszciphertext = strchr(szencryptmethod,'\n');
            len = pszciphertext - szencryptmethod;
            memcpy(pstreq_header_info->stauth.szauth_cipher,szencryptmethod,len);
            pstreq_header_info->stauth.szauth_cipher[len+1] = '\0';
        }
    }
}

int handle_client_connection() {
    char buffer[8096];
    int buffer_len; // Length of buffer

    int request = 0;
    int ret = -1;
    char *pszmethod;
    char *pszurl;
    char *pszversion;
    st_http_session st_session;
    // Read first line
    buffer_len = read_line(client_sockfd, buffer, sizeof(buffer));
    memset(&st_session, 0, sizeof(st_session));
    st_session.fd = client_sockfd;
    // Unable to read from socket, not sure what to do in this case
    if (buffer_len <= 0) {
        return -1;
    }
    else
    {
        msg(M_INFO,"url get from broswer is %s",buffer);
    }

    pszmethod = strchr(buffer,' ');
    request = (pszmethod++) - buffer;
    memcpy(st_session.szmethod,buffer,request);
    st_session.szmethod[request+1] = '\0';
    fprintf(stderr, "method: %s\n", st_session.szmethod);

    pszurl = strchr(pszmethod, ' ');
    request = (pszurl++) - pszmethod;
    memcpy(st_session.szurl, pszmethod, request);
    fprintf(stderr, "url: %s\n", st_session.szurl);
    st_session.szurl[request+1] = '\0';

    pszversion = strchr(pszurl,'\n');
    request = pszversion - pszurl;
    memcpy(st_session.szversion, pszurl, request);
    st_session.szversion[request+1] = '\0';
//    analysis_request(buffer_len,buffer,&st_session);

    request = gethttpmethod(st_session.szmethod);
    printf("operate is **%d**\n",request);
    fprintf(stderr, "==== Read Next Request ====\n");
    switch (request)
    {
        case GET:
            ret = http_get(&st_session);
            break;
        case POST:
            ret = http_post_request(&st_session);
            break;
        default:
            fprintf(stderr, "Method Not Allowed: %s\n", st_session.szmethod);
            method_not_allowed();
            return 0;
    }
    return ret;
}

static void terminate(int err_code) {
    shutdown(client_sockfd, SHUT_RDWR);
    close(client_sockfd);
    close(sockfd);

    exit(err_code);
}

int http_server(void) {
    struct sockaddr_in serverhost;
    struct sockaddr_in clienthost;
    struct hostent *host_entity;
    int addr_len;
        // Clear all invalid data
    memset((char*) &serverhost, 0, sizeof(serverhost));
    // Clear all invalid data
    memset((char*) &serverhost, 0, sizeof(serverhost));

    // Get host information for 127.0.0.1
    host_entity = gethostbyname(HOST_NAME);
    if (host_entity == NULL) {
        msg(M_INFO,"Unable to resolve hostname %s\n", HOST_NAME);
        terminate(1);
        return 1;
    }

    // Copy host information to serverhost
//    memcpy((char*) &serverhost.sin_addr, host_entity->h_addr_list[0], host_entity->h_length);

    serverhost.sin_addr.s_addr = INADDR_ANY;
    serverhost.sin_port = htons((short) PORT);
    serverhost.sin_family = host_entity->h_addrtype;

    // Open socket for listening
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    int tmp = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &tmp, sizeof(tmp));

    if (sockfd < 0) {
        msg(M_INFO,"Unable to open socket");
        terminate(1);
        return 1;
    }

    // Try to bind to socket
    if (bind(sockfd, (struct sockaddr*) &serverhost, sizeof(serverhost)) < 0) {
        msg(M_INFO,"Unable to bind to socket");
        terminate(1);
        return 1;
    }

    // Try to listen to socket
    if (listen(sockfd, MAX_CONNECTIONS) < 0) {
        msg(M_INFO,"Unable to listen on socket");
        terminate(1);
        return 1;
    }

    while(TRUE)
    {
        // Clear all invalid data
        memset(&clienthost, 0, sizeof(clienthost));

        // Try to accept connection with client
        addr_len = sizeof(clienthost);

        client_sockfd = accept(sockfd, (struct sockaddr*) &clienthost, (socklen_t*) &addr_len);


        if (client_sockfd < 0) {
            msg(M_ERRNO,"Unable to accept connection");
            terminate(1);
            return 1;
        }

        printf("%d.%d.%d.%d\n",(int)(clienthost.sin_addr.s_addr&0xFF), (int)((clienthost.sin_addr.s_addr&0xFF00)>>8), (clienthost.sin_addr.s_addr&0xFF0000)>>16, (clienthost.sin_addr.s_addr&0xFF000000)>>24);
        // Timestamp the connection accept
        time(&seconds);
        timestamp = localtime(&seconds);
        memset(timestamp_str, 0, MAX_TIMESTAMP_LENGTH);
        strftime(timestamp_str, MAX_TIMESTAMP_LENGTH, "%r %A %d %B, %Y", timestamp);

        // Get client host
        host_entity = gethostbyaddr((char*) &(clienthost.sin_addr), sizeof(clienthost.sin_addr), AF_INET);


        if (host_entity > 0) {
            msg(M_INFO,"[%s] Connection accepted from %s (%s)", timestamp_str, host_entity->h_name, host_entity->h_addr_list[0]);
        } else {
            msg(M_INFO,"[%s] Connection accepted from unresolvable host", timestamp_str);
        }

        // Handle the new connection with a child process
        child = fork();
        if (child == 0)
        {
            // Continuously perform GET responses until we no longer wish to keep alive
            while (keep_alive)
            {
                content_length = -1;
                cookie = FALSE;
                header_err_flag = FALSE;
                if_modified_since = NULL;
                time_is_valid = TRUE;
                content = NULL;
                not_eng = FALSE;
                acceptable_text = TRUE;
                acceptable_charset = TRUE;
                acceptable_encoding = TRUE;

                handle_client_connection();
                if (content != NULL) free(content);
            }
            exit(0);
        }
    }
    return 0;
}
