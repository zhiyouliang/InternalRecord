/*
 * MediaExtractor_jni.cpp
 *
 *  Created on: 2015年7月1日
 *      Author: Administrator
 */

#include <string.h>
#include "MediaExtractor_jni.h"
#include "hvjava.h"
#include "tool.h"
extern "C"
{
#include "jni_helper.h"
}

struct __tag_MediaExtractor_jni
{
	jclass m_MediaExtractorCls;
	jobject m_MediaExtractorObj;
	jmethodID m_ConstructorID;
	jobject m_MediaFormatObj;

	char m_mime[256];
	int m_SampleRate;
	int m_ChannelCount;
};


void* InitMediaExtractor_jni(JNIEnv *env)
{
	LOGI( "InitMediaExtractor_jni begin" );

	jclass _MediaExtractorCls = 0;
	jobject _MediaExtractorObj = 0;


	if( 0 == env)
	{
		LOGE( "InitMediaExtractor_jni env NULL" );
		return 0;
	}

	MediaExtractor_jni * p_media_extractor = (MediaExtractor_jni*)malloc(sizeof(MediaExtractor_jni));
	if(!p_media_extractor)
		goto failed;

	memset(p_media_extractor,0,sizeof(MediaExtractor_jni));

	_MediaExtractorCls = env->FindClass("android/media/MediaExtractor");
	if( 0 == _MediaExtractorCls )
	{
		LOGE( "InitMediaExtractor_jni m_MediaExtractorCls  NULL" );
		goto failed;
	}
	p_media_extractor->m_MediaExtractorCls = (jclass) env->NewGlobalRef(_MediaExtractorCls);
	SAFE_RELEASE_LOCAL_REF(env,_MediaExtractorCls);

	p_media_extractor->m_ConstructorID = env->GetMethodID(p_media_extractor->m_MediaExtractorCls,"<init>", "()V");
	if ( 0 == p_media_extractor->m_ConstructorID )
	{
		LOGE( "InitMediaExtractor_jni m_ConstructorID  NULL" );
		goto failed;
	}


	_MediaExtractorObj = env->NewObject(p_media_extractor->m_MediaExtractorCls,p_media_extractor->m_ConstructorID);
	if( 0 == _MediaExtractorObj )
	{
		LOGE( "InitMediaExtractor_jni m_ConstructorID  NULL" );
		goto failed;
	}
	p_media_extractor->m_MediaExtractorObj = env->NewGlobalRef(_MediaExtractorObj);
	SAFE_RELEASE_LOCAL_REF(env,_MediaExtractorObj);

	LOGI( "InitMediaExtractor_jni end" );

	return p_media_extractor;

failed:
	LOGE( "InitMediaExtractor_jni failed" );
	SAFE_RELEASE_LOCAL_REF(env,_MediaExtractorCls);
	SAFE_RELEASE_LOCAL_REF(env,_MediaExtractorObj);

	if(p_media_extractor)
		free(p_media_extractor);
	return 0;

}

void UninitMediaExtractor_jni(void* media_extractor_context,JNIEnv *env)
{
	LOGI( "UninitMediaExtractor_jni begin" );

	MediaExtractor_jni * p_media_extractor = (MediaExtractor_jni*)media_extractor_context;

	if( !media_extractor_context || 0 == env)
	{
		LOGE( "UninitMediaExtractor_jni env NULL" );
		goto end;
	}

	SAFE_RELEASE_GLOBAL_REF(env,p_media_extractor->m_MediaExtractorCls)
	SAFE_RELEASE_GLOBAL_REF(env,p_media_extractor->m_MediaExtractorObj)
	SAFE_RELEASE_GLOBAL_REF(env,p_media_extractor->m_MediaFormatObj)

	free(media_extractor_context);
	LOGI( "UninitMediaExtractor_jni end" );

end:
	return;
}


