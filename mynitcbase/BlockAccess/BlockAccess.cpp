#include "BlockAccess.h"

#include <cstring>

RecId BlockAccess::linearSearch(int relId,char attrName[ATTR_SIZE],union Attribute attrVal,int op){
    RecId prevRecId;

    int ret = RelCacheTable::getSearchIndex(relId,&prevRecId);

    if(ret != SUCCESS){
        return {-1,-1};
    }

    int block,slot;

    if(prevRecId.block == -1 && prevRecId.slot == -1){
        RelCatEntry relCatEntry;

        ret = RelCacheTable::getRelCatEntry(relId, &relCatEntry);
        if(ret != SUCCESS){
            return {-1,-1};
        }

        block = relCatEntry.firstBlk;
        slot = 0;
    }else{
        block = prevRecId.block;
        slot = prevRecId.slot + 1;
    }

    while(block != -1){
        RecBuffer recBuffer(block);
        struct HeadInfo head;
        ret = recBuffer.getHeader(&head);

        if(ret != SUCCESS){
            return {-1,-1};
        }

        unsigned char slotMap[head.numSlots];
        recBuffer.getSlotMap(slotMap);

        if(slot >= head.numSlots){
            block = head.rblock;
            slot = 0;
            continue;
        }

        if(slotMap[slot] == SLOT_UNOCCUPIED){
            slot++;
            continue;
        }

        Attribute record[head.numAttrs];
        recBuffer.getRecord(record,slot);

        AttrCatEntry attrCatEntry;

        AttrCacheTable::getAttrCatEntry(relId,attrName,&attrCatEntry);

        int offset = attrCatEntry.offset;

        Attribute recordAttr = record[offset];

        int cmpVal = compareAttrs(recordAttr,attrVal,attrCatEntry.attrType);

        if (
            (op == NE && cmpVal != 0) ||    // if op is "not equal to"
            (op == LT && cmpVal < 0) ||     // if op is "less than"
            (op == LE && cmpVal <= 0) ||    // if op is "less than or equal to"
            (op == EQ && cmpVal == 0) ||    // if op is "equal to"
            (op == GT && cmpVal > 0) ||     // if op is "greater than"
            (op == GE && cmpVal >= 0)       // if op is "greater than or equal to"
        ) {
             RecId foundRecId;
             foundRecId.block = block;
             foundRecId.slot = slot;

             RelCacheTable::setSearchIndex(relId,&foundRecId);

            return foundRecId;
        }
        slot++;
    }

    return {-1,-1};
}