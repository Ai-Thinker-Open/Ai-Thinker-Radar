/**
  ******************************************************************************
  * @file           : dataprocess.c
  * @author         : iclm team
  * @brief          : data process module
  ******************************************************************************
  */
#include <stdio.h>
#include <string.h>
#include "global_conf.h"
#include "utilities.h"
#include "dataprocess.h"
#include "banyan.h"
#include "cmdprocess.h"
#include "system.h"
#include "body_presence.h"
#include "config.h"

#include "bsp_nvs.h"
#include "bsp_spi.h"
#include "mcu_sleep.h"
#include "bsp_uart.h"
#include <FreeRTOS.h>
#include <timers.h>

#include "hosal_gpio.h"

#include "axk_radar_cfg.h"

#define  NOISE_CONVERGENCE_CNT    (15)
#define  FT_NOISE_START           (4)
#define  FT_NOISE_STOP            (8)
#define  FT_NOISE_NUM             (FT_NOISE_STOP - FT_NOISE_START)
#define  FT_NOISE_VAL             (14)

#define RADAR_CHIRP_NUM           (64U)

RADAR_DATA_PARSE_T RadarDataParse[CHANNEL_MAX];
RADAR_PARA_T RadarPara;
static uint8_t data_process_finished = 0;
static uint8_t     is_yone_buffer_clear = 0;
static COMPLEX16_T factory_test_filter[FT_NOISE_NUM] = {0};
static COMPLEX32_T factory_test_chirp_power[FT_NOISE_NUM] = {0};

static uint32_t NoCoherentAccBuf[NOCOHERENTLEN][FT_NOISE_NUM] = {0};
static uint32_t FT_bin_mag[FT_NOISE_NUM] = {0};
static uint32_t AccSum[FT_NOISE_NUM];
static uint16_t nStable_cnt = 0;
static uint16_t nCurLoop = 0;

/* 串口回调函数指针 */
void (*SendResultCallback)(uint8_t *buff, uint16_t len);

/* 上报数据缓冲区 */
static uint8_t ReportBuffer[REPORT_LENGTH] __ALIGNED(4) = {0};

extern TimerHandle_t spi_timeout_handle;

/*************************************************
Function: RegisterCallback

Description: registe usart callback function
Input: debugfunc, callback function pointer
Output: none
Return: none
*************************************************/
static void RegisterCallback(void (*debugfunc)(uint8_t *, uint16_t))
{
    SendResultCallback = debugfunc;
}

/* 低功耗类型，用于两种低功耗切换   POWER_TYPE_1HZ：1s检测一次   POWER_TYPE_10HZ：100ms检测一次 */
static uint8_t lowPowerType = POWER_TYPE_10HZ;
/*************************************************
Function: isFirstRun
Description: Judge whether it is the first run after download
Input: none,
Output: none
Return: 0:first run
		1:not first run
*************************************************/
static uint8_t isFirstRun(void)
{
	uint32_t value = 0;

    bsp_nv_read(elem_key.magic, (char *)&value, sizeof(value));

    if (value != FLASH_MAGIC_NUM)
    {
        value = FLASH_MAGIC_NUM;
        bsp_nv_write(elem_key.magic, (char *)value, sizeof(value));
        return 0;
    }
    else
        return 1;
}

/********************************************
 @名称；CheckFullFrame
 @功能：判断MCU是否收到完整帧
 @参数：chirpIndex， 待检查chirp索引
 @返回：-1，错误的帧索引; 0，帧索引正确，不是完整帧; 1，接收完整帧
 @作者：AE TEAM
*********************************************/
static int8_t CheckFullFrame(uint16_t index)
{
    static uint8_t nExpIndex = 0;
    if (index == nExpIndex) 
    {
        nExpIndex = (nExpIndex + 1) % FRAME_DEPTH;
    }
    else
    {
        nExpIndex = 0;
		/*解决烧录后上电第一次数据异常的问题*/
		if(isFirstRun() == 0)
			// NVIC_SystemReset();  //TODO 暂时注释
        return -1;
    }

    if (index == (FRAME_DEPTH-1))
    {
        return 1;
    }

    return 0;
}

static float FastSqrt(float x)
{
    float a = x;
    uint32_t i = *(uint32_t *)&x;

    i = (i + 0x3f76cf62) >> 1;

    x = *(float_t *)&i;

    x = (x + a / x) * 0.5f;

    //x = (x + a / x) * 0.5f;//迭代次数，精度要求不高，可以注释这一行

    return x;
}


