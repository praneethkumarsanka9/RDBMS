#include "BlockBuffer.h"

#include <cstdlib>
#include <cstring>

BlockBuffer::BlockBuffer(int blockNum){
    this->blockNum = blockNum;
}

RecBuffer::RecBuffer(int blockNum):BlockBuffer::BlockBuffer(blockNum){}

int BlockBuffer::loadBlockAndGetBufferPtr(unsigned char **bufferptr){

    int bufferNum = StaticBuffer::getBufferNum(this->blockNum);

    if(bufferNum == E_BLOCKNOTINBUFFER){
        bufferNum = StaticBuffer::getFreeBuffer(this->blockNum);
        
        if(bufferNum == E_OUTOFBOUND){
            return E_OUTOFBOUND;
        } 

        Disk::readBlock(StaticBuffer::blocks[bufferNum],this->blockNum);
    }

    *bufferptr = StaticBuffer::blocks[bufferNum];
    
    return SUCCESS;
}

int BlockBuffer::getHeader(struct HeadInfo *head){
    
    unsigned char *bufferptr;
    int ret = loadBlockAndGetBufferPtr(&bufferptr);
    if(ret != SUCCESS){
        return ret;
    }

    memcpy(&head->numSlots,bufferptr+24,4);
    memcpy(&head->numEntries,bufferptr+16,4);
    memcpy(&head->numAttrs,bufferptr+20,4);
    memcpy(&head->rblock,bufferptr+12,4); 
    memcpy(&head->lblock,bufferptr+8,4);

    return SUCCESS;
}

int RecBuffer::getRecord(union Attribute *rec,int slotNum){
    struct HeadInfo head;
    this->getHeader(&head);
    int attrCount = head.numAttrs;
    int slotCount = head.numSlots; 

    unsigned char *bufferptr;
    int ret = loadBlockAndGetBufferPtr(&bufferptr);

    if(ret != SUCCESS){
        return ret;
    }

    int offset = 32 + head.numSlots + ((ATTR_SIZE * attrCount) * slotNum);
    
    memcpy(rec,bufferptr+offset,ATTR_SIZE * attrCount);

    return SUCCESS;
}


int RecBuffer::getSlotMap(unsigned char *slotMap){
    unsigned char *bufferPtr;

    int ret = loadBlockAndGetBufferPtr(&bufferPtr);
    if(ret != SUCCESS){
        return ret;
    }
    struct HeadInfo head;
    ret = getHeader(&head);
    if(ret != SUCCESS){
        return ret;
    }

    int slotCounts = head.numSlots;

    unsigned char *slotMapInBuffer = bufferPtr + HEADER_SIZE;

    memcpy(slotMap,slotMapInBuffer,slotCounts);
    
    return SUCCESS;
}

int compareAttrs(Attribute attr1, Attribute attr2, int attrType) {

    if(attrType == NUMBER){

        if(attr1.nVal < attr2.nVal)
            return -1;

        if(attr1.nVal > attr2.nVal)
            return 1;

        return 0;
    }

    return strcmp(attr1.sVal, attr2.sVal);
}
/*int RecBuffer::setRecord(union Attribute *rec,int slotNum){
    struct HeadInfo head;
    this->getHeader(&head);
    int attrCount = head.numAttrs;
    int slotCount = head.numSlots;

    unsigned char *bufferptr;
    int ret = loadBlockAndGetBufferPtr(&bufferptr);

    if(ret != SUCCESS){
        return ret;
    }

    int offset = 32 + slotCount + ((ATTR_SIZE * attrCount) * slotNum);

    memcpy(bufferptr + offset,rec,ATTR_SIZE * attrCount);

    Disk::writeBlock(bufferptr,this->blockNum);

    return SUCCESS;
}*/