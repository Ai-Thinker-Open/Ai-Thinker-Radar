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
 * �״�MCU˯�߿��أ�
 * 1 --> MCU���״�NOP����˯��
 * 0 --> MCU������˯��ģʽ
 ***************************************************************/
#define isMCU_SLEEP_MODE                    (0) //BL602����������

/****************************************************************
 * ʹ��MCU�ϵ�ǰN֡ʱ���ڲ�˯��,��Ҫ��MCU˯�߻�����ʹ��
 * 0 --> MCU һֱ����˯��ģʽ
 * 1 --> MCU ���ϵ�ǰN֡ʱ���ڲ�����˯��
 ***************************************************************/
#if (isMCU_SLEEP_MODE == 1)
#define MCU_POWER_UP_NO_SLEEP_EN            (1)
#define MCU_POWER_UP_NO_SLEEP_FRAME_CNT     (30) /* MCU�ϵ粻˯��֡�� */
#endif

/****************************************************************
 * �״�̼��汾�� ������޸�
 ***************************************************************/
#define RADAR_DEV_VER_MAJOR                 0
#define RADAR_DEV_VER_MINOR                 1
#define RADAR_DEV_VER_PATCH                 0

#ifdef __cplusplus
}
#endif

#endif