static void CheckChirpIndex(uint8_t channel, uint8_t chirpIndex)
{
    static uint8_t curIndex[CHANNEL_MAX] = {0};
    static uint8_t oldCurIndex[CHANNEL_MAX] = {0};
    static uint8_t skipNum = SKIP_NUM;

    if (channel >= CHANNEL_MAX)
    {
        return;
    }
        
	if (skipNum) 
    {
		skipNum--;
		curIndex[channel] = oldCurIndex[channel] = chirpIndex % RadarPara.chirpNum;
	} 
    else 
    {
		curIndex[channel] = chirpIndex % RadarPara.chirpNum;
		if (curIndex[channel] != ((oldCurIndex[channel] + 1) % RadarPara.chirpNum))
        {
			Indicator_RadarDataIndexError();
		}
		oldCurIndex[channel] = curIndex[channel];
	}
}

static void CheckFrameCnt(uint8_t channel, uint16_t frameCnt)
{
    static uint16_t oldFrameCnt[CHANNEL_MAX] = {0};
    static uint8_t skipNum = SKIP_NUM;

    if (channel >= CHANNEL_MAX)
    {
        return;
    }
    
	if (skipNum) 
    {
		skipNum--;
		oldFrameCnt[channel] = frameCnt;
	} 
    else if (frameCnt != oldFrameCnt[channel] + 1)
    {
        Indicator_RadarDataIndexError();
    }
    oldFrameCnt[channel] = frameCnt;
}

static void DoNopConfig(uint8_t channel)
{
    switch (RadarPara.dataType)
    {
		case DATA_TYPE_DSRAW:
        case DATA_TYPE_FFT:
            if ((RadarDataParse[channel].chirpIndex == RadarPara.chirpNum - 1) && (channel == g_ChannelCount - 1))
            {
				CmdProc_NopConfig();
            }

            break;
        
        case DATA_TYPE_DFFT:             
            if (channel == g_ChannelCount - 1)
            {
				CmdProc_NopConfig();
            }
            break;
        
        case DATA_TYPE_DFFT_PEAK:
            break;
        
        default:
            break;
    }	
}


static void DataStateIdParse(uint8_t data, uint8_t channel)
{
    uint8_t flag = 0;
    
    switch (RadarPara.dataType)
    {
        case DATA_TYPE_FFT: // ID由4bit组成，FFT_ID: 0 – FFT0, 1 – FFT1，所以数据为3或7
            if ((data & ID_MASK) == FFT0_ID || (data & ID_MASK) == FFT1_ID)
            {
				if(channel > 1)
				{
					data += 0x20;
				}
                RadarDataParse[channel].chirpIndex = (data & CHIRP_INDEX_MASK) << CHIRP_INDEX_POS0;
                flag = 1;
            }
            break;
        case DATA_TYPE_DSRAW:
            if ((data & ID_MASK) == DSRAW0_ID || (data & ID_MASK) == DSRAW1_ID)
            {
				if(channel > 1)
				{
					data += 0x20;
				}
                RadarDataParse[channel].chirpIndex = (data & CHIRP_INDEX_MASK) << CHIRP_INDEX_POS0;
                flag = 1;
            }
            break;
        case DATA_TYPE_DFFT:
			if ((data & ID_MASK) == DFFT0_ID || (data & ID_MASK) == DFFT1_ID)
            {
				if(channel > 1)
				{
					data -= 0x80;
				}
                flag = 1;
			}
            break;
        
        case DATA_TYPE_DFFT_PEAK:
            if ((data & ID_MASK) == DFFT_PEAK_ID)
            {			
                flag = 1;
			}
            break;
        
        default:
            break;
    }

    if (flag)
    {
        RadarDataParse[channel].buf[RadarDataParse[channel].curIndex++] = data;
        RadarDataParse[channel].state = DATA_STATE_INDEX1;
    }
    else
    {
        RadarDataParse[channel].state = DATA_STATE_HEAD;
    }
}

