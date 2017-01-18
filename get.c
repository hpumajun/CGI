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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <assert.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "http_server.h"
#include "http_get.h"
#include "openvpn_client.h"
#include "../../include/dvr.h"
/*------------------------------- Private typedef ----------------------------*/
/*-------------------------------- Private macro -----------------------------*/


#define UNIX_DOMAIN "/tmp/cgi.domain"
/*------------------------------ Private variables ---------------------------*/
static char apszurl[][128] = {"/cgi-bin/admin/vpn_check_status.cgi",
						  "/cgi-bin/admin/vpn_start.cgi",
						  "/cgi-bin/admin/vpn_stop.cgi",
						  "/cgi-bin/admin/vpn_restart.cgi",
						  "/System/deviceInfo",
						 };

extern DEVICECONFIG *pDevCfgParam;
extern DEV_HARD_INFO devHardInfo;
/*--------------------------- Private function prototypes --------------------*/
int opensocket(char *pszpath)
{
    int connect_fd;
    int ret;
    static struct sockaddr_un srv_addr;
//creat unix socket
    connect_fd=socket(PF_UNIX,SOCK_STREAM,0);
    if(connect_fd<0)
    {
        perror("cannot create communication socket");
        return -1;
    }
    srv_addr.sun_family=AF_UNIX;
    strcpy(srv_addr.sun_path,pszpath);
//connect server
    ret=connect(connect_fd,(struct sockaddr*)&srv_addr,sizeof(srv_addr));
    if(ret==-1)
    {
        perror("cannot connect to the server");
        close(connect_fd);
        return -1;
    }
    return connect_fd;

}
void sendcmdtoopenvpn(int msg)
{
    char szpath[] = UNIX_DOMAIN;
    printf("opensockt to send cmd to openvpn\n");
    int fd = opensocket(szpath);
    if (fd == -1)
    {
        printf("open socket failed\n");
        return ;
    }
    char szsendbuf = (char)msg;
    printf("send cmd %d to openvpn\n", msg);
    write(fd,&szsendbuf,1);

//    sleep(2);
    close(fd);
}

static void wakeupvpn()
{
    
}
static void getdeviceinfo(ST_DEV_INFO *pstdevinfo)
{
    memcpy(pstdevinfo->szdevname,pDevCfgParam->deviceName,32);
    pstdevinfo->idevid = pDevCfgParam->deviceId;
    memcpy(pstdevinfo->szserialnum,devHardInfo.devSerialNo,48);
}
static void backtoclient (int client_sockfd,ST_DEV_INFO pstdevinfo)
{
    char buffer[8096];
    char body[8096];
    sprintf(buffer, "HTTP/1.1 200 OK\r\n");
    write_socket(client_sockfd, buffer, strlen(buffer));
    sprintf(buffer, SERVER_STRING);
    write_socket(client_sockfd, buffer, strlen(buffer));
    sprintf(buffer, "Content-Type: text/xml\r\n");
    write_socket(client_sockfd, buffer, strlen(buffer));

    sprintf(body, "<?xml version=\"1.0\" encoding=\"UTF-8\"?> <DeviceInfo version=\"1.0\" \
            xmlns=\"http://www.std-cgi.com/ver10/XMLSchema\"> ");
   
    sprintf(buffer, "Content-Length: %ld\r\n", strlen(body));
    write_socket(client_sockfd, buffer, strlen(buffer));
    write_socket(client_sockfd, "\r\n", strlen("\r\n"));

    write_socket(client_sockfd, body, strlen(body));
    
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
    int ret;
    ST_DEV_INFO stdevinfo;
    memset(&stdevinfo, 0, sizeof(stdevinfo));
  //  argv[][64] = {"ca.crt","ca.key"}
    assert(INVALID != cgicmdindex);
    switch (cgicmdindex)
    {
        case CHECKSTATUS:
        case VPN_START :

        case VPN_STOP:

        case VPN_RESTART:
            sendcmdtoopenvpn(cgicmdindex);
            break;
        case GET_DEVICEINFO:
            getdeviceinfo(&stdevinfo);
            backtoclient(fd,stdevinfo);
            break;
        default:
            forbidden(fd);
            break;
    }
            
}


/*------------------------------- Private functions --------------------------*/


int http_get(st_http_session *pst_session)
{
    int ret,urlcnt;
    int cmdindex;
    St_header_info st_http_req_header_info;
    assert(pst_session);
    memset(&st_http_req_header_info, 0, sizeof(st_http_req_header_info));
    read_headers(pst_session->fd,&st_http_req_header_info);
    urlcnt = sizeof(apszurl)/sizeof(apszurl[0]);
    cmdindex = checkurl(pst_session->szurl,apszurl, urlcnt);
    if(INVALID == ret)
    {
        invalid_url(pst_session->fd);
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
    dealwithcgi(cmdindex,pst_session->fd);
    char line[512] = { 0 };

    ok(pst_session->fd, line);

    return 0;
}
