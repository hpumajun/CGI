/**
  ******************************************************************************
  * @Filename 	:http_get.c
  * @Author 		:junma
  * @Version  	:V1.0
  * @Date 		:Dec 29, 2016 6:40:53 PM
  * @Brief 		:This file provides all the openvpn functions. 
  ******************************************************************************
  * @attention
  *
  * FILE FOR DMET-SKL ONLY
  *
  * Copyright (C), 2016-2025, junma198902@gmail.com
  ******************************************************************************
  */ 

/*---------------------------------- Includes -------------___----------------*/
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include "http_server.h"
#include "http_get.h"
#include "error.h"
/*-------------------------------- Private macro -----------------------------*/
#define CHECKSTATUS    0
#define VPN_START      1
#define VPN_STOP       2
#define VPN_RESTART    3
/*------------------------------ Private variables ---------------------------*/
static char apszurl[][128] = {"/cgi-bin/admin/vpn_check_status.cgi",
                          "/cgi-bin/admin/vpn_start.cgi",
                          "/cgi-bin/admin/vpn_stop.cgi",
                          "/cgi-bin/admin/vpn_restart.cgi",
                         };

/*--------------------------- Private function prototypes --------------------*/
static void returnvpnstatus(int fd)
{
    printf("vpn is running\n");
}
static void wakeupvpn()
{

}

void forbidden(int client_sockfd) {
    // 403 Error
    char buffer[8096];
    char body[8096];
    sprintf(buffer, "HTTP/1.1 403 Forbidden\r\n");
    write_socket(client_sockfd, buffer, strlen(buffer));
    sprintf(buffer, SERVER_STRING);
    write_socket(client_sockfd, buffer, strlen(buffer));
    sprintf(buffer, "Content-Type: text/html\r\n");
    write_socket(client_sockfd, buffer, strlen(buffer));

    sprintf(body, "<HTML><HEAD><TITLE>Forbidden</TITLE></HEAD>\r\n");
    sprintf(body + strlen(body), "<BODY><P>The server understood the request, but is refusing to fulfill it.</P></BODY></HTML>\r\n");

    sprintf(buffer, "Content-Length: %ld\r\n", strlen(body));
    write_socket(client_sockfd, buffer, strlen(buffer));
    write_socket(client_sockfd, "\r\n", strlen("\r\n"));

    write_socket(client_sockfd, body, strlen(body));
}


int is_valid_fname(char *fname) {
    char *it = fname;
    while(TRUE) {
        if (strncmp(it, "..", 2) == 0) {
            return FALSE;
        }
        it = strchr(it, '/');
        if (it == NULL) break;
        it++;
    }
    return TRUE;
}

static void not_modified(int client_sockfd) {
    // 304
    char buffer[8096];
//    char body[8096];
    sprintf(buffer, "HTTP/1.1 304 Not Modified\r\n");
    write_socket(client_sockfd, buffer, strlen(buffer));
    sprintf(buffer, SERVER_STRING);
    write_socket(client_sockfd, buffer, strlen(buffer));

    time_t raw_time;
    struct tm *current_time;
    time(&raw_time);
    current_time = localtime(&raw_time);
    char date_buf[512];
    strftime(date_buf, 512, "Date: %a, %d %b %Y %T %Z", current_time);

    write_socket(client_sockfd, date_buf, strlen(date_buf));

    // Body isn't sent for this type of error
    write_socket(client_sockfd, "\r\n", strlen("\r\n"));
}

static void ok(int client_sockfd,char *body)
{
    // 200 OK
    char buffer[8096];
    sprintf(buffer, "HTTP/1.1 200 OK\r\n");
    write_socket(client_sockfd, buffer, strlen(buffer));
    sprintf(buffer, SERVER_STRING);
    write_socket(client_sockfd, buffer, strlen(buffer));
     /*
     * Content-Type is very import, if you set the type to text/plain rather than
     * text/html, or the broswer will show the soucer html
     * jpg : image/jpeg
     * */
    sprintf(buffer, "Content-Type: text/html\r\n");
    write_socket(client_sockfd, buffer, strlen(buffer));
    sprintf(buffer, "Content-Length: %ld\r\n", strlen(body));
    write_socket(client_sockfd, buffer, strlen(buffer));
    write_socket(client_sockfd, "\r\n", strlen("\r\n"));

    write_socket(client_sockfd, body, strlen(body));
    free(body);
}


static void dealwithcgi(int cgicmdindex,int fd)
{
    ASSERT(INVALID != cgicmdindex);
    switch (cgicmdindex)
    {
        case CHECKSTATUS:
            returnvpnstatus(fd);
            break;
        case VPN_START :
 //           openvpn_start();
            break;
        case VPN_STOP:
//            openvpn_stop();
            break;
        case VPN_RESTART:
//            openvpn_restart_client();
            break;
        default:
            forbidden(fd);
            break;
    }

}