static void DataStateIndex1Parse(uint8_t data, uint8_t channel)
{
    switch (RadarPara.dataType)
		case DATA_TYPE_DSRAW:
    {
        case DATA_TYPE_FFT:
            RadarDataParse[channel].chirpIndex += data >> CHIRP_INDEX_POS1;
            break;
        
        case DATA_TYPE_DFFT:
            RadarDataParse[channel].frameCnt = data << FRAME_CNT_POS;
            break;
        
        case DATA_TYPE_DFFT_PEAK:
            break;
        
        default:
            break;
    }

    RadarDataParse[channel].buf[RadarDataParse[channel].curIndex++] = data;
    RadarDataParse[channel].state = DATA_STATE_INDEX2;
}

static void DataStateIndex2Parse(uint8_t data, uint8_t channel)
{
    switch (RadarPara.dataType)
    {
        case DATA_TYPE_FFT:
            break;
        
        case DATA_TYPE_DFFT:
            RadarDataParse[channel].frameCnt += data;
            break;
        
        case DATA_TYPE_DFFT_PEAK:
            break;
        
        default:
            break;
    }
    
    RadarDataParse[channel].buf[RadarDataParse[channel].curIndex++] = data;
    RadarDataParse[channel].state = DATA_STATE_DATA;
}

static uint8_t DataStateTail2Parse(uint8_t data, uint8_t channel)
{
    switch (RadarPara.dataType)
    {
        case DATA_TYPE_FFT:
			if(channel > 1)
			{
				data += 0x20;
			}
            break;
				
        case DATA_TYPE_DSRAW:
			if(channel > 1)
			{
				data += 0x20;
			}
            
            break;
        
        default:
            break;
    }

	RadarDataParse[channel].buf[RadarDataParse[channel].curIndex++] = data;
	RadarDataParse[channel].state = DATA_STATE_TAIL3;
    return 1;
}

static uint8_t DataStateTail3Parse(uint8_t data, uint8_t channel)
{
    RadarDataParse[channel].state = DATA_STATE_HEAD;
    if (data != DATA_TAIL) 
    {
        return 0;
    }

    RadarDataParse[channel].buf[RadarDataParse[channel].curIndex++] = data;
    
    return 1;
}

static void DataCopy(uint8_t* buf, uint16_t len, uint8_t channel, uint16_t *i)
{
    uint16_t copyLen = 0;

    if (NULL == buf || NULL == i)
    {
        return;
    }
    
	copyLen = (RadarDataParse[channel].needCopyLen > (len - *i))?
            (len - *i) : RadarDataParse[channel].needCopyLen;
    memcpy(&RadarDataParse[channel].buf[RadarDataParse[channel].curIndex], &buf[*i], copyLen);

    RadarDataParse[channel].curIndex += copyLen;
    *i += (copyLen - 1);
    RadarDataParse[channel].needCopyLen -= copyLen;

    if (!RadarDataParse[channel].needCopyLen)
    {
    	RadarDataParse[channel].state = DATA_STATE_TAIL0;
        RadarDataParse[channel].needCopyLen = RadarPara.dataLen - SPI_FRAME_HLEN - SPI_FRAME_TLEN;
    }
}

static uint8_t DataParse(uint8_t* buf, uint16_t len, uint8_t channel, uint16_t* left)
{
    uint16_t i = 0;
    uint8_t parseFinish = 0;
	*left = len;
	
    if (NULL == buf || 0 == len || channel >= CHANNEL_MAX)
    {
        return 0;
    }

	for (i = 0; (i < len) && (parseFinish == 0); i++) 
    {		
        switch(RadarDataParse[channel].state)   // 数据解析状态
		{                    
			case DATA_STATE_HEAD:               // 包头原封不动存入buffer
				if (buf[i] == DATA_HEAD) 
                {
                    RadarDataParse[channel].curIndex = 0;
                    RadarDataParse[channel].buf[RadarDataParse[channel].curIndex++] = buf[i];
					RadarDataParse[channel].state = DATA_STATE_ID;
				}
				break;
                
			case DATA_STATE_ID:
                DataStateIdParse(buf[i], channel);
				break;
                
			case DATA_STATE_INDEX1:
                DataStateIndex1Parse(buf[i], channel);
				break;
                
			case DATA_STATE_INDEX2:
                DataStateIndex2Parse(buf[i], channel);
				break;
            
			case DATA_STATE_DATA:	
                DataCopy(buf, len, channel, &i);
				break;

            case DATA_STATE_TAIL0:
                RadarDataParse[channel].buf[RadarDataParse[channel].curIndex++] = buf[i];
				RadarDataParse[channel].state = DATA_STATE_TAIL1;
				break;

            case DATA_STATE_TAIL1:
                RadarDataParse[channel].buf[RadarDataParse[channel].curIndex++] = buf[i];
				RadarDataParse[channel].state = DATA_STATE_TAIL2;
				break;

            case DATA_STATE_TAIL2:
				DataStateTail2Parse(buf[i], channel);
				break;
                
   		    case DATA_STATE_TAIL3:
                parseFinish = DataStateTail3Parse(buf[i], channel);
				break;
			
			default:
				RadarDataParse[channel].state = DATA_STATE_HEAD;
				break;
		}
	}

	*left -= i;
	
    return parseFinish;
}


