#include "SerialPort.h"  
#include "global_const.h"
#include <Windows.h>
#include <iostream>  
#include "temExperiment.h" 
#include <time.h>


#include "visaUtils.h"

//typedef char * _TCHAR;
extern char mydatedir[100];
static char cmdbuf[100];
int main(int argc, char* argv[])  
{  
	time_t t = time(0);
	tm tim = *localtime(&t);
#ifdef AutoTest
	sprintf(mydatedir, "%d-%d-%d-AutoTest", 1900 + tim.tm_year, 1 + tim.tm_mon, tim.tm_mday);
#endif
#ifdef ManualTest
	sprintf(mydatedir, "%d-%d-%d-ManualTest", 1900 + tim.tm_year, 1 + tim.tm_mon, tim.tm_mday);
#endif
#ifdef Seriously
	sprintf(mydatedir, "%d-%d-%d-Seriously", 1900 + tim.tm_year, 1 + tim.tm_mon, tim.tm_mday);
#endif
	sprintf(cmdbuf, "%s%s", "mkdir ", mydatedir);
	system(cmdbuf);
	//sprintf(cmdbuf, "%s\\tem %d meas %d.txt", mydatedir, 1, 1);
	//FILE *fp = fopen(cmdbuf, "w");
	//fprintf(fp, "%s\n", "been here");
	//fclose(fp);
	//return 0;
	HANDLE  thtable[10] = {0};
	int thCnt = 0;
	hsemDataReady = CreateSemaphore(NULL, 0, 1, NULL);//
	hsemMeasDone = CreateSemaphore(NULL, 0, 1, NULL);//
	hRotate = CreateSemaphore(NULL, 0, 1, NULL);//
	initGpib();


	//unsigned int WINAPI CardInformer_test();
	//CardInformer_test();

	//return 0;



//#ifndef Use_Two_Port
	//hsemNewTemSensed = CreateSemaphore(NULL, 0, 1, NULL);/////half auto mode, does not need this
//#endif

//#ifdef _simulation
	//hsemMoreMeasure = CreateSemaphore(NULL, 0, 1, NULL);///////then dont define simulation
//#endif

	//定义各端口
	CSerialPort portCardInformer; 
	CSerialPort portMcuComm;
	CSerialPort portListener;

	//portCardInformer.InitPort(5);
	//thtable[thCnt++] = portCardInformer.OpenThread(testWave);
	//WaitForMultipleObjects(thCnt, thtable, true, INFINITE);
	//return 0;

//初始化各端口，同时打开对应线程
	//card informer
	if (!portCardInformer.InitPort(2))  {  
		std::cout << "initPort portCardInformer fail !" << std::endl;  
	}  
	else {  
		std::cout << "initPort portCardInformer success !" << std::endl;  
	}
	if (!(thtable[thCnt++] = portCardInformer.OpenThread(CardInformer))){ 
		std::cout << "Open CardInformer Thread fail !" << std::endl;  
	}  
	else {  
		std::cout << "Open CardInformer Thread success !" << std::endl;  
	}

	//tem listner
	if (!portListener.InitPort(1))  {  
		std::cout << "initPort fail !" << std::endl;  
	}  
	else {  
		std::cout << "initPort success !" << std::endl;  
	}  
	if (!(thtable[thCnt++] = portListener.OpenThread(TemOvenListener))){  
		std::cout << "Open TemOvenListen Thread fail !" << std::endl;  
	}  
	else {  
		std::cout << "Open TemOvenListen Thread success !" << std::endl;  
	}
// central controller
	if (!portMcuComm.InitPort(5))  {  
		std::cout << "initPort portMcuComm fail !" << std::endl;  
	}  
	else {  
		std::cout << "initPort portMcuComm success !" << std::endl;  
	}  
	if (!(thtable[thCnt++] = portMcuComm.OpenThread(McuComm))){  
		std::cout << "Open McuComm Thread fail !" << std::endl;  
	}  
	else{  
		std::cout << "Open McuComm Thread success !" << std::endl;  
	}


//等待线程结束,然后结束整个程序
	WaitForMultipleObjects(thCnt, thtable, true, INFINITE);

	CloseHandle(hsemDataReady);
	CloseHandle(hsemMeasDone);
	CloseHandle(hRotate);
	releaseGpib();
	//system("notify.wav");
	system("temByMcu.txt");
	system("temByTemListener.txt");
	system("temByTemOven.txt");
	if(fpAll){
		fclose(fpAll);
	}
    return 0;  
} 