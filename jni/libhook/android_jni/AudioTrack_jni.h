/*
 * AudioTrack_jni.h
 *
 *  Created on: 2015年7月3日
 *      Author: Administrator
 */

#ifndef LIBHOOK_ANDROID_JNI_AUDIOTRACK_JNI_H_
#define LIBHOOK_ANDROID_JNI_AUDIOTRACK_JNI_H_


typedef struct __tag_AudioTrack_jni AudioTrack_jni;

AudioTrack_jni* InitAudioTrack_jni(JNIEnv* env);
void UninitAudioTrack_jni(AudioTrack_jni* audio_track_context ,JNIEnv* env);
int AudioTrack_getMinBufferSize(AudioTrack_jni* audio_track_context ,JNIEnv* env,int sampleRateInHz, int channelConfig, int audioFormat);
int AudioTrack_Config(AudioTrack_jni* audio_track_context ,JNIEnv* env,int streamType, int sampleRateInHz, int channelConfig, int audioFormat, int bufferSizeInBytes, int mode);
int AudioTrack_write(AudioTrack_jni* audio_track_context ,JNIEnv* env,jbyte* audioData, int offsetInBytes, int sizeInBytes);
void AudioTrack_play(AudioTrack_jni* audio_track_context ,JNIEnv* env);
void AudioTrack_pause(AudioTrack_jni* audio_track_context ,JNIEnv* env);
void AudioTrack_release(AudioTrack_jni* audio_track_context ,JNIEnv* env);
void AudioTrack_flush(AudioTrack_jni* audio_track_context ,JNIEnv* env);
int AudioTrack_setStereoVolume(AudioTrack_jni* audio_track_context ,JNIEnv* env,float leftVolume, float rightVolume);
int AudioTrack_MODE_STREAM();

#endif /* LIBHOOK_ANDROID_JNI_AUDIOTRACK_JNI_H_ */