static uint16_t nRecvCnt[CHANNEL_MAX] = {0};
void DataProc_ResetRecvCnt(void)
{
    memset(nRecvCnt, 0, sizeof(nRecvCnt));
}

uint8_t isFrameLastChirpDataFinished(void)
{
    return data_process_finished;
}

void setFrameLsatChirpDataFinished(void)
{
    data_process_finished = 1;   
}

void clearFrameLsatChirpDataFinished(void)
{
    data_process_finished = 0;   
}

// 没用到
void SendResultData(uint8_t * buf, uint16_t size)
{
    if (g_uartDmaSendingFlag)
    {
        Indicator_RadarDataSendOverFlow();
    }
    else
    {
        g_uartDmaSendingFlag = 1;
        uart_dma_send(buf, size);
    }
}

static void OneRecursiveCanceller_for_ft(COMPLEX16_T *range_chirp, uint8_t start_pos, uint8_t num, uint8_t shift)
{
    uint8_t fast_index = 0;
    COMPLEX32_T clear32 = {0,0};

    for(fast_index=start_pos; fast_index< num; fast_index++)
    {
        clear32.real = ((int32_t)range_chirp[fast_index].real << 10) - (Wone[fast_index].real >> shift);
        clear32.imag = ((int32_t)range_chirp[fast_index].imag << 10) - (Wone[fast_index].imag >> shift);

        Wone[fast_index].real += clear32.real;
        Wone[fast_index].imag += clear32.imag;

        Yone[fast_index].real = clear32.real >> 10;
        Yone[fast_index].imag = clear32.imag >> 10;
    }
}

