/*
 * Copyright (c) 2015, Simone Margaritelli <evilsocket at gmail dot com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of ARM Inject nor the names of its contributors may be used
 *     to endorse or promote products derived from this software without
 *     specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "hook.h"
#include "io.h"
#include "report.h"
#include <map>
#include <sstream>
#include <pthread.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <string.h>


#include "tool.h"


#include "MediaExtractor_jni.h"
#include "AudioTrack_jni.h"
#include "AudioFormat_jni.h"
#include "AudioManager_jni.h"
#include "avcodec.h"
#include "AssetManager.h"

extern "C"
{
#include "hvjava.h"
#include "AudioDecode_jn.h"
#include "os.h"
#include "List.h"
#include "LoopBuf.h"
}

//*********************************** data

typedef std::map< int, std::string > fds_map_t;

static pthread_mutex_t __lock_context = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t __lock_descriptors = PTHREAD_MUTEX_INITIALIZER;
static fds_map_t __descriptors;


#define lock_context() pthread_mutex_lock(&__lock_context)
#define unlock_context() pthread_mutex_unlock(&__lock_context)

#define lock_descriptors() pthread_mutex_lock(&__lock_descriptors)
#define unlock_descriptors() pthread_mutex_unlock(&__lock_descriptors)

#define DEFAULT_SAMPLE_RATE				 44100
#define DEFAULT_CHANNLE_COUNT            2
#define DEFAULT_BIT_COUNT				 16
#define BIT_COUNT_8						 8
#define PCM_PRODUCE_VIRTUAL_OBJ			 0x00000088
#define PCM_BUF_LEN			 			 1024*1024
#define PRE_MIXER_DATA_SIZE				 1
#define PRE_MIXER_DATA_LEN	             2
#define MAX_MIX_NUM						 10
#define MIX_DURATION					 20		//200ms

#define MUSIC_FILE_PATH					 "/sdcard/.InteranlRec"

typedef enum __tag_Play_Status
{
	Play_Status_Idle = 0,
	Play_Status_Start,
	Play_Status_Pause,
	Play_Status_Stop
}Play_Status;


typedef struct __tag_AudioFormat
{
	unsigned int sample_rate;
	unsigned int channel_count;
	unsigned int bit_count;
}AudioFormat;

typedef struct __tag_MediaPlayContext
{
	void *media_cls_obj;
	int fd;
	long offset;
	long length;
	MediaExtractor_jni* media_extractor;
	media_codec_context *_media_codec_context;
	AudioTrack_jni *audio_track;
	Suntech_Thread *media_play_thread;
	bool exit_media_play_thread;
	Suntech_Thread *audio_play_thread;
	bool exit_audio_play_thread;
	Play_Status play_status;
	bool isLooping;
	AudioFormat audio_play_af;
	unsigned int audio_play_len;
	loop_buf *audio_play_loop_buf;
	Suntech_Mutex *audio_play_loop_buf_lock;
}MediaPlayContext;

typedef struct __tag_SoundPoolContext
{
	void *sound_pool_cls_obj;
	int fd;
	long offset;
	long length;
	int sound_id;
	MediaExtractor_jni* media_extractor;
	media_codec_context *_media_codec_context;
	AudioTrack_jni *audio_track;
	Suntech_Thread *sound_pool_thread;
	bool exit_sound_pool_thread;
	Play_Status play_status;
	char* decode_buffer;
	Suntech_Semaphore* play_sema;
	int Looping;
}SoundPoolContext;

typedef struct __tag_AudioTrackContext
{
	void *audio_track_cls_obj;
	AudioFormat audio_format;
	loop_buf *pcm_loop_buffer;
	Suntech_Mutex *pcm_loop_buffer_lock;
	loop_buf *pcm_resample_loop_buffer;
	Suntech_Mutex *pcm_resample_loop_buffer_lock;
	int raw_data_size;
	short *raw_data;
	ReSampleContext *resample_context;
	bool nead_resample;
	char* resample_in_buffer;
	int resample_in_buffer_len;
	char* resample_out_buffer;
	int resample_out_buffer_len;
	Play_Status play_status;
	bool is_audio_effect;
}AudioTrackContext;

typedef struct __tag_InternalAudioContext
{
	bcm_timer_module_id _timer_moudle_id;
	bcm_timer_id pcm_produce_timer;
	Suntech_Thread *pcm_produce_thread;
	bool	exit_pcm_produce_thread;
	Suntech_List *media_play_context_list;
	Suntech_Mutex *media_play_context_list_lock;
	Suntech_List *audio_track_context_list;
	Suntech_Mutex *audio_track_context_lock;
	Suntech_List *sound_pool_context_list;
	Suntech_Mutex *sound_pool_context_list_lock;
	loop_buf *pcm_mix_buffer;
	Suntech_Mutex *pcm_mix_buffer_lock;
	AudioFormat output_audio_format;
	unsigned long long audio_time;
	unsigned int pcm_mixer_len;
	int mixer_duration;
	InternalRecordCallback _InternalRecordCallback;
	bool is_start_record;
	char* mixer_push_buffer;
	int audio_effect_counter;
}InternalAudioContext;


InternalAudioContext g_context_instance;
InternalAudioContext * g_context = &g_context_instance;
bool g_init_context = false;

static bool AddAudioTrackContextByObject(JNIEnv *env,void *obj,unsigned int sample_rate,unsigned int channel_count,unsigned int bit_count);
static void DelAudioTrackContext(AudioTrackContext *audio_track_context);
static AudioTrackContext* GetAudioTrackContext(void *obj);
static bool AudioTrackWrite(void *obj,const void* buffer, size_t userSize);
static MediaPlayContext* GetMediaPlayContext(void *obj);
static bool AudioTrackRelease(AudioTrackContext * audio_track_context);


//***********************fun

bool file_exists(char *filename)
{
	return (access(filename, 0) == 0);
}

void dfs_remove_dir(const char *path_raw)
{
    DIR *cur_dir = opendir(path_raw);
    struct dirent *ent = NULL;
    struct stat st;
    char file_path[256] = {0};

    if (!cur_dir)
    {
        perror("opendir:");
        return;
    }

    while ((ent = readdir(cur_dir)) != NULL)
    {
        stat(ent->d_name, &st);
        sprintf(file_path,"%s/%s",MUSIC_FILE_PATH,ent->d_name);

        remove(file_path);
    }

    closedir(cur_dir);
}

void remove_dir(const char *path_raw)
{
    char old_path[256];

    if (!path_raw)
    {
        return;
    }

    dfs_remove_dir(path_raw);

    unlink(path_raw);

}

void DeleteMusicDir()
{
	if(file_exists((char*)MUSIC_FILE_PATH))
	{
		//文件夹存在，删除
		remove_dir(MUSIC_FILE_PATH);

	}
}

bool CreateMusicDir()
{
	DeleteMusicDir();

	mkdir( MUSIC_FILE_PATH,S_IRUSR | S_IWUSR | S_IXUSR );

	return true;
}

bool CreateAndCopyMusicFile(int fd,long offset,long length)
{
	int one_read_size = 1024;
	char szname[256] = {0};
	sprintf(szname,"%s/music_%ld_%ld",MUSIC_FILE_PATH,offset,length);

	if( fd < 0 )
		return false;
	//文件是否存在
	if(file_exists(szname))
		return true;

	char* buffer = (char*)malloc(one_read_size);
	int new_fd = open(szname,O_WRONLY | O_CREAT | O_TRUNC,0664);
	if(new_fd)
	{
		int remain = length;
		int total_reads = 0;
		int reads = 0;
		lseek(fd,offset,SEEK_SET);

		while( remain>0 )
		{
			if( remain < one_read_size )
				one_read_size = remain;
			reads = read(fd,buffer,one_read_size);
			write(new_fd,buffer,reads);
			total_reads += reads;
			remain -= reads;
		}

		close(new_fd);
	}

	free(buffer);

	return true;
}


extern uintptr_t find_original( const char *name );

void io_add_descriptor( int fd, const char *name ) {
	lock_descriptors();

    __descriptors[fd] = name;

    unlock_descriptors();
}

void io_del_descriptor( int fd ) {
	lock_descriptors();

    fds_map_t::iterator i = __descriptors.find(fd);
    if( i != __descriptors.end() ){
        __descriptors.erase(i);
    }

    unlock_descriptors();
}

std::string io_resolve_descriptor( int fd ) {
    std::string name;

    lock_descriptors();

    fds_map_t::iterator i = __descriptors.find(fd);
    if( i == __descriptors.end() ){
        // attempt to read descriptor from /proc/self/fd
        char descpath[0xFF] = {0},
             descbuff[0xFF] = {0};

        sprintf( descpath, "/proc/self/fd/%d", fd );
        if( readlink( descpath, descbuff, 0xFF ) != -1 ){
            name = descbuff;
        }
        else {
            std::ostringstream s;
            s << "(" << fd << ")";
            name = s.str();
        }
    }
    else {
        name = i->second;
    }

    unlock_descriptors();

    return name;
}

/********************************* audio handler begin **************************************/

void AudioMix(char sourseFile[MAX_MIX_NUM][PRE_MIXER_DATA_LEN],int number,char *objectFile)
{
	//归一化混音
	int const MAX=32767;
	int const MIN=-32768;

	double f=1;
	int output;
	int i = 0,j = 0;
	for (i=0;i<PRE_MIXER_DATA_LEN/2;i++)
	{
		int temp=0;
		for (j=0;j<number;j++)
		{
			temp+=*(short*)(sourseFile[j]+i*2);
		}
		output=(int)(temp*f);
		if (output>MAX)
		{
			f=(double)MAX/(double)(output);
			output=MAX;
		}
		if (output<MIN)
		{
			f=(double)MIN/(double)(output);
			output=MIN;
		}
		if (f<1)
		{
			f+=((double)1-f)/(double)32;
		}
		*(short*)(objectFile+i*2)=(short)output;
	}
}

static void ResampleAudio()
{
	unsigned int size = 10240;
	unsigned int count = 0;
	unsigned int i = 0;
	char* buffer = 0;

	suntech_mx_lock(g_context->audio_track_context_lock);

	count = suntech_list_count(g_context->audio_track_context_list);
	for ( i = 0; i < count; i++ )
	{
		int get_len = 0;
		int data_len = 0;
		AudioTrackContext * audio_track_context = (AudioTrackContext*) suntech_list_get(g_context->audio_track_context_list, i);

		loop_buf *pcm_loop_buffer = audio_track_context->pcm_loop_buffer;
		loop_buf *pcm_resample_loop_buffer = audio_track_context->pcm_resample_loop_buffer;


		//获得要重采样的数据


		while(GetDataLen(pcm_loop_buffer) > sizeof(int))
		{

			GetData(pcm_loop_buffer, (char*)&data_len,sizeof(int));

			if( data_len > audio_track_context->resample_in_buffer_len )
			{
				audio_track_context->resample_in_buffer_len = data_len;
				audio_track_context->resample_in_buffer = (char*)realloc(audio_track_context->resample_in_buffer,audio_track_context->resample_in_buffer_len);
				if(!audio_track_context->resample_in_buffer)
				{
					audio_track_context->resample_in_buffer_len = 0;
					break;
				}
			}

			buffer = audio_track_context->resample_in_buffer;

			get_len = GetData(pcm_loop_buffer, buffer,data_len);
			//LOGI("ResampleAudio get_len:%d",get_len);


			if(get_len>0)
			{
				if(audio_track_context->nead_resample && audio_track_context->resample_context)
				{
					//resample
					int samples = get_len/audio_track_context->audio_format.channel_count/(audio_track_context->audio_format.bit_count/8);
					int nead_len = samples*DEFAULT_CHANNLE_COUNT*(DEFAULT_BIT_COUNT/8)*(DEFAULT_SAMPLE_RATE/audio_track_context->audio_format.sample_rate);

					if( nead_len > audio_track_context->resample_out_buffer_len )
					{
						audio_track_context->resample_out_buffer_len = nead_len*2;
						audio_track_context->resample_out_buffer = (char*)realloc(audio_track_context->resample_out_buffer,audio_track_context->resample_out_buffer_len);
						if(!audio_track_context->resample_out_buffer)
						{
							audio_track_context->resample_out_buffer_len = 0;
							break;
						}
					}

					int len = audio_resample(audio_track_context->resample_context,(short*)audio_track_context->resample_out_buffer,(short*)buffer,samples);
					if( len > 0)
					{
						len *= (DEFAULT_CHANNLE_COUNT*(DEFAULT_BIT_COUNT/8));
						suntech_mx_lock(audio_track_context->pcm_resample_loop_buffer_lock);
						PutData(pcm_resample_loop_buffer,(char*)audio_track_context->resample_out_buffer,len);
						suntech_mx_unlock(audio_track_context->pcm_resample_loop_buffer_lock);
					}
				}
				else
				{
					suntech_mx_lock(audio_track_context->pcm_resample_loop_buffer_lock);
					int len = PutData(pcm_resample_loop_buffer,buffer,get_len);
					suntech_mx_unlock(audio_track_context->pcm_resample_loop_buffer_lock);
				}
			}



		}
	}

	suntech_mx_unlock(g_context->audio_track_context_lock);

}

