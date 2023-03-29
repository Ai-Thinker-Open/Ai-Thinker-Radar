/**
  ******************************************************************************
  * @file           : cmdprocess.c
  * @author         : iclm team
  * @brief          : command process module
  ******************************************************************************
  */
#include <stdio.h>
#include <string.h>
#include "global_conf.h"
#include "platform.h"
#include "cmdprocess.h"
#include "banyan.h"
#include "config.h"
#include "utilities.h"
#include "system.h"
#include "AlgorithmConfig.h"
#include "bodysensing_types.h"
#include "mcu_sleep.h"
#include "ft_system.h"
#include "bsp_nvs.h"

#include "bsp_iic.h"
#include "bsp_uart.h"
#include <FreeRTOS.h>
#include <timers.h>

#define FACTORY_RESULT_KEY  "ft_result"

//XXX防止编译报错
#define GD32_PLATFORM

#define VER_STR() _TO_STR(v)TO_STR(RADAR_DEV_VER_MAJOR)"." TO_STR(RADAR_DEV_VER_MINOR)"." TO_STR(RADAR_DEV_VER_PATCH)

static CMD_PARSE_T CmdDataParse;
static uint8_t CmdModeFlag = 0;

static uint8_t     DebugModeFlag = 0;

static uint8_t CmdHead[CMD_HEAD_MAX] = {0xFD, 0xFC, 0xFB, 0xFA};
static uint8_t CmdTail[CMD_TAIL_MAX] = {0x04, 0x03, 0x02, 0x01};
static uint8_t CmdAck[CMD_LEN_MAX] __ALIGN(4) = {0};
const uint16_t      read_ver_ack[5] = {XEND103_TYPE_VALUE, 0x00, RADAR_DEV_VER_MAJOR,
                                                                 RADAR_DEV_VER_MINOR,
                                                                 RADAR_DEV_VER_PATCH};
static uint16_t masterProtocolVer = 0;

static uint8_t enableNopConfig = 0;
static uint8_t nopConfigStartRegisterIndex = 0;
static uint8_t nopConfigStopRegisterIndex = 0;
static uint16_t nopConfigStartMs = RESET_DELAY_TIME;
static uint16_t nopConfigStopMs = RESET_DELAY_TIME + 2;
int32_t nopConfigStartRegister[16] = {0};
int32_t nopConfigStopRegister[16] = {0};
static uint16_t     start_cfg_ack[2] = {0};

ALGORITHM_PARAM_T gAlgorithmParam;

void CmdProc_NopConfig(void)
{
    if (enableNopConfig 
		&& (nopConfigStopMs > nopConfigStartMs)
		&& CmdModeFlag == 0)
    {     
		uint16_t loop = 0;

		Delay(nopConfigStartMs);

		for(loop = 0; loop < nopConfigStartRegisterIndex; loop++)
		{
			int32_t val = nopConfigStartRegister[loop];
			CMD_REG_T *cmdReg = (CMD_REG_T *)(&val);
            bsp_iic_write(I2C_ADDR_BanYan_Chip0, cmdReg->addr, cmdReg->val);
			if(g_TxCount > 1)
			{
                bsp_iic_write(I2C_ADDR_BanYan_Chip1, cmdReg->addr, cmdReg->val);
			}
			
		}

		Delay(nopConfigStartMs);

		for(loop = 0; loop < nopConfigStopRegisterIndex; loop++)
		{
			int32_t val = nopConfigStopRegister[loop];
			CMD_REG_T *cmdReg = (CMD_REG_T *)(&val);

            bsp_iic_write(I2C_ADDR_BanYan_Chip0, cmdReg->addr, cmdReg->val);

			if(g_TxCount > 1)
			{
                bsp_iic_write(I2C_ADDR_BanYan_Chip1, cmdReg->addr, cmdReg->val);
			}
		}
    }
}


static uint16_t FillCmdAck(const uint16_t *data, uint16_t dataLen, uint16_t cmdType, uint16_t status)
{
    uint16_t index = 0;

    if (dataLen * sizeof(uint16_t) > (CMD_LEN_MAX - CMD_OVERHEAD_LEN))
    {
        return 0;
    }
    
	memcpy(&CmdAck[index], CmdHead, sizeof(CmdHead));
    index += sizeof(CmdHead);

    if (data == NULL)
    {
        *((uint16_t*)(&CmdAck[index])) = CMD_TYPE_LEN + CMD_STATUS_LEN;
    }
	else if(masterProtocolVer != 0)
    {
        #if (RADAR_PROTOCOL_VERSION == 1)
        /* 协议版本1, 协议长度 = command length + ack length + data length */
        *((uint16_t*)(&CmdAck[index])) = CMD_TYPE_LEN + CMD_STATUS_LEN + (dataLen * sizeof(uint16_t));
        #elif (RADAR_PROTOCOL_VERSION == 2)
        /* 协议版本2 */
        /* Read version command */
        if(READ_VER_CMD == cmdType)
        {
            /* 协议长度 = command length + ack length + command string length + data length word legnth */
            *((uint16_t*)(&CmdAck[index])) = CMD_TYPE_LEN + CMD_STATUS_LEN + CMD_STR_LEN + dataLen;
        }
        else
        {
            /* 协议长度 = command length + ack length + data length */
            *((uint16_t*)(&CmdAck[index])) = CMD_TYPE_LEN + CMD_STATUS_LEN + (dataLen * sizeof(uint16_t));
        }
        #endif
    }
    else
    {
        *((uint16_t*)(&CmdAck[index])) = CMD_TYPE_LEN + (dataLen * sizeof(uint16_t));
    }
    index += sizeof(uint16_t);
    
    *((uint16_t*)(&CmdAck[index])) = cmdType | CMD_ACK_TYPE;
    index += sizeof(uint16_t);
    
	if(masterProtocolVer != 0 || data == NULL)
	{
		if(masterProtocolVer == 0)
		{
			status = (status == ACK_OK) ? ACK_OK_OLD : ACK_FAIL_OLD;
		}
		*((uint16_t*)(&CmdAck[index])) = status;
		index += sizeof(uint16_t);
	}
	
    if (data != NULL)
    {
        if((RADAR_PROTOCOL_VERSION == 2) && (READ_VER_CMD == cmdType))
        {
            *((uint16_t*)(&CmdAck[index])) = dataLen;
            index += sizeof(uint16_t);
            memcpy(&CmdAck[index], data, dataLen);
            index += dataLen;
        }
        else
        {
            memcpy(&CmdAck[index], data, dataLen*sizeof(uint16_t));
            index += dataLen * sizeof(uint16_t);
        } 
    }

	memcpy(&CmdAck[index], CmdTail, sizeof(CmdTail));
    index += sizeof(CmdTail);
    
	return index;
}

