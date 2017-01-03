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


/*------------------------------- Private functions --------------------------*/
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

static void read_post_headers(int fd) {
    int header_end = TRUE;
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

        msg(M_INFO, "**%s**", header);

        if (strcmp(header, "\n") == 0) {
            if (header_end)
            {
            // Empty line signals and flag of header_end is TRUE means end of HTTP Headers
                return;
            }
            else
            {
               header_end = TRUE;
               continue;
            }
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


        if (strncasecmp(header, "Authorization", header_type_len) == 0)
        {
            msg(M_INFO,"%s",header_value_start);
            szencryptmethod = strchr(header_value_start, ' ');
            Authorization = TRUE;
        }
        else if (strncasecmp(header, "Content-Length", header_type_len) == 0)
        {
            content_length = atoi(header_value_start);
        }

        else if (strncasecmp(header, "content", header_type_len) == 0)
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
        else if (strncasecmp(header, "From", header_type_len) == 0)
        {
            strcpy(from_email, header_value_start);
        }
        else if (strncasecmp(header, "User-Agent", header_type_len) == 0)
        {
            strcpy(user_agent, header_value_start);
        }
        else if (strncasecmp(header, "Content-Type",header_type_len) == 0)
        {
            strcpy(aszcontent,header_value_start);
            pszcontent_type = strstr(aszcontent,"multipart");
            if (pszcontent_type)
            {
                header_end = FALSE;
            }

        }
    }
}


int http_post_request(st_http_session *pst_session)
{
    int pf;
    ASSERT(pst_session->fd > 0);

    msg(M_INFO,"this is post request");
    read_post_headers(pst_session->fd);
    printf("adfasdf\n");
    if (!Authorization)
    {
        msg(M_INFO,"unauthorized");
        Unauthorized_response(pst_session->fd);
        keep_alive = FALSE;
        return -1;
    }

//    if (header_err_flag) {
//        keep_alive = FALSE;
//        bad_request(pst_session->fd);
//        return -1;
//    }
    if (content_length > 0) {
        msg(M_INFO,"content length is %d",content_length);
        content = (char*) malloc(content_length + 1);
        int bytes = read_socket(pst_session->fd, content, content_length);
        msg(M_INFO,"content is %s, bytes is %d",content, bytes);
    }
    printf("adfasdf\n");
    ok(pst_session->fd);

    return 0;
}
