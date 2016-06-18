/*******************************************************************************
 * Software Name :safe
 *
 * Copyright (C) 2014 HanVideo.
 *
 * author : 梁智游
 ******************************************************************************/

#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>

#include "../include/network.h"

#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define LASTSOCKERROR errno

/*internal flags*/
enum
{
	HV_SOCK_IS_TCP = 1<<9,
	HV_SOCK_IS_IPV6 = 1<<10,
	HV_SOCK_NON_BLOCKING = 1<<11,
	HV_SOCK_IS_MULTICAST = 1<<12,
	HV_SOCK_IS_LISTENING = 1<<13,
	/*socket is bound to a specific dest (server) or source (client) */
	HV_SOCK_HAS_PEER = 1<<14,
	HV_SOCK_IS_MIP = 1<<15
};

struct __tag_socket
{
	unsigned int flags;
	int socket;
	struct sockaddr_in dest_addr;
	unsigned int dest_addr_len;
};

void hv_sk_free(HV_Socket *sock)
{
	if ( !sock )
		return;

	/*leave multicast*/
	if (sock->socket && (sock->flags & HV_SOCK_IS_MULTICAST) )
	{
		struct ip_mreq mreq;

		mreq.imr_multiaddr.s_addr = sock->dest_addr.sin_addr.s_addr;
		mreq.imr_interface.s_addr = INADDR_ANY;
		setsockopt(sock->socket, IPPROTO_IP, IP_DROP_MEMBERSHIP, (char *) &mreq, sizeof(mreq));

	}

	if (sock->socket)
		//closesocket(sock->socket);
		close(sock->socket);

	sock->socket = (int) 0L;


}



HV_Socket *hv_sk_new(unsigned int SocketType)
{
	HV_Socket *tmp;

		/*init WinSock*/
	if ((SocketType != HV_SOCK_TYPE_UDP) && (SocketType != HV_SOCK_TYPE_TCP))
			return 0;

		tmp = (HV_Socket*)malloc(sizeof(HV_Socket));
		if (tmp )
			memset(tmp,0,sizeof(HV_Socket));
		else
			return 0;


		if (SocketType == HV_SOCK_TYPE_TCP)
			tmp->flags |= HV_SOCK_IS_TCP;


		memset(&tmp->dest_addr, 0, sizeof(struct sockaddr_in));
		tmp->dest_addr_len = sizeof(struct sockaddr);

		return tmp;
}

void hv_sk_del(HV_Socket *sock)
{
	if( !sock )
		return;

	hv_sk_free(sock);

	free(sock);
}

HV_Err hv_sk_set_buffer_size(HV_Socket *sock, Bool SendBuffer, unsigned int NewSize)
{
	int res;
	if (!sock || !sock->socket)
		return HV_BAD_PARAM;

	if (SendBuffer)
	{
		res = setsockopt(sock->socket, SOL_SOCKET, SO_SNDBUF, (char *) &NewSize, sizeof(unsigned int) );
	}
	else
	{
		res = setsockopt(sock->socket, SOL_SOCKET, SO_RCVBUF, (char *) &NewSize, sizeof(unsigned int) );
	}

	if (res<0)
	{
		//HV_LOG(HV_LOG_ERROR, HV_LOG_NETWORK, ("[Core] Couldn't set socket %s buffer size: %d\n", SendBuffer ? "send" : "receive", res));
	}
	else
	{
		//HV_LOG(HV_LOG_DEBUG, HV_LOG_NETWORK, ("[Core] Set socket %s buffer size\n", SendBuffer ? "send" : "receive"));
	}
	return HV_OK;
}

HV_Err hv_sk_set_block_mode(HV_Socket *sock, Bool NonBlockingOn)
{
	int res;

	int flag = fcntl(sock->socket, F_GETFL, 0);
	if (sock->socket)
	{
		res = fcntl(sock->socket, F_SETFL, flag | O_NONBLOCK);
		if (res) return HV_SERVICE_ERROR;
	}

	if (NonBlockingOn)
	{
		sock->flags |= HV_SOCK_NON_BLOCKING;
	}
	else
	{
		sock->flags &= ~HV_SOCK_NON_BLOCKING;
	}
	return HV_OK;
}



HV_Err hv_sk_connect(HV_Socket *sock, const char *peer_ip, unsigned short PortNumber)
{
	int ret;


	if (!sock->socket)
	{
		sock->socket = socket(AF_INET, (sock->flags & HV_SOCK_IS_TCP) ? SOCK_STREAM : SOCK_DGRAM, 0);
		int flag = fcntl(sock->socket, F_GETFL, 0);
		if (sock->flags & HV_SOCK_NON_BLOCKING)
			hv_sk_set_block_mode(sock, 1);
	}

	/*setup the address*/
	sock->dest_addr.sin_family = AF_INET;
	sock->dest_addr.sin_port = htons(PortNumber);
	/*get the server IP*/
	sock->dest_addr.sin_addr.s_addr = inet_addr(peer_ip);

	if (sock->flags & HV_SOCK_IS_TCP)
	{
		ret = connect(sock->socket, (struct sockaddr *) &sock->dest_addr, sizeof(struct sockaddr));
		if (ret == SOCKET_ERROR)
		{
			unsigned int res = LASTSOCKERROR;
			//HV_LOG(HV_LOG_ERROR, HV_LOG_NETWORK, ("[Core] Couldn't connect socket - last sock error %d\n", res));
			switch (res) {
			case EAGAIN:
				return HV_IP_SOCK_WOULD_BLOCK;

			case EISCONN:
				return HV_OK;
			case ENOTCONN:
				return HV_IP_CONNECTION_FAILURE;
			case ECONNRESET:
				return HV_IP_CONNECTION_FAILURE;
			case EMSGSIZE:
				return HV_IP_CONNECTION_FAILURE;
			case ECONNABORTED:
				return HV_IP_CONNECTION_FAILURE;
			case ENETDOWN:
				return HV_IP_CONNECTION_FAILURE;
			default:
				return HV_IP_CONNECTION_FAILURE;
			}
		}
	}

	return HV_OK;
}