static uint16_t DoWriteReg(CMD_T *cmd, uint32_t cmdLen)
{
    uint16_t ackLen = 0;
    uint16_t status = ACK_OK;
    uint16_t regNum = 0;
    uint16_t loop = 0;
    uint16_t regVal = 0;
	uint16_t devAddress = I2C_ADDR_BanYan_Chip0;
    CMD_REG_T *cmdReg = NULL;

    if (NULL == cmd || 0 == cmdLen)
    {
        return 0;
    }
        
	if (CmdModeFlag) 
    {
		devAddress = cmd->cmdData[0] << 1;
		regNum = (cmdLen - CMD_TYPE_LEN - CMD_DEV_ADDR_LEN) / sizeof(CMD_REG_T);
		for (loop = 0; loop < regNum; loop++) 
        {
            cmdReg = (CMD_REG_T*)((uint8_t*)(cmd->cmdData) + CMD_DEV_ADDR_LEN + (loop * sizeof(CMD_REG_T)));
            bsp_iic_write(devAddress, (uint8_t)(cmdReg->addr), cmdReg->val);
			if(devAddress == I2C_ADDR_BanYan_Chip0)
			{
                //update master register
				Radar_UpdateReg(cmdReg->addr, cmdReg->val);
			}

            bsp_iic_read(devAddress, (uint8_t)(cmdReg->addr), &regVal);
			if (regVal != cmdReg->val) 
            {
				status = ACK_FAIL;
			}
		}
	} 
    else 
    {
		status = ACK_FAIL;
	}
    
    ackLen = FillCmdAck(NULL, 0, cmd->cmdType, status);
    
    return ackLen;
}

static uint16_t DoReadReg(CMD_T *cmd, uint32_t cmdLen)
{
    uint16_t ackLen = 0;
    uint16_t status = ACK_OK;
    uint16_t regNum = 0;
    uint16_t loop = 0;
    uint16_t *readBuf = NULL;
	uint16_t devAddress = I2C_ADDR_BanYan_Chip0;
	
    if (NULL == cmd || 0 == cmdLen)
    {
        return 0;
    }
        
    if (CmdModeFlag) 
    {
		devAddress = cmd->cmdData[0] << 1;
        regNum = (cmdLen - CMD_TYPE_LEN - CMD_DEV_ADDR_LEN) / CMD_REG_ADDR_LEN;
        
        if (regNum > CMD_REG_MAX)
        {
			regNum = 0;
            status = ACK_FAIL;
        }
        else
        {
			uint16_t dataIndex = (masterProtocolVer != 0) ? CMD_DATA_POS : CMD_DATA_POS_OLD;
            readBuf = (uint16_t*)(&CmdAck[dataIndex]);
            for (loop = 0; loop < regNum; loop++) 
            {
				uint16_t regVal = 0;
                bsp_iic_read(devAddress, (uint8_t)(cmd->cmdData[loop + 1]), &regVal);
				readBuf[loop] = regVal;
            }
        }   
    } 
    else 
    {
        status = ACK_FAIL;
    }
    
    ackLen = FillCmdAck(readBuf, regNum, cmd->cmdType, status);

    return ackLen;
}

int8_t MTT_ParaUpdate(uint16_t type, int32_t val)
{
    return -1;
}

int8_t fftzerofill_ParaUpdate(uint16_t type, int32_t val)
{
    return -1;
}

