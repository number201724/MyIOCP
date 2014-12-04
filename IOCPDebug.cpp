#include "IOCPCommon.h"
#include "IOCPBuffer.h"

#include "BaseIOCPServer.h"
#include "TextIOCPServer.h"



void DumpBuffer(BYTE* buffer, int length){
	for (int i = 0; i < length; i++){
		fprintf(stderr, "%02X ", buffer[i]);
	}
	fprintf(stderr, "\n");
}

int main(int argc,char** argv)
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	CTextIOCPServer BaseServer;

	BaseServer.Startup(20000,1,1000);
	Sleep(5000000);
	BaseServer.Shutdown();

	//

	

	////_CrtDumpMemoryLeaks();

	return 0;
}