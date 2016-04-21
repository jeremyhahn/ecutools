#ifndef J2534_H_
#define J2534_H_

enum ProtocolFilter {
  PASS,
  BLOCK,
  FLOW_CONTROL
};

enum ProtocolType {
  J1850VPW        = 0x01,
  J1850PWM        = 0x02,
  ISO9141         = 0x03,
  ISO14230        = 0x04,
  CAN             = 0x05,
  ISO15765        = 0x06,
  SCI_A_ENGINE    = 0x07,
  SCI_A_TRANS     = 0x08,
  SCI_B_ENGINE    = 0x09,
  SCI_B_TRANS     = 0x0A
};

enum Error {
  STATUS_NOERROR            = 0,
  ERR_DEVICE_NOT_CONNECTED  = 0x01,
  ERR_NULL_PARAMETER        = 0x02,
  ERR_INVALID_FLAGS         = 0x03,
  ERR_FAILED                = 0x04,
  ERR_CHANNEL_IN_USE        = 0x05
};

typedef struct {
  unsigned long ProtocolID;     /* vehicle network protocol */
  unsigned long RxStatus;       /* receive message status */
  unsigned long TxFlags;        /* transmit message flags */
  unsigned long Timestamp;      /* receive message timestamp(in microseconds) */
  unsigned long DataSize;       /* byte size of message payload in the Data array */
  unsigned long ExtraDataIndex; /* start of extra data(i.e. CRC, checksum, etc) in Data array */
  unsigned char Data[4128];     /* message payload or data */
} PASSTHRU_MSG;

typedef struct {
  unsigned long NumOfBytes;     /* Number of functional addresses in array */
  unsigned char *BytePtr;       /* pointer to functional address array */
} SBYTE_ARRAY;

typedef struct {
  unsigned long Parameter;      /* Name of configuration parameter */
  unsigned long Value;          /* Value of configuration parameter */
} SCONFIG;

typedef struct {
  unsigned long NumOfParams;    /* size of SCONFIG array */
  SCONFIG *ConfigPtr;             /* array containing configuration item(s) */
} SCONFIG_LIST;

long PassThruConnect(unsigned long ProtocolID, unsigned long Flags, unsigned long *pChannelID);
long PassThruDisconnect(unsigned long ChannelID);
long PassThruReadMsgs(unsigned long ChannelID, PASSTHRU_MSG *pMsg, unsigned long *pNumMsgs, unsigned long Timeout);
long PassThruWriteMsgs(unsigned long ChannelID, PASSTHRU_MSG *pMsg, unsigned long *pNumMsgs, unsigned long Timeout);
long PassThruStartPeriodicMsg(unsigned long ChannelID, PASSTHRU_MSG *pMsg, unsigned long *pMsgID, unsigned long TimeInterval);
long PassThruStopPeriodicMsg(unsigned long ChannelID, unsigned long MsgID);
long PassThruStartMsgFilter(unsigned long ChannelID, unsigned long FilterType, PASSTHRU_MSG *pMaskMsg, PASSTHRU_MSG *pPatternMsg,
		PASSTHRU_MSG *pFlowControlMsg,	unsigned long *pMsgID);
long PassThruStopMsgFilter(unsigned long ChannelID, unsigned long MsgID);
long PassThruSetProgrammingVoltage(unsigned long PinNumber, unsigned long Voltage);
long PassThruReadVersion(char *pFirmwareVersion, char * pDllVersion, char *pApiVersion);
long PassThruGetLastError(char *pErrorDescription);
long PassThruIoctl(unsigned long ChannelID, unsigned long IoctlID, void *pInput, void *pOutput);

#endif /* J2534_H_ */
