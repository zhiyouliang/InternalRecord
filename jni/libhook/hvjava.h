/*******************************************************************************
 * Software Name :safe
 *
 * Copyright (C) 2014 HanVideo.
 *
 * author : 梁智游
 ******************************************************************************/

#ifndef HV_JAVA_H
#define HV_JAVA_H

#include<jni.h>


void hv_set_jvm(JavaVM *vm);

JavaVM *ms_get_jvm(void);

JNIEnv *hv_get_jni_env(int* nead_detach);

void hv_detach_jni_env(JNIEnv *env);






#endif //HV_JAVA_H