static bool IsHaveEnoughDataToMixer()
{
	unsigned int count = 0;
	unsigned int i = 0;
	bool IsHaveEnoughData = true;
	AudioTrackContext * audio_track_context;
	loop_buf *pcm_resample_loop_buffer;
	void* virtual_obj = (void*)PCM_PRODUCE_VIRTUAL_OBJ;

	suntech_mx_lock(g_context->audio_track_context_lock);

	count = suntech_list_count(g_context->audio_track_context_list);
	if( count <= 0 )
	{
		IsHaveEnoughData = false;
		goto end;

	}

	//虚拟音频流数据量必须先满足

	audio_track_context = GetAudioTrackContext(virtual_obj);
	if(!audio_track_context)
	{
		IsHaveEnoughData = false;
		goto end;
	}

	pcm_resample_loop_buffer = audio_track_context->pcm_resample_loop_buffer;
	if(GetDataLen(pcm_resample_loop_buffer) < g_context->pcm_mixer_len)
	{
		IsHaveEnoughData = false;
		goto end;
	}

#if 0
	if(g_context->audio_effect_counter > 0 )
	{
		IsHaveEnoughData = true;
		g_context->audio_effect_counter--;
		goto end;
	}


	for ( i = 0; i < count; i++ )
	{
		AudioTrackContext * audio_track_context = (AudioTrackContext*) suntech_list_get(g_context->audio_track_context_list, i);
		loop_buf *pcm_resample_loop_buffer = audio_track_context->pcm_resample_loop_buffer;

		suntech_mx_lock(audio_track_context->pcm_resample_loop_buffer_lock);
		if(GetDataLen(pcm_resample_loop_buffer) < g_context->pcm_mixer_len)
		{
			if( audio_track_context->play_status == Play_Status_Start )
			{
				IsHaveEnoughData = false;
				suntech_mx_unlock(audio_track_context->pcm_resample_loop_buffer_lock);
				break;
			}
		}
		suntech_mx_unlock(audio_track_context->pcm_resample_loop_buffer_lock);

	}
#endif

end:

	suntech_mx_unlock(g_context->audio_track_context_lock);

	return IsHaveEnoughData;
}

static void RecoveryAudioTrackContext(JNIEnv *env)
{

	unsigned int count = 0;
	unsigned int i = 0;

	suntech_mx_lock(g_context->audio_track_context_lock);

	count = suntech_list_count(g_context->audio_track_context_list);
	for ( i = 0; i < count;i++)
	{
		AudioTrackContext * audio_track_context = (AudioTrackContext*) suntech_list_get(g_context->audio_track_context_list, i);
		loop_buf *pcm_resample_loop_buffer = audio_track_context->pcm_resample_loop_buffer;
		if(audio_track_context->play_status == Play_Status_Stop
				&& GetDataLen(pcm_resample_loop_buffer) <= 0 )
		{
			AudioTrackRelease(audio_track_context);
			LOGE("RecoveryAudioTrackContext DelAudioTrackContext");
			break;
		}
	}

	suntech_mx_unlock(g_context->audio_track_context_lock);
}

static unsigned int DoAudioMixer(void *param)
{

	LOGI("DoAudioMixer begin");

	if(!param)
		return 0;


	JNIEnv *env = (JNIEnv*)param;
	unsigned long long mix_audio_time = 0;
	unsigned int mix_data_size = 0;
	char sourse_data[MAX_MIX_NUM][2];
	short mix_data;
	char *buffer = g_context->mixer_push_buffer;
	if(!buffer)
		return 0;


	unsigned int i = 0;
	unsigned int j = 0;
	unsigned int mix_count = 0;
	unsigned int count = 0;


	//重采样
	ResampleAudio();

	while(IsHaveEnoughDataToMixer())
	{

		suntech_mx_lock(g_context->audio_track_context_lock);

		count = suntech_list_count(g_context->audio_track_context_list);
		//获得音频值

		mix_count = count > MAX_MIX_NUM ? MAX_MIX_NUM : count;

		AudioTrackContext *audio_track_context_list[mix_count];

		//LOGE("DoAudioMixer mix_count:%d ",mix_count);


		for ( i = 0; i < mix_count;i++)
		{
			int get_data_len = 0;
			AudioTrackContext * audio_track_context = (AudioTrackContext*) suntech_list_get(g_context->audio_track_context_list, i);
			loop_buf *pcm_resample_loop_buffer = audio_track_context->pcm_resample_loop_buffer;


			if(0 == mix_data_size)
				mix_data_size = audio_track_context->raw_data_size;
			audio_track_context_list[i] = audio_track_context;

			memset(audio_track_context->raw_data,0,g_context->pcm_mixer_len);

			suntech_mx_lock(audio_track_context->pcm_resample_loop_buffer_lock);
			if(GetDataLen(pcm_resample_loop_buffer)>=g_context->pcm_mixer_len)
				get_data_len =	GetData(pcm_resample_loop_buffer, (char*)audio_track_context->raw_data,g_context->pcm_mixer_len);
			else
			{
				int pad_len = 0;
				get_data_len =	GetData(pcm_resample_loop_buffer, (char*)audio_track_context->raw_data,g_context->pcm_mixer_len);
				pad_len = g_context->pcm_mixer_len-get_data_len;
				memset((char*)audio_track_context->raw_data+get_data_len,0,pad_len);
			}

			suntech_mx_unlock(audio_track_context->pcm_resample_loop_buffer_lock);


			//LOGE("DoAudioMixer audio index:%d mix_data_size:%d",i,mix_data_size);

		}


		//开始混音
		for ( i = 0; i < mix_data_size;i++)
		{
			for ( j = 0; j < mix_count; j++ )
			{
				AudioTrackContext * audio_track_context = audio_track_context_list[j];
				*(short*) sourse_data[j] = audio_track_context->raw_data[i];
			}
			AudioMix(sourse_data,mix_count,(char*)&mix_data);

			//混音后的数据放入pcm_mix_buffer
			suntech_mx_lock(g_context->pcm_mix_buffer_lock);
			PutData(g_context->pcm_mix_buffer, (char*)&mix_data,sizeof(short));
			suntech_mx_unlock(g_context->pcm_mix_buffer_lock);

		}

		suntech_mx_unlock(g_context->audio_track_context_lock);

		if(g_context->is_start_record)
		{
			suntech_mx_lock(g_context->pcm_mix_buffer_lock);

			while( GetDataLen(g_context->pcm_mix_buffer) >= PUSH_DATA_LEN )
			{
				GetData(g_context->pcm_mix_buffer, buffer,PUSH_DATA_LEN);
				if(g_context->_InternalRecordCallback)
					g_context->_InternalRecordCallback(buffer,PUSH_DATA_LEN);

			}
			suntech_mx_unlock(g_context->pcm_mix_buffer_lock);
		}

		mix_audio_time += g_context->mixer_duration;

		//删除AudioTrackContext表项
		RecoveryAudioTrackContext(env);

	}

	LOGI("DoAudioMixer end");

	return 1;
}

static void ClearAllAudioData()
{
	suntech_mx_lock(g_context->audio_track_context_lock);
	for ( unsigned int i = 0; i < suntech_list_count(g_context->audio_track_context_list); i++ )
	{
		AudioTrackContext * audio_track_context = (AudioTrackContext*) suntech_list_get(g_context->audio_track_context_list, i);
		loop_buf *pcm_loop_buffer = audio_track_context->pcm_loop_buffer;
		loop_buf *pcm_resample_loop_buffer = audio_track_context->pcm_resample_loop_buffer;
		ResetLoopBuf(pcm_loop_buffer);
		ResetLoopBuf(pcm_resample_loop_buffer);
	}
	suntech_mx_unlock(g_context->audio_track_context_lock);
}

static unsigned int PcmProduceThreadFun(void *param)
{
	LOGI("PcmProduceThreadFun begin");
	if(!param)
		return 0;

	JNIEnv *env = 0;
	int nead_detach = 0;
	void* virtual_obj = (void*)PCM_PRODUCE_VIRTUAL_OBJ;
	AudioTrackContext * audio_track_context = 0;
	char *pcm_produce_buffer = 0;
	unsigned long long start_time = 0;
	unsigned long long now_time = 0;
	int diff = 0;
	int milliseconds = 0;

	env = hv_get_jni_env(&nead_detach);
	if(!env)
	{
		LOGE("PcmProduceThreadFun env NULL");
		goto end;
	}

	AddAudioTrackContextByObject(env,virtual_obj,DEFAULT_SAMPLE_RATE,DEFAULT_CHANNLE_COUNT,DEFAULT_BIT_COUNT);
	audio_track_context = GetAudioTrackContext(virtual_obj);
	if(!audio_track_context)
	{
		LOGE("PcmProduceThreadFun audio_track_context NULL");
		goto end;
	}

	audio_track_context->play_status = Play_Status_Start;


	pcm_produce_buffer = (char*)malloc(g_context->pcm_mixer_len);
	if(!pcm_produce_buffer)
	{
		LOGE("PcmProduceThreadFun pcm_produce_buffer NULL");
		goto end;
	}

	//LOGI("PcmProduceThreadFun pcm_produce_len:%d",g_context->pcm_mixer_len);

	g_context->audio_time = 0;
	start_time = GetCurSysTime();
	while(!g_context->exit_pcm_produce_thread)
	{

		memset(pcm_produce_buffer,0,g_context->pcm_mixer_len);
		AudioTrackWrite(virtual_obj,pcm_produce_buffer,g_context->pcm_mixer_len);
		g_context->audio_time += g_context->mixer_duration;

		//混音
		if(g_context->is_start_record)
			DoAudioMixer(env);
		else
			ClearAllAudioData();


		now_time = GetCurSysTime();
		milliseconds = now_time-start_time;
		diff = g_context->audio_time-milliseconds;
		if(diff>0)
		{
			mysleep(diff);
		}

		//LOGE("PcmProduceThreadFun diff:%d",diff);
	}



end:
	LOGI("PcmProduceThreadFun end");
	DelAudioTrackContext(audio_track_context);
	if( nead_detach > 0)
		hv_detach_jni_env(env);
	if(pcm_produce_buffer)
		free(pcm_produce_buffer);

	return 1;
}

static unsigned int AudioPlayThreadFun(void *param)
{
	LOGI("AudioPlayThreadFun begin");

	if(!param)
		return 0;

	JNIEnv *env = 0;
	int nead_detach = 0;
	unsigned char* media_buffer = 0;
	unsigned int media_buffer_len = 0;
	unsigned long long audio_time = 0;
	int diff = 0;
	int duration = 200;
	int sample_rate = 44100;
	int channel_count = 2;
	MediaPlayContext* media_play_context = (MediaPlayContext*)param;

	env = hv_get_jni_env(&nead_detach);
	if(!env)
	{
		LOGE("AudioPlayThreadFun env NULL");
		goto end;
	}

	sample_rate = media_play_context->audio_play_af.sample_rate;
	channel_count = media_play_context->audio_play_af.channel_count;

	media_buffer_len = (sample_rate*channel_count*(media_play_context->audio_play_af.bit_count/8))/(1000/duration);

	media_buffer = (unsigned char*) malloc(media_buffer_len);
	if (!media_buffer)
	{
		LOGE("MediaPlayThreadFun media_buffer NULL");
		goto end;
	}

	audio_time =  g_context->audio_time;

	while (!media_play_context->exit_audio_play_thread)
	{
		unsigned int get_len = 0;
		unsigned int data_len = 0;

		if(media_play_context->play_status == Play_Status_Stop && GetDataLen(media_play_context->audio_play_loop_buf) <= 0)
		{
			mysleep(300);
		}


		if( GetDataLen(media_play_context->audio_play_loop_buf) > sizeof(int) )
		{
			GetData(media_play_context->audio_play_loop_buf, (char*)&data_len,sizeof(int));
			if(data_len > 0 )
			{
				if(data_len > media_buffer_len )
				{
					media_buffer_len = data_len;
					media_buffer = (unsigned char*)realloc(media_buffer,media_buffer_len);
					if(!media_buffer)
						media_buffer_len = 0;

				}

				if(media_buffer)
					get_len = GetData(media_play_context->audio_play_loop_buf, (char*)media_buffer,data_len);

			}
		}


		if( get_len > 0 && get_len == data_len)
		{
			AudioTrack_write(media_play_context->audio_track, env, (jbyte*) media_buffer, 0,get_len);
			duration = ((float) get_len / (float) (sample_rate * channel_count * 2)) * 1000;
			audio_time += duration;
		}
		else
			mysleep(10);


		diff = audio_time -  g_context->audio_time;

		if(media_play_context->play_status == Play_Status_Pause)
			audio_time =  g_context->audio_time;
		//LOGI("AudioPlayThreadFun  diff:%d",diff);


	}

	if(media_buffer)
		free(media_buffer);


end:

	if( nead_detach > 0)
		hv_detach_jni_env(env);
    return 1;

	LOGI("AudioPlayThreadFun end");
}

