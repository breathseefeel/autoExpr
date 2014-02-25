#include "prepareDataPc.h"
#include <stdio.h>
#include <fstream>
#include <iostream>


char mydatedir[100];
char strToSend2[CTRLDATALEN*5];
//7 3 5 1 5 1 6 1
char *logfilenameform = "%s\\logfile%d-%d.txt";
char logfilename[100];
int filecnt=1;
int temcnt = 1;
FILE *fp=NULL;

unsigned char default_1CtrlData[CTRLDATALEN];//={0,0,0,0,0,0,0,0};
unsigned char default0CtrlData[CTRLDATALEN];//={-1,3,0,0,0, 0, 0, 0};//last byte 0 or 1 both ok, exists a optimum one
unsigned char default1CtrlData[CTRLDATALEN];//={16,-1,0,0,0, 0, 0, 0};//last byte 0 or 1 both ok, exists a optimum one
unsigned char default2CtrlData[CTRLDATALEN];//={8,3,-1,1,31, 0, 63, 0};//last byte 0 or 1 both ok, exists a optimum one

unsigned char default4CtrlData[CTRLDATALEN];//={8,3,0,0,-1, 1, 63, 0};//last byte 0 or 1 both ok, exists a optimum one

unsigned char default6CtrlData[CTRLDATALEN];//={16,3,0,0,0,0,-1, 1};


unsigned int dataToSend[CTRLDATALEN];

const char *fmt = "%d, %d, %d, %d, %d, %d, %d, %dz";
unsigned char counterPPD = 0;
unsigned char defaultDataIsWritten0 = 0;
unsigned char defaultDataIsWritten1 = 0;
unsigned char defaultDataIsWritten2 = 0;

unsigned char defaultDataIsWritten4 = 0;

unsigned char defaultDataIsWritten6 = 0;


void fillArrayWithNextLineInFp(std::ifstream& ifile, unsigned char *arr)
{
	static char buf[100];
	static char bufz[10];
	static int dummyArray[8];
	ifile.getline(buf, 100);
	sscanf(buf, fmt, &dummyArray[0], &dummyArray[1],&dummyArray[2],&dummyArray[3],&dummyArray[4],&dummyArray[5],&dummyArray[6],&dummyArray[7], bufz);
	for(int i = 0; i < 8; ++i){
		arr[i]  = dummyArray[i];
		std::cout << (int)arr[i] << " ";
	}std::cout << std::endl;
}
void resetData()
{
	static bool isDataLoaded = false;
	if(!isDataLoaded){
		std::cout << "reset data" << std::endl;
		std::ifstream ifile("D:\\项目组工作\\matlabfunction\\temExpr\\defaultDataFile.txt");
		if(!ifile){
			std::cout << "can not read file: defaultDataFile.txt" << std::endl;
			exit(-1);
		}
		std::cout << "loaded data:" << std::endl;
		fillArrayWithNextLineInFp(ifile, default_1CtrlData);
		fillArrayWithNextLineInFp(ifile, default0CtrlData);
		fillArrayWithNextLineInFp(ifile, default1CtrlData);
		fillArrayWithNextLineInFp(ifile, default2CtrlData);
		fillArrayWithNextLineInFp(ifile, default4CtrlData);
		fillArrayWithNextLineInFp(ifile, default6CtrlData);
		ifile.close();
		isDataLoaded = true;

	}
	counterPPD = 0;
	defaultDataIsWritten0 = 0;
	defaultDataIsWritten1 = 0;
	defaultDataIsWritten2 = 0;

	defaultDataIsWritten4 = 0;

	defaultDataIsWritten6 = 0;

}

