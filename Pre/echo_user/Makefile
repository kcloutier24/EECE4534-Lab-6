TARGET:=echo_user
SRCS:=echo_user.c
OBJS:=$(SRCS:.c=.o)
CFLAGS:= -g
ESL_KERNEL_VERSION ?= 5.10.0-esl
ifeq ($(OECORE_TARGET_SYSROOT),)
$(error "Can't determine SDK path. Verify that you have sourced the environment correctly.")
endif
KERNEL_SRC ?= $(OECORE_TARGET_SYSROOT)/lib/modules/$(ESL_KERNEL_VERSION)/build

ESL_ZEDBOARD_IP?=qemu

PLOAD_PATH ?= /home/root/
ZEDBOARD_IP?=qemu

.PHONY: clean upload

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $@ $^

%.o: %.c %.h
	$(CC) $(CFLAGS) -c $<

%.o: %.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -rf $(OBJS) $(TARGET)

upload: $(TARGET)
	scp $(TARGET) root@$(ESL_ZEDBOARD_IP):$(UPLOAD_PATH)