void MediaExtractor_setDataSourceByFD(void* media_extractor_context,JNIEnv *env,int fd,long offset,long length)
{
	jmethodID _SetDataSourceID = 0;
	jstring jstr_path;
	jmethodID _GetTrackFormatID = 0;
	jclass __MediaFormatCls = 0;
	jmethodID _GetIntegerID = 0;
	jmethodID _GetStringID = 0;
	jstring _jstr_channel_count;
	jstring _jstr_samplerate;
	jstring _jstr_mime;
	jstring _jstr_mime_value;
	jboolean isCopy = 1;
	char* temp_mime_p = 0;
	jclass _FileDescriptorCls = 0;
	jobject _FileDescriptorObj = 0;
	jmethodID _FileDescriptorConstructorID = 0;
	jfieldID _FileDescriptorDescriptorID = 0;
	jmethodID _FileDescriptorSetIntID = 0;

	MediaExtractor_jni * p_media_extractor = (MediaExtractor_jni*)media_extractor_context;



	if(!p_media_extractor)
	{
		LOGE( "MediaExtractor_jni::setDataSource p_media_extractor NULL" );
		goto end;
	}



	if( 0 == env)
	{
		LOGE( "MediaExtractor_jni::setDataSource env NULL" );
		goto end;
	}

	//文件描述符
	_FileDescriptorCls = env->FindClass("java/io/FileDescriptor");
	if(!_FileDescriptorCls)
	{
		LOGE( "MediaExtractor_jni::setDataSource _FileDescriptorCls NULL" );
		goto end;
	}

	_FileDescriptorConstructorID =  env->GetMethodID(_FileDescriptorCls, "<init>", "()V");
	if(!_FileDescriptorConstructorID)
	{
		LOGE( "MediaExtractor_jni::setDataSource _FileDescriptorConstructorID NULL" );
		goto end;
	}

	_FileDescriptorSetIntID = env->GetMethodID(_FileDescriptorCls, "setInt$", "(I)V");
	if(!_FileDescriptorSetIntID)
	{
		LOGE( "MediaExtractor_jni::setDataSource _FileDescriptorSetIntID NULL" );
		goto end;
	}

	_FileDescriptorDescriptorID = env->GetFieldID(_FileDescriptorCls, "descriptor", "I");
	if(!_FileDescriptorDescriptorID)
	{
		LOGE( "MediaExtractor_jni::setDataSource _FileDescriptorDescriptorID NULL" );
		goto end;
	}

	_FileDescriptorObj = env->NewObject(_FileDescriptorCls,_FileDescriptorConstructorID);
	if(!_FileDescriptorObj)
	{
		LOGE( "MediaExtractor_jni::setDataSource _FileDescriptorObj NULL" );
		goto end;
	}

	//env->CallVoidMethod(_FileDescriptorObj, _FileDescriptorSetIntID,fd);


	env->SetIntField(_FileDescriptorObj,_FileDescriptorDescriptorID,fd);
	if (env->ExceptionOccurred())
	{
		LOGE( "MediaExtractor_jni::SetIntField _FileDescriptorDescriptorID failed" );
	    env->ExceptionClear();
	    goto end;
	}

	//设置fd
	_SetDataSourceID = env->GetMethodID(p_media_extractor->m_MediaExtractorCls, "setDataSource", "(Ljava/io/FileDescriptor;JJ)V");
	if (_SetDataSourceID == 0)
	{
		LOGE( "MediaExtractor_jni::setDataSource _SetDataSourceID NULL" );
		goto end;
	}

	env->CallVoidMethod(p_media_extractor->m_MediaExtractorObj, _SetDataSourceID,_FileDescriptorObj,offset,length);
	if (CHECK_EXCEPTION())
	{
		LOGE( "MediaExtractor_jni::setDataSource _SetDataSourceID failed" );
		goto end;
	}

	//得到媒体格式
	__MediaFormatCls = env->FindClass("android/media/MediaFormat");
	if (__MediaFormatCls == 0)
	{
		LOGE( "MediaExtractor_jni::setDataSource __MediaFormatCls NULL" );
		goto end;
	}

	_GetIntegerID = env->GetMethodID(__MediaFormatCls, "getInteger", "(Ljava/lang/String;)I");
	if (_GetIntegerID == 0)
	{
		LOGE( "MediaExtractor_jni::setDataSource _GetIntegerID NULL" );
		goto end;
	}

	_GetStringID = env->GetMethodID(__MediaFormatCls, "getString", "(Ljava/lang/String;)Ljava/lang/String;");
	if (_GetStringID == 0)
	{
		LOGE( "MediaExtractor_jni::setDataSource _GetStringID NULL" );
		goto end;
	}

	_GetTrackFormatID = env->GetMethodID(p_media_extractor->m_MediaExtractorCls, "getTrackFormat", "(I)Landroid/media/MediaFormat;");
	if (_GetTrackFormatID == 0)
	{
		LOGE( "MediaExtractor_jni::setDataSource _GetTrackFormatID NULL" );
		goto end;
	}

	p_media_extractor->m_MediaFormatObj = env->CallObjectMethod(p_media_extractor->m_MediaExtractorObj,_GetTrackFormatID, 0);
	p_media_extractor->m_MediaFormatObj = (jclass) env->NewGlobalRef(p_media_extractor->m_MediaFormatObj);
	if (p_media_extractor->m_MediaFormatObj == 0)
	{
		LOGE( "MediaExtractor_jni::setDataSource p_media_extractor->m_MediaFormatObj NULL" );
		goto end;
	}

	_jstr_channel_count = env->NewStringUTF("channel-count");
	p_media_extractor->m_ChannelCount = env->CallIntMethod(p_media_extractor->m_MediaFormatObj,_GetIntegerID,_jstr_channel_count);

	_jstr_samplerate = env->NewStringUTF("sample-rate");
	p_media_extractor->m_SampleRate = env->CallIntMethod(p_media_extractor->m_MediaFormatObj,_GetIntegerID,_jstr_samplerate);

	_jstr_mime = env->NewStringUTF("mime");
	_jstr_mime_value = (jstring)env->CallObjectMethod(p_media_extractor->m_MediaFormatObj,_GetStringID,_jstr_mime);

	temp_mime_p = (char*) env->GetStringUTFChars(_jstr_mime_value, &isCopy);
	if( temp_mime_p )
	{
		strcpy(p_media_extractor->m_mime,temp_mime_p);
		LOGI( "MediaExtractor_jni::setDataSource m_mime:%s",p_media_extractor->m_mime );
	}

	env->ReleaseStringUTFChars(_jstr_mime_value, temp_mime_p);


end:
	if(_jstr_channel_count)
		env->DeleteLocalRef(_jstr_channel_count);
	if(_jstr_samplerate)
		env->DeleteLocalRef(_jstr_samplerate);
	if(_jstr_mime)
		env->DeleteLocalRef(_jstr_mime);
	if(_jstr_mime_value)
		env->DeleteLocalRef(_jstr_mime_value);
	if(__MediaFormatCls)
		env->DeleteLocalRef(__MediaFormatCls);
	return;
}


