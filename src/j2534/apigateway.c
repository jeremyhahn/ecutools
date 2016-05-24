/**
 * ecutools: IoT Automotive Tuning, Diagnostics & Analytics
 * Copyright (C) 2014 Jeremy Hahn
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

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
  //char postData[] = "key1=val1&key2=val2";
  curl = curl_easy_init();
  if(curl) {
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postData);
    res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
  }
  return 0;
}
