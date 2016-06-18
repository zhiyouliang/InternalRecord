/*
 * MediaExtractor_jni.h
 *
 *  Created on: 2015年7月1日
 *      Author: Administrator
 */

#ifndef LIBHOOK_JNI_MEDIAEXTRACTOR_H_
#define LIBHOOK_JNI_MEDIAEXTRACTOR_H_

#include <jni.h>
#include "def.h"

typedef struct __tag_MediaExtractor_jni MediaExtractor_jni;

void* InitMediaExtractor_jni(JNIEnv *env);
void UninitMediaExtractor_jni(void* media_extractor_context,JNIEnv *env);

void MediaExtractor_setDataSourceByFD(void* media_extractor_context,JNIEnv *env,int fd,long offset,long length);
bool MediaExtractor_setDataSource(void* media_extractor_context,JNIEnv *env,const char* path);
void MediaExtractor_selectTrack(void* media_extractor_context,JNIEnv *env,int index);
char* MediaExtractor_getmime(void* media_extractor_context,JNIEnv *env);
int MediaExtractor_getChannelCount(void* media_extractor_context,JNIEnv *env);
int MediaExtractor_getSampleRate(void* media_extractor_context,JNIEnv *env);
jobject MediaExtractor_getMediaFormat(void* media_extractor_context,JNIEnv *env);
int MediaExtractor_readSampleData(void* media_extractor_context,JNIEnv *env,unsigned char* out_data,int size,int offset);
long MediaExtractor_getSampleTime(void* media_extractor_context,JNIEnv *env);
bool MediaExtractor_advance(void* media_extractor_context,JNIEnv *env);
void MediaExtractor_seekTo(void* media_extractor_context,JNIEnv *env,long timeUs);




#endif /* LIBHOOK_JNI_MEDIAEXTRACTOR_H_ */
