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

#ifndef J2534_H_
#define J2534_H_

#include <stdbool.h>
#include <syslog.h>
#include <sys/time.h>
#include <linux/can.h>
#include "vector.h"
#include "passthru_thing.h"
#include "passthru_shadow_parser.h"
#include "awsiot_client.h"
#include "j2534/apigateway.h"

// Return Values
#define STATUS_NOERROR                   0x00000000  // Function completed successfully.
#define ERR_NOT_SUPPORTED                0x00000001  // Device does not support the API function. A fully compliant SAE J2534-1 Pass-Thru Interface shall never return ERR_NOT_SUPPORTED.
#define ERR_INVALID_CHANNEL_ID           0x00000002  // Invalid <ChannelID> value
#define ERR_PROTOCOL_ID_NOT_SUPPORTED    0x00000003  // <ProtocolID> value is not supported (either invalid or unknown)
#define ERR_NULL_PARAMETER               0x00000004  // NULL pointer supplied where a valid pointer is required
#define ERR_IOCTL_VALUE_NOT_SUPPORTED    0x00000005  /* Value referenced in the SCONFIG_LIST structure is either invalid, out of range, or not applicable for the current channel */
#define ERR_FLAG_NOT_SUPPORTED           0x00000006  // <Flags> value(s) are either invalid, unknown, or not applicable for the current channel
#define ERR_FAILED                       0x00000007  /* Undefined error, use PassThruGetLastError for text description. A fully compliant SAE J2534-1 Pass-Thru  Interface shall never return ERR_FAILED. */
#define ERR_DEVICE_NOT_CONNECTED         0x00000008  /* Pass-Thru Device communication error. This indicates that the PassThru Interface DLL has, at some point, failed to communicate with the Pass-Thru Device – even though it may not currently be disconnected. */
#define ERR_TIMEOUT                      0x000000    // Request could not be completed in the designated time
#define ERR_INVALID_MSG                  0x0000000A  // Message structure is invalid for the given <ChannelID> (refer to Section 7.2.4 for more details)
#define ERR_TIME_INTERVAL_NOT_SUPPORTED  0x0000000B  // Value for the <TimeInterval> is either invalid or out of range for the current channel
#define ERR_EXCEEDED_LIMIT               0x0000000C  // Exceeded the allowed limits
#define ERR_INVALID_MSG_ID               0x0000000D  // Invalid <MsgID> value
#define ERR_DEVICE_IN_USE                0x0000000E  // Device is currently open
#define ERR_IOCTL_ID_NOT_SUPPORTED       0x0000000F  // <IoctlID> value is either invalid, unknown, or not applicable for the current channel
#define ERR_BUFFER_EMPTY                 0x00000010  // The buffer is empty, no data available
#define ERR_BUFFER_FULL                  0x00000011  // Buffer is full
#define ERR_BUFFER_OVERFLOW              0x00000012  // Indicates a buffer overflow occurred, data was lost
#define ERR_PIN_NOT_SUPPORTED            0x00000013  // Pin number and/or connector specified is either invalid or unknown
#define ERR_RESOURCE_CONFLICT            0x00000014  // Request causes a resource conflict (such as a pin on a connector is already in use, a data link controller is already in use, etc.)
#define ERR_MSG_PROTOCOL_ID              0x00000015  /* Protocol ID in the PASSTHRU_MSG structure does not match the Protocol ID from the original call to PassThruConnect / PassThruLogicalConnect for the Channel ID */
#define ERR_INVALID_FILTER_ID            0x00000016  // Invalid <FilterID> value
#define ERR_MSG_NOT_ALLOWED              0x00000017  /* Attempting to queue a Segmented Message whose network address and/or <TxFlags> does not match those defined for the <RemoteAddress> or <RemoteTxFlags> during channel creation on a logical communication channel (This Return Value is only applicable to ISO 15765 logical communication channels) */
#define ERR_NOT_UNIQUE                   0x00000018  // Attempt was made to create a duplicate where one is not allowed.
#define ERR_BAUDRATE_NOT_SUPPORTED       0x00000019  // Baud rate is either invalid or unachievable for the current channel
#define ERR_INVALID_DEVICE_ID            0x0000001A  // PassThruOpen has been successfully called, but the current Device ID is not valid
#define ERR_DEVICE_NOT_OPEN              0x0000001B  // PassThruOpen has not been successfully called
#define ERR_NULL_REQUIRED                0x0000001C  // A parameter that is required to be NULL is not set to NULL
#define ERR_FILTER_TYPE_NOT_SUPPORTED    0x0000001D  // <FilterType> is either invalid or unknown for the current channel
#define ERR_IOCTL_PARAM_ID_NOT_SUPPORTED 0x0000001E  // Parameter referenced in the SCONFIG_LIST structure is not supported (either invalid, unknown, or not applicable for the current channel)
#define ERR_VOLTAGE_IN_USE               0x0000001F  // Programming voltage is currently being applied to another pin
#define ERR_PIN_IN_USE                   0x00000020  // Pin number specified is currently in use (either for voltage, ground, or by another channel)
#define ERR_INIT_FAILED                  0x00000021  // Physical vehicle bus initialization failed
#define ERR_OPEN_FAILED                  0x00000022  // There is an invalid name or there is a configuration issue (e.g., firmware/DLL mismatch, etc.) and the associated device could not be opened – run the device configuration application (from the Pass-Thru Interface manufacturer) to resolve
#define ERR_BUFFER_TOO_SMALL             0x00000023  // The size of <DataBuffer>, as indicated by the parameter <DataBufferSize> in the PASSTHRU_MSG structure, is too small to accommodate the full message
#define ERR_LOG_CHAN_NOT_ALLOWED         0x00000024  // Logical communication channel is not allowed for the designated physical communication channel and Protocol ID combination
#define ERR_SELECT_TYPE_NOT_SUPPORTED    0x00000025  // <SelectType> is either invalid or unknown
#define ERR_CONCURRENT_API_CALL          0x00000026  // A J2534 API function has been called before the previous J2534 function call has completed
//Reserved for SAE J2534-1               0x00000027 - 0x00007FFF
//Reserved for SAE                       0x00008000 - 0xFFFFFFFF

