#include "BlockBuffer.h"

#include <cstdlib>
#include <cstring>

BlockBuffer::BlockBuffer(char blockType) {
    int type;

    if(blockType == 'R')
        type = REC;
    else if(blockType == 'I')
        type = IND_INTERNAL;
    else
        type = IND_LEAF;

    this->blockNum = getFreeBlock(type);
}

BlockBuffer::BlockBuffer(int blockNum){
    this->blockNum = blockNum;
}

int BlockBuffer::getBlockNum(){
    return this->blockNum;
}

RecBuffer::RecBuffer():BlockBuffer('R'){}

RecBuffer::RecBuffer(int blockNum):BlockBuffer::BlockBuffer(blockNum){}

int BlockBuffer::loadBlockAndGetBufferPtr(unsigned char **bufferptr){

    int bufferNum = StaticBuffer::getBufferNum(this->blockNum);

    if(bufferNum != E_BLOCKNOTINBUFFER){
        for(int i = 0; i < BUFFER_CAPACITY; i++){
            if(!StaticBuffer::metainfo[i].free){
                if(i == bufferNum){
                    StaticBuffer::metainfo[i].timeStamp = 0;
                }
                else{
                    StaticBuffer::metainfo[i].timeStamp++;
                }
            }
        }
    }else{
        bufferNum = StaticBuffer::getFreeBuffer(this->blockNum);

        if(bufferNum == E_OUTOFBOUND){
            return E_OUTOFBOUND;
        }

        Disk::readBlock(StaticBuffer::blocks[bufferNum],this->blockNum);
    }

    *bufferptr = StaticBuffer::blocks[bufferNum];
    
    return SUCCESS;
}

int BlockBuffer::setHeader(struct HeadInfo *head){
    unsigned char *bufferPtr;

    int ret = loadBlockAndGetBufferPtr(&bufferPtr);

    if(ret != SUCCESS){
        return ret;
    }

    struct HeadInfo *bufferHeader = (struct HeadInfo *)bufferPtr;

    bufferHeader->pblock = head->pblock;
    bufferHeader->lblock = head->lblock;
    bufferHeader->rblock = head->rblock;
    bufferHeader->numEntries = head->numEntries;
    bufferHeader->numAttrs = head->numAttrs;
    bufferHeader->numSlots = head->numSlots;

    ret = StaticBuffer::setDirtyBit(this->blockNum);

    if(ret != SUCCESS){
        return ret;
    }

    return SUCCESS;
}

int BlockBuffer::getHeader(struct HeadInfo *head){
    
    unsigned char *bufferptr;
    int ret = loadBlockAndGetBufferPtr(&bufferptr);
    if(ret != SUCCESS){
        return ret;
    }

    memcpy(&head->blockType,  bufferptr + 0,  4);
    memcpy(&head->pblock,     bufferptr + 4,  4);
    memcpy(&head->lblock,     bufferptr + 8,  4);
    memcpy(&head->rblock,     bufferptr + 12, 4);
    memcpy(&head->numEntries, bufferptr + 16, 4);
    memcpy(&head->numAttrs,   bufferptr + 20, 4);
    memcpy(&head->numSlots,   bufferptr + 24, 4);

    return SUCCESS;
}

int RecBuffer::getRecord(union Attribute *rec,int slotNum){
    struct HeadInfo head;
    int ret = getHeader(&head);
    int attrCount = head.numAttrs;
    int slotCount = head.numSlots; 

    if(slotNum < 0 || slotNum >= slotCount){
        return E_OUTOFBOUND;
    }

    unsigned char *bufferptr;
    ret = loadBlockAndGetBufferPtr(&bufferptr);

    if(ret != SUCCESS){
        return ret;
    }

    int offset = HEADER_SIZE + head.numSlots + ((ATTR_SIZE * attrCount) * slotNum);
    
    memcpy(rec,bufferptr+offset,ATTR_SIZE * attrCount);

    return SUCCESS;
}

int RecBuffer::setSlotMap(unsigned char *slotMap) {

    unsigned char *bufferPtr;

    int ret = loadBlockAndGetBufferPtr(&bufferPtr);

    if(ret != SUCCESS){
        return ret;
    }

    HeadInfo head;

    ret = getHeader(&head);

    if(ret != SUCCESS){
        return ret;
    }

    int numSlots = head.numSlots;

    unsigned char *slotMapInBuffer = bufferPtr + HEADER_SIZE;

    memcpy(slotMapInBuffer, slotMap, numSlots);

    ret = StaticBuffer::setDirtyBit(this->blockNum);

    if(ret != SUCCESS){
        return ret;
    }

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
int RecBuffer::setRecord(union Attribute *rec,int slotNum){
    unsigned char *bufferPtr;
    int ret = loadBlockAndGetBufferPtr(&bufferPtr);
    if(ret != SUCCESS){
        return ret;
    }

    HeadInfo head;
    ret = getHeader(&head);

    if(ret != SUCCESS){
        return ret;
    }

    int numAttrs = head.numAttrs;
    int numSlots = head.numSlots;

    if(slotNum < 0 || slotNum >= numSlots){
        return E_OUTOFBOUND;
    }

    int recordSize = ATTR_SIZE * numAttrs;

    int offset = HEADER_SIZE + numSlots + slotNum * recordSize;

    memcpy(bufferPtr + offset,rec,recordSize);

    ret = StaticBuffer::setDirtyBit(this->blockNum);

    if(ret != SUCCESS){
        return ret;
    }

    return SUCCESS;
}

int BlockBuffer::setBlockType(int blockType){
    unsigned char *bufferPtr;

    int ret = loadBlockAndGetBufferPtr(&bufferPtr);

    if(ret != SUCCESS){
        return ret;
    }

    *((int32_t *)bufferPtr) = blockType;

    StaticBuffer::blockAllocMap[this->blockNum] = blockType;

    ret = StaticBuffer::setDirtyBit(this->blockNum);
    if(ret != SUCCESS){
        return ret;
    }

    return SUCCESS;
}

int BlockBuffer::getFreeBlock(int blockType){
    int freeBlock = -1;

    for(int i = 4;i < DISK_BLOCKS;i++){
        if(StaticBuffer::blockAllocMap[i] == UNUSED_BLK){
            freeBlock = i;
            break;
        }
    }

    if(freeBlock == -1){
        return E_DISKFULL;
    }

    this->blockNum = freeBlock;

    int bufferNum = StaticBuffer::getFreeBuffer(this->blockNum);

    if(bufferNum == E_OUTOFBOUND){
        return E_OUTOFBOUND;
    }

    HeadInfo head;

    head.pblock = -1;
    head.lblock = -1;
    head.rblock = -1;
    head.numEntries = 0;
    head.numAttrs = 0;
    head.numSlots = 0;

    int ret = setHeader(&head);

    if(ret != SUCCESS){
        return ret;
    }

    ret = setBlockType(blockType);

    if(ret != SUCCESS){
        return ret;
    }

    return this->blockNum;
}