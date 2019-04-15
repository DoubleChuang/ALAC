#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <stdint.h>

#include "alac.h"
#include "shm_ipc.h"

FILE *aac_fp = NULL;
ALAC *alac = NULL;
BYTE *pbAACBuffer = NULL;

AUDIO_SHM_FRAME *audio_shm_q, *audio_shm_tmp;
int msg_id = 0;
key_t audio_shm_key = 9696;
key_t msg_key = 9999;


void *hipc_init(int key, int size){
    int shmid;
    void * paddr;
    if((shmid = shmget(key, size, IPC_CREAT | 0666)) < 0) 
        return (void *)-1;
    if((paddr = (void *)shmat(shmid, NULL, 0)) < 0) 
        return (void *)-1;
    return paddr;
}


int send_ipc_frame(frame_type type, unsigned int num, unsigned char * buf, unsigned int size, unsigned int time)
{
    msg pmsg;
    switch(type){
#if 0
        case MSG_P_FRAME:
            video_tmp = video_start + (SHM_I_NUM * sizeof(VIDEO_SHM_I_FRAME));
            video_shm_p_tmp = (VIDEO_SHM_P_FRAME *)video_tmp + (num % SHM_P_NUM);

            video_shm_p_tmp->num = num;
            video_shm_p_tmp->size = size;
            video_shm_p_tmp->timestamp = time;
            memcpy(video_shm_p_tmp->frame, buf, size);

            pmsg.type = MSG_P_FRAME; 
        break;

        case MSG_I_FRAME:
            video_shm_i_tmp = (VIDEO_SHM_I_FRAME *)video_start + (num % SHM_I_NUM);
            video_shm_i_tmp->num = num;
            video_shm_i_tmp->size = size;
            video_shm_i_tmp->timestamp = time;
            memcpy(video_shm_i_tmp->frame, buf, size);
            
            pmsg.type = MSG_I_FRAME;
        break;
		case MSG_SPS_PPS_FRAME:
            video_shm_i_tmp = (VIDEO_SHM_I_FRAME *)video_start + (num % SHM_I_NUM);
            video_shm_i_tmp->num = num;
            video_shm_i_tmp->size = size;
            video_shm_i_tmp->timestamp = time;
            memcpy(video_shm_i_tmp->frame, buf, size);
            pmsg.type = MSG_SPS_PPS_FRAME;
        break;
#endif
		case MSG_AUDIO:
            audio_shm_tmp = audio_shm_q + (num % SHM_A_NUM);
            audio_shm_tmp->num = num;
            audio_shm_tmp->size = size;
            audio_shm_tmp->timestamp = time;
            memcpy(audio_shm_tmp->frame, buf, size);
            
            pmsg.type = MSG_AUDIO;            
        	break;
        case MSG_UNKOWN:
		default:
            //pmsg.type = MSG_UNKOWN;
            return 0;
		
        break;
    }
    pmsg.num = num;
    pmsg.msg_type = 2;
    if((msgsnd(msg_id, &pmsg, sizeof(msg) - sizeof(long), 0)) < 0)
    {
        //printf("[ERROR]%s:%s:%d:msgsnd:%s\n",__FILE__, __func__, __LINE__, strerror(errno));
        return -1;
    }
    return 0;
    
}

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
	int a_frame_count = 0;
    setSignal(SIGINT);
    setSignal(SIGTERM);
    setSignal(SIGSEGV);

    if (argc == 1){
        argv[1] =  (char *)"hw:0,0";
	}

	if((audio_shm_q = (AUDIO_SHM_FRAME *)hipc_init(audio_shm_key, AUDIO_SHM_SIZE)) == (AUDIO_SHM_FRAME *)-1) {
        printf("[ERROR]%s:%s:%d:[AUDIO]hipc_init:%s\n"
			,__FILE__, __func__, __LINE__, strerror(errno));
    }

    if((msg_id = msgget(msg_key, IPC_CREAT | 0666)) < 0)
    {
        printf("[ERROR]%s:%s:%d:msgget:%s\n"
			,__FILE__, __func__, __LINE__, strerror(errno));
    }
    if (NULL == (alac = alacNew(44100, 128, argv[1], SND_PCM_FORMAT_S16_LE, 1))){
        dbg(LOGERROR, "alacNew fail\n");
        exit(1);
    }

    BYTE *pbAACBuffer = (BYTE *)calloc(1, alac->aacMaxOutputBytes);
	struct timespec nowTime;
    while (1) {
        if ((nRet = alacGetFrame(alac, pbAACBuffer, alac->aacMaxOutputBytes)) > 0) {
			clock_gettime (CLOCK_MONOTONIC, &nowTime);
			if( send_ipc_frame(MSG_AUDIO, a_frame_count,
							 pbAACBuffer, nRet, nowTime.tv_sec*1000 + nowTime.tv_nsec/1.0e6) == -1) {
                        printf("%s:%s:%d:[AUDIO]send_ipc_frame fail\n"
								,__FILE__, __func__, __LINE__);
                        //e_rtmp_ap_status = No_Rtmp_AP_Connect;
            } else  {
                if(++a_frame_count == SHM_A_NUM * 10000)
					a_frame_count = 0;
            }
        }
    }
    

    alacDel(alac);
    exit (0);
}

