#include "BlockBuffer.h"

#include <cstdlib>
#include <cstring>

BlockBuffer::BlockBuffer(int blockNum){
    this->blockNum = blockNum;
}

RecBuffer::RecBuffer(int blockNum):BlockBuffer::BlockBuffer(blockNum){}

int BlockBuffer::getHeader(struct HeadInfo *head){
    unsigned char buffer[BLOCK_SIZE];

    Disk::readBlock(buffer,this->blockNum);

    memcpy(&head->numSlots,buffer+24,4);
    memcpy(&head->numEntries,buffer+16,4);
    memcpy(&head->numAttrs,buffer+20,4);
    memcpy(&head->rblock,buffer+12,4); 
    memcpy(&head->lblock,buffer+8,4);

    return SUCCESS;
}

int RecBuffer::getRecord(union Attribute *rec,int slotNum){
    struct HeadInfo head;
    this->getHeader(&head);
    int attrCount = head.numAttrs;
    int slotCount = head.numSlots; 

    unsigned char buffer[BLOCK_SIZE];

    Disk::readBlock(buffer,this->blockNum);

    int offset = 32 + head.numSlots + ((ATTR_SIZE * attrCount) * slotNum);
    
    memcpy(rec,buffer+offset,ATTR_SIZE * attrCount);

    return SUCCESS;
}

int RecBuffer::setRecord(union Attribute *rec,int slotNum){
    struct HeadInfo head;
    this->getHeader(&head);
    int attrCount = head.numAttrs;
    int slotCount = head.numSlots;

    unsigned char buffer[BLOCK_SIZE];

    Disk::readBlock(buffer,this->blockNum);

    int offset = 32 + slotCount + ((ATTR_SIZE * attrCount) * slotNum);

    memcpy(buffer + offset,rec,ATTR_SIZE * attrCount);

    Disk::writeBlock(buffer,this->blockNum);

    return SUCCESS;
}