bool MediaExtractor_setDataSource(void* media_extractor_context,JNIEnv *env,const char* path)
{
	bool ret = false;
	jobject _MediaFormatObj = 0;
	jmethodID _SetDataSourceID = 0;
	jstring jstr_path = 0;
	jmethodID _GetTrackFormatID = 0;
	jclass __MediaFormatCls = 0;
	jmethodID _GetIntegerID = 0;
	jmethodID _GetStringID = 0;
	jstring _jstr_channel_count = 0;
	jstring _jstr_samplerate = 0;
	jstring _jstr_mime = 0;
	jstring _jstr_mime_value = 0;
	jboolean isCopy = 1;
	char* temp_mime_p = 0;

	MediaExtractor_jni * p_media_extractor = (MediaExtractor_jni*)media_extractor_context;

	LOGI( "MediaExtractor_jni::setDataSource path:%s",path );

	if(!p_media_extractor)
	{
		LOGE( "MediaExtractor_jni::setDataSource p_media_extractor NULL" );
		goto end;
	}

	if( !path || strlen(path) <= 0 )
	{
		LOGE( "MediaExtractor_jni::setDataSource path NULL" );
		goto end;
	}

	if( 0 == env)
	{
		LOGE( "MediaExtractor_jni::setDataSource env NULL" );
		goto end;
	}

	//设置url
	jstr_path = env->NewStringUTF(path);
	_SetDataSourceID = env->GetMethodID(p_media_extractor->m_MediaExtractorCls, "setDataSource", "(Ljava/lang/String;)V");
	if (_SetDataSourceID == 0)
	{
		LOGE( "MediaExtractor_jni::setDataSource _SetDataSourceID NULL" );
		goto end;
	}

	env->CallVoidMethod(p_media_extractor->m_MediaExtractorObj, _SetDataSourceID,jstr_path);
	if (CHECK_EXCEPTION())
	{
		LOGE( "MediaExtractor_jni::setDataSource _SetDataSourceID failed" );
		goto end;
	}

	//得到媒体格式
	__MediaFormatCls = env->FindClass("android/media/MediaFormat");
	if (__MediaFormatCls == 0)
	{
		LOGE( "MediaExtractor_jni::setDataSource __MediaFormatCls NULL" );
		goto end;
	}

	_GetIntegerID = env->GetMethodID(__MediaFormatCls, "getInteger", "(Ljava/lang/String;)I");
	if (_GetIntegerID == 0)
	{
		LOGE( "MediaExtractor_jni::setDataSource _GetIntegerID NULL" );
		goto end;
	}

	_GetStringID = env->GetMethodID(__MediaFormatCls, "getString", "(Ljava/lang/String;)Ljava/lang/String;");
	if (_GetStringID == 0)
	{
		LOGE( "MediaExtractor_jni::setDataSource _GetStringID NULL" );
		goto end;
	}

	_GetTrackFormatID = env->GetMethodID(p_media_extractor->m_MediaExtractorCls, "getTrackFormat", "(I)Landroid/media/MediaFormat;");
	if (_GetTrackFormatID == 0)
	{
		LOGE( "MediaExtractor_jni::setDataSource _GetTrackFormatID NULL" );
		goto end;
	}

	_MediaFormatObj = env->CallObjectMethod(p_media_extractor->m_MediaExtractorObj,_GetTrackFormatID, 0);
	if (_MediaFormatObj == 0)
	{
		LOGE( "MediaExtractor_jni::setDataSource p_media_extractor->m_MediaFormatObj NULL" );
		goto end;
	}
	p_media_extractor->m_MediaFormatObj = (jclass) env->NewGlobalRef(_MediaFormatObj);


	_jstr_channel_count = env->NewStringUTF("channel-count");
	p_media_extractor->m_ChannelCount = env->CallIntMethod(p_media_extractor->m_MediaFormatObj,_GetIntegerID,_jstr_channel_count);

	_jstr_samplerate = env->NewStringUTF("sample-rate");
	p_media_extractor->m_SampleRate = env->CallIntMethod(p_media_extractor->m_MediaFormatObj,_GetIntegerID,_jstr_samplerate);

	_jstr_mime = env->NewStringUTF("mime");
	_jstr_mime_value = (jstring)env->CallObjectMethod(p_media_extractor->m_MediaFormatObj,_GetStringID,_jstr_mime);

	temp_mime_p = (char*) env->GetStringUTFChars(_jstr_mime_value, &isCopy);
	if( temp_mime_p )
	{
		strcpy(p_media_extractor->m_mime,temp_mime_p);
		LOGI( "MediaExtractor_jni::setDataSource m_mime:%s",p_media_extractor->m_mime );
	}

	env->ReleaseStringUTFChars(_jstr_mime_value, temp_mime_p);

	ret = true;
end:
	SAFE_RELEASE_LOCAL_REF(env,jstr_path);
	SAFE_RELEASE_LOCAL_REF(env,_MediaFormatObj);
	SAFE_RELEASE_LOCAL_REF(env,_jstr_channel_count);
	SAFE_RELEASE_LOCAL_REF(env,_jstr_samplerate);
	SAFE_RELEASE_LOCAL_REF(env,_jstr_mime);
	SAFE_RELEASE_LOCAL_REF(env,_jstr_mime_value);
	SAFE_RELEASE_LOCAL_REF(env,__MediaFormatCls);

	return ret;
}


