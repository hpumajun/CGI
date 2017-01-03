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
typedef struct session st_http_session;
struct session {
    int fd ;
    char *pszmethod;
    char *pszurl;
    char *pszversion;
};

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
void not_found();
void Unauthorized_response(int fd);
void not_implemented(int reason);
void bad_request(int client_sockfd);
int read_socket(int fd, char *buffer, int size) ;
int write_socket(int fd, char *msg, int size) ;
int read_line(int fd, char *buffer, int size);
void read_headers();
int http_server(void);
#endif /* HTTP_SERVER_H_ */
