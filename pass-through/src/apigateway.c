#include "apigateway.h"

void apigateway_init_string(struct string *s) {
  s->len = 0;
  s->ptr = malloc(s->len+1);
  if (s->ptr == NULL) {
    syslog(LOG_ERR, "apigateway init_string malloc() failed");
    exit(EXIT_FAILURE);
  }
  s->ptr[0] = '\0';
}

size_t apigateway_writefunc(void *ptr, size_t size, size_t nmemb, struct string *s) {
  size_t new_len = s->len + size * nmemb;
  s->ptr = realloc(s->ptr, new_len+1);
  if (s->ptr == NULL) {
    syslog(LOG_ERR, "apigateway init_string realloc() failed");
    exit(EXIT_FAILURE);
  }
  memcpy(s->ptr+s->len, ptr, size*nmemb);
  s->ptr[new_len] = '\0';
  s->len = new_len;
  return size*nmemb;
}

const char* apigateway_get(const char *resource) {
  char url[255];
  memset(url, '\0', sizeof(url));
  strcpy(url, APIGATEWAY_ENDPOINT);
  strcat(url, resource);
  CURL *curl;
  CURLcode res;
  struct string s;
  curl = curl_easy_init();
  if(curl) {
    apigateway_init_string(&s);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, apigateway_writefunc);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);
    res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
  }
  return s.ptr;
}

int apigateway_post(const char *resource, const char *postData) {
  char url[255];
  memset(url, '\0', sizeof(url));
  strcpy(url, APIGATEWAY_ENDPOINT);
  strcat(url, resource);
  CURL *curl;
  CURLcode res;
  //char postData[] = "username=newuser&password=newpasswd&msg=test&msisdn=9999999999&tagname=Demo&shortcode=8888&telcoId=5&dnRequired=false";
  curl = curl_easy_init();
  if(curl) {
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postData);
    res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
  }
  return 0;
}