static unsigned int MediaPlayPreDecodeThreadFun(void *param)
{
	LOGI("MediaPlayPreDecodeThreadFun begin");

	if(!param)
		return 0;


	JNIEnv *env = 0;
	int nead_detach = 0;
	std::string file_name;
	char music_file[256] = {0};
	unsigned char *media_buffer = 0;
	unsigned char* hw_out_data = 0;
	jobject media_format = 0;
	char *p_mime = 0;
	int sample_rate = 0;
	int channel_count = 0;
	int channel_out = 0;
	int min_buffer_size = 0;
	bool saw_input_eos = false;
	AudioManager_jni audio_manager;
	MediaPlayContext* media_play_context = (MediaPlayContext*)param;
	unsigned long long audio_time = 0;
	unsigned long long start_time = 0;
	unsigned long long now_time = 0;
	unsigned long long milliseconds = 0;
	int diff = 0;
	int duration = 0;
	bool is_start_audio_play_th = false;
	char* decode_buffer = 0;
	int decode_buffer_len = media_play_context->length*5;		//5倍文件大小
	int cur_decode_data_len = 0;
	int read_pos = 0;
	int write_audiotrack_len = 4096;


	env = hv_get_jni_env(&nead_detach);
	if(!env)
	{
		LOGE("MediaPlayPreDecodeThreadFun env NULL");
		goto end;
	}

	//预先分配解码缓存
	decode_buffer = (char*)malloc(decode_buffer_len);
	if(!decode_buffer)
		goto end;

	//初始化AudioManager
	if( JNI_OK != audio_manager.InitJNI(env) )
	{
		LOGE("MediaPlayPreDecodeThreadFun audio_manager.InitJNI failed");
		goto end;
	}


	media_play_context->_media_codec_context = init_media_codec(env);
	if (!media_play_context->_media_codec_context)
	{
		LOGE("AddMediaPlayContextByFileDescriptor media_play_context->media_codec_context NULL");
		goto end;
	}

	media_play_context->audio_track = InitAudioTrack_jni(env);
	if (!media_play_context->audio_track)
	{
		LOGE("AddMediaPlayContextByFileDescriptor media_play_context->audio_track NULL");
			goto end;
	}

	file_name = io_resolve_descriptor(media_play_context->fd);

	//设置url
	sprintf(music_file, "%s/music_%ld_%ld", MUSIC_FILE_PATH,media_play_context->offset, media_play_context->length);
	if (!file_exists(music_file))
	{
		LOGE("MediaPlayPreDecodeThreadFun 文件%s不存在", music_file);
		goto end;
	}

	media_play_context->media_extractor = (MediaExtractor_jni*) InitMediaExtractor_jni(env);
	if (!media_play_context->media_extractor)
	{
		LOGE("MediaPlayPreDecodeThreadFun media_play_context->media_extractor NULL");
		goto end;
	}

	if(!MediaExtractor_setDataSource(media_play_context->media_extractor, env, music_file))
	{
		LOGE("MediaPlayPreDecodeThreadFun MediaExtractor_setDataSource failed");
		goto end;
	}
	MediaExtractor_selectTrack(media_play_context->media_extractor, env, 0);


	media_format = MediaExtractor_getMediaFormat(media_play_context->media_extractor, env);
	p_mime = MediaExtractor_getmime(media_play_context->media_extractor, env);
	sample_rate = MediaExtractor_getSampleRate(media_play_context->media_extractor, env);
	channel_count = MediaExtractor_getChannelCount(media_play_context->media_extractor, env);
	if (channel_count == 1)
		channel_out = AudioFormat_jni::CHANNEL_OUT_MONO();
	else if (channel_count == 2)
		channel_out = AudioFormat_jni::CHANNEL_OUT_STEREO();
	min_buffer_size = AudioTrack_getMinBufferSize(media_play_context->audio_track, env,sample_rate, channel_out,AudioFormat_jni::ENCODING_PCM_16BIT());

	media_play_context->audio_play_len = min_buffer_size;
	media_play_context->audio_play_af.sample_rate = sample_rate;
	media_play_context->audio_play_af.channel_count = channel_count;
	media_play_context->audio_play_af.bit_count = 16;

	LOGI("MediaPlayPreDecodeThreadFun sample_rate:%d channel_count:%d", sample_rate,channel_count);

	if (!media_format || !p_mime)
	{
		LOGE("MediaPlayPreDecodeThreadFun media_format or p_mime NULL");
		goto end;
	}

	media_buffer = (unsigned char*) malloc(min_buffer_size * 2);
	if (!media_buffer)
	{
		LOGE("MediaPlayPreDecodeThreadFun media_buffer NULL");
		goto end;
	}

	if ( JNI_OK	!= AudioTrack_Config(media_play_context->audio_track, env,
					audio_manager.STREAM_MUSIC(env), sample_rate, channel_out,
					AudioFormat_jni::ENCODING_PCM_16BIT(), min_buffer_size,
					AudioTrack_MODE_STREAM()))
		{
			LOGE("MediaPlayPreDecodeThreadFun AudioTrack_Config failed");
			goto end;
		}

	AudioTrack_play(media_play_context->audio_track, env);
	AudioTrack_setStereoVolume(media_play_context->audio_track, env, 0, 0);

	if (!media_codec_start(env, media_play_context->_media_codec_context, p_mime,media_format))
	{
		LOGE("MediaPlayPreDecodeThreadFun media_codec_start failed");
		goto end;
	}

	//解码
	while ( !saw_input_eos && !media_play_context->exit_media_play_thread)
	{
		long sample_time = 0;
		int sample_size = 0;
		int index = -1;
		int64_t hw_out_timeUs = 0;
		int64_t hw_timeout = 0;
		int hw_out_flags = 0;
		int hw_out_len = 0;

		if (!saw_input_eos)
		{
			sample_size = MediaExtractor_readSampleData(media_play_context->media_extractor,env, media_buffer, min_buffer_size, 0);
			if (sample_size < 0)
			{
				saw_input_eos = true;
				sample_size = 0;
				LOGI("MediaPlayPreDecodeThreadFun MediaExtractor_readSampleData len < 0");
			}
			else
			{
				sample_time = MediaExtractor_getSampleTime(media_play_context->media_extractor,env);
			}

			media_codec_decode(env, media_play_context->_media_codec_context, media_buffer,sample_size, sample_time);

			if (!saw_input_eos)
			{
				MediaExtractor_advance(media_play_context->media_extractor, env);
			}


			index = media_codec_get_frame(env, media_play_context->_media_codec_context,&hw_out_data, &hw_out_len, &hw_out_timeUs,&hw_out_flags, hw_timeout);
			if (hw_out_len > 0)
			{
				int write_pos = cur_decode_data_len;
				cur_decode_data_len += hw_out_len;
				if( cur_decode_data_len > decode_buffer_len)
				{
					decode_buffer = (char*)realloc(decode_buffer,cur_decode_data_len*10);
					decode_buffer_len = cur_decode_data_len*10;
					if(!decode_buffer)
						goto end;
				}

				memcpy(decode_buffer+write_pos,hw_out_data,hw_out_len);

				if (index >= 0)
					media_codec_release_output(env, media_play_context->_media_codec_context, index);

				//mysleep(10);

			}

		}
	}


	duration = ((float) write_audiotrack_len / (float) (sample_rate * channel_count * 2)) * 1000;

	//播放
	do{

		while(!media_play_context->exit_media_play_thread)
		{
			if( media_play_context->play_status == Play_Status_Start )
			{
				start_time = GetCurSysTime();
				break;
			}
			else
			{
				mysleep(50);
				continue;
			}

		}


		while(!media_play_context->exit_media_play_thread)
		{

			if (media_play_context->play_status == Play_Status_Pause
							|| media_play_context->play_status == Play_Status_Idle)
			{
				//LOGI("MediaPlayPreDecodeThreadFun pause play");
				mysleep(100);
				continue;
			}
			else if (media_play_context->play_status == Play_Status_Stop)
			{
				LOGI("MediaPlayPreDecodeThreadFun stop play");
				read_pos = 0;
				audio_time = 0;
				mysleep(100);
				break;
			}


			if( read_pos+write_audiotrack_len <=  cur_decode_data_len)
			{
				AudioTrack_write(media_play_context->audio_track, env, (jbyte*) decode_buffer+read_pos, 0,write_audiotrack_len);
				read_pos += write_audiotrack_len;
				audio_time += duration;

				now_time = GetCurSysTime();
				milliseconds = now_time - start_time;

				diff = audio_time -  milliseconds;
				if(diff>50)
					mysleep(diff/2);

				//LOGI("MediaPlayPreDecodeThreadFun diff:%d",diff);
			}
			else
				break;
		}

		read_pos = 0;
		audio_time = 0;
	}while(media_play_context->isLooping && !media_play_context->exit_media_play_thread);


end:

	//停止解码器
	if(media_play_context->_media_codec_context)
		media_codec_stop(env, media_play_context->_media_codec_context);
	if(media_play_context->audio_track)
	{
		AudioTrack_flush(media_play_context->audio_track, env);
		AudioTrack_release(media_play_context->audio_track, env);
	}

	if(media_play_context->media_extractor)
		UninitMediaExtractor_jni(media_play_context->media_extractor, env);

	if(media_play_context->_media_codec_context)
		uninit_media_codec(env, media_play_context->_media_codec_context);

	if(media_play_context->audio_track)
		UninitAudioTrack_jni(media_play_context->audio_track, env);

	if( nead_detach > 0)
		hv_detach_jni_env(env);
	if(media_buffer)
		free(media_buffer);
	if(decode_buffer)
		free(decode_buffer);
	LOGE("MediaPlayPreDecodeThreadFun end");


	return 1;
}

