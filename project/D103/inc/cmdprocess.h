/**
  ******************************************************************************
  * @file           : cmdprocess.h
  * @author         : iclm team
  * @brief          : command process header file
  ******************************************************************************
  */
#ifndef __CMDPROCESS_H__
#define __CMDPROCESS_H__

#ifdef __cplusplus
 extern "C" {
#endif 

#include "stdint.h"
#include "bodysensing_types.h"

typedef enum  
{
	CMD_STATE_HEAD0 = 0,
    CMD_STATE_HEAD1,
    CMD_STATE_HEAD2,
    CMD_STATE_HEAD3,
    CMD_STATE_LEN0,
    CMD_STATE_LEN1,
    CMD_STATE_DATA,
    CMD_STATE_TAIL0,
    CMD_STATE_TAIL1,
    CMD_STATE_TAIL2,
    CMD_STATE_TAIL3
}cmdStateEnum;

typedef enum  
{
	CMD_HEAD_0 = 0,
    CMD_HEAD_1,
    CMD_HEAD_2,
    CMD_HEAD_3,
    CMD_HEAD_MAX
}cmdHeadEnum;

typedef enum  
{
	CMD_TAIL_0 = 0,
    CMD_TAIL_1,
    CMD_TAIL_2,
    CMD_TAIL_3,
    CMD_TAIL_MAX
}cmdTailEnum;

typedef enum  
{
	NOPCONFIG_ENABLE = 0,
    NOPCONFIG_STARTTIME,
    NOPCONFIG_STOPTIME,
	NOPCONFIG_STARTREGISTER,
	NOPCONFIG_STOPREGISTER,
}nopCofigParaEnum;

#ifdef STM32_PLATFORM
#define CMD_LEN_MAX         (1024)
#define CMD_REG_MAX         (500)
#define CMD_PARA_MAX        (200)
#define CMD_DATA_QUEUE_SIZE     (CMD_RECV_BUF_MAX + 2)
#else
#define CMD_LEN_MAX         (64)
#define CMD_REG_MAX         (25)
#define CMD_PARA_MAX        (13)
#define CMD_DATA_QUEUE_SIZE     (2)
#endif


#define CMD_LENGTH_LEN      (2)
#define CMD_TYPE_LEN        (2)
#define CMD_STATUS_LEN      (2)
#define CMD_STR_LEN         (2)
#define CMD_OVERHEAD_LEN    (CMD_HEAD_MAX + CMD_LENGTH_LEN + CMD_TYPE_LEN + CMD_TAIL_MAX)
#define CMD_BUF_LEN         (CMD_LEN_MAX - CMD_HEAD_MAX - CMD_TAIL_MAX - CMD_LENGTH_LEN)
#define CMD_LEN_HIGH_POS    (8)
#define CMD_DATA_POS_OLD    (CMD_HEAD_MAX + CMD_LENGTH_LEN + CMD_TYPE_LEN)
#define CMD_DATA_POS        (CMD_HEAD_MAX + CMD_LENGTH_LEN + CMD_TYPE_LEN + CMD_STATUS_LEN)

#define CMD_DEV_ADDR_LEN    (2)
#define CMD_REG_ADDR_LEN    (2)
#define CMD_PARA_NAME_LEN   (2)

#define START_CFG_CMD       (0x00FF)
#define FINISH_CFG_CMD      (0x00FE)
#define READ_VER_CMD	    (0x0000)
#define WRITE_REG_CMD       (0x0001)
#define READ_REG_CMD        (0x0002)
#define WRITE_MTT_CMD       (0x0003)
#define READ_MTT_CMD        (0x0004)
#define ADC_RESET_CMD       (0x0010)
#define READ_SN_CMD	        (0x0011)
#define WRITE_SYS_CMD       (0x0012)
#define READ_SYS_CMD        (0x0013)
#define CASCADING_MODE_CMD  (0x0014)
#define FFT_ZEROFILL_CMD  	(0x0015)
#define START_I2C_TEST_CMD  (0x0020)
#define STOP_I2C_TEST_CMD   (0x0021)
#define GET_I2C_TEST_RESULT_CMD  (0x0022)
#define NOP_CONFIG_CMD   		(0x0023)
#define ENTER_FACTORYMODE_CMD   (0x0024)
#define EXIT_FACTORYMODE_CMD   	(0x0025)


#define PARAM_CFG_CMD		(0x0060)    //cmd: config algorithm param to flash
#define PARAM_READ_CMD	    (0x0061)    //cmd: read algorithm param from flash or ram
#define ENABLE_DEBUG_CMD	(0x0062)
#define DISABLE_DEBUG_CMD	(0x0063)
#define THRESHOLD_SET_CMD   (0x0064)

// 增加AMPS读取雷达测试结果指令
#define AMPS_READ_RESULT   (0x00A1)


#define CMD_ACK_TYPE        (0x0100)
#define ACK_OK              (0)
#define ACK_FAIL            (1)

#define ACK_OK_OLD              (1)
#define ACK_FAIL_OLD            (0)

#define RESET_DELAY_TIME    (2)
#define SN_LEN              (3)

#define CMD_PROC_STACK_SIZE      (512)

#define _TO_STR(__str) #__str 
#define TO_STR(__str) _TO_STR(__str)// convert to string

#define XEND103_TYPE_VALUE              (0x8005)

typedef struct CMD 
{
	uint16_t cmdType;
	uint16_t cmdData[0];
}CMD_T;

typedef struct CMD_REG 
{
	uint16_t addr;
	uint16_t val;
}CMD_REG_T;

#pragma pack(1)
typedef struct CMD_PARA 
{
	uint16_t type;
	int32_t val;
}CMD_PARA_T;
#pragma pack()

#pragma pack(4)
typedef struct ALGORITHM_PARAM 
{
    uint32_t nMaxMotionRangeBin;
    uint32_t nMaxMotionLessRangeBin;
    uint32_t nThresholdValOfMotion[ONEFFT_POINT-1 ];
    uint32_t nThresholdValOfMotionLess[ONEFFT_POINT-1 ];
    uint32_t nOffTime;
}ALGORITHM_PARAM_T;
#pragma pack()

typedef struct CMD_PARSE
{
    uint8_t buf[CMD_BUF_LEN];
    uint8_t state;
    uint16_t curIndex;
    uint16_t len;
}CMD_PARSE_T;

#pragma pack(1)
typedef struct FACTORY_PARA
{
    uint16_t boardType;
    uint16_t chipCount;	
    uint16_t channelCount;	
    uint16_t dataType;	
    uint16_t fftSize;	
    uint16_t chirpCount;	
	uint16_t downSample;
}FACTORY_PARA_T;
#pragma pack()

extern ALGORITHM_PARAM_T gAlgorithmParam;
void CmdProc_Recv(void);
void CmdProc_Init(void);
uint8_t CmdProc_InCmdMode(void);
void CmdProc_NopConfig(void);
void DoExitWakeUpACK(void);
void CmdProc_InCmdModeClear(void);

#ifdef __cplusplus
}
#endif

#endif


