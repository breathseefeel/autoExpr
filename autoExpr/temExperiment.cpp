#include <process.h> 
#include <Windows.h>
#include <iostream>
#include <fstream>
#include "SerialPort.h"
#include "temExperiment.h"
#include "prepareDataPc.h"
#include "global_const.h"


#include "visaUtils.h"

using std::ofstream;
using std::cout;
using std::endl;

HANDLE hsemDataReady, hsemMeasDone, hsemNewTemSensed, hsemMoreMeasure, hRotate;
int dataFileID = 0;
bool enough_of_sen = false;
bool expecting_man = false;
unsigned int ExitFlag1 = 0;//没有更多需要测量的温度点了
unsigned int ExitFlag = 0;//最后温度点已经测量完成了——没有更多的测量需求了

const unsigned int WAIT_AFTER_RXCHAR = 1000;
#define  WAIT_BETWEEN_SUCCESIVE_SENDING 1000
#define OutOfRangeForComparison 4000
//defined for getState() and randomInqTem() and TemOvenListener()
#define InqInteval (0 + WAIT_AFTER_RXCHAR / 10 * 2)
#define stableTimeSpan (1000 * 5)
#define TimeForTemToSettle (stableTimeSpan * 0.1)
#define stepHight 20
#define TimeOfRising (1000 * 1)
#define InitTem 2000
#define TemDeltaOneTime (stepHight / (TimeOfRising / InqInteval))
#define TemOvenListenerWaitTillTemSensorInOvenReady 2000

#ifdef Use_Two_Port
	bool increasing = false;
	int curtem = InitTem;
	int curStepLeft;
	bool switchToMcu = false;
	CRITICAL_SECTION switcher;
#endif

#ifdef _simSensorInOven
unsigned int WINAPI TemSensorInOven(void *pParam)
{
	ofstream temgenerated("temByTemOven.txt");
	CSerialPort *pSerialPort = reinterpret_cast<CSerialPort *>(pParam);
	int oddeven = 0, cnt = TemCnt - 1;
	int stableCnt = stableTimeSpan / InqInteval;
	DWORD instanceflag;
	int nbytegot, curtem = InitTem;
	char buf[20], tembuf[5];
	SetCommMask(pSerialPort ->m_hComm, EV_RXCHAR);

	int i = 0;
	while (i < stableCnt)
	{
		WaitCommEvent(pSerialPort ->m_hComm, &instanceflag, NULL);//read and store
		Sleep(WAIT_AFTER_RXCHAR / 10);
		receive(pSerialPort, buf, nbytegot);
		_itoa((curtem/* = curtem + rand()%3 - 1*/), tembuf, 10);
		temgenerated << tembuf << endl;
		send(pSerialPort, (unsigned char *)tembuf, strlen(tembuf));
		++i;
		WaitForSingleObject(hsemMoreMeasure, INFINITE);
	}

	while(cnt){
		cout << "TemSensorInoven: tem is changing!" << endl;

		for(i = 0; i < TimeOfRising / InqInteval; ++i){
			WaitCommEvent(pSerialPort ->m_hComm, &instanceflag, NULL);//read and store
			Sleep(WAIT_AFTER_RXCHAR / 10);
			receive(pSerialPort, buf, nbytegot);
			_itoa((curtem = curtem + TemDeltaOneTime), tembuf, 10);
			temgenerated << tembuf << endl;
			send(pSerialPort, (unsigned char *)tembuf, strlen(tembuf));
			WaitForSingleObject(hsemMoreMeasure, INFINITE);
			if(ExitFlag1){
				break;
			}
		}

		//if(cnt == 1){
		//EnterCriticalSection(&crtcCout);
		//ExitFlag0 = 1;//never referenced
		//LeaveCriticalSection(&crtcCout);
		//}

		i = 0;
		while (i < stableCnt)//very possible that TemSensorInOven stucking here stops McuComm from quitting normally
		{
			WaitCommEvent(pSerialPort ->m_hComm, &instanceflag, NULL);//read and store
			Sleep(WAIT_AFTER_RXCHAR / 10);
			receive(pSerialPort, buf, nbytegot);
			_itoa((curtem/* = curtem + rand()%3 - 1*/), tembuf, 10);
			temgenerated << tembuf << endl;
			send(pSerialPort, (unsigned char *)tembuf, strlen(tembuf));

			WaitForSingleObject(hsemMoreMeasure, INFINITE);//作用：listner可能不需要更多温度信号了，所以等待它设置ExitFlag1，然后本线程退出

			if(ExitFlag1){
				//assert(cnt == 1);
				break;
			}

			++i;
		}
		--cnt;

	}

	cout << "\nTemSensorInOven: i'm done!\n" << endl;
	temgenerated.close();
	return 0;
}
#endif

#ifdef _simSensorInOven
unsigned int WINAPI TemOvenListener(void *pParam)//replace by randomInqTem()
{

	Sleep(TemOvenListenerWaitTillTemSensorInOvenReady);// for real experiment, just let mcu runs first

	unsigned curstate;
	int *temDeltaQueue = new int[qMaxLen];
	int i = 0, counter = 0, qIndex = 0, lastState = NOTCHANGING, cnt = TemCnt - 1;
	for (; i < qMaxLen; ++i)
	{
		temDeltaQueue[i] = 0;
	}

	CSerialPort *pSerialPort = reinterpret_cast<CSerialPort*>(pParam);
	DWORD instanceflag = 0;
	char tembuf[8];
	int nbytegot;
	ofstream temfile("temByTemListener.txt");
	 
	SetCommMask(pSerialPort ->m_hComm, EV_RXCHAR);
	while (1)
	{ 
		send(pSerialPort, (unsigned char *)"ask4tem", strlen((const char *)"ask4tem"));

		WaitCommEvent(pSerialPort ->m_hComm, &instanceflag, NULL);
		Sleep(WAIT_AFTER_RXCHAR / 10);
		receive(pSerialPort, tembuf, nbytegot);

		temfile << ++counter << "\t" << tembuf << endl;
		temDeltaQueue[qIndex++] = atoi(tembuf);
		if(qIndex == qMaxLen){
			qIndex = 0;
		}
		curstate = getTemState(temDeltaQueue, qIndex);

		if( curstate == BEGINCHANGING){
#ifdef _de_bug
			cout << "TemOvenListen:: BEGINCHANGING sensed!" << endl;
#endif
			if(lastState == NOTCHANGING){
				temfile << ++counter << "\t" << OutOfRangeForComparison << endl;  //多插入一个毛刺信号，便于观察温度变化时候能否检测到
			}
			
		}
		else if(curstate == ISCHANGING){
#ifdef _de_bug
			cout << "TemOvenListen:: ISCHANGING sensed!" << endl;
#endif
			if(lastState == BEGINCHANGING){
				temfile << ++counter << "\t" << OutOfRangeForComparison << endl;
			}
			

		}
		else if(curstate == ENDCHANGING){
#ifdef _de_bug
			cout << "TemOvenListen:: ENDCHANGING sensed!" << endl;
#endif
			if(lastState == ISCHANGING){
				temfile << ++counter << "\t" << OutOfRangeForComparison << endl;
			}
			
		}
		else{
#ifdef _de_bug
			cout << "TemOvenListen:: NOTCHANGING sensed!" << endl;
#endif
			if (lastState == ENDCHANGING)
			{
				cout << "TemOvenListen:: tem changed detected!" << endl;
				temfile << ++counter << "\t" << OutOfRangeForComparison << endl;
				Sleep((unsigned)TimeForTemToSettle);
				--cnt;				
				ReleaseSemaphore(hsemNewTemSensed, 1, 0);//do sth
				if (!cnt)
				{
					EnterCriticalSection(&crtc);
					ExitFlag1 = 1;//表示没有更多需要测量的温度点了
					LeaveCriticalSection(&crtc);
					cout << "\nTemOvenListen: i'm done!\n" << endl;					
				}
				
				if (!cnt)
				{
					ReleaseSemaphore(hsemMoreMeasure, 1, 0);//do sth
					break;
				}
			}
		}

		ReleaseSemaphore(hsemMoreMeasure, 1, 0);//do sth

		lastState = curstate;
		Sleep(InqInteval + 1 - WAIT_AFTER_RXCHAR / 10 * 2);
	}

	 temfile.close();
	 delete[] temDeltaQueue;
	 return 0;
}
#else
#ifdef ManualTest
unsigned int WINAPI TemOvenListener(void *pParam)//replace by randomInqTem()
{
	return 0;
}
#endif