void MediaExtractor_selectTrack(void* media_extractor_context,JNIEnv *env,int index)
{
	jmethodID _SelectTrackID = 0;

	MediaExtractor_jni * p_media_extractor = (MediaExtractor_jni*)media_extractor_context;

	if(!p_media_extractor)
	{
		LOGE( "MediaExtractor_selectTrack p_media_extractor NULL" );
		goto end;
	}

	if( 0 == env)
	{
		LOGE( "MediaExtractor_jni::selectTrack env NULL" );
		goto end;
	}

	_SelectTrackID = env->GetMethodID(p_media_extractor->m_MediaExtractorCls, "selectTrack", "(I)V");
	if (_SelectTrackID == 0)
	{
		LOGE( "MediaExtractor_jni::selectTrack _SelectTrackID NULL" );
		goto end;
	}

	env->CallVoidMethod(p_media_extractor->m_MediaExtractorObj, _SelectTrackID,index);

end:
	return;
}

char* MediaExtractor_getmime(void* media_extractor_context,JNIEnv *env)
{
	MediaExtractor_jni * p_media_extractor = (MediaExtractor_jni*)media_extractor_context;

	if(!p_media_extractor)
	{
		LOGE( "MediaExtractor_getmime p_media_extractor NULL" );
		return 0;
	}

	return p_media_extractor->m_mime;
}

int MediaExtractor_getChannelCount(void* media_extractor_context,JNIEnv *env)
{
	MediaExtractor_jni * p_media_extractor = (MediaExtractor_jni*)media_extractor_context;

	if(!p_media_extractor)
	{
		LOGE( "MediaExtractor_getChannelCount p_media_extractor NULL" );
		return 0;
	}

	return p_media_extractor->m_ChannelCount;
}

int MediaExtractor_getSampleRate(void* media_extractor_context,JNIEnv *env)
{
	MediaExtractor_jni * p_media_extractor = (MediaExtractor_jni*)media_extractor_context;

	if(!p_media_extractor)
	{
		LOGE( "MediaExtractor_getSampleRate p_media_extractor NULL" );
		return 0;
	}

	return p_media_extractor->m_SampleRate;
}

