#ifndef _global_const_h
#define _global_const_h
#include "prepareDataPc.h"

#define  NUMOFSLAVES 1
#define TemCnt  3
#define RoundPerTem  SEGMENT6
extern char mydatedir[100];

#define MEAS_INTERVAL 1000


//选择使用Agilent 34401，如果取消该宏定义，默认使用采集仪
#define USING34401

//选择工作模式
//#define  ManualTest
#define AutoTest
//#define Seriously
#ifdef AutoTest
#define AutoTestCNT 100
#endif
#endif