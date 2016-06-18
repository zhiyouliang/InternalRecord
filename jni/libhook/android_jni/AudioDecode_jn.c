#include <jni.h>
#include <stdint.h>
#include <assert.h>

#include "def.h"
#include "tool.h"
#include "AudioDecode_jn.h"


#include "jni_helper.h"


#define INPUT_BUFFER_TIMEOUT_US (5 * 1000ll)

#define INFO_OUTPUT_BUFFERS_CHANGED -3
#define INFO_OUTPUT_FORMAT_CHANGED  -2
#define INFO_TRY_AGAIN_LATER        -1


#define CLASS_ELEMENT_NUM 			7
#define MEMBER_ELEMENT_NUM 			35

typedef struct __tag_media_static_fedlds
{
	//clolor_format
	int COLOR_FormatYUV420Planar;
	int COLOR_FormatYUV420SemiPlanar;
}media_static_fedlds;

struct jfields
{
    jclass media_codec_list_class, media_codec_class, media_format_class;
    jclass buffer_info_class, byte_buffer_class;
    jmethodID tostring;
    jmethodID get_codec_count, get_codec_info_at, is_encoder, get_capabilities_for_type;
    jfieldID profile_levels_field, profile_field, level_field;
    jmethodID get_supported_types, get_name;
    jmethodID create_by_codec_type,create_by_codec_name, configure, start, stop, flush, release;
    jmethodID get_output_format;
    jmethodID get_input_buffers, get_input_buffer;
    jmethodID get_output_buffers, get_output_buffer;
    jmethodID dequeue_input_buffer, dequeue_output_buffer, queue_input_buffer;
    jmethodID release_output_buffer;
    jmethodID create_video_format, set_integer, set_bytebuffer,get_integer;
    jmethodID buffer_info_ctor;
    jfieldID size_field, offset_field, pts_field;
};


enum Types
{
    METHOD, STATIC_METHOD, FIELD
};

#define OFF(x) offsetof(struct jfields, x)
struct classname
{
    const char *name;
    int offset;
};


struct member
{
    const char *name;
    const char *sig;
    const char *class;
    int offset;
    int type;
    bool critical;
};


typedef struct __tag_media_codec_context
{
	struct jfields  jfields;
    jobject codec;
    jobject buffer_info;
    jobject input_buffers, output_buffers;
    bool b_started;
    char *psz_name;
    media_static_fedlds _media_static_fedlds;

    struct classname classes[CLASS_ELEMENT_NUM];
    struct member members[MEMBER_ELEMENT_NUM];
}_media_codec_context;

static void SetClassElement(media_codec_context* context,const char* name,int offset,int index)
{
	if( !context || index < 0 || index > CLASS_ELEMENT_NUM)
		return;
	context->classes[index].name = name;
	context->classes[index].offset = offset;
}

static void SetMemberElement(media_codec_context* context, const char *name,const char *sig,const char *class,int offset,int type,bool critical,int index)
{
	if( !context || index < 0 || index > MEMBER_ELEMENT_NUM)
		return;

	context->members[index].name = name;
	context->members[index].sig = sig;
	context->members[index].class = class;
	context->members[index].offset = offset;
	context->members[index].type = type;
	context->members[index].critical = critical;

}