/*************************************************

Function: InitAlgorithmParam

Description: Init all algorithm para,
             first read paras from Flash,
             if there no para in the flash,
             then config them for default
Input: none
Output: none
Return: none
*************************************************/
static void InitAlgorithmParam(void)
{

    /* Read old paras from flash, if there no paras, then set them default */
    if (Algo_ReadParameterExistFlag() != 0xffffffff)    // 读取flash判断是否有保存值
    {
        // 读取数据
        printf("read nv parameter\r\n");
        Algo_ReadParameter((uint32_t *)&gAlgorithmParam, sizeof(ALGORITHM_PARAM_T));
        printf("gAlgorithmParam.nMaxMotionLessRangeBin=%d\r\n", gAlgorithmParam.nMaxMotionLessRangeBin);
        printf("ggAlgorithmParam.nMaxMotionRangeBin=%d\r\n", gAlgorithmParam.nMaxMotionRangeBin);
        printf("gAlgorithmParam.nThresholdValOfMotion[0]=%d\r\n", gAlgorithmParam.nThresholdValOfMotion[0]);
        printf("gAlgorithmParam.nThresholdValOfMotion[1]=%d\r\n", gAlgorithmParam.nThresholdValOfMotion[1]);
        printf("gAlgorithmParam.nThresholdValOfMotion[2]=%d\r\n", gAlgorithmParam.nThresholdValOfMotion[2]);
        printf("gAlgorithmParam.nThresholdValOfMotionLess[2]=%d\r\n", gAlgorithmParam.nThresholdValOfMotionLess[2]);
        printf("gAlgorithmParam.nThresholdValOfMotion[3]=%d\r\n", gAlgorithmParam.nThresholdValOfMotion[3]);
        printf("gAlgorithmParam.nThresholdValOfMotionLess[3]=%d\r\n", gAlgorithmParam.nThresholdValOfMotionLess[3]);
        printf("gAlgorithmParam.nThresholdValOfMotion[4]=%d\r\n", gAlgorithmParam.nThresholdValOfMotion[4]);
        printf("gAlgorithmParam.nThresholdValOfMotionLess[4]=%d\r\n", gAlgorithmParam.nThresholdValOfMotionLess[4]);
        printf("gAlgorithmParam.nThresholdValOfMotion[5]=%d\r\n", gAlgorithmParam.nThresholdValOfMotion[5]);
        printf("gAlgorithmParam.nThresholdValOfMotionLess[5]=%d\r\n", gAlgorithmParam.nThresholdValOfMotionLess[5]);
        printf("gAlgorithmParam.nThresholdValOfMotion[6]=%d\r\n", gAlgorithmParam.nThresholdValOfMotion[6]);
        printf("gAlgorithmParam.nThresholdValOfMotionLess[6]=%d\r\n", gAlgorithmParam.nThresholdValOfMotionLess[6]);
        printf("gAlgorithmParam.nThresholdValOfMotion[7]=%d\r\n", gAlgorithmParam.nThresholdValOfMotion[7]);
        printf("gAlgorithmParam.nThresholdValOfMotionLess[7]=%d\r\n", gAlgorithmParam.nThresholdValOfMotionLess[7]);
        printf("gAlgorithmParam.nThresholdValOfMotion[8]=%d\r\n", gAlgorithmParam.nThresholdValOfMotion[8]);
        printf("gAlgorithmParam.nThresholdValOfMotionLess[8]=%d\r\n", gAlgorithmParam.nThresholdValOfMotionLess[8]);
        printf("gAlgorithmParam.nOffTime=%d\r\n", gAlgorithmParam.nOffTime);
    }
    else
    {
        printf("write init nv parameter\r\n");
        // 赋初值然后保存flash
        gAlgorithmParam.nMaxMotionLessRangeBin = PARAM_MOTION_MAX ;
        gAlgorithmParam.nMaxMotionRangeBin = PARAM_MOTIONLESS_MAX ;

        gAlgorithmParam.nThresholdValOfMotion[0] = PARAM_MOTION_SENSITIBITY_RANG0;
//        gAlgorithmParam.nThresholdValOfMotionLess[0] = PARAM_MOTIONLESS_SENSITIBITY_RANG0;

        gAlgorithmParam.nThresholdValOfMotion[1] = PARAM_MOTION_SENSITIBITY_RANG1;
//        gAlgorithmParam.nThresholdValOfMotionLess[1] = PARAM_MOTIONLESS_SENSITIBITY_RANG1;

        gAlgorithmParam.nThresholdValOfMotion[2] = PARAM_MOTION_SENSITIBITY_RANG2;
        gAlgorithmParam.nThresholdValOfMotionLess[2] = PARAM_MOTIONLESS_SENSITIBITY_RANG2;

        gAlgorithmParam.nThresholdValOfMotion[3] = PARAM_MOTION_SENSITIBITY_RANG3;
        gAlgorithmParam.nThresholdValOfMotionLess[3] = PARAM_MOTIONLESS_SENSITIBITY_RANG3;

        gAlgorithmParam.nThresholdValOfMotion[4] = PARAM_MOTION_SENSITIBITY_RANG4;
        gAlgorithmParam.nThresholdValOfMotionLess[4] = PARAM_MOTIONLESS_SENSITIBITY_RANG4;

        gAlgorithmParam.nThresholdValOfMotion[5] = PARAM_MOTION_SENSITIBITY_RANG5;
        gAlgorithmParam.nThresholdValOfMotionLess[5] = PARAM_MOTIONLESS_SENSITIBITY_RANG5;

        gAlgorithmParam.nThresholdValOfMotion[6] = PARAM_MOTION_SENSITIBITY_RANG6;
        gAlgorithmParam.nThresholdValOfMotionLess[6] = PARAM_MOTIONLESS_SENSITIBITY_RANG6;
		
		gAlgorithmParam.nThresholdValOfMotion[6] = PARAM_MOTION_SENSITIBITY_RANG6;
        gAlgorithmParam.nThresholdValOfMotionLess[6] = PARAM_MOTIONLESS_SENSITIBITY_RANG6;
		
		gAlgorithmParam.nThresholdValOfMotion[7] = PARAM_MOTION_SENSITIBITY_RANG7;
        gAlgorithmParam.nThresholdValOfMotionLess[7] = PARAM_MOTIONLESS_SENSITIBITY_RANG7;
		
		gAlgorithmParam.nThresholdValOfMotion[8] = PARAM_MOTION_SENSITIBITY_RANG8;
        gAlgorithmParam.nThresholdValOfMotionLess[8] = PARAM_MOTIONLESS_SENSITIBITY_RANG8;
		
        gAlgorithmParam.nOffTime = PARAM_OFF_TIME;

        printf("gAlgorithmParam.nMaxMotionLessRangeBin=%d\r\n", gAlgorithmParam.nMaxMotionLessRangeBin);
        printf("ggAlgorithmParam.nMaxMotionRangeBin=%d\r\n", gAlgorithmParam.nMaxMotionRangeBin);
        printf("gAlgorithmParam.nThresholdValOfMotion[0]=%d\r\n", gAlgorithmParam.nThresholdValOfMotion[0]);
        printf("gAlgorithmParam.nThresholdValOfMotion[1]=%d\r\n", gAlgorithmParam.nThresholdValOfMotion[1]);
        printf("gAlgorithmParam.nThresholdValOfMotion[2]=%d\r\n", gAlgorithmParam.nThresholdValOfMotion[2]);
        printf("gAlgorithmParam.nThresholdValOfMotionLess[2]=%d\r\n", gAlgorithmParam.nThresholdValOfMotionLess[2]);
        printf("gAlgorithmParam.nThresholdValOfMotion[3]=%d\r\n", gAlgorithmParam.nThresholdValOfMotion[3]);
        printf("gAlgorithmParam.nThresholdValOfMotionLess[3]=%d\r\n", gAlgorithmParam.nThresholdValOfMotionLess[3]);
        printf("gAlgorithmParam.nThresholdValOfMotion[4]=%d\r\n", gAlgorithmParam.nThresholdValOfMotion[4]);
        printf("gAlgorithmParam.nThresholdValOfMotionLess[4]=%d\r\n", gAlgorithmParam.nThresholdValOfMotionLess[4]);
        printf("gAlgorithmParam.nThresholdValOfMotion[5]=%d\r\n", gAlgorithmParam.nThresholdValOfMotion[5]);
        printf("gAlgorithmParam.nThresholdValOfMotionLess[5]=%d\r\n", gAlgorithmParam.nThresholdValOfMotionLess[5]);
        printf("gAlgorithmParam.nThresholdValOfMotion[6]=%d\r\n", gAlgorithmParam.nThresholdValOfMotion[6]);
        printf("gAlgorithmParam.nThresholdValOfMotionLess[6]=%d\r\n", gAlgorithmParam.nThresholdValOfMotionLess[6]);
        printf("gAlgorithmParam.nThresholdValOfMotion[7]=%d\r\n", gAlgorithmParam.nThresholdValOfMotion[7]);
        printf("gAlgorithmParam.nThresholdValOfMotionLess[7]=%d\r\n", gAlgorithmParam.nThresholdValOfMotionLess[7]);
        printf("gAlgorithmParam.nThresholdValOfMotion[8]=%d\r\n", gAlgorithmParam.nThresholdValOfMotion[8]);
        printf("gAlgorithmParam.nThresholdValOfMotionLess[8]=%d\r\n", gAlgorithmParam.nThresholdValOfMotionLess[8]);
        printf("gAlgorithmParam.nOffTime=%d\r\n", gAlgorithmParam.nOffTime);

        Algo_SaveParameter((uint32_t *)&gAlgorithmParam, sizeof(ALGORITHM_PARAM_T));
    }
}

