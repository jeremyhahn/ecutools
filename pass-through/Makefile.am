#AUTOMAKE_OPTIONS = foreign
APP_DIR = ./src
APP_INCLUDE_DIRS = -I $(APP_DIR)
APP_NAME=ecutune

#ecutools
ECUTOOLS_SRC_FILES = src/canbus.h src/canbus.c src/wcbridge.h src/wcbridge.c src/awsiot_client.h src/awsiot_client.c src/iotbridge.h src/iotbridge.c src/ecutune.c src/j2534.h src/j2534.c src/apigateway.h src/apigateway.c

#cwebsocket
#CWEBSOCKET_SRC_FILES = $(shell find src/cwebsocket -name '*.c')
CWEBSOCKET_SRC_FILES = src/cwebsocket.c src/cwebsocket/common.c src/cwebsocket/utf8.c

#IoT client directory
IOT_CLIENT_DIR=./src/aws_iot_src
IOT_INCLUDE_DIRS =
IOT_INCLUDE_DIRS += -I $(IOT_CLIENT_DIR)/protocol/mqtt
IOT_INCLUDE_DIRS += -I $(IOT_CLIENT_DIR)/protocol/mqtt/aws_iot_embedded_client_wrapper
IOT_INCLUDE_DIRS += -I $(IOT_CLIENT_DIR)/protocol/mqtt/aws_iot_embedded_client_wrapper/platform_linux
IOT_INCLUDE_DIRS += -I $(IOT_CLIENT_DIR)/protocol/mqtt/aws_iot_embedded_client_wrapper/platform_linux/common
IOT_INCLUDE_DIRS += -I $(IOT_CLIENT_DIR)/protocol/mqtt/aws_iot_embedded_client_wrapper/platform_linux/openssl
IOT_INCLUDE_DIRS += -I $(IOT_CLIENT_DIR)/utils

PLATFORM_DIR = $(IOT_CLIENT_DIR)/protocol/mqtt/aws_iot_embedded_client_wrapper/platform_linux/openssl
PLATFORM_COMMON_DIR = $(IOT_CLIENT_DIR)/protocol/mqtt/aws_iot_embedded_client_wrapper/platform_linux/common
IOT_SRC_FILES =
IOT_SRC_FILES += $(IOT_CLIENT_DIR)/protocol/mqtt/aws_iot_embedded_client_wrapper/aws_iot_mqtt_embedded_client_wrapper.c
IOT_SRC_FILES += $(shell find $(PLATFORM_DIR)/ -name '*.c')
IOT_SRC_FILES += $(shell find $(PLATFORM_COMMON_DIR)/ -name '*.c')

#MQTT Paho Embedded C client directory
MQTT_DIR = ./src/aws_mqtt_embedded_client_lib
MQTT_C_DIR = $(MQTT_DIR)/MQTTClient-C/src
MQTT_EMB_DIR = $(MQTT_DIR)/MQTTPacket/src

MQTT_INCLUDE_DIR =
MQTT_INCLUDE_DIR += -I $(MQTT_EMB_DIR)
MQTT_INCLUDE_DIR += -I $(MQTT_C_DIR)

MQTT_SRC_FILES =
MQTT_SRC_FILES += $(shell find $(MQTT_EMB_DIR)/ -name '*.c')
MQTT_SRC_FILES += $(MQTT_C_DIR)/MQTTClient.c

#TLS - openSSL
TLS_LIB_DIR = /usr/lib/
TLS_INCLUDE_DIR = -I /usr/include/openssl
EXTERNAL_LIBS = -L$(TLS_LIB_DIR)
LD_FLAG := -ldl -lssl -lcrypto
LD_FLAG += -Wl,-rpath,$(TLS_LIB_DIR)

#Aggregate all include and src directories
INCLUDE_ALL_DIRS =
INCLUDE_ALL_DIRS += $(IOT_INCLUDE_DIRS) 
INCLUDE_ALL_DIRS += $(MQTT_INCLUDE_DIR) 
INCLUDE_ALL_DIRS += $(TLS_INCLUDE_DIR)
INCLUDE_ALL_DIRS += $(APP_INCLUDE_DIRS)

SRC_FILES =
SRC_FILES += $(MQTT_SRC_FILES)
SRC_FILES += $(IOT_SRC_FILES)
SRC_FILES += $(CWEBSOCKET_SRC_FILES)
SRC_FILES += $(ECUTOOLS_SRC_FILES)

# Logging level control
LOG_FLAGS = -DIOT_DEBUG
LOG_FLAGS += -DIOT_INFO
LOG_FLAGS += -DIOT_WARN
LOG_FLAGS += -DIOT_ERROR

COMPILER_FLAGS = -g
COMPILER_FLAGS += $(LOG_FLAGS)

# ecutools
COMPILER_FLAGS += -Wall -g3 -pthread -DTHREADED -DUSESSL 
LD_FLAG += -lpthread -lssl -lcurl -ljansson

bin_PROGRAMS = ecutune
ecutune_SOURCES = $(INCLUDE_ALL_DIRS) $(SRC_FILES)

AM_CFLAGS =
AM_LDFLAGS =

if DEBUG
  AM_CFLAGS += -Wall -g3
else
  AM_CFLAGS += -Wall -O3
endif

if THREADED
  AM_CFLAGS += -pthread -DTHREADED
  AM_LDFLAGS += -lpthread
endif

if USESSL
  AM_CFLAGS += -DUSESSL
  AM_LDFLAGS += -lssl
endif

all:
	$(CC) $(INCLUDE_ALL_DIRS) $(SRC_FILES) $(COMPILER_FLAGS) -o $(APP_NAME) $(EXTERNAL_LIBS) $(LD_FLAG)

clean:
	rm -rf compile config.h.in config.h config.cache configure install-sh aclocal.m4 autom4te.cache/ config.log config.status Debug/ depcomp .deps/ m4/ Makefile Makefile.in missing stamp-h1 *.o src/*.o src/.deps/ src/.dirstamp src/cwebsocket/.deps/ src/cwebsocket/.dirstamp *~ ecutune