// 初始化所有 jni fields.仅在开始的时候初始化一次
static bool InitJNIFields (JNIEnv *env,media_codec_context* context)
{
	LOGI("InitJNIFields begin");
	if( !env || !context)
		return false;


    bool ret = false;
    int i = 0;


    //初始化所有类
    for (i = 0; context->classes[i].name; i++)
    {
        jclass clazz = (*env)->FindClass(env, context->classes[i].name);
        if (CHECK_EXCEPTION())
        {
        	LOGE("InitJNIFields Unable to find class %s", context->classes[i].name);
            goto end;
        }
        *(jclass*)((uint8_t*)&context->jfields + context->classes[i].offset) = (jclass) (*env)->NewGlobalRef(env, clazz);
        SAFE_RELEASE_LOCAL_REF_C(env,clazz)
    }

    //初始化所有类公共成员
    jclass last_class = 0;
    for (i = 0; context->members[i].name; i++)
    {
    	last_class = (*env)->FindClass(env, context->members[i].class);
        if (CHECK_EXCEPTION())
        {
        	LOGE("InitJNIFields Unable to find class %s", context->members[i].class);
            goto end;
        }

        switch (context->members[i].type) {
        case METHOD:
            *(jmethodID*)((uint8_t*)&context->jfields + context->members[i].offset) =
                (*env)->GetMethodID(env, last_class, context->members[i].name, context->members[i].sig);
            break;
            case STATIC_METHOD:
            *(jmethodID*)((uint8_t*)&context->jfields + context->members[i].offset) =
                (*env)->GetStaticMethodID(env, last_class, context->members[i].name, context->members[i].sig);
            break;
        case FIELD:
            *(jfieldID*)((uint8_t*)&context->jfields + context->members[i].offset) =
                (*env)->GetFieldID(env, last_class, context->members[i].name, context->members[i].sig);
            break;
        }
        if (CHECK_EXCEPTION())
        {
        	LOGE("InitJNIFields Unable to find the member %s in %s",context->members[i].name, context->members[i].class);
            if (context->members[i].critical)
                goto end;
        }

        SAFE_RELEASE_LOCAL_REF_C(env,last_class)

    }

    /* getInputBuffers and getOutputBuffers are deprecated if API >= 21
     * use getInputBuffer and getOutputBuffer instead. */

    if (context->jfields.get_input_buffer && context->jfields.get_output_buffer)
    {
    	context->jfields.get_output_buffers = NULL;
    	context->jfields.get_input_buffers = NULL;
    }
    else if (!context->jfields.get_output_buffers && !context->jfields.get_input_buffers)
    {
    	LOGE("InitJNIFields Unable to find get Output/Input Buffer/Buffers");
        goto end;
    }

    ret = true;


end:
    if( !ret )
    	LOGE("InitJNIFields jni init failed" );


    LOGI("InitJNIFields end");

    return ret;
}

static void UninitJNIFields (JNIEnv *env,media_codec_context* context)
{
	int i;
	for (i = 0; context->classes[i].name; i++)
	{
		SAFE_RELEASE_GLOBAL_REF_C(env,*(jclass*)((uint8_t*)&context->jfields + context->classes[i].offset))
	}
}

static void media_format_set_integer(JNIEnv *env,media_codec_context* context, jobject obj, const char *psz_name,int value)
 {
	LOGI("media_format_set_integer begin");


	jstring jname = (*env)->NewStringUTF(env, psz_name);
	(*env)->CallVoidMethod(env, obj, context->jfields.set_integer, jname, value);
	(*env)->DeleteLocalRef(env, jname);

	LOGI("media_format_set_integer end");
}

static int jstrcmp(JNIEnv* env, jobject str, const char* str2)
{
    jsize len = (*env)->GetStringUTFLength(env, str);
    if (len != (jsize) strlen(str2))
        return -1;
    const char *ptr = (*env)->GetStringUTFChars(env, str, NULL);
    int ret = memcmp(ptr, str2, len);
    (*env)->ReleaseStringUTFChars(env, str, ptr);
    return ret;
}

static bool populate_static_fields(JNIEnv* env,media_codec_context* context)
{
	LOGI("populate_static_fields begin");

	if(!context)
		return false;
	jclass cls;
	jfieldID fid;
	const char* class_name = "android/media/MediaCodecInfo$CodecCapabilities";


	cls = (*env)->FindClass(env,class_name);
	if (CHECK_EXCEPTION())
	{
	    LOGE("populate_static_fields Unable to find class %s", class_name);
	    return false;
	}

	fid = (*env)->GetStaticFieldID(env,cls, "COLOR_FormatYUV420Planar","I");
	context->_media_static_fedlds.COLOR_FormatYUV420Planar = (int)(*env)->GetStaticIntField(env,cls, fid);

	fid = (*env)->GetStaticFieldID(env,cls, "COLOR_FormatYUV420SemiPlanar","I");
	context->_media_static_fedlds.COLOR_FormatYUV420SemiPlanar = (int)(*env)->GetStaticIntField(env,cls, fid);

	SAFE_RELEASE_LOCAL_REF_C(env,cls)

	return true;

	LOGI("populate_static_fields end");
}