int8_t NopCofig_ParaUpdate(uint16_t type, int32_t val)
{
    switch (type)  
    {
		case NOPCONFIG_ENABLE:
			enableNopConfig = (int8_t)val;
			nopConfigStartRegisterIndex = 0;
			nopConfigStopRegisterIndex = 0;
			//printf("enablenopreg:%d\r\n", enableNopConfig);
			break;
		case NOPCONFIG_STARTTIME:
			nopConfigStartMs = (uint16_t)val;
			//printf("starttime:%d\r\n", nopConfigStartMs);
			break;
		case NOPCONFIG_STOPTIME:
			nopConfigStopMs = (uint16_t)val;
			//printf("stoptime:%d\r\n", nopConfigStopMs);
			break;
		case NOPCONFIG_STARTREGISTER:
			nopConfigStartRegister[nopConfigStartRegisterIndex++] = val;
			//printf("NOPCONFIG_STARTREGISTER:%x\r\n", val);
			break;
		case NOPCONFIG_STOPREGISTER:
			nopConfigStopRegister[nopConfigStopRegisterIndex++] = val;
			//printf("NOPCONFIG_STOPREGISTER:%x\r\n", val);
			break;        
        default:
            return -1;
    }
    return 0;
}

static uint16_t DoWritePara(CMD_T *cmd, uint32_t cmdLen)
{
    uint16_t ackLen = 0;
    uint16_t status = ACK_OK;
    uint16_t paraNum = 0;
    uint16_t loop = 0;
    CMD_PARA_T *cmdPara = NULL;
    int8_t ret = 0;

    if (NULL == cmd || 0 == cmdLen)
    {
        return 0;
    }
        
	if (CmdModeFlag) 
    {
		paraNum = (cmdLen - CMD_TYPE_LEN) / sizeof(CMD_PARA_T);
		for (loop = 0; loop < paraNum; loop++) 
        {
            cmdPara = (CMD_PARA_T*)((uint8_t*)(cmd->cmdData) + (loop * sizeof(CMD_PARA_T)));
            switch (cmd->cmdType)
            {
                case WRITE_MTT_CMD: 
                    ret = MTT_ParaUpdate(cmdPara->type, cmdPara->val);
                    break;

                case WRITE_SYS_CMD: 				
                    ret = System_ParaUpdate(cmdPara->type, cmdPara->val);
                    break;
                case FFT_ZEROFILL_CMD:
					ret = fftzerofill_ParaUpdate(cmdPara->type, cmdPara->val);
					break;
				case NOP_CONFIG_CMD:
					ret = NopCofig_ParaUpdate(cmdPara->type, cmdPara->val);
					break;
                default:
                    ret = -1;
                    break;
            }
            if (ret)
            {
                status = ACK_FAIL;
            }
			else
			{
				Config_NeedFlashWrite();
			}
		}
	} 
    else 
    {
		status = ACK_FAIL;
	}
    
    ackLen = FillCmdAck(NULL, 0, cmd->cmdType, status);
    
    return ackLen;
}

int32_t MTT_ParaRead(uint16_t type)
{
    return 0x7fffffff; /*invalid value*/
}

static uint16_t DoReadPara(CMD_T *cmd, uint32_t cmdLen)
{
    uint16_t ackLen = 0;
    uint16_t status = ACK_OK;
    uint16_t paraNum = 0;
    uint16_t loop = 0;
    uint32_t *readBuf = NULL;

    if (NULL == cmd || 0 == cmdLen)
    {
        return 0;
    }
        
    if (CmdModeFlag) 
    {
        paraNum = (cmdLen - CMD_TYPE_LEN) / CMD_PARA_NAME_LEN;
        
        if (paraNum > CMD_PARA_MAX)
        {
			paraNum = 0;
            status = ACK_FAIL;
        }
        else
        {
			uint16_t dataIndex = (masterProtocolVer != 0) ? CMD_DATA_POS : CMD_DATA_POS_OLD;
            readBuf = (uint32_t*)(&CmdAck[dataIndex]);
            for (loop = 0; loop < paraNum; loop++) 
            {
                switch (cmd->cmdType)
                {
                    case READ_MTT_CMD:
                        readBuf[loop] = MTT_ParaRead(cmd->cmdData[loop]);
                        break;
                    
                    case READ_SYS_CMD:
                        readBuf[loop] = System_ParaRead(cmd->cmdData[loop]);
                        break;
                    
                    default:
                        break;
                }
            }
        }   
    } 
    else 
    {
        status = ACK_FAIL;
    }
    
    ackLen = FillCmdAck((uint16_t*)readBuf, paraNum*2, cmd->cmdType, status);
    
    return ackLen;
}