// Values for <DeviceAvailable>
#define DEVICE_STATE_UNKNOWN  0x00
#define DEVICE_AVAILABLE      0x01
#define DEVICE_IN_USE         0x02

// Values for <DeviceDLLFWStatus>
#define DEVICE_DLL_FW_COMPATIBILTY_UNKNOWN  0x00000000
#define DEVICE_DLL_FW_COMPATIBLE            0x00000001
#define DEVICE_DLL_OR_FW_NOT_COMPATIBLE     0x00000002
#define DEVICE_DLL_NOT_COMPATIBLE           0x00000003
#define DEVICE_FW_NOT_COMPATIBLE            0x00000004

// Values for <DeviceConnectedMedia>
#define DEVICE_CONN_UNKNOWN  0x00000000
#define DEVICE_CONN_WIRELESS 0x00000001
#define DEVICE_CONN_WIRED    0x00000002

// Values for <ProtocolID>
//Reserved for SAE J2534-1 0x00000000
#define J1850VPW           0x00000001    // GM / Chrysler CLASS2
#define J1850PWM           0x00000002    // Ford SCP
#define ISO9141            0x00000003    // ISO 9141 and ISO 9141-2
#define ISO14230           0x00000004    // ISO 14230 (Keyword Protocol 2000)
#define CAN                0x00000005    // CAN Frames (no transport layer)
//Reserved for SAE J2534-1 0x00000006
#define J2610              0x00000007    // SAE J2610 (Chrysler SCI) for: Configuration A for engine, Configuration A for transmission, Configuration B for engine, Configuration B for transmission
//Reserved for physical communication channels 0x00000008 - 0x00000001FF
#define ISO15765_LOGICAL   0x00000200
//Reserved for logical communication channels 0x00000201 - 0x000003FF
//Reserved for SAE J2534-1 0x00000400 - 0x00007FFF
//Reserved for SAE - 0x00008000 - 0xFFFFFFFF