static void DoFactoryTestDataSend(uint8_t* frameBuf, uint16_t bufLen, uint16_t index)
{
    uint8_t         i,j           = 0;
    float           f_real      = 0.0f;
    uint16_t        u16_real    = 0;
    const  uint8_t  coe         = 3;
    uint32_t           Noise       = 0;
    static uint8_t  nExpIndex   = 0;
    static uint8_t   stable_flag = 0;

    /* Check chirp index */
    if (index != nExpIndex)
    {
        nExpIndex = 0;
    }
    nExpIndex = (nExpIndex + 1) % FRAME_DEPTH;

    /* Data transfer into complex */
    for(i = FT_NOISE_START; i < FT_NOISE_STOP; i++)
    {
        factory_test_filter[i-FT_NOISE_START].real = (frameBuf[i*4+0+4] << 8) + frameBuf[i*4+1+4];
        factory_test_filter[i-FT_NOISE_START].imag = (frameBuf[i*4+2+4] << 8) + frameBuf[i*4+3+4];
    }

    /* Do IIR filter */
    OneRecursiveCanceller_for_ft(factory_test_filter, 0, FT_NOISE_NUM, coe);

    /* chirp累积（实数） */
    for(i = 0; i < FT_NOISE_NUM; i++)
    {
        factory_test_chirp_power[i].real += (int32_t)Yone[i].real;
        factory_test_chirp_power[i].imag += (int32_t)Yone[i].imag;
    }

    /* Last chirp, start to send data */
    if (index == (RADAR_CHIRP_NUM - 1))
    {
        printf("last chirp\r\n");
        nStable_cnt++;

        for(i = 0; i < FT_NOISE_NUM; i++)
        {
            FT_bin_mag[i] = Euclidean_32(abs_32(factory_test_chirp_power[i].real),
                                         abs_32(factory_test_chirp_power[i].imag));
        }
        
        for(i = 0; i < FT_NOISE_NUM; i++)
        {
             NoCoherentAccBuf[nCurLoop][i] = FT_bin_mag[i];
        }

        nCurLoop++;
        if (nCurLoop == NOCOHERENTLEN)
        {
            nCurLoop = 0;
        }
        
        /* 非相干累积 */
        for(i = 0; i < FT_NOISE_NUM; i++)
        {
            for(j = 0; j < NOCOHERENTLEN; j++)
            {
                AccSum[i] += (uint32_t)NoCoherentAccBuf[j][i];
            }
            AccSum[i] = AccSum[i] >> 5;
        }

        for(i = 0; i < FT_NOISE_NUM; i++)
        {
            Noise += AccSum[i];
        }

        f_real = (float)Noise / 1.414f;
        f_real /= (float)FT_NOISE_NUM;

        u16_real = (uint16_t)(f_real * 100.0f + 0.5f);

        // printf("Noise=%d\r\n", Noise);
        // printf("Noise / FT_NOISE_NUM = %d\r\n", Noise / FT_NOISE_NUM);
        // printf("nStable_cnt:%d\r\n", nStable_cnt);
        if(Noise / FT_NOISE_NUM < FT_NOISE_VAL || nStable_cnt > NOISE_CONVERGENCE_CNT)
        {
            stable_flag = 1;
        }
        if(0 == stable_flag)
        {
            // printf("0 == stable_flag\r\n");
            memset(factory_test_chirp_power, 0, sizeof(factory_test_chirp_power));
            return;
        }

        // printf("%.3f\r\n", 10.0f*log10f(u16_real*u16_real*2.0f)-40.0f);
        // printf("Mag:%d,Real:%d,Noise:%.3fdBm\r\n", Noise/4, u16_real, 10.0f*log10f(u16_real*u16_real*2.0f)-40.0f);

        frameBuf[1] = 0x30;
        frameBuf[2] = 0x38;
        frameBuf[38] = 0x37;

        /* Recover last range bin data */
        frameBuf[(ONEFFT_POINT-1)*4+0+4] = (u16_real & 0xFF00) >> 8;
        frameBuf[(ONEFFT_POINT-1)*4+1+4] = (u16_real & 0x00FF) >> 0;
        frameBuf[(ONEFFT_POINT-1)*4+2+4] = (u16_real & 0xFF00) >> 8;
        frameBuf[(ONEFFT_POINT-1)*4+3+4] = (u16_real & 0x00FF) >> 0;

        /* Send data */
        while(1 == g_uartDmaSendingFlag){}
        g_uartDmaSendingFlag = 1;
        uart_dma_send(frameBuf, bufLen);

        memset(factory_test_chirp_power, 0, sizeof(factory_test_chirp_power));

        while(1 == g_uartDmaSendingFlag){}
    }
}

/*************************************************
Function: FillReportBuf
Description: fill report data into ReportBuffer array
Input: result, target result pointer
Output: none
Return: report data length
*************************************************/

