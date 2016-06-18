/*
 * InternalRecord.h
 *
 *  Created on: 2015年7月14日
 *      Author: Administrator
 */

#ifndef INTERNALRECORD_H_
#define INTERNALRECORD_H_

/* 例子
 *
 *     const int PCM_LEN = 3528;		//44100,2channel,2 byte 大概200ms
 *     int sample_rate = 44100
 *     int channel_count = 2;
 *     int bit_num = 16;
 *     int total_data_len = 0;
 *     int get_len = 0;
 *     char *buffer = 0;
 *     int sleep_ms = 10;
 *     unsigned long long audio_time = 0;
 *	   unsigned long long start_time = 0;
 *	   unsigned long long now_time = 0;
 *	   int diff = 0;
 *	   int milliseconds = 0;
 *	   int duration = 0;
 *
 *     buffer = (char*)malloc(PCM_LEN);
 *     if(!buffer)
 *     		return;
 *
 *     if(!IR_InitInternalAudioRecord(vm))
 *     {
 *     	   free(buffer);
 *         return;
 *     }
 *     sample_rate = IR_GetSampleRate();
 *     channel_count = IR_GetChannel();
 *     bit_num = IR_GetBit();
 *
 *	   start_time = GetCurSysTime();		//系统时间，单位毫秒
 *	   audio_time = 0;
 *
 *
 *     while(!exit)
 *     {
 *	   	   total_data_len =	IR_GetTotalLen();
 *	       if( total_data_len >= PCM_LEN )
 *	       {
 *	   		    get_len = IR_GetData(buffer,PCM_LEN)
 *
 *	   			duration = ((float)get_len/(float)(sample_rate*channel_count*(bit_num/8))*1000;
 *	   	        audio_time += duration;
 *
 *	   	    	now_time = GetCurSysTime();
 *				milliseconds = now_time-start_time;
 *				diff = audio_time-milliseconds;
 *				if(diff>0)
 *				{
 *					sleep(diff);	//sleep diff毫秒
 *				}
 *			}
 *			else
 *				sleep(50);
 *
 *
 *	   	}
 *
 *	   	IR_UninitInternalAudioRecord(env);*
 *
 *	   	free(buffer);
 *
 */

/* 功能:
 * 获得pcm数据回调函数
 *
 */

#define PUSH_DATA_LEN 		4096


typedef void (*InternalRecordCallback)(const char* data,int data_len);

/* 功能:
 * 设置内录回调函数，回调返回pcm数据
 * 参数:
 * cb InternalRecordCallback类型回调函数
 * 返回值:
 *	0 成功
 *	-1 失败
 */
int IR_SetInternalRecordCallback(InternalRecordCallback cb);


/* 功能:
 * 获得音频的采样率
 * 返回值:
 * 采样率
 */
int IR_GetSampleRate();

/* 功能:
 * 得到声道数
 * 返回值:
 * 1 单声道 ;2 双声道
 */
int IR_GetChannel();

/* 功能:
 * 得到每个音频采样的bit数
 * 返回值:
 * 16 一个采样2个字节;8一个采样1个字节
 */
int IR_GetBit();

/* 功能:
 * 得到录制的pcm数据长度
 * 返回值:
 * pcm数据长度(单位:字节)
 *
 */
int IR_GetTotalLen();

/* 功能:
 * 得到录制的pcm数据
 * 参数:
 * data 保存pcm数据的缓存
 * len 预读出的pcm数据长度
 * 返回值:
 * 实际得到的pcm数据长度
 */
int IR_GetData(char* data,int len);

/* 功能:
 * 开始录音
 * 返回值:
 * 大于0 成功否则失败
 */
int IR_StartRecord();

/* 功能:
 * 停止录音
 * 返回值:
 * 大于0 成功否则失败
 */
int IR_StopRecord();



#endif /* INTERNALRECORD_H_ */
