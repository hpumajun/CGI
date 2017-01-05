/*********************************************************************************
  *Copyright(C),2010-2011,Your Company
  *FileName:  // base64.c
  *Author:  //junma
  *Version:  //
  *Date:  //Dec 31, 2016 6:22:06 AM
  *Description:
  *Others:
  *Function List:
  *
  *History:
      1.Date:
       Author:
       Modification:
     2.…………
**********************************************************************************/


/*************************************INCLUDE*************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>

/************************************Variable*************************************/
static char base64_chars[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
/***********************************MACRO DEFINE**********************************/
#define DECODE_ERROR 0xffffffff
/********************************Private Function*********************************/
static int pos(char c)
{
    char *p;
    for (p = base64_chars; *p; p++)
    if (*p == c)
        return p - base64_chars;
    return -1;
}

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
/********************************Public Function *********************************/
int base64_decode(const char *str, void *data, int size)
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
/*
int main()
{
    char str[] = "YWRtaW46MTIzNDU=" ;
    char data[256] = { 0 };
    base64_decode(str,data, sizeof(data));
    printf("%s", data);
}
*/
