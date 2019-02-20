#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <stdint.h>
#include "alac.h"

FILE *aac_fp = NULL;
ALAC *alac = NULL;
BYTE *pbAACBuffer = NULL;

void sigHandler(int signum){ 
    dbg(LOGERROR, "%s signal(%d) PID=%d %d\n", __FILE__, signum, getppid(),getpid());

    if(aac_fp){
        dbg(LOGINFO,"");
           fclose(aac_fp);aac_fp = NULL;
    }
	if(pbAACBuffer)
		free(pbAACBuffer);
    alacDel(alac);
    
    dbg(LOGINFO, "audio interface closed\n");
    exit(1);
}

void setSignal(int signum){
    if(signal(signum, sigHandler) == SIG_ERR){
        dbg(LOGERROR, "Can't set signal for signal(%d)\n", signum);
    }
    return;
}

int main (int argc, char *argv[])
{
    size_t nRet = 0;   

    setSignal(SIGINT);
    setSignal(SIGTERM);
    setSignal(SIGSEGV);

    if (argc == 1)
        argv[1] =  (char *)"hw:0,0";

    if (NULL == (alac = alacNew(44100, 128, argv[1], SND_PCM_FORMAT_S16_LE, 1))){
        dbg(LOGERROR, "alacNew fail\n");
        exit(1);
    }
    
    if (NULL == (aac_fp = fopen("./pcm2aac.aac", "wb+"))) {
        dbg(LOGERROR, "fopen fail\n");
        exit(1);
    }

    BYTE *pbAACBuffer = (BYTE *)calloc(1, alac->aacMaxOutputBytes);

    while (1) {
        if ((nRet = alacGetFrame(alac, pbAACBuffer, alac->aacMaxOutputBytes)) > 0) {
            fwrite(pbAACBuffer, 1, nRet, aac_fp);
            fflush(aac_fp);
        }
    }
    
    if (aac_fp) {
        fclose(aac_fp);aac_fp = NULL;
    }

    alacDel(alac);
    exit (0);
}

