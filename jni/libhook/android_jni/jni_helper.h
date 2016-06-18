/*
 * jni_helper.h
 *
 *  Created on: 2015年7月27日
 *      Author: Administrator
 */

#ifndef LIBHOOK_ANDROID_JNI_JNI_HELPER_H_
#define LIBHOOK_ANDROID_JNI_JNI_HELPER_H_

#include <jni.h>
#include <assert.h>
#include "def.h"

#define SAFE_RELEASE_LOCAL_REF(e,x) if ((x) != 0){(e)->DeleteLocalRef(x); (x) = 0;}
#define SAFE_RELEASE_GLOBAL_REF(e,x) if ((x) != 0){(e)->DeleteGlobalRef(x); (x) = 0;}

#define SAFE_RELEASE_LOCAL_REF_C(e,x) if ((x) != 0){((*e))->DeleteLocalRef(e,x); (x) = 0;}
#define SAFE_RELEASE_GLOBAL_REF_C(e,x) if ((x) != 0){((*e))->DeleteGlobalRef(e,x); (x) = 0;}

#define CHECK_EXCEPTION() check_exception( env )

bool check_exception(JNIEnv *env);


#endif /* LIBHOOK_ANDROID_JNI_JNI_HELPER_H_ */
