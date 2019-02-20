#ifndef __ALAC_H__
#define __ALAC_H__


#include <alsa/asoundlib.h>
#include "faac.h"

#include <stdio.h>

typedef unsigned char BYTE;

typedef enum{
    LOGCRIT=0, 
    LOGERROR, 
    LOGWARNING, 
    LOGINFO,
    LOGDEBUG, 
    LOGDEBUG2, 
    LOGALL
} LogLevel;

#define DBG_LV LOGERROR
#define BLUE_BOLD "\x1b[;34;1m"
#define RED_BOLD "\x1b[;34;31m"
#define RESET "\x1b[0;m"

#define dbg(level, fmt, args...)                                      \
    if (level <= DBG_LV) {                                            \
        fprintf(level == LOGERROR?stderr:stdout,                      \
            BLUE_BOLD "%s, %s, %d: " RESET "%s" fmt "%s",             \
            __FILE__, __func__, __LINE__, level==LOGERROR?RED_BOLD:"",\
            ##args, level==LOGERROR?RESET:"");                        \
    }


typedef struct{
    faacEncHandle       aacEncoder;    
    unsigned long       aacInputSamples;
    unsigned long       aacMaxOutputBytes;
    int                 pcmChannel;
    int                 pcmBufFrames;
    unsigned int        pcmBitSize;
    snd_pcm_t           *pcmHandle;
    snd_pcm_format_t    pcmFormat;
    snd_pcm_hw_params_t *pcmHwParams;
}ALAC;

#ifdef  __cplusplus
extern "C" {
#endif
ALAC *alacNew(unsigned int sampleRate, int pcmBufFrames, char *hardWare, snd_pcm_format_t format, int channel);
void  alacDel(ALAC *alac);
int   alacSetConf(ALAC *alac);
int   alacGetFrame(ALAC *alac, BYTE *aacBuf, unsigned long aacSize);
#ifdef  __cplusplus
}
#endif

#endif
