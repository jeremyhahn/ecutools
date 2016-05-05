/**
 * ecutools: IoT Automotive Tuning, Diagnostics & Analytics
 * Copyright (C) 2014  Jeremy Hahn
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
#include "passthru_thing.h"

passthru_thing *thing;

int main_exit(int exit_status) {
  syslog(LOG_DEBUG, "exiting ecutune");
  closelog();
  return exit_status;
}

void signal_handler(int sig) {
  switch(sig) {
	case SIGHUP:
	  syslog(LOG_DEBUG, "Received SIGHUP signal");
	  // Reload config and reopen files
	  break;
	case SIGINT:
    if(thing != NULL) {
      passthru_thing_close(thing);
    }
	case SIGTERM:
	  syslog(LOG_DEBUG, "SIGINT/SIGTERM");
	  //passthru_thing_close(thing);
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
}

int main(int argc, char **argv) {

  print_program_header();

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
  openlog("ecutuned", LOG_CONS | LOG_PERROR, LOG_USER);
  syslog(LOG_DEBUG, "starting ecutune");

  thing = passthru_thing_new();
  passthru_thing_run(thing);
  passthru_thing_close(thing);
  passthru_thing_destroy(thing);

  return main_exit(EXIT_SUCCESS);
}
