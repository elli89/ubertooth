# Hey Emacs, this is a -*- makefile -*-

# Target file name (without extension).
TARGET = ubertooth

# List C source files here. (C dependencies are automatically generated.)
SRC = $(TARGET).cpp \
	bluetooth.cpp \
	bluetooth_le.cpp \
	ubertooth_usb.cpp \
	ubertooth_rssi.cpp \
	ubertooth_cs.cpp \
	ubertooth_clock.cpp \
	ubertooth_dma.cpp \
	cc2400_rangetest.cpp \
	ego.cpp \
	$(LIBS_PATH)/LPC17xx_Startup.c \
	$(LIBS_PATH)/LPC17xx_Interrupts.c \
	$(LIBS_PATH)/ubertooth.c \
	$(LPCUSB_PATH)/usbcontrol.c \
	$(LPCUSB_PATH)/usbinit.c \
	$(LPCUSB_PATH)/usbhw_lpc.c \
	$(LPCUSB_PATH)/usbstdreq.c

include ../common.mk
