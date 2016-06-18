/*
 * Copyright (C) 2009 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include <jni.h>
#include <fcntl.h>
#include <dlfcn.h>
#include "tool.h"
#include <sys/system_properties.h>





//////////////////////////////////////////////////////----
#include "InternalRecord.h"
#include <hook.h>
#include <hooks/io.h>
extern "C"
{
#include "hvjava.h"
}
#include <media/AudioTrack.h>


typedef struct
{
    const char *name;
    uintptr_t   original;
    uintptr_t   hook;
}
hook_t;


static hook_t __hooks[] = {
	//AudioTrack set
	ADDHOOK( _ZN7android10AudioTrack3setE19audio_stream_type_tj14audio_format_tji20audio_output_flags_tPFviPvS4_ES4_iRKNS_2spINS_7IMemoryEEEbi ),
	//AudioTrack start
	ADDHOOK( _ZN7android10AudioTrack5startEv ),
	//AudioTrack pause
	ADDHOOK( _ZN7android10AudioTrack5pauseEv ),
	//AudioTrack stop
	ADDHOOK( _ZN7android10AudioTrack4stopEv ),
	//AudioTrack write
	ADDHOOK( _ZN7android10AudioTrack5writeEPKvj ),
	ADDHOOK( _ZN7android10AudioTrack5writeEPKvjb ),
	//AudioTrack releaseBuffer
	ADDHOOK( _ZN7android10AudioTrack13releaseBufferEPNS0_6BufferE ),
	//MediaPlayer setDataSource
	ADDHOOK(_ZN7android11MediaPlayer13setDataSourceEixx),
	//MediaPlayer disconnect
	ADDHOOK(_ZN7android11MediaPlayer10disconnectEv),
	//MediaPlayer start
	ADDHOOK(_ZN7android11MediaPlayer5startEv),
	//MediaPlayer pause
	ADDHOOK(_ZN7android11MediaPlayer5pauseEv),
	//MediaPlayer stop
	ADDHOOK(_ZN7android11MediaPlayer4stopEv),
	//MediaPlay::setLooping
	ADDHOOK(_ZN7android11MediaPlayer10setLoopingEi),
	//SoundPool::load
	ADDHOOK(_ZN7android9SoundPool4loadEPKci),
	ADDHOOK(_ZN7android9SoundPool4loadEixxi),
	//SoundPool::unload
	ADDHOOK(_ZN7android9SoundPool6unloadEi),
	//SoundPool::play
	ADDHOOK(_ZN7android9SoundPool4playEiffiif),
	//SoundPool::stop
	ADDHOOK(_ZN7android9SoundPool4stopEi),
	//SoundPool::setLoop
	ADDHOOK(_ZN7android9SoundPool7setLoopEii),
};

#define NHOOKS ( sizeof(__hooks) / sizeof(__hooks[0] ) )




uintptr_t find_original( const char *name ) {
    for( size_t i = 0; i < NHOOKS; ++i ) {
        if( strcmp( __hooks[i].name, name ) == 0 ){
            return __hooks[i].original;
        }
    }

    LOGI( "[%d] !!! COULD NOT FIND ORIGINAL POINTER OF FUNCTION '%s' !!!", getpid(), name );

    return 0;
}



void __attribute__ ((constructor)) libhook_main()
{
	  LOGI( "LIBRARY LOADED FROM PID %d.", getpid() );

	    // get a list of all loaded modules inside this process.
	    ld_modules_t modules = ir_get_modules();

	    LOGI( "Found %u loaded modules.", modules.size() );
	    LOGI( "Installing %u hooks.", NHOOKS );

	    for( ld_modules_t::const_iterator i = modules.begin(), e = modules.end(); i != e; ++i ){
	        // don't hook ourself :P
	        if( i->name.find( "libinternal_audio_record.so" ) == std::string::npos ) {
	            LOGI( "[0x%X] Hooking %s ...", i->address, i->name.c_str() );

	            for( size_t j = 0; j < NHOOKS; ++j ) {

	            	// LOGI( "ir_add_new 1 __hooks[j].hook:%d", __hooks[j].hook );


	                unsigned tmp = ir_add_new( i->name.c_str(), __hooks[j].name, __hooks[j].hook );

	                //LOGI( "ir_add_new 2 __hooks[j].hook:%d", __hooks[j].hook );

	                // update the original pointer only if the reference we found is valid
	                // and the pointer itself doesn't have a value yet.
	                if( __hooks[j].original == 0 && tmp != 0 ){
	                    __hooks[j].original = (uintptr_t)tmp;

	                   // LOGI( "  %s - 0x%x -> 0x%x", __hooks[j].name, __hooks[j].original, __hooks[j].hook );
	                }
	            }
	        }
	    }
}

void __attribute__ ((constructor)) hook_libjavacore()
{
	LOGI("LIBRARY LOADED FROM PID %d.", getpid());

	for (size_t j = 0; j < NHOOKS; ++j)
	{
		unsigned tmp = ir_add_new("libmedia.so", __hooks[j].name,__hooks[j].hook);

		// update the original pointer only if the reference we found is valid
		// and the pointer itself doesn't have a value yet.
		if (__hooks[j].original == 0 && tmp != 0) {
			__hooks[j].original = (uintptr_t) tmp;
		}
	}

	for (size_t j = 0; j < NHOOKS; ++j)
	{
			unsigned tmp = ir_add_new("libandroid_runtime.so", __hooks[j].name,__hooks[j].hook);

			//LOGI( "ir_add_new 2 __hooks[j].hook:%d", __hooks[j].hook );

			// update the original pointer only if the reference we found is valid
			// and the pointer itself doesn't have a value yet.
			if (__hooks[j].original == 0 && tmp != 0) {
				__hooks[j].original = (uintptr_t) tmp;
			}
	}

	for (size_t j = 0; j < NHOOKS; ++j)
	{
			unsigned tmp = ir_add_new("libsoundpool.so", __hooks[j].name,__hooks[j].hook);

			//LOGI( "ir_add_new 2 __hooks[j].hook:%d", __hooks[j].hook );

			// update the original pointer only if the reference we found is valid
			// and the pointer itself doesn't have a value yet.
			if (__hooks[j].original == 0 && tmp != 0) {
				__hooks[j].original = (uintptr_t) tmp;
			}
	}

}



int IR_SetInternalRecordCallback(InternalRecordCallback cb)
{
	if(!cb)
		return -1;

	return IO_SetInternalRecordCallback(cb);
}

int IR_InitInternalAudioRecord(void* vm)
{
	int ret = -1;

	if(!vm )
		return ret;

	//hv_set_jvm((JavaVM*)vm);


	if(!IO_InitInternalAudioHandler((JavaVM*)vm))
		return ret;



	return 0;

}

int IR_GetSampleRate()
{
	return IO_GetSampleRate();
}

int IR_GetChannel()
{
	return IO_GetChannel();
}

int IR_GetBit()
{
	return IO_GetBit();
}

int IR_GetTotalLen()
{
	return IO_GetTotalLen();
}

int IR_GetData(char* data,int len)
{
	return IO_GetData(data,len);
}

void IR_UninitInternalAudioRecord()
{
	IO_UninitInternalAudioHandler();


}

int IR_StartRecord()
{
	return IO_StartRecord();

}

int IR_StopRecord()
{
	return IO_StopRecord();
}


int GetSDKVersion()
{
	int version = 0;
	char szSdkVer[256] = {0};
	__system_property_get("ro.build.version.sdk", szSdkVer);

	version = atoi(szSdkVer);

	return  version;
}


#if 1
//////////////////////////////////////////////////////----

#define RGGISTER_CLASS_NAME		"com/piterwilson/mp3streamplayer/MainActivity"
#define DEMO_TEST      0

/* This is a trivial JNI example where we use a native method
 * to return a new VM String. See the corresponding Java source
 * file located at:
 *
 *   apps/samples/hello-jni/project/src/com/example/hellojni/HelloJni.java
 *   com_paranerd_mediaplayer_Activities_HomeActivity
 */


