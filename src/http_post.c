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
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include "error.h"
/*------------------------------ Private variables ---------------------------*/

static char apszurl[][128] = {"/cgi-bin/admin/upload_ovpn_key.cgi",
                             "/cgi-bin/admin/upload_ovpn_ca.cgi",
                             "/cgi-bin/admin/upload_ovpn_clientcrt.cgi",
                             "/cgi-bin/admin/upload_ovpn_clientkey.cgi",
                             "/cgi-bin/admin/upload_ovpn_conffile.cgi",
                             };

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
        char *pszciphertext;
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
        printf( "**%s**", header);

        if (strcmp(header, "\n") == 0)
        {
            // Empty line signals and flag of header_end is TRUE means end of HTTP Headers
            return;

        }
        if (strstr(header,"--------------------------") != NULL )
        {
            printf("this is body header");
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
            pstreq_header_info->stauth.ucflag = TRUE;
            printf("%s",header_value_start);
            szencryptmethod = strchr(header_value_start, ' ');
            len = szencryptmethod-header_value_start ;
            memcpy(pstreq_header_info->stauth.szauth_method,header_value_start,len);
            pstreq_header_info->stauth.szauth_method[len+1] = '\0';

            szencryptmethod++;
            pszciphertext = strchr(szencryptmethod,'\n');
            len = pszciphertext - szencryptmethod;
            memcpy(pstreq_header_info->stauth.szauth_cipher,szencryptmethod,len);
            pstreq_header_info->stauth.szauth_cipher[len+1] = '\0';
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
        else if (strncasecmp(header, "Content-Disposition",header_type_len) == 0)
        {
            if(ibodyflag)
            {
                strcpy(pstreq_header_info->stbodyheader.szdcontdisp, header_value_start);
            }
        }
    }
}
void getfilename(St_header_info *pstheaderinfo,char * szfilename)
{
    char *pszcntdsp;
    char *psz;
    int inamelen = 0;
    ASSERT(pstheaderinfo != NULL);
    pszcntdsp = strstr(pstheaderinfo->stbodyheader.szdcontdisp, "filename");
    if(pszcntdsp)
    {
//        msg(M_INFO,"pszcntdsp is %s", pszcntdsp);
        pszcntdsp = strstr(pszcntdsp,"\"");
        inamelen = strlen(pszcntdsp);
        psz = pszcntdsp+1 ;
        psz[inamelen-3] = '\0';   // 3 is " + \r + \n
    }
    msg(M_INFO,"filename is %s", psz);
    memcpy(szfilename,psz,strlen(psz)+1);
}
static int write_file(int fd, int ilength,char *pszboundary)
{
    char header[8096];
    int len;
    char szfilename[128] = { 0 };
    int ibdylen = strlen(pszboundary);
    St_header_info stheaderinfo ;
    read_post_headers(fd, &stheaderinfo);
    getfilename(&stheaderinfo,szfilename);
    pszboundary[ibdylen-2] = '\0';
    pszboundary +=24 ; // ignore the boundary symble ----------

    int fp = open(szfilename,O_RDWR | O_CREAT);
    if(fp < 0)
    {
        msg(M_ERRNO,"open file %s failed",szfilename);
        return -1;
    }
    msg(M_INFO,"%s",pszboundary);
    while(ilength > 0)
    {
        len = read_line(fd, header, sizeof(header));
        ilength -= len;
        msg(M_INFO,"%s",header);
        if (strstr(header,pszboundary))
        {
            msg(M_INFO,"read to file end");
            break;
        }
        else
        {
            write(fp,header,len);
        }
    }
    close(fp);
    return 0;
}

int http_post_request(st_http_session *pst_session)
{
    int ret,cmdindex;
    int pf;
    int ilength;
    char *pszboundary ;
    St_header_info st_http_req_header_info;
    ASSERT(pst_session->fd > 0);

    cmdindex = checkurl(pst_session->szurl,apszurl,5);
    printf("%d\n",cmdindex);
    if(!ret)
    {
        not_found(pst_session->fd);
        keep_alive = FALSE;
        return 0;
    }
    memset(&st_http_req_header_info, 0, sizeof(st_http_req_header_info));
    read_post_headers(pst_session->fd,&st_http_req_header_info);
    printf("adfasdf\n");
    if (!st_http_req_header_info.stauth.ucflag)
    {
        printf("unauthorized\n");
        Unauthorized_response(pst_session->fd);
        keep_alive = FALSE;
        return 0;
    }
    else
    {
        ret = check_authorization(st_http_req_header_info.stauth.szauth_cipher);
        if(!ret)
        {
            printf("Please check your username and pwd!\n");
            Authorizedfailed_response(pst_session->fd);
            keep_alive = FALSE;
            return ret;
        }
    }
    pszboundary = strstr(st_http_req_header_info.szconttype,"boundary=");
    printf( "boundary is **%s**",pszboundary);
    if(pszboundary)
    {
        pszboundary = strstr(pszboundary, "----");
        printf( "boundary is *%s*, len is %d",pszboundary,strlen(pszboundary));
    }

    ilength = st_http_req_header_info.icontent_length;
    if ( ilength > 0)
    {
        printf("content length is %d",ilength);

//        content = (char*) malloc(ilength + 1);
//
//        int bytes = read_socket(pst_session->fd,content,ilength);
//        printf("content is %s, bytes is %d\n",content, bytes);
//        fflush(stdout);
        ret = write_file(pst_session->fd, ilength, pszboundary);
        if (ret < 0 )
        {
            return -1;
        }
    }
    printf("adfasdf\n");
    ok(pst_session->fd);

    return 0;
}

