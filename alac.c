#include "alac.h"

int alacSetConf(ALAC *alac){
    if(alac == NULL) return -1;
    faacEncConfigurationPtr pConfiguration = faacEncGetCurrentConfiguration(alac->aacEncoder);
    
	pConfiguration->inputFormat = FAAC_INPUT_16BIT;
    pConfiguration->outputFormat = 1;
    pConfiguration->aacObjectType = LOW;
    pConfiguration->allowMidside = 0;//M/S??
    pConfiguration->useLfe = 0;
    pConfiguration->useTns = 1; //降噪
    pConfiguration->bitRate = 48000;
    pConfiguration->bandWidth = 32000;
    return faacEncSetConfiguration(alac->aacEncoder, pConfiguration);
}

ALAC *alacNew(unsigned int sampleRate, int pcmBufFrames, char *hardWare, snd_pcm_format_t format, int channel){
    int err;
    ALAC *alac = (ALAC *)calloc(1, sizeof(ALAC));
    alac->pcmHwParams = NULL;
    if ((err = snd_pcm_open (&alac->pcmHandle, hardWare, SND_PCM_STREAM_CAPTURE, 0)) < 0) {
        dbg(LOGERROR, "cannot open audio device %s (%s)\n", 
                hardWare,
                snd_strerror (err));
        goto Fail;
    }

    dbg(LOGINFO, "audio interface opened\n");
            
    if ((err = snd_pcm_hw_params_malloc (&alac->pcmHwParams)) < 0) {
        dbg(LOGERROR, "cannot allocate hardware parameter structure (%s)\n",
                snd_strerror (err));
        goto Fail;
    }

    dbg(LOGINFO, "hw_params allocated\n");
                    
    if ((err = snd_pcm_hw_params_any (alac->pcmHandle, alac->pcmHwParams)) < 0) {
        dbg(LOGERROR, "cannot initialize hardware parameter structure (%s)\n",
                snd_strerror (err));
        goto Fail;
    }

    dbg(LOGINFO, "pcmHwParams initialized\n");
        
    if ((err = snd_pcm_hw_params_set_access (alac->pcmHandle, alac->pcmHwParams, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
        dbg(LOGERROR, "cannot set access type (%s)\n",
                snd_strerror (err));
        goto Fail;
    }

    dbg(LOGINFO, "pcmHwParams access setted\n");
        
    if ((err = snd_pcm_hw_params_set_format (alac->pcmHandle, alac->pcmHwParams, format)) < 0) {
        dbg(LOGERROR, "cannot set sample format (%s)\n",
                snd_strerror (err));
        goto Fail;
    }

    dbg(LOGINFO, "pcmHwParams format setted\n");
        
    if ((err = snd_pcm_hw_params_set_rate_near (alac->pcmHandle, alac->pcmHwParams, &sampleRate, 0)) < 0) {
        dbg(LOGERROR, "cannot set sample rate (%s)\n",
                snd_strerror (err));
        goto Fail;
    }
        
    dbg(LOGINFO, "pcmHwParams rate setted\n");

    if ((err = snd_pcm_hw_params_set_channels (alac->pcmHandle, alac->pcmHwParams, channel)) < 0) {
        dbg(LOGERROR, "cannot set channel count (%s)\n",
                snd_strerror (err));
        goto Fail;
    }
        snd_pcm_hw_params_set_period_size_near(alac->pcmHandle, alac->pcmHwParams, (snd_pcm_uframes_t *)&pcmBufFrames, 0);
    dbg(LOGINFO, "pcmHwParams channels setted\n");
        
    if ((err = snd_pcm_hw_params (alac->pcmHandle, alac->pcmHwParams)) < 0) {
        dbg(LOGERROR, "cannot set parameters (%s)\n",
                snd_strerror (err));
        goto Fail;
    }

    dbg(LOGINFO, "pcmHwParams setted\n");
        
    snd_pcm_hw_params_free (alac->pcmHwParams); alac->pcmHwParams = NULL;
    
    dbg(LOGINFO, "pcmHwParams freed\n");

	alac->pcmFormat = format;
    alac->pcmChannel = channel;
    alac->pcmBufFrames = pcmBufFrames;
        
    if ((err = snd_pcm_prepare (alac->pcmHandle)) < 0) {
        dbg(LOGERROR, "cannot prepare audio interface for use (%s)\n",
                snd_strerror (err));
        goto Fail;
    }
    // === PCM INIT END ===   

    alac->pcmBitSize = snd_pcm_format_width(format);
    alac->aacEncoder = faacEncOpen(sampleRate, channel, &alac->aacInputSamples, &alac->aacMaxOutputBytes);
    
    if (alac->aacEncoder == NULL) {
        dbg(LOGERROR, "faacEncOpen fail!\n");
        goto Fail;
    }
    
    dbg(LOGERROR, "aacMaxOutputBytes:%lu\n", alac->aacInputSamples);
    
    if (!alacSetConf(alac)) {
        dbg(LOGERROR, "alacSetConf fail!\n");
        goto Fail;
    }

    return alac;
Fail:
    if(alac->aacEncoder){
            faacEncClose(alac->aacEncoder); alac->aacEncoder = NULL;
    }
    if(alac->pcmHwParams){
        snd_pcm_hw_params_free (alac->pcmHwParams); alac->pcmHwParams = NULL;
    }
    if(alac->pcmHandle){
        snd_pcm_close (alac->pcmHandle);
    }
    if(alac){
        free(alac); alac=NULL;
    }
    return NULL;
}

void alacDel(ALAC *alac){
    if(alac){
        if(alac->aacEncoder){
            dbg(LOGINFO,"");
            faacEncClose(alac->aacEncoder); alac->aacEncoder = NULL;
        }
        if(alac->pcmHandle){
            snd_pcm_close (alac->pcmHandle);
        }
        if(alac->pcmHwParams){
            snd_pcm_hw_params_free (alac->pcmHwParams); alac->pcmHwParams = NULL;
        }
        free(alac); alac=NULL;
    }
    return;
}

int alacGetFrame(ALAC *alac, BYTE *aacBuf, unsigned long aacSize){
    if (!alac || !aacBuf)  return -4;
    if (aacSize < alac->aacMaxOutputBytes) return -3;

    int err;
    int num = 0;
    int	nPCMBufferSize = alac->aacInputSamples * alac->pcmBitSize / 8;
        
    BYTE* pbPCMBuffer = (BYTE *)calloc(1, nPCMBufferSize);
    BYTE* pbAACBuffer = (BYTE *)calloc(1, alac->aacMaxOutputBytes);

    int pcmOneFrameBytes = alac->pcmBitSize / 8 * alac->pcmChannel;
    int pcmBufSize = alac->pcmBufFrames * pcmOneFrameBytes;
    BYTE* pcmBuffer = (BYTE *)calloc(1, pcmBufSize);

    size_t nRet = 0;    
    
    while (!nRet) {
        if ((err = snd_pcm_readi (alac->pcmHandle, pcmBuffer, alac->pcmBufFrames)) != alac->pcmBufFrames) {
            dbg(LOGERROR, "read from audio interface failed (%s)\n",
                        snd_strerror (err));
            nRet = -1;
            goto Fail;
        }
        memcpy(pbPCMBuffer + num, pcmBuffer, alac->pcmBufFrames * pcmOneFrameBytes);
        num += alac->pcmBufFrames * pcmOneFrameBytes;
        if (num == nPCMBufferSize) {
            alac->aacInputSamples = num / (alac->pcmBitSize/8);
            nRet = faacEncEncode(alac->aacEncoder, (int *)pbPCMBuffer, alac->aacInputSamples, pbAACBuffer, alac->aacMaxOutputBytes);
            if (nRet > 0) {
                dbg(LOGINFO, "nRet:%4zd, alac->aacInputSamples:%lu, nPCMBufferSize:%d\n", nRet, alac->aacInputSamples, nPCMBufferSize);
                memcpy(aacBuf ,pbAACBuffer, nRet);                
            }
            num = 0;
        }else if (num > nPCMBufferSize){
            dbg(LOGWARNING, "Something setting wrong.");
            nRet = -2;
            goto Fail;
        }
    }
Fail:
    if (pbPCMBuffer) free(pbPCMBuffer);
    if (pbAACBuffer) free(pbAACBuffer);
    if (pcmBuffer) free(pcmBuffer);

    return nRet;
}