static jstring get_media_codec_name(media_codec_context* context, JNIEnv *env, const char *psz_mime, jstring jmime)
{
	LOGI("get_media_codec_name begin");
	if(!context)
		NULL;

	int i,j;
	int num_codecs;
	jstring jcodec_name = NULL;

	//得到编码器数 MediaCodecList.getCodecCount()
	num_codecs = (*env)->CallStaticIntMethod(env,context->jfields.media_codec_list_class,context->jfields.get_codec_count);
	LOGI("get_media_codec_name num_codecs:%d",num_codecs);
	for ( i = 0; i < num_codecs; i++)
	{
		jobject codec_capabilities = NULL;
		jobject profile_levels = NULL;
		jobject info = NULL;
		jobject name = NULL;
		jobject types = NULL;
		jsize name_len = 0;
		int profile_levels_len = 0, num_types = 0;
		const char *name_ptr = NULL;
		bool found = false;

		//得到编码信息 MediaCodecList.getCodecInfoAt
		info = (*env)->CallStaticObjectMethod(env, context->jfields.media_codec_list_class,context->jfields.get_codec_info_at, i);
		name = (*env)->CallObjectMethod(env, info, context->jfields.get_name);
		name_len = (*env)->GetStringUTFLength(env, name);
		name_ptr = (*env)->GetStringUTFChars(env, name, NULL);
		LOGI("get_media_codec_name coede name:%s",name_ptr);

		//查看是否编码器
		if(! (*env)->CallObjectMethod(env, info,context->jfields.is_encoder) )
		{
			LOGI("get_media_codec_name not encoder");
			goto loopclean;
		}

		//得到编码能力
		codec_capabilities = (*env)->CallObjectMethod(env, info,context->jfields.get_capabilities_for_type,jmime);
		if (CHECK_EXCEPTION())
		{
			LOGI("get_media_codec_name Exception occurred in MediaCodecInfo.getCapabilitiesForType");
			goto loopclean;
		} else if (codec_capabilities)
		{
			profile_levels = (*env)->GetObjectField(env, codec_capabilities,context->jfields.profile_levels_field);
			if (profile_levels)
				profile_levels_len = (*env)->GetArrayLength(env,profile_levels);
		}
		LOGI("get_media_codec_name Number of profile levels: %d", profile_levels_len);

		//得到支持的类型字符数组 codecInfo.getSupportedTypes()
		types = (*env)->CallObjectMethod(env, info,context->jfields.get_supported_types);
		num_types = (*env)->GetArrayLength(env, types);
		found = false;

		LOGI("get_media_codec_name num_types:%d",num_types);
		for (j = 0; j < num_types && !found; j++)
		{
		    jobject type = (*env)->GetObjectArrayElement(env, types, j);
		    if (!jstrcmp(env, type, psz_mime))
		    {
		    	 found = true;
		    }
		    (*env)->DeleteLocalRef(env, type);
		}

		if (found)
		{
			LOGI("get_media_codec_name using %.*s", name_len, name_ptr);
			context->psz_name = malloc(name_len + 1);
			memcpy(context->psz_name, name_ptr, name_len);
			context->psz_name[name_len] = '\0';
			jcodec_name = name;
		}

loopclean:
		if (name)
		{
			(*env)->ReleaseStringUTFChars(env, name, name_ptr);
			if (jcodec_name != name)
				(*env)->DeleteLocalRef(env, name);
		}
		if (types)
			(*env)->DeleteLocalRef(env, types);
		if (codec_capabilities)
			(*env)->DeleteLocalRef(env, codec_capabilities);
		if (info)
			(*env)->DeleteLocalRef(env, info);
		if (found)
			break;
	}



	LOGI("get_media_codec_name end");

	return jcodec_name;
}

