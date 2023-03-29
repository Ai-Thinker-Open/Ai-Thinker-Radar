#
# "main" pseudo-component makefile.
#
# (Uses default behaviour of compiling all source files in directory, adding 'include' to include path.)

include $(BL60X_SDK_PATH)/components/network/ble/ble_common.mk

# Component Makefile
#
include $(BL60X_SDK_PATH)/components/network/ble/ble_common.mk

COMPONENT_ADD_INCLUDEDIRS := ./ inc

COMPONENT_OBJS := $(patsubst %.c,%.o, $(COMPONENT_SRCS))

COMPONENT_SRCDIRS := ./ src
