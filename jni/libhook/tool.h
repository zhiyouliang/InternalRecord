/*
 * def.h
 *
 *  Created on: 2015年7月1日
 *      Author: Administrator
 */

#ifndef LIBHOOK_DEF_H_
#define LIBHOOK_DEF_H_

#include <android/log.h>

#define LOG_DEBUG	0
#define ENABLE_INTERNAL_RECORD		1

#define LOG_TAG "internal_audio_record"

#if LOG_DEBUG
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#else
#define LOGI(...)
#define LOGE(...)
#endif


unsigned long long GetCurSysTime();
void mysleep(unsigned int milliseconds);

#endif /* LIBHOOK_DEF_H_ */