// Values for <Flags>
//Reserved for SAE          31-16
//Reserved for SAE J2534-1  15-13
#define K_LINE_ONLY         (0 << 12)
#define CAN_ID_BOTH         (0 << 11)
//Reserved for SAE          10
#define CHECKSUM_DISABLED   (0 << 9)
#define CAN_29BIT_ID        (0 << 8)
//Reserved for SAE J2534-1  2-7
//Reserved for SAE          1
#define FULL_DUPLEX         (0 << 0)

// Values for <Connector>
#define J1962_CONNECTOR 0x00000001

// Values for <SelectType>
#define READABLE_TYPE 0x00000001


// Values for <FilterType>
#define PASS_FILTER  0x0000001
#define BLOCK_FILTER 0x0000002


// Values for <Voltage>
#define SHORT_TO_GROUND 0xFFFFFFFE
#define PIN_OFF         0xFFFFFFFF

// Values for <IoctlID>
// Reserved for SAE J2534-1                0x00000000
#define GET_CONFIG                         0x00000001
#define SET_CONFIG                         0x00000002
#define READ_PIN_VOLTAGE                   0x00000003
#define FIVE_BAUD_INIT                     0x00000004
#define FAST_INIT                          0x00000005
//Reserved for SAE J2534-1                 0x00000006
#define CLEAR_TX_QUEUE                     0x00000007
#define CLEAR_RX_QUEUE                     0x00000008
#define CLEAR_PERIODIC_MSGS                0x00000009
#define CLEAR_MSG_FILTERS                  0x0000000A
#define CLEAR_FUNCT_MSG_LOOKUP_TABLE       0x0000000B
#define ADD_TO_FUNCT_MSG_LOOKUP_TABLE      0x0000000C
#define DELETE_FROM_FUNCT_MSG_LOOKUP_TABLE 0x0000000D
#define READ_PROG_VOLTAGE                  0x0000000E
#define BUS_ON                             0x0000000F
// Reserved for SAE J2534-1
// Reserved for SAE


// Values for GET_CONFIG / SET_CONFIG <Parameters>
#define DATA_RATE                0x00000001
//Reserved for J2534-1           0x0000002
//Reserved for J2534-1           0x0000003
#define NODE_ADDRESS             0x00000004
#define NETWORK_LINE             0x00000005
#define P1_MIN                   0x00000006
#define P1_MAX                   0x00000007
#define P2_MIN                   0x00000008
#define P2_MAX                   0x00000009
#define P3_MIN                   0x0000000A
#define P3_MAX                   0x0000000B
#define P4_MIN                   0x0000000C
#define P4_MAX                   0x0000000D
#define W1_MAX                   0x0000000E
#define W2_MAX                   0x0000000F
#define W3_MAX                   0x00000010
#define W4_MIN                   0x00000011
#define W5_MIN                   0x00000012
#define TIDLE                    0x00000013
#define TINIL                    0x00000014
#define TWUP                     0x00000015
#define PARITY                   0x00000016
//Reserved for J2534-1           0x00000017
//Reserved for J2534-1           0x00000018
#define W0_MIN                   0x00000019
#define T1_MAX                   0x0000001A
#define T2_MIN                   0x0000001B
#define T4_MAX                   0x0000001C
#define T5_MIN                   0x0000001D
#define ISO15765_BS              0x0000001E
#define ISO15765_STMIN           0x0000001F
#define DATA_BITS                0x00000020
#define FIVE_BAUD_MOD            0x00000021
#define BS_TX                    0x00000022
#define STMIN_TX                 0x00000023
#define T3_MAX                   0x00000024
#define ISO15765_WAIT_LIMIT      0x00000025
#define W1_MIN                   0x00000026
#define W2_MIN                   0x00000027
#define W3_MIN                   0x00000028
#define W4_MAX                   0x00000029
#define N_BR_MIN                 0x0000002A
#define ISO15765_PAD_VALUE       0x0000002B
#define N_AS_MAX                 0x0000002C
#define N_AR_MAX                 0x0000002D
#define N_BS_MAX                 0x0000002E
#define N_CR_MAX                 0x0000002F
#define N_CS_MIN                 0x00000030
#define ECHO_PHYSICAL_CHANNEL_TX 0x00000031
//Reserved for J2534-1 $00000000 and $00000032 to $00007FFF
//Reserved for SAE $00008000 to $FFFFFFFF