static uint16_t FillReportBuf(Result * result)
{
    uint8_t  pos = 0;
    uint16_t ret = REPORT_LENGTH - MAX_MOTION_BINS_LENGTH - MAX_MOTIONLESS_BINS_LENGTH - MOTION_DATA_RANGE_BIN_LENGTH - MOTIONLESS_DATA_RANGE_BIN_LENGTH;
    uint16_t report_length = TARGET_DATA_LENGTH + DATA_TYPE_LENGTH + DATA_HEAD_LENGTH + DATA_TAIL_LENGTH + DATA_CHECK_LENGTH;
    uint8_t  report_type   = REPORT_TARGET_TYPE;
    memset(ReportBuffer, 0, sizeof(ReportBuffer));

    /* 1、填充协议头部 */
    *(uint32_t*)&ReportBuffer[pos] = (uint32_t)PROTOCOL_HEAD;
    pos += REPORT_HEAD_LENGTH;
    
    /* 2、填充数据类型，长度。长度包括数据类型，数据头，数据，数据尾，校验 */
    if(IsInDebugMode())
    {
        report_length += (MOTION_DATA_RANGE_BIN_LENGTH + MOTIONLESS_DATA_RANGE_BIN_LENGTH + MAX_MOTION_BINS_LENGTH + MAX_MOTIONLESS_BINS_LENGTH);
        report_type =  REPORT_DEBUG_TYPE;
    }
    *(uint16_t*)&ReportBuffer[pos] =  report_length;
    pos += DATA_LEN_LENGTH;
    ReportBuffer[pos] = report_type;
    pos += DATA_TYPE_LENGTH;
    
    /* 4、填充数据头部 */
    ReportBuffer[pos] = DATA_HEAD;
    pos += DATA_HEAD_LENGTH;

    /* 5、填充目标信息 */
    ReportBuffer[pos++] = result->status;

   *(uint16_t*)&ReportBuffer[pos] = result->Motion_Distance;
    pos += 2;

    ReportBuffer[pos++] = result->Motion_MaxVal;
    
    *(uint16_t*)&ReportBuffer[pos] = result->MotionLess_Distance;
    pos += 2;

    ReportBuffer[pos++] = result->MotionLess_MaxVal;
    
    *(uint16_t*)&ReportBuffer[pos] = result->Distance;
    pos += 2;
   
    /* 6、如果是DEBUG类型，则填充距离门信息，最后一个距离门的信息上报，但作为保留信息，上位机不解析 */
    if(IsInDebugMode())
    {
        /* 最远运动距离门 */
        ReportBuffer[pos] = (uint16_t)(APP_NFFT - 1);
        pos += MAX_MOTION_BINS_LENGTH;

        /* 最远微动距离门 */
        ReportBuffer[pos] = (uint16_t)(APP_NFFT - 1);
        pos += MAX_MOTIONLESS_BINS_LENGTH;

        /* 运动距离门能量值 */
        for(uint8_t k = 0; k < APP_NFFT; k++)
        {
            ReportBuffer[pos++] = NormalDopplerMaxVal[k];
        }
    
        /* 微动距离门能量值 */
        NormalAccSum[0] = 0;
        NormalAccSum[1] = 0;
        for(uint8_t k = 0; k < APP_NFFT; k++)
        {
           ReportBuffer[pos++] = NormalAccSum[k];
        }
        ReportBuffer[pos++] = NormalDopplerMaxVal[APP_NFFT];
        ReportBuffer[pos++] = NormalAccSum[APP_NFFT];
    } 
     /* 7、填充协议尾部 */
    ReportBuffer[pos] = DATA_TAIL;
    pos += DATA_TAIL_LENGTH;

    ReportBuffer[pos] = 0x00;
    pos += DATA_CHECK_LENGTH;

    *(uint32_t*)&ReportBuffer[pos] = (uint32_t)PROTOCOL_TAIL;

    if(IsInDebugMode())
    {
        ret = REPORT_LENGTH;
    }
    return ret;
}

static void AlgoResultTransfer(Result * result)
{
    uint8_t len = 0;
    /* 填充并发送数据 */
    len = FillReportBuf(result);
    
    /* Uart DMA 发送数据 */
    if(SendResultCallback != NULL)
    {
        SendResultCallback(ReportBuffer, len);
    }
}

