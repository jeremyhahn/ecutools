#include "j2534_shadow.h"

awsiot_client awsiot;

void j2534_shadow_init() {
  //awsiot
}



int j2534_shadow_start_listening() {
  awsiot_client_connect(bridge->awsiot);
  if(bridge->awsiot->rc != NONE_ERROR) {
    syslog(LOG_CRIT, "iotbridge_run: unable to connect to AWS IoT service");
    return -1;
  }
}