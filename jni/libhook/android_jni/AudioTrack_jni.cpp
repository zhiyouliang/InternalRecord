/*
 * AudioTrack_jni.cpp
 *
 *  Created on: 2015年7月3日
 *      Author: Administrator
 */


#include <jni.h>
#include <stddef.h>
#include <string.h>
#include "tool.h"
#include "AudioTrack_jni.h"
#include "jni_helper.h"

struct __tag_AudioTrack_jni
 {
	JNIEnv* env;
	jclass _jcAudioTrack;
	jmethodID _mConstructor;
	jmethodID _mWrite;
	jmethodID _mPlay;
	jmethodID _mPause;
	jmethodID _mRelease;
	jmethodID _mFlush;
	jmethodID _mgetMinBufferSize;
	jmethodID _msetStereoVolume;
	jobject _instance;
};


static int AudioTrack_InitJNI(AudioTrack_jni* audio_track_context ,JNIEnv* env)
{
	if(!audio_track_context || !env)
	{
		LOGE("AudioTrack_InitJNI audio_track_context NULL");
		return JNI_ERR;
	}

	{
		jclass _jcAudioTrack = 0;
		const char* strClass = "android/media/AudioTrack";
		LOGI("InitJNI Searching for %s",strClass);
		_jcAudioTrack = env->FindClass(strClass);
		if (_jcAudioTrack)
		{
			LOGI("InitJNI Found %s",strClass);
		}
		else
		{
			LOGE("InitJNI Failed to find %s",strClass);
			return JNI_ERR;
		}

		audio_track_context->_jcAudioTrack = (jclass) env->NewGlobalRef(_jcAudioTrack);
		SAFE_RELEASE_LOCAL_REF(env,_jcAudioTrack);
	}

	{
		const char* strConstructor = "<init>";
		audio_track_context->_mConstructor = env->GetMethodID(audio_track_context->_jcAudioTrack, strConstructor,"(IIIIII)V");
		if (audio_track_context->_mConstructor)
		{
			LOGI(  "InitJNI Found %s",strConstructor);
		}
		else
		{
			LOGE(  "InitJNI Failed to find %s",strConstructor);
			return JNI_ERR;
		}
	}

	{
		const char* strWrite = "write";
		audio_track_context->_mWrite = env->GetMethodID(audio_track_context->_jcAudioTrack, strWrite, "([BII)I");
		if (audio_track_context->_mWrite)
		{
			LOGI(  "InitJNI Found %s",
					strWrite);
		}
		else
		{
			LOGE(  "InitJNI Failed to find %s",strWrite);
			return JNI_ERR;
		}
	}

	{
		const char* strPlay = "play";
		audio_track_context->_mPlay = env->GetMethodID(audio_track_context->_jcAudioTrack, strPlay, "()V");
		if (audio_track_context->_mPlay)
		{
			LOGI(  "InitJNI Found %s",strPlay);
		}
		else
		{
			LOGE(  "InitJNI Failed to find %s",strPlay);
			return JNI_ERR;
		}
	}

	{
		const char* strPause = "pause";
		audio_track_context->_mPause = env->GetMethodID(audio_track_context->_jcAudioTrack, strPause, "()V");
		if (audio_track_context->_mPause)
		{
			LOGI(  "InitJNI Found %s",strPause);
		}
		else
		{
			LOGE(  "InitJNI Failed to find %s",strPause);
			return JNI_ERR;
		}
	}

	{
		const char* strRelease = "release";
		audio_track_context->_mRelease = env->GetMethodID(audio_track_context->_jcAudioTrack, strRelease, "()V");
		if (audio_track_context->_mRelease)
		{
			LOGI(  "InitJNI Found %s",strRelease);
		}
		else
		{
			LOGE(  "InitJNI Failed to find %s",strRelease);
			return JNI_ERR;
		}
	}

	{
		const char* strFlush = "flush";
		audio_track_context->_mFlush = env->GetMethodID(audio_track_context->_jcAudioTrack, strFlush, "()V");
		if (audio_track_context->_mFlush)
		{
			LOGI(  "InitJNI Found %s",strFlush);
		}
		else
		{
			LOGE(  "InitJNI Failed to find %s",strFlush);
			return JNI_ERR;
		}
	}

	{
		const char* strgetMinBufferSize = "getMinBufferSize";
		audio_track_context->_mgetMinBufferSize = env->GetStaticMethodID(audio_track_context->_jcAudioTrack, strgetMinBufferSize, "(III)I");
		if (audio_track_context->_mgetMinBufferSize)
		{
			LOGI(  "InitJNI Found %s",strgetMinBufferSize);
		}
		else
		{
			LOGE(  "InitJNI Failed to find %s",strgetMinBufferSize);
			return JNI_ERR;
		}
	}

	{
		const char* strsetVolume = "setStereoVolume";
		audio_track_context->_msetStereoVolume = env->GetMethodID(audio_track_context->_jcAudioTrack, strsetVolume, "(FF)I");
		if (audio_track_context->_msetStereoVolume)
		{
			LOGI(  "InitJNI Found %s",strsetVolume);
		}
		else
		{
			LOGE(  "InitJNI Failed to find %s",strsetVolume);
			return JNI_ERR;
		}
	}

	return JNI_OK;
}



