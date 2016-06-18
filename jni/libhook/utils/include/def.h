/*******************************************************************************
 * Software Name :safe
 *
 * Copyright (C) 2014 HanVideo.
 *
 * author : 梁智游
 ******************************************************************************/

#ifndef DEF_H_
#define DEF_H_ 

#include <jni.h>

//head file
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
//#include <sys/time.h>
#if defined(WIN32)
#include <windows.h>
#endif

//type define
#define bool  unsigned char
#define true  1
#define false 0

typedef enum
{
	SUNTECH_OK								= 0,
	SUNTECH_BAD_PARAM						= -1,
	SUNTECH_OUT_OF_MEM						= -2,
	SUNTECH_SERVICE_ERROR					= -13,
	SUNTECH_IP_ADDRESS_NOT_FOUND			= -40,
	SUNTECH_IP_CONNECTION_FAILURE			= -41,
	SUNTECH_IP_NETWORK_FAILURE				= -42,
	SUNTECH_IP_CONNECTION_CLOSED			= -43,
	SUNTECH_IP_NETWORK_EMPTY				= -44,
	SUNTECH_IP_SOCK_WOULD_BLOCK				= -45,
}SUNTECH_Err;

#define INVALID_HANDLE 						-1

#define SIZE_AUDIO_FRAME 960
#define SIZE_AUDIO_FRAME_SMALL 480
#define SIZE_AUDIO_PACKED 20//30
 

#define SOCKET_RECV_BUFFER_SIZE      1024*10
#define PCM_PACKET_BUF_LEN			 1024*64//1024*256      

#define LISTEN_PORT                  9091

#define NOT_USE_MIXER                   0

#define USE_G729B                       0

#ifndef USE_DSOUND_RECORD
#define USE_DSOUND_RECORD  (0)
#endif

#ifndef USE_WAVEIN_CLASS
#define USE_WAVEIN_CLASS  (0)
#endif

#ifndef USE_DSOUND_PLAY
#define USE_DSOUND_PLAY  (0)
#endif

#define CAPTURE_VALID  1
#define RENDER_VALID   1

enum 
{
	PACKETTYPE_AUDIO_DATA = 0,
	PACKETTYPE_EXIT_GROUP_CHAT = 1
};


typedef struct __tag_AudioCodePacketHeader
{
	int PacketType;
	int OutLen;
	char LocalIP[16];	
}AudioCodePacketHeader; 

typedef struct __tag_AudioCodePacket
{
	AudioCodePacketHeader hdr;
	union {
	char data1[SIZE_AUDIO_FRAME];				//big
	char data2[SIZE_AUDIO_FRAME_SMALL];         //small
	}data;
}AudioCodePacket; 



#endif //DEF_H_
