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
	INIT    = 0,      // 初始状态 
	SENDING = 1,      // 发送中
};

//enum TcpSessionState
enum TcpSockState
{
	CLOSED     = 0,    // 关闭
	CONNECTING = 1,    // 连接中
	CONNECTED  = 2,    // 已连接
	TESTING    = 3,    // 验证中
	TESTED     = 4,    // 验证成功
	CLOSING    = 5,    // 关闭中
	FAILED     = 6     // 失败
};

enum
{
	PACK_RECVBUFF_LEN = (1024 * 8),    // 8K

	PACK_HEAD_LEN     = 2,             // 2 bytes
	PACK_MAX_SEND_LEN = (1024 * 63),   // 发送网络报包最大限制 63K
	PACK_MAX_RECV_LEN = (1024 * 63),   // 接收网络报包最大限制 63K
};

NamespaceEnd

#endif LIBIPC_IPC_CONFIG_H_
