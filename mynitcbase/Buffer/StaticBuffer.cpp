#include "StaticBuffer.h"

unsigned char StaticBuffer::blocks[BUFFER_CAPACITY][BLOCK_SIZE];

struct BufferMetaInfo StaticBuffer::metainfo[BUFFER_CAPACITY];

StaticBuffer::StaticBuffer(){
    for(int i = 0;i < BUFFER_CAPACITY;i++){
        metainfo[i].free = true;
    }
}

StaticBuffer::~StaticBuffer(){}

int StaticBuffer::getBufferNum(int blockNum){
    if(blockNum < 0 || blockNum >= DISK_BLOCKS){
        return E_OUTOFBOUND;
    }
    for(int i = 0;i < BUFFER_CAPACITY;i++){
        if(!metainfo[i].free && metainfo[i].blockNum == blockNum){
            return i;
        }
    }
    return E_BLOCKNOTINBUFFER;
}

int StaticBuffer::getFreeBuffer(int blockNum){
    if(blockNum < 0 || blockNum >= DISK_BLOCKS){
        return E_OUTOFBOUND;
    }
    for(int i = 0;i < BUFFER_CAPACITY;i++){
        if(metainfo[i].free){
            metainfo[i].free = false;
            metainfo[i].blockNum = blockNum;

            return i;
        }
    }
    return E_BLOCKNOTINBUFFER;
}