static unsigned int MediaPlayThreadFun(void *param)
{
	LOGI("MediaPlayThreadFun begin");

	if(!param)
		return 0;


	JNIEnv *env = 0;
	int nead_detach = 0;
	std::string file_name;
	char music_file[256] = {0};
	unsigned char *media_buffer = 0;
	unsigned char* hw_out_data = 0;
	jobject media_format = 0;
	char *p_mime = 0;
	int sample_rate = 0;
	int channel_count = 0;
	int channel_out = 0;
	int min_buffer_size = 0;
	bool saw_input_eos = false;
	AudioManager_jni audio_manager;
	MediaPlayContext* media_play_context = (MediaPlayContext*)param;
	unsigned long long audio_time = 0;
	unsigned long long start_time = 0;
	unsigned long long now_time = 0;
	unsigned long long milliseconds = 0;
	int diff = 0;
	int duration = 0;
	bool is_init = false;
	bool is_start_audio_play_th = false;


	env = hv_get_jni_env(&nead_detach);
	if(!env)
	{
		LOGE("MediaPlayThreadFun env NULL");
		goto end;
	}

	//初始化AudioManager
	if( JNI_OK != audio_manager.InitJNI(env) )
	{
		LOGE("MediaPlayThreadFun audio_manager.InitJNI failed");
		goto end;
	}


	media_play_context->_media_codec_context = init_media_codec(env);
	if (!media_play_context->_media_codec_context)
	{
		LOGE("AddMediaPlayContextByFileDescriptor media_play_context->media_codec_context NULL");
		goto end;
	}

	media_play_context->audio_track = InitAudioTrack_jni(env);
	if (!media_play_context->audio_track)
	{
		LOGE("AddMediaPlayContextByFileDescriptor media_play_context->audio_track NULL");
			goto end;
	}

	file_name = io_resolve_descriptor(media_play_context->fd);

	//设置url
	sprintf(music_file, "%s/music_%ld_%ld", MUSIC_FILE_PATH,media_play_context->offset, media_play_context->length);
	if (!file_exists(music_file))
	{
		LOGE("MediaPlayThreadFun 文件%s不存在", music_file);
		goto end;
	}

	//如果media_play_context->isLooping为true则循环播放
	do
	{
		media_play_context->media_extractor = (MediaExtractor_jni*) InitMediaExtractor_jni(env);
		if (!media_play_context->media_extractor)
		{
			mysleep(100);
			LOGE("AddMediaPlayContextByFileDescriptor media_play_context->media_extractor NULL");
			goto loop;
		}

		if(!MediaExtractor_setDataSource(media_play_context->media_extractor, env, music_file))
		{
			mysleep(100);
			LOGE("MediaPlayThreadFun MediaExtractor_setDataSource failed");
			goto loop;
		}
		MediaExtractor_selectTrack(media_play_context->media_extractor, env, 0);

		if(!is_init)
		{
			media_format = MediaExtractor_getMediaFormat(media_play_context->media_extractor, env);
			p_mime = MediaExtractor_getmime(media_play_context->media_extractor, env);
			sample_rate = MediaExtractor_getSampleRate(media_play_context->media_extractor, env);
			channel_count = MediaExtractor_getChannelCount(media_play_context->media_extractor, env);
			if (channel_count == 1)
				channel_out = AudioFormat_jni::CHANNEL_OUT_MONO();
			else if (channel_count == 2)
				channel_out = AudioFormat_jni::CHANNEL_OUT_STEREO();
			min_buffer_size = AudioTrack_getMinBufferSize(media_play_context->audio_track, env,sample_rate, channel_out,AudioFormat_jni::ENCODING_PCM_16BIT());

			media_play_context->audio_play_len = min_buffer_size;
			media_play_context->audio_play_af.sample_rate = sample_rate;
			media_play_context->audio_play_af.channel_count = channel_count;
			media_play_context->audio_play_af.bit_count = 16;

			LOGI("MediaPlayThreadFun sample_rate:%d channel_count:%d", sample_rate,channel_count);

			if (!media_format || !p_mime)
			{
				LOGE("MediaPlayThreadFun media_format or p_mime NULL");
				goto end;
			}

			media_buffer = (unsigned char*) malloc(min_buffer_size * 2);
			if (!media_buffer)
			{
				LOGE("MediaPlayThreadFun media_buffer NULL");
				goto end;
			}

			if ( JNI_OK	!= AudioTrack_Config(media_play_context->audio_track, env,
						audio_manager.STREAM_MUSIC(env), sample_rate, channel_out,
						AudioFormat_jni::ENCODING_PCM_16BIT(), min_buffer_size,
						AudioTrack_MODE_STREAM()))
			{
				LOGE("MediaPlayThreadFun AudioTrack_Config failed");
				goto end;
			}

			AudioTrack_play(media_play_context->audio_track, env);
			AudioTrack_setStereoVolume(media_play_context->audio_track, env, 0, 0);


			if (!media_codec_start(env, media_play_context->_media_codec_context, p_mime,media_format))
			{
				LOGE("MediaPlayThreadFun media_codec_start failed");
				goto end;
			}
			is_init = true;
		}//init

		//等待开始播放事件
		while(!media_play_context->exit_media_play_thread)
		{
			if( media_play_context->play_status == Play_Status_Start )
			{
				break;
			}
			else
			{
				mysleep(10);
				continue;
			}

		}

		//启动音频播放线程
		if(!is_start_audio_play_th)
		{
			suntech_thread_run(media_play_context->audio_play_thread, AudioPlayThreadFun, media_play_context);
			is_start_audio_play_th = true;
		}

		start_time = GetCurSysTime();
		if( audio_time == 0 )
			audio_time =  g_context->audio_time;

		//播放音频文件
		while ( !saw_input_eos && !media_play_context->exit_media_play_thread)
		{
			long sample_time = 0;
			int sample_size = 0;
			int index = -1;
			int64_t hw_out_timeUs = 0;
			int64_t hw_timeout = 0;
			int hw_out_flags = 0;
			int hw_out_len = 0;

			if (media_play_context->play_status == Play_Status_Pause
					|| media_play_context->play_status == Play_Status_Idle)
			{
				//LOGI("MediaPlayThreadFun pause play");
				mysleep(50);
				continue;
			}
			else if (media_play_context->play_status == Play_Status_Stop)
			{
				LOGI("MediaPlayThreadFun stop play");
				break;
			}

			if (!saw_input_eos)
			{
				sample_size = MediaExtractor_readSampleData(media_play_context->media_extractor,env, media_buffer, min_buffer_size, 0);
				if (sample_size < 0)
				{
					saw_input_eos = true;
					sample_size = 0;
					LOGI("MediaPlayThreadFun MediaExtractor_readSampleData len < 0");
				}
				else
				{
					sample_time = MediaExtractor_getSampleTime(media_play_context->media_extractor,env);
				}

				media_codec_decode(env, media_play_context->_media_codec_context, media_buffer,sample_size, sample_time);

				if (!saw_input_eos)
				{
					MediaExtractor_advance(media_play_context->media_extractor, env);
				}


				index = media_codec_get_frame(env, media_play_context->_media_codec_context,&hw_out_data, &hw_out_len, &hw_out_timeUs,&hw_out_flags, hw_timeout);
				if (hw_out_len > 0)
				{
					bool data_puted = false;

					while(!data_puted && !media_play_context->exit_media_play_thread)
					{
						unsigned int data_len = hw_out_len+sizeof(int);
						if(GetIdleBufLen(media_play_context->audio_play_loop_buf) >= data_len)
						{
							PutData(media_play_context->audio_play_loop_buf, (char*)&hw_out_len, sizeof(int));
							PutData(media_play_context->audio_play_loop_buf, (char*)hw_out_data,hw_out_len);
							data_puted = true;

							break;
						}
						else
						{
							if(diff>0)
								mysleep(diff/10);
						}


					}


					duration = ((float) hw_out_len / (float) (sample_rate * channel_count * 2)) * 1000;
					audio_time += duration;

					LOGI("MediaPlayThreadFun duration:%d hw_out_len:%d", duration,hw_out_len);
				}

				if (index >= 0)
					media_codec_release_output(env, media_play_context->_media_codec_context, index);

				now_time = GetCurSysTime();
				milliseconds = now_time - start_time;
				diff = audio_time -  g_context->audio_time;
				if (diff > 100 )
				{
					mysleep(diff/2);

				}
				//LOGE("++++++++++++++++++++++++++++++++++++++++++++++++++++ diff:%d",diff);
				//LOGI("MediaPlayThreadFun diff:%d",diff);

			}

		}

loop:
		if(media_play_context->media_extractor)
		{
			UninitMediaExtractor_jni(media_play_context->media_extractor, env);
			media_play_context->media_extractor = 0;
		}

		saw_input_eos = false;

	}while(media_play_context->isLooping && !media_play_context->exit_media_play_thread);

	audio_manager.UninitJNI(env);


end:

	//停止解码器
	if(media_play_context->_media_codec_context)
		media_codec_stop(env, media_play_context->_media_codec_context);
	if(media_play_context->audio_track)
	{
		AudioTrack_flush(media_play_context->audio_track, env);
		AudioTrack_release(media_play_context->audio_track, env);
	}

	if(media_play_context->media_extractor)
		UninitMediaExtractor_jni(media_play_context->media_extractor, env);

	if(media_play_context->_media_codec_context)
		uninit_media_codec(env, media_play_context->_media_codec_context);

	if(media_play_context->audio_track)
		UninitAudioTrack_jni(media_play_context->audio_track, env);



	if( nead_detach > 0)
		hv_detach_jni_env(env);
	if(media_buffer)
		free(media_buffer);
	LOGI("MediaPlayThreadFun end");

	return 1;
}

static unsigned int SoundPoolThreadFun(void *param)
{
	LOGI("SoundPoolThreadFun begin");

	if(!param)
		return 0;

	JNIEnv *env = 0;
	int nead_detach = 0;
	std::string file_name;
	char music_file[256] = {0};
	unsigned char *media_buffer = 0;
	unsigned char* hw_out_data = 0;
	jobject media_format = 0;
	char *p_mime = 0;
	int sample_rate = 0;
	int channel_count = 0;
	int channel_out = 0;
	int min_buffer_size = 0;
	bool saw_input_eos = false;
	MediaExtractor_jni* media_extractor = 0;
	media_codec_context *_media_codec_context = 0;
	AudioTrack_jni *audio_track = 0;
	AudioManager_jni audio_manager;
	SoundPoolContext*sound_pool_context = (SoundPoolContext*)param;
	int decode_buffer_len = 0;
	int decode_data_len = 0;

	env = hv_get_jni_env(&nead_detach);
	if(!env)
	{
		LOGE("SoundPoolThreadFun env NULL");
		return 0;
	}


	//初始化AudioManager
	if( JNI_OK != audio_manager.InitJNI(env) )
	{
		LOGE("SoundPoolThreadFun audio_manager.InitJNI failed");
		goto end;
	}


	sound_pool_context->media_extractor = (MediaExtractor_jni*)InitMediaExtractor_jni(env);
	if(!sound_pool_context->media_extractor)
	{
		LOGE("SoundPoolThreadFun sound_pool_context->media_extractor NULL");
		goto end;
	}

	sound_pool_context->_media_codec_context = init_media_codec(env);
	if(!sound_pool_context->_media_codec_context)
	{
		LOGE("SoundPoolThreadFun sound_pool_context->media_codec_context NULL");
		goto end;
	}

	sound_pool_context->audio_track = InitAudioTrack_jni(env);
	if(!sound_pool_context->audio_track)
	{
		LOGE("SoundPoolThreadFun sound_pool_context->audio_track NULL");
		goto end;
	}

	media_extractor = sound_pool_context->media_extractor;
	_media_codec_context = sound_pool_context->_media_codec_context;
	audio_track = sound_pool_context->audio_track;
	file_name = io_resolve_descriptor(sound_pool_context->fd);

	//设置url
	sprintf(music_file,"%s/music_%ld_%ld",MUSIC_FILE_PATH,sound_pool_context->offset,sound_pool_context->length);
	if(!file_exists(music_file))
	{
		LOGE("SoundPoolThreadFun 文件%s不存在",music_file);
		goto end;
	}


	if(!MediaExtractor_setDataSource(media_extractor,env,music_file))
	{
		LOGE("MediaExtractor_setDataSource failed");
		goto end;
	}


	media_format = MediaExtractor_getMediaFormat(media_extractor,env);
	p_mime = MediaExtractor_getmime(media_extractor,env);
	sample_rate = MediaExtractor_getSampleRate(media_extractor,env);
	channel_count = MediaExtractor_getChannelCount(media_extractor,env);
	if( channel_count == 1 )
		channel_out = AudioFormat_jni::CHANNEL_OUT_MONO();
	else if( channel_count == 2 )
		channel_out = AudioFormat_jni::CHANNEL_OUT_STEREO();
	min_buffer_size = AudioTrack_getMinBufferSize(audio_track,env,sample_rate,channel_out,AudioFormat_jni::ENCODING_PCM_16BIT());
	LOGI("SoundPoolThreadFun sample_rate:%d channel_count:%d",sample_rate,channel_count);

	if(!media_format || !p_mime )
	{
		LOGE("SoundPoolThreadFun media_format or p_mime NULL");
		goto end;
	}

	media_buffer = (unsigned char*)malloc(min_buffer_size*2);
	if(!media_buffer)
	{
		LOGE("SoundPoolThreadFun media_buffer NULL");
		goto end;
	}

	if( JNI_OK != AudioTrack_Config(audio_track,env,audio_manager.STREAM_MUSIC(env),sample_rate,channel_out,AudioFormat_jni::ENCODING_PCM_16BIT(),min_buffer_size,AudioTrack_MODE_STREAM()) )
	{
		LOGE("SoundPoolThreadFun AudioTrack_Config failed");
		goto end;
	}

	//AudioTrack_play(audio_track,env);
	AudioTrack_setStereoVolume(audio_track,env,0,0);
	MediaExtractor_selectTrack(media_extractor,env,0);

	if( !media_codec_start(env,_media_codec_context,p_mime,media_format) )
	{
		LOGE("SoundPoolThreadFun media_codec_start failed");
		goto end;
	}


	//先解码
	while( !saw_input_eos && !sound_pool_context->exit_sound_pool_thread)
	{
		long sample_time = 0;
		int sample_size = 0;
		int index = -1;
		int64_t hw_out_timeUs = 0;
		int64_t hw_timeout = 10;
		int hw_out_flags = 0;
		int hw_out_len = 0;

		if(!saw_input_eos)
		{
			sample_size = MediaExtractor_readSampleData(media_extractor,env,media_buffer,min_buffer_size,0);
			if( sample_size < 0 )
			{
				saw_input_eos = true;
				sample_size = 0;
				LOGI("SoundPoolThreadFun MediaExtractor_readSampleData len < 0");
			}
			else
			{
				sample_time = MediaExtractor_getSampleTime(media_extractor,env);
			}

			media_codec_decode(env,_media_codec_context,media_buffer,sample_size,sample_time);

			 if (!saw_input_eos)
			 {
				 MediaExtractor_advance(media_extractor,env);
			 }

			 index = media_codec_get_frame(env,_media_codec_context,&hw_out_data,&hw_out_len,&hw_out_timeUs,&hw_out_flags,hw_timeout);

			 if(hw_out_len>0)
			 {
				 decode_buffer_len += hw_out_len;
				 if(!sound_pool_context->decode_buffer)
				 {
					 sound_pool_context->decode_buffer = (char*)malloc(decode_buffer_len);
				 }
				 else
				 {
					 sound_pool_context->decode_buffer = (char*)realloc(sound_pool_context->decode_buffer,decode_buffer_len);
				 }

				 if(sound_pool_context->decode_buffer)
				 {
					 memcpy(sound_pool_context->decode_buffer+decode_data_len,hw_out_data,hw_out_len);
					 decode_data_len += hw_out_len;
				 }
			 }

			 if(index>0)
			 	media_codec_release_output(env,_media_codec_context,index);
		}


	}

	//等待播放
	while(!sound_pool_context->exit_sound_pool_thread && sound_pool_context->play_status != Play_Status_Stop)
	{
		suntech_sema_wait(sound_pool_context->play_sema);

		if( sound_pool_context->Looping > 0 )
		{
			for ( int i = 0; i < sound_pool_context->Looping; i++ )
			{

				if(sound_pool_context->play_status == Play_Status_Stop)
					break;

				AudioTrack_write(audio_track,env,(jbyte*) sound_pool_context->decode_buffer,0,decode_data_len);
				g_context->audio_effect_counter++;
				mysleep(100);
			}
		}
		else
		{
			AudioTrack_play(audio_track,env);
			AudioTrack_write(audio_track,env,(jbyte*) sound_pool_context->decode_buffer,0,decode_data_len);
			AudioTrack_pause(audio_track,env);
			g_context->audio_effect_counter++;
		}

	}

	//停止解码器
	media_codec_stop(env,_media_codec_context);
	AudioTrack_flush(audio_track,env);
	AudioTrack_release(audio_track,env);
	audio_manager.UninitJNI(env);
	if(sound_pool_context->decode_buffer)
		free(sound_pool_context->decode_buffer);

end:
	audio_manager.UninitJNI(env);
	if(sound_pool_context->media_extractor)
		UninitMediaExtractor_jni(sound_pool_context->media_extractor,env);
	if(sound_pool_context->_media_codec_context)
		uninit_media_codec(env,sound_pool_context->_media_codec_context);
	if(sound_pool_context->audio_track)
		UninitAudioTrack_jni(sound_pool_context->audio_track,env);
	if(media_buffer)
		free(media_buffer);
	if( nead_detach > 0)
		hv_detach_jni_env(env);
	LOGI("SoundPoolThreadFun end");
	return 1;
}

