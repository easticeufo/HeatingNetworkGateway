
ROOT_DIR = .

include $(ROOT_DIR)/compile_config

CC := $(TOOL_PREFIX)gcc

LIBS_DIR := $(ROOT_DIR)/libs

INCLUDES := 
INCLUDES += -I$(ROOT_DIR)/include

LIBS := 
#链接静态库
#mqtt相关库
LIBS += $(LIBS_DIR)/libmqtt.a
#appweb相关库
LIBS += $(LIBS_DIR)/libappweb.a
LIBS += $(LIBS_DIR)/libmpr.a
LIBS += $(LIBS_DIR)/libhttp.a
LIBS += $(LIBS_DIR)/libpcre.a
#libupnp相关库
LIBS += $(LIBS_DIR)/libupnp.a
LIBS += $(LIBS_DIR)/libixml.a
LIBS += $(LIBS_DIR)/libthreadutil.a

#链接动态库
LIBS += -lpthread
LIBS += -ldl
LIBS += -lm
LIBS += -lrt

SRCS := 
SRCS += src/main.c
SRCS += src/base_fun.c
SRCS += src/cmd_server.c
SRCS += src/xbee.c
SRCS += src/xbee_interface.c
SRCS += src/trans_server.c
SRCS += src/mqtt.c
SRCS += src/upnp.c
SRCS += src/web_server.c
SRCS += src/cJSON.c
SRCS += src/md5.c

OBJS = $(SRCS:%.c=build/%.o)

CFLAGS += -pipe -Wall -O2 $(INCLUDES)

TARGET = build/HeatingNetworkGateway

all:$(TARGET)

$(TARGET):$(OBJS)
	$(CC) -L$(LIBS_DIR) -o $@ $^ $(LIBS)
	cp $(TARGET) package/HeatingNetworkGateway

$(OBJS):build/%.o:%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	-rm -rf build