// Pre-defined values for GET_CONFIG / SET_CONFIG Parameters
#define BUS_NORMAL    0x00000000
#define BUS_PLUS      0x00000001
#define BUS_MINUS     0x00000002
#define NO_PARITY     0x00000000
#define ODD_PARITY    0x00000001
#define EVEN_PARITY   0x00000002
#define DATA_BITS_8   0x00000000
#define DATA_BITS_7   0x00000001
#define ISO_STD_INIT  0x00000000
#define ISO_INV_KB2   0x00000001
#define ISO_INV_ADD   0x00000002
#define ISO_9141_STD  0x00000003
#define DISABLE_ECHO  0x00000000
#define ENABLE_ECHO   0x00000001

typedef struct {
  char DeviceName[80];
  unsigned long DeviceAvailable;
  unsigned long DeviceDLLFWStatus;
  unsigned long DeviceConnectMedia;
  unsigned long DeviceConnectSpeed;
  unsigned long DeviceSignalQuality;
  unsigned long DeviceSignalStrength;
} SDEVICE;

typedef struct {
  unsigned long Connector;        /* connector identifier */
  unsigned long NumOfResources;   /* number of resources pointed to by ResourceListPtr */
  unsigned long *ResourceListPtr; /* pointer to list of resources */
} RESOURCE_STRUCT;

typedef struct {
 unsigned long LocalTxFlags;      /* TxFlags for the Local Address */
 unsigned long RemoteTxFlags;     /* TxFlags for the Remote Address */
 char LocalAddress[5];            /* Address for J2534 Device ISO 15765 end point */
 char RemoteAddress[5];           /* Address for remote/vehicle ISO 15765 end point */
} ISO15765_CHANNEL_DESCRIPTOR;

typedef struct {
  unsigned long ChannelCount;     /* number of ChannelList elements */
  unsigned long ChannelThreshold; /* minimum number of channels that must have messages */
  unsigned long *ChannelList;     /* pointer to an array of Channel IDs to be monitored */
} SCHANNELSET;

typedef struct {
  unsigned long Parameter;        /* ID of parameter */
  unsigned long Value;            /* value of the parameter */
} SCONFIG;

typedef struct {
  unsigned long NumOfParams;      /* size of SCONFIG array */
  SCONFIG *ConfigPtr;             /* array of SCONFIG */
} SCONFIG_LIST;

typedef struct {
  unsigned long NumOfBytes;       /* number of bytes in the array */
  unsigned char *BytePtr;         /* array of bytes */
} SBYTE_ARRAY;

typedef struct {
 unsigned long ProtocolID;
 unsigned long MsgHandle;
 unsigned long RxStatus;
 unsigned long TxFlags;
 unsigned long Timestamp;
 unsigned long DataLength;
 unsigned long ExtraDataIndex;
 unsigned char *DataBuffer;
 unsigned long DataBufferSize;
} PASSTHRU_MSG;