static uint16_t DoReadSn(CMD_T *cmd)
{
    uint16_t ackLen = 0;
    uint16_t status = ACK_OK;
    uint16_t sn[SN_LEN] = {0};

    if (NULL == cmd)
    {
        return 0;
    }
        
    if (CmdModeFlag) 
    {   
        sn[0] = RADAR_DEV_MODEL_TYPE_SN;
        *(uint32_t*)(&sn[1]) = Config_GetSN();      
    } 
    else 
    {
        status = ACK_FAIL;
    }
    
    ackLen = FillCmdAck(sn, sizeof(sn), cmd->cmdType, status);
    return ackLen;
}

static uint16_t DoCascadingMode(CMD_T *cmd, uint32_t cmdLen)
{
    uint16_t ackLen = 0;
    uint16_t status = ACK_OK;

    if (NULL == cmd || 0 == cmdLen)
    {
        return 0;
    }

    ackLen = FillCmdAck(NULL, 0, cmd->cmdType, status);

    return ackLen;
}

static uint16_t StartI2CTest(CMD_T *cmd, uint32_t cmdLen)
{
    uint16_t ackLen = 0;
    uint16_t status = ACK_OK;
	
    if (NULL == cmd || 0 == cmdLen)
    {
        return 0;
    }

    ackLen = FillCmdAck(NULL, 0, cmd->cmdType, status);
    return ackLen;
}

static uint16_t StopI2CTest(CMD_T *cmd, uint32_t cmdLen)
{
	uint16_t ackLen = 0;
    uint16_t status = ACK_OK;

    ackLen = FillCmdAck(NULL, 0, cmd->cmdType, status);
	return ackLen;	
}

static uint16_t GetI2CTestResult(CMD_T *cmd, uint32_t cmdLen)
{
	uint16_t ackLen = 0;
	uint16_t status = ACK_OK;
	uint8_t buf[10] = {0};

	ackLen = FillCmdAck((uint16_t*)buf, 5, cmd->cmdType, status);
	return ackLen;
}

static uint16_t DoEnterFactoryMode(CMD_T *cmd, uint32_t cmdLen)
{
	uint16_t ackLen = 0;
	FACTORY_PARA_T factory;
	factory.boardType = 0;
	factory.chipCount = g_TxCount;
	factory.channelCount = g_ChannelCount;
	factory.dataType = Radar_GetDataType();
	factory.fftSize = Radar_GetFftPoint();
	factory.chirpCount = Radar_GetOneFrameChirpNum();
	factory.downSample = System_GetUploadSampleRate();
	
	
	ackLen = FillCmdAck((uint16_t*)&factory, sizeof(FACTORY_PARA_T) / sizeof(uint16_t), cmd->cmdType, ACK_OK);
    return ackLen;
}

static uint16_t DoExitFactoryMode(CMD_T *cmd, uint32_t cmdLen)
{
	uint16_t ackLen = 0;
	ackLen = FillCmdAck(NULL, 0, cmd->cmdType, ACK_OK);
    return ackLen;
}