#ifdef AutoTest
unsigned int WINAPI TemOvenListener(void *pParam)//replace by randomInqTem()
{

	Sleep(TemOvenListenerWaitTillTemSensorInOvenReady);// for real experiment, just let mcu runs first
	unsigned char addr[10] = "a99z";
	unsigned char *tem = reinterpret_cast<unsigned char *>("temz");
	unsigned curstate;
	int *temDeltaQueue = new int[qMaxLen];
	int i = 0, counter = 0, qIndex = 0, lastState = NOTCHANGING, cnt = TemCnt - 1;
	for (; i < qMaxLen; ++i)
	{
		temDeltaQueue[i] = 0;
	}

	CSerialPort *pSerialPort = reinterpret_cast<CSerialPort*>(pParam);
	DWORD instanceflag = 0;
	char tembuf[100];
	int nbytegot;


	while(true){

		std::cin >> tembuf;
		std::cout << "\"quit\" to quit the 3rd thread\n\"continue\" to stop waiting temlistner\n\"rotate\" to read data" << endl;
		if(0 == strcmp(tembuf, "quit"))
			return 0;
		else if(0 ==strcmp(tembuf, "continue")){
			enough_of_sen = true;
		}
		else if(0 == strcmp(tembuf, "rotate")){
			if(expecting_man)
				ReleaseSemaphore(hRotate, 1, 0);
		}
	}

	char *pname = tembuf;
	sprintf(pname, "%s\\temByTemListener.txt", mydatedir);
	ofstream temfile(pname);




	SetCommMask(pSerialPort ->m_hComm, EV_RXCHAR);
	while (1)
	{ 
		send(pSerialPort, addr, strlen((const char *)addr));
		WaitCommEvent(pSerialPort ->m_hComm, &instanceflag, NULL);
		Sleep(WAIT_AFTER_RXCHAR / 10);
		receive(pSerialPort, tembuf, nbytegot);

		send(pSerialPort, tem, strlen((char*)tem));//发送读取温度指令	
		WaitCommEvent(pSerialPort ->m_hComm, &instanceflag, NULL);
		Sleep(WAIT_AFTER_RXCHAR / 10);
		receive(pSerialPort, tembuf, nbytegot);
		//here, the mcu must know to pause awhile between two succesive sending
		WaitCommEvent(pSerialPort ->m_hComm, &instanceflag, NULL);//read and store
		Sleep(WAIT_AFTER_RXCHAR / 10);
		receiveNum(pSerialPort, tembuf, nbytegot);
		send(pSerialPort, (unsigned char *)"McuComm:: tem gotz", strlen((char*)"McuComm:: tem gotz"));

		temfile << ++counter << "\t" << tembuf << endl;
		temDeltaQueue[qIndex++] = atoi(tembuf);
		if(qIndex == qMaxLen){
			qIndex = 0;
		}
		curstate = getTemState(temDeltaQueue, qIndex);

		if( curstate == BEGINCHANGING){
#ifdef _de_bug
			cout << "TemOvenListen:: BEGINCHANGING sensed!" << endl;
#endif
			if(lastState == NOTCHANGING){
				temfile << ++counter << "\t" << OutOfRangeForComparison << endl;  //多插入一个毛刺信号，便于观察温度变化时候能否检测到
			}

		}
		else if(curstate == ISCHANGING){
#ifdef _de_bug
			cout << "TemOvenListen:: ISCHANGING sensed!" << endl;
#endif
			if(lastState == BEGINCHANGING){
				temfile << ++counter << "\t" << OutOfRangeForComparison << endl;
			}


		}
		else if(curstate == ENDCHANGING){
#ifdef _de_bug
			cout << "TemOvenListen:: ENDCHANGING sensed!" << endl;
#endif
			if(lastState == ISCHANGING){
				temfile << ++counter << "\t" << OutOfRangeForComparison << endl;
			}

		}
		else{
#ifdef _de_bug
			cout << "TemOvenListen:: NOTCHANGING sensed!" << endl;
#endif
			if (lastState == ENDCHANGING)
			{
				cout << "TemOvenListen:: tem changed detected!" << endl;
				temfile << ++counter << "\t" << OutOfRangeForComparison << endl;
				Sleep((unsigned)TimeForTemToSettle);
				--cnt;				

				if (!cnt)
				{
					cout << "\nTemOvenListen: i'm done!\n" << endl;		
					break;
				}

			}
		}

		lastState = curstate;
		Sleep(InqInteval + 1 - WAIT_AFTER_RXCHAR / 10 * 2);
	}

	temfile.close();
	delete[] temDeltaQueue;
	return 0;
}
#endif

#ifdef Seriously
unsigned int WINAPI TemOvenListener(void *pParam)//replace by randomInqTem()
{

	Sleep(TemOvenListenerWaitTillTemSensorInOvenReady);// for real experiment, just let mcu runs first
	unsigned char addr[10] = "a99z";
	unsigned char *tem = reinterpret_cast<unsigned char *>("temz");
	unsigned curstate;
	int *temDeltaQueue = new int[qMaxLen];
	int i = 0, counter = 0, qIndex = 0, lastState = NOTCHANGING, cnt = TemCnt - 1;
	for (; i < qMaxLen; ++i)
	{
		temDeltaQueue[i] = 0;
	}

	CSerialPort *pSerialPort = reinterpret_cast<CSerialPort*>(pParam);
	DWORD instanceflag = 0;
	char tembuf[100];
	int nbytegot;


	while(true){

		std::cin >> tembuf;
		std::cout << "\"quit\" to quit the 3rd thread\n\"continue\" to stop waiting temlistner\n\"rotate\" to read data" << endl;
		if(0 == strcmp(tembuf, "quit"))
			return 0;
		else if(0 ==strcmp(tembuf, "continue")){
			enough_of_sen = true;
		}
		else if(0 == strcmp(tembuf, "rotate")){
			if(expecting_man)
				ReleaseSemaphore(hRotate, 1, 0);
		}
	}

	char *pname = tembuf;
	sprintf(pname, "%s\\temByTemListener.txt", mydatedir);
	ofstream temfile(pname);




	SetCommMask(pSerialPort ->m_hComm, EV_RXCHAR);
	while (1)
	{ 
		send(pSerialPort, addr, strlen((const char *)addr));
		WaitCommEvent(pSerialPort ->m_hComm, &instanceflag, NULL);
		Sleep(WAIT_AFTER_RXCHAR / 10);
		receive(pSerialPort, tembuf, nbytegot);

		send(pSerialPort, tem, strlen((char*)tem));//发送读取温度指令	
		WaitCommEvent(pSerialPort ->m_hComm, &instanceflag, NULL);
		Sleep(WAIT_AFTER_RXCHAR / 10);
		receive(pSerialPort, tembuf, nbytegot);
		//here, the mcu must know to pause awhile between two succesive sending
		WaitCommEvent(pSerialPort ->m_hComm, &instanceflag, NULL);//read and store
		Sleep(WAIT_AFTER_RXCHAR / 10);
		receiveNum(pSerialPort, tembuf, nbytegot);
		send(pSerialPort, (unsigned char *)"McuComm:: tem gotz", strlen((char*)"McuComm:: tem gotz"));

		temfile << ++counter << "\t" << tembuf << endl;
		temDeltaQueue[qIndex++] = atoi(tembuf);
		if(qIndex == qMaxLen){
			qIndex = 0;
		}
		curstate = getTemState(temDeltaQueue, qIndex);

		if( curstate == BEGINCHANGING){
#ifdef _de_bug
			cout << "TemOvenListen:: BEGINCHANGING sensed!" << endl;
#endif
			if(lastState == NOTCHANGING){
				temfile << ++counter << "\t" << OutOfRangeForComparison << endl;  //多插入一个毛刺信号，便于观察温度变化时候能否检测到
			}

		}
		else if(curstate == ISCHANGING){
#ifdef _de_bug
			cout << "TemOvenListen:: ISCHANGING sensed!" << endl;
#endif
			if(lastState == BEGINCHANGING){
				temfile << ++counter << "\t" << OutOfRangeForComparison << endl;
			}


		}
		else if(curstate == ENDCHANGING){
#ifdef _de_bug
			cout << "TemOvenListen:: ENDCHANGING sensed!" << endl;
#endif
			if(lastState == ISCHANGING){
				temfile << ++counter << "\t" << OutOfRangeForComparison << endl;
			}

		}
		else{
#ifdef _de_bug
			cout << "TemOvenListen:: NOTCHANGING sensed!" << endl;
#endif
			if (lastState == ENDCHANGING)
			{
				cout << "TemOvenListen:: tem changed detected!" << endl;
				temfile << ++counter << "\t" << OutOfRangeForComparison << endl;
				Sleep((unsigned)TimeForTemToSettle);
				--cnt;				

				if (!cnt)
				{
					cout << "\nTemOvenListen: i'm done!\n" << endl;		
					break;
				}

			}
		}

		lastState = curstate;
		Sleep(InqInteval + 1 - WAIT_AFTER_RXCHAR / 10 * 2);
	}

	temfile.close();
	delete[] temDeltaQueue;
	return 0;
}
#endif

#endif
int temDeltaQueueLen = 3;
int upriseTemDelta = 3;
int stableTemDeltaMax = 1;

