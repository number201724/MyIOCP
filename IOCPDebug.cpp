//#include "IOCPCommon.h"
//#include "IOCPBuffer.h"
//#include "IOCPBufferWriter.h"
//#include "IOCPBufferReader.h"
//#include "BaseIOCPServer.h"
//#include "PackageIOCP.h"
//#include "IOCPBuffer.h"
//#include "PackageIOCPClient.h"
//#include "TextIOCPServer.h"
//#include "TextIOCPClient.h"
//
//
//
//class MyF:public CTextIOCPClient
//{
//public:
//	void NotifyReceivedStringBuffer(const char* lpszText)
//	{
//		fprintf(stderr,lpszText);
//	}
//};
//
//int main(int argc,char** argv)
//{
//	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
//
//
//	CTextIOCPServer BaseServer;
//
//	BaseServer.Startup(20000,1,1000);
//
//	CTextIOCPClient BaseClient;
//
//	BaseClient.Connect("localhost",20000);
//
//
//	//Sleep(1000);
//	//BaseServer.Shutdown();
//	//CTextClient myClient;
//
//	//if(myClient.Connect("127.0.0.1",20000) == FALSE){
//	//	fprintf(stderr,"connect failed");
//	//	exit(0);
//	//}
//
//
//	//myClient.Send("Hello");
//
//	while(true){
//		getchar();
//		BaseClient.Send("hello1111");
//	}
//
//
//	return 0;
//}