#ifndef _global_const_h
#define _global_const_h
#include "prepareDataPc.h"

#define  NUMOFSLAVES 1
#define TemCnt  3
#define RoundPerTem  SEGMENT6
extern char mydatedir[100];

#define MEAS_INTERVAL 1000


//ѡ��ʹ��Agilent 34401�����ȡ���ú궨�壬Ĭ��ʹ�òɼ���
#define USING34401

//ѡ����ģʽ
//#define  ManualTest
#define AutoTest
//#define Seriously
#ifdef AutoTest
#define AutoTestCNT 100
#endif
#endif