int temDeltaQueueLenLearned = 30;
int upriseTemDeltaLearned = 5;
int stableTemDeltaMaxLearned = 1;
int qMaxLen = 3;

const int BEGINCHANGING = 1;
const int ISCHANGING = 2;
const int  ENDCHANGING = 4;
const int NOTCHANGING = 8;

unsigned getTemState(int *arr, int idx)
{
	static int cnt = 0;
	if(cnt < qMaxLen){
		++cnt;
		return NOTCHANGING;
	}

#ifdef _showQueueInGetStateFun
	int indx = idx;
	while(1){
		cout << arr[indx] << "\t";
		++indx;
		if(indx == qMaxLen){
			indx = 0;
		}
		if(indx == idx){
			break;
		}

	}cout << endl;
#endif

	int stp = idx;//this is the latest data's index
	int tmp = arr[idx++];//this is the latest data
	if(idx == qMaxLen){
		idx = 0;
	}
	if(arr[idx] - tmp > upriseTemDelta){
		while (arr[idx] - tmp > upriseTemDelta)
		{
			tmp = arr[idx++];
			if (idx == qMaxLen)
			{
				idx = 0;
			}
			if(idx == stp){
				break;
			}
		}
		if(idx == stp){
			return ISCHANGING;
		}
		else{
			return ENDCHANGING;
		}
	}
	else{
		while (arr[idx] - tmp < stableTemDeltaMax && arr[idx] - tmp > -stableTemDeltaMax)
		{
			tmp = arr[idx++];

			if (idx == qMaxLen)
			{
				idx = 0;
			}
			if(idx == stp){
				break;
			}
		}
		if(idx == stp){
			return NOTCHANGING;
		}
		else{
			return	BEGINCHANGING;
		}

	}

}

#ifdef _simCard
unsigned int WINAPI Card(void *pParam)
{
	CSerialPort *hcomm = reinterpret_cast<CSerialPort *>(pParam);
	DWORD tmp, oldmask;
	int  cnt = TemCnt * RoundPerTem;
	//GetCommMask(hcomm ->m_hComm, &oldmask);
	SetCommMask(hcomm ->m_hComm, EV_CTS);	 

	while(1){
		WaitCommEvent(hcomm ->m_hComm, &tmp, 0);
		cout << "Card: Yes, I'll collect data" << endl;
		--cnt;
		if(!cnt){
			cout << "\nCard: i'm done!\n" << endl;
			return 0;
		}
	}
}
#endif
unsigned int WINAPI CardInformer_test()
{
	const unsigned int WAIT_BEFORE_COLLECT = 1 * 1000;

	unsigned int cnt = TemCnt * RoundPerTem;
	int index = 0;
	int indexTem = 1;
	int indexMeas = 0;
	char filename[100];

	while (1)
	{ 
		getchar();
		indexMeas++;
		if(indexMeas == RoundPerTem + 1){
			if(indexTem == TemCnt){
				return 0;
			}
			indexMeas = 1;
			indexTem++;
		}
		sprintf(filename, "fileID %d tem %d meas %d", dataFileID++, indexTem, indexMeas);
		FILE *fp = fopen(filename, "w");

		scan_meas_cnt(pvi, fp);
		fclose(fp);


	} 

	cout << "quiting!" << endl;
	return 0;
}
#ifdef _use_gpib
#ifdef ManualTest
unsigned int WINAPI CardInformer(void *pParam)
{
	
	return 0;
}
#endif

#ifdef AutoTest
unsigned int WINAPI CardInformer(void *pParam)
{
	const unsigned int WAIT_BEFORE_COLLECT = 1 * 1000;
	CSerialPort *pSerialPort = reinterpret_cast<CSerialPort*>(pParam);  

	unsigned int cnt = TemCnt * RoundPerTem;
	int index = 0;
	int indexTem = 1;
	int indexMeas = 0;
	char filename[100];

	while (1)
	{ 
		//getchar();
		WaitForSingleObject(hsemDataReady, INFINITE);
		cout << "CardInformer: To collect data "
			<< "in " << WAIT_BEFORE_COLLECT / 1000 << " seconds" << endl;

		Sleep(WAIT_BEFORE_COLLECT);
		//name a file fp;
		indexMeas++;
		if(indexMeas == RoundPerTem + 1){
			indexMeas = 1;
			indexTem++;
		}
		sprintf(filename, "%s\\tem %d meas %d.txt", mydatedir, indexTem, indexMeas);
		FILE *fp = fopen(filename, "w");
		fprintf(fp, "%d\n", ++dataFileID);
		writeFileInfo(fp);

		scan_meas_cnt(pvi, fp);
		fclose(fp);
		ReleaseSemaphore(hsemMeasDone, 1, 0);

		if(ExitFlag){
			break;
		}
	} 

	cout << "\nCardInformer: i'm done!\n" << endl;
	return 0;
}
#endif

#ifdef Seriously
unsigned int WINAPI CardInformer(void *pParam)
{
	const unsigned int WAIT_BEFORE_COLLECT = 1 * 1000;
	CSerialPort *pSerialPort = reinterpret_cast<CSerialPort*>(pParam);  

	unsigned int cnt = TemCnt * RoundPerTem;
	int index = 0;
	int indexTem = 1;
	int indexMeas = 0;
	char filename[100];

	while (1)
	{ 
		//getchar();
		WaitForSingleObject(hsemDataReady, INFINITE);
		cout << "CardInformer: To collect data "
			<< "in " << WAIT_BEFORE_COLLECT / 1000 << " seconds" << endl;

		Sleep(WAIT_BEFORE_COLLECT);
		//name a file fp;
		indexMeas++;
		if(indexMeas == RoundPerTem + 1){
			indexMeas = 1;
			indexTem++;
		}
		sprintf(filename, "%s\\tem %d meas %d.txt", mydatedir, indexTem, indexMeas);
		FILE *fp = fopen(filename, "w");
		fprintf(fp, "%d\n", ++dataFileID);
		writeFileInfo(fp);

		scan_meas_cnt(pvi, fp);
		fclose(fp);
		ReleaseSemaphore(hsemMeasDone, 1, 0);

		if(ExitFlag){
			break;
		}
	} 

	cout << "\nCardInformer: i'm done!\n" << endl;
	return 0;
}
#endif

#else
unsigned int WINAPI CardInformer(void *pParam)
{
	const unsigned int WAIT_BEFORE_COLLECT = 1 * 1000;
	CSerialPort *pSerialPort = reinterpret_cast<CSerialPort*>(pParam);  

	unsigned int cnt = TemCnt * RoundPerTem;
	unsigned int oddeven = 1;

	while (1)
	{ 
		WaitForSingleObject(hsemDataReady, INFINITE);
		cout << "CardInformer: Now that written is done, I'll inform the card "
			 << "in " << WAIT_BEFORE_COLLECT / 1000 << " seconds" << endl;

		Sleep(WAIT_BEFORE_COLLECT);

		cout << "CardInformer: a pulse is generated!" << endl;
		if(oddeven)
			EscapeCommFunction(pSerialPort ->m_hComm, CLRRTS);
		else
			EscapeCommFunction(pSerialPort ->m_hComm, SETRTS);

		oddeven = 1 - oddeven;	

		cout << "CardInformer: let's wait for card collecting!" << endl;
#ifndef _simulation
		//实际要用脉冲来触发
		//thus generating a positive pulse each time when informs card
		EscapeCommFunction(pSerialPort ->m_hComm, SETRTS);
		//wait for 20 seconds for card collecting
		Sleep(WAIT_WHILE_COLLECT);
#endif
		ReleaseSemaphore(hsemMeasDone, 1, 0);

		if(ExitFlag2){
			break;
		}
	} 

	cout << "\nCardInformer: i'm done!\n" << endl;
	return 0;
}
#endif

#define CMD_INTERVAL 1000
#define LETMCUGOFIRST 2000
#ifdef Use_Two_Port