void* libHookHelper = 0;
void (*hook_entry)();

void InternalRecordCallbackFun(const char* data,int data_len)
{
	static FILE *f = fopen("/sdcard/recordtest.pcm","a+b");

	fwrite(data, 1, data_len, f);
	fflush(f);
}





static JNINativeMethod gMethods[] = {


};

/*
* 为某一个类注册本地方法
*/
static int registerNativeMethods(JNIEnv* env, const char* className, JNINativeMethod* gMethods, int numMethods)
{
    jclass clazz;
    clazz = env->FindClass(className);
    if (clazz == NULL) {
        return JNI_FALSE;
    }
    if (env->RegisterNatives( clazz, gMethods, numMethods) < 0) {
        return JNI_FALSE;
    }

    return JNI_TRUE;
}


/*
* 为所有类注册本地方法
*/
static int registerNatives(JNIEnv* env) {
    const char* kClassName = RGGISTER_CLASS_NAME;//指定要注册的类
    return registerNativeMethods(env, kClassName, gMethods,
            sizeof(gMethods) / sizeof(gMethods[0]));
}



jint JNI_OnLoad(JavaVM* vm, void* reserved) {

	JNIEnv* env = 0;
	int result = -1;

	hv_set_jvm(vm);

	libhook_main();

	IO_InitInternalAudioHandler(vm);
#if DEMO_TEST
	IR_SetInternalRecordCallback(InternalRecordCallbackFun);
	IR_StartRecord();
#endif

	if (vm->GetEnv((void**) &env, JNI_VERSION_1_6) != JNI_OK)
	{
	    return -1;
	}

	result = JNI_VERSION_1_6;


	return result;
}

JNIEXPORT void JNICALL JNI_OnUnload(JavaVM* vm, void* reserved)
{
	JNIEnv* env = 0;

	IO_UninitInternalAudioHandler();

	if (vm->GetEnv((void**) &env, JNI_VERSION_1_6) != JNI_OK)
		return;

}

#endif


