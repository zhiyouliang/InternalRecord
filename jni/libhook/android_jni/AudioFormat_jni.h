/*
 * AudioFormat_jni.h
 *
 *  Created on: 2015年7月7日
 *      Author: Administrator
 */

#ifndef LIBHOOK_ANDROID_JNI_AUDIOFORMAT_JNI_H_
#define LIBHOOK_ANDROID_JNI_AUDIOFORMAT_JNI_H_



class AudioFormat_jni
{
public:
	static int CHANNEL_OUT_MONO();
	static int CHANNEL_OUT_STEREO();
	static int ENCODING_PCM_16BIT();
	static int ENCODING_PCM_8BIT();
};

#endif /* LIBHOOK_ANDROID_JNI_AUDIOFORMAT_JNI_H_ */
