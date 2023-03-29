/**
  ******************************************************************************
  * @file    global_conf.h
  * @author  iclm team
  * @brief   global config for whole project
  ******************************************************************************
  */
#ifndef __GLOBAL_CONF_H__
#define __GLOBAL_CONF_H__

#ifdef __cplusplus
 extern "C" {
#endif

#include "global_def.h"

/**********function switch*************/
#ifdef EVBSN01
#define SUPPORT_DYNAMIC_SYS_MODE

#ifdef SUPPORT_DYNAMIC_SYS_MODE
#define SYS_MODE_DEFAULT          SYS_MODE_PASSTHROUGH
#else
#define SUPPORT_DATA_PASSTHROUGH
//#define SUPPORT_MOTION_TARGET_TRACKING
//#define SUPPORT_VITAL_SIGN
//#define SUPPORT_GESTURE_RECOGNITION
#endif
#endif

#if defined(DEVMB)
#define SUPPORT_DYNAMIC_SYS_MODE
#define SUPPORT_2TX_4RX_BOARD_DEVMB

#ifdef SUPPORT_DYNAMIC_SYS_MODE
#define SYS_MODE_DEFAULT          SYS_MODE_PASSTHROUGH
#else
#define SUPPORT_DATA_PASSTHROUGH
//#define SUPPORT_MOTION_TARGET_TRACKING
//#define SUPPORT_VITAL_SIGN
//#define SUPPORT_GESTURE_RECOGNITION
#endif
#endif

#if defined(DEVMB_PLUS)
#define SUPPORT_DYNAMIC_SYS_MODE
#define SUPPORT_2TX_4RX_BOARD_DEVMB_PLUS

#ifdef SUPPORT_DYNAMIC_SYS_MODE
#define SYS_MODE_DEFAULT          SYS_MODE_PASSTHROUGH
#else
#define SUPPORT_DATA_PASSTHROUGH
//#define SUPPORT_MOTION_TARGET_TRACKING
//#define SUPPORT_VITAL_SIGN
//#define SUPPORT_GESTURE_RECOGNITION
#endif
#endif

#ifdef ISK1101
#define SUPPORT_DATA_PASSTHROUGH
#endif

#ifdef XenD103
//#define SUPPORT_DATA_PASSTHROUGH
#endif

/**********para config****************/
#ifdef EVBSN01
#define UPLOAD_SAMPLE_RATE        (1)
#define RADAR_DATA_MAX_LEN        (4096)
#endif

#if defined(DEVMB) || defined(DEVMB_PLUS)
#define UPLOAD_SAMPLE_RATE        (1)
#define RADAR_DATA_MAX_LEN        (4096)
#endif

#ifdef ISK1101
#define UPLOAD_SAMPLE_RATE        (16)
#define RADAR_DATA_MAX_LEN        (1024)
#endif

#ifdef XenD103
#define UPLOAD_SAMPLE_RATE        (16)
#define RADAR_DATA_MAX_LEN        (40)
#endif

#ifdef TEAK
#define UPLOAD_SAMPLE_RATE        (1)
#define RADAR_DATA_MAX_LEN        (4096)
#endif

#define DEBUG_MODE_DEFAULT        DEBUG_MODE_OFF
#define SEND_DATA_SAMPLE_RATE     (8)

/**********debug**********************/
//#define CONFIG_DEBUG

/****************************************************************
 * 雷达MCU睡眠开关，
 * 1 --> MCU在雷达NOP器件睡眠
 * 0 --> MCU不进入睡眠模式
 ***************************************************************/
#define isMCU_SLEEP_MODE                    (0) //BL602不进入休眠

/****************************************************************
 * 使能MCU上电前N帧时间内不睡眠,需要在MCU睡眠机制下使能
 * 0 --> MCU 一直进入睡眠模式
 * 1 --> MCU 在上电前N帧时间内不进入睡眠
 ***************************************************************/
#if (isMCU_SLEEP_MODE == 1)
#define MCU_POWER_UP_NO_SLEEP_EN            (1)
#define MCU_POWER_UP_NO_SLEEP_FRAME_CNT     (30) /* MCU上电不睡眠帧数 */
#endif

/****************************************************************
 * 雷达固件版本， 请谨慎修改
 ***************************************************************/
#define RADAR_DEV_VER_MAJOR                 0
#define RADAR_DEV_VER_MINOR                 1
#define RADAR_DEV_VER_PATCH                 0

#ifdef __cplusplus
}
#endif

#endif