static MediaPlayContext* GetMediaPlayContext(void *obj)
{
	LOGI("GetMediaPlayContext begin");

	if( !obj || !g_context )
		return 0;

	MediaPlayContext * media_play_context = 0;

	suntech_mx_lock(g_context->media_play_context_list_lock);

	for( unsigned int i = 0; i < suntech_list_count(g_context->media_play_context_list); i++  )
	{
		media_play_context = (MediaPlayContext*)suntech_list_get(g_context->media_play_context_list,i);
		if( media_play_context )
		{
			if( obj ==  media_play_context->media_cls_obj )
				break;

			media_play_context = 0;
		}
	}

	suntech_mx_unlock(g_context->media_play_context_list_lock);

	return media_play_context;

	LOGI("GetMediaPlayContext end");
}

bool AddMediaPlayContextByFileDescriptor(JNIEnv *env,void *media_cls_obj,int fd,long offset,long length)
{
	LOGI("AddMediaPlayContextByFileDescriptor begin");

	bool ret = false;

	if(!g_context)
		return ret;

	if(!env || fd < 0 )
		return ret;

	MediaPlayContext* media_play_context = 0;
	media_play_context  = (MediaPlayContext*)malloc(sizeof(MediaPlayContext));
	if(!media_play_context)
		return ret;


	memset(media_play_context,0,sizeof(MediaPlayContext));

	media_play_context->fd = fd;
	media_play_context->offset = offset;
	media_play_context->length = length;
	media_play_context->media_cls_obj = media_cls_obj;
	media_play_context->play_status = Play_Status_Idle;

	//创建音频缓存锁
	media_play_context->audio_play_loop_buf_lock = suntech_mx_new();
	if(!media_play_context->audio_play_loop_buf_lock)
	{
		LOGE("AddMediaPlayContextByFileDescriptor media_play_context->audio_play_loop_buf_lock NULL");
		goto failed;
	}

	//创建音频播放缓存
	media_play_context->audio_play_loop_buf = (loop_buf*) malloc(sizeof(loop_buf));
	if(!media_play_context->audio_play_loop_buf)
	{
		LOGE("AddMediaPlayContextByFileDescriptor audio_track_context->pcm_loop_buffer NULL");
		goto failed;
	}

	memset(media_play_context->audio_play_loop_buf,0,sizeof(loop_buf));
	if (0 != InitLoopBuf(media_play_context->audio_play_loop_buf, PCM_BUF_LEN))
	{
		LOGE("AddMediaPlayContextByFileDescriptor InitLoopBuf failed");
		goto failed;
	}

	media_play_context->media_play_thread = suntech_thread_new();
	if(!media_play_context->media_play_thread)
	{
		LOGE("AddMediaPlayContextByFileDescriptor media_play_context->media_play_thread NULL");
		goto failed;
	}

	media_play_context->audio_play_thread = suntech_thread_new();
	if(!media_play_context->audio_play_thread)
	{
		LOGE("AddMediaPlayContextByFileDescriptor media_play_context->audio_play_thread NULL");
		goto failed;
	}

	suntech_mx_lock(g_context->media_play_context_list_lock);
	suntech_list_add(g_context->media_play_context_list,media_play_context);
	suntech_mx_unlock(g_context->media_play_context_list_lock);

	//文件小于1M则用预先解码模式
	if ( length <= 1024*1024 )
		suntech_thread_run(media_play_context->media_play_thread, MediaPlayPreDecodeThreadFun, media_play_context);
	else
		suntech_thread_run(media_play_context->media_play_thread, MediaPlayThreadFun, media_play_context);

	LOGI("CreateMediaPlayContextByFileDescriptor end");

	return true;

failed:
	if(media_play_context->media_play_thread)
	{
		suntech_thread_del(media_play_context->media_play_thread);
	}
	if(media_play_context->audio_play_loop_buf)
	{
		UninitLoopBuf(media_play_context->audio_play_loop_buf);
		free(media_play_context->audio_play_loop_buf);
	}
	if(media_play_context->audio_play_loop_buf_lock)
		suntech_mx_del(media_play_context->audio_play_loop_buf_lock);

	if(media_play_context)
		free(media_play_context);

	LOGE("CreateMediaPlayContextByFileDescriptor failed");

	return false;
}


static void DelMediaPlayContext(MediaPlayContext *media_play_context)
{
	LOGI("DelMediaPlayContext begin");

	if( !media_play_context)
		return;

	suntech_mx_lock(g_context->media_play_context_list_lock);

	media_play_context->exit_media_play_thread = true;
	if(media_play_context->media_play_thread)
	{
		suntech_thread_del(media_play_context->media_play_thread);
		media_play_context->media_play_thread = 0;
	}

	if(media_play_context->audio_play_loop_buf)
	{
		UninitLoopBuf(media_play_context->audio_play_loop_buf);
		free(media_play_context->audio_play_loop_buf);
	}

	if(media_play_context->audio_play_loop_buf_lock)
		suntech_mx_del(media_play_context->audio_play_loop_buf_lock);

	suntech_list_del_item(g_context->media_play_context_list,media_play_context);

	suntech_mx_unlock(g_context->media_play_context_list_lock);

	LOGI("DelMediaPlayContext begin");
}

static void DelMediaPlayContextItem(void *obj)
{
	LOGI("RemMediaPlayContext begin");

	if(  !obj )
		return;

	suntech_mx_lock(g_context->media_play_context_list_lock);

	MediaPlayContext* media_play_context = GetMediaPlayContext(obj);
	if(media_play_context)
		DelMediaPlayContext(media_play_context);

	suntech_mx_unlock(g_context->media_play_context_list_lock);

	LOGI("RemMediaPlayContext end");
}

static bool MediaPlaySetDataSource(void *obj,int fd,long offset,long length)
{
	LOGI("MediaPlaySetDataSource begin");

	int nead_detach = 0;

	if( !g_context || !obj || length <= 0)
		return false;

	JNIEnv *env = hv_get_jni_env(&nead_detach);
	if(!env)
		return false;

	AddMediaPlayContextByFileDescriptor(env,obj,fd,offset,length);

	if( nead_detach > 0)
		hv_detach_jni_env(env);

	LOGI("MediaPlaySetDataSource end");

	return true;
}

static SoundPoolContext* GetSoundPoolContext(void *obj,int sound_id)
{
	LOGI("GetSoundPoolContext begin");

	if( !obj || !g_context )
		return 0;

	SoundPoolContext* sound_pool_context = 0;

	suntech_mx_lock(g_context->sound_pool_context_list_lock);

	for( unsigned int i = 0; i < suntech_list_count(g_context->sound_pool_context_list); i++  )
	{
		sound_pool_context = (SoundPoolContext*)suntech_list_get(g_context->sound_pool_context_list,i);
		if( sound_pool_context )
		{
			if( obj ==  sound_pool_context->sound_pool_cls_obj &&  sound_id == sound_pool_context->sound_id)
				break;

			sound_pool_context = 0;
		}
	}

	suntech_mx_unlock(g_context->sound_pool_context_list_lock);

	return sound_pool_context;

	LOGI("GetSoundPoolContext end");
}

bool AddSoundPoolContextByFileDescriptor(JNIEnv *env,void *sound_pool_cls_obj,int sound_id,int fd,long offset,long length)
{
	LOGI("AddSoundPoolContextByFileDescriptor begin");

	bool ret = false;

	if(!g_context)
		return ret;

	if(!env || fd < 0 )
		return ret;

	SoundPoolContext* sound_pool_context = 0;

	sound_pool_context  = (SoundPoolContext*)malloc(sizeof(SoundPoolContext));
	if(!sound_pool_context)
		return ret;

	memset(sound_pool_context,0,sizeof(SoundPoolContext));

	sound_pool_context->fd = fd;
	sound_pool_context->offset = offset;
	sound_pool_context->length = length;
	sound_pool_context->sound_id = sound_id;
	sound_pool_context->sound_pool_cls_obj = sound_pool_cls_obj;
	sound_pool_context->play_status = Play_Status_Idle;

	sound_pool_context->play_sema = suntech_sema_new(1,0);
	if(!sound_pool_context->play_sema)
	{
		LOGE("AddSoundPoolContextByFileDescriptor create play_sema failed");
		goto failed;
	}

	suntech_mx_lock(g_context->sound_pool_context_list_lock);
	suntech_list_add(g_context->sound_pool_context_list,sound_pool_context);
	suntech_mx_unlock(g_context->sound_pool_context_list_lock);

	//开始SoundPool线程
	sound_pool_context->sound_pool_thread = suntech_thread_new();
	if(!sound_pool_context->sound_pool_thread)
	{
		LOGE("AddSoundPoolContextByFileDescriptor sound_pool_context->media_play_thread NULL");
		goto failed;
	}

	sound_pool_context->exit_sound_pool_thread = false;
	sound_pool_context->play_status = Play_Status_Idle;
	suntech_thread_run(sound_pool_context->sound_pool_thread, SoundPoolThreadFun, sound_pool_context);


	LOGI("AddSoundPoolContextByFileDescriptor end");

	return true;

failed:

	if(sound_pool_context->play_sema)
		suntech_sema_del(sound_pool_context->play_sema);

	free(sound_pool_context);

	LOGE("AddSoundPoolContextByFileDescriptor failed");

	return false;
}

static void DelSoundPoolContext(JNIEnv *env,SoundPoolContext* sound_pool_context)
{
	LOGI("DelSoundPoolContext begin");

	if( !env || !sound_pool_context)
		return;

	sound_pool_context->exit_sound_pool_thread = true;
	suntech_sema_notify(sound_pool_context->play_sema,1);
	suntech_thread_del(sound_pool_context->sound_pool_thread);

	suntech_sema_del(sound_pool_context->play_sema);

	suntech_mx_lock(g_context->sound_pool_context_list_lock);
	suntech_list_del_item(g_context->sound_pool_context_list,sound_pool_context);
	suntech_mx_unlock(g_context->sound_pool_context_list_lock);

	LOGI("DelSoundPoolContext begin");
}