jobject MediaExtractor_getMediaFormat(void* media_extractor_context,JNIEnv *env)
{
	MediaExtractor_jni * p_media_extractor = (MediaExtractor_jni*)media_extractor_context;

	if(!p_media_extractor)
	{
		LOGE( "MediaExtractor_getmime p_media_extractor NULL" );
		return 0;
	}

	return p_media_extractor->m_MediaFormatObj;
}

int MediaExtractor_readSampleData(void* media_extractor_context,JNIEnv *env,unsigned char* out_data,int size,int offset)
{
	jint ret = 0;
	jmethodID _ReadSampleDataID = 0;
	jobject byteBuffer = 0;

	MediaExtractor_jni * p_media_extractor = (MediaExtractor_jni*)media_extractor_context;

	if(!p_media_extractor)
	{
		LOGE( "MediaExtractor_readSampleData p_media_extractor NULL" );
		return 0;
	}

	if( 0 == env)
	{
		LOGE( "MediaExtractor_jni::readSampleData env NULL" );
		goto end;
	}

	_ReadSampleDataID = env->GetMethodID(p_media_extractor->m_MediaExtractorCls, "readSampleData", "(Ljava/nio/ByteBuffer;I)I");
	if (_ReadSampleDataID == 0)
	{
		LOGE( "MediaExtractor_jni::readSampleData _ReadSampleDataID NULL" );
		goto end;
	}

	byteBuffer = env->NewDirectByteBuffer(out_data, (jint)size);

	ret = env->CallIntMethod(p_media_extractor->m_MediaExtractorObj,_ReadSampleDataID,byteBuffer,offset);

	LOGI( "MediaExtractor_jni::readSampleData ret:%d",ret);

end:
	SAFE_RELEASE_LOCAL_REF(env,byteBuffer)
	return ret;
}

long MediaExtractor_getSampleTime(void* media_extractor_context,JNIEnv *env)
{
	jlong ret = 0;
	jmethodID _GetSampleTimeID = 0;

	MediaExtractor_jni * p_media_extractor = (MediaExtractor_jni*)media_extractor_context;

	if(!p_media_extractor)
	{
		LOGE( "MediaExtractor_getSampleTime p_media_extractor NULL" );
		return 0;
	}

	if( 0 == env)
	{
		LOGE( "MediaExtractor_jni::getSampleTime env NULL" );
		goto end;
	}

	_GetSampleTimeID = env->GetMethodID(p_media_extractor->m_MediaExtractorCls, "getSampleTime", "()J");
	if (_GetSampleTimeID == 0)
	{
		LOGE( "MediaExtractor_jni::readSampleData _GetSampleTimeID NULL" );
		goto end;
	}

	ret = env->CallLongMethod(p_media_extractor->m_MediaExtractorObj,_GetSampleTimeID);

	LOGI( "MediaExtractor_jni::getSampleTime ret:%ld",(long)ret);

end:
	return ret;
}

bool MediaExtractor_advance(void* media_extractor_context,JNIEnv *env)
{
	jboolean ret = 0;

	jmethodID _AdvanceID = 0;

	MediaExtractor_jni * p_media_extractor = (MediaExtractor_jni*)media_extractor_context;

	if(!p_media_extractor)
	{
		LOGE( "MediaExtractor_advance p_media_extractor NULL" );
		return 0;
	}


	_AdvanceID = env->GetMethodID(p_media_extractor->m_MediaExtractorCls, "advance", "()Z");
	if (_AdvanceID == 0)
	{
		LOGE( "MediaExtractor_jni::advance _AdvanceID NULL" );
		goto end;
	}

	ret = env->CallBooleanMethod(p_media_extractor->m_MediaExtractorObj,_AdvanceID);

	LOGI( "MediaExtractor_jni::advance ret:%d",ret);

end:
    return ret;
}

void MediaExtractor_seekTo(void* media_extractor_context,JNIEnv *env,long timeUs)
{
	jmethodID seekToID = 0;

	MediaExtractor_jni * p_media_extractor = (MediaExtractor_jni*)media_extractor_context;

	if(!p_media_extractor)
	{
		LOGE( "MediaExtractor_seekTo p_media_extractor NULL" );
		return;
	}


	seekToID = env->GetMethodID(p_media_extractor->m_MediaExtractorCls, "seekTo", "(JI)V");
	if (seekToID == 0)
	{
		LOGE( "MediaExtractor_jni::MediaExtractor_seekTo seekToID NULL" );
		return;
	}

	env->CallVoidMethod(p_media_extractor->m_MediaExtractorObj,seekToID,timeUs,2);
}




