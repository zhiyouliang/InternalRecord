/*
 * Copyright (c) 2015, Simone Margaritelli <evilsocket at gmail dot com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of ARM Inject nor the names of its contributors may be used
 *     to endorse or promote products derived from this software without
 *     specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef IO_H
#define IO_H

#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <media/AudioTrack.h>
#include <jni.h>

#include "InternalRecord.h"
#include "def.h"






extern "C"
{
#include "timer.h"
}

using namespace android;


/*android 函数声明*/

//AudioTrack
status_t new__ZN7android10AudioTrack3setE19audio_stream_type_tj14audio_format_tji20audio_output_flags_tPFviPvS4_ES4_iRKNS_2spINS_7IMemoryEEEbi(void *obj,audio_stream_type_t streamType,
        uint32_t sampleRate,
        audio_format_t format,
        audio_channel_mask_t channelMask,
        int frameCount,
        audio_output_flags_t flags,
		AudioTrack::callback_t cbf,
        void* user,
        int notificationFrames,
        const sp<IMemory>& sharedBuffer,
		bool threadCanCallJava,
        int sessionId);

void new__ZN7android11MediaPlayer10disconnectEv(void *obj);

void     new__ZN7android10AudioTrack5startEv(void *obj);
void     new__ZN7android10AudioTrack5pauseEv(void *obj);
void     new__ZN7android10AudioTrack4stopEv(void *obj);
ssize_t  new__ZN7android10AudioTrack5writeEPKvj(void *obj,const void* buffer, size_t userSize);
ssize_t  new__ZN7android10AudioTrack5writeEPKvjb(void *obj,const void* buffer, size_t size, bool blocking = true);
void     new__ZN7android10AudioTrack13releaseBufferEPNS0_6BufferE(void *obj,AudioTrack::Buffer* audioBuffer);

//MediaPlayer
status_t new__ZN7android11MediaPlayer13setDataSourceEixx(void *obj,int fd, int64_t offset, int64_t length);
status_t new__ZN7android11MediaPlayer5startEv(void *obj);
status_t new__ZN7android11MediaPlayer4stopEv(void *obj);
status_t new__ZN7android11MediaPlayer5pauseEv(void *obj);
void     new__ZN7android11MediaPlayer10setLoopingEi(void *obj,bool looping);

//SoundPool
int new__ZN7android9SoundPool4loadEPKci(void *obj,const char* url, int priority);
int new__ZN7android9SoundPool4loadEixxi(void *obj,int fd, int64_t offset, int64_t length, int priority);
bool new__ZN7android9SoundPool6unloadEi(void *obj,int sampleID);
int new__ZN7android9SoundPool4playEiffiif(void *obj,int sampleID, float leftVolume, float rightVolume, int priority,
        int loop, float rate);
void new__ZN7android9SoundPool4stopEi(void *obj,int channelID);
void new__ZN7android9SoundPool7setLoopEii(void *obj,int channelID,int loop);





/* 录音处理函数 */

int IO_SetInternalRecordCallback(InternalRecordCallback cb);
int IO_InitInternalAudioHandler(JavaVM* vm);
int IO_GetSampleRate();
int IO_GetChannel();
int IO_GetBit();
int IO_GetTotalLen();
int IO_GetData(char* data,int len);
void IO_UninitInternalAudioHandler();
int IO_StartRecord();
int IO_StopRecord();

#endif
