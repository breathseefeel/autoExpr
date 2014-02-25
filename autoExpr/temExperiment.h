#ifndef _temExperiment
#define _temExperiment

//used in debug
#define _showQueueInGetStateFun
//used to determine which thread to open
#define  Use_Two_Port_
#define Use_One_Port
#define _use_gpib

extern HANDLE hsemDataReady, hsemMeasDone, hsemNewTemSensed, hsemMoreMeasure, hRotate;
//extern const unsigned int TemCnt, RoundPerTem;

unsigned int WINAPI TemOvenListener(void *pParam);
unsigned int WINAPI CardInformer(void *pParam);
unsigned int WINAPI Mcu(void *pParam);
unsigned int WINAPI McuComm(void *pParam);
unsigned int WINAPI Card(void *pParam);
unsigned int WINAPI TemSensorInOven(void *pParam);


extern int temDeltaQueueLen;
extern int upriseTemDelta;
extern int stableTemDeltaMax;

extern int temDeltaQueueLenLearned;
extern int upriseTemDeltaLearned;
extern int stableTemDeltaMaxLearned;
extern int qMaxLen;

extern const int BEGINCHANGING;
extern const int ISCHANGING;
extern const int ENDCHANGING;
extern const int NOTCHANGING;


unsigned getTemState(int *arr, int idx);
unsigned int WINAPI randomInqTem(void *pParam);
#endif