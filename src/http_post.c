/*
 ============================================================================
 Name        : http_post.c
 Author      : junma
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include "http_server.h"

typedef struct user{
    char user[128];
    char pwd [128];
}USER;

typedef struct stbody_header_info{
char szdcontdisp[128];
char szcontype[128];
}Stbody_header;

typedef struct st_header_info{
int iheaderlen;
int icontent_length;
char szhost[32];
char szusragt[64];
char szconttype[128];
char szAuthorization[128];
Stbody_header stbodyheader;
}St_header_info;


USER astuser[] = {
        {"admin","hik12345"},
        {"junma","nsn@2014"},
};
/********************************Private Function*********************************/
//char * get_auth_info(char * pszstr)
//{
//    ASSERT(NULL != pszstr);
//    char pauthmethod[16] = {0};
//
//
//    char * pszciphertext;
//    int authmethod_len;
//    int  uname_len;
//    pszciphertext = strchr(pszstr, ' ');
//    msg(M_INFO,"ciphertext is %s",pszciphertext);
//    authmethod_len = pszciphertext - pszstr;
//    pszciphertext++;
//    msg(M_INFO,"%d",authmethod_len);
//    memcpy(pauthmethod,pszstr,authmethod_len);
//    pauthmethod[authmethod_len] = '\0';
//    msg(M_INFO,"crypto method is %s",pauthmethod);
////    msg(M_INFO,"ciphertext is %s",pszciphertext);
//    return pszciphertext;
//
//
//}
//
//int auth_user(char *usr,char *pwd)
//{
//    USER *pstusr;
//    pstusr = astuser;
//    int i ;
//    int size = sizeof(astuser)/sizeof(struct user);
//    for (i = 0; i < size; i ++)
//    {
//        msg(M_INFO,"-----");
//        if (0 == strcmp(astuser[i].user,usr) && 0 == strcmp(astuser[i].pwd,pwd))
//            return TRUE;
//    }
//    return FALSE;
//}
//
//int check_authorization(char *pszciphertext)
//{
//    char *user;
//    char *pwd;
//    int  uname_len;
//    char * pcipher;
//    char szplaintext[256]       = {0};
//    pcipher = get_auth_info(pszciphertext);
//    base64_decode(pcipher,szplaintext,sizeof(szplaintext));
//    pwd = strchr(szplaintext,':');
//    uname_len = (pwd++) - szplaintext;
//    szplaintext[uname_len] = '\0';
//    user = szplaintext;
//    msg(M_INFO,"user is %s,pwd is **%s**",user,pwd);
//
//    return auth_user(user,pwd);
//}


static void ok(int client_sockfd)
{
    char buffer[8096];
    char body[8096];
    sprintf(buffer, "HTTP/1.1 201 OK\r\n");
    write_socket(client_sockfd, buffer, strlen(buffer));
    sprintf(buffer, SERVER_STRING);
    write_socket(client_sockfd, buffer, strlen(buffer));
    sprintf(buffer, "Allow: POST\r\n");
    write_socket(client_sockfd, buffer, strlen(buffer));
    sprintf(buffer, "Content-Type: text/html\r\n");
    write_socket(client_sockfd, buffer, strlen(buffer));

    sprintf(body, "<HTML><HEAD><TITLE>POST OK</TITLE></HEAD>\r\n");
    sprintf(body + strlen(body), "<BODY><P>POST OK.</P></BODY></HTML>\r\n");

    sprintf(buffer, "Content-Length: %ld\r\n", strlen(body));
    write_socket(client_sockfd, buffer, strlen(buffer));
    sprintf(buffer, "\r\n");
    write_socket(client_sockfd, buffer, strlen(buffer));

    write_socket(client_sockfd, body, strlen(body));
}