HV_Err hv_sk_send(HV_Socket *sock, const char *buffer, unsigned int length)
{
	unsigned int count;
	int res;

	int ready;
	struct timeval timeout;
	fd_set Group;


	//the socket must be bound or connected
	if (!sock || !sock->socket)
		return HV_BAD_PARAM;


	//can we write?
	FD_ZERO(&Group);
	FD_SET(sock->socket, &Group);
	timeout.tv_sec = 0;
	timeout.tv_usec = SOCK_MICROSEC_WAIT;

	//TODO CHECK IF THIS IS CORRECT
	ready = select((int) sock->socket+1, 0, &Group, 0, &timeout);
	if (ready == SOCKET_ERROR)
	{
		switch (LASTSOCKERROR)
		{
		case EAGAIN:
			return HV_IP_SOCK_WOULD_BLOCK;
		default:
			return HV_IP_NETWORK_FAILURE;
		}
	}
	//should never happen (to check: is writeability is guaranteed for not-connected sockets)
	if (!ready || !FD_ISSET(sock->socket, &Group))
	{
		return HV_IP_NETWORK_EMPTY;
	}


	//direct writing
	count = 0;
	while (count < length)
	{
		if (sock->flags & HV_SOCK_HAS_PEER)
		{
			res = sendto(sock->socket, (char *) buffer+count,  length - count, 0, (struct sockaddr *) &sock->dest_addr, sock->dest_addr_len);
		}
		else
		{
			res = send(sock->socket, (char *) buffer+count, length - count, 0);
		}
		if (res == SOCKET_ERROR)
		{
			switch (res = LASTSOCKERROR)
			{
			case EAGAIN:
				return HV_IP_SOCK_WOULD_BLOCK;

			case ENOTCONN:
			case ECONNRESET:
				return HV_IP_CONNECTION_CLOSED;

			default:
				return HV_IP_NETWORK_FAILURE;
			}
		}
		count += res;
	}
	return HV_OK;
}

HV_Err hv_sk_receive(HV_Socket *sock, char *buffer, unsigned int length, unsigned int startFrom, unsigned int *BytesRead)
{
	int res;

	int ready;
	struct timeval timeout;
	fd_set Group;


	*BytesRead = 0;
	if (!sock->socket)
		return HV_BAD_PARAM;
	if (startFrom >= length)
		return HV_IO_ERR;


	//can we read?
	FD_ZERO(&Group);
	FD_SET(sock->socket, &Group);
	timeout.tv_sec = SOCK_SEC_WAIT;
	timeout.tv_usec = SOCK_MICROSEC_WAIT;

	res = 0;
	//TODO - check if this is correct
	ready = select((int) sock->socket+1, &Group, 0, 0, &timeout);
	if (ready == SOCKET_ERROR)
	{
		switch (LASTSOCKERROR)
		{
		case EBADF:
			//HV_LOG(HV_LOG_ERROR, HV_LOG_NETWORK, ("[socket] cannot select, BAD descriptor\n"));
			return HV_IP_CONNECTION_CLOSED;
		case EAGAIN:
			return HV_IP_SOCK_WOULD_BLOCK;
		case EINTR:
			/* Interrupted system call, not really important... */
			//HV_LOG(HV_LOG_ERROR, HV_LOG_NETWORK, ("[socket] network is lost\n"));
			return HV_IP_NETWORK_EMPTY;
		default:
			//HV_LOG(HV_LOG_ERROR, HV_LOG_NETWORK, ("[socket] cannot select (error %d)\n", LASTSOCKERROR));
			return HV_IP_NETWORK_FAILURE;
		}
	}
	if (!ready || !FD_ISSET(sock->socket, &Group))
	{
		//HV_LOG(HV_LOG_DEBUG, HV_LOG_NETWORK, ("[socket] nothing to be read - ready %d\n", ready));
		return HV_IP_NETWORK_EMPTY;
	}

	if (sock->flags & HV_SOCK_HAS_PEER)
		res = recvfrom(sock->socket, (char *) buffer + startFrom, length - startFrom, 0, (struct sockaddr *)&sock->dest_addr, &sock->dest_addr_len);
	else {
		res = recv(sock->socket, (char *) buffer + startFrom, length - startFrom, 0);
		if (res == 0)
			return HV_IP_CONNECTION_CLOSED;
	}

	if (res == SOCKET_ERROR)
	{
		res = LASTSOCKERROR;
		//HV_LOG(HV_LOG_ERROR, HV_LOG_NETWORK, ("[socket] error reading - socket error %d\n",  res));
		switch (res) {
		case EAGAIN:
			return HV_IP_SOCK_WOULD_BLOCK;

		case EMSGSIZE:
			return HV_OUT_OF_MEM;
		case ENOTCONN:
		case ECONNRESET:
		case ECONNABORTED:
			return HV_IP_CONNECTION_CLOSED;

		default:
			return HV_IP_NETWORK_FAILURE;
		}
	}
	if (!res) return HV_IP_NETWORK_EMPTY;
	*BytesRead = res;
	return HV_OK;
}

int hv_sk_get_handle(HV_Socket *sock)
{
	return (int) sock->socket;
}

