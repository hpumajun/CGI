/*
 ============================================================================
 Name        : cgi.c
 Author      : junma
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include "http_server.h"
int main(int argc,char *argv[])
{
    error_reset();
    http_server();
}
