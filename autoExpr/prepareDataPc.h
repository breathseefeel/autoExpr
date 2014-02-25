#ifndef prepareDataPc_h
#define prepareDataPc_h
#include <stdio.h>
#define DIV0 64
#define DIV1 4
#define DIV2 16
#define DIV4 16
#define DIV6 32


#define SEGMENT0 (DIV0*DIV6)
#define SEGMENT1 (SEGMENT0 + (DIV1*DIV6))
#define SEGMENT2 (SEGMENT1 + DIV2)

#define SEGMENT4 (SEGMENT2 + DIV4)

#define SEGMENT6 (SEGMENT4 + DIV6)


#define LOWERBND0 0
#define UPPERBND0 127
#define LOWERBND1 0
#define UPPERBND1 7
#define LOWERBND2 0
#define UPPERBND2 31

#define LOWERBND4 0
#define UPPERBND4 31

#define LOWERBND6 0
#define UPPERBND6 63


#define ctrl0(n) (LOWERBND0 + (UPPERBND0 - LOWERBND0) / (DIV0 - 1) * (n))//
#define ctrl1(n) (LOWERBND1 + (UPPERBND1 - LOWERBND1) / (DIV1 - 1) * (n))
#define ctrl2(n) (LOWERBND2 + (UPPERBND2 - LOWERBND2) / (DIV2 - 1) * (n))

#define ctrl4(n) (LOWERBND4 + (UPPERBND4 - LOWERBND4) / (DIV4 - 1) * (n))

#define ctrl6(n) (LOWERBND6 + (UPPERBND6 - LOWERBND6) / (DIV6 - 1) * (n))


#define CTRLDATALEN 8//

extern unsigned char default_1CtrlData[CTRLDATALEN];
extern unsigned char default0CtrlData[CTRLDATALEN];
extern unsigned char default1CtrlData[CTRLDATALEN];  //to be init later////////////////////////
extern unsigned char default2CtrlData[CTRLDATALEN]; 

extern unsigned char default4CtrlData[CTRLDATALEN];

extern unsigned char default6CtrlData[CTRLDATALEN];


extern unsigned dataToSend[CTRLDATALEN];
extern char strToSend2[CTRLDATALEN*5];
extern const char *fmt;
extern unsigned char counterPPD;
extern unsigned char defaultDataIsWritten0;
extern unsigned char defaultDataIsWritten1;
extern unsigned char defaultDataIsWritten2;

extern unsigned char defaultDataIsWritten4;

extern unsigned char defaultDataIsWritten6;

void writeFileInfo(FILE *fp);
void resetData();
void setData(const unsigned char *arr);
void getNextData(void);

#endif