static void read_post_headers(int fd,St_header_info *pstreq_header_info)
{

    int ibodyflag = 0;
    // fprintf(stderr, "\n--READ HEADERS--\n\n");
    while(1)
    {
        char header[8096];
        int len;
        char next;
        int err;
        char *header_value_start;
        char *szencryptmethod;
        char *pszcontent_type;
        char *pszboundary;
        len = read_line(fd, header, sizeof(header));

//        if (content_length > 0 && len > 0)
//        {
//            content_length = content_length - len;
//        }

        if (len <= 0)
        {
            // Error in reading from socket
            header_err_flag = TRUE;
            continue;
        }
        pstreq_header_info->iheaderlen += len;
        msg(M_INFO, "**%s**", header);

        if (strcmp(header, "\n") == 0)
        {
            // Empty line signals and flag of header_end is TRUE means end of HTTP Headers
            return;

        }
        if (strstr(header,"--------------------------") != NULL )
        {
            ibodyflag = 1;
            continue;
        }
        // If the next line begins with a space or tab, it is a continuation of the previous line.
        err = recv(fd, &next, 1, MSG_PEEK);
        while (isspace(next) && next != '\n' && next != '\r')
        {
            if (err)
            {
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
        while (isspace(*header_value_start) && (*header_value_start != '\n') && (*header_value_start != '\r'))
        {
            header_value_start++;
        }
//        int header_value_len = len - (header_value_start - header);

        if (strncasecmp(header, "Authorization", header_type_len) == 0)
        {
            msg(M_INFO,"%s",header_value_start);
            szencryptmethod = strchr(header_value_start, ' ');
            Authorization = TRUE;
        }
        else if (strncasecmp(header, "Content-Length", header_type_len) == 0)
        {
            pstreq_header_info->icontent_length = atoi(header_value_start);
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
        else if (strncasecmp(header, "Accept-Encoding", header_type_len) == 0)
        {
            acceptable_encoding = FALSE;
        }
        else if (strncasecmp(header, "User-Agent", header_type_len) == 0)
        {
            strcpy(user_agent, header_value_start);
        }
        else if (strncasecmp(header, "Content-Type",header_type_len) == 0)
        {
            if(ibodyflag)
            {
                strcpy(pstreq_header_info->stbodyheader.szcontype, header_value_start);
            }
            else
            {
                strcpy(pstreq_header_info->szconttype,header_value_start);
            }
        }
    }
}

static void write_file(int fd, int ilength,char *pszboundary)
{
    char header[8096];
    int len;
    char szfilename[128] = { 0 };
    St_header_info stheaderinfo ;
    read_post_headers(fd, &stheaderinfo);

    ilength -=stheaderinfo.icontent_length ;
    while(ilength > 0)
    {
        len = read_line(fd, header, sizeof(header));
        ilength -= len;
        msg(M_INFO,"%s",header);
        if (strstr(header,))
    }

}

int http_post_request(st_http_session *pst_session)
{
    int pf;
    int ilength;
    char pszboundary ;
    St_header_info st_http_req_header_info;
    ASSERT(pst_session->fd > 0);
    memset(&st_http_req_header_info, 0, sizeof(st_http_req_header_info));
    msg(M_INFO,"this is post request");
    read_post_headers(pst_session->fd,&st_http_req_header_info);
    printf("adfasdf\n");
    if (!Authorization)
    {
        msg(M_INFO,"unauthorized");
        Unauthorized_response(pst_session->fd);
        keep_alive = FALSE;
        return -1;
    }

    pszboundary = strstr(st_http_req_header_info.szconttype,"boundary=");
    msg(M_INFO, "boundary is **%s**",pszboundary);
    if(pszboundary)
    {
        pszboundary = strstr(pszboundary, "----");
        msg(M_INFO, "boundary is *%s*, len is %d",pszboundary,sizeof(pszboundary));
    }

    ilength = st_http_req_header_info.icontent_length;
    if ( ilength > 0)
    {
        msg(M_INFO,"content length is %d",ilength);

//        content = (char*) malloc(ilength + 1);
//
//        int bytes = read_socket(pst_session->fd,content,ilength);
//        msg(M_INFO,"content is %s, bytes is %d\n",content, bytes);
//        fflush(stdout);
        write_file(pst_session->fd,ilength,char *pszboundary);
    }
    printf("adfasdf\n");
    ok(pst_session->fd);

    return 0;
}