static bool SoundPoolLoad(void *obj,int sound_id,int fd,long offset,long length)
{
	LOGI("SoundPoolLoad begin");

	int nead_detach = 0;

	if( !obj)
		return false;

	JNIEnv *env = hv_get_jni_env(&nead_detach);
	if(!env)
		return false;


	AddSoundPoolContextByFileDescriptor(env,obj,sound_id,fd,offset,length);

	if( nead_detach > 0)
		hv_detach_jni_env(env);

	return true;

	LOGI("SoundPoolLoad end");
}

static void SoundPoolUnLoad(void *obj,int sampleID)
{
	LOGI("SoundPoolUnLoad begin");

	int nead_detach = 0;

	if( !obj )
		return;

	JNIEnv *env = hv_get_jni_env(&nead_detach);
	if(!env)
		return;

	suntech_mx_lock(g_context->sound_pool_context_list_lock);

	SoundPoolContext* sound_pool_context = GetSoundPoolContext(obj,sampleID);
	if(sound_pool_context)
		DelSoundPoolContext(env,sound_pool_context);

	suntech_mx_unlock(g_context->sound_pool_context_list_lock);

	if( nead_detach > 0)
		hv_detach_jni_env(env);

	LOGI("SoundPoolUnLoad end");
}

static bool SoundPoolPlay(void *obj,int sound_id)
{
	LOGI("SoundPoolPlay begin");

	if( !obj)
		return false;

	suntech_mx_lock(g_context->sound_pool_context_list_lock);

	SoundPoolContext* sound_pool_context = GetSoundPoolContext(obj,sound_id);
	if(sound_pool_context)
	{
		sound_pool_context->play_status = Play_Status_Start;
		suntech_sema_notify(sound_pool_context->play_sema,1);
	}

	suntech_mx_unlock(g_context->sound_pool_context_list_lock);

	LOGI("SoundPoolPlay end");

	return true;
}

static bool SoundPoolStop(void *obj,int sound_id)
{
	LOGI("SoundPoolStop begin");

	if( !obj)
		return false;

	suntech_mx_lock(g_context->sound_pool_context_list_lock);

	SoundPoolContext* sound_pool_context = GetSoundPoolContext(obj,sound_id);
	if(sound_pool_context)
	{
		sound_pool_context->play_status = Play_Status_Stop;
		suntech_sema_notify(sound_pool_context->play_sema,1);
	}

	suntech_mx_unlock(g_context->sound_pool_context_list_lock);

	LOGI("SoundPoolStop end");

	return true;
}

static void SoundPoolsetLooping(void *obj,int channelID,int loop)
{
	LOGI("SoundPoolsetLooping begin");

	if( !obj)
		return;

	suntech_mx_lock(g_context->sound_pool_context_list_lock);

	SoundPoolContext* sound_pool_context = GetSoundPoolContext(obj,channelID);
	if(sound_pool_context)
	{
		sound_pool_context->Looping = loop;
	}

	suntech_mx_unlock(g_context->sound_pool_context_list_lock);

	LOGI("SoundPoolsetLooping end");

}

static bool MediaPlayStart(void *obj)
{
	LOGI("MediaPlayStart begin");

	if( !g_context || !obj)
		return false;

	suntech_mx_lock(g_context->media_play_context_list_lock);

	MediaPlayContext* media_play_context = GetMediaPlayContext(obj);
	if(media_play_context)
	{
		//开始媒体播放线程
		media_play_context->play_status = Play_Status_Start;

	}

	suntech_mx_unlock(g_context->media_play_context_list_lock);

	LOGI("MediaPlayStart end");

	return true;
}

static bool MediaPlayStop(void *obj)
{
	LOGI("MediaPlayStop begin");

	if( !g_context  || !obj)
		return false;

	suntech_mx_lock(g_context->media_play_context_list_lock);

	MediaPlayContext* media_play_context = GetMediaPlayContext(obj);
	if(media_play_context)
	{
		//退出播放
		media_play_context->play_status = Play_Status_Stop;
	}

	suntech_mx_unlock(g_context->media_play_context_list_lock);


	LOGI("MediaPlayStop end");

	return true;
}

static bool MediaPlayPause(void *obj)
{
	LOGI("MediaPlayPause begin");

	if( !g_context )
		return false;

	suntech_mx_lock(g_context->media_play_context_list_lock);

	MediaPlayContext* media_play_context = GetMediaPlayContext(obj);
	if(media_play_context)
		media_play_context->play_status = Play_Status_Pause;

	suntech_mx_unlock(g_context->media_play_context_list_lock);

	LOGI("MediaPlayPause end");

	return true;
}

static void MediaPlaySetLooping(void *obj,bool looping)
{
	LOGI("MediaPlaySetLooping begin");

	suntech_mx_lock(g_context->media_play_context_list_lock);

	MediaPlayContext* media_play_context = GetMediaPlayContext(obj);
	if(media_play_context)
		media_play_context->isLooping = looping;

	suntech_mx_unlock(g_context->media_play_context_list_lock);

	LOGI("MediaPlaySetLooping end");
}

static AudioTrackContext* GetAudioTrackContext(void *obj)
{
	//LOGI("GetAudioTrackContext begin");

	if( !obj || !g_context )
		return 0;

	AudioTrackContext * audio_track_context = 0;


	for( unsigned int i = 0; i < suntech_list_count(g_context->audio_track_context_list); i++  )
	{
		audio_track_context = (AudioTrackContext*)suntech_list_get(g_context->audio_track_context_list,i);
		if( audio_track_context )
		{
			if( obj ==  audio_track_context->audio_track_cls_obj )
				break;

			audio_track_context = 0;
		}
	}


	return audio_track_context;

	//LOGI("GetAudioTrackContext end");
}

static bool AddAudioTrackContextByObject(JNIEnv *env,void *obj,unsigned int sample_rate,unsigned int channel_count,unsigned int bit_count)
{
	LOGI("AddAudioTrackContextByObject begin");

	bool ret = false;
	int resample_buffer_len = 0;

	if( !env || !obj )
		return ret;

	AudioTrackContext *audio_track_context = (AudioTrackContext*)malloc(sizeof(AudioTrackContext));
	if(!audio_track_context)
	{
		LOGE("AddAudioTrackContextByObject audio_track_context NULL");
		goto failed;
	}
	memset(audio_track_context,0,sizeof(AudioTrackContext));

	audio_track_context->play_status = Play_Status_Idle;

	//获得音频格式
	audio_track_context->audio_format.sample_rate = sample_rate;
	audio_track_context->audio_format.channel_count = channel_count > 2 ? 2 : channel_count;
	audio_track_context->audio_format.bit_count = bit_count;
	LOGI("AddAudioTrackContextByObject sample_rate:%d channel_count:%d",sample_rate,channel_count);

	//resample context
	if( audio_track_context->audio_format.sample_rate != DEFAULT_SAMPLE_RATE || audio_track_context->audio_format.channel_count != DEFAULT_CHANNLE_COUNT )
	{
		audio_track_context->resample_context = audio_resample_init(DEFAULT_CHANNLE_COUNT,audio_track_context->audio_format.channel_count
				,DEFAULT_SAMPLE_RATE,audio_track_context->audio_format.sample_rate);
		if(!audio_track_context->resample_context)
		{
			LOGE("AddAudioTrackContextByObject resample_context NULL");
			goto failed;
		}

		audio_track_context->nead_resample = true;
	}

	//先分配一秒的长度
	resample_buffer_len =  DEFAULT_SAMPLE_RATE*DEFAULT_CHANNLE_COUNT*(DEFAULT_BIT_COUNT/2);
	audio_track_context->resample_in_buffer_len = resample_buffer_len;
	audio_track_context->resample_in_buffer = (char*)malloc(audio_track_context->resample_in_buffer_len);
	if(!audio_track_context->resample_in_buffer)
	{
		LOGE("AddAudioTrackContextByObject resample_in_buffer NULL");
		goto failed;
	}
	audio_track_context->resample_out_buffer_len = resample_buffer_len;
	audio_track_context->resample_out_buffer = (char*)malloc(audio_track_context->resample_out_buffer_len);
	if(!audio_track_context->resample_out_buffer)
	{
		LOGE("AddAudioTrackContextByObject resample_out_buffer NULL");
		goto failed;
	}

	audio_track_context->raw_data_size = g_context->pcm_mixer_len/2;		//44100HZ 16bit 2channel MIX_DURATION数据长
	audio_track_context->raw_data = (short*)malloc(g_context->pcm_mixer_len);
	if(!audio_track_context->raw_data)
	{
		LOGE("AddAudioTrackContextByObject audio_track_context->raw_data NULL");
		goto failed;
	}

	audio_track_context->pcm_loop_buffer_lock = suntech_mx_new();
	if(!audio_track_context->pcm_loop_buffer_lock)
	{
		LOGE("AddAudioTrackContextByObject audio_track_context->pcm_loop_buffer_lock NULL");
		goto failed;
	}

	audio_track_context->pcm_resample_loop_buffer_lock = suntech_mx_new();
	if(!audio_track_context->pcm_resample_loop_buffer_lock)
	{
		LOGE("AddAudioTrackContextByObject audio_track_context->pcm_resample_loop_buffer_lock NULL");
		goto failed;
	}

	audio_track_context->audio_track_cls_obj = obj;

	//pcm buffer
	audio_track_context->pcm_loop_buffer = (loop_buf*) malloc(sizeof(loop_buf));
	if(!audio_track_context->pcm_loop_buffer)
	{
		LOGE("AddAudioTrackContextByObject audio_track_context->pcm_loop_buffer NULL");
		goto failed;
	}
	memset(audio_track_context->pcm_loop_buffer,0,sizeof(loop_buf));
	if (0 != InitLoopBuf(audio_track_context->pcm_loop_buffer, PCM_BUF_LEN))
	{
		LOGE("AddAudioTrackContextByObject InitLoopBuf failed");
		goto failed;
	}

	//pcm resample buffer
	audio_track_context->pcm_resample_loop_buffer = (loop_buf*) malloc(sizeof(loop_buf));
	if(!audio_track_context->pcm_resample_loop_buffer)
	{
		LOGE("AddAudioTrackContextByObject audio_track_context->pcm_resample_loop_buffer NULL");
		goto failed;
	}
	memset(audio_track_context->pcm_resample_loop_buffer,0,sizeof(loop_buf));
	if (0 != InitLoopBuf(audio_track_context->pcm_resample_loop_buffer, PCM_BUF_LEN))
	{
		LOGE("AddAudioTrackContextByObject InitLoopBuf failed");
		goto failed;
	}

	//音效判断
	audio_track_context->is_audio_effect = false;

	suntech_mx_lock(g_context->audio_track_context_lock);
	suntech_list_add(g_context->audio_track_context_list,audio_track_context);
	suntech_mx_unlock(g_context->audio_track_context_lock);

	LOGI("AddAudioTrackContextByObject end");

	return true;

failed:
	LOGI("AddAudioTrackContextByObject failed");
	if (audio_track_context->pcm_loop_buffer)
	{
		UninitLoopBuf(audio_track_context->pcm_loop_buffer);
		free(audio_track_context->pcm_loop_buffer);
	}

	if (audio_track_context->pcm_resample_loop_buffer)
	{
		UninitLoopBuf(audio_track_context->pcm_resample_loop_buffer);
		free(audio_track_context->pcm_resample_loop_buffer);
	}

	if(audio_track_context->pcm_loop_buffer_lock)
		suntech_mx_del(audio_track_context->pcm_loop_buffer_lock);

	if(audio_track_context->pcm_resample_loop_buffer_lock)
		suntech_mx_del(audio_track_context->pcm_resample_loop_buffer_lock);

	if(audio_track_context->raw_data)
		free(audio_track_context->raw_data);

	if(audio_track_context->resample_context)
		audio_resample_close(audio_track_context->resample_context);

	if(audio_track_context->resample_in_buffer)
		free(audio_track_context->resample_in_buffer);

	if(audio_track_context->resample_out_buffer)
		free(audio_track_context->resample_out_buffer);

	if(audio_track_context)
		free(audio_track_context);

	return ret;
}

