#include "global_const.h"
#include "visaUtils2.h"

#ifdef USING34401

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <Windows.h>
#include "prepareDataPc.h"

static int begNUm = 18;
static char visa_cmd[100];
static char rout_scan_form0[]= "rout:scan (@%d%d)\n";
static char rout_scan_form[]= "rout:scan (@%d%d:%d%d)\n";
static char rout_scan_formSep[]= "rout:scan (@%d%d,%d%d)\n";
static char trig_coun_form[] = "trig:coun %d\n";

#define bufLen (10 + NUMOFSLAVES * 20)*10
static char buf[bufLen];
unsigned long len;
ViSession defaultRM, vi;
ViSession *pvi;

FILE *fpAll = NULL;

void initGpib()
{
	char deviceName[256];
	FindRSrc(deviceName);
	viOpenDefaultRM (&defaultRM);
	viOpen (defaultRM, deviceName, VI_NULL,VI_NULL, &vi);
	pvi = &vi;

	init_equipment(pvi);
}

void releaseGpib()
{
	viClose (vi);
	viClose (defaultRM);
}
int anyErr(ViSession *pvi)
{

  ViStatus err;
  static char buf[1024]={0};
  int err_no;
  int err_cnt = 0;
  err=viPrintf(*pvi, "SYSTEM:ERR?\n");


  err=viScanf (*pvi, "%d%t", &err_no, &buf);


  if(err_no >0){
	++err_cnt;
  }

  err=viFlush(*pvi, VI_READ_BUF);
  err=viFlush(*pvi, VI_WRITE_BUF);
  viPrintf(*pvi, "*rst\n");
  return err_cnt;

}
int set_slot_num(ViSession *pvi)
{
	char cmd[] = "rout:scan (@101)\n";
	int res = 0;
	ViStatus status = -1;

	do{
		++res;
		if(res == 4)
			return -1;
		cmd[12] = res + '0';
		status = viPrintf(*pvi, cmd);
	}while(anyErr(pvi) > 0);

	viPrintf(*pvi, "*rst\n");

	return res;

}
void init_equipment(ViSession *pvi)
{
	int slotNum;
	/*if( (slotNum = set_slot_num(pvi)) == -1){
		printf("error finding route\n");
		exit(-1);

	}*/

	viPrintf(*pvi, "*rst\n");
/*
	//setup routes
	if(NUMOFSLAVES == 1){
		sprintf(visa_cmd, rout_scan_form0, slotNum, begNUm);
	}
	else{
		//sprintf(visa_cmd, rout_scan_form, slotNum, begNUm, slotNum, begNUm + NUMOFSLAVES - 1);
		sprintf(visa_cmd, rout_scan_formSep, slotNum, 11, slotNum, 14);
	}

	viPrintf(*pvi, visa_cmd);
	Sleep(500);*/

	//setup trig source
	//viPrintf(*pvi, "trig:sour bus\n");

	//setup trig count
	sprintf(visa_cmd, trig_coun_form, MEAS_CNT);
	viPrintf(*pvi, visa_cmd);
}


int get_cnt(ViSession *pvi)
{
	int res;
	viPrintf(*pvi, "data:poin?\n");
	viRead (*pvi, (ViBuf)buf, bufLen, &len);
	buf[len]=0;
	sscanf(buf, "%d", &res);
	return res;
}


void scan_meas_cnt_line(ViSession *pvi, FILE *fp)
{
	int cnt = 0;
	int actualCnt;
	int i;
	if(!fpAll){
		sprintf(buf, "%s%c%c%s", mydatedir, '\\', '\\', "allData.txt");
		fpAll = fopen("buf", "w");
	}
	sprintf(visa_cmd, trig_coun_form, MEAS_CNT);
	viPrintf(*pvi, visa_cmd);Sleep(10);
	viPrintf(*pvi, "trig:sour bus\n");Sleep(10);
	viPrintf(*pvi, "init\n");

	do{
		Sleep(10);
		viPrintf(*pvi, "*trg\n");
		Sleep(MEAS_INTERVAL);
		while(get_cnt(pvi) < 1){
			Sleep(10);
		}
		cnt++;

		//////////////////////////////////////////////////////////////////////////注意器件过多或者重复次数过多可能会导致存储器溢出

	}while(cnt != MEAS_CNT);

	viPrintf(*pvi, "fetc?\n");
	//viPrintf(*pvi, "r?\n");
	viRead (*pvi, (ViBuf)buf, bufLen, &len);
	buf[len]='\0';
	for(i = 0; i < len; ++i){
		if(buf[i] == ',')
			buf[i] = ' ';
	}

	fprintf(fp, "%s", buf);//////////////////////////////////////////////////////////////////////////
	viPrintf(*pvi, "*rst\n");
	fprintf(fpAll, "%s -- %s\n", strToSend2, buf);
	fflush(fpAll);
}