unsigned int WINAPI McuCommWithTemListner(void *pParam)
{
	CSerialPort *pSerialPort = reinterpret_cast<CSerialPort*>(pParam);
	unsigned char addr[10];
	unsigned char *tem = reinterpret_cast<unsigned char *>("temz");
	unsigned char *beginwrite = reinterpret_cast<unsigned char *>("nextdataz");
	DWORD instanceflag = 0;
	ofstream temfile("temByMcu.txt");
	char tembuf[IN_BUF_SZ];
	int nbytegot, counter = 0, i = 0;


#ifdef Use_Two_Port
	unsigned curstate;
	int *temDeltaQueue = new int[qMaxLen];
	int lineNumberInFile = 0, qIndex = 0, lastState = NOTCHANGING, cnt = TemCnt - 1;
	char tembuflistner[8];
	//int nbytegot;
	ofstream temfilelistner("temByTemListener.txt");
	for (i = 0; i < qMaxLen; ++i)
	{
		temDeltaQueue[i] = InitTem;
	}
#endif


	//in real experiment, just let mcu runs first
#ifdef _simulation
	Sleep(LETMCUGOFIRST);
#endif

	SetCommMask(pSerialPort ->m_hComm, EV_RXCHAR);
	addr[0] = 'a';
	while (1)
	{
		temfile << ++counter << ":\t";
		for(i = 1; i <= NUMOFSLAVES; ++i){

			_itoa(i, (char*)(addr + 1), 10);
			strcat((char*)addr, "z");
			send(pSerialPort, addr, strlen((const char *)addr));
			WaitCommEvent(pSerialPort ->m_hComm, &instanceflag, NULL);
			Sleep(WAIT_AFTER_RXCHAR / 10);
			receive(pSerialPort, tembuf, nbytegot);

			send(pSerialPort, tem, strlen((char*)tem));//发送读取温度指令	
			WaitCommEvent(pSerialPort ->m_hComm, &instanceflag, NULL);
			Sleep(WAIT_AFTER_RXCHAR / 10);
			receive(pSerialPort, tembuf, nbytegot);
			//here, the mcu must know to pause awhile between two succesive sending
			WaitCommEvent(pSerialPort ->m_hComm, &instanceflag, NULL);//read and store
			Sleep(WAIT_AFTER_RXCHAR / 10);
			receiveNum(pSerialPort, tembuf, nbytegot);

			send(pSerialPort, (unsigned char *)"McuCommWithTemListner:: tem gotz", strlen((char*)"McuCommWithTemListner:: tem gotz"));
			Sleep(WAIT_AFTER_RXCHAR / 10);
			temfile << tembuf << "\t";
			for(int iii = 0; iii < sizeof(tembuf); ++iii){
				tembuf[iii] = 0;
			}

		}
		temfile << "\n";


		for(i = 0; i < RoundPerTem; ++i){
			getNextData();///////////////////////////////
			for(int j = 1; j <= NUMOFSLAVES; ++j){

				_itoa(j, (char*)(addr + 1), 10);
				strcat((char*)addr, "z");
				send(pSerialPort, addr, strlen((const char *)addr));
				WaitCommEvent(pSerialPort ->m_hComm, &instanceflag, NULL);
				Sleep(WAIT_AFTER_RXCHAR / 10);
				receive(pSerialPort, tembuf, nbytegot);//useless replied data must be received and ditch

				send(pSerialPort, beginwrite, strlen((char*)beginwrite));
				WaitCommEvent(pSerialPort ->m_hComm, &instanceflag, NULL);
				Sleep(WAIT_AFTER_RXCHAR / 10);
				receive(pSerialPort, tembuf, nbytegot);	//useless replied data must be received and ditch

				send(pSerialPort, (unsigned char*)strToSend2, strlen(strToSend2));//////////////////////////////////////////////////////////////
				WaitCommEvent(pSerialPort ->m_hComm, &instanceflag, NULL);
				Sleep(WAIT_AFTER_RXCHAR / 10);				
				receive(pSerialPort, tembuf, nbytegot);

				//send(pSerialPort, (unsigned char*)"McuCommWithTemListner:: reply to mcu's datawrittenz", strlen("McuCommWithTemListner:: reply to mcu's datawrittenz"));
				cout << "McuComm: data has been written into #" << j << " mcu" << endl;
			}

			cout << "McuCommWithTemListner:: data has been written into all mcus" << endl;
			cout << "McuCommWithTemListner:: Now, Cardinformer is to generate pulse!" << endl;				
			EnterCriticalSection(&crtc);
			if(ExitFlag1 && (i == RoundPerTem - 1)){//当没有更多温度点需要测量的时候作如下判断
				ExitFlag2 = 1;//表示没有更多测量需求了
			}
			LeaveCriticalSection(&crtc);

			//inform cardinformer thread to run
			ReleaseSemaphore(hsemDataReady, 1, 0);	
			WaitForSingleObject(hsemMeasDone, INFINITE);
			cout << "MucComm: one data has been written into all mcus and measure is done!" << endl;

		}
#ifdef _de_bug
		cout << "almost there!\n" << "exitflag 1 is :" << ExitFlag1 << endl;
#endif
		EnterCriticalSection(&crtc);
		if(ExitFlag1){
			cout << "\nMcuCommWithTemListner:: i'm done!\n" << endl;

		}
		LeaveCriticalSection(&crtc);

		if(ExitFlag1){
			break;
		}					
		//////////////////////////////////////////////////////////////////////
		//talk with the sensor before continue

#ifdef Use_Two_Port
		//实际呼叫地址为a99
#define specialAddr 99
		while (1)
		{ 
			//向专门的sensor发送tem命令，以获取温度
			_itoa(specialAddr, (char*)(addr + 1), 10);
			strcat((char*)addr, "z");
			send(pSerialPort, addr, strlen((const char *)addr));
			WaitCommEvent(pSerialPort ->m_hComm, &instanceflag, NULL);
			Sleep(WAIT_AFTER_RXCHAR / 10);
			receive(pSerialPort, tembuf, nbytegot);

			/*send(pSerialPort, (unsigned char *)"ask4tem", strlen((const char *)"ask4tem"));

			WaitCommEvent(pSerialPort ->m_hComm, &instanceflag, NULL);
			Sleep(WAIT_AFTER_RXCHAR / 10);
			receive(pSerialPort, tembuflistner, nbytegot);*/

			send(pSerialPort, tem, strlen((char*)tem));//发送读取温度指令	
			WaitCommEvent(pSerialPort ->m_hComm, &instanceflag, NULL);
			Sleep(WAIT_AFTER_RXCHAR / 10);
			receive(pSerialPort, tembuf, nbytegot);
			//here, the mcu must know to pause awhile between two succesive sending
			WaitCommEvent(pSerialPort ->m_hComm, &instanceflag, NULL);//read and store
			Sleep(WAIT_AFTER_RXCHAR / 10);
			receiveNum(pSerialPort, tembuf, nbytegot);

			send(pSerialPort, (unsigned char *)"McuCommWithTemListner:: tem gotz", strlen((char*)"McuCommWithTemListner:: tem gotz"));
			Sleep(WAIT_AFTER_RXCHAR / 10);
			//把数据放入队列，调用getTemState(,)获取当前状态
			//stupid error bug
			temfilelistner << ++lineNumberInFile << "\t" << tembuf << endl;

			temDeltaQueue[qIndex++] = atoi(tembuf);

			if(qIndex == qMaxLen){
				qIndex = 0;
			}
			//bug found: 
			curstate = getTemState(temDeltaQueue, qIndex);
			//根据状态值判断新温度是否到达，到达之后结束此内循环，开始和mcu交换数据
			if( curstate == BEGINCHANGING){
#ifdef _de_bug
				cout << "McuCommWithTemListner:: BEGINCHANGING sensed!" << endl;
#endif
				if(lastState == NOTCHANGING){
					temfilelistner << ++lineNumberInFile << "\t" << OutOfRangeForComparison << endl;  //多插入一个毛刺信号，便于观察温度变化时候能否检测到
				}

			}
			else if(curstate == ISCHANGING){
#ifdef _de_bug
				cout << "McuCommWithTemListner:: ISCHANGING sensed!" << endl;
#endif
				if(lastState == BEGINCHANGING){
					temfilelistner << ++lineNumberInFile << "\t" << OutOfRangeForComparison << endl;
				}


			}
			else if(curstate == ENDCHANGING){
#ifdef _de_bug
				cout << "McuCommWithTemListner:: ENDCHANGING sensed!" << endl;
#endif
				if(lastState == ISCHANGING){
					temfilelistner << ++lineNumberInFile << "\t" << OutOfRangeForComparison << endl;
				}

			}
			else{
#ifdef _de_bug
				cout << "McuCommWithTemListner:: NOTCHANGING sensed!" << endl;
#endif
				if (lastState == ENDCHANGING)
				{
					EnterCriticalSection(&switcher);
					switchToMcu = true;//告诉仿真线程可以计入Mcu模式，准备交换数据
					LeaveCriticalSection(&switcher);
					cout << "McuCommWithTemListner:: tem changed detected!" << endl;
					temfilelistner << ++lineNumberInFile << "\t" << OutOfRangeForComparison << endl;
					Sleep((unsigned)TimeForTemToSettle);
					--cnt;				
					//ReleaseSemaphore(hsemNewTemSensed, 1, 0);//do sth
					if (!cnt)
					{
						EnterCriticalSection(&crtc);
						ExitFlag1 = 1;//表示没有更多需要测量的温度点了
						LeaveCriticalSection(&crtc);
						cout << "\nMcuCommWithTemListner:: i'm done!\n" << endl;	
						temfilelistner.close();
						ReleaseSemaphore(hsemMoreMeasure, 1, 0);//do sth
						break;
					}

					ReleaseSemaphore(hsemMoreMeasure, 1, 0);//do sth
					lastState = curstate;
					Sleep(InqInteval + 1 - WAIT_AFTER_RXCHAR / 10 * 2);
					break;
				}
			}

			ReleaseSemaphore(hsemMoreMeasure, 1, 0);//do sth

			lastState = curstate;
			Sleep(InqInteval + 1 - WAIT_AFTER_RXCHAR / 10 * 2);
		}
#else
		WaitForSingleObject(hsemNewTemSensed, INFINITE);
		cout << "McuComm: tem come copied" << endl;
#endif
	}  
	temfile.close();
	return 0;
}


