/*******************************************************************************
 * Software Name :safe
 *
 * Copyright (C) 2014 HanVideo.
 *
 * author : 梁智游
 ******************************************************************************/
#ifndef _NETWORK_H
#define _NETWORK_H


#ifdef __cplusplus extern "C" {
#endif



#ifndef Bool
typedef enum {
	HV_FALSE = 0,
	HV_TRUE
} Bool;
#endif

typedef enum
{

	/*!
	\n\n
	*/
	/*!Operation success (no error).*/
	HV_OK								= 0,
	/*!\n*/
	/*!One of the input parameter is not correct or cannot be used in the current operating mode of the framework.*/
	HV_BAD_PARAM							= -1,
	/*! Memory allocation failure.*/
	HV_OUT_OF_MEM							= -2,
	/*! Input/Output failure (disk access, system call failures)*/
	HV_IO_ERR								= -3,
	/*! An service error has occured at the local side*/
	HV_SERVICE_ERROR						= -13,
	/*! The connection to the remote peer has failed*/
	HV_IP_CONNECTION_FAILURE				= -41,
	/*! The network operation has failed*/
	HV_IP_NETWORK_FAILURE					= -42,
	/*! The network connection has been closed*/
	HV_IP_CONNECTION_CLOSED					= -43,
	/*! The network operation has failed because no data is available*/
	HV_IP_NETWORK_EMPTY						= -44,
	/*! The network operation has been discarded because it would be a blocking one*/
	HV_IP_SOCK_WOULD_BLOCK					= -45,
} HV_Err;

#define SOCK_SEC_WAIT		10
#define SOCK_MICROSEC_WAIT	500

enum
{
	/*!Reuses port.*/
	HV_SOCK_REUSE_PORT = 1,
	/*!Forces IPV6 if available.*/
	HV_SOCK_FORCE_IPV6 = 1<<1
};


typedef struct __tag_socket HV_Socket;

/*!socket is a TCP socket*/
#define HV_SOCK_TYPE_TCP		0x01
/*!socket is a UDP socket*/
#define HV_SOCK_TYPE_UDP		0x02

HV_Socket *hv_sk_new(unsigned int SocketType);

void hv_sk_del(HV_Socket *sock);

void hv_sk_free(HV_Socket *sock);

HV_Err hv_sk_set_buffer_size(HV_Socket *sock, Bool send_buffer, unsigned int new_size);

HV_Err hv_sk_set_block_mode(HV_Socket *sock, Bool NonBlockingOn);

HV_Err hv_sk_connect(HV_Socket *sock, const char *peer_ip, unsigned short port);

HV_Err hv_sk_send(HV_Socket *sock, const char *buffer, unsigned int length);

HV_Err hv_sk_receive(HV_Socket *sock, char *buffer, unsigned int length, unsigned int start_from, unsigned int *read);

int hv_sk_get_handle(HV_Socket *sock);


#ifdef __cplusplus }
#endif

#endif	//_NETWORK_H


