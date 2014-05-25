/**
 *  The MIT License (MIT)
 *
 *  Copyright (c) 2014 Jeremy Hahn
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in
 *  all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 *  THE SOFTWARE.
 */

#ifndef CWEBSOCKET_CLIENT_H
#define CWEBSOCKET_CLIENT_H

#include <time.h>
#include <ctype.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/resource.h>
#include "common.h"

#define WEBSOCKET_FLAG_AUTORECONNECT (1 << 1)

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _cwebsocket {
	int fd;
	int retry;
	char *uri;
	uint8_t flags;
	uint8_t state;
#ifdef ENABLE_SSL
	SSL_CTX *sslctx;
	SSL *ssl;
#endif
#ifdef ENABLE_THREADS
	pthread_t thread;
	pthread_mutex_t lock;
	pthread_mutex_t write_lock;
#endif
	size_t subprotocol_len;
	cwebsocket_subprotocol *subprotocol;
	cwebsocket_subprotocol *subprotocols[];
} cwebsocket_client;

typedef struct {
	cwebsocket_client *socket;
	cwebsocket_message *message;
} cwebsocket_client_thread_args;

// "public"
void cwebsocket_client_init(cwebsocket_client *websocket, cwebsocket_subprotocol *subprotocols[], int subprotocol_len);
int cwebsocket_client_connect(cwebsocket_client *websocket);
int cwebsocket_client_read_data(cwebsocket_client *websocket);
ssize_t cwebsocket_client_write_data(cwebsocket_client *websocket, const char *data, uint64_t len, opcode code);
void cwebsocket_client_run(cwebsocket_client *websocket);
void cwebsocket_client_close(cwebsocket_client *websocket, uint16_t code, const char *reason);
void cwebsocket_client_listen(cwebsocket_client *websocket);

// "private"
void cwebsocket_client_parse_uri(cwebsocket_client *websocket, const char *uri, char *hostname, char *port, char *resource, char *querystring);
int cwebsocket_client_handshake_handler(cwebsocket_client *websocket, const char *handshake_response, char *seckey);
int cwebsocket_client_read_handshake(cwebsocket_client *websocket, char *seckey);
int cwebsocket_client_send_control_frame(cwebsocket_client *websocket, opcode opcode, const char *frame_type, uint8_t *payload, int payload_len);
void cwebsocket_client_create_masking_key(uint8_t *masking_key);
ssize_t inline cwebsocket_client_read(cwebsocket_client *websocket, void *buf, int len);
ssize_t inline cwebsocket_client_write(cwebsocket_client *websocket, void *buf, int len);
void cwebsocket_client_onopen(cwebsocket_client *websocket);
void cwebsocket_client_onmessage(cwebsocket_client *websocket, cwebsocket_message *message);
void cwebsocket_client_onclose(cwebsocket_client *websocket, int code, const char *message);
void cwebsocket_client_onerror(cwebsocket_client *websocket, const char *error);

#ifdef __cplusplus
}
#endif

#endif