AudioTrack_jni* InitAudioTrack_jni(JNIEnv* env)
{
	if (!env)
	{
		LOGE("AudioTrack_jni JNI must be initialized with a valid environment!");
		return 0;
	}

	AudioTrack_jni * audio_track_context = (AudioTrack_jni*)malloc(sizeof(AudioTrack_jni));
	if(!audio_track_context)
	{
		LOGE("InitAudioTrack_jni audio_track_context NULL");
		return 0;
	}
	memset(audio_track_context,0,sizeof(AudioTrack_jni));


	if( JNI_OK != AudioTrack_InitJNI(audio_track_context,env) )
	{
		LOGE("InitAudioTrack_jni AudioTrack_InitJNI error");
		return 0;
	}


	return audio_track_context;
}

void UninitAudioTrack_jni(AudioTrack_jni* audio_track_context ,JNIEnv* env)
{
	if (!audio_track_context || !env)
	{
		LOGE("UninitAudioTrack_jni audio_track_context NULL");
		return;
	}

	SAFE_RELEASE_GLOBAL_REF(env,audio_track_context->_jcAudioTrack)
	SAFE_RELEASE_GLOBAL_REF(env,audio_track_context->_instance)

	free(audio_track_context);
}

int AudioTrack_getMinBufferSize(AudioTrack_jni* audio_track_context ,JNIEnv* env,int sampleRateInHz, int channelConfig, int audioFormat)
{
	LOGI("AudioTrack_getMinBufferSize begin");

	if (!audio_track_context || !env)
	{
		LOGE("AudioTrack_getMinBufferSize audio_track_context NULL");
		return -1;
	}

	jint result = env->CallStaticIntMethod(audio_track_context->_jcAudioTrack, audio_track_context->_mgetMinBufferSize, sampleRateInHz, channelConfig, audioFormat);
	if (env->ExceptionCheck())
	{
		env->ExceptionDescribe();
		env->ExceptionClear();
		LOGE(  "write Failed to write");
		return 0;
	}

	LOGI("AudioTrack_getMinBufferSize result:%d end",result);

	return result;

}

int AudioTrack_Config(AudioTrack_jni* audio_track_context ,JNIEnv* env,int streamType, int sampleRateInHz, int channelConfig, int audioFormat, int bufferSizeInBytes, int mode)
{
	if (!audio_track_context || !env)
	{
		LOGE("AudioTrack_Config audio_track_context NULL");
		return JNI_ERR;
	}

	int ret = JNI_ERR;

	jobject _instance = 0;
	jint arg1 = streamType;
	jint arg2 = sampleRateInHz;
	jint arg3 = channelConfig;
	jint arg4 = audioFormat;
	jint arg5 = bufferSizeInBytes;
	jint arg6 = mode;

	LOGI("AudioTrack_Config arg1:%d arg2:%d arg3:%d arg4:%d arg5:%d arg6:%d",arg1,arg2,arg3,arg4,arg5,arg6);

	_instance = env->AllocObject(audio_track_context->_jcAudioTrack);
	audio_track_context->_instance = env->NewGlobalRef(_instance);
	if (audio_track_context->_instance)
	{
		env->CallVoidMethod(audio_track_context->_instance, audio_track_context->_mConstructor, arg1, arg2, arg3, arg4,arg5, arg6);
		if (env->ExceptionCheck())
		{
			env->ExceptionDescribe();
			env->ExceptionClear();
			LOGE("AudioTrack_jni Failed to construct");
		}
		else
		{
			ret = JNI_OK;
			LOGI("AudioTrack_jni Constructed AudioTrack_jni");
		}
	}
	else
	{
		LOGE("AudioTrack_jni Failed to construct AudioTrack_jni");
	}

	SAFE_RELEASE_LOCAL_REF(env,_instance);

	return ret;
}