static void DelAudioTrackContext(AudioTrackContext *audio_track_context)
{
	LOGI("DelAudioTrackContext begin");

	if( !audio_track_context)
		return;

	suntech_mx_lock(g_context->audio_track_context_lock);

	suntech_mx_lock(audio_track_context->pcm_loop_buffer_lock);
	UninitLoopBuf(audio_track_context->pcm_loop_buffer);
	free(audio_track_context->pcm_loop_buffer);
	suntech_mx_unlock(audio_track_context->pcm_loop_buffer_lock);

	suntech_mx_lock(audio_track_context->pcm_resample_loop_buffer_lock);
	UninitLoopBuf(audio_track_context->pcm_resample_loop_buffer);
	free(audio_track_context->pcm_resample_loop_buffer);
	suntech_mx_unlock(audio_track_context->pcm_resample_loop_buffer_lock);


	suntech_mx_del(audio_track_context->pcm_loop_buffer_lock);
	suntech_mx_del(audio_track_context->pcm_resample_loop_buffer_lock);

	if(audio_track_context->resample_context)
		audio_resample_close(audio_track_context->resample_context);

	if(audio_track_context->resample_in_buffer)
		free(audio_track_context->resample_in_buffer);

	if(audio_track_context->resample_out_buffer)
		free(audio_track_context->resample_out_buffer);


	suntech_list_del_item(g_context->audio_track_context_list,audio_track_context);
	suntech_mx_unlock(g_context->audio_track_context_lock);

	LOGI("DelAudioTrackContext end");
}

static bool AudioTrackSet(void *obj,unsigned int sample_rate,unsigned int channel_count,unsigned int bit_count)
{
	LOGI("AudioTrackSet begin");
	if( !g_context || !obj)
		return false;

	int nead_detach = 0;

	JNIEnv *env = hv_get_jni_env(&nead_detach);
	if(!env)
		return false;

	suntech_mx_lock(g_context->audio_track_context_lock);
	AudioTrackContext * audio_track_context = GetAudioTrackContext(obj);
	if(!audio_track_context)
		AddAudioTrackContextByObject(env,obj,sample_rate,channel_count,bit_count);
	suntech_mx_unlock(g_context->audio_track_context_lock);


	if( nead_detach > 0)
		hv_detach_jni_env(env);


	LOGI("AudioTrackSet begin");

	return true;
}

static void AudioTrackStart(void *obj)
{
	LOGI("AudioTrackStart begin");

	suntech_mx_lock(g_context->audio_track_context_lock);

	AudioTrackContext * audio_track_context = GetAudioTrackContext(obj);
		if(audio_track_context)
			audio_track_context->play_status = Play_Status_Start;

	suntech_mx_unlock(g_context->audio_track_context_lock);

	LOGI("AudioTrackStart end");
}

static void AudioTrackPause(void *obj)
{
	LOGI("AudioTrackPause begin");

	suntech_mx_lock(g_context->audio_track_context_lock);

	AudioTrackContext * audio_track_context = GetAudioTrackContext(obj);
	if(audio_track_context)
		audio_track_context->play_status = Play_Status_Pause;

	suntech_mx_unlock(g_context->audio_track_context_lock);

	LOGI("AudioTrackPause end");
}

static bool AudioTrackRelease(AudioTrackContext * audio_track_context)
{
	LOGI("AudioTrackRelease begin");
	if( !g_context || !audio_track_context)
		return false;

	DelAudioTrackContext(audio_track_context);

	LOGI("AudioTrackRelease end");

	return true;
}

static void AudioTrackStop(void *obj)
{
	LOGI("AudioTrackStop begin");

	int nead_detach = 0;

	JNIEnv *env = hv_get_jni_env(&nead_detach);
	if(!env)
		return;

	suntech_mx_lock(g_context->audio_track_context_lock);


	AudioTrackContext * audio_track_context = GetAudioTrackContext(obj);
	if(audio_track_context)
	{
		audio_track_context->play_status = Play_Status_Stop;
	}

	suntech_mx_unlock(g_context->audio_track_context_lock);

	if( nead_detach > 0)
		hv_detach_jni_env(env);

	LOGI("AudioTrackStop end");
}

static bool AudioTrackWrite(void *obj,const void* buffer, size_t userSize)
{
	LOGI("AudioTrackWrite begin");
	if( !g_context || !obj || !buffer || userSize <= 0)
		return false;

	suntech_mx_lock(g_context->audio_track_context_lock);

 	AudioTrackContext * audio_track_context = GetAudioTrackContext(obj);
	if(audio_track_context)
	{
		loop_buf *pcm_loop_buffer = audio_track_context->pcm_loop_buffer;
		if(pcm_loop_buffer)
		{
			int data_len = (int)userSize;
			suntech_mx_lock(audio_track_context->pcm_loop_buffer_lock);
			if(GetIdleBufLen(pcm_loop_buffer)>= (data_len+sizeof(int)))
			{
				PutData(pcm_loop_buffer, (char*)&data_len, sizeof(int));
				PutData(pcm_loop_buffer, (char*)buffer, userSize);
			}
			suntech_mx_unlock(audio_track_context->pcm_loop_buffer_lock);

			LOGI("AudioTrackWrite --------------------------obj:%d userSize:%d",(int)obj,(int)userSize);
		}
	}

	suntech_mx_unlock(g_context->audio_track_context_lock);

	//LOGI("AudioTrackWrite end");

	return true;
}

static void PCMProduceTimerFun(bcm_timer_id id, int data)
{
	LOGI("SCHeartBeatTimerFun");

	if (!g_context)
		return;

	LOGE("PCMProduceTimerFun-------------------------");

}

static bool CreatePCMProduceTimer(void* context)
{
	LOGI("CreatePCMProduceTimer begin");

	if(!g_context)
		return false;

	struct itimerspec ts;
	ts.it_interval.tv_sec = 0;
	ts.it_interval.tv_nsec = 1000000 * 100;
	ts.it_value.tv_sec = 0;
	ts.it_value.tv_nsec = 1000000 * 100;

	bcm_timer_create(g_context->_timer_moudle_id,&g_context->pcm_produce_timer);
	bcm_timer_settime(g_context->pcm_produce_timer, &ts);
	bcm_timer_connect(g_context->pcm_produce_timer, PCMProduceTimerFun,(int) g_context);

	return true;

	LOGI("CreatePCMProduceTimer end");
}

static void DeletePCMProduceTimer(void* context)
{
	LOGI("DeletePCMProduceTimer begin");

	if(!g_context)
		return;

	bcm_timer_cancel(g_context->pcm_produce_timer);
	bcm_timer_module_cleanup( g_context->_timer_moudle_id);

	LOGI("DeletePCMProduceTimer end");
}

int IO_SetInternalRecordCallback(InternalRecordCallback cb)
{
	if(!g_context)
		return -1;

	g_context->_InternalRecordCallback = cb;

	return 0;
}

//必须在应用启动的时候调该函数
int IO_InitInternalAudioHandler(JavaVM* vm)
{
#if ENABLE_INTERNAL_RECORD
	LOGI("IO_InitInternalAudioHandler begin");

	if(g_init_context)
		return 1;

	if(!CreateMusicDir())
		return 0;

	int ret = 0;

	memset(g_context,0,sizeof(InternalAudioContext));


	//初始化输出音频格式
	g_context->output_audio_format.sample_rate = DEFAULT_SAMPLE_RATE;
	g_context->output_audio_format.channel_count = DEFAULT_CHANNLE_COUNT;
	g_context->output_audio_format.bit_count = DEFAULT_BIT_COUNT;

	//mixer 设置
	g_context->mixer_duration = MIX_DURATION;		//200ms
	g_context->pcm_mixer_len = (DEFAULT_SAMPLE_RATE*DEFAULT_CHANNLE_COUNT*(DEFAULT_BIT_COUNT/8))/(1000/g_context->mixer_duration);


	g_context->pcm_mix_buffer_lock = suntech_mx_new();
	if(!g_context->pcm_mix_buffer_lock)
		goto failed;

	g_context->pcm_mix_buffer = (loop_buf*) malloc(sizeof(loop_buf));
	if(!g_context->pcm_mix_buffer)
		goto failed;
	memset(g_context->pcm_mix_buffer,0,sizeof(loop_buf));
	if (0 != InitLoopBuf(g_context->pcm_mix_buffer, PCM_BUF_LEN*10))
	{
		LOGE("IO_InitInternalAudioHandler InitLoopBuf failed");
		goto failed;
	}

	g_context->mixer_push_buffer = (char*)malloc(PUSH_DATA_LEN);
	if(!g_context->mixer_push_buffer)
	{
		LOGE("IO_InitInternalAudioHandler InitLoopBuf failed");
		goto failed;
	}

	//创建MediaPlayContext list
	g_context->media_play_context_list = suntech_list_new();
	if(!g_context->media_play_context_list)
		goto failed;

	g_context->media_play_context_list_lock = suntech_mx_new();
	if(!g_context->media_play_context_list_lock)
		goto failed;

	//AudioTrackContext list
	g_context->audio_track_context_list = suntech_list_new();
	if(!g_context->audio_track_context_list)
		goto failed;

	g_context->audio_track_context_lock = suntech_mx_new();
	if(!g_context->audio_track_context_lock)
		goto failed;

	//创建SoundPoolContext list
	g_context->sound_pool_context_list = suntech_list_new();
	if(!g_context->sound_pool_context_list)
		goto failed;

	g_context->sound_pool_context_list_lock = suntech_mx_new();
	if(!g_context->sound_pool_context_list_lock)
		goto failed;


	//创建pcm生成线程
	g_context->pcm_produce_thread = suntech_thread_new();
	if(!g_context->pcm_produce_thread)
		goto failed;
	suntech_thread_run(g_context->pcm_produce_thread, PcmProduceThreadFun, g_context);



	//创建PCM数据生成定时器
	//bcm_timer_module_init(2, &g_context->_timer_moudle_id);
	//CreatePCMProduceTimer(g_context);

	g_init_context = true;

	LOGI("IO_InitInternalAudioHandler end");

	return 1;


failed:
	LOGE("IO_InitInternalAudioHandler failed");

	if(g_context->media_play_context_list)
		suntech_list_del(g_context->media_play_context_list);

	if(g_context->audio_track_context_list)
		suntech_list_del(g_context->audio_track_context_list);

	if(g_context->sound_pool_context_list)
		suntech_list_del(g_context->sound_pool_context_list);

	if(g_context->media_play_context_list_lock)
		suntech_mx_del(g_context->media_play_context_list_lock);

	if(g_context->audio_track_context_lock)
		suntech_mx_del(g_context->audio_track_context_lock);

	if(g_context->sound_pool_context_list_lock)
		suntech_mx_del(g_context->sound_pool_context_list_lock);


	g_context->exit_pcm_produce_thread = true;
	if(g_context->pcm_produce_thread)
		suntech_thread_del(g_context->pcm_produce_thread);

	if(g_context->mixer_push_buffer)
		free(g_context->mixer_push_buffer);

	if (g_context->pcm_mix_buffer)
	{
		UninitLoopBuf(g_context->pcm_mix_buffer);
		free(g_context->pcm_mix_buffer);
	}


	if(g_context->pcm_mix_buffer_lock)
		suntech_mx_del(g_context->pcm_mix_buffer_lock);

	return ret;
#else
	return 0;
#endif

}

void IO_UninitInternalAudioHandler()
{
#if ENABLE_INTERNAL_RECORD
	LOGI("IO_UninitInternalAudioHandler begin");

	if(!g_init_context)
		return;

	unsigned int i = 0;

	//DeletePCMProduceTimer(g_context);
	IO_StopRecord();

	g_context->exit_pcm_produce_thread = true;
	suntech_thread_del(g_context->pcm_produce_thread);

	//删除media_play_context_list_lock
	suntech_mx_lock(g_context->media_play_context_list_lock);
	for ( i = 0; i < suntech_list_count(g_context->media_play_context_list); i++ )
	{
		MediaPlayContext *media_play_context = (MediaPlayContext*)suntech_list_get(g_context->media_play_context_list,i);
		DelMediaPlayContext(media_play_context);
	}
	suntech_list_del(g_context->media_play_context_list);
	suntech_mx_unlock(g_context->media_play_context_list_lock);

	//删除audio_track_context_list
	suntech_mx_lock(g_context->audio_track_context_lock);
	for ( i = 0; i < suntech_list_count(g_context->audio_track_context_list); i++ )
	{
		AudioTrackContext * audio_track_context = (AudioTrackContext*) suntech_list_get(g_context->audio_track_context_list, i);
		DelAudioTrackContext(audio_track_context);
	}
	suntech_list_del(g_context->audio_track_context_list);
	suntech_mx_unlock(g_context->audio_track_context_lock);

	suntech_mx_lock(g_context->pcm_mix_buffer_lock);
	UninitLoopBuf(g_context->pcm_mix_buffer);
	free(g_context->pcm_mix_buffer);
	suntech_mx_unlock(g_context->pcm_mix_buffer_lock);


	suntech_mx_del(g_context->media_play_context_list_lock);
	suntech_mx_del(g_context->audio_track_context_lock);
	suntech_mx_del(g_context->pcm_mix_buffer_lock);

	free(g_context->mixer_push_buffer);


	DeleteMusicDir();


	LOGI("IO_UninitInternalAudioHandler end");
#endif

}

