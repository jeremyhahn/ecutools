ACLOCAL_AMFLAGS = -I m4

APP_DIR = src
APP_INCLUDE_DIRS = -I$(top_srcdir)/include -I$(APP_DIR)

ECUTOOLS_SRC_FILES = src/canbus.c src/awsiot_client.c src/canbus_iotbridge.c src/apigateway.c
ECUTOOLS_TEST_FILES = tests/check_j2534.c

# AWS IoT client directory
IOT_CLIENT_DIR = src/aws_iot_src
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
IOT_SRC_FILES += $(PLATFORM_DIR)/network_openssl_wrapper.c $(PLATFORM_DIR)/openssl_hostname_validation.c $(PLATFORM_DIR)/hostname_compare.c $(PLATFORM_DIR)/rawstr.c
IOT_SRC_FILES += $(PLATFORM_COMMON_DIR)/timer.c

# MQTT Paho Embedded C client directory
MQTT_DIR = src/aws_mqtt_embedded_client_lib
MQTT_C_DIR = $(MQTT_DIR)/MQTTClient-C/src
MQTT_EMB_DIR = $(MQTT_DIR)/MQTTPacket/src

MQTT_INCLUDE_DIR =
MQTT_INCLUDE_DIR += -I $(MQTT_EMB_DIR)
MQTT_INCLUDE_DIR += -I $(MQTT_C_DIR)

MQTT_SRC_FILES =
MQTT_SRC_FILES += $(MQTT_EMB_DIR)/MQTTConnectClient.c $(MQTT_EMB_DIR)/MQTTDeserializePublish.c $(MQTT_EMB_DIR)/MQTTPacket.c $(MQTT_EMB_DIR)/MQTTSerializePublish.c $(MQTT_EMB_DIR)/MQTTSubscribeClient.c $(MQTT_EMB_DIR)/MQTTUnsubscribeClient.c
MQTT_SRC_FILES += $(MQTT_C_DIR)/MQTTClient.c

# TLS - openSSL
TLS_LIB_DIR = /usr/lib/
TLS_INCLUDE_DIR = -I /usr/include/openssl
EXTERNAL_LIBS = -L$(TLS_LIB_DIR)
LD_FLAG = -ldl -lssl -lcrypto
LD_FLAG += -Wl,-rpath,$(TLS_LIB_DIR)

# Aggregate all include and src directories
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

# AWS IoT Logging level control
#LOG_FLAGS = -DIOT_DEBUG
#LOG_FLAGS += -DIOT_INFO
#LOG_FLAGS += -DIOT_WARN
#LOG_FLAGS += -DIOT_ERROR

COMPILER_FLAGS = -g3

# ecutools
LD_FLAG += -lm -lpthread -lssl -lcurl -ljansson

AM_CFLAGS = -DUSESSL -DTHREADED $(INCLUDE_ALL_DIRS) $(COMPILER_FLAGS)
AM_LDFLAGS = $(LD_FLAG) $(EXTERNAL_LIBS)

include_HEADERS = src/j2534.h

# J2534 .so
lib_LTLIBRARIES = libj2534.la
libj2534_la_SOURCES = src/j2534.c src/apigateway.c
libj2534_la_LDFLAGS = -version-info 0:0:0 -shared -module $(LD_FLAG)

bin_PROGRAMS = ecutuned
ecutuned_SOURCES = $(SRC_FILES) src/ecutuned.c
ecutuned_CFLAGS = -DUSESSL -DTHREADED $(INCLUDE_ALL_DIRS) $(COMPILER_FLAGS)
ecutuned_LDFLAGS = $(LD_FLAG) $(EXTERNAL_LIBS)

TESTS = check_j2534
check_PROGRAMS = check_j2534
check_j2534_SOURCES = $(ECUTOOLS_TEST_FILES)
check_j2534_LDFLAGS = -lm -lcheck -lrt -lpthread
check_j2534_LDADD = libj2534.la

clean:
	rm -rf compile config.h.in config.h config.cache configure install-sh aclocal.m4 autom4te.cache/ config.log config.status Debug/ depcomp .deps/ m4/ Makefile Makefile.in missing stamp-h1 *.o src/*.o src/.deps/ src/.dirstamp config.guess config.sub .libs libj2534.* libtool ar-lib *.lo *~ ltmain.sh ecutuned check_j2534* test-driver test-suite.log COPYING INSTALL