/*************************************************
Function: DoConfigurePara

Description: config para from PC
Input: cmd, cmd data; cmdLen cmd length
Output: none
Return: ack length
*************************************************/
static uint16_t DoConfigurePara(CMD_T *cmd, uint32_t cmdLen)
{
    uint16_t ackLen = 0;
    uint16_t range_bin;
    uint8_t i;
    ALGORITHM_PARAM_T tempParam = gAlgorithmParam;
    
    /* Note: the transfer data is merged by ID and DATA. The ID is 6bits and DATA is 32bits,
             PARAM_CFG_CMD(0x0060) protocol: cmd->cmdData[0] is MaxMotinRangeBinID(16bits)
                                             cmd->cmdData[1] is MaxMotinRangeBin(32bits) 
                                             cmd->cmdData[3] is MaxMotinRangeLessBinID(16bits)
                                             cmd->cmdData[4] is MaxMotinRangeLessBin(32bits)
                                             cmd->cmdData[6] is OFF_TimeID(16bits)
                                             cmd->cmdData[7] is OFF_Time(32bits)
                     
             THRESHOLD_SET_CMD(0x0064) proyocol: cmd->cmdData[0] is RangeBinID(16bits)
                                            cmd->cmdData[1] is RangeBin(32bits) 
                                            cmd->cmdData[3] is MotionThresholdID(16bits)
                                            cmd->cmdData[4] is MotionThreshold(32bits) 
                                            cmd->cmdData[6] is MotionLessThresholdID(16bits) 
                                            cmd->cmdData[7] is MotionLessThreshold(32bits),
                                            if RangeBin value is 0xFFFF, Then the threshold is for all Bins. 
     */
    
    /* Radar must be in cmd mode */
    if(0 == CmdModeFlag)
    {
        clearCmdHeadFlag();
        goto ACK_FAIL_RETURN; 
    }
    /* config RangeBin threshold cmd */
    if(THRESHOLD_SET_CMD == cmd->cmdType)
    {
        /* check paras ID */
        if(0x0000 != cmd->cmdData[0] || 0x0001 != cmd->cmdData[3] || 0x0002 != cmd->cmdData[6])
        {
            goto ACK_FAIL_RETURN;
        }

         range_bin =  cmd->cmdData[1];
         /* range bin is out of range */
        if((0xFFFF != range_bin) && (range_bin > ONEFFT_POINT-2))
        {
            goto ACK_FAIL_RETURN;
        } 

        /* config all range bins threshold */
        if(0xFFFF == range_bin)
        {
            for(i = 0; i < ONEFFT_POINT-1 ; i++)
            {
                 tempParam.nThresholdValOfMotion[i] = (uint32_t)(cmd->cmdData[4]);
                 tempParam.nThresholdValOfMotionLess[i] = (uint32_t)(cmd->cmdData[7]);
            }
        }
        /* config sigle bin threshold */
        else
        {
            tempParam.nThresholdValOfMotion[range_bin] = (uint32_t)(cmd->cmdData[4]);
            tempParam.nThresholdValOfMotionLess[range_bin] = (uint32_t)(cmd->cmdData[7]);
        }
    }
    /* config max range bin and off time cmd */
    else if(PARAM_CFG_CMD == cmd->cmdType)
    {
         /* check para ID */
        if(0x0000 != cmd->cmdData[0] || 0x0001 != cmd->cmdData[3] || 0x0002 != cmd->cmdData[6])
        {
            goto ACK_FAIL_RETURN;
        }

        tempParam.nMaxMotionRangeBin = cmd->cmdData[1] ;
        tempParam.nMaxMotionLessRangeBin = cmd->cmdData[4];
        tempParam.nOffTime = cmd->cmdData[7];
    }

    /* is some differences between new para with old para */
    if (memcmp((void*)(&gAlgorithmParam), (void*)(&tempParam), sizeof(ALGORITHM_PARAM_T)) != 0)
    {
         memcpy((void*)(&gAlgorithmParam), (void*)(&tempParam), sizeof(ALGORITHM_PARAM_T));
         Algo_SaveParameter((uint32_t *)&gAlgorithmParam, sizeof(ALGORITHM_PARAM_T));
    }

    ackLen = FillCmdAck(NULL, 2, cmd->cmdType, ACK_OK); 
    return ackLen;

ACK_FAIL_RETURN:
    ackLen = FillCmdAck(NULL, 2, cmd->cmdType, ACK_FAIL);
    return ackLen;
}


/*************************************************
Function: DoReadConfigureParam


Description: Read paras data to PC 
Input: cmd, cmd data; cmdLen cmd length
Output: none
Return: ack length
*************************************************/
static uint16_t DoReadConfigureParam(CMD_T *cmd, uint32_t cmdLen)
{
    uint16_t ackLen = 0;
    uint8_t i;
    uint8_t pos = 0;

    union
    {
       uint8_t  tx_buf_u8[6 + (ONEFFT_POINT-1)  * 2];
       uint16_t tx_buf_u16[3 +(ONEFFT_POINT-1) ];
		
    }Buf;

    /* Radar must be in cmd mode */
    if(0 == CmdModeFlag)
    {
        clearCmdHeadFlag();
        ackLen = FillCmdAck(Buf.tx_buf_u16, sizeof(Buf.tx_buf_u16)/sizeof(Buf.tx_buf_u16[0]), cmd->cmdType, ACK_FAIL);
        return ackLen;
    }
    /* fill ack data */
    Buf.tx_buf_u8[pos++] = 0xAA;
    Buf.tx_buf_u8[pos++] = ONEFFT_POINT - 1-1;
    Buf.tx_buf_u8[pos++] = (uint8_t)gAlgorithmParam.nMaxMotionRangeBin;
    Buf.tx_buf_u8[pos++] = (uint8_t)gAlgorithmParam.nMaxMotionLessRangeBin;
    for(i = 0; i < ONEFFT_POINT-1 ; i++)
    {
        Buf.tx_buf_u8[pos++] = gAlgorithmParam.nThresholdValOfMotion[i];
    }

    for(i = 0; i < ONEFFT_POINT-1 ; i++)
    {
        Buf.tx_buf_u8[pos++] = gAlgorithmParam.nThresholdValOfMotionLess[i];
    }

    *(uint16_t*)&Buf.tx_buf_u8[pos] = gAlgorithmParam.nOffTime ;

    ackLen = FillCmdAck(Buf.tx_buf_u16, sizeof(Buf.tx_buf_u16)/sizeof(Buf.tx_buf_u16[0]), cmd->cmdType, ACK_OK); 
    return ackLen;
}

void DoExitWakeUpACK(void)
{
    uint16_t ackLen = ACK_FAIL;

    CmdModeFlag = 1;

    //  TimerStart(TIMER2); 
    xTimerReset(timer_60ms_handle, pdMS_TO_TICKS(10));

    start_cfg_ack[0] = RADAR_PROTOCOL_VERSION;
    start_cfg_ack[1] = CMD_LEN_MAX;

    masterProtocolVer = 0x01;

    ackLen = FillCmdAck(start_cfg_ack, ARRAY_SIZE(start_cfg_ack), START_CFG_CMD, ACK_OK);

    //  UART0_DmaSend(CmdAck, ackLen);
    uart_dma_send(CmdAck, ackLen);
}

void CmdProc_InCmdModeClear(void)
{
    CmdModeFlag = 0;
}

