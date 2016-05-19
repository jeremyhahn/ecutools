ACLOCAL_AMFLAGS = -I m4

APP_DIR = src
APP_INCLUDE_DIRS = -I$(top_srcdir)/include -I$(APP_DIR)

ECUTOOLS_SRC_FILES = src/canbus.c src/apigateway.c src/awsiot_client.c
ECUTOOLS_SRC_FILES += src/passthru_shadow.c src/passthru_thing.c src/passthru_shadow_parser.c src/passthru_shadow_router.c
ECUTOOLS_SRC_FILES += src/passthru_shadow_connection_handler.c src/passthru_shadow_log_handler.c
ECUTOOLS_SRC_FILES += src/canbus_logger.c src/canbus_log.c src/canbus_filelogger.c src/canbus_awsiotlogger.c

ECUTOOLS_TEST_FILES = tests/check_j2534.c

# AWS IoT client directory
IOT_CLIENT_DIR = src/aws_iot_src
IOT_CLIENT_SRC_DIR = src/aws_iot_src/src

PLATFORM_DIR = $(IOT_CLIENT_DIR)/platform/linux/mbedtls
PLATFORM_COMMON_DIR = $(IOT_CLIENT_DIR)/platform/linux/common

IOT_INCLUDE_DIRS =  -I $(IOT_CLIENT_DIR)/include
IOT_INCLUDE_DIRS += -I $(IOT_CLIENT_DIR)/external_libs/jsmn
IOT_INCLUDE_DIRS += -I $(PLATFORM_COMMON_DIR)
IOT_INCLUDE_DIRS += -I $(PLATFORM_DIR)

IOT_SRC_FILES =  $(IOT_CLIENT_SRC_DIR)/aws_iot_json_utils.c $(IOT_CLIENT_SRC_DIR)/aws_iot_mqtt_client.c $(IOT_CLIENT_SRC_DIR)/aws_iot_mqtt_client_common_internal.c $(IOT_CLIENT_SRC_DIR)/aws_iot_mqtt_client_connect.c
IOT_SRC_FILES += $(IOT_CLIENT_SRC_DIR)/aws_iot_mqtt_client_publish.c $(IOT_CLIENT_SRC_DIR)/aws_iot_mqtt_client_subscribe.c $(IOT_CLIENT_SRC_DIR)/aws_iot_mqtt_client_unsubscribe.c
IOT_SRC_FILES += $(IOT_CLIENT_SRC_DIR)/aws_iot_mqtt_client_yield.c $(IOT_CLIENT_SRC_DIR)/aws_iot_shadow.c $(IOT_CLIENT_SRC_DIR)/aws_iot_shadow_actions.c
IOT_SRC_FILES += $(IOT_CLIENT_SRC_DIR)/aws_iot_shadow_json.c $(IOT_CLIENT_SRC_DIR)/aws_iot_shadow_records.c
IOT_SRC_FILES += $(IOT_CLIENT_DIR)/external_libs/jsmn/jsmn.c
IOT_SRC_FILES += $(PLATFORM_DIR)/network_mbedtls_wrapper.c
IOT_SRC_FILES += $(PLATFORM_COMMON_DIR)/timer.c

#TLS - mbedtls
MBEDTLS_DIR = $(IOT_CLIENT_DIR)/external_libs/mbedTLS
TLS_LIB_DIR = $(MBEDTLS_DIR)/library
TLS_INCLUDE_DIR = -I $(MBEDTLS_DIR)/include
EXTERNAL_LIBS = -L$(TLS_LIB_DIR)
LD_FLAG = -Wl,-rpath,$(TLS_LIB_DIR)
LD_FLAG += -ldl $(TLS_LIB_DIR)/libmbedtls.a $(TLS_LIB_DIR)/libmbedcrypto.a $(TLS_LIB_DIR)/libmbedx509.a

# Aggregate all include and src directories
INCLUDE_ALL_DIRS = $(IOT_INCLUDE_DIRS) 
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
COMPILER_FLAGS += $(LOG_FLAGS)

# ecutools
LD_FLAG += -lpthread -lssl -lcurl -ljansson

AM_CFLAGS = -DUSESSL -DTHREADED $(INCLUDE_ALL_DIRS) $(COMPILER_FLAGS)
AM_LDFLAGS = $(LD_FLAG) $(EXTERNAL_LIBS)

include_HEADERS = src/j2534.h

lib_LTLIBRARIES = libj2534.la
libj2534_la_SOURCES = src/j2534.c src/apigateway.c
libj2534_la_LDFLAGS = $(LD_FLAG) -version-info 0:0:0 -shared

bin_PROGRAMS = ecutuned
ecutuned_SOURCES = $(SRC_FILES) src/ecutuned.c
ecutuned_CFLAGS = -DUSESSL -DTHREADED $(INCLUDE_ALL_DIRS) $(COMPILER_FLAGS) $(LOG_FLAGS)
ecutuned_LDFLAGS = $(LD_FLAG) $(EXTERNAL_LIBS)

TESTS = check_j2534
check_PROGRAMS = check_j2534
check_j2534_SOURCES = $(ECUTOOLS_TEST_FILES)
check_j2534_LDFLAGS = $(LD_FLAG) -lcheck -lj2534

clean:
	rm -rf compile config.h.in config.h config.cache configure install-sh aclocal.m4 autom4te.cache/ config.log config.status Debug/ depcomp .deps/ m4/ Makefile Makefile.in missing stamp-h1 *.o src/*.o src/.deps/ src/.dirstamp config.guess config.sub .libs libj2534.* libtool ar-lib *.lo *~ ltmain.sh ecutuned check_j2534* test-driver test-suite.log COPYING INSTALL /usr/local/lib/libj2534.*
