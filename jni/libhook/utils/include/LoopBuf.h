/*******************************************************************************
 * Software Name :safe
 *
 * Copyright (C) 2014 HanVideo.
 *
 * author : 梁智游
 ******************************************************************************/

#ifndef LOOPBUF_H_
#define LOOPBUF_H_

#ifdef __cplusplus extern "C" {
#endif

#include <pthread.h>
#include "../include/def.h"

#define MAX(a,b)        ((a) > (b) ? (a) : (b))
#define MIN(a,b)        ((a) < (b) ? (a) : (b))
 

typedef struct __tag_loop_buf {
	unsigned int in;
	unsigned int out;
	unsigned int buff_len;
	char* buffer;
	pthread_mutex_t mutex;
}loop_buf;

//初始化缓冲池
//len必须为1024的2、4、8、16....倍数
int InitLoopBuf(loop_buf *buf,unsigned int len);

//反初始化缓冲池
void UninitLoopBuf(loop_buf *buf);

//将数据拷贝到缓存池里
unsigned int PutData(loop_buf *buf, const char *buffer, unsigned int len);


//从缓存池拷贝出数据
unsigned int GetData(loop_buf *buf, char *buffer, unsigned int len);

//丢弃一些数据
unsigned int DiscardSomeData(loop_buf *buf,unsigned int len);

 //重置缓冲池
void ResetLoopBuf(loop_buf *buf);

//返回缓存总长度
unsigned int GetBufLen(loop_buf *buf);

//返回缓存数据长度
unsigned int GetDataLen(loop_buf *buf);

//返回剩余空闲缓存长度
unsigned int GetIdleBufLen(loop_buf *buf);

void LockBuf(loop_buf *buf);

void UnlockBuf(loop_buf *buf);

#ifdef __cplusplus }
#endif

#endif //LOOPBUF_H_
