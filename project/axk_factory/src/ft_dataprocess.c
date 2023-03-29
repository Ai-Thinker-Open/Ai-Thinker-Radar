#include "ft_dataprocess.h"
#include "dataprocess.h"
#include "banyan.h"
#include <string.h>

#define FT_RADAR_DATA_MAX_LEN       (256)

#define FT_SPI_FRAME_DLEN_MAX       FT_RADAR_DATA_MAX_LEN
#define FT_SPI_FRAME_HLEN           (4)
#define FT_SPI_FRAME_TLEN           (4)
#define FT_SPI_FRAME_LEN_MAX        (FT_SPI_FRAME_DLEN_MAX + FT_SPI_FRAME_HLEN + FT_SPI_FRAME_TLEN)

#define DFFT_PEAK_DATA_LEN          (0x6C)//108bytes


#if 1
void Ft_DataProc_Init(void)
{
    uint8_t dfftDataNum = 0;
    uint8_t dfftChirpNum = 0;
    
    memset(&RadarDataParse, 0 ,sizeof(RadarDataParse));
    memset(&RadarPara, 0 ,sizeof(RadarPara));
    RadarPara.dataType = Radar_GetDataType();//FFT - 0, DFFT - 1, dFFT PEAK -2, MAX -3
    printf("dataType: %d\r\n", RadarPara.dataType);

    RadarPara.chirpNum = Radar_GetOneFrameChirpNum();
    printf("chirpNum: %d\r\n", RadarPara.chirpNum);

    switch (RadarPara.dataType)
    {
        case DATA_TYPE_FFT:
            RadarPara.dataLen = Radar_GetFftPoint() * 2 * 2 + SPI_FRAME_HLEN + SPI_FRAME_TLEN;
            break;
        case DATA_TYPE_DFFT:
            dfftDataNum = Radar_GetDfftDataNum();
            dfftChirpNum = Radar_GetDfftChirpNum();
            RadarPara.dataLen = dfftDataNum * 2 * 2 * dfftChirpNum + SPI_FRAME_HLEN + SPI_FRAME_TLEN;  /*16 bit, IQ*/
            break;
        case DATA_TYPE_DFFT_PEAK:
            RadarPara.dataLen = DFFT_PEAK_DATA_LEN + SPI_FRAME_HLEN + SPI_FRAME_TLEN;
            break;
        default:
            break;
    }

    printf("dataLen: %d\r\n", RadarPara.dataLen);

    RadarDataParse[0].needCopyLen = RadarPara.dataLen - SPI_FRAME_HLEN - SPI_FRAME_TLEN;

    // bsp_spi_slave_init();
}
#endif