double res[MEAS_CNT];
double scan_meas_cnt_aver(ViSession *pvi, FILE *fp)
{
	int cnt = 0;
	int actualCnt;
	int i;
	char *pbuf = buf+1;
	double res = 0, tmp;

	if(!fpAll){
		sprintf(buf, "%s%c%c%s", mydatedir, '\\', '\\', "allData.txt");
		fpAll = fopen("buf", "w");
	}
	sprintf(visa_cmd, trig_coun_form, MEAS_CNT);
	viPrintf(*pvi, visa_cmd);Sleep(10);
	viPrintf(*pvi, "trig:sour bus\n");Sleep(10);
	viPrintf(*pvi, "init\n");

	do{
		Sleep(10);
		viPrintf(*pvi, "*trg\n");
		Sleep(MEAS_INTERVAL);
		while(get_cnt(pvi) < 1){
			Sleep(10);
		}
		cnt++;

		//////////////////////////////////////////////////////////////////////////注意器件过多或者重复次数过多可能会导致存储器溢出

	}while(cnt != MEAS_CNT);

	viPrintf(*pvi, "fetc?\n");
	//viPrintf(*pvi, "r?\n");
	viRead (*pvi, (ViBuf)buf, bufLen, &len);
	buf[len]='\0';
	for(i = 0; i < len; ++i){
		if(buf[i] == ',')
			buf[i] = ' ';
	}



	for(i = 0; i < MEAS_CNT; ++i){

		sscanf(pbuf, "%lf", &tmp);
		printf("%lf\n", tmp);
		res+=tmp;
		for(;*pbuf;++pbuf){
			if(*pbuf == ' '){
				pbuf++;pbuf++;
				break;
			}
		}
		if(!*pbuf){
			i++;
			break;
		}
	}
	
	fprintf(fp, "%s", buf);//////////////////////////////////////////////////////////////////////////
	viPrintf(*pvi, "*rst\n");
	fprintf(fpAll, "%s -- %s\n", strToSend2, buf);
	fflush(fpAll);
	return res / i;
}


void scan_meas_cnt(ViSession *pvi, FILE *fp)
{
	int cnt = 0;
	int actualCnt;
	int i;
	if(!fpAll){
		sprintf(buf, "%s%c%c%s", mydatedir, '\\', '\\', "allData.txt");
		fpAll = fopen("buf", "w");
	}
	sprintf(visa_cmd, trig_coun_form, MEAS_CNT);
	viPrintf(*pvi, visa_cmd);Sleep(10);
	viPrintf(*pvi, "trig:sour bus\n");Sleep(10);
	viPrintf(*pvi, "init\n");

	do{
		Sleep(10);
		viPrintf(*pvi, "*trg\n");
		Sleep(MEAS_INTERVAL);
		while(get_cnt(pvi) < 1){
			Sleep(10);
		}
		cnt++;

		//////////////////////////////////////////////////////////////////////////注意器件过多或者重复次数过多可能会导致存储器溢出

	}while(cnt != MEAS_CNT);

	viPrintf(*pvi, "fetc?\n");
	//viPrintf(*pvi, "r?\n");
	viRead (*pvi, (ViBuf)buf, bufLen, &len);
	buf[len]='\0';

	for(i = 0; i < len; ++i){
		if(buf[i] == ',')
			buf[i] = '\n';
	}
	fprintf(fp, "%s", buf);//////////////////////////////////////////////////////////////////////////
	viPrintf(*pvi, "*rst\n");

	for(i = 0; i < len; ++i){
		if(buf[i] == '\n')
			buf[i] = ' ';
	}
	fprintf(fpAll, "%s -- %s\n", strToSend2, buf);
	fflush(fpAll);
}




int FindRSrc(char *instrDescriptor)
 {
 ViUInt32 numInstrs;
 ViFindList findList;
 ViStatus status;
 ViSession defaultRM, instr;

 status = viOpenDefaultRM(&defaultRM);
 if ( status < VI_SUCCESS ) {
 printf("Could not open a session to the VISA Resource Manager!");
 return status;
 }

 status = viFindRsrc( defaultRM, "GPIB?*INSTR", &findList, &numInstrs, instrDescriptor );
 if ( status < VI_SUCCESS ) {
	 printf("An error occurred while finding resources.");
	 return status;
 }

 if ( numInstrs > 0 ) {
 printf("%s found!\n", instrDescriptor);//Agilent->ComboBox_GPIB->Items->Add(instrDescriptor);
 }
 viClose (defaultRM);
 return 0;
 //status = viOpen( defaultRM, instrDescriptor, VI_NULL, VI_NULL, &instr );

 while ( --numInstrs ) {
 status = viFindNext( findList, instrDescriptor );
 if ( status < VI_SUCCESS ) {
 printf("An error occurred finding the next resource.");
 return status;
 }
 //这个时候的instrDescriptor就是你电脑上找到的资源，并且可用，如果为空就说明你的连线有问题
}

   //      viClose(instr);
 viClose(findList);

 return status;
 } 
#else
#include "visaUtils.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <Windows.h>
#include "findRsrc.h"
static int begNUm = 18;
static char visa_cmd[100];
static char rout_scan_form0[]= "rout:scan (@%d%d)\n";
static char rout_scan_form[]= "rout:scan (@%d%d:%d%d)\n";
static char rout_scan_formSep[]= "rout:scan (@%d%d,%d%d)\n";
static char trig_coun_form[] = "trig:coun %d\n";

