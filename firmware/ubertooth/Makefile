# Hey Emacs, this is a -*- makefile -*-
#----------------------------------------------------------------------------
# WinAVR Makefile Template written by Eric B. Weddington, Jörg Wunsch, et al.
#  >> Modified for use with the LUFA project. <<
#
# Released to the Public Domain
#
# Altered for ARM Cortex-M3 by Opendous Inc. - 2010-01-05
# Altered for Project Ubertooth by Michael Ossmann 2010
#
# Additional material for this makefile was written by:
# Peter Fleury
# Tim Henigan
# Colin O'Flynn
# Reiner Patommel
# Markus Pfaff
# Sander Pool
# Frederik Rouleau
# Carlos Lamas
# Dean Camera
# Denver Gingerich
# Opendous Inc.
# Michael Ossmann
# Jared Boone
#
#----------------------------------------------------------------------------
# On command line:
#
# make all = Make software.
#
# make clean = Clean out built project files.
#
# make program = Download the hex file to the device
#
# make doxygen = Generate DoxyGen documentation for the project (must have
#                DoxyGen installed)
#
# make filename.s = Just compile filename.c into the assembler code only.
#
# make filename.i = Create a preprocessed source file for use in submitting
#                   bug reports to the GCC project.
#
# To rebuild project do "make clean" then "make all".
#----------------------------------------------------------------------------

# Target file name (without extension).
TARGET = ubertooth

# List C source files here. (C dependencies are automatically generated.)
SRC = $(LIBS_PATH)/LPC17xx_Startup.c \
      $(LIBS_PATH)/LPC17xx_Interrupts.c \
      $(LIBS_PATH)/ubertooth.c \
      $(LPCUSB_PATH)/usbcontrol.c \
      $(LPCUSB_PATH)/usbinit.c \
      $(LPCUSB_PATH)/usbhw_lpc.c \
      $(LPCUSB_PATH)/usbstdreq.c

CPPSRC = $(TARGET).cpp \
         bluetooth.cpp \
         bluetooth_le.cpp \
         ubertooth_usb.cpp \
         ubertooth_rssi.cpp \
         ubertooth_cs.cpp \
         ubertooth_clock.cpp \
         ubertooth_dma.cpp \
         cc2400_rangetest.cpp \
         ego.cpp

include ../common.mk
