

#include <WinSock2.h>
#include <Windows.h>
#include <MSWSock.h>
#include <map>
#include <queue>
#include <stack>
#include <set>
#include <list>
#include <string>
#include <assert.h>
#include <process.h>

#include "IOCPMutex.h"
#include "IOCPMemPool.h"
#include "IOCPQueue.h"


#include "IOCPMemoryOverride.h"
//IOCPBuffer初始数据长度
#define IOCP_BUFFER_ALLOC_SIZE 0x400

//IOCP投递到内核的内存大小（被系统锁定）
#define IOCP_SWAP_BUFFER_SIZE 0x1000

//对齐内存
#define IOCP_BUFFER_ALIGN_MEMORY(a, b) (a + (b - ((a % b) ? (a % b) : b)))

#define IOCP_MAX_PACKETS_SIZE 0x1000000

#define _FILE_AND_LINE_ __FILE__,__LINE__



void __inline DumpBuffer(BYTE* buffer, int length){
	for (int i = 0; i < length; i++){
		fprintf(stderr, "%02X ", buffer[i]);
	}
	fprintf(stderr, "\n");
}