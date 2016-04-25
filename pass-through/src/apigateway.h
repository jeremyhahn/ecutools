#ifndef APIGATEWAY_H_
#define APIGATEWAY_H_

#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <string.h>
#include <curl/curl.h>
#include <jansson.h>

#define APIGATEWAY_ENDPOINT "https://6ydl4r3wc0.execute-api.us-east-1.amazonaws.com/test"

struct string {
  char *ptr;
  size_t len;
};

const char* apigateway_get(const char *url);
int apigateway_post(const char *url, const char *postData);

#endif
