module Ecutools::J2534::Constants

  # Values for <DeviceAvailable>
  DEVICE_STATE_UNKNOWN  = 0x00
  DEVICE_AVAILABLE      = 0x01
  DEVICE_IN_USE         = 0x02

  # Values for <DeviceDLLFWStatus>
  DEVICE_DLL_FW_COMPATIBILTY_UNKNOWN  = 0x00000000
  DEVICE_DLL_FW_COMPATIBLE            = 0x00000001
  DEVICE_DLL_OR_FW_NOT_COMPATIBLE     = 0x00000002
  DEVICE_DLL_NOT_COMPATIBLE           = 0x00000003
  DEVICE_FW_NOT_COMPATIBLE            = 0x00000004

  # Values for <DeviceConnectedMedia>
  DEVICE_CONN_UNKNOWN  = 0x00000000
  DEVICE_CONN_WIRELESS = 0x00000001
  DEVICE_CONN_WIRED    = 0x00000002

  # Values for <ProtocolID>
  #Reserved for SAE J2534-1 0x00000000
  J1850VPW           = 0x00000001    # GM / Chrysler CLASS2
  J1850PWM           = 0x00000002    # Ford SCP
  ISO9141            = 0x00000003    # ISO 9141 and ISO 9141-2
  ISO14230           = 0x00000004    # ISO 14230 (Keyword Protocol 2000)
  CAN                = 0x00000005    # CAN Frames (no transport layer)
  #Reserved for SAE J2534-1 0x00000006
  J2610              = 0x00000007    # SAE J2610 (Chrysler SCI) for: Configuration A for engine, Configuration A for transmission, Configuration B for engine, Configuration B for transmission
  #Reserved for physical communication channels 0x00000008 - 0x00000001FF
  ISO15765_LOGICAL   = 0x00000200
  #Reserved for logical communication channels 0x00000201 - 0x000003FF
  #Reserved for SAE J2534-1 0x00000400 - 0x00007FFF
  #Reserved for SAE - 0x00008000 - 0xFFFFFFFF

  # Values for <Flags>
  #Reserved for SAE          31-16
  #Reserved for SAE J2534-1  15-13
  K_LINE_ONLY         = (0 << 12)
  CAN_ID_BOTH         = (0 << 11)
  #Reserved for SAE          10
  CHECKSUM_DISABLED   = (0 << 9)
  CAN_29BIT_ID        = (0 << 8)
  #Reserved for SAE J2534-1  2-7
  #Reserved for SAE          1
  FULL_DUPLEX         = (0 << 0)

  # Values for <Connector>
  J1962_CONNECTOR = 0x00000001

  # Values for <SelectType>
  READABLE_TYPE = 0x00000001


  # Values for <FilterType>
  PASS_FILTER  = 0x0000001
  BLOCK_FILTER = 0x0000002


  # Values for <Voltage>
  SHORT_TO_GROUND = 0xFFFFFFFE
  PIN_OFF         = 0xFFFFFFFF

  # Values for <IoctlID>
  # Reserved for SAE J2534-1                0x00000000
  GET_CONFIG                         = 0x00000001
  SET_CONFIG                         = 0x00000002
  READ_PIN_VOLTAGE                   = 0x00000003
  FIVE_BAUD_INIT                     = 0x00000004
  FAST_INIT                          = 0x00000005
  #Reserved for SAE J2534-1                 0x00000006
  CLEAR_TX_QUEUE                     = 0x00000007
  CLEAR_RX_QUEUE                     = 0x00000008
  CLEAR_PERIODIC_MSGS                = 0x00000009
  CLEAR_MSG_FILTERS                  = 0x0000000A
  CLEAR_FUNCT_MSG_LOOKUP_TABLE       = 0x0000000B
  ADD_TO_FUNCT_MSG_LOOKUP_TABLE      = 0x0000000C
  DELETE_FROM_FUNCT_MSG_LOOKUP_TABLE = 0x0000000D
  READ_PROG_VOLTAGE                  = 0x0000000E
  BUS_ON                             = 0x0000000F
  # Reserved for SAE J2534-1
  # Reserved for SAE


  # Values for GET_CONFIG / SET_CONFIG <Parameters>
  DATA_RATE                = 0x00000001
  #Reserved for J2534-1           0x0000002
  #Reserved for J2534-1           0x0000003
  NODE_ADDRESS             = 0x00000004
  NETWORK_LINE             = 0x00000005
  P1_MIN                   = 0x00000006
  P1_MAX                   = 0x00000007
  P2_MIN                   = 0x00000008
  P2_MAX                   = 0x00000009
  P3_MIN                   = 0x0000000A
  P3_MAX                   = 0x0000000B
  P4_MIN                   = 0x0000000C
  P4_MAX                   = 0x0000000D
  W1_MAX                   = 0x0000000E
  W2_MAX                   = 0x0000000F
  W3_MAX                   = 0x00000010
  W4_MIN                   = 0x00000011
  W5_MIN                   = 0x00000012
  TIDLE                    = 0x00000013
  TINIL                    = 0x00000014
  TWUP                     = 0x00000015
  PARITY                   = 0x00000016
  #Reserved for J2534-1           0x00000017
  #Reserved for J2534-1           0x00000018
  W0_MIN                   = 0x00000019
  T1_MAX                   = 0x0000001A
  T2_MIN                   = 0x0000001B
  T4_MAX                   = 0x0000001C
  T5_MIN                   = 0x0000001D
  ISO15765_BS              = 0x0000001E
  ISO15765_STMIN           = 0x0000001F
  DATA_BITS                = 0x00000020
  FIVE_BAUD_MOD            = 0x00000021
  BS_TX                    = 0x00000022
  STMIN_TX                 = 0x00000023
  T3_MAX                   = 0x00000024
  ISO15765_WAIT_LIMIT      = 0x00000025
  W1_MIN                   = 0x00000026
  W2_MIN                   = 0x00000027
  W3_MIN                   = 0x00000028
  W4_MAX                   = 0x00000029
  N_BR_MIN                 = 0x0000002A
  ISO15765_PAD_VALUE       = 0x0000002B
  N_AS_MAX                 = 0x0000002C
  N_AR_MAX                 = 0x0000002D
  N_BS_MAX                 = 0x0000002E
  N_CR_MAX                 = 0x0000002F
  N_CS_MIN                 = 0x00000030
  ECHO_PHYSICAL_CHANNEL_TX = 0x00000031
  #Reserved for J2534-1 $00000000 and $00000032 to $00007FFF
  #Reserved for SAE $00008000 to $FFFFFFFF

  # Pre-defined values for GET_CONFIG / SET_CONFIG Parameters
  BUS_NORMAL    = 0x00000000
  BUS_PLUS      = 0x00000001
  BUS_MINUS     = 0x00000002
  NO_PARITY     = 0x00000000
  ODD_PARITY    = 0x00000001
  EVEN_PARITY   = 0x00000002
  DATA_BITS_8   = 0x00000000
  DATA_BITS_7   = 0x00000001
  ISO_STD_INIT  = 0x00000000
  ISO_INV_KB2   = 0x00000001
  ISO_INV_ADD   = 0x00000002
  ISO_9141_STD  = 0x00000003
  DISABLE_ECHO  = 0x00000000
  ENABLE_ECHO   = 0x00000001
end
