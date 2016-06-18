/*
 * AudioManager_jni.cpp
 *
 *  Created on: 2015年7月7日
 *      Author: Administrator
 */

#include <jni.h>
#include "AudioManager_jni.h"
#include "tool.h"
#include "jni_helper.h"


int AudioManager_jni::InitJNI(JNIEnv* env)
{
	m_jcAudioManager = 0;
	m_fStreamMusic = 0;

	{
		jclass _jcAudioManager = 0;
		const char* strClass = "android/media/AudioManager";
		LOGI("Searching for %s", strClass);
		_jcAudioManager = env->FindClass(strClass);
		if (_jcAudioManager)
		{
			LOGI("Found %s", strClass);
		}
		else
		{
			LOGI("Failed to find %s", strClass);
			return JNI_ERR;
		}
		m_jcAudioManager = (jclass) env->NewGlobalRef(_jcAudioManager);
		SAFE_RELEASE_LOCAL_REF(env,_jcAudioManager);
	}

	{
		const char* strStreamMusic = "STREAM_MUSIC";
		m_fStreamMusic = env->GetStaticFieldID(m_jcAudioManager, strStreamMusic,"I");
		if (m_fStreamMusic)
		{
			LOGI("Found %s", strStreamMusic);
		}
		else
		{
			LOGI("Failed to find %s", strStreamMusic);
			return JNI_ERR;
		}
	}


	return JNI_OK;
}

void AudioManager_jni::UninitJNI(JNIEnv* env)
{
	SAFE_RELEASE_GLOBAL_REF(env,m_jcAudioManager)
}

int AudioManager_jni::STREAM_MUSIC(JNIEnv* env)
{
	jint result = env->GetStaticIntField(m_jcAudioManager, m_fStreamMusic);
	if (env->ExceptionCheck())
	{
		env->ExceptionDescribe();
		env->ExceptionClear();
		LOGI("Failed to get static field");
		return 0;
	}
	else
	{
		LOGI("Success on get static field");
		return result;
	}
}