// v05.00
long PassThruScanForDevices(unsigned long *pDeviceCount);
long PassThruGetNextDevice(SDEVICE *psDevice);
long PassThruOpen(const char *pName, unsigned long *pDeviceID);
long PassThruClose(unsigned long DeviceID);
long PassThruConnect(unsigned long DeviceID, unsigned long ProtocolID, unsigned long Flags, unsigned long BaudRate, RESOURCE_STRUCT ResourceStruct, unsigned long *pChannelID);
long PassThruDisconnect(unsigned long ChannelID);
long PassThruLogicalConnect(unsigned long PhysicalChannelID, unsigned long ProtocolID, unsigned long Flags, void *pChannelDescriptor, unsigned long *pChannel);
long PassThruLogicalDisconnect(unsigned long ChannelID);
long PassThruSelect(SCHANNELSET *ChannelSetPtr, unsigned long SelectType, unsigned long Timeout);
long PassThruReadMsgs(unsigned long ChannelID, PASSTHRU_MSG *pMsg, unsigned long *pNumMsgs, unsigned long Timeout);
long PassThruQueueMsgs(unsigned long ChannelID, PASSTHRU_MSG *pMsg, unsigned long *pNumMsgs);
long PassThruStartPeriodicMsg(unsigned long ChannelID, PASSTHRU_MSG *pMsg, unsigned long *pMsgID, unsigned long TimeInterval);
long PassThruStopPeriodicMsg(unsigned long ChannelID, unsigned long MsgID);
long PassThruStartMsgFilter(unsigned long ChannelID, unsigned long FilterType, PASSTHRU_MSG *pMaskMsg, PASSTHRU_MSG *pPatternMsg, unsigned long *pFilterID);
long PassThruStopMsgFilter(unsigned long ChannelID, unsigned long FilterID);
long PassThruSetProgrammingVoltage(unsigned long DeviceID, RESOURCE_STRUCT ResourceStruct, unsigned long Voltage);
long PassThruReadVersion(unsigned long DeviceID, char *pFirmwareVersion, char *pDllVersion, char *pApiVersion);
long PassThruGetLastError(char *pErrorDescription);
long PassThruIoctl(unsigned long ControlTarget, unsigned long IoctlID, void *InputPtr, void *OutputPtr);

// non-J2534 spec (implementation specific)
#define J2534_API_VERSION                   "0.5.0"
#define J2534_DLL_VERSION                   "0.1.0"
#define J2534_ERROR_TOPIC                   "$aws/things/%s/shadow/update/rejected"
#define J2534_MSG_RX_TOPIC                  "ecutools/j2534/%s/rx"
#define J2534_MSG_TX_TOPIC                  "ecutools/j2534/%s/tx"
#define J2534_MSG_BUFFER_SIZE               1000
#define J2534_TIMEOUT_MILLIS                30000
#define J2534_PassThruScanForDevices        1
#define J2534_PassThruGetNextDevice         2
#define J2534_PassThruOpen                  3
#define J2534_PassThruClose                 4
#define J2534_PassThruConnect               5
#define J2534_PassThruDisconnect            6
#define J2534_PassThruLogicalConnect        7
#define J2534_PassThruLogicalDisconnect     8
#define J2534_PassThruSelect                9
#define J2534_PassThruReadMsgs              10
#define J2534_PassThruQueueMsgs             11
#define J2534_PassThruStartPeriodicMsg      12
#define J2534_PassThruStopPeriodicMsg       13
#define J2534_PassThruStartMsgFilter        14
#define J2534_PassThruStopMsgFilter         15
#define J2534_PassThruSetProgrammingVoltage 16
#define J2534_PassThruReadVersion           17
#define J2534_PassThruGetLastError          18
#define J2534_PassThruIoctl                 19

typedef struct {
  unsigned long *id;
  canid_t can_id;
  canid_t can_mask;
} j2534_canfilter;

typedef struct {
  char *name;
  int *state;
  unsigned long deviceId;
  unsigned long channelId;
  unsigned long protocolId;
  bool opened;
  char *shadow_update_topic;
  char *shadow_update_accepted_topic;
  char *shadow_error_topic;
  char *msg_rx_topic;
  char *msg_tx_topic;
  SDEVICE *device;
  SCHANNELSET *channelSet;
  awsiot_client *awsiot;
  canbus_client *canbus;
  vector *rxQueue;
  vector *txQueue;
  vector *filters;
} j2534_client;

void j2534_send_error(awsiot_client *awsiot, unsigned int error);
// end non-J2534 spec

#endif