#elif defined Use_One_Port


#include <string>
	char filename[1000];
	char tmpBuf[1000];
	char *somedir = "somedir";
#ifdef ManualTest
#include "simuType.h"
	unsigned int WINAPI McuComm(void *pParam)
	{
		int fileIden = 0;
		CSerialPort *pSerialPort = reinterpret_cast<CSerialPort*>(pParam);
		unsigned char addr[10];
		unsigned char *tem = reinterpret_cast<unsigned char *>("temz");
		unsigned char *beginwrite = reinterpret_cast<unsigned char *>("nextdataz");
		DWORD instanceflag = 0;

		char tembuf[IN_BUF_SZ];
		int nbytegot, counter2 = 0, i = 0;
		int whenToQuit = 0;

		strcpy(filename, mydatedir);
		strcat(filename, "\\");
		strcat(filename, "temByMcu.txt");
		ofstream temfile(filename);

#ifdef _simulation
		Sleep(LETMCUGOFIRST);
#endif


		unsigned curstate;
		int *temDeltaQueue = new int[qMaxLen];
		int counter = 0, qIndex = 0, lastState = NOTCHANGING, cnt = 1;
		for (i = 0; i < qMaxLen; ++i)
		{
			temDeltaQueue[i] = 0;
		}



		SetCommMask(pSerialPort ->m_hComm, EV_RXCHAR);
		addr[0] = 'a';
		FILE *fp2;


		sprintf(strToSend2, fmt, UPPERBND0, UPPERBND1, 0, 0,0,0,0,0);

		cout << "writing default data into mcus ..." << endl;
		for(int j = 1; j <= NUMOFSLAVES; ++j){

			_itoa(j, (char*)(addr + 1), 10);
			strcat((char*)addr, "z");
			send(pSerialPort, addr, strlen((const char *)addr));
			WaitCommEvent(pSerialPort ->m_hComm, &instanceflag, NULL);
			Sleep(WAIT_AFTER_RXCHAR / 10);
			receive(pSerialPort, tembuf, nbytegot);//useless replied data must be received and ditch

			send(pSerialPort, beginwrite, strlen((char*)beginwrite));
			WaitCommEvent(pSerialPort ->m_hComm, &instanceflag, NULL);
			Sleep(WAIT_AFTER_RXCHAR / 10);
			receive(pSerialPort, tembuf, nbytegot);	//useless replied data must be received and ditch
			///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

			send(pSerialPort, (unsigned char*)strToSend2, strlen(strToSend2));//////////////////////////////////////////////////////////////
			WaitCommEvent(pSerialPort ->m_hComm, &instanceflag, NULL);
			Sleep(WAIT_AFTER_RXCHAR / 10);				
			receive(pSerialPort, tembuf, nbytegot);


			WaitCommEvent(pSerialPort ->m_hComm, &instanceflag, NULL);
			Sleep(WAIT_AFTER_RXCHAR / 10);				
			receive(pSerialPort, tembuf, nbytegot);

			send(pSerialPort, (unsigned char*)"McuComm: reply to mcu's datawrittenz", strlen("McuComm: reply to mcu's datawrittenz"));
		}
		cout << "McuComm: default data written." << endl;



		while (1)
		{
			cout << "\nmodify old data and press enter:" << endl;
			simulateType(strToSend2);
			std::cin.getline(filename, 1000);

			if(0 == strcmp(filename, "tem")){
				cout << "querying temperature ..." << endl;
				temfile << ++counter << ":\t";
				for(i = 1; i <= NUMOFSLAVES; ++i){

					_itoa(i, (char*)(addr + 1), 10);
					strcat((char*)addr, "z");
					send(pSerialPort, addr, strlen((const char *)addr));
					WaitCommEvent(pSerialPort ->m_hComm, &instanceflag, NULL);
					Sleep(WAIT_AFTER_RXCHAR / 10);
					receive(pSerialPort, tembuf, nbytegot);

					send(pSerialPort, tem, strlen((char*)tem));//发送读取温度指令	
					WaitCommEvent(pSerialPort ->m_hComm, &instanceflag, NULL);
					Sleep(WAIT_AFTER_RXCHAR / 10);
					receive(pSerialPort, tembuf, nbytegot);
					//here, the mcu must know to pause awhile between two succesive sending
					WaitCommEvent(pSerialPort ->m_hComm, &instanceflag, NULL);//read and store
					Sleep(WAIT_AFTER_RXCHAR / 10);
					receiveNum(pSerialPort, tembuf, nbytegot);
					send(pSerialPort, (unsigned char *)"McuComm:: tem gotz", strlen((char*)"McuComm:: tem gotz"));


					Sleep(WAIT_AFTER_RXCHAR / 10);
					temfile << tembuf << "\t";
					cout << tembuf << "\t";
					for(int iii = 0; iii < sizeof(tembuf); ++iii){
						tembuf[iii] = 0;
					}

				}
				temfile << "\n";
				cout << endl;
				temfile.flush();
			}
			else{
				cout << "writing data into mcus ..." << endl;
				strcpy(strToSend2, filename);
				for(int j = 1; j <= NUMOFSLAVES; ++j){

					_itoa(j, (char*)(addr + 1), 10);
					strcat((char*)addr, "z");
					send(pSerialPort, addr, strlen((const char *)addr));
					WaitCommEvent(pSerialPort ->m_hComm, &instanceflag, NULL);
					Sleep(WAIT_AFTER_RXCHAR / 10);
					receive(pSerialPort, tembuf, nbytegot);//useless replied data must be received and ditch

					send(pSerialPort, beginwrite, strlen((char*)beginwrite));
					WaitCommEvent(pSerialPort ->m_hComm, &instanceflag, NULL);
					Sleep(WAIT_AFTER_RXCHAR / 10);
					receive(pSerialPort, tembuf, nbytegot);	//useless replied data must be received and ditch
					///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

					send(pSerialPort, (unsigned char*)strToSend2, strlen(strToSend2));//////////////////////////////////////////////////////////////
					WaitCommEvent(pSerialPort ->m_hComm, &instanceflag, NULL);
					Sleep(WAIT_AFTER_RXCHAR / 10);				
					receive(pSerialPort, tembuf, nbytegot);


					WaitCommEvent(pSerialPort ->m_hComm, &instanceflag, NULL);
					Sleep(WAIT_AFTER_RXCHAR / 10);				
					receive(pSerialPort, tembuf, nbytegot);

					send(pSerialPort, (unsigned char*)"reply to mcu's datawrittenz", strlen("reply to mcu's datawrittenz"));

					cout << "data has been written into #" << j << " mcu" << endl;
				}
				sprintf(tmpBuf, "%s%c%c%s", mydatedir, '\\', '\\', filename);
				strcpy(filename, tmpBuf);
				for(int k = 0; k < strlen(filename); k++)
					if(filename[k] == ',')
						filename[k] = ' ';
				sprintf(filename+strlen(filename), "--%d.txt", ++fileIden);
				fp2 = fopen(filename, "w");
				fprintf(fp2, "%s\n\n\n", strToSend2);
				std::cout << "measuring ..." << std::endl;
				std::cout << "result: " << scan_meas_cnt_aver(pvi, fp2) << endl;
				fclose(fp2);

			}


		}

	}
#endif