uint8_t g_FtcmdFlag =  0;
static void CmdExec(CMD_T *cmd, uint32_t cmdLen)
{
    // CMD_REG_T *cmdReg = NULL;
    uint16_t  ackLen = 0;
    uint16_t status = ACK_OK;
    uint16_t temp_result = 0;
    int ret;

    // cmd为两字节数据类型+两字节数据
    #if (RADAR_PROTOCOL_VERSION == 2)
        char      *str = VER_STR();
    #endif

    if (NULL == cmd || 0 == cmdLen)
    {
        return;
    }

    printf("cmd->cmdType=0X%04X\r\n", cmd->cmdType);

	switch(cmd->cmdType)
    {
		 /* Enable debug mode, Send all rangebin mag to PC */
        case ENABLE_DEBUG_CMD:
            if(1 == CmdModeFlag)
            {
                DebugModeFlag = 1;

                #if (isMCU_SLEEP_MODE == 1)
                TimerStopAndClearCounter(TIMER2);
                #endif
                ackLen = FillCmdAck(NULL, 0, cmd->cmdType, ACK_OK);
            }
            else
            {
                clearCmdHeadFlag();
            }
            break;

            /* Disable debug mode, only send target information to PC */
        case DISABLE_DEBUG_CMD:
            if(1 == CmdModeFlag)
            {
                ackLen = FillCmdAck(NULL, 0, cmd->cmdType, ACK_OK);
                DebugModeFlag = 0;
            }
            else
            {
                clearCmdHeadFlag();
            }
            break;

		case START_CFG_CMD:
            printf("start cfg\r\n");
			CmdModeFlag = 1;
		
			if(cmdLen > CMD_TYPE_LEN)
			{
				start_cfg_ack[0] = RADAR_PROTOCOL_VERSION;;
				start_cfg_ack[1] = CMD_LEN_MAX;
				
				masterProtocolVer = cmd->cmdData[0];
				ackLen = FillCmdAck(start_cfg_ack, ARRAY_SIZE(start_cfg_ack), cmd->cmdType, ACK_OK);
			}
			else
			{
				masterProtocolVer = 0;
				ackLen = FillCmdAck(NULL, 0, cmd->cmdType, ACK_OK);
			}
		    break;
        
		case FINISH_CFG_CMD:
            printf("finish cfg\r\n");
			ackLen = FillCmdAck(NULL, 0, cmd->cmdType, ACK_OK);

            if (g_FtcmdFlag == 1)       // 产测模式
            {
                //TODO复位???
            #if 0
                printf("reconfig radar iic\r\n");
                Ft_System_Reconfig();
            #endif

            }
            else
            {
                Config_SavePara2Flash();
                System_Reconfig();
            }
		    break;
            
		case WRITE_REG_CMD:
            ackLen = DoWriteReg(cmd, cmdLen);
		    break;
            
		case READ_REG_CMD:
            ackLen = DoReadReg(cmd, cmdLen);
		    break;
            
		case READ_VER_CMD:
            printf("read version\r\n");
			if(masterProtocolVer != 0)
			{
                #if (RADAR_PROTOCOL_VERSION == 1)
                ackLen = FillCmdAck(read_ver_ack, ARRAY_SIZE(read_ver_ack), cmd->cmdType, ACK_OK);
                #elif (RADAR_PROTOCOL_VERSION == 2)
                ackLen = FillCmdAck((uint16_t *)str, strlen(str), cmd->cmdType, ACK_OK);
                #endif
			}
			else
			{
				ackLen = FillCmdAck(&read_ver_ack[2], ARRAY_SIZE(read_ver_ack) - 2 , cmd->cmdType, ACK_OK);
			}
            
            break;
        
		case WRITE_MTT_CMD:
        case WRITE_SYS_CMD:
		case FFT_ZEROFILL_CMD:
		case NOP_CONFIG_CMD:
            ackLen = DoWritePara(cmd, cmdLen);
		    break;

        case READ_MTT_CMD:
        case READ_SYS_CMD:
            ackLen = DoReadPara(cmd, cmdLen);
		    break;
        
        case READ_SN_CMD:
            ackLen = DoReadSn(cmd);
		    break;
		
		 /* config para */
        case PARAM_CFG_CMD: 
        case THRESHOLD_SET_CMD:

            #if (isMCU_SLEEP_MODE == 1) 
            TimerStopAndClearCounter(TIMER2);
            #endif
            ackLen = DoConfigurePara(cmd, cmdLen);
            break;

        /* Read all para */
        case PARAM_READ_CMD: 

            #if (isMCU_SLEEP_MODE == 1)
            TimerStopAndClearCounter(TIMER2);
            #endif
            ackLen = DoReadConfigureParam(cmd, cmdLen);
           break;
		
		case CASCADING_MODE_CMD:
			ackLen = DoCascadingMode(cmd, cmdLen);
            break;
		
		case START_I2C_TEST_CMD:
			ackLen = StartI2CTest(cmd, cmdLen);
            break;
		
		case STOP_I2C_TEST_CMD:
			ackLen = StopI2CTest(cmd, cmdLen);
            break;
        
		case GET_I2C_TEST_RESULT_CMD:
			ackLen = GetI2CTestResult(cmd, cmdLen);
            break;
		
		case ENTER_FACTORYMODE_CMD:     // 进产测
            printf("\r\nenter factory!\r\n\r\n");
            if(1 == CmdModeFlag)
            {
                status = ACK_OK;
            }
            else
            {
                status = ACK_FAIL;
            }

            //TODO写进入产测标志？？？
            //TODO此处配置iic？
            extern int ft_create_queue_and_task(void);
            ft_create_queue_and_task();
            Ft_System_Reconfig();

            g_FtcmdFlag =  1;
            /* Factory test command ack buffer */
            uint16_t  FT_ack[7] = {0x00, 0x01, 0x01, 0x00, 0x08, 0x08, 0x08};
			ackLen = FillCmdAck(FT_ack, ARRAY_SIZE(FT_ack), cmd->cmdType, status);
			break;
		
		case EXIT_FACTORYMODE_CMD:      // 退出产测
            if(1 == CmdModeFlag)
            {
                status = ACK_OK;
            }
            else
            {
                status = ACK_FAIL;
            }

            g_FtcmdFlag = 0;

            //写产测结果,后续AMPS通过检测结果判断雷达有无进行过产测,1为通过，0为失败
            printf("\r\nexit factory\r\n\r\n");
            printf("result: %d, len(%d)\r\n", cmd->cmdData[0], cmdLen);
            bsp_nv_write(FACTORY_RESULT_KEY, (char *)&cmd->cmdData[0], 2);

			ackLen = DoExitFactoryMode(cmd, cmdLen);
			break;

        case AMPS_READ_RESULT: //获取产测结果指令
            // FD FC FB FA 02 00 A1 00 04 03 02 01
            CmdModeFlag = 1;        // 不让串口输出信息
            printf("AMPS read result!\r\n");
            ret = bsp_nv_read(FACTORY_RESULT_KEY, (char *)&temp_result, sizeof(temp_result));
            if (ret != 0)
            {
                printf("AMPS read result failed!\r\n");
                uart_dma_send((uint8_t *)"\r\nRESULT FAILED\r\n", strlen("\r\nRESULT FAILED\r\n"));
            }
            else
            {
                printf("AMPS read result:%d\r\n", temp_result);

                while(1 == g_uartDmaSendingFlag){}
                g_uartDmaSendingFlag = 1;

                if (temp_result == 1)
                {
                    uart_dma_send((uint8_t *)"\r\nRESULT OK\r\n", strlen("\r\nRESULT OK\r\n"));
                }
                else
                {
                    uart_dma_send((uint8_t *)"\r\nRESULT FAILED\r\n", strlen("\r\nRESULT FAILED\r\n"));
                }
            }
            CmdModeFlag = 0;

            break;

		default:
            printf("unknown cmd\r\n");
		    break;
	}
	
    if (ackLen > 0)
    {
        while(1 == g_uartDmaSendingFlag){}
        g_uartDmaSendingFlag = 1;
        uart_dma_send(CmdAck, ackLen);

		if(cmd->cmdType == FINISH_CFG_CMD)
		{
			CmdModeFlag = 0;
		}
    }
}