static void DataDispatch(uint8_t* frameBuf, uint16_t bufLen, uint8_t channel, uint16_t index)
{
    uint8_t* dataBuf = NULL;
    uint16_t dataLen = 0;
	Result mResult;
	
    if (NULL == frameBuf || 0 == bufLen)
    {
        return;
    }

    dataBuf = frameBuf + SPI_FRAME_HLEN;
    dataLen = bufLen - SPI_FRAME_HLEN - SPI_FRAME_TLEN;

	if (CmdProc_InCmdMode())
    {
    #if RADAR_WORK_MODE_LOG_ENABLE
        printf("Cmd Mode!!!\r\n");
    #endif

        if (spi_timeout_handle != NULL)
            xTimerReset(spi_timeout_handle, pdMS_TO_TICKS(20));
        return;
    }

	if (getFTMode() == 1) 
    {
    #if RADAR_WORK_MODE_LOG_ENABLE
        printf("Factory Mode!!!\r\n");
    #endif
        DoFactoryTestDataSend(frameBuf, bufLen, index);
        if (spi_timeout_handle != NULL)
            xTimerReset(spi_timeout_handle, pdMS_TO_TICKS(20)); // spi超时处理
        return;
    }

	if(CmdProc_IsInDebugMode()) 
    {
        EnableDebugMode();
    }
    else
    {
        DisableDebugMode();
    }
    
	if(is_yone_buffer_clear == 0)
    {
        is_yone_buffer_clear = 1;
        memset(Yone, 0, sizeof(Yone));
        memset(Wone, 0, sizeof(Wone));
    }

    if(1 == CheckFullFrame(index))
    {
    #if COMPLETE_DATA_LOG_ENABLE
        printf("\r\nfull frame\r\n");
    #endif

    #if 1
        bsp_spi_deinit();

        /* SoC进入低功耗 */
        power_off();

        // extern void cs_interrupt_init(void);
        // cs_interrupt_init();
        // extern int hosal_gpio_irq_mask(hosal_gpio_dev_t *gpio, uint8_t mask);
        // extern hosal_gpio_dev_t key1;
        // hosal_gpio_irq_mask(&key1, 0); 
    #endif

        if (spi_timeout_handle != NULL)
            xTimerReset(spi_timeout_handle, pdMS_TO_TICKS(20)); // spi超时处理

        extern QueueHandle_t SpiDataQueue;
        if (SpiDataQueue != NULL)
            xQueueReset(SpiDataQueue);

        xTimerReset(timer_60ms_handle, pdMS_TO_TICKS(10));
    }

    mResult = BodyPresence(frameBuf, index,
                           gAlgorithmParam.nMaxMotionRangeBin,
                           gAlgorithmParam.nMaxMotionLessRangeBin,
                           gAlgorithmParam.nThresholdValOfMotion,
                           gAlgorithmParam.nThresholdValOfMotionLess,
                           gAlgorithmParam.nOffTime);

    if(NOBODY != mResult.status)
    {
		lowPowerType = POWER_TYPE_10HZ;
    }
    else
    {
		lowPowerType = POWER_TYPE_1HZ;
    }

	if(1 == mResult.bNeedReport)
    {
    #if COMPLETE_DATA_LOG_ENABLE
        printf("report result\r\n");
    #endif

        /* 填充并发送数据 */
        AlgoResultTransfer(&mResult);

        /* 等待数据完成 */
        while(1 == g_uartDmaSendingFlag){}

        /* 置数据处理完成标志位 */
        setFrameLsatChirpDataFinished();
    }
}

static uint8_t DispatchFFTDataFromRaw(uint8_t channel, uint16_t index, uint16_t threshold)
{	
	return 0;
}

void DataProcess(uint8_t channel, uint8_t dmaFlag, uint8_t *recvBuf, uint16_t bufLen)
{
    uint8_t parseFinish = 0;
    uint16_t bufLeftLen = bufLen;
    uint16_t index = 0;
    uint16_t threshold = INDICATOR_RECV_THRESHOLD;
    
    if (channel >= CHANNEL_MAX || dmaFlag >= DMA_RECV_FLAG_MAX || NULL == recvBuf)
    {
        printf("Error para!\r\n");
        return;
    }

	while (bufLeftLen > 0)
	{
		parseFinish = DataParse(recvBuf + bufLen - bufLeftLen, bufLeftLen, channel, &bufLeftLen);

		g_dataRecvFlag[channel][dmaFlag] = 0;

		if (!parseFinish)
		{
			continue;
		}
		
		switch (RadarPara.dataType)
		{
			case DATA_TYPE_DSRAW:
			case DATA_TYPE_FFT:
				CheckChirpIndex(channel, RadarDataParse[channel].chirpIndex);
				index = RadarDataParse[channel].chirpIndex;
				break;
			
			case DATA_TYPE_DFFT:
				CheckFrameCnt(channel, RadarDataParse[channel].frameCnt);	
				index = RadarDataParse[channel].frameCnt;
				threshold >>= INDICATOR_RECV_THD_DFFT_SHIFT;
				break;
			
			case DATA_TYPE_DFFT_PEAK:
				threshold >>= INDICATOR_RECV_THD_DPEAK_SHIFT;
				break;
			
			default:
				break;
		}
		
		if(!DispatchFFTDataFromRaw(channel, index, threshold))
		{
            // 此处必定执行
			Indicator_RadarDataReceived(threshold);
			DataDispatch(RadarDataParse[channel].buf, RadarPara.dataLen, channel, index);	
		}	
	}	

    DoNopConfig(channel);    
}

