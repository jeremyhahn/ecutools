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

#ifndef UDS_H
#define UDS_H

#include <stdio.h>
//#include "log.h"

// UDS Frame Types
const static unsigned int UDS_FRAME_TYPE_SF = 0x01;
const static unsigned int UDS_FRAME_TYPE_FF = 0x02;
const static unsigned int UDS_FRAME_TYPE_CF = 0x03;

// Diagnostic Session Control
const static unsigned int UDS_DSC_PCI                      = 0x01;
const static unsigned int UDS_DSC_REQUEST_SERVICE_ID       = 0x10;
const static unsigned int UDS_DSC_RESPONSE_SERVICE_ID      = 0x50;
const static unsigned int UDS_DSC_TYPE_DEFAULT_SESSION     = 0x01;
const static unsigned int UDS_DSC_TYPE_EXTENDED_SESSION    = 0x02;
const static unsigned int UDS_DSC_TYPE_PROGRAMMING_SESSION = 0x03;

// Diagnostic Trouble Code (DTC) Settings
const static unsigned int UDS_DTC_PCI                 = 0x01;
const static unsigned int UDS_DTC_REQUEST_SERVICE_ID  = 0x85;
const static unsigned int UDS_DTC_RESPONSE_SERVICE_ID = 0xC5;
const static unsigned int UDS_DTC_TYPE_ON             = 0x01;
const static unsigned int UDS_DTC_TYPE_OFF            = 0x02;

// Communication Control Control
const static unsigned int UDS_CCC_PCI                                   = 0x01;
const static unsigned int UDS_CCC_REQUEST_SERVICE_ID                    = 0x28;
const static unsigned int UDS_CCC_RESPONSE_SERVICE_ID                   = 0x68;
const static unsigned int UDS_CCC_COMMUNICATION_TYPE_NORMAL             = 0x01;
const static unsigned int UDS_CCC_COMMUNICATION_TYPE_NETWORK            = 0x02;
const static unsigned int UDS_CCC_COMMUNICATION_TYPE_NETWORK_AND_NORMAL = 0x03;
const static unsigned int UDS_CCC_CONTROL_TYPE_enableRxAndTx            = 0x00;
const static unsigned int UDS_CCC_CONTROL_TYPE_enableRxAndDisableTx     = 0x01;
const static unsigned int UDS_CCC_CONTROL_TYPE_disableRxAndEnableTx     = 0x02;
const static unsigned int UDS_CCC_CONTROL_TYPE_disableRxAndTx           = 0x03;

// Security Access
const static unsigned int UDS_SA_PCI          = 0x01;
const static unsigned int UDS_SA_REQUEST_ID   = 0x27;
const static unsigned int UDS_SA_RESPONSE_ID  = 0x67;
const static unsigned int UDS_SA_REQUEST_SEED = 0x01;
const static unsigned int UDS_SA_SEND_KEY     = 0x02;

// Request Download
const static unsigned int UDS_RD_PCI = 0x01;
const static unsigned int UDS_RD_REQUEST_ID = 0x34;
const static unsigned int UDS_RD_RESPONSE_ID = 0x74;

// Transfer Data
const static unsigned int UDS_TD_PCI = 0x01; // 0x01/0x10/0x20-0x2F
const static unsigned int UDS_TD_REQUEST_ID = 0x36;
const static unsigned int UDS_TD_RESPONSE_ID = 0x76;

// Request Transfer Exit
const static unsigned int UDS_RTE_PCI         = 0x01;
const static unsigned int UDS_RTE_REQUEST_ID  = 0x37;
const static unsigned int UDS_RTE_RESPONSE_ID = 0x77;

// Routine Control
const static unsigned int UDS_RC_PCI                  = 0x01;
const static unsigned int UDS_RC_REQUEST_ID           = 0x31;
const static unsigned int UDS_RC_CONTROL_TYPE_START   = 0x01;
const static unsigned int UDS_RC_CONTROL_TYPE_STOP    = 0x02;
const static unsigned int UDS_RC_CONTROL_TYPE_REQUEST = 0x03;

// ECU Reset
const static unsigned int UDS_ECURST_PCI                            = 0x01;
const static unsigned int UDS_ECURST_REQUEST_ID                     = 0x11;
const static unsigned int UDS_ECURST_RESPONSE_ID                    = 0x51;
const static unsigned int UDS_ECURST_TYPE_hardReset                 = 0x01;
const static unsigned int UDS_ECURST_TYPE_keyOffOnReset             = 0x02;
const static unsigned int UDS_ECURST_TYPE_softReset                 = 0x03;
const static unsigned int UDS_ECURST_TYPE_enableRapidPowerShutDown  = 0x04;
const static unsigned int UDS_ECURST_TYPE_disableRapidPowerShutdown = 0x05;

// Negative Responses
const static unsigned int UDS_RESPONSE_subFunctionNotSupported               = 0x12; // 0x11, 0x12, 0x7E, 0x7F
const static unsigned int UDS_RESPONSE_incorrectMessageLengthOrInvalidFormat = 0x13;
const static unsigned int UDS_RESPONSE_busyRepeatRequest                     = 0x21;
const static unsigned int UDS_RESPONSE_conditionsNotCorrect                  = 0x22;
const static unsigned int UDS_RESPONSE_requestSequenceError                  = 0x24;
const static unsigned int UDS_RESPONSE_requestOutOfRange                     = 0x31;
const static unsigned int UDS_RESPONSE_securityAccessDenied                  = 0x33;
const static unsigned int UDS_RESPONSE_invalidKey                            = 0x35;
const static unsigned int UDS_RESPONSE_exceededNumberOfAttempts              = 0x36;
const static unsigned int UDS_RESPONSE_requiredTimeDelayNotExpired           = 0x37;
const static unsigned int UDS_RESPONSE_uploadDownloadNotAccepted             = 0x70;
const static unsigned int UDS_RESPONSE_transferDataSuspended                 = 0x71;
const static unsigned int UDS_RESPONSE_generalProgrammingFailure             = 0x72;
const static unsigned int UDS_RESPONSE_wrongBlockSequenceCounter             = 0x73;
const static unsigned int UDS_RESPONSE_busyResponsePending                   = 0x78;
const static unsigned int UDS_RESPONSE_udsUndefinedError                     = 0x80;

unsigned int blockSequenceNumber; // 0x00-0xFF, 0-256
void uds_increment_block_sequence_counter();

int uds_set_diagnostic_session_control(int session);
int uds_set_diagnostic_trouble_code_setting(int setting);
int* uds_set_communication_control(int communicationType, int controlType);
int uds_request_seed();
int uds_request_download(int dataFormatIdentifier, int addressAndLengthFormatIdentifier, int memoryAddress, int memorySize);
int uds_transfer_data(int address, unsigned char *data);
int uds_request_transfer_exit();
int uds_set_routine_control(int controlType);
int uds_ecu_reset(int resetType);

#endif // UDS_H
