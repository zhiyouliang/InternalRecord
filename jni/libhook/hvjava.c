/*******************************************************************************
 * Software Name :safe
 *
 * Copyright (C) 2014 HanVideo.
 *
 * author : 梁智游
 ******************************************************************************/


#include <pthread.h>
#include "hvjava.h"

JavaVM *hv_vm = 0;
pthread_key_t jnienv_key;

JNIEnv *hv_get_java_env(void);

void android_key_cleanup(void *data){

	JNIEnv* env=(JNIEnv*)pthread_getspecific(jnienv_key);
	if (env != 0) {
		(*hv_vm)->DetachCurrentThread(hv_vm);
		pthread_setspecific(jnienv_key,0);
	}
}


void hv_set_jvm(JavaVM *vm){
	hv_vm = vm;
	pthread_key_create(&jnienv_key,android_key_cleanup);
}

JavaVM *ms_get_jvm(void){
	return hv_vm;
}

JNIEnv *hv_get_jni_env(int* nead_detach)
{
	JNIEnv *env=0;
	if ( 0 == hv_vm )
		return 0;

	*nead_detach = 0;

	env = hv_get_java_env();
	if(env)
	{
		return env;
	}
	else
	{

		env=(JNIEnv*)pthread_getspecific(jnienv_key);
		if (env==0)
		{
			if ((*hv_vm)->AttachCurrentThread(hv_vm,&env,0)!=0)
			{
				return 0;
			}
			pthread_setspecific(jnienv_key,env);

			*nead_detach = 1;
		}
	}

	return env;
}

void hv_detach_jni_env(JNIEnv *env)
{
	if ( !env )
		return;

	if (hv_vm == 0)
		return;

	(*hv_vm)->DetachCurrentThread(hv_vm);
}

JNIEnv *hv_get_java_env(void)
{
	JNIEnv *env=0;
	if (hv_vm == 0)
	{
		return 0;
	}
	else
	{
		if ((*hv_vm)->GetEnv(hv_vm,(void**) &env, JNI_VERSION_1_6) != JNI_OK)
		{
		    return 0;
		}
	}

	return env;
}
