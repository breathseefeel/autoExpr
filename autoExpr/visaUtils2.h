#ifndef _visaUtils_h
#define _visaUtils_h
#include <stdio.h>

extern "C"{
#include "C:\\Program Files (x86)\\IVI Foundation\\VISA\\WinNT\\include\\visa.h"
	//#include "C:\\Program Files\\IVI Foundation\\VISA\\Win64\\Include\\visa.h"
#pragma comment(lib, "C:\\Program Files (x86)\\IVI Foundation\\VISA\\WinNT\\lib\\msc\\visa32.lib")
}
extern ViSession defaultRM;
extern ViSession vi;
extern ViSession *pvi;
#define MEAS_CNT 10
int set_slot_num(ViSession *pvi);
void scan_meas_cnt(ViSession *, FILE *fp);
void scan_meas_cnt_line(ViSession *pvi, FILE *fp);
double scan_meas_cnt_aver(ViSession *pvi, FILE *fp);
int get_cnt(ViSession *);
void init_equipment(ViSession *);
int FindRSrc(char *instrDescriptor);
void initGpib();
void releaseGpib();
extern FILE *fpAll;
#endif