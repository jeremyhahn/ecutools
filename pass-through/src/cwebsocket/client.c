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

#include "client.h"

void cwebsocket_client_init(cwebsocket_client *websocket, cwebsocket_subprotocol *subprotocols[], int subprotocol_len) {
	websocket->fd = 0;
	websocket->retry = 0;
	websocket->uri = '\0';
	websocket->flags = 0;
	websocket->state = WEBSOCKET_STATE_CLOSED;
	websocket->subprotocol_len = subprotocol_len;
	int i;
	for(i=0; i<subprotocol_len; i++) {
		syslog(LOG_DEBUG, "cwebsocket_client_init: loading subprotocol %s", subprotocols[i]->name);
		websocket->subprotocols[i] = subprotocols[i];
	}
	const rlim_t kStackSize = CWS_STACK_SIZE_MIN * 1024 * 1024;
	struct rlimit rl;
	int result;
	result = getrlimit(RLIMIT_STACK, &rl);
	if (result == 0) {
		if (rl.rlim_cur < kStackSize) {
			rl.rlim_cur = kStackSize;
			result = setrlimit(RLIMIT_STACK, &rl);
			if(result != 0) {
			   syslog(LOG_CRIT, "cwebsocket_client_init: unable to set stack space");
			   exit(1);
			}
		}
	}
	getrlimit(RLIMIT_STACK, &rl);
	syslog(LOG_DEBUG, "cwebsocket_client_init: stack limit min=%ld, max=%ld\n", rl.rlim_cur, rl.rlim_max);
}

void cwebsocket_client_parse_uri(cwebsocket_client *websocket, const char *uri,
		char *hostname, char *port, char *resource, char *querystring) {

	if(sscanf(uri, "ws://%[^:]:%[^/]%[^?]%s", hostname, port, resource, querystring) == 4) {
		return;
	}
	else if(sscanf(uri, "ws://%[^:]:%[^/]%s", hostname, port, resource) == 3) {
		strcpy(querystring, "");
		return;
	}
	else if(sscanf(uri, "ws://%[^:]:%[^/]%s", hostname, port, resource) == 2) {
		strcpy(resource, "/");
		strcpy(querystring, "");
		return;
	}
	else if(sscanf(uri, "ws://%[^/]%s", hostname, resource) == 2) {
		strcpy(port, "80");
		strcpy(querystring, "");
		return;
	}
	else if(sscanf(uri, "ws://%[^/]", hostname) == 1) {
		strcpy(port, "80");
		strcpy(resource, "/");
		strcpy(querystring, "");
		return;
	}
#ifdef ENABLE_SSL
	else if(sscanf(uri, "wss://%[^:]:%[^/]%[^?]%s", hostname, port, resource, querystring) == 4) {
		websocket->flags |= WEBSOCKET_FLAG_SSL;
		return;
	}
	else if(sscanf(uri, "wss://%[^:]:%[^/]%s", hostname, port, resource) == 3) {
		strcpy(querystring, "");
		websocket->flags |= WEBSOCKET_FLAG_SSL;
		return;
	}
	else if(sscanf(uri, "wss://%[^:]:%[^/]%s", hostname, port, resource) == 2) {
		strcpy(resource, "/");
		strcpy(querystring, "");
		websocket->flags |= WEBSOCKET_FLAG_SSL;
		return;
	}
	else if(sscanf(uri, "wss://%[^/]%s", hostname, resource) == 2) {
		strcpy(port, "443");
		strcpy(querystring, "");
		websocket->flags |= WEBSOCKET_FLAG_SSL;
		return;
	}
	else if(sscanf(uri, "wss://%[^/]", hostname) == 1) {
		strcpy(port, "443");
		strcpy(resource, "/");
		strcpy(querystring, "");
		websocket->flags |= WEBSOCKET_FLAG_SSL;
		return;
	}
#endif
	else if(strstr(uri, "wss://") > 0) {
		syslog(LOG_CRIT, "cwebsocket_client_parse_uri: recompile with SSL support to use a secure connection");
		exit(1);
	}
	else {
		syslog(LOG_CRIT, "cwebsocket_client_parse_uri: invalid websocket URL\n");
		exit(1);
	}
}