media_codec_context* init_media_codec(JNIEnv *env)
{
	LOGI("init_media_codec_context begin");

	media_codec_context* _media_codec_context = (media_codec_context*)malloc(sizeof(media_codec_context));
	if(!_media_codec_context)
		return NULL;

	memset(_media_codec_context,0,sizeof(media_codec_context));

	//为classes赋值
	SetClassElement(_media_codec_context,"android/media/MediaCodecList", OFF(media_codec_list_class),0);
	SetClassElement(_media_codec_context,"android/media/MediaCodec",     OFF(media_codec_class),1);
	SetClassElement(_media_codec_context,"android/media/MediaFormat", 	OFF(media_format_class),2);
	SetClassElement(_media_codec_context,"android/media/MediaFormat", 	OFF(media_format_class),3);
	SetClassElement(_media_codec_context,"android/media/MediaCodec$BufferInfo", OFF(buffer_info_class),4);
	SetClassElement(_media_codec_context,"java/nio/ByteBuffer", 			OFF(byte_buffer_class),5);
	SetClassElement(_media_codec_context,NULL, 0,6);

	//为member赋值



    SetMemberElement(_media_codec_context,"toString", "()Ljava/lang/String;", "java/lang/Object", OFF(tostring), METHOD, true,0);

    SetMemberElement(_media_codec_context,"getCodecCount", "()I", "android/media/MediaCodecList", OFF(get_codec_count), STATIC_METHOD, true ,1);
    SetMemberElement(_media_codec_context,"getCodecInfoAt", "(I)Landroid/media/MediaCodecInfo;", "android/media/MediaCodecList", OFF(get_codec_info_at), STATIC_METHOD, true ,2);

    SetMemberElement(_media_codec_context,"isEncoder", "()Z", "android/media/MediaCodecInfo", OFF(is_encoder), METHOD, true ,3);
    SetMemberElement(_media_codec_context,"getSupportedTypes", "()[Ljava/lang/String;", "android/media/MediaCodecInfo", OFF(get_supported_types), METHOD, true ,4);
    SetMemberElement(_media_codec_context,"getName", "()Ljava/lang/String;", "android/media/MediaCodecInfo", OFF(get_name), METHOD, true ,5);
    SetMemberElement(_media_codec_context,"getCapabilitiesForType", "(Ljava/lang/String;)Landroid/media/MediaCodecInfo$CodecCapabilities;", "android/media/MediaCodecInfo", OFF(get_capabilities_for_type), METHOD, true ,6);

    SetMemberElement(_media_codec_context,"profileLevels", "[Landroid/media/MediaCodecInfo$CodecProfileLevel;", "android/media/MediaCodecInfo$CodecCapabilities", OFF(profile_levels_field), FIELD, true ,7);
    SetMemberElement(_media_codec_context,"profile", "I", "android/media/MediaCodecInfo$CodecProfileLevel", OFF(profile_field), FIELD, true ,8);
    SetMemberElement(_media_codec_context,"level", "I", "android/media/MediaCodecInfo$CodecProfileLevel", OFF(level_field), FIELD, true ,9);

	SetMemberElement(_media_codec_context,"createDecoderByType", "(Ljava/lang/String;)Landroid/media/MediaCodec;", "android/media/MediaCodec", OFF(create_by_codec_type), STATIC_METHOD, true ,10);
    SetMemberElement(_media_codec_context,"createByCodecName", "(Ljava/lang/String;)Landroid/media/MediaCodec;", "android/media/MediaCodec", OFF(create_by_codec_name), STATIC_METHOD, true ,11);
    SetMemberElement(_media_codec_context,"configure", "(Landroid/media/MediaFormat;Landroid/view/Surface;Landroid/media/MediaCrypto;I)V", "android/media/MediaCodec", OFF(configure), METHOD, true ,12);
    SetMemberElement(_media_codec_context,"start", "()V", "android/media/MediaCodec", OFF(start), METHOD, true ,13);
    SetMemberElement(_media_codec_context,"stop", "()V", "android/media/MediaCodec", OFF(stop), METHOD, true ,14);
    SetMemberElement(_media_codec_context,"flush", "()V", "android/media/MediaCodec", OFF(flush), METHOD, true ,15);
    SetMemberElement(_media_codec_context,"release", "()V", "android/media/MediaCodec", OFF(release), METHOD, true ,16);
    SetMemberElement(_media_codec_context,"getOutputFormat", "()Landroid/media/MediaFormat;", "android/media/MediaCodec", OFF(get_output_format), METHOD, true ,17);
    SetMemberElement(_media_codec_context,"getInputBuffers", "()[Ljava/nio/ByteBuffer;", "android/media/MediaCodec", OFF(get_input_buffers), METHOD, false ,18);
    SetMemberElement(_media_codec_context,"getInputBuffer", "(I)Ljava/nio/ByteBuffer;", "android/media/MediaCodec", OFF(get_input_buffer), METHOD, false ,19);
    SetMemberElement(_media_codec_context,"getOutputBuffers", "()[Ljava/nio/ByteBuffer;", "android/media/MediaCodec", OFF(get_output_buffers), METHOD, false ,20);
    SetMemberElement(_media_codec_context,"getOutputBuffer", "(I)Ljava/nio/ByteBuffer;", "android/media/MediaCodec", OFF(get_output_buffer), METHOD, false ,21);
    SetMemberElement(_media_codec_context,"dequeueInputBuffer", "(J)I", "android/media/MediaCodec", OFF(dequeue_input_buffer), METHOD, true ,22);
    SetMemberElement(_media_codec_context,"dequeueOutputBuffer", "(Landroid/media/MediaCodec$BufferInfo;J)I", "android/media/MediaCodec", OFF(dequeue_output_buffer), METHOD, true ,23);
    SetMemberElement(_media_codec_context,"queueInputBuffer", "(IIIJI)V", "android/media/MediaCodec", OFF(queue_input_buffer), METHOD, true ,24);
    SetMemberElement(_media_codec_context,"releaseOutputBuffer", "(IZ)V", "android/media/MediaCodec", OFF(release_output_buffer), METHOD, true ,25);

    SetMemberElement(_media_codec_context,"createVideoFormat", "(Ljava/lang/String;II)Landroid/media/MediaFormat;", "android/media/MediaFormat", OFF(create_video_format), STATIC_METHOD, true ,26);
    SetMemberElement(_media_codec_context,"setInteger", "(Ljava/lang/String;I)V", "android/media/MediaFormat", OFF(set_integer), METHOD, true ,27);
    SetMemberElement(_media_codec_context,"getInteger", "(Ljava/lang/String;)I", "android/media/MediaFormat", OFF(get_integer), METHOD, true ,28);
    SetMemberElement(_media_codec_context,"setByteBuffer", "(Ljava/lang/String;Ljava/nio/ByteBuffer;)V", "android/media/MediaFormat", OFF(set_bytebuffer), METHOD, true ,29);

   SetMemberElement(_media_codec_context, "<init>", "()V", "android/media/MediaCodec$BufferInfo", OFF(buffer_info_ctor), METHOD, true ,30);
    SetMemberElement(_media_codec_context,"size", "I", "android/media/MediaCodec$BufferInfo", OFF(size_field), FIELD, true ,31);
    SetMemberElement(_media_codec_context,"offset", "I", "android/media/MediaCodec$BufferInfo", OFF(offset_field), FIELD, true ,32);
    SetMemberElement(_media_codec_context,"presentationTimeUs", "J", "android/media/MediaCodec$BufferInfo", OFF(pts_field), FIELD, true ,33);
    SetMemberElement(_media_codec_context,NULL, NULL, NULL, 0, 0, false ,34);



	if(!InitJNIFields(env,_media_codec_context))
	{
		free(_media_codec_context);
		LOGE("init_media_codec_context InitJNIFields failed");
		return NULL;
	}
	if(!populate_static_fields(env,_media_codec_context))
	{
		free(_media_codec_context);
		LOGE("init_media_codec_context InitJNIFields populate_static_fields");
		return NULL;
	}

	LOGI("init_media_codec_context end");

	return _media_codec_context;
}