#ifdef AutoTest
#include "simuType.h"
	unsigned int WINAPI McuComm(void *pParam)
	{
		CSerialPort *pSerialPort = reinterpret_cast<CSerialPort*>(pParam);
		unsigned char addr[10];
		unsigned char *tem = reinterpret_cast<unsigned char *>("temz");
		unsigned char *beginwrite = reinterpret_cast<unsigned char *>("nextdataz");
		DWORD instanceflag = 0;

		char tembuf[IN_BUF_SZ];
		int nbytegot, counter2 = 0, i = 0;
		int whenToQuit = 0;

		strcpy(filename, mydatedir);
		strcat(filename, "\\");
		strcat(filename, "temByMcu.txt");
		ofstream temfile(filename);
		cout << "auto test\n" << endl;
#ifdef _simulation
		Sleep(LETMCUGOFIRST);
#endif


		unsigned curstate;
		int *temDeltaQueue = new int[qMaxLen];
		int counter = 0, qIndex = 0, lastState = NOTCHANGING, cnt = 1;
		for (i = 0; i < qMaxLen; ++i)
		{
			temDeltaQueue[i] = 0;
		}



		SetCommMask(pSerialPort ->m_hComm, EV_RXCHAR);
		addr[0] = 'a';
		while (1)
		{

			FILE *fp2;
			for(int i = 0; i < RoundPerTem; ++i){
				getNextData();///////////////////////////////
				for(int j = 1; j <= NUMOFSLAVES; ++j){

					_itoa(j, (char*)(addr + 1), 10);
					strcat((char*)addr, "z");
					send(pSerialPort, addr, strlen((const char *)addr));
					WaitCommEvent(pSerialPort ->m_hComm, &instanceflag, NULL);
					Sleep(WAIT_AFTER_RXCHAR / 10);
					receive(pSerialPort, tembuf, nbytegot);//useless replied data must be received and ditch

					send(pSerialPort, beginwrite, strlen((char*)beginwrite));
					WaitCommEvent(pSerialPort ->m_hComm, &instanceflag, NULL);
					Sleep(WAIT_AFTER_RXCHAR / 10);
					receive(pSerialPort, tembuf, nbytegot);	//useless replied data must be received and ditch
					///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

					send(pSerialPort, (unsigned char*)strToSend2, strlen(strToSend2));//////////////////////////////////////////////////////////////
					WaitCommEvent(pSerialPort ->m_hComm, &instanceflag, NULL);
					Sleep(WAIT_AFTER_RXCHAR / 10);				
					receive(pSerialPort, tembuf, nbytegot);


					WaitCommEvent(pSerialPort ->m_hComm, &instanceflag, NULL);
					Sleep(WAIT_AFTER_RXCHAR / 10);				
					receive(pSerialPort, tembuf, nbytegot);

					send(pSerialPort, (unsigned char*)"McuComm: reply to mcu's datawrittenz", strlen("McuComm: reply to mcu's datawrittenz"));

					cout << "McuComm: data has been written into #" << j << " mcu" << endl;
				}

				cout << "McuComm: data has been written into all mcus" << endl;


				EnterCriticalSection(&crtc);
				if(whenToQuit == TemCnt - 1 && i == RoundPerTem - 1){
					ExitFlag = 1;//表示没有更多测量需求了
				}
				LeaveCriticalSection(&crtc);

				//inform cardinformer thread to run
				ReleaseSemaphore(hsemDataReady, 1, 0);	
				WaitForSingleObject(hsemMeasDone, INFINITE);
				cout << "MucComm: one data has been written into all mcus and measure is done!" << endl;

			}
			++whenToQuit;
			++cnt;
			system("cmd.bat");



			if(ExitFlag){
				break;
			}


		}  


		cout << "McuComm: quiting" << endl;
		temfile.close();

		simulateType("quit\n\n");
		std::cout.flush();
		return 0;
	}
#endif

#ifdef Seriously
	unsigned int WINAPI McuComm(void *pParam)
	{
		CSerialPort *pSerialPort = reinterpret_cast<CSerialPort*>(pParam);
		unsigned char addr[10];
		unsigned char *tem = reinterpret_cast<unsigned char *>("temz");
		unsigned char *beginwrite = reinterpret_cast<unsigned char *>("nextdataz");
		DWORD instanceflag = 0;

		char tembuf[IN_BUF_SZ];
		int nbytegot, counter2 = 0, i = 0;
		int whenToQuit = 0;

		strcpy(filename, mydatedir);
		strcat(filename, "\\");
		strcat(filename, "temByMcu.txt");
		ofstream temfile(filename);

#ifdef _simulation
		Sleep(LETMCUGOFIRST);
#endif


		unsigned curstate;
		int *temDeltaQueue = new int[qMaxLen];
		int counter = 0, qIndex = 0, lastState = NOTCHANGING, cnt = 1;
		for (i = 0; i < qMaxLen; ++i)
		{
			temDeltaQueue[i] = 0;
		}



		SetCommMask(pSerialPort ->m_hComm, EV_RXCHAR);
		addr[0] = 'a';
		while (1)
		{

			FILE *fp2;
			sprintf(strToSend2, fmt, default_1CtrlData[0], default_1CtrlData[1], default_1CtrlData[2], default_1CtrlData[3],default_1CtrlData[4],default_1CtrlData[5],default_1CtrlData[6],default_1CtrlData[7]);

			for(int j = 1; j <= NUMOFSLAVES; ++j){

				_itoa(j, (char*)(addr + 1), 10);
				strcat((char*)addr, "z");
				send(pSerialPort, addr, strlen((const char *)addr));
				WaitCommEvent(pSerialPort ->m_hComm, &instanceflag, NULL);
				Sleep(WAIT_AFTER_RXCHAR / 10);
				receive(pSerialPort, tembuf, nbytegot);//useless replied data must be received and ditch

				send(pSerialPort, beginwrite, strlen((char*)beginwrite));
				WaitCommEvent(pSerialPort ->m_hComm, &instanceflag, NULL);
				Sleep(WAIT_AFTER_RXCHAR / 10);
				receive(pSerialPort, tembuf, nbytegot);	//useless replied data must be received and ditch
				///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

				send(pSerialPort, (unsigned char*)strToSend2, strlen(strToSend2));//////////////////////////////////////////////////////////////
				WaitCommEvent(pSerialPort ->m_hComm, &instanceflag, NULL);
				Sleep(WAIT_AFTER_RXCHAR / 10);				
				receive(pSerialPort, tembuf, nbytegot);


				WaitCommEvent(pSerialPort ->m_hComm, &instanceflag, NULL);
				Sleep(WAIT_AFTER_RXCHAR / 10);				
				receive(pSerialPort, tembuf, nbytegot);

				send(pSerialPort, (unsigned char*)"McuComm: reply to mcu's datawrittenz", strlen("McuComm: reply to mcu's datawrittenz"));

				cout << "McuComm: default data for a has been written into #" << j << " mcu" << endl;
			}
			cout << "McuComm: default data for a has been written into all mcus" << endl;




			std::cout << "90 degree:" << endl;
			expecting_man=true;		WaitForSingleObject(hRotate, INFINITE);expecting_man=false;
			std::cout << "measuring ..." << endl;
			sprintf(filename, "%s\\tem %d deg %d.txt", mydatedir, cnt, 90);
			fp2 = fopen(filename, "w");
			fprintf(fp2, "%d\n", ++dataFileID);
			fprintf(fp2, "no used line\ndeg = %d\n", 90);

			scan_meas_cnt(pvi, fp2);
			fclose(fp2);


			std::cout << "270 degree:" << endl;
			expecting_man=true;		WaitForSingleObject(hRotate, INFINITE);expecting_man=false;
			std::cout << "measuring ..." << endl;
			sprintf(filename, "%s\\tem %d deg %d.txt", mydatedir, cnt, 270);
			fp2 = fopen(filename, "w");
			fprintf(fp2, "%d\n", ++dataFileID);
			fprintf(fp2, "no used line\ndeg = %d\n", 270);

			scan_meas_cnt(pvi, fp2);
			fclose(fp2);





			temfile << ++counter << ":\t";
			for(i = 1; i <= NUMOFSLAVES; ++i){

				_itoa(i, (char*)(addr + 1), 10);
				strcat((char*)addr, "z");
				send(pSerialPort, addr, strlen((const char *)addr));
				WaitCommEvent(pSerialPort ->m_hComm, &instanceflag, NULL);
				Sleep(WAIT_AFTER_RXCHAR / 10);
				receive(pSerialPort, tembuf, nbytegot);

				send(pSerialPort, tem, strlen((char*)tem));//发送读取温度指令	
				WaitCommEvent(pSerialPort ->m_hComm, &instanceflag, NULL);
				Sleep(WAIT_AFTER_RXCHAR / 10);
				receive(pSerialPort, tembuf, nbytegot);
				//here, the mcu must know to pause awhile between two succesive sending
				WaitCommEvent(pSerialPort ->m_hComm, &instanceflag, NULL);//read and store
				Sleep(WAIT_AFTER_RXCHAR / 10);
				receiveNum(pSerialPort, tembuf, nbytegot);
				send(pSerialPort, (unsigned char *)"McuComm:: tem gotz", strlen((char*)"McuComm:: tem gotz"));


				Sleep(WAIT_AFTER_RXCHAR / 10);
				temfile << tembuf << "\t";
				for(int iii = 0; iii < sizeof(tembuf); ++iii){
					tembuf[iii] = 0;
				}

			}
			temfile << "\n";
			temfile.flush();


			for(int i = 0; i < RoundPerTem; ++i){
				getNextData();///////////////////////////////
				for(int j = 1; j <= NUMOFSLAVES; ++j){

					_itoa(j, (char*)(addr + 1), 10);
					strcat((char*)addr, "z");
					send(pSerialPort, addr, strlen((const char *)addr));
					WaitCommEvent(pSerialPort ->m_hComm, &instanceflag, NULL);
					Sleep(WAIT_AFTER_RXCHAR / 10);
					receive(pSerialPort, tembuf, nbytegot);//useless replied data must be received and ditch

					send(pSerialPort, beginwrite, strlen((char*)beginwrite));
					WaitCommEvent(pSerialPort ->m_hComm, &instanceflag, NULL);
					Sleep(WAIT_AFTER_RXCHAR / 10);
					receive(pSerialPort, tembuf, nbytegot);	//useless replied data must be received and ditch
					///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

					send(pSerialPort, (unsigned char*)strToSend2, strlen(strToSend2));//////////////////////////////////////////////////////////////
					WaitCommEvent(pSerialPort ->m_hComm, &instanceflag, NULL);
					Sleep(WAIT_AFTER_RXCHAR / 10);				
					receive(pSerialPort, tembuf, nbytegot);


					WaitCommEvent(pSerialPort ->m_hComm, &instanceflag, NULL);
					Sleep(WAIT_AFTER_RXCHAR / 10);				
					receive(pSerialPort, tembuf, nbytegot);

					send(pSerialPort, (unsigned char*)"McuComm: reply to mcu's datawrittenz", strlen("McuComm: reply to mcu's datawrittenz"));

					cout << "McuComm: data has been written into #" << j << " mcu" << endl;
				}

				cout << "McuComm: data has been written into all mcus" << endl;


				EnterCriticalSection(&crtc);
				if(whenToQuit == TemCnt - 1 && i == RoundPerTem - 1){
					ExitFlag = 1;//表示没有更多测量需求了
				}
				LeaveCriticalSection(&crtc);

				//inform cardinformer thread to run
				ReleaseSemaphore(hsemDataReady, 1, 0);	
				WaitForSingleObject(hsemMeasDone, INFINITE);
				cout << "MucComm: one data has been written into all mcus and measure is done!" << endl;

			}
			++whenToQuit;
			++cnt;
			system("cmd.bat");



			if(ExitFlag){
				break;
			}


		}  


		cout << "McuComm: quiting" << endl;
		temfile.close();
		return 0;
	}
