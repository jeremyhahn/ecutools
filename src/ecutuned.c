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

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <syslog.h>
#include <string.h>
#include "mystring.h"
#include "passthru_thing.h"
#include "j2534.h"

int daemonize = 0;

int main_exit(int exit_status, passthru_thing_params *params) {
  syslog(LOG_DEBUG, "exiting %s", params->thingName);
  closelog();
  if(params->iface != NULL) {
    free(params->iface);
  }
  if(params->logdir != NULL) {
    free(params->logdir);
  }
  free(params->thingName);
  free(params);
  return exit_status;
}

void signal_handler(int sig) {
  switch(sig) {
  case SIGHUP:
    syslog(LOG_DEBUG, "Received SIGHUP signal");
    // Reload config and reopen files
    break;
  case SIGINT:
    passthru_thing_close();
    break;
  case SIGTERM:
    passthru_thing_close();
    break;
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
  fprintf(stdout, "* IoT Automotive Tuning, Diagnostics & Analytics                           *\n");
  fprintf(stdout, "* J2534-1 / CAN / OBD-II / Unified Diagnostic Services                     *\n");
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
  fprintf(stdout, "PassThru Firmware: v%s\n", PASSTHRU_FIRMWARE_VERSION);
  fprintf(stdout, "J2534 DLL:         v%s\n", J2534_DLL_VERSION);
  fprintf(stdout, "J2534 API:         v%s\n", J2534_API_VERSION);
  fprintf(stdout, "AWS IoT SDK:       v%d.%d.%d-%s\n", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, VERSION_TAG);
  fprintf(stdout, "Jansson:           v%s\n", JANSSON_VERSION);
  fprintf(stdout, "\n");
  fprintf(stdout, "\n");
}

void parse_args(int argc, char** argv, passthru_thing_params *params) {
  int opt;
  while((opt = getopt(argc, argv, "n:i:l:s:c:d")) != -1) {
    switch(opt) {
      case 'n':
        if(strlen(optarg) > 80) {
          printf("ERROR: Thing name must not exceed 80 chars");
          main_exit(1, params);
        }
        params->thingName = MYSTRING_COPY(optarg, strlen(optarg));
        break;
      case 'i':
        if(strlen(optarg) > 10) {
          printf("ERROR: interface value must not exceed 10 chars");
          main_exit(1, params);
        }
        params->iface = MYSTRING_COPY(optarg, strlen(optarg));
        break;
      case 'l':
        if(strlen(optarg) > 255) {
          printf("ERROR: log directory must not exceed 255 chars");
          main_exit(1, params);
        }
        params->logdir = MYSTRING_COPY(optarg, strlen(optarg));
        break;
      case 's':
        if(strlen(optarg) > 255) {
          printf("ERROR: state directory must not exceed 255 chars");
          main_exit(1, params);
        }
        params->cacheDir = MYSTRING_COPY(optarg, strlen(optarg));
        break;
      case 'c':
        if(strlen(optarg) > 255) {
          printf("ERROR: cert directory must not exceed 255 chars");
          main_exit(1, params);
        }
        params->certDir = MYSTRING_COPY(optarg, strlen(optarg));
        break;
      case 'd':
        daemonize = 1;
        break;
      case '?':
        if(isprint(optopt)) {
          syslog(LOG_WARNING, "Unknown option '-%c'.", optopt);
        }
        else {
          syslog(LOG_WARNING, "Unknown option character '\\x%x'.", optopt);
        }
        main_exit(1, params);
        break;
      default:
        syslog(LOG_ERR, "parse_args: error parsing command line arguments");
        main_exit(1, params);
        break;
    }
  }

  if(params->thingName == NULL) {
    params->thingName = malloc(sizeof(char) * 8);
    strcpy(params->thingName, AWS_IOT_MY_THING_NAME);
  }

  if(params->certDir == NULL) {
    params->certDir = PASSTHRU_CERT_DIR;
  }

  if(params->cacheDir == NULL) {
    params->cacheDir = PASSTHRU_CACHE_DIR;
  }
}

int main(int argc, char **argv) {

  print_program_header();

  passthru_thing_params *params = malloc(sizeof(passthru_thing_params));
  params->thingName = NULL;
  params->logdir = NULL;
  params->iface = NULL;
  params->certDir = NULL;
  params->cacheDir = NULL;
  parse_args(argc, argv, params);

  struct sigaction newSigAction;
  sigset_t newSigSet;

  sigemptyset(&newSigSet);
  sigaddset(&newSigSet, SIGCHLD);
  sigaddset(&newSigSet, SIGTSTP);
  sigaddset(&newSigSet, SIGTTOU);
  sigaddset(&newSigSet, SIGTTIN);
  sigprocmask(SIG_BLOCK, &newSigSet, NULL);

  newSigAction.sa_handler = signal_handler;
  sigemptyset(&newSigAction.sa_mask);
  newSigAction.sa_flags = 0;

  sigaction(SIGHUP, &newSigAction, NULL);
  sigaction(SIGTERM, &newSigAction, NULL);
  sigaction(SIGINT, &newSigAction, NULL);

  setlogmask(LOG_UPTO(LOG_DEBUG));
  openlog(params->thingName, LOG_CONS | LOG_PERROR, LOG_USER);
  syslog(LOG_DEBUG, "Starting IoT PassThru Thing: %s", params->thingName);

  if(daemonize) daemon(1, 0);

  passthru_thing_init(params);
  passthru_thing_run();
  passthru_thing_close();
  passthru_thing_destroy();

  return main_exit(EXIT_SUCCESS, params);
}