int cwebsocket_client_connect(cwebsocket_client *websocket) {

	if(websocket->state & WEBSOCKET_STATE_CONNECTED) {
		syslog(LOG_CRIT, "cwebsocket_client_connect: socket already connected");
		return -1;
	}

	if(websocket->state & WEBSOCKET_STATE_CONNECTING) {
		syslog(LOG_CRIT, "cwebsocket_client_connect: socket already connecting");
		return -1;
	}

	if(websocket->state & WEBSOCKET_STATE_OPEN) {
		syslog(LOG_CRIT, "cwebsocket_client_connect: socket already open");
		return -1;
	}

#ifdef ENABLE_THREADS
	if(pthread_mutex_init(&websocket->lock, NULL) != 0) {
		syslog(LOG_ERR, "cwebsocket_client_connect: unable to initialize websocket mutex: %s\n", strerror(errno));
		cwebsocket_client_onerror(websocket, strerror(errno));
		return -1;
	}
	if(pthread_mutex_init(&websocket->write_lock, NULL) != 0) {
		syslog(LOG_ERR, "cwebsocket_client_connect: unable to initialize websocket write mutex: %s\n", strerror(errno));
		cwebsocket_client_onerror(websocket, strerror(errno));
		return -1;
	}
	pthread_mutex_lock(&websocket->lock);
	websocket->state = WEBSOCKET_STATE_CONNECTING;
	pthread_mutex_unlock(&websocket->lock);
#else
	websocket->state = WEBSOCKET_STATE_CONNECTING;
#endif

	char hostname[100];
	char port[6];
	char resource[256];
	char querystring[256];
	cwebsocket_client_parse_uri(websocket, websocket->uri, hostname, port, resource, querystring);

	syslog(LOG_DEBUG, "cwebsocket_client_connect: hostname=%s, port=%s, resource=%s, querystring=%s, secure=%i\n",
			hostname, port, resource, querystring, (websocket->flags & WEBSOCKET_FLAG_SSL));

	char handshake[1024];
    struct addrinfo hints, *servinfo;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	srand(time(NULL));
	char nonce[16];
	static const char alphanum[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVXYZabcdefghijklmnopqrstuvwxyz";
	int i;
	for(i = 0; i < 16; i++) {
		nonce[i] = alphanum[rand() % 61];
	}
	char *seckey = cwebsocket_base64_encode((const unsigned char *)nonce, sizeof(nonce));

	snprintf(handshake, 1024,
		      "GET %s%s HTTP/1.1\r\n"
		      "Host: %s:%s\r\n"
		      "Upgrade: WebSocket\r\n"
		      "Connection: Upgrade\r\n"
		      "Sec-WebSocket-Key: %s\r\n"
		      "Sec-WebSocket-Version: 13\r\n"
			  ,resource, querystring, hostname, port, seckey);

	if(websocket->subprotocol_len > 0) {
		strcat(handshake, "Sec-WebSocket-Protocol: ");
		for(i=0; i<websocket->subprotocol_len; i++) {
			strcat(handshake, websocket->subprotocols[i]->name);
			if(i<websocket->subprotocol_len) {
				strcat(handshake, " ");
			}
			else {
				strcat(handshake, "\r\n");
			}
		}
	}

	strcat(handshake, "\r\n");

	if(getaddrinfo(hostname, port, &hints, &servinfo) != 0 ) {
		freeaddrinfo(servinfo);
		const char *errmsg = "invalid hostname or IP";
		syslog(LOG_ERR, "cwebsocket_client_connect: %s", errmsg);
		cwebsocket_client_onerror(websocket, errmsg);
		return -1;
	}

	websocket->fd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
	if(websocket->fd < 0) {
		freeaddrinfo(servinfo);
		syslog(LOG_ERR, "cwebsocket_client_connect: %s", strerror(errno));
		cwebsocket_client_onerror(websocket, strerror(errno));
		return -1;
	}

	if(connect(websocket->fd, servinfo->ai_addr, servinfo->ai_addrlen) != 0 ) {
		syslog(LOG_ERR, "cwebsocket_client_connect: %s", strerror(errno));
		cwebsocket_client_onerror(websocket, strerror(errno));
		websocket->state = WEBSOCKET_STATE_CLOSED;
		if(websocket->retry > 0) {
			sleep(websocket->retry);
			cwebsocket_client_connect(websocket);
		}
		return -1;
	}
	freeaddrinfo(servinfo);

    int optval = 1;
    if(setsockopt(websocket->fd, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof optval) == -1) {
		syslog(LOG_ERR, "cwebsocket_client_connect: %s", strerror(errno));
		cwebsocket_client_onerror(websocket, strerror(errno));
		return -1;
    }

#ifdef ENABLE_SSL

    websocket->ssl = NULL;
    websocket->sslctx = NULL;

	if(websocket->flags & WEBSOCKET_FLAG_SSL) {

	   syslog(LOG_DEBUG, "cwebsocket_client_connect: using secure (SSL) connection");

	   SSL_load_error_strings();
	   SSL_library_init();

	   websocket->sslctx = SSL_CTX_new(SSLv23_client_method());
	   if(websocket->sslctx == NULL) {
		  ERR_print_errors_fp(stderr);
		  return -1;
	   }

	   websocket->ssl = SSL_new(websocket->sslctx);
	   if(websocket->ssl == NULL) {
		  ERR_print_errors_fp(stderr);
		  return -1;
	   }

	   if(!SSL_set_fd(websocket->ssl, websocket->fd)) {
		  ERR_print_errors_fp(stderr);
		  return -1;
	   }

	   if(SSL_connect(websocket->ssl) != 1) {
		  ERR_print_errors_fp(stderr);
		  return -1;
	   }
	}
#endif

#ifdef ENABLE_THREADS
	pthread_mutex_lock(&websocket->lock);
	websocket->state = WEBSOCKET_STATE_CONNECTED;
	pthread_mutex_unlock(&websocket->lock);
#else
	websocket->state = WEBSOCKET_STATE_CONNECTED;
#endif

	if(cwebsocket_client_write(websocket, handshake, strlen(handshake)) == -1) {
		syslog(LOG_ERR, "cwebsocket_client_connect: %s", strerror(errno));
		cwebsocket_client_onerror(websocket, strerror(errno));
		return -1;
	}

	if(cwebsocket_client_read_handshake(websocket, seckey) == -1) {
		syslog(LOG_ERR, "cwebsocket_client_connect: %s", strerror(errno));
		cwebsocket_client_onerror(websocket, strerror(errno));
		return -1;
	}

#ifdef ENABLE_THREADS
	pthread_mutex_lock(&websocket->lock);
	websocket->state = WEBSOCKET_STATE_OPEN;
	pthread_mutex_unlock(&websocket->lock);
#else
	websocket->state = WEBSOCKET_STATE_OPEN;
#endif

	cwebsocket_client_onopen(websocket);

	return 0;
}

int cwebsocket_client_handshake_handler(cwebsocket_client *websocket, const char *handshake_response, char *seckey) {
	uint8_t flags = 0;
	syslog(LOG_DEBUG, "cwebsocket_client_handshake_handler: handshake response: \n%s\n", handshake_response);
	char *ptr = NULL, *token = NULL;
	for(token = strtok((char *)handshake_response, "\r\n"); token != NULL; token = strtok(NULL, "\r\n")) {
		if(*token == 'H' && *(token+1) == 'T' && *(token+2) == 'T' && *(token+3) == 'P') {
			ptr = strchr(token, ' ');
			ptr = strchr(ptr+1, ' ');
			*ptr = '\0';
			if(strcmp(token, "HTTP/1.1 101") != 0 && strcmp(token, "HTTP/1.0 101") != 0) {
				cwebsocket_client_onerror(websocket, "cwebsocket_client_handshake_handler: invalid HTTP status response code");
				return -1;
			}
		} else {
			ptr = strchr(token, ' ');
			if(ptr == NULL) {
				syslog(LOG_ERR, "cwebsocket_client_handshake_handler: invalid HTTP header sent: %s", token);
				cwebsocket_client_onerror(websocket, "invalid HTTP header sent");
				return -1;
			}
			*ptr = '\0';
			if(strcasecmp(token, "Upgrade:") == 0) {
				if(strcasecmp(ptr+1, "websocket") != 0) {
					cwebsocket_client_onerror(websocket, "cwebsocket_client_handshake_handler: invalid HTTP upgrade header");
					return -1;
				}
				flags |= CWS_HANDSHAKE_HAS_UPGRADE;
			}
			if(strcasecmp(token, "Connection:") == 0) {
				if(strcasecmp(ptr+1, "upgrade") != 0) {
					cwebsocket_client_onerror(websocket, "cwebsocket_client_handshake_handler: invalid HTTP connection header");
					return -1;
				}
				flags |= CWS_HANDSHAKE_HAS_CONNECTION;
			}
			if(strcasecmp(token, "Sec-WebSocket-Protocol:") == 0) {
				int i;
				for(i=0; i<websocket->subprotocol_len; i++) {
					if(strcasecmp(ptr+1, websocket->subprotocols[i]->name) == 0) {
						websocket->subprotocol = websocket->subprotocols[i];
						syslog(LOG_DEBUG, "cwebsocket_client_handshake_handler: setting subprotocol to %s", websocket->subprotocol->name);
					}
				}
			}
			if(strcasecmp(token, "Sec-WebSocket-Accept:") == 0) {
				char* response = cwebsocket_create_key_challenge_response(seckey);
				if(strcmp(ptr+1, response) != 0) {
					free(seckey);
					if(websocket->subprotocol->onerror != NULL) {
						char errmsg[255];
						sprintf(errmsg, "cwebsocket_client_handshake_handler: Sec-WebSocket-Accept header does not match computed sha1/base64 response. expected=%s, actual=%s", response, ptr+1);
						cwebsocket_client_onerror(websocket, errmsg);
						free(response);
					    return -1;
					}
					return -1;
				}
				free(response);
				free(seckey);
				flags |= CWS_HANDSHAKE_HAS_ACCEPT;
			}
		}
	}
	if(((flags & CWS_HANDSHAKE_HAS_UPGRADE) == 0) || ((flags & CWS_HANDSHAKE_HAS_CONNECTION) == 0) ||
				((flags & CWS_HANDSHAKE_HAS_ACCEPT) == 0)) {
		// TODO send http error code (500?)
		cwebsocket_client_close(websocket, 1002, "invalid websocket HTTP headers");
		return -1;
	}
	syslog(LOG_DEBUG, "cwebsocket_client_handshake_handler: handshake successful");
	return 0;
}

int cwebsocket_client_read_handshake(cwebsocket_client *websocket, char *seckey) {

	int byte, tmplen = 0;
	uint32_t bytes_read = 0;
	uint8_t data[CWS_HANDSHAKE_BUFFER_MAX];
	memset(data, 0, CWS_HANDSHAKE_BUFFER_MAX);

	while(bytes_read <= CWS_HANDSHAKE_BUFFER_MAX) {

		byte = cwebsocket_client_read(websocket, data+bytes_read, 1);

		if(byte == 0) return -1;
		if(byte == -1) {
			syslog(LOG_ERR, "cwebsocket_client_read_handshake: %s", strerror(errno));
			cwebsocket_client_onerror(websocket, strerror(errno));
			return -1;
		}
		if(bytes_read == CWS_HANDSHAKE_BUFFER_MAX) {
			syslog(LOG_ERR, "cwebsocket_client_read_handshake: handshake response too large. CWS_HANDSHAKE_BUFFER_MAX = %i bytes.", CWS_HANDSHAKE_BUFFER_MAX);
			cwebsocket_client_onerror(websocket, "handshake response too large");
			return -1;
		}
		if((data[bytes_read] == '\n' && data[bytes_read-1] == '\r' && data[bytes_read-2] == '\n' && data[bytes_read-3] == '\r')) {
			break;
		}
		bytes_read++;
	}

	tmplen = bytes_read - 3;
	char buf[tmplen+1];
	memcpy(buf, data, tmplen);
	buf[tmplen] = '\0';

	return cwebsocket_client_handshake_handler(websocket, buf, seckey);
}

void cwebsocket_client_listen(cwebsocket_client *websocket) {
	while(websocket->state & WEBSOCKET_STATE_OPEN) {
		syslog(LOG_DEBUG, "cwebsocket_client_listen: calling cwebsocket_client_read_data");
		cwebsocket_client_read_data(websocket);
	}
	syslog(LOG_DEBUG, "cwebsocket_client_listen: shutting down");
}

#ifdef ENABLE_THREADS
void *cwebsocket_client_onmessage_thread(void *ptr) {
	cwebsocket_client_thread_args *args = (cwebsocket_client_thread_args *)ptr;
	cwebsocket_client_onmessage(args->socket, args->message);
	//free(args->message->payload);
	free(args->message);
	free(ptr);
	return NULL;
}
#endif

int cwebsocket_client_send_control_frame(cwebsocket_client *websocket, opcode code, const char *frame_type, uint8_t *payload, int payload_len) {
	if(websocket->fd <= 0) return -1;
	ssize_t bytes_written;
	int header_len = 6;
	int frame_len = header_len + payload_len;
	uint8_t control_frame[frame_len];
	memset(control_frame, 0, frame_len);
	uint8_t masking_key[4];
	cwebsocket_client_create_masking_key(masking_key);
	control_frame[0] = (code | 0x80);
	control_frame[1] = (payload_len | 0x80);
	control_frame[2] = masking_key[0];
	control_frame[3] = masking_key[1];
	control_frame[4] = masking_key[2];
	control_frame[5] = masking_key[3];
	if(code & CLOSE) {
		uint16_t close_code = 1000;
		if(payload_len >= 2) {
		   if(payload_len > 2) {
			  char parsed_payload[payload_len];
			  memcpy(parsed_payload, &payload[0], payload_len);
			  parsed_payload[payload_len] = '\0';
			  close_code = (control_frame[6] << 8) + control_frame[7];
			  int i;
			  for(i=0; i<payload_len; i++) {
				  control_frame[6+i] = (parsed_payload[i] ^ masking_key[i % 4]) & 0xff;
			  }
			  syslog(LOG_DEBUG, "cwebsocket_client_send_control_frame: opcode=%#04x, frame_type=%s, payload_len=%i, code=%i, payload=%s",
					  code, frame_type, payload_len, close_code, parsed_payload);
		   }
		   else {
				syslog(LOG_DEBUG, "cwebsocket_client_send_control_frame: opcode=%#04x, frame_type=%s, payload_len=%i, code=%i, payload=(null)",
						code, frame_type, payload_len, close_code);
		   }
		}
		else {
			syslog(LOG_DEBUG, "cwebsocket_client_send_control_frame: opcode=%#04x, frame_type=%s, payload_len=%i, code=%i, payload=(null)",
					code, frame_type, payload_len, close_code);
		}
	}
	else {
		int i;
		for(i=0; i<payload_len; i++) {
			control_frame[header_len+i] = (payload[i] ^ masking_key[i % 4]) & 0xff;
		}
	}
	bytes_written = cwebsocket_client_write(websocket, control_frame, frame_len);
	if(bytes_written == 0) {
		syslog(LOG_DEBUG, "cwebsocket_client_send_control_frame: remote host closed the connection");
		return 0;
	}
	else if(bytes_written == -1) {
		syslog(LOG_CRIT, "cwebsocket_client_send_control_frame: error sending %s control frame. %s", frame_type, strerror(errno));
		cwebsocket_client_onerror(websocket, strerror(errno));
		return -1;
	}
	else {
		syslog(LOG_DEBUG, "cwebsocket_client_send_control_frame: wrote %zd byte %s frame", bytes_written, frame_type);
	}
	return bytes_written;
}

int cwebsocket_client_read_data(cwebsocket_client *websocket) {

	int header_length = 2, bytes_read = 0;
	const int header_length_offset = 2;
	const int extended_payload16_end_byte = 4;
	const int extended_payload64_end_byte = 10;
	uint64_t payload_length = 0;

	uint8_t *data = malloc(CWS_DATA_BUFFER_MAX);
	if(data == NULL) {
		perror("out of memory");
		exit(-1);
	}
	memset(data, 0, CWS_DATA_BUFFER_MAX);

	cwebsocket_frame frame;
	memset(&frame, 0, sizeof(frame));

	uint64_t frame_size = header_length;
	while(bytes_read < frame_size && (websocket->state & WEBSOCKET_STATE_OPEN)) {

		if(bytes_read >= CWS_DATA_BUFFER_MAX) {
			syslog(LOG_ERR, "cwebsocket_client_read_data: frame too large. RECEIVE_BUFFER_MAX = %i bytes. bytes_read=%i, header_length=%i",
					CWS_DATA_BUFFER_MAX, bytes_read, header_length);
			cwebsocket_client_close(websocket, 1009, "frame too large");
			return -1;
		}

		ssize_t byte = cwebsocket_client_read(websocket, data+bytes_read, 1);

		if(byte == 0) {
		   char *errmsg = "server closed the connection";
		   cwebsocket_client_onerror(websocket, errmsg);
		   cwebsocket_client_close(websocket, 1006, errmsg);
		   return -1;
		}
		if(byte == -1) {
		   syslog(LOG_ERR, "cwebsocket_client_read_data: error reading frame: %s", strerror(errno));
		   cwebsocket_client_onerror(websocket, strerror(errno));
		   return -1;
		}
		bytes_read++;

		if(bytes_read == header_length_offset) {

		   frame.fin = (data[0] & 0x80) == 0x80 ? 1 : 0;
		   frame.rsv1 = (data[0] & 0x40) == 0x40 ? 1 : 0;
		   frame.rsv2 = (data[0] & 0x20) == 0x20 ? 1 : 0;
		   frame.rsv3 = (data[0] & 0x10) == 0x10 ? 1 : 0;
		   frame.opcode = (data[0] & 0x7F);
		   frame.mask = data[1] & 0x80;
		   frame.payload_len = (data[1] & 0x7F);

		   if(frame.mask == 1) {
			   const char *errmsg = "received masked frame from server";
			   syslog(LOG_CRIT, "cwebsocket_client_read_data: %s", errmsg);
			   cwebsocket_client_onerror(websocket, errmsg);
			   return -1;
		   }

		   payload_length = frame.payload_len;
		   frame_size = header_length + payload_length;
		}

		if(frame.payload_len == 126 && bytes_read == extended_payload16_end_byte) {

			header_length += 2;

			uint16_t extended_payload_length = 0;
			extended_payload_length |= ((uint8_t) data[2]) << 8;
			extended_payload_length |= ((uint8_t) data[3]) << 0;

			payload_length = extended_payload_length;
			frame_size = header_length + payload_length;
		}
		else if(frame.payload_len == 127 && bytes_read == extended_payload64_end_byte) {

			header_length += 6;

			uint64_t extended_payload_length = 0;
			extended_payload_length |= ((uint64_t) data[2]) << 56;
			extended_payload_length |= ((uint64_t) data[3]) << 48;
			extended_payload_length |= ((uint64_t) data[4]) << 40;
			extended_payload_length |= ((uint64_t) data[5]) << 32;
			extended_payload_length |= ((uint64_t) data[6]) << 24;
			extended_payload_length |= ((uint64_t) data[7]) << 16;
			extended_payload_length |= ((uint64_t) data[8]) << 8;
			extended_payload_length |= ((uint64_t) data[9]) << 0;

			payload_length = extended_payload_length;
			frame_size = header_length + payload_length;
		}
	}

	if(frame.fin && frame.opcode == TEXT_FRAME) {

		char *payload = malloc(sizeof(char) * payload_length+1);
		if(payload == NULL) {
			perror("out of memory");
			exit(-1);
		}
		memcpy(payload, &data[header_length], payload_length);
		payload[payload_length] = '\0';
		free(data);

		size_t utf8_code_points = 0;
		if(utf8_count_code_points((uint8_t *)payload, &utf8_code_points)) {
			syslog(LOG_ERR, "cwebsocket_client_read_data: received %lld byte malformed utf8 text payload: %s", payload_length, payload);
			cwebsocket_client_onerror(websocket, "received malformed utf8 payload");
			return -1;
		}

		syslog(LOG_DEBUG, "cwebsocket_client_read_data: received %lld byte text payload: %s", payload_length, payload);

		if(websocket->subprotocol != NULL && websocket->subprotocol->onmessage != NULL) {

#ifdef ENABLE_THREADS
			cwebsocket_message *message = malloc(sizeof(cwebsocket_message));
			if(message == NULL) {
				perror("out of memory");
				exit(-1);
			}
			memset(message, 0, sizeof(cwebsocket_message));
			message->opcode = frame.opcode;
			message->payload_len = frame.payload_len;
			message->payload = payload;

		    cwebsocket_client_thread_args *args = malloc(sizeof(cwebsocket_client_thread_args));
		    if(args == NULL) {
				perror("out of memory");
				exit(-1);
			}
		    memset(args, 0, sizeof(cwebsocket_client_thread_args));
		    args->socket = websocket;
		    args->message = message;

		    if(pthread_create(&websocket->thread, NULL, cwebsocket_client_onmessage_thread, (void *)args) == -1) {
		    	syslog(LOG_ERR, "cwebsocket_client_read_data: %s", strerror(errno));
		    	cwebsocket_client_onerror(websocket, strerror(errno));
		    	return -1;
		    }
		    return bytes_read;
#else
		    cwebsocket_message message = {0};
			message.opcode = frame.opcode;
			message.payload_len = frame.payload_len;
			message.payload = payload;
		    cwebsocket_client_onmessage(websocket, &message);
			//free(payload);
		    return bytes_read;
#endif
		}

		syslog(LOG_WARNING, "cwebsocket_client_read_data: onmessage callback undefined");
		return bytes_read;
	}
	else if(frame.fin && frame.opcode == BINARY_FRAME) {

		syslog(LOG_DEBUG, "cwebsocket_client_read_data: received BINARY payload. bytes=%lld", payload_length);

		char *payload = malloc(sizeof(char) * payload_length);
		if(payload == NULL) {
			perror("out of memory");
			exit(-1);
		}
		memcpy(payload, &data[header_length], payload_length);
		free(data);

		if(websocket->subprotocol->onmessage != NULL) {

#ifdef ENABLE_THREADS
			cwebsocket_message *message = malloc(sizeof(cwebsocket_message));
			if(message == NULL) {
				perror("out of memory");
				exit(-1);
			}
			message->opcode = frame.opcode;
			message->payload_len = frame.payload_len;
			message->payload = payload;

			cwebsocket_client_thread_args *args = malloc(sizeof(cwebsocket_client_thread_args));
			args->socket = websocket;
			args->message = message;

			if(pthread_create(&websocket->thread, NULL, cwebsocket_client_onmessage_thread, (void *)args) == -1) {
				syslog(LOG_ERR, "cwebsocket_client_read_data: %s", strerror(errno));
				cwebsocket_client_onerror(websocket, strerror(errno));
				return -1;
			}
			return bytes_read;
#else
			cwebsocket_message message;
			message.opcode = frame.opcode;
			message.payload_len = frame.payload_len;
			message.payload = payload;
			websocket->subprotocol->onmessage(websocket, &message);
			free(payload);
			return bytes_read;
#endif
		}
		syslog(LOG_WARNING, "cwebsocket_client_read_data: onmessage callback undefined");
		return bytes_read;
	}
	else if(frame.opcode == CONTINUATION) {
		syslog(LOG_DEBUG, "cwebsocket_client_read_data: received CONTINUATION opcode");
		return 0;
	}
	else if(frame.opcode == PING) {
		if(frame.fin == 0) {
			cwebsocket_client_close(websocket, 1002, "control message must not be fragmented");
		}
		if(frame.payload_len > 125) {
			cwebsocket_client_close(websocket, 1002, "control frames must not exceed 125 bytes");
			return -1;
		}
		syslog(LOG_DEBUG, "cwebsocket_client_read_data: received PING control frame");
		uint8_t payload[payload_length];
		memcpy(payload, &data[header_length], payload_length);
		payload[payload_length] = '\0';
		free(data);
		return cwebsocket_client_send_control_frame(websocket, 0x0A, "PONG", payload, payload_length);
	}
	else if(frame.opcode == PONG) {
		syslog(LOG_DEBUG, "cwebsocket_client_read_data: received PONG control frame");
		return 0;
	}
	else if(frame.opcode == CLOSE) {
		if(frame.payload_len > 125) {
			cwebsocket_client_close(websocket, 1002, "control frames must not exceed 125 bytes");
			return -1;
		}
		int code = 0;
		if(payload_length > 2) {
		   code = (data[header_length] << 8) + (data[header_length+1]);
		   header_length += 2;
		   payload_length -= 2;
		}
		uint8_t payload[payload_length];
		memcpy(payload, &data[header_length], (payload_length) * sizeof(uint8_t));
		payload[payload_length] = '\0';
		free(data);
		syslog(LOG_DEBUG, "cwebsocket_client_read_data: received CLOSE control frame. payload_length=%lld, code=%i, reason=%s", payload_length, code, payload);
		cwebsocket_client_close(websocket, code, NULL);
		return 0;
	}

	free(data);
	char closemsg[50];
	sprintf(closemsg, "received unsupported opcode: %#04x", frame.opcode);
	syslog(LOG_ERR, "cwebsocket_client_read_data: %s", closemsg);
	cwebsocket_print_frame(&frame);
	cwebsocket_client_onerror(websocket, closemsg);
	cwebsocket_client_close(websocket, 1002, closemsg);
	return -1;
}

void cwebsocket_client_create_masking_key(uint8_t *masking_key) {
	uint8_t mask_bit;
	struct timeval tv;
	gettimeofday(&tv, NULL);
	srand(tv.tv_usec * tv.tv_sec);
	mask_bit = rand();
	memcpy(masking_key, &mask_bit, 4);
}

ssize_t cwebsocket_client_write_data(cwebsocket_client *websocket, const char *data, uint64_t payload_len, opcode code) {

	if((websocket->state & WEBSOCKET_STATE_OPEN) == 0) {
		syslog(LOG_DEBUG, "cwebsocket_client_write_data: websocket closed");
		cwebsocket_client_onerror(websocket, "websocket closed");
		return -1;
	}

	uint32_t header_length = 6 + (payload_len > 125 ? 2 : 0) + (payload_len > 0xffff ? 8 : 0);
	uint8_t masking_key[4];
	uint8_t header[header_length];
	ssize_t bytes_written;

	cwebsocket_client_create_masking_key(masking_key);
	header[0] = (code | 0x80);

	if(payload_len <= 125) {

		header[1] = (payload_len | 0x80);
		header[2] = masking_key[0];
		header[3] = masking_key[1];
		header[4] = masking_key[2];
		header[5] = masking_key[3];
	}
	else if(payload_len > 125 && payload_len <= 0xffff) { // 125 && 65535

		uint16_t len16 = htons(payload_len);
		header[1] = (126 | 0x80);
		memcpy(header+2, &len16, 2);
		header[4] = masking_key[0];
		header[5] = masking_key[1];
		header[6] = masking_key[2];
		header[7] = masking_key[3];
	}
	else if(payload_len > 0xffff && payload_len <= 0xffffffffffffffffLL) {  // 65535 && 18446744073709551615

		char len64[8] = htonl64(payload_len);
		header[1] = (127 | 0x80);
		memcpy(header+2, len64, 8);
		header[10] = masking_key[0];
		header[11] = masking_key[1];
		header[12] = masking_key[2];
		header[13] = masking_key[3];
	}
	else {
		syslog(LOG_CRIT, "cwebsocket_client_write_data: frame too large");
		cwebsocket_client_close(websocket, 1009, "frame too large");
		return -1;
	}

	int frame_length = header_length + payload_len;
	char framebuf[frame_length];
	memset(framebuf, 0, frame_length);
	memcpy(framebuf, header, header_length);
	memcpy(&framebuf[header_length], data, payload_len);

	int i;
	for(i=0; i<payload_len; i++) {
		framebuf[header_length+i] ^= masking_key[i % 4] & 0xff;
	}

	bytes_written = cwebsocket_client_write(websocket, framebuf, frame_length);

	if(bytes_written == -1) {
		syslog(LOG_ERR, "cwebsocket_client_write_data: error: %s", strerror(errno));
		cwebsocket_client_onerror(websocket, strerror(errno));
		return -1;
	}

	syslog(LOG_DEBUG, "cwebsocket_client_write_data: bytes_written=%zu, frame_length=%i, payload_len=%lld, payload=%s\n",
			bytes_written, frame_length, (long long)payload_len, data);

	return bytes_written;
}

void cwebsocket_client_close(cwebsocket_client *websocket, uint16_t code, const char *message) {

	if((websocket->state & WEBSOCKET_STATE_OPEN) == 0 || websocket->fd < 1) {
		return;
	}

#ifdef ENABLE_THREADS
	pthread_mutex_lock(&websocket->lock);
	websocket->state = WEBSOCKET_STATE_CLOSING;
	pthread_mutex_unlock(&websocket->lock);
#else
	websocket->state = WEBSOCKET_STATE_CLOSING;
#endif

	syslog(LOG_DEBUG, "cwebsocket_client_close: code=%i, message=%s\n", code, message);

	int code32 = 0;
	if(code > 0) {
		code = code ? htons(code) : htons(1005);
		int message_len = (message == NULL) ? 0 : strlen(message) + 2;
		uint8_t close_frame[message_len];
		close_frame[0] = code & 0xFF;
		close_frame[1] = (code >> 8);
		code32 = (close_frame[0] << 8) + (close_frame[1]);
		int i;
		for(i=0; i<message_len; i++) {
			close_frame[i+2] = message[i];
		}
		cwebsocket_client_send_control_frame(websocket, CLOSE, "CLOSE", close_frame, message_len);
	}
	else {
		cwebsocket_client_send_control_frame(websocket, CLOSE, "CLOSE", NULL, 0);
	}

#ifdef ENABLE_SSL
	if(websocket->ssl != NULL) {
       SSL_shutdown(websocket->ssl);
	   SSL_free(websocket->ssl);
    }
	if(websocket->sslctx != NULL) {
	    SSL_CTX_free(websocket->sslctx);
	}
#else
	if(shutdown(websocket->fd, SHUT_WR) == -1) {
		syslog(LOG_ERR, "cwebsocket_client_close: unable to shutdown websocket: %s", strerror(errno));
	}
	char buf[1];
	while(read(websocket->fd, buf, 1) > 0) { buf[0] = '\0'; }
	if(close(websocket->fd) == -1) {
		syslog(LOG_ERR, "cwebsocket_client_close: error closing websocket: %s\n", strerror(errno));
		cwebsocket_client_onclose(websocket, 1011, strerror(errno));
	}
	websocket->fd = 0;
#endif

	cwebsocket_client_onclose(websocket, code32, message);

#ifdef ENABLE_THREADS
	pthread_mutex_lock(&websocket->lock);
	websocket->state = WEBSOCKET_STATE_CLOSED;
	pthread_mutex_unlock(&websocket->lock);
	pthread_cancel(websocket->thread);
#else
	websocket->state = WEBSOCKET_STATE_CLOSED;
#endif

	syslog(LOG_DEBUG, "cwebsocket_client_close: websocket closed\n");
	websocket->state = 0;

	if(websocket->flags & WEBSOCKET_FLAG_AUTORECONNECT) {
		cwebsocket_client_connect(websocket);
	}
}

ssize_t inline cwebsocket_client_read(cwebsocket_client *websocket, void *buf, int len) {
#ifdef ENABLE_SSL
	return (websocket->flags & WEBSOCKET_FLAG_SSL) ?
			SSL_read(websocket->ssl, buf, len) :
			read(websocket->fd, buf, len);
#else
	return read(websocket->fd, buf, len);
#endif
}

ssize_t inline cwebsocket_client_write(cwebsocket_client *websocket, void *buf, int len) {
#ifdef ENABLE_THREADS
	ssize_t bytes_written;
	pthread_mutex_lock(&websocket->write_lock);
	#ifdef USESSL
		bytes_written = (websocket->flags & WEBSOCKET_FLAG_SSL) ?
				SSL_write(websocket->ssl, buf, len) :
				write(websocket->fd, buf, len);
	#else
		bytes_written = write(websocket->fd, buf, len);
	#endif
	pthread_mutex_unlock(&websocket->write_lock);
	return bytes_written;
#else
	#ifdef ENABLE_SSL
		return (websocket->flags & WEBSOCKET_FLAG_SSL) ?
				SSL_write(websocket->ssl, buf, len) :
				write(websocket->fd, buf, len);
	#else
		return write(websocket->fd, buf, len);
	#endif
#endif
}

void cwebsocket_client_onopen(cwebsocket_client *websocket) {
	if(websocket->subprotocol != NULL && websocket->subprotocol->onopen != NULL) {
		websocket->subprotocol->onopen(websocket);
	}
}

void cwebsocket_client_onmessage(cwebsocket_client *websocket, cwebsocket_message *message) {
	if(websocket->subprotocol != NULL && websocket->subprotocol->onmessage != NULL) {
		websocket->subprotocol->onmessage(websocket, message);
	}
}

void cwebsocket_client_onclose(cwebsocket_client *websocket, int code, const char *message) {
	if(websocket->subprotocol != NULL && websocket->subprotocol->onclose != NULL) {
		websocket->subprotocol->onclose(websocket, code, message);
	}
}

void cwebsocket_client_onerror(cwebsocket_client *websocket, const char *error) {
	if(websocket->subprotocol != NULL && websocket->subprotocol->onerror != NULL) {
		websocket->subprotocol->onerror(websocket, error);
	}
}
