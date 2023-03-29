#
# This is a project Makefile. It is assumed the directory this Makefile resides in is a
# project subdirectory.
#

PROJECT_NAME := radar
PROJECT_PATH := $(abspath .)
PROJECT_BOARD := evb
export PROJECT_PATH PROJECT_BOARD
#CONFIG_TOOLPREFIX :=

BLECONTROLLER_LIBS := std m0s1 m0s1s
BLECONTROLLER_LIB_DEFAULT := std
BLECONTROLLER_LIB_PREFIX := blecontroller_602_

-include ./proj_config.mk

ifeq ($(origin BL60X_SDK_PATH), undefined)
BL60X_SDK_PATH_GUESS ?= $(shell pwd)
BL60X_SDK_PATH ?= $(BL60X_SDK_PATH_GUESS)/../rd-01_sdk
$(info ****** Please SET BL60X_SDK_PATH ******)
$(info ****** Trying SDK PATH [$(BL60X_SDK_PATH)])
endif

COMPONENTS_NETWORK := sntp dns_server
COMPONENTS_BLSYS   := bltime blfdt blmtd blota bloop loopadc looprt loopset
COMPONENTS_VFS     := romfs
# COMPONENTS_MQTT    := esp_common esp_log tcp_transport http-parser esp_tls esp_mqtt
COMPONENTS_HTTP    := esp_common esp_log tcp_transport http-parser esp_tls esp_http_client


ifeq ($(CONFIG_BLECONTROLLER_LIB),all)
COMPONENTS_BLECONTROLLER := $(addprefix $(BLECONTROLLER_LIB_PREFIX), $(BLECONTROLLER_LIBS))
else
ifeq ($(findstring $(CONFIG_BLECONTROLLER_LIB), $(BLECONTROLLER_LIBS)),)
COMPONENTS_BLECONTROLLER := $(addprefix $(BLECONTROLLER_LIB_PREFIX), $(BLECONTROLLER_LIB_DEFAULT))
else
COMPONENTS_BLECONTROLLER := $(addprefix $(BLECONTROLLER_LIB_PREFIX), $(CONFIG_BLECONTROLLER_LIB))
endif
endif

ifeq ($(CONFIG_BT_TL),1)
COMPONENTS_BLE     := $(COMPONENTS_BLECONTROLLER)
else
COMPONENTS_BLE     := $(COMPONENTS_BLECONTROLLER) blestack
endif

INCLUDE_COMPONENTS += lwip
INCLUDE_COMPONENTS += $(SOC_DRV) $(SOC_DRV)_std
INCLUDE_COMPONENTS += freertos_riscv_ram bl602 bl602_std newlibc wifi wifi_manager wpa_supplicant bl602_os_adapter wifi_hosal hosal mbedtls_lts lwip lwip_dhcpd vfs yloop utils cli netutils blog blog_testc blcrypto_suite
INCLUDE_COMPONENTS += easyflash4 coredump bl_os_adapter rfparam_adapter_tmp
INCLUDE_COMPONENTS += $(COMPONENTS_NETWORK)
INCLUDE_COMPONENTS += $(COMPONENTS_BLSYS)
INCLUDE_COMPONENTS += $(COMPONENTS_VFS)
INCLUDE_COMPONENTS += $(PROJECT_NAME)
INCLUDE_COMPONENTS += $(COMPONENTS_HTTP)

INCLUDE_COMPONENTS += D103 hal_wifi bodysense_lib nmsis axk_tcp_ip axk_factory axk_cfg
# INCLUDE_COMPONENTS += ota

ifeq ($(CONFIG_BT),1)
INCLUDE_COMPONENTS += $(COMPONENTS_BLE)
ifeq ($(CONFIG_BT_MESH),1)
INCLUDE_COMPONENTS += blemesh
endif
endif

EXTRA_COMPONENT_DIRS += $(PROJECT_PATH)/D103
# EXTRA_COMPONENT_DIRS += $(PROJECT_PATH)/ota
EXTRA_COMPONENT_DIRS += $(PROJECT_PATH)/hal_wifi
EXTRA_COMPONENT_DIRS += $(PROJECT_PATH)/bodysense_lib
EXTRA_COMPONENT_DIRS += $(PROJECT_PATH)/axk_tcp_ip
EXTRA_COMPONENT_DIRS += $(PROJECT_PATH)/axk_factory
EXTRA_COMPONENT_DIRS += $(PROJECT_PATH)/axk_cfg

include $(BL60X_SDK_PATH)/make_scripts_riscv/project.mk