int IO_StartRecord()
{
#if ENABLE_INTERNAL_RECORD
	if(g_context->is_start_record)
		return 1;

	//清空PCM
	ClearAllAudioData();

	g_context->is_start_record = true;

	return 1;
#else
	return 0;
#endif

}

int IO_StopRecord()
{
#if ENABLE_INTERNAL_RECORD
	if(!g_context->is_start_record)
		return 0;

	g_context->is_start_record = false;

	return 1;
#else
	return 0;
#endif
}

int IO_GetSampleRate()
{
	return DEFAULT_SAMPLE_RATE;
}

int IO_GetChannel()
{
	return DEFAULT_CHANNLE_COUNT;
}

int IO_GetBit()
{
	return DEFAULT_BIT_COUNT;
}

int IO_GetTotalLen()
{
	int ret = 0;
	int len = 0;
	if(!g_context)
		return ret;

	suntech_mx_lock(g_context->pcm_mix_buffer_lock);
	len = GetDataLen(g_context->pcm_mix_buffer);
	suntech_mx_unlock(g_context->pcm_mix_buffer_lock);

	return len;
}

int IO_GetData(char* data,int len)
{
	int ret = 0;
	if(!g_context)
		return ret;

	suntech_mx_lock(g_context->pcm_mix_buffer_lock);
	ret = GetData(g_context->pcm_mix_buffer, data,len);
	suntech_mx_unlock(g_context->pcm_mix_buffer_lock);

	return ret;

}

/********************************* audio handler end **************************************/




//AudioTrack::set
DEFINEHOOK(status_t,_ZN7android10AudioTrack3setE19audio_stream_type_tj14audio_format_tji20audio_output_flags_tPFviPvS4_ES4_iRKNS_2spINS_7IMemoryEEEbi,(void *obj,audio_stream_type_t streamType,
        uint32_t sampleRate,
        audio_format_t format,
        audio_channel_mask_t channelMask,
        int frameCount,
        audio_output_flags_t flags,
		AudioTrack::callback_t cbf,
        void* user,
        int notificationFrames,
        const sp<IMemory>& sharedBuffer,
		bool threadCanCallJava,
        int sessionId))
{
#if ENABLE_INTERNAL_RECORD
	int channel_count =  __builtin_popcount(channelMask);
	int bit_count = DEFAULT_BIT_COUNT;

	if (format == AUDIO_FORMAT_DEFAULT || format == AUDIO_FORMAT_PCM_16_BIT)
		bit_count = DEFAULT_BIT_COUNT;
	else if(format == AUDIO_FORMAT_PCM_8_BIT)
		bit_count = BIT_COUNT_8;

	AudioTrackSet(obj,sampleRate,channel_count,bit_count);
#endif

	status_t ret = ORIGINAL(_ZN7android10AudioTrack3setE19audio_stream_type_tj14audio_format_tji20audio_output_flags_tPFviPvS4_ES4_iRKNS_2spINS_7IMemoryEEEbi,obj,streamType,sampleRate,format,channelMask,frameCount,flags,cbf,user,notificationFrames,sharedBuffer,threadCanCallJava,sessionId);
	return ret;
}

//AudioTrack::start
DEFINEHOOK( void, _ZN7android10AudioTrack5startEv, (void *obj) )
{
#if ENABLE_INTERNAL_RECORD
	AudioTrackStart(obj);
#endif

	return ORIGINAL(_ZN7android10AudioTrack5startEv,obj );
}

//AudioTrack::pause
DEFINEHOOK( void, _ZN7android10AudioTrack5pauseEv, (void *obj) )
{
#if ENABLE_INTERNAL_RECORD
	AudioTrackPause(obj);
#endif

	return ORIGINAL(_ZN7android10AudioTrack5pauseEv,obj);
}

//AudioTrack::stop
DEFINEHOOK( void, _ZN7android10AudioTrack4stopEv, (void *obj) )
{
#if ENABLE_INTERNAL_RECORD
	AudioTrackStop(obj);
#endif

	return ORIGINAL(_ZN7android10AudioTrack4stopEv,obj );
}

//AudioTrack::write
//android5.0以下
DEFINEHOOK( ssize_t, _ZN7android10AudioTrack5writeEPKvj, (void *obj,const void* buffer, size_t userSize) )
{
#if ENABLE_INTERNAL_RECORD
	suntech_mx_lock(g_context->audio_track_context_lock);
	AudioTrackContext * audio_track_context = GetAudioTrackContext(obj);
	if(!audio_track_context)
	{
		AudioTrack *track = (AudioTrack *)obj;
		int bit_count = DEFAULT_BIT_COUNT;
		uint32_t sampleRate = track->getSampleRate();

		audio_format_t _format = track->format();
		if (_format == AUDIO_FORMAT_DEFAULT || _format == AUDIO_FORMAT_PCM_16_BIT)
			bit_count = DEFAULT_BIT_COUNT;
		else if(_format == AUDIO_FORMAT_PCM_8_BIT)
			bit_count = BIT_COUNT_8;

		AudioTrackSet(obj,sampleRate,track->channelCount(),bit_count);
		AudioTrackStart(obj);
	}
	suntech_mx_unlock(g_context->audio_track_context_lock);


	if(g_context->is_start_record)
		AudioTrackWrite(obj,buffer,userSize);
#endif

	ssize_t ret = ORIGINAL(_ZN7android10AudioTrack5writeEPKvj,obj,buffer,userSize );

	return ret;

}

//AudioTrack::write
//android5.0及以上
DEFINEHOOK( ssize_t, _ZN7android10AudioTrack5writeEPKvjb, (void *obj,const void* buffer, size_t size, bool blocking) )
{
#if ENABLE_INTERNAL_RECORD
	suntech_mx_lock(g_context->audio_track_context_lock);
	AudioTrackContext * audio_track_context = GetAudioTrackContext(obj);
	if(!audio_track_context)
	{
		AudioTrack *track = (AudioTrack *)obj;
		int bit_count = DEFAULT_BIT_COUNT;
		uint32_t sampleRate = track->getSampleRate();

		audio_format_t _format = track->format();
		if (_format == AUDIO_FORMAT_DEFAULT || _format == AUDIO_FORMAT_PCM_16_BIT)
			bit_count = DEFAULT_BIT_COUNT;
		else if(_format == AUDIO_FORMAT_PCM_8_BIT)
			bit_count = BIT_COUNT_8;

		AudioTrackSet(obj,sampleRate,track->channelCount(),bit_count);
		AudioTrackStart(obj);
	}
	suntech_mx_unlock(g_context->audio_track_context_lock);

	if(g_context->is_start_record)
		AudioTrackWrite(obj,buffer,size);
#endif

	ssize_t ret = ORIGINAL(_ZN7android10AudioTrack5writeEPKvjb,obj,buffer,size,blocking );

	return ret;

}

//AudioTrack::releaseBuffer
DEFINEHOOK( void, _ZN7android10AudioTrack13releaseBufferEPNS0_6BufferE, (void *obj,AudioTrack::Buffer* audioBuffer) )
{
	ORIGINAL(_ZN7android10AudioTrack13releaseBufferEPNS0_6BufferE,obj,audioBuffer );

}



//MediaPlay::setDataSource
DEFINEHOOK(status_t,_ZN7android11MediaPlayer13setDataSourceEixx,(void *obj,int fd, int64_t offset, int64_t length) )
{
#if ENABLE_INTERNAL_RECORD
	LOGE("MediaPlaySetDataSource offset:%ld length:%ld",(long)offset,(long)length);

	if(length>0)
	{
		int dup_fd = 0;
		dup_fd = dup(fd);
		CreateAndCopyMusicFile((dup_fd < 0) ? fd :dup_fd,offset,length);
		close(dup_fd);

		MediaPlaySetDataSource(obj,fd,offset,length);
	}
#endif

	status_t ret = ORIGINAL(_ZN7android11MediaPlayer13setDataSourceEixx,obj,fd,offset,length);
	return ret;
}

//MediaPlayer disconnect
DEFINEHOOK(void,_ZN7android11MediaPlayer10disconnectEv,(void *obj) )
{
#if ENABLE_INTERNAL_RECORD
	DelMediaPlayContextItem(obj);
#endif

	ORIGINAL(_ZN7android11MediaPlayer10disconnectEv,obj);
}


//MediaPlay::start
DEFINEHOOK(status_t,_ZN7android11MediaPlayer5startEv,(void *obj) )
{
#if ENABLE_INTERNAL_RECORD
	MediaPlayStart(obj);
#endif

	status_t ret = ORIGINAL(_ZN7android11MediaPlayer5startEv,obj);
	return ret;
}

//MediaPlay::stop
DEFINEHOOK(status_t,_ZN7android11MediaPlayer4stopEv,(void *obj) )
{
#if ENABLE_INTERNAL_RECORD
	MediaPlayStop(obj);
#endif

	status_t ret = ORIGINAL(_ZN7android11MediaPlayer4stopEv,obj);
	return ret;
}

//MediaPlay::pause
DEFINEHOOK(status_t,_ZN7android11MediaPlayer5pauseEv,(void *obj) )
{
#if ENABLE_INTERNAL_RECORD
	MediaPlayPause(obj);
#endif

	status_t ret = ORIGINAL(_ZN7android11MediaPlayer5pauseEv,obj);
	return ret;
}

//MediaPlay::setLooping
DEFINEHOOK(void,_ZN7android11MediaPlayer10setLoopingEi,(void *obj,bool looping) )
{
#if ENABLE_INTERNAL_RECORD
	MediaPlaySetLooping(obj,looping);
#endif

	ORIGINAL(_ZN7android11MediaPlayer10setLoopingEi,obj,looping);
}


//SoundPool::load
DEFINEHOOK(int,_ZN7android9SoundPool4loadEPKci, (void *obj,const char* url, int priority) )
{
	return ORIGINAL(_ZN7android9SoundPool4loadEPKci,obj,url,priority);
}

DEFINEHOOK(int,_ZN7android9SoundPool4loadEixxi,(void *obj,int fd, int64_t offset, int64_t length, int priority) )
{
#if ENABLE_INTERNAL_RECORD
	if(length>0)
	{
		int dup_fd = 0;
		dup_fd = dup(fd);
		CreateAndCopyMusicFile( (dup_fd < 0) ? fd :dup_fd,offset,length);
		close(dup_fd);
	}
#endif

	int sound_id = ORIGINAL(_ZN7android9SoundPool4loadEixxi,obj,fd,offset,length,priority);

#if ENABLE_INTERNAL_RECORD
	if(length>0)
	{
		SoundPoolLoad(obj,sound_id,fd,offset,length);
	}
#endif

	return sound_id;
}

//SoundPool::unload
DEFINEHOOK(bool,_ZN7android9SoundPool6unloadEi,(void *obj,int sampleID) )
{
#if ENABLE_INTERNAL_RECORD
	SoundPoolUnLoad(obj,sampleID);
#endif

	return ORIGINAL(_ZN7android9SoundPool6unloadEi,obj,sampleID);
}

//SoundPool::play
DEFINEHOOK(int,_ZN7android9SoundPool4playEiffiif,(void *obj,int sampleID, float leftVolume, float rightVolume, int priority,
        int loop, float rate) )
{
#if ENABLE_INTERNAL_RECORD
	SoundPoolPlay(obj,sampleID);
#endif

	return ORIGINAL(_ZN7android9SoundPool4playEiffiif,obj,sampleID,leftVolume,rightVolume,priority,loop,rate);
}

//SoundPool::stop
DEFINEHOOK(void,_ZN7android9SoundPool4stopEi,(void *obj,int channelID) )
{
#if ENABLE_INTERNAL_RECORD
	SoundPoolStop(obj,channelID);
#endif

	return ORIGINAL(_ZN7android9SoundPool4stopEi,obj,channelID);
}

//SoundPool::setLoop
DEFINEHOOK(void,_ZN7android9SoundPool7setLoopEii,(void *obj,int channelID,int loop) )
{
#if ENABLE_INTERNAL_RECORD
	SoundPoolsetLooping(obj,channelID,loop);
#endif

	return ORIGINAL(_ZN7android9SoundPool7setLoopEii,obj,channelID,loop);
}




