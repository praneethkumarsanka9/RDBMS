#include "Buffer/StaticBuffer.h"
#include "Cache/OpenRelTable.h"
#include "Disk_Class/Disk.h"
#include "FrontendInterface/FrontendInterface.h"
#include "cstring"
#include <iostream>

int main(int argc, char *argv[]) {
  /* Initialize the Run Copy of Disk */ 
  Disk disk_run;
  StaticBuffer buffer;
  OpenRelTable Cache;

  return FrontendInterface::handleFrontend(argc,argv);
}