/*------------------------------- Private functions --------------------------*/


int http_get(st_http_session *pst_session)
{

    int ret;
    int cmdindex;
    int urlcnt ;
    St_header_info st_http_req_header_info;

    ASSERT(pst_session);
    memset(&st_http_req_header_info, 0, sizeof(st_http_req_header_info));
    read_headers(pst_session->fd,&st_http_req_header_info);

    urlcnt = sizeof(apszurl)/sizeof(apszurl[0]);
    cmdindex = checkurl(pst_session->szurl, apszurl, urlcnt);
    printf("%d\n",cmdindex);
    if(INVALID == ret)
    {
        not_found(pst_session->fd);
        keep_alive = FALSE;
        return 0;
    }

    if (header_err_flag) {
        keep_alive = FALSE;
        bad_request(pst_session->fd);
        return -1;
    }

    if (content_length > 0) {
        content = (char*) malloc(content_length + 1);
        read_socket(pst_session->fd, content, content_length);
    }

    // fprintf(stderr, "Content-Length: %d\n", content_length);
    // fprintf(stderr, "Connection (keep_alive): %d\n", keep_alive);
    // fprintf(stderr, "Cookie: %d\n", cookie);
    // fprintf(stderr, "If-Modified-Since Valid Time: %d\n", time_is_valid);
    // fprintf(stderr, "If-Modified-Since Time: %p\n", if_modified_since);
    if (content != NULL) {
        // fprintf(stderr, "Content: %s\n", content);
    }

    /***********************************************************/
    /*       Full message has been read, respond to client     */
    /***********************************************************/


    if (cookie) {
        // Inform client we don't support cookies
        not_implemented(0);
        return 0;
    }

    if (not_eng) {
        // Inform client we only support English
        msg(M_INFO,"sorry,we only support English");
        not_implemented(1);
        return 0;
    }

    if (!acceptable_text) {
        // Inform client we only support plain text
        not_implemented(2);
        return 0;
    }

    if (!acceptable_charset) {
        // Inform client we only support ASCII
        not_implemented(3);
        return 0;
    }

    // Fix filename
    char file_path[512];
    sprintf(file_path, "doc%s", pst_session->szurl);
    if (file_path[strlen(file_path)-1] == '/') {
        file_path[strlen(file_path)-1] = '\0';
    }

    // fprintf(stderr, "%s\n", file_path);

    int fname_valid = is_valid_fname(file_path);

    struct stat file_info;

    if (!fname_valid) {
        // invalid filename
        fprintf(stderr, "403 Forbidden: Invalid file name\n");
        forbidden(pst_session->fd);
        return 0;
    }

    if (stat(file_path, &file_info)) {
        msg(M_ERRNO_SOCK, "404 Not Found %s: Stat failed\n",file_path);
        // Stat failed
        not_found(pst_session->fd);
        return 1;
    }

    if (!S_ISREG(file_info.st_mode)) {
        // Not a file
        forbidden(pst_session->fd);
        fprintf(stderr, "403 Forbidden: Not a regular file\n");
        return 0;
    }


    if (!(file_info.st_mode & S_IRUSR)) {
        // No read permissions
        forbidden(pst_session->fd);
        fprintf(stderr, "403 Forbidden: No read permissions\n");
        return 0;
    }

    FILE *f = fopen(file_path, "r");
    if (f == NULL) {
        // No file
        not_found(pst_session->fd);
        fprintf(stderr, "404 Not Found: Unable to open file\n");
        return 0;
    }

    if (if_modified_since != NULL) {
        struct tm *last_modified = gmtime(&file_info.st_mtime);

        time_t last = mktime(last_modified);
        time_t since = mktime(if_modified_since);

        double diff = difftime(last, since);
        if (diff <= 0) {
            fprintf(stderr, "304 Not Modified\n");
            not_modified(pst_session->fd);
            return 0;
        }
    }

    fprintf(stderr, "All looks good, serving up content in %s\n", file_path);

    char *file_contents = NULL;
    int contents_length = 0;
    char line[512];

    while (fgets(line, sizeof(line), f) != NULL)
    {
        if (file_contents != NULL) {
            char *new_contents = (char*) malloc(contents_length + strlen(line) + 1);
            strcpy(new_contents, file_contents);
            strcpy(new_contents + strlen(new_contents), line);
            contents_length += strlen(line);

            free(file_contents);
            file_contents = new_contents;
        }
        else
        {
            file_contents = (char*) malloc(strlen(line) + 1);
            strcpy(file_contents, line);
            contents_length += strlen(line);
        }
    }
    fclose(f);

    fprintf(stderr, "File Contents:\n");

    fprintf(stderr, "%s\n", file_contents);

    ok(pst_session->fd, file_contents);

    return 0;
}