void uninit_media_codec(JNIEnv *env,media_codec_context* context)
{
	LOGI("uninit_media_codec_context begin");

	if(!context)
		return;

	UninitJNIFields(env,context);

	free(context);

	LOGI("uninit_media_codec_context end");
}

bool media_codec_start(JNIEnv *env,media_codec_context* context,const char *psz_mime,jobject media_format)
{
	LOGI("start_media_codec begin");

	if(!context || !psz_mime || !media_format)
		return false;


	bool ret = false;
	bool b_direct_rendering;
	jstring jmime = NULL;
	jobject jcodec = NULL;
	jobject jinput_buffers = NULL;
	jobject joutput_buffers = NULL;
	jobject jbuffer_info = NULL;


	 jmime = (*env)->NewStringUTF(env, psz_mime);
	 if (!jmime)
	     return ret;

	 LOGI("start_media_codec NewStringUTF jmime");

	 //创建编码对象 MediaCodec.createByCodecName
	jcodec = (*env)->CallStaticObjectMethod(env, context->jfields.media_codec_class,
			context->jfields.create_by_codec_type, jmime);
	if (CHECK_EXCEPTION())
	{
		LOGE("start_media_codec Exception occurred in MediaCodec.createByCodecType");
		goto error;
	}

	context->codec = (*env)->NewGlobalRef(env, jcodec);

	SAFE_RELEASE_LOCAL_REF_C(env,jmime)
	SAFE_RELEASE_LOCAL_REF_C(env,jcodec)

	//配置解码器
	(*env)->CallVoidMethod(env, context->codec, context->jfields.configure, media_format,NULL,NULL, 0);
	if (CHECK_EXCEPTION())
	{
		LOGE("start_media_codec Exception occurred in MediaCodec.configure");
		goto error;
	}

	//开始
	(*env)->CallVoidMethod(env, context->codec, context->jfields.start);
	if (CHECK_EXCEPTION())
	{
		LOGE("start_media_codec Exception occurred in MediaCodec.start");
	    goto error;
	}
	context->b_started = true;

	 LOGI("start_media_codec context->context->jfields.start end");

	//获得输入输出缓存
	 if (context->jfields.get_input_buffers && context->jfields.get_output_buffers)
	 {

	        jinput_buffers = (*env)->CallObjectMethod(env, context->codec,
	                                                  context->jfields.get_input_buffers);
	        if (CHECK_EXCEPTION())
	        {
	        	LOGE("start_media_codec Exception in MediaCodec.getInputBuffers");
	            goto error;
	        }
	        context->input_buffers = (*env)->NewGlobalRef(env, jinput_buffers);
	        SAFE_RELEASE_LOCAL_REF_C(env,jinput_buffers)

	        joutput_buffers = (*env)->CallObjectMethod(env, context->codec,
	                                                   context->jfields.get_output_buffers);
	        if (CHECK_EXCEPTION())
	        {
	        	LOGE("start_media_codec Exception in MediaCodec.getOutputBuffers");
	            goto error;
	        }
	        context->output_buffers = (*env)->NewGlobalRef(env, joutput_buffers);
	        SAFE_RELEASE_LOCAL_REF_C(env,joutput_buffers)
	 }
	 jbuffer_info = (*env)->NewObject(env, context->jfields.buffer_info_class,context->jfields.buffer_info_ctor);
	 context->buffer_info = (*env)->NewGlobalRef(env, jbuffer_info);
	 SAFE_RELEASE_LOCAL_REF_C(env,jbuffer_info)


	LOGI("start_media_codec end");

	return true;

error:
	SAFE_RELEASE_LOCAL_REF_C(env,jmime)
	SAFE_RELEASE_LOCAL_REF_C(env,jcodec)
	SAFE_RELEASE_LOCAL_REF_C(env,jinput_buffers)
	SAFE_RELEASE_LOCAL_REF_C(env,joutput_buffers)
	SAFE_RELEASE_LOCAL_REF_C(env,jbuffer_info)

	if (!ret)
		media_codec_stop(env,context);

	LOGE("start_media_codec error end");
	return ret;
}

