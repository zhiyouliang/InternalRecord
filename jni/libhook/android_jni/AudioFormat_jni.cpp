/*
 * AudioFormat_jni.cpp
 *
 *  Created on: 2015年7月7日
 *      Author: Administrator
 */

#include "AudioFormat_jni.h"

int AudioFormat_jni::CHANNEL_OUT_MONO()
{
	return 4;
}

int AudioFormat_jni::CHANNEL_OUT_STEREO()
{
	return 12;
}

int AudioFormat_jni::ENCODING_PCM_16BIT()
{
	return 2;
}

int AudioFormat_jni::ENCODING_PCM_8BIT()
{
	return 3;
}


