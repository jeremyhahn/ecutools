#include "wcbridge.h"

void wcbridge_websocket_onopen(cwebsocket_client *websocket) {
	syslog(LOG_DEBUG, "wcbridge_onopen: websocket file descriptor: %i\n", websocket->socket);
}

void wcbridge_websocket_onmessage(cwebsocket_client *websocket, cwebsocket_message *message) {

	while((bridge->canbus->state & CANBUS_STATE_CONNECTED) == 0) {
		syslog(LOG_DEBUG, "wcbridge_websocket_onmessage: waiting for canbus to connect\n");
		sleep(1);
	}

	if(message == NULL || message->payload == NULL || message->payload_len <= 0) return;

	if(strcmp(message->payload, "log") == 0) {
		if(pthread_create(&bridge->canbus_thread, NULL, wcbridge_canbus_logger_thread, (void *)bridge) == -1) {
			syslog(LOG_ERR, "wcbridge_websocket_onmessage: %s", strerror(errno));
			return;
		}
		syslog(LOG_DEBUG, "thread created\n");
		return;
	}
	else if(strcmp(message->payload, "nolog") == 0) {
		if(pthread_cancel(bridge->canbus_thread) == -1) {
			syslog(LOG_ERR, "wcbridge_websocket_onmessage: %s", strerror(errno));
			return;
		}
		return;
	}
	else if(strcmp(message->payload, "filter") == 0) {

		struct can_filter filter[1];
		filter[0].can_id = 0x411;
		filter[0].can_mask = 0x000007FF;
		if(setsockopt(bridge->canbus->socket, SOL_CAN_RAW, CAN_RAW_FILTER, &filter, sizeof(filter)) == -1) {
			syslog(LOG_ERR, "wcbridge_websocket_onmessage: %s", strerror(errno));
			return;
		}
		syslog(LOG_DEBUG, "---------------->filter hit");
		return;
	}
	else if(strcmp(message->payload, "nofilter") == 0) {

		/*
		struct can_filter filter[0];
		if(setsockopt(bridge->canbus->socket, SOL_CAN_RAW, CAN_RAW_FILTER, &filter, sizeof(filter)) == -1) {
			syslog(LOG_ERR, "wcbridge_websocket_onmessage: %s", strerror(errno));
			return;
		}
		return;
		*/
		pthread_cancel(bridge->canbus_thread);
		pthread_create(&bridge->canbus_thread, NULL, wcbridge_canbus_connect_thread, (void *)bridge);
		syslog(LOG_DEBUG, "---------------->nofilter hit");
		return;
	}
	else {

		syslog(LOG_DEBUG, "payload: %s\n", message->payload);

		char *can_id = strsep(&message->payload, "#");
		char *can_message = strsep(&message->payload, "#");

		struct can_frame *frame = malloc(sizeof(struct can_frame));
		memset(frame, 0, sizeof(struct can_frame));
		frame->can_id = strtol(can_id, NULL, 16);
		frame->can_dlc = 8;

		size_t count = 0;
		for(count = 0; count < sizeof(frame->data)/sizeof(frame->data[0]); count++) {
			sscanf(can_message, "%2hhx", &frame->data[count]);
			can_message += 2 * sizeof(char);
		}

		int i;
		for(i=0; i<sizeof(bridge->filters); i++) {
			wcbridge_process_filter(bridge, frame);
		}

		if(canbus_write(bridge->canbus, frame) == -1) {
			syslog(LOG_ERR, "wcbridge_websocket_onmessage: unable to forward frame: %s", strerror(errno));
		}

		if(bridge->onmessage != NULL) {
			bridge->onmessage(bridge, frame);
		}

		free(frame);
	}
}

void wcbridge_process_filter(wcbridge *bridge, struct can_frame *frame) {
	syslog(LOG_DEBUG, "wcbridge_process_filter: ");
	canbus_print_frame(frame);
}

void wcbridge_websocket_onclose(cwebsocket_client *websocket, const char *message) {
	if(message != NULL) {
		syslog(LOG_DEBUG, "wcbridge_websocket_onclose: %s", message);
	}
}

void wcbridge_websocket_onerror(cwebsocket_client *websocket, const char *message) {
	syslog(LOG_DEBUG, "wcbridge_websocket_onerror: message=%s\n", message);
}

void *wcbridge_websocket_thread(void *ptr) {

	syslog(LOG_DEBUG, "wcbridge_websocket_thread: running\n");

	wcbridge *args = (wcbridge *)ptr;

	args->websocket->onopen = &wcbridge_websocket_onopen;
	args->websocket->onmessage = &wcbridge_websocket_onmessage;
	args->websocket->onclose = &wcbridge_websocket_onclose;
	args->websocket->onerror = &wcbridge_websocket_onerror;

	cwebsocket_init();
	//args->websocket->flags |= WEBSOCKET_FLAG_AUTORECONNECT;
	//args->websocket->retry = 5;
	args->websocket->uri = (char *)WCBRIDGE_WEBSOCKET_ENDPOINT;
	if(cwebsocket_connect(args->websocket) == -1) {
		syslog(LOG_ERR, "wcbridge_websocket_thread: unable to connect to websocket server\n");
	}

	cwebsocket_listen(args->websocket);

	syslog(LOG_DEBUG, "wcbridge_websocket_thread: stopping\n");

	return NULL;
}

void *wcbridge_canbus_connect_thread(void *ptr) {

	if(canbus_connect(bridge->canbus) != 0) {
		syslog(LOG_CRIT, "wcbridge_run: unable to connect to CAN\n");
		return NULL;
	}

	return NULL;
}

void *wcbridge_canbus_logger_thread(void *ptr) {

	syslog(LOG_DEBUG, "wcbridge_canbus_thread: running\n");

	wcbridge *bridge = (wcbridge *)ptr;

	int can_frame_len = sizeof(struct can_frame);
	struct can_frame frame;
	memset(&frame, 0, can_frame_len);

	int data_len = can_frame_len + 25;
	char data[data_len];
	memset(data, 0, data_len);

	/*
	while((bridge->websocket->state & WEBSOCKET_STATE_OPEN) == 0) {
		syslog(LOG_DEBUG, "wcbridge_canbus_listen_thread: waiting for websocket to connect\n");
		sleep(1);
	}*/

	struct can_frame lastframe1, lastframe2, lastframe3, lastframe4, lastframe5;
	memset(&lastframe1, 0, sizeof(lastframe1));
	memset(&lastframe2, 0, sizeof(lastframe2));
	memset(&lastframe3, 0, sizeof(lastframe3));
	memset(&lastframe4, 0, sizeof(lastframe4));
	memset(&lastframe5, 0, sizeof(lastframe5));

	while((bridge->canbus->state & CANBUS_STATE_CONNECTED) &&
			(bridge->websocket->state & WEBSOCKET_STATE_OPEN) &&
			canbus_read(bridge->canbus, &frame) > 0) {

		// Discard the frame if it's been broadcast in the last 5 frames -- reduce noise
		if(canbus_framecmp(&frame, &lastframe1) == 0) {
			syslog(LOG_DEBUG, "wcbridge_canbus_thread: discarding noisy frame\n");
			continue;
		}
		if(canbus_framecmp(&frame, &lastframe2) == 0) {
			syslog(LOG_DEBUG, "wcbridge_canbus_thread: discarding noisy frame\n");
			continue;
		}
		if(canbus_framecmp(&frame, &lastframe3) == 0) {
			syslog(LOG_DEBUG, "wcbridge_canbus_thread: discarding noisy frame\n");
			continue;
		}
		if(canbus_framecmp(&frame, &lastframe4) == 0) {
			syslog(LOG_DEBUG, "wcbridge_canbus_thread: discarding noisy frame\n");
			continue;
		}
		if(canbus_framecmp(&frame, &lastframe5) == 0) {
			syslog(LOG_DEBUG, "wcbridge_canbus_thread: discarding noisy frame\n");
			continue;
		}

		memcpy(&lastframe5, &lastframe4, sizeof(lastframe4));
		memcpy(&lastframe4, &lastframe3, sizeof(lastframe3));
		memcpy(&lastframe3, &lastframe2, sizeof(lastframe2));
		memcpy(&lastframe2, &lastframe1, sizeof(lastframe1));
		memcpy(&lastframe1, &frame, sizeof(frame));

		memset(data, 0, data_len);
		canbus_framecpy(&frame, data);

		if(cwebsocket_write_data(bridge->websocket, data, strlen(data)) == -1) {
			syslog(LOG_ERR, "wcbridge_canbus_thread: unable to forward CAN frame to websocket");
		}
	}

	syslog(LOG_DEBUG, "wcbridge_canbus_thread: stopping\n");

	return NULL;
}

wcbridge *wcbridge_new() {
	wcbridge *bridge = malloc(sizeof(wcbridge));
	memset(bridge, 0, sizeof(wcbridge));
	bridge->websocket = malloc(sizeof(cwebsocket_client));
	memset(bridge->websocket, 0, sizeof(cwebsocket_client));
	bridge->canbus = malloc(sizeof(canbus_client));
	memset(bridge->canbus, 0, sizeof(canbus_client));
	return bridge;
}

int wcbridge_run(wcbridge *bridge) {

	//pthread_attr_init(&bridge->websocket_thread_attr);
	//pthread_attr_setstacksize(&bridge->websocket_thread_attr, STACK_SIZE_MIN);
	pthread_create(&bridge->websocket_thread, NULL, wcbridge_websocket_thread, (void *)bridge);
	pthread_create(&bridge->canbus_thread, NULL, wcbridge_canbus_connect_thread, (void *)bridge);
	pthread_join(bridge->websocket_thread, NULL);

	syslog(LOG_DEBUG, "wcbridge_run: bridge closed\n");

	return 0;
}

void wcbridge_close(wcbridge *bridge, const char *message) {
	syslog(LOG_DEBUG, "wcbridge_close: closing bridge\n");
	canbus_close(bridge->canbus);
	cwebsocket_close(bridge->websocket, message);
	wcbridge_destroy(bridge);
	syslog(LOG_DEBUG, "wcbridge_close: bridge closed\n");
}

void wcbridge_destroy(wcbridge *bridge) {
	free(bridge->websocket);
	free(bridge->canbus);
	free(bridge);
}