void DataProc_Recv(void)
{
    uint8_t channel = 0;
    uint8_t dmaFlag = 0;
    uint16_t dataPos = 0;

    for (channel = 0; channel < g_ChannelCount; channel++)  // 判断1个通道
    {
        for (dmaFlag = 0; dmaFlag < DMA_RECV_FLAG_MAX; dmaFlag++)   // 2个buffer
        {
            if (!g_dataRecvFlag[channel][dmaFlag])  // 未接收完成
            {
                continue;
            }
            
            if (DMA_RECV_FLAG_MEM_0 == dmaFlag)     // buffer0
            {
                dataPos = 0;
            }
            else                                    // buffer1
            {
                dataPos = RadarPara.dataLen;        // 第二个buffer的起始位置
            }
            
            DataProcess(channel, dmaFlag, &g_dataRecvBuf[channel][dataPos], RadarPara.dataLen);
        }
    }
}

static int8_t GetRadarPara(RADAR_PARA_T *radarPara)
{
    if (NULL == radarPara)
    {
        return FAIL;
    }
    
    radarPara->dataType = Radar_GetDataType();

    printf("dataType=%d\r\n", radarPara->dataType);

    switch (radarPara->dataType)
    {
		case DATA_TYPE_DSRAW:
            radarPara->dataLen = Radar_GetRawPoint() * 2 * 2 + SPI_FRAME_HLEN + SPI_FRAME_TLEN;  /*16 bit, IQ*/
            break;
        
        case DATA_TYPE_FFT:
            radarPara->dataLen = Radar_GetFftPoint() * 2 * 2 + SPI_FRAME_HLEN + SPI_FRAME_TLEN;  /*16 bit, IQ*/
            break;
        
        case DATA_TYPE_DFFT_PEAK:
            radarPara->dataLen = Radar_GetDfftPeakSize() + SPI_FRAME_HLEN + SPI_FRAME_TLEN;
            break;
        
        default:
            printf("Error: unsupport dataType\r\n");
            return FAIL;
    }


    printf("dataLen=%d\r\n", radarPara->dataLen);

    if (radarPara->dataLen > SPI_FRAME_LEN_MAX)
    {
        printf("Error: dataLen is too long\r\n");
        return FAIL;
    }

    radarPara->chirpNum = Radar_GetOneFrameChirpNum();

    printf("chirpNum=%d\r\n", radarPara->chirpNum);
    
    return OK;
}

void DataProc_Init(void)
{
    uint8_t channel = 0;
    
    memset(&RadarDataParse, 0 ,sizeof(RadarDataParse));
    memset(&RadarPara, 0 ,sizeof(RadarPara));

    if (FAIL == GetRadarPara(&RadarPara))       // IIC读取雷达参数
    {
        //RunFailed((uint8_t *)__FILE__, __LINE__);
    }

    for (channel = 0; channel < CHANNEL_MAX; channel++)
    {
        RadarDataParse[channel].needCopyLen = RadarPara.dataLen - SPI_FRAME_HLEN - SPI_FRAME_TLEN;
    }

    // spi、dma初始化，RadarPara.dataType没用上
    // SPI_Init(RadarPara.dataLen * 2, RadarPara.dataType); /*radar data received by spi dma, ping-pang buffer*/
    // bsp_spi_slave_init();
	// 回调使用dma发送usart0的数据
    // 此处的回调似乎是给算法部分用的
    RegisterCallback(SendResultData); //add by hdn
}

uint8_t DataProc_NeedReconfig(void)
{
    uint8_t channel = 0;
    uint8_t needReconfig = 0;
    RADAR_PARA_T radarParaTmp = {0};
    
    if (FAIL == GetRadarPara(&radarParaTmp))
    {
        //RunFailed((uint8_t *)__FILE__, __LINE__);
    }

    RadarPara.chirpNum = radarParaTmp.chirpNum;
    if ((radarParaTmp.dataType != RadarPara.dataType) || (radarParaTmp.dataLen != RadarPara.dataLen))
    {
        needReconfig = 1;
        RadarPara.dataType = radarParaTmp.dataType;
        RadarPara.dataLen = radarParaTmp.dataLen;
    }
	
	memset(&RadarDataParse, 0 ,sizeof(RadarDataParse));
	for (channel = 0; channel < CHANNEL_MAX; channel++)
	{
		RadarDataParse[channel].needCopyLen = RadarPara.dataLen - SPI_FRAME_HLEN - SPI_FRAME_TLEN;
	}
	
    return needReconfig;
}

uint8_t DataProc_GetRadarDataType(void)
{    
    return RadarPara.dataType;
}

uint16_t DataProc_GetRadarDataLen(void)
{    
    return RadarPara.dataLen;
}

uint8_t GetLowPowerType(void)
{
	return lowPowerType;
}
