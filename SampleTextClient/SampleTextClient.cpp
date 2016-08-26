// SampleTextClient.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "MyIOCP.h"
#include <process.h>

MyIOCP myiocp;




unsigned int __stdcall worker_thread_test(void*param){
	MyIOCP* instance = new MyIOCP();

	while(true){
		if(instance->m_bIsConnected == FALSE){
			instance->Connect("127.0.0.1",20000);
		}
		Sleep(10000);
	}
	return 0;
}
int _tmain(int argc, _TCHAR* argv[])
{
	for(int i=0;i<100;i++){
		_beginthreadex(NULL,0,worker_thread_test,NULL,0,NULL);
	}


	while(true){
	Sleep(10000);
	}


	/*myiocp.Connect("localhost",20000);



	while(true){
		Sleep(1000);
	}*/
	return 0;
}

