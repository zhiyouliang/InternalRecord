/*
 * AudioManager_jni.h
 *
 *  Created on: 2015年7月7日
 *      Author: Administrator
 */

#ifndef LIBHOOK_ANDROID_JNI_AUDIOMANAGER_JNI_H_
#define LIBHOOK_ANDROID_JNI_AUDIOMANAGER_JNI_H_


#include <string>

struct _JNIEnv;
typedef _JNIEnv JNIEnv;

class _jclass;
typedef _jclass* jclass;

class AudioManager_jni {
public:
	int InitJNI(JNIEnv* env);
	void UninitJNI(JNIEnv* env);
	int STREAM_MUSIC(JNIEnv* env);

private:
	jclass m_jcAudioManager;
	jfieldID m_fStreamMusic;
};



#endif /* LIBHOOK_ANDROID_JNI_AUDIOMANAGER_JNI_H_ */
