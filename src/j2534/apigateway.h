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
