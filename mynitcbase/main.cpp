#include "Buffer/StaticBuffer.h"
#include "Cache/OpenRelTable.h"
#include "Disk_Class/Disk.h"
#include "FrontendInterface/FrontendInterface.h"
#include "cstring"
#include <iostream>

int main(int argc, char *argv[]) {
  /* Initialize the Run Copy of Disk */ 
  Disk disk_run;

  RecBuffer relcatbuffer(RELCAT_BLOCK);
  RecBuffer attcatbuffer(ATTRCAT_BLOCK);

  HeadInfo relcatHead;
  HeadInfo attcatHead;

  relcatbuffer.getHeader(&relcatHead);
  attcatbuffer.getHeader(&attcatHead);

  for(int i = 0;i < relcatHead.numEntries;i++){
    Attribute relcatRecord[RELCAT_NO_ATTRS];
    relcatbuffer.getRecord(relcatRecord,i);

    printf("Relation : %s\n",relcatRecord[RELCAT_REL_NAME_INDEX].sVal,relcatRecord[RELCAT_REL_NAME_INDEX].sVal);
 
    RecBuffer tempBuffer = attcatbuffer;
    int currentBlock = ATTRCAT_BLOCK;

    while(currentBlock != -1){
      tempBuffer = RecBuffer(currentBlock);  

      HeadInfo temp;
      tempBuffer.getHeader(&temp);

      for(int j = 0;j < temp.numEntries;j++){

        Attribute attcatRecord[ATTRCAT_NO_ATTRS];
        tempBuffer.getRecord(attcatRecord,j);

        if(strcmp(relcatRecord[RELCAT_REL_NAME_INDEX].sVal,attcatRecord[ATTRCAT_REL_NAME_INDEX].sVal) == 0){

          const char* atttype = (attcatRecord[ATTRCAT_ATTR_TYPE_INDEX].nVal == 0.0) ? "NUM" : "STR";

          printf(" %s : %s\n",attcatRecord[ATTRCAT_ATTR_NAME_INDEX].sVal,atttype);

        }

      }

      currentBlock = temp.rblock;
    }

    printf("\n");
  }

  /*int currentBlock = ATTRCAT_BLOCK;
  RecBuffer tempBuffer = attcatbuffer;

  while(currentBlock != -1){

    tempBuffer = RecBuffer(currentBlock);
    HeadInfo temp;
    tempBuffer.getHeader(&temp);

    for(int j = 0;j < temp.numEntries;j++){

      Attribute attcatRecord[ATTRCAT_NO_ATTRS];
      tempBuffer.getRecord(attcatRecord,j);

      if(strcmp(attcatRecord[ATTRCAT_REL_NAME_INDEX].sVal,"Students") == 0){

        if(strcmp(attcatRecord[ATTRCAT_ATTR_NAME_INDEX].sVal,"Class") == 0){

          strcpy(attcatRecord[ATTRCAT_ATTR_NAME_INDEX].sVal,"Batch");
          tempBuffer.setRecord(attcatRecord,j);

          return 0;
        }

      }

    }
    currentBlock = temp.rblock;

  }*/

  //return FrontendInterface::handleFrontend(argc, argv);
}