#endif
#else
unsigned int WINAPI McuComm(void *pParam)
{
	CSerialPort *pSerialPort = reinterpret_cast<CSerialPort*>(pParam);
	unsigned char addr[10];
	unsigned char *tem = reinterpret_cast<unsigned char *>("temz");
	unsigned char *beginwrite = reinterpret_cast<unsigned char *>("nextdataz");
	DWORD instanceflag = 0;
	ofstream temfile("temByMcu.txt");
	char tembuf[IN_BUF_SZ];
	int nbytegot, counter = 0, i = 0;

	//in real experiment, just let mcu runs first
#ifdef _simulation
	Sleep(LETMCUGOFIRST);
#endif

	SetCommMask(pSerialPort ->m_hComm, EV_RXCHAR);
	addr[0] = 'a';
	while (1)
	{
		temfile << ++counter << ":\t";
		for(i = 1; i <= NUMOFSLAVES; ++i){

			_itoa(i, (char*)(addr + 1), 10);
			strcat((char*)addr, "z");
			send(pSerialPort, addr, strlen((const char *)addr));
			WaitCommEvent(pSerialPort ->m_hComm, &instanceflag, NULL);
			Sleep(WAIT_AFTER_RXCHAR / 10);
			receive(pSerialPort, tembuf, nbytegot);

			send(pSerialPort, tem, strlen((char*)tem));//发送读取温度指令	
			WaitCommEvent(pSerialPort ->m_hComm, &instanceflag, NULL);
			Sleep(WAIT_AFTER_RXCHAR / 10);
			receive(pSerialPort, tembuf, nbytegot);
			//here, the mcu must know to pause awhile between two succesive sending
			WaitCommEvent(pSerialPort ->m_hComm, &instanceflag, NULL);//read and store
			Sleep(WAIT_AFTER_RXCHAR / 10);
			receiveNum(pSerialPort, tembuf, nbytegot);

			send(pSerialPort, (unsigned char *)"McuComm:: tem gotz", strlen((char*)"McuComm:: tem gotz"));
			Sleep(WAIT_AFTER_RXCHAR / 10);
			temfile << tembuf << "\t";
			for(int iii = 0; iii < sizeof(tembuf); ++iii){
				tembuf[iii] = 0;
			}

		}
		temfile << "\n";


		for(int i = 0; i < RoundPerTem; ++i){

			for(int j = 1; j <= NUMOFSLAVES; ++j){

				_itoa(j, (char*)(addr + 1), 10);
				strcat((char*)addr, "z");
				send(pSerialPort, addr, strlen((const char *)addr));
				WaitCommEvent(pSerialPort ->m_hComm, &instanceflag, NULL);
				Sleep(WAIT_AFTER_RXCHAR / 10);
				receive(pSerialPort, tembuf, nbytegot);//useless replied data must be received and ditch

				send(pSerialPort, beginwrite, strlen((char*)beginwrite));
				WaitCommEvent(pSerialPort ->m_hComm, &instanceflag, NULL);
				Sleep(WAIT_AFTER_RXCHAR / 10);
				receive(pSerialPort, tembuf, nbytegot);	//useless replied data must be received and ditch

				WaitCommEvent(pSerialPort ->m_hComm, &instanceflag, NULL);
				Sleep(WAIT_AFTER_RXCHAR / 10);				
				receive(pSerialPort, tembuf, nbytegot);
				send(pSerialPort, (unsigned char*)"McuComm: reply to mcu's datawrittenz", strlen("McuComm: reply to mcu's datawrittenz"));
				cout << "McuComm: data has been written into #" << j << " mcu" << endl;
			}

			cout << "McuComm: data has been written into all mcus" << endl;
			cout << "MucComm: Now, Cardinformer is to generate pulse!" << endl;				
			EnterCriticalSection(&crtc);
			if(ExitFlag1 && (i == RoundPerTem - 1)){
				ExitFlag2 = 1;//表示没有更多测量需求了
			}
			LeaveCriticalSection(&crtc);

			//inform cardinformer thread to run
			ReleaseSemaphore(hsemDataReady, 1, 0);	
			WaitForSingleObject(hsemMeasDone, INFINITE);
			cout << "MucComm: one data has been written into all mcus and measure is done!" << endl;

		}
#ifdef _de_bug
		cout << "almost there!\n" << "exitflag 1 is :" << ExitFlag1 << endl;
#endif
		EnterCriticalSection(&crtc);
		if(ExitFlag1){
			cout << "\nMcuComm: i'm done!\n" << endl;

		}
		LeaveCriticalSection(&crtc);

		if(ExitFlag1){
			break;
		}					

		WaitForSingleObject(hsemNewTemSensed, INFINITE);
		cout << "McuComm: tem come copied" << endl;

	}  
	temfile.close();
	return 0;
}
#endif


#ifdef _simMcu

#define DATA_WRITING_TIME 1000
#include <stdlib.h>
#include <time.h>
unsigned int WINAPI Mcu(void *pParam)
{
	CSerialPort *hcomm = reinterpret_cast<CSerialPort *>(pParam);
	int temcnt = TemCnt, cnt = RoundPerTem, nrcvd;
	DWORD temp;
	char buf[IN_BUF_SZ];
	bool ext = false;
	char randomTem[10];
#define SELFADDR 1
	
	srand((unsigned int)time((unsigned)0));

	SetCommMask(hcomm ->m_hComm, EV_RXCHAR);
	while(1){

		WaitCommEvent(hcomm ->m_hComm, &temp, 0);
		Sleep(WAIT_AFTER_RXCHAR / 10);
		receive(hcomm, buf, nrcvd);
		if(SELFADDR != atoi((const char*)(buf + 1))){
			continue;
		}
		send(hcomm,(unsigned char*)"address verified!", strlen("address verified!"));

		//wait for a cmd
		WaitCommEvent(hcomm ->m_hComm, &temp, 0);
		Sleep(WAIT_AFTER_RXCHAR / 10);
		receive(hcomm, buf, nrcvd);

		if(!strcmp(buf, "temz")){
			//ack to the cmd
			send(hcomm,(unsigned char*)"tem cmd received", strlen("tem cmd received"));
			//wait for master to receive before sending real data
			Sleep(WAIT_BETWEEN_SUCCESIVE_SENDING);

			int x = rand() % 100 + 1;
			randomTem[0] = char(x / 100 + '0');
			randomTem[1] = char((x / 10) % 10 + '0');
			randomTem[2] = char(x % 10 + '0');
			randomTem[3] = '\0';
			send(hcomm,(unsigned char *) randomTem, strlen(randomTem));
			--temcnt;
			//wait for a ack
			WaitCommEvent(hcomm ->m_hComm, &temp, 0);
			Sleep(WAIT_AFTER_RXCHAR / 10);
			receive(hcomm, buf, nrcvd);

		}
		else if(!strcmp(buf, "nextdataz")){
			//ack to the cmd
			send(hcomm,(unsigned char*)"nextdata cmd received", strlen("nextdata cmd received"));
			//wait for master to receive before sending real data
			Sleep(WAIT_BETWEEN_SUCCESIVE_SENDING);

			//模拟写入数据
			Sleep(DATA_WRITING_TIME / 100);		
			cout << "Mcu: writing data to asic done!" << endl;
			//written done notification
			send(hcomm,(unsigned char *) "datawritten", strlen("datawritten"));
			--cnt;
			//wait for a ack
			WaitCommEvent(hcomm ->m_hComm, &temp, 0);
			Sleep(WAIT_AFTER_RXCHAR / 10);
			receive(hcomm, buf, nrcvd);

			if(!cnt){
				if(!temcnt){
					cout << "\nMcu: i'm done!\n" << endl;
					return 0;
				}
				cnt = RoundPerTem;
			}	

		}
		else{//transmission error, more possible when receive nextdata cmd, because there more of them
			//ack to the cmd
			send(hcomm,(unsigned char*)"nextdata cmd received", strlen("nextdata cmd received"));
			//wait for master to receive before sending real data
			Sleep(WAIT_BETWEEN_SUCCESIVE_SENDING);

			//模拟写入数据
			Sleep(DATA_WRITING_TIME / 100);		
			cout << "Mcu: writing data to asic done!" << endl;
			//written done notification
			send(hcomm,(unsigned char *) "datawritten", strlen("datawritten"));
			--cnt;
			//wait for a ack
			WaitCommEvent(hcomm ->m_hComm, &temp, 0);
			Sleep(WAIT_AFTER_RXCHAR / 10);
			receive(hcomm, buf, nrcvd);

			if(!cnt){
				if(!temcnt){
					cout << "\nMcu: i'm done!\n" << endl;
					return 0;
				}
				cnt = RoundPerTem;
			}	
		}
	}
}
#endif
/*

*/

