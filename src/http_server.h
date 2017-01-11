/*
 * http_server.h
 *
 *  Created on: Dec 29, 2016
 *      Author: junma
 */

#ifndef HTTP_SERVER_H_
#define HTTP_SERVER_H_
#include "../../logprint/src/error.h"

#define SERVER_STRING "Server: httpServer/0.1.1\r\n"
#define TRUE 1
#define FALSE 0

/*********HTTP Method***************/
#define GET     0
#define POST    1
#define HEAD    2
#define PUT     3
#define DELETE   4
#define OPTION  5
#define TRACE   6
#define CONNECT 7
#define INVALID 0xFFFFFFFF

/******************struct define***************************/
typedef struct stcgisession st_http_session;
struct stcgisession {
    unsigned char uckeepalive;
    int fd ;
    char szmethod[16];
    char szversion[16];
    char szurl[256];
};

typedef struct st_http_auth {
    unsigned char ucflag;
    char szauth_method[16];
    char szauth_cipher[128];
}Stauthinfo;

typedef struct stbody_header_info{
char szdcontdisp[128];
char szcontype[128];
}Stbody_header;


typedef struct st_http_header_info{
int iheaderlen;
int icontent_length;
char szhost[32];
char szusragt[64];
char szconttype[128];
Stauthinfo stauth;
Stbody_header stbodyheader;
}St_header_info;


typedef struct st_request_header{
char aszheader[64];
char content[128];
}st_req_header;


typedef struct user{
    char user[128];
    char pwd [128];
}CGIUSER;

/************Global variable**********************/
extern int keep_alive ; // Default in HTTP/1.1
extern int content_length ;
extern int cookie ;
extern int header_err_flag ;
extern struct tm *if_modified_since;
extern int time_is_valid ;
extern char *content ;
extern int not_eng ;
extern int acceptable_text ;
extern int acceptable_charset ;
extern int acceptable_encoding ;
extern int Authorization ;
extern char from_email[512];
extern char user_agent[512];

/***************public function*********************/
void not_found(int client_sockfd);
void Unauthorized_response(int fd);
void not_implemented(int reason);
void bad_request(int client_sockfd);
void Authorizedfailed_response(int client_sockfd);
int read_socket(int fd, char *buffer, int size) ;
int write_socket(int fd, char *msg, int size) ;
int read_line(int fd, char *buffer, int size);
void read_headers(int fd, St_header_info *pstreq_header_info);
int http_server(void);
int cgitask();

int check_authorization(char *pszciphertext);
int checkurl(char *pszurl,const char (*aaszurl)[128],int urlcnt);

#endif /* HTTP_SERVER_H_ */
