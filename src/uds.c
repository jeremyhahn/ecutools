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
#include "uds.h"

/**
 * The Diagnostic Session Control service is used to switch between
 * different sessions. Three session types have been implemented,
 * default session, extended diagnostic session and programming session.
 *
 * Example: Request Programming Session
 * Byte #: 0       1       2       3       4       5       6       7
 * Value:  0x01    0x10    0x02    0x55    0x55    0x55    0x55    0x55
 *
 * Example: Positive response for programming session request
 * Byte #: 0       1       2       3       4       5       6       7
 * Value:  0x01    0x50    0x02    0xAA    0xAA    0xAA    0xAA    0xAA
 */
int uds_set_diagnostic_session_control(int session) {

  if(session != UDS_DSC_TYPE_DEFAULT_SESSION &&
	session != UDS_DSC_TYPE_EXTENDED_SESSION &&
	session != UDS_DSC_TYPE_PROGRAMMING_SESSION) {

	return UDS_RESPONSE_subFunctionNotSupported;
  }

  // Send ECU command
  printf("uds_set_diagnostic_session_control invoked!\n");
  //log_info("uds_set_diagnostic_session_control invoked!");
  //log_close();

  return session;
}

/**
 * Shall be used to stop or resume the setting of diagnostic trouble codes in the ECU.
 *
 * Example: Request Control DTC Settings Off
 * Byte #: 0       1       2       3       4       5       6       7
 * Value:  0x01    0x85    0x02    0x55    0x55    0x55    0x55    0x55
 *
 * Example: Positive response for Control DTC Settings request
 * Byte #: 0       1       2       3       4       5       6       7
 * Value:  0x01    0xC5    0x02    0xAA    0xAA    0xAA    0xAA    0xAA
 */
int uds_set_diagnostic_trouble_code_setting(int setting) {

  if(setting != UDS_DTC_TYPE_ON && setting != UDS_DTC_TYPE_OFF) {
	return UDS_RESPONSE_subFunctionNotSupported;
  }

  // Send ECU command
  return setting;
}

/**
 * This service is used to switch on/off reception and/or transmission of certain messages.
 *
 * Example: Request Communication Control enables Rx and Tx for normal communication messages
 * Byte #: 0       1       2       3       4       5       6       7
 * Value:  0x01    0x28    0x01    0x00    0x55    0x55    0x55    0x55
 *
 * Example: Positive response for Communication Control request
 * Byte #: 0       1       2       3       4       5       6       7
 * Value:  0x01    0x68    0x00    0xAA    0xAA    0xAA    0xAA    0xAA
 */
int* uds_set_communication_control(int communicationType, int controlType) {

  // Send ECU command
  return (int *)1;
}

/**
 * To prevent that the ECU is modified by unauthorized persons most UDS services are locked. To
 * get access to services that are used to modify the ECU the user first has to grant access
 * through the Security Access Service. Only after the security access service have been passed,
 * services like Request Download and Transfer Data can be used. The security concept used is
 * called “Seed and Key”.
 *
 * Security Access Service flow:
 * 1. The client sends a request for a “seed” to the server that it wants to unlock.
 * 2. The server replies by sending the “seed” back to the client.
 * 3. The client then generates a “key” based on the “seed” and sends the key to the server.
 * 4. If the client generated the “key” with the correct algorithm the server will respond that the “key” was valid and that it will unlock itself.
 *
 * Example: Request Seed
 * Byte #: 0       1       2       3       4       5       6       7
 * Value:  0x01    0x27    0x01    0x55    0x55    0x55    0x55    0x55
 *
 * Example: Positive response for security access request
 * Byte #: 0       1       2       3       4       5       6       7
 * Value:  0x01    0x67    0x02    0x01    0x02    0xAA    0xAA    0xAA
 */
int uds_request_seed() {

  // Send ECU command
  int seed = 1;
  return seed;
}

/**
 * The request download service is called when data is to be transferred to the ECU. The
 * request shall contain the size of the data to be transferred and the address to where
 * it shall be placed. The ECU responds with the size of its buffer so that the sender
 * can divide the data into appropriate sized blocks and send them one at a time.
 *
 * dataFormatIdentifier
 * 		This data parameter is a one-byte value with each nibble encoded separately.
 * 		The high nibble specifies the "compressionMethod" and the low nibble specifies
 * 		the "encryptingMethod". The value 0x00 specifies that no compressionMethod or
 * 		encryptingMethod is used.
 *
 * addressAndLengthFormatIdentifier
 *		bit 7 - 4: Length (number of bytes) of the memorySize parameter.
 *		bit 3 - 0: Length (number of bytes) of the memoryAddress parameter.
 *
 * memoryAddress
 * 		The parameter memoryAddress is the starting address of the server memory to
 * 		which the data is to be written.
 *
 * memorySize (unCompressedMemorySize)
 * 		This parameter shall be used by the server to compare the uncompressed memory
 * 		size with the total amount of data transferred during the TransferData service.
 */
int uds_request_download(int dataFormatIdentifier, int addressAndLengthFormatIdentifier, int memoryAddress, int memorySize) {
  // Send ECU command
  return 1;
}

/**
 * The Transfer Data service receives blocks of data and check so the blocks are received
 * in the right order. If the right block is received, then it is written to correct memory
 * location and a positive response is send.
 *
 * blockSequenceCounter
 * 		The blockSequenceCounter parameter value starts at 01 hex with the first TransferData
 * 		request that follows the RequestDownload (34 hex) service. Its value is incremented by
 * 		1 for each subsequent TransferData request. At the value of FF hex, the blockSequenceCounter
 * 		rolls over and starts at 00 hex with the next TransferData request message.
 */
int uds_transfer_data(int address, unsigned char *data) {

  //uds_increment_block_sequence_counter();

  // Send ECU command
  int blockSequenceCounter = 1;  // replace w/ response from ecu

  //char text[8];
  //sprintf(text, "0x%d", blockSequenceCounter);

  return blockSequenceCounter;
}

/**
 * When the transfer of data is complete a message to the Request Transfer Exit service is sent. If
 * Transfer Data is complete and all data was received a positive response is sent back.
 */
int uds_request_transfer_exit() {

  // Send ECU command
  return 1;
}

/**
 * The Routine Control service is used to start or stop a routine or to request routine results.
 */
int uds_set_routine_control(int controlType) {

  // Send ECU command
  return controlType;
}

/**
 * To do a hard reset of the ECU, the ECU Reset service can be used by sending a ECUReset request message.
 *
 * Example: Positive response for ECU Reset request
 * Byte #: 0       1       2       3       4       5       6       7
 * Value:  0x01    0x51    0x01    0x00    0xAA    0xAA    0xAA    0xAA
 */
int uds_ecu_reset(int resetType) {

  // Send ECU command
  return resetType;
}

/**
 * The blockSequenceCounter parameter value starts at 01 hex with the first TransferData request that
 * follows the RequestDownload (34 hex) service. Its value is incremented by 1 for each subsequent
 * TransferData request. At the value of FF hex, the blockSequenceCounter rolls over and starts
 * at 00 hex with the next TransferData request message.
 */
void uds_increment_block_sequence_counter() {
  blockSequenceNumber = blockSequenceNumber + 1;
  if(blockSequenceNumber == 256) {
	blockSequenceNumber = 0;
  }
}

/**
 * pci
 *    Protocol Control Information
 *
 * sid
 *    Service Identifier
 *
char* uds_create_frame(int pci, int sid) {
	return char[pci, sid];
}*/