#ifdef Use_Two_Port
#define DATA_WRITING_TIME 1000
#include <stdlib.h>
#include <time.h>
unsigned int WINAPI McuAndSensor(void *pParam)
{
	ofstream temgenerated("temByTemOven.txt");
	int oddeven = 0, cnttem = TemCnt - 1;
	const int stableCnt = stableTimeSpan / InqInteval;
	DWORD instanceflag;
	int nbytegot, curtem = InitTem;
	char tembuf[5];
	int i = 0;
	curStepLeft = stableCnt;
	InitializeCriticalSection(&switcher);

	CSerialPort *hcomm = reinterpret_cast<CSerialPort *>(pParam);
	int temcnt = TemCnt, cnt = RoundPerTem, nrcvd;
	DWORD temp;
	char buf[IN_BUF_SZ];
	bool ext = false;
	char randomTem[10];
#define SELFADDR 1

	srand((unsigned int)time((unsigned)0));

	SetCommMask(hcomm ->m_hComm, EV_RXCHAR);
	while(1){

		while(1){
			WaitCommEvent(hcomm ->m_hComm, &temp, 0);
			Sleep(WAIT_AFTER_RXCHAR / 10);
			receive(hcomm, buf, nrcvd);
			if(SELFADDR != atoi((const char*)(buf + 1))){
				continue;
			}
			send(hcomm,(unsigned char*)"address verified!", strlen("address verified!"));

			//wait for a cmd
			WaitCommEvent(hcomm ->m_hComm, &temp, 0);
			Sleep(WAIT_AFTER_RXCHAR / 10);
			receive(hcomm, buf, nrcvd);

			if(!strcmp(buf, "temz")){
				//ack to the cmd
				send(hcomm,(unsigned char*)"tem cmd received", strlen("tem cmd received"));
				//wait for master to receive before sending real data
				Sleep(WAIT_BETWEEN_SUCCESIVE_SENDING);

				int x = rand() % 100 + 1;
				randomTem[0] = char(x / 100 + '0');
				randomTem[1] = char((x / 10) % 10 + '0');
				randomTem[2] = char(x % 10 + '0');
				randomTem[3] = '\0';
				send(hcomm,(unsigned char *) randomTem, strlen(randomTem));
				--temcnt;
				//wait for a ack
				WaitCommEvent(hcomm ->m_hComm, &temp, 0);
				Sleep(WAIT_AFTER_RXCHAR / 10);
				receive(hcomm, buf, nrcvd);

			}
			else if(!strcmp(buf, "nextdataz")){
				//ack to the cmd
				send(hcomm,(unsigned char*)"nextdata cmd received", strlen("nextdata cmd received"));
				//wait for master to receive before sending real data
				Sleep(WAIT_BETWEEN_SUCCESIVE_SENDING);

				//模拟写入数据
				Sleep(DATA_WRITING_TIME / 100);		
				cout << "Mcu: writing data to asic done!" << endl;
				//written done notification
				send(hcomm,(unsigned char *) "datawritten", strlen("datawritten"));
				--cnt;
				//wait for a ack
				WaitCommEvent(hcomm ->m_hComm, &temp, 0);
				Sleep(WAIT_AFTER_RXCHAR / 10);
				receive(hcomm, buf, nrcvd);

				if(!cnt){
					if(!temcnt){
						cout << "\nMcu: i'm done!\n" << endl;
						temgenerated.close();
						return 0;
					}
					cnt = RoundPerTem;
					break;
				}	

			}
			else{//transmission error, more possible when receive nextdata cmd, because there more of them
				//ack to the cmd
				send(hcomm,(unsigned char*)"nextdata cmd received", strlen("nextdata cmd received"));
				//wait for master to receive before sending real data
				Sleep(WAIT_BETWEEN_SUCCESIVE_SENDING);

				//模拟写入数据99
				Sleep(DATA_WRITING_TIME / 100);		
				cout << "Mcu: writing data to asic done!" << endl;
				//written done notification
				send(hcomm,(unsigned char *) "datawritten", strlen("datawritten"));
				--cnt;
				//wait for a ack
				WaitCommEvent(hcomm ->m_hComm, &temp, 0);
				Sleep(WAIT_AFTER_RXCHAR / 10);
				receive(hcomm, buf, nrcvd);

				if(!cnt){
					if(!temcnt){
						cout << "\nMcu: i'm done!\n" << endl;
						temgenerated << "error" << endl;
						temgenerated.close();
						return 0;
					}
					cnt = RoundPerTem;
				}	
			}

		}
#define InqInteval (0 + WAIT_AFTER_RXCHAR / 10 * 2)
#define stableTimeSpan (1000 * 5)
#define TimeForTemToSettle (stableTimeSpan * 0.1)
#define stepHight 20
#define TimeOfRising (1000 * 1)
#define InitTem 2000
#define TemDeltaOneTime (stepHight / (TimeOfRising / InqInteval))
		while(1){
			EnterCriticalSection(&switcher);
			if(switchToMcu){
				switchToMcu = false;
				LeaveCriticalSection(&switcher);//之前少了这句，结果没离开临界区就break，导致临界区再也无法进入了
				break;
			}
			LeaveCriticalSection(&switcher);

			//wait for inqiry;
			WaitCommEvent(hcomm ->m_hComm, &instanceflag, NULL);//read and store
			Sleep(WAIT_AFTER_RXCHAR / 10);
			receive(hcomm, buf, nbytegot);

			if(99 != atoi((const char*)(buf + 1))){
				cout << "sensor:: sensor is expecting 99 cmd" << endl;
				exit(-1);
			}
			send(hcomm,(unsigned char*)"sensor::ack", strlen("sensor::ack"));

			WaitCommEvent(hcomm ->m_hComm, &instanceflag, NULL);//read and store
			Sleep(WAIT_AFTER_RXCHAR / 10);
			receive(hcomm, buf, nbytegot);

			if(strcmp(buf, "temz")){
				cout << "sensor:: sensor is expecting temz cmd" << endl;
				exit(-1);
			}
			send(hcomm,(unsigned char*)"ack to tem query", strlen("ack to tem query"));
			Sleep(WAIT_BETWEEN_SUCCESIVE_SENDING);
			//give next tem;
			if(curStepLeft == 0){
				if (increasing){
					curStepLeft = stableCnt;
				}
				else{
					curStepLeft = TimeOfRising / InqInteval;
				}
				increasing = !increasing;
			}//bug found
			--curStepLeft;
				

			if (increasing){
				_itoa((curtem = curtem + TemDeltaOneTime), tembuf, 10);
			}
			else{
				_itoa(curtem, tembuf, 10);
			}
			temgenerated << tembuf << endl;
			send(hcomm, (unsigned char *)tembuf, strlen(tembuf));
			WaitCommEvent(hcomm ->m_hComm, &temp, 0);
			Sleep(WAIT_AFTER_RXCHAR / 10);
			receive(hcomm, buf, nrcvd);

			WaitForSingleObject(hsemMoreMeasure, INFINITE);
		}
	}
}
#endif
