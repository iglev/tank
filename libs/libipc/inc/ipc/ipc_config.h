#ifndef LIBIPC_IPC_CONFIG_H_
#define LIBIPC_IPC_CONFIG_H_

#include "base/typedef.h"
#include "string"

#ifdef _MSC_VER
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601
#endif
#endif

#define IPC_TCP_SRV_IDENT		"tcp_srv"
#define IPC_TCP_SESSION_IDENT	"tcp_session"
#define IPC_TCP_CLT_IDENT		"tcp_clt"

NamespaceStart

enum TcpSendState
{
	INIT    = 0,      // ��ʼ״̬ 
	SENDING = 1,      // ������
};

//enum TcpSessionState
enum TcpSockState
{
	CLOSED     = 0,    // �ر�
	CONNECTING = 1,    // ������
	CONNECTED  = 2,    // ������
	TESTING    = 3,    // ��֤��
	TESTED     = 4,    // ��֤�ɹ�
	CLOSING    = 5,    // �ر���
	FAILED     = 6     // ʧ��
};

enum
{
	PACK_RECVBUFF_LEN = (1024 * 8),    // 8K

	PACK_HEAD_LEN     = 2,             // 2 bytes
	PACK_MAX_SEND_LEN = (1024 * 63),   // �������籨��������� 63K
	PACK_MAX_RECV_LEN = (1024 * 63),   // �������籨��������� 63K
};

NamespaceEnd

#endif LIBIPC_IPC_CONFIG_H_