uint8_t CmdProc_InCmdMode(void)
{
    return CmdModeFlag;
}

static void CmdProcess(uint8_t* buf, uint16_t len)
{
    uint16_t loop = 0;

    if (NULL == buf || 0 == len)
    {
        return;
    }

    printf("recvLen:%d\r\n", len);
    for (int i = 0; i < len; i++)
    {
        printf("0x%02x ", buf[i]);
    }
    printf("\r\n");

	for (loop = 0; loop < len; loop++) 
    {
		switch(CmdDataParse.state)
		{                    
			case CMD_STATE_HEAD0:
				if (buf[loop] == CmdHead[CMD_HEAD_0]) 
                {
                    CmdDataParse.curIndex = 0;
					CmdDataParse.state = CMD_STATE_HEAD1;
				}
				break;
                
			case CMD_STATE_HEAD1:
				if (buf[loop] == CmdHead[CMD_HEAD_1])
                {
                    CmdDataParse.state = CMD_STATE_HEAD2;
				}
                else
                {
                    CmdDataParse.state = CMD_STATE_HEAD0;
                }
				break;

            case CMD_STATE_HEAD2:
				if (buf[loop] == CmdHead[CMD_HEAD_2])
                {
                    CmdDataParse.state = CMD_STATE_HEAD3;
				}
                else
                {
                    CmdDataParse.state = CMD_STATE_HEAD0;
                }
				break;

            case CMD_STATE_HEAD3:
				if (buf[loop] == CmdHead[CMD_HEAD_3])
                {
                    CmdDataParse.state = CMD_STATE_LEN0;
				}
                else
                {
                    CmdDataParse.state = CMD_STATE_HEAD0;
                }
				break;

            case CMD_STATE_LEN0:
                CmdDataParse.len = buf[loop];
                CmdDataParse.state = CMD_STATE_LEN1;
				break;

            case CMD_STATE_LEN1:
                CmdDataParse.len += buf[loop] << CMD_LEN_HIGH_POS;
                if (CmdDataParse.len <= CMD_BUF_LEN)
                {
                    CmdDataParse.state = CMD_STATE_DATA;
                }
                else
                {
                    CmdDataParse.state = CMD_STATE_HEAD0;
                }
				break;
                
            case CMD_STATE_DATA:
                CmdDataParse.buf[CmdDataParse.curIndex++] = buf[loop];
				if (CmdDataParse.curIndex == CmdDataParse.len)
                {
					CmdDataParse.state = CMD_STATE_TAIL0;
				}
				break;
                
            case CMD_STATE_TAIL0:
				if (buf[loop] == CmdTail[CMD_TAIL_0])
                {
                    CmdDataParse.state = CMD_STATE_TAIL1;
				}
                else
                {
                    CmdDataParse.state = CMD_STATE_HEAD0;
                }
				break;

            case CMD_STATE_TAIL1:
				if (buf[loop] == CmdTail[CMD_TAIL_1])
                {
                    CmdDataParse.state = CMD_STATE_TAIL2;
				}
                else
                {
                    CmdDataParse.state = CMD_STATE_HEAD0;
                }
				break;
                
            case CMD_STATE_TAIL2:
				if (buf[loop] == CmdTail[CMD_TAIL_2])
                {
                    CmdDataParse.state = CMD_STATE_TAIL3;
				}
                else
                {
                    CmdDataParse.state = CMD_STATE_HEAD0;
                }
				break;

            case CMD_STATE_TAIL3:
				if (buf[loop] == CmdTail[CMD_TAIL_3])
                {
                    CmdExec((CMD_T*)(CmdDataParse.buf), CmdDataParse.len);  
				}
                CmdDataParse.state = CMD_STATE_HEAD0;
				break; 
			
			default:
				CmdDataParse.state = CMD_STATE_HEAD0;
				break;

		}
	}
}

void CmdProc_Recv(void)
{
    if (!g_cmdRecv.cmdReady)
    {
        return;
    }

    CmdProcess(g_cmdRecv.buf[g_cmdRecv.bufProc], g_cmdRecv.bufLen);

    g_cmdRecv.cmdReady = 0;
}

void CmdProc_Init(void)
{
    memset(&g_cmdRecv, 0, sizeof(g_cmdRecv));
	InitAlgorithmParam();//hdn added this 
}

uint8_t CmdProc_IsInDebugMode(void)
{
    return DebugModeFlag;
}