int AudioTrack_write(AudioTrack_jni* audio_track_context ,JNIEnv* env,jbyte* audioData, int offsetInBytes,int sizeInBytes)
{
	if (!audio_track_context || !env)
	{
		LOGE("AudioTrack_write audio_track_context NULL");
		return 0;
	}

	jbyteArray arg1 = env->NewByteArray(sizeInBytes);
	jint arg2 = offsetInBytes;
	jint arg3 = sizeInBytes;
	env->SetByteArrayRegion(arg1, offsetInBytes, sizeInBytes, audioData);
	jint result = env->CallIntMethod(audio_track_context->_instance, audio_track_context->_mWrite, arg1, arg2, arg3);
	if (env->ExceptionCheck())
	{
		env->ExceptionDescribe();
		env->ExceptionClear();
		LOGE(  "write Failed to write");
		return 0;
	}
	SAFE_RELEASE_LOCAL_REF(env,arg1);

	return result;
}

void AudioTrack_play(AudioTrack_jni* audio_track_context ,JNIEnv* env)
{
	if (!audio_track_context || !env)
	{
		LOGE("AudioTrack_play audio_track_context NULL");
		return;
	}

	env->CallVoidMethod(audio_track_context->_instance, audio_track_context->_mPlay);
	if (env->ExceptionCheck())
	{
		env->ExceptionDescribe();
		env->ExceptionClear();
		LOGE(  "play Failed to play");
	}
}

void AudioTrack_pause(AudioTrack_jni* audio_track_context ,JNIEnv* env)
{
	if (!audio_track_context || !env)
	{
		LOGE("AudioTrack_pause audio_track_context NULL");
		return;
	}

	env->CallVoidMethod(audio_track_context->_instance, audio_track_context->_mPause);
	if (env->ExceptionCheck())
	{
		env->ExceptionDescribe();
		env->ExceptionClear();
		LOGE(  "AudioTrack_pause Failed to pause");
	}
}

void AudioTrack_release(AudioTrack_jni* audio_track_context ,JNIEnv* env)
{
	if (!audio_track_context || !env)
	{
		LOGE("AudioTrack_release audio_track_context NULL");
		return ;
	}

	env->CallVoidMethod(audio_track_context->_instance, audio_track_context->_mRelease);
	if (env->ExceptionCheck())
	{
		env->ExceptionDescribe();
		env->ExceptionClear();
		LOGE(  "release Failed to release");
	}

}

void AudioTrack_flush(AudioTrack_jni* audio_track_context ,JNIEnv* env)
{
	if (!audio_track_context || !env)
	{
		LOGE("AudioTrack_flush audio_track_context NULL");
		return;
	}

	env->CallVoidMethod(audio_track_context->_instance, audio_track_context->_mFlush);
	if (env->ExceptionCheck())
	{
		env->ExceptionDescribe();
		env->ExceptionClear();
		LOGE(  "flush Failed to flush");
	}

}

int AudioTrack_setStereoVolume(AudioTrack_jni* audio_track_context ,JNIEnv* env,float leftVolume, float rightVolume)
{
	if (!audio_track_context || !env)
	{
		LOGE("AudioTrack_setVolume audio_track_context NULL");
		return JNI_ERR;
	}

	env->CallIntMethod(audio_track_context->_instance, audio_track_context->_msetStereoVolume,leftVolume,rightVolume);
	if (env->ExceptionCheck())
	{
		env->ExceptionDescribe();
		env->ExceptionClear();
		LOGE(  "setVolume Failed to volume");
	}

	return JNI_OK;
}

int AudioTrack_MODE_STREAM()
{
	return 1;
}