void setData(const unsigned char *arr)
{
	int i;
	for(i = 0; i < 8; i++){
		dataToSend[i] = arr[i];
	}
}
void getNextData(void)
{
	static char buf[CTRLDATALEN*5];
	static bool initalResetIsDone = false;
	if(!initalResetIsDone){
		initalResetIsDone = true;
		resetData();
	}
	if(fp == NULL){
		sprintf(logfilename, logfilenameform, mydatedir, temcnt, filecnt);
		fp = fopen(logfilename, "w");
		if(fp == NULL){
			printf("cannot open file");
		}
	}
	if(counterPPD < SEGMENT0 || counterPPD == SEGMENT6){//对counterPPD分段处理，将对应的数据写入resetData();
		if(counterPPD == SEGMENT6){
			resetData();
		}
		if(!defaultDataIsWritten0){
			setData(default0CtrlData);//set the default data for 1
			defaultDataIsWritten0 = 1;
		}
		{
			int ctrl0Index = counterPPD / DIV6;
			int ctrl6Index = counterPPD % DIV6;
			dataToSend[6] = ctrl6(ctrl6Index);
			dataToSend[0] = ctrl0(ctrl0Index);
			sprintf(strToSend2, fmt, dataToSend[0], dataToSend[1], dataToSend[2], dataToSend[3], dataToSend[4], dataToSend[5], dataToSend[6], dataToSend[7]);
		}
		counterPPD++;
		//set the accelerometer manually

	}
	else if(counterPPD < SEGMENT1){//对counterPPD分段处理，将对应的数据写入
		if(!defaultDataIsWritten1){
			setData(default1CtrlData);//set the default data for 1
			defaultDataIsWritten1 = 1;
		}
		{///////////////////////////////////////////////////////
			int ctrl1Index = (counterPPD-SEGMENT0) / DIV6;
			int ctrl6Index = (counterPPD-SEGMENT0) % DIV6;
			dataToSend[6] = ctrl6(ctrl6Index);
			dataToSend[1] = ctrl1(ctrl1Index);
			sprintf(strToSend2, fmt, dataToSend[0], dataToSend[1], dataToSend[2], dataToSend[3], dataToSend[4], dataToSend[5], dataToSend[6], dataToSend[7]);
		}
		counterPPD++;
		//set the accelerometer manually

	}
	else if(counterPPD < SEGMENT2){
		if(!defaultDataIsWritten2){
			setData(default2CtrlData);//set the default data for 2
			defaultDataIsWritten2 = 1;
		}
		{
			dataToSend[2] = ctrl2(counterPPD-SEGMENT1);
			sprintf(strToSend2, fmt, dataToSend[0], dataToSend[1], dataToSend[2], dataToSend[3], dataToSend[4], dataToSend[5], dataToSend[6], dataToSend[7]);
		}
		counterPPD++;
	}
	else if(counterPPD < SEGMENT4){
		if(!defaultDataIsWritten4){
			setData(default4CtrlData);//set the default data for 4
			defaultDataIsWritten4 = 1;
		}
		{
			dataToSend[4] = ctrl4(counterPPD-SEGMENT2);
			sprintf(strToSend2, fmt, dataToSend[0], dataToSend[1], dataToSend[2], dataToSend[3], dataToSend[4], dataToSend[5], dataToSend[6], dataToSend[7]);
		}
		counterPPD++;
	}
	else if(counterPPD < SEGMENT6){
		if(!defaultDataIsWritten6){
			setData(default6CtrlData);//set the default data for 2
			defaultDataIsWritten6 = 1;
			dataToSend[7] = 1 - dataToSend[7];
		}
		{
			dataToSend[6] = ctrl6(counterPPD-SEGMENT4);
			sprintf(strToSend2, fmt, dataToSend[0], dataToSend[1], dataToSend[2], dataToSend[3], dataToSend[4], dataToSend[5], dataToSend[6], dataToSend[7]);
		}
		counterPPD++;
	}

	
	strcpy(buf, strToSend2);
	buf[strlen(buf)-1] = '\0';
	fprintf(fp, strToSend2);
	fprintf(fp, "\n");
	fflush(fp);
	if(counterPPD == SEGMENT6){
		fclose(fp);
		fp = NULL;
		filecnt = 1;
		temcnt++;
		dataToSend[7] = 1 - dataToSend[7];
	}
}


void writeFileInfo(FILE *fp)
{

	fprintf(fp, "%s\n", strToSend2);

	if(counterPPD <= SEGMENT0){//对counterPPD分段处理，将对应的数据写入resetData();
		int ctrl0Index = (counterPPD-1) / DIV6;
		int ctrl6Index = (counterPPD-1) % DIV6;
		fprintf(fp, "cg = %d, coarse = %d\n", ctrl0(ctrl0Index), ctrl6(ctrl6Index));

	}
	else if(counterPPD <= SEGMENT1){//对counterPPD分段处理，将对应的数据写入
			int ctrl1Index = (counterPPD-SEGMENT0-1) / DIV6;
			int ctrl6Index = (counterPPD-SEGMENT0-1) % DIV6;
			fprintf(fp, "cf = %d, coarse = %d\n", ctrl1(ctrl1Index), ctrl6(ctrl6Index));

	}
	else if(counterPPD <= SEGMENT2){
			fprintf(fp, "finebot = %d\n", ctrl2(counterPPD-SEGMENT1-1));
	}
	else if(counterPPD <= SEGMENT4){
			fprintf(fp, "finetop = %d\n", ctrl4(counterPPD-SEGMENT2-1));

	}
	else if(counterPPD <= SEGMENT4+DIV6){
		fprintf(fp, "coarsetop = %d\n", ctrl6((counterPPD-SEGMENT4-1) % DIV6));
	}
	else{
		fprintf(fp, "coarsebot = %d\n", ctrl6((counterPPD-SEGMENT4-DIV6-1) % DIV6));
	}
}
