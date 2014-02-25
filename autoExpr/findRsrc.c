#include "findRsrc.h"
#include "visaUtils.h"

#include <stdio.h>

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