#define bufLen (10 + NUMOFSLAVES * 20)*10
static char buf[bufLen];
unsigned long len;
ViSession defaultRM, vi;
ViSession *pvi;



void initGpib()
{
	char deviceName[256];
	FindRSrc(deviceName);
	viOpenDefaultRM (&defaultRM);
	viOpen (defaultRM, deviceName, VI_NULL,VI_NULL, &vi);
	pvi = &vi;

	init_equipment(pvi);
}

void releaseGpib()
{
	viClose (vi);
	viClose (defaultRM);
}
int anyErr(ViSession *pvi)
{

	ViStatus err;
	static char buf[1024]={0};
	int err_no;
	int err_cnt = 0;
	err=viPrintf(*pvi, "SYSTEM:ERR?\n");


	err=viScanf (*pvi, "%d%t", &err_no, &buf);


	if(err_no >0){
		++err_cnt;
	}

	err=viFlush(*pvi, VI_READ_BUF);
	err=viFlush(*pvi, VI_WRITE_BUF);
	viPrintf(*pvi, "*rst\n");
	return err_cnt;

}
int set_slot_num(ViSession *pvi)
{
	char cmd[] = "rout:scan (@101)\n";
	int res = 0;
	ViStatus status = -1;

	do{
		++res;
		if(res == 4)
			return -1;
		cmd[12] = res + '0';
		status = viPrintf(*pvi, cmd);
	}while(anyErr(pvi) > 0);

	viPrintf(*pvi, "*rst\n");

	return res;

}
void init_equipment(ViSession *pvi)
{
	int slotNum;
	if( (slotNum = set_slot_num(pvi)) == -1){
		printf("error finding route\n");
		exit(-1);

	}

	viPrintf(*pvi, "*rst\n");

	//setup routes
	if(NUMOFSLAVES == 1){
		sprintf(visa_cmd, rout_scan_form0, slotNum, begNUm);
	}
	else{
		//sprintf(visa_cmd, rout_scan_form, slotNum, begNUm, slotNum, begNUm + NUMOFSLAVES - 1);
		sprintf(visa_cmd, rout_scan_formSep, slotNum, 11, slotNum, 14);
	}

	//viPrintf(*pvi, visa_cmd);
	Sleep(500);

	//setup trig source
	viPrintf(*pvi, "trig:sour bus\n");

	//setup trig count
	sprintf(visa_cmd, trig_coun_form, MEAS_CNT);
	viPrintf(*pvi, visa_cmd);
}


int get_cnt(ViSession *pvi)
{
	int res;
	viPrintf(*pvi, "data:poin?\n");
	viRead (*pvi, (ViBuf)buf, bufLen, &len);
	buf[len]=0;
	sscanf(buf, "%d", &res);
	return res;
}

void scan_meas_cnt(ViSession *pvi, FILE *fp)
{
	int cnt = 0;
	int actualCnt;
	int i;

	viPrintf(*pvi, "init\n");
	do{
		Sleep(10);
		viPrintf(*pvi, "*trg\n");
		Sleep(MEAS_INTERVAL);
		while(get_cnt(pvi) < NUMOFSLAVES){
			Sleep(10);
		}
		cnt++;

		viPrintf(*pvi, "r?\n");
		viRead (*pvi, (ViBuf)buf, bufLen, &len);
		buf[len]='\0';
		for(i = 0; i < len; ++i){
			if(buf[i] == ',')
				buf[i] = ' ';
		}
		fprintf(fp, "%s", buf + 2 + buf[1] - '0');

	}while(cnt != MEAS_CNT);
}




int FindRSrc(char *instrDescriptor)
{
	ViUInt32 numInstrs;
	ViFindList findList;
	ViStatus status;
	ViSession defaultRM, instr;

	status = viOpenDefaultRM(&defaultRM);
	if ( status < VI_SUCCESS ) {
		printf("Could not open a session to the VISA Resource Manager!");
		return status;
	}

	status = viFindRsrc( defaultRM, "GPIB?*INSTR", &findList, &numInstrs, instrDescriptor );
	if ( status < VI_SUCCESS ) {
		printf("An error occurred while finding resources.");
		return status;
	}

	if ( numInstrs > 0 ) {
		printf("%s found!\n", instrDescriptor);//Agilent->ComboBox_GPIB->Items->Add(instrDescriptor);
	}
	viClose (defaultRM);
	return 0;
	//status = viOpen( defaultRM, instrDescriptor, VI_NULL, VI_NULL, &instr );

	while ( --numInstrs ) {
		status = viFindNext( findList, instrDescriptor );
		if ( status < VI_SUCCESS ) {
			printf("An error occurred finding the next resource.");
			return status;
		}
		//这个时候的instrDescriptor就是你电脑上找到的资源，并且可用，如果为空就说明你的连线有问题
	}

	//      viClose(instr);
	viClose(findList);

	return status;
} 
#endif