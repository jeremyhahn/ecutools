#include "wcbridge.h"

int main_exit(int exit_status) {
  syslog(LOG_DEBUG, "exiting ecutune\n");
  closelog();
  return exit_status;
}

void signal_handler(int sig) {
  switch(sig) {
	case SIGHUP:
	  syslog(LOG_DEBUG, "Received SIGHUP signal\n");
	  // Reload config and reopen files
	  break;
	case SIGINT:
	case SIGTERM:
	  syslog(LOG_DEBUG, "SIGINT/SIGTERM");
	  wcbridge_close(bridge, "SIGINT/SIGTERM");
	  main_exit(EXIT_SUCCESS);
	  exit(0);
	default:
	  syslog(LOG_WARNING, "Unhandled signal %s\n", strsignal(sig));
	  break;
  }
}

void print_program_header() {

  fprintf(stdout, "****************************************************************************\n");
  fprintf(stdout, "* ______________________  _____________  ______   __________________       *\n");
  fprintf(stdout, "* ___  ____/_  ____/_  / / /__  __/_  / / /__  | / /__  ____/__  __ \\      *\n");
  fprintf(stdout, "* __  __/  _  /    _  / / /__  /  _  / / /__   |/ /__  __/  __  / / /      *\n");
  fprintf(stdout, "* _  /___  / /___  / /_/ / _  /   / /_/ / _  /|  / _  /___  _  /_/ /       *\n");
  fprintf(stdout, "* /_____/  \\____/  \\____/  /_/    \\____/  /_/ |_/  /_____/  /_____/        *\n");
  fprintf(stdout, "*                                                                          *\n");
  fprintf(stdout, "*                                                                          *\n");
  fprintf(stdout, "* Open Source ECU Tuning and Diagnostics                                   *\n");
  fprintf(stdout, "* Unified Diagnostic Services / OBD-II / CAN                               *\n");
  fprintf(stdout, "* Copyright (c) 2014 Jeremy Hahn                                           *\n");
  fprintf(stdout, "*                                                                          *\n");
  fprintf(stdout, "* ecutools is free software: you can redistribute it and/or modify         *\n");
  fprintf(stdout, "* it under the terms of the GNU General Public License as published        *\n");
  fprintf(stdout, "* by the Free Software Foundation, either version 3 of the License, or     *\n");
  fprintf(stdout, "* (at your option) any later version.                                      *\n");
  fprintf(stdout, "*                                                                          *\n");
  fprintf(stdout, "* ecutools is distributed in the hope that it will be useful,              *\n");
  fprintf(stdout, "* but WITHOUT ANY WARRANTY; without even the implied warranty of           *\n");
  fprintf(stdout, "* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *\n");
  fprintf(stdout, "* GNU General Public License for more details.                             *\n");
  fprintf(stdout, "*                                                                          *\n");
  fprintf(stdout, "* You should have received a copy of the GNU General Public License        *\n");
  fprintf(stdout, "* along with ecutools.  If not, see <http://www.gnu.org/licenses/>.        *\n");
  fprintf(stdout, "****************************************************************************\n");
  fprintf(stdout, "\n");
}

void create_mock_metrics(char *metrics) {

  int min = 1;
  int max = 100;
  int max2 = 8000;
  char metricbuf[255];

  sprintf(metricbuf, "rpm %i,itt %i,mrp %i,be %i,pwd %i,toa %i,cf %i,afc1 %i,afl1 %i,ma %i,eld %i,fkc %i,flkc %i,iam %i",
	rand()%(max2-min + 1) + min, rand()%(max-min + 1) + min, rand()%(max-min + 1) + min, rand()%(max-min + 1) + min,
	rand()%(max-min + 1) + min, rand()%(max-min + 1) + min, rand()%(max-min + 1) + min, rand()%(max-min + 1) + min,
	rand()%(max-min + 1) + min, rand()%(max-min + 1) + min, rand()%(max-min + 1) + min, rand()%(max-min + 1) + min,
	rand()%(max-min + 1) + min, rand()%(max-min + 1) + min);

  memcpy(metrics, metricbuf, 255);
}

void run_send_metrics(cwebsocket_client *websocket) {

  uint64_t messages_sent = 0;
  char metrics[255];

  time_t start_time, finish_time;
  start_time = time(0);

  do {
	create_mock_metrics(metrics);
	//printf("Metrics: %s\n", metrics);
	cwebsocket_write_data(websocket, metrics, strlen(metrics));
	sleep(1);
	messages_sent++;
  }
  while((websocket->state & WEBSOCKET_STATE_OPEN) != 0);

  finish_time = time(0);
  printf("run_send_metrics: sent %lld messages in %i seconds\n", (long long)messages_sent, (int) (finish_time-start_time));
}

void ecutune_onmessage(wcbridge *bridge, struct can_frame *frame) {
  //char sframe[50];
  //canbus_framecpy(frame, sframe);
  //syslog(LOG_DEBUG, "wcbridge_bridge_onmessage: %s", sframe);
  syslog(LOG_DEBUG, "ecutune_onmessage fired\n");
}

void ecutune_bridge_filter1(wcbridge *bridge, struct can_frame *frame) {
  //bridge->canbus_thread->canbus_print_frame(frame);
}

int main(int argc, char **argv) {

  print_program_header();

  struct sigaction newSigAction;
  sigset_t newSigSet;

  // Set signal mask - signals to block
  sigemptyset(&newSigSet);
  sigaddset(&newSigSet, SIGCHLD);  			/* ignore child - i.e. we don't need to wait for it */
  sigaddset(&newSigSet, SIGTSTP);  			/* ignore Tty stop signals */
  sigaddset(&newSigSet, SIGTTOU);  			/* ignore Tty background writes */
  sigaddset(&newSigSet, SIGTTIN);  			/* ignore Tty background reads */
  sigprocmask(SIG_BLOCK, &newSigSet, NULL);   /* Block the above specified signals */

  // Set up a signal handler
  newSigAction.sa_handler = signal_handler;
  sigemptyset(&newSigAction.sa_mask);
  newSigAction.sa_flags = 0;

  sigaction(SIGHUP, &newSigAction, NULL);     /* catch hangup signal */
  sigaction(SIGTERM, &newSigAction, NULL);    /* catch term signal */
  sigaction(SIGINT, &newSigAction, NULL);     /* catch interrupt signal */

  setlogmask(LOG_UPTO(LOG_DEBUG)); // LOG_INFO, LOG_DEBUG
  openlog("ecutune", LOG_CONS | LOG_PERROR, LOG_USER);
  syslog(LOG_DEBUG, "starting ecutune\n");

  bridge = wcbridge_new();
  bridge->onmessage = &ecutune_onmessage;
  //bridge->bridge_filters[0] = &bridge_filter1;
  wcbridge_run(bridge);
  wcbridge_close(bridge, "main: run loop complete\n");

  return main_exit(EXIT_SUCCESS);
}