bool media_codec_stop(JNIEnv *env,media_codec_context* context)
{
	LOGI("stop_media_codec begin");


	if (!context)
		return false;

	if (!context->b_started)
		return false;

	free(context->psz_name);

	if (context->input_buffers)
	{
		(*env)->DeleteGlobalRef(env, context->input_buffers);
		context->input_buffers = NULL;
	}
	if (context->output_buffers)
	{
		(*env)->DeleteGlobalRef(env, context->output_buffers);
		context->output_buffers = NULL;
	}
	if (context->codec)
	{
		if (context->b_started)
		{
			(*env)->CallVoidMethod(env, context->codec, context->jfields.stop);
			if (CHECK_EXCEPTION())
				LOGE("stop_media_codec Exception in MediaCodec.stop");
			context->b_started = false;
		}

		(*env)->CallVoidMethod(env, context->codec, context->jfields.release);
		if (CHECK_EXCEPTION())
			LOGE("stop_media_codec Exception in MediaCodec.release");
		(*env)->DeleteGlobalRef(env, context->codec);
		context->codec = NULL;
	}
	if (context->buffer_info)
	{
		(*env)->DeleteGlobalRef(env, context->buffer_info);
		context->buffer_info = NULL;
	}
	LOGI("stop_media_codec MediaCodec via JNI closed");

	LOGI("stop_media_codec end");

	return true;
}

bool media_codec_decode(JNIEnv *env,media_codec_context* context,const unsigned char* p_buf, int i_size,int64_t timestamp)
 {
	LOGI("media_codec_encode begin");
	if (!context || !p_buf || i_size <= 0)
		return false;

	bool ret = false;

	int index;
	uint8_t *p_mc_buf;
	jobject j_mc_buf;
	jsize j_mc_size;
	jint jflags = 0;


	//出队列
	index = (*env)->CallIntMethod(env, context->codec,context->jfields.dequeue_input_buffer, INPUT_BUFFER_TIMEOUT_US);
	if (CHECK_EXCEPTION())
	{
		LOGE("media_codec_encode Exception occurred in MediaCodec.dequeueInputBuffer");
		return ret;
	}
	LOGI("media_codec_encode index: %d", index);
	if (index < 0)
		return ret;
	LOGI("media_codec_encode index: %d", index);

	if (context->jfields.get_input_buffers)
		j_mc_buf = (*env)->GetObjectArrayElement(env, context->input_buffers,index);
	else
		j_mc_buf = (*env)->CallObjectMethod(env, context->codec,context->jfields.get_input_buffer, index);

	j_mc_size = (*env)->GetDirectBufferCapacity(env, j_mc_buf);
	p_mc_buf = (*env)->GetDirectBufferAddress(env, j_mc_buf);
	if (j_mc_size < 0)
	{
		LOGE("media_codec_encode Java buffer has invalid size");
		(*env)->DeleteLocalRef(env, j_mc_buf);
		return false;
	}

	if ((size_t) j_mc_size > i_size)
		j_mc_size = i_size;
	memcpy(p_mc_buf, p_buf, j_mc_size);

	(*env)->CallVoidMethod(env, context->codec, context->jfields.queue_input_buffer,index, 0, j_mc_size, timestamp, jflags);
	(*env)->DeleteLocalRef(env, j_mc_buf);
	if (CHECK_EXCEPTION())
	{
		LOGE("media_codec_encode Exception in MediaCodec.queueInputBuffer");
		return false;
	}

	LOGI("media_codec_encode end");

	return true;
}

int media_codec_get_frame(JNIEnv *env,media_codec_context* context,unsigned char** p_out,int* out_len,int64_t* out_timestamp, int* output_flags,int64_t timeout)
{
	LOGI("media_codec_get_frame begin");
	if (!context || !p_out || !out_len )
		return -1;

	int i_index;


	i_index = (*env)->CallIntMethod(env, context->codec,context->jfields.dequeue_output_buffer, context->buffer_info, timeout);
	if (CHECK_EXCEPTION())
	{
		LOGE("media_codec_get_frame Exception in MediaCodec.dequeueOutputBuffer");
		return -1;
	}

	if (i_index >= 0)
	{
		jobject buf;
		uint8_t *ptr;
		int offset;
		if (context->jfields.get_output_buffers)
			buf = (*env)->GetObjectArrayElement(env, context->output_buffers,i_index);
		else
			buf = (*env)->CallObjectMethod(env, context->codec,context->jfields.get_output_buffer, i_index);

		 ptr = (*env)->GetDirectBufferAddress(env, buf);

		 offset = (*env)->GetIntField(env, context->buffer_info,context->jfields.offset_field);
		 *p_out = ptr + offset;
		 *out_len = (*env)->GetIntField(env, context->buffer_info,context->jfields.size_field);
		 (*env)->DeleteLocalRef(env, buf);

		 LOGI("media_codec_get_frame offset:%d", offset);

	}
	else if (i_index == INFO_OUTPUT_FORMAT_CHANGED)
	{
		jobject format = NULL;
		jobject format_string = NULL;
		jsize format_len;
		const char *format_ptr;
		LOGI("media_codec_get_frame output format changed");
		format = (*env)->CallObjectMethod(env, context->codec,context->jfields.get_output_format);
		if (CHECK_EXCEPTION())
		{
			LOGE("media_codec_get_frame Exception in MediaCodec.getOutputFormat");
			return i_index;
		}
		format_string = (*env)->CallObjectMethod(env, format, context->jfields.tostring);
		format_len = (*env)->GetStringUTFLength(env, format_string);
		format_ptr = (*env)->GetStringUTFChars(env, format_string, NULL);

		LOGI("media_codec_get_frame format_string:%s", format_ptr);
		(*env)->ReleaseStringUTFChars(env, format_string, format_ptr);

		(*env)->DeleteLocalRef(env, format);

	} else if (i_index == INFO_OUTPUT_BUFFERS_CHANGED)
	{
		jobject joutput_buffers;

		LOGI("media_codec_get_frame output buffers changed");
		if (!context->jfields.get_output_buffers)
			return i_index;
		(*env)->DeleteGlobalRef(env, context->output_buffers);

		joutput_buffers = (*env)->CallObjectMethod(env, context->codec,context->jfields.get_output_buffers);
		if (CHECK_EXCEPTION())
		{
			LOGE("media_codec_get_frame Exception in MediaCodec.getOutputBuffer");
			context->output_buffers = NULL;
			return i_index;
		}
		context->output_buffers = (*env)->NewGlobalRef(env, joutput_buffers);
		(*env)->DeleteLocalRef(env, joutput_buffers);
	}

	LOGI("media_codec_get_frame end");

	return i_index;
}

bool media_codec_release_output(JNIEnv *env,media_codec_context* context, int i_index)
{
	LOGI("release_output begin");

    (*env)->CallVoidMethod(env,context->codec, context->jfields.release_output_buffer,i_index);
    if (CHECK_EXCEPTION())
    {
    	LOGE("Exception in MediaCodec.releaseOutputBuffer");
        return false;
    }

    LOGI("release_output end");

    return true;
}




