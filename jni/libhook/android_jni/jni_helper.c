/*
 * jni_helper.cpp
 *
 *  Created on: 2015年7月27日
 *      Author: Administrator
 */

#include "jni_helper.h"



bool check_exception(JNIEnv *env)
{
	jthrowable swigerror = (*env)->ExceptionOccurred(env);
    if (swigerror)
    {
    	(*env)->ExceptionDescribe(env);
        (*env)->ExceptionClear(env);
        SAFE_RELEASE_LOCAL_REF_C(env,swigerror)
        return true;
    }
    else
        return false;
}





