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

int BlockAccess::renameRelation(char oldName[ATTR_SIZE],char newName[ATTR_SIZE]){
    RelCacheTable::resetSearchIndex(RELCAT_RELID);

    Attribute newRelationName;
    strcpy(newRelationName.sVal,newName);

    RecId recId = linearSearch(RELCAT_RELID,(char *)RELCAT_ATTR_RELNAME,newRelationName,EQ);
    if(recId.block != -1 && recId.slot != -1){
        return E_RELEXIST;
    }

    RelCacheTable::resetSearchIndex(RELCAT_RELID);

    Attribute oldRelationName;
    strcpy(oldRelationName.sVal,oldName);

    RecId relRecId = linearSearch(RELCAT_RELID,(char *)RELCAT_ATTR_RELNAME,oldRelationName,EQ);

    if(relRecId.block == -1 && relRecId.slot == -1){
        return E_RELNOTEXIST;
    }

    RecBuffer relCatBlock(relRecId.block);

    Attribute record[RELCAT_NO_ATTRS];

    relCatBlock.getRecord(record,relRecId.slot);

    strcpy(record[RELCAT_REL_NAME_INDEX].sVal,newName);

    relCatBlock.setRecord(record,relRecId.slot);

    RelCacheTable::resetSearchIndex(ATTRCAT_RELID);

    RelCatEntry relCatEntry;

    int numAttrs = (int)record[RELCAT_NO_ATTRIBUTES_INDEX].nVal;

    for(int i = 0;i < numAttrs;i++){
        RecId attrRecId = linearSearch(ATTRCAT_RELID,(char *)ATTRCAT_ATTR_RELNAME,oldRelationName,EQ);

        RecBuffer attrBlock(attrRecId.block);

        Attribute attrRecord[ATTRCAT_NO_ATTRS];
        attrBlock.getRecord(attrRecord,attrRecId.slot);

        strcpy(attrRecord[ATTRCAT_REL_NAME_INDEX].sVal,newName);

        attrBlock.setRecord(attrRecord,attrRecId.slot);
    }

    return SUCCESS;
}

int BlockAccess::renameAttribute(char relName[ATTR_SIZE],char oldName[ATTR_SIZE],char newName[ATTR_SIZE]){
    RelCacheTable::resetSearchIndex(RELCAT_RELID);

    Attribute relNameAttr;
    strcpy(relNameAttr.sVal,relName);

    RecId recid = linearSearch(RELCAT_RELID,(char *)RELCAT_ATTR_RELNAME,relNameAttr,EQ);
    if(recid.block == -1 && recid.slot == -1){
        return E_RELNOTEXIST;
    }

    RelCacheTable::resetSearchIndex(ATTRCAT_RELID);

    RecId attrToRenameRecId{-1,-1};
    Attribute attrCatEntryRecord[ATTRCAT_NO_ATTRS];

    while(true){
        RecId attrRecId = linearSearch(ATTRCAT_RELID,(char *)ATTRCAT_ATTR_RELNAME,relNameAttr,EQ);
        if(attrRecId.block == -1 && attrRecId.slot == -1){
            break;
        }

        RecBuffer attrBlock(attrRecId.block);
        attrBlock.getRecord(attrCatEntryRecord,attrRecId.slot);

        if(strcmp(attrCatEntryRecord[ATTRCAT_ATTR_NAME_INDEX].sVal,oldName) == 0){
            attrToRenameRecId.block = attrRecId.block;
            attrToRenameRecId.slot = attrRecId.slot;
        }

        if(strcmp(attrCatEntryRecord[ATTRCAT_ATTR_NAME_INDEX].sVal,newName) == 0){
            return E_ATTREXIST;
        }
    }

    if(attrToRenameRecId.block == -1){
        return E_ATTRNOTEXIST;
    }

    RecBuffer oldAttrBlock(attrToRenameRecId.block);
    oldAttrBlock.getRecord(attrCatEntryRecord,attrToRenameRecId.slot);
    strcpy(attrCatEntryRecord[ATTRCAT_ATTR_NAME_INDEX].sVal,newName);

    oldAttrBlock.setRecord(attrCatEntryRecord,attrToRenameRecId.slot);

    return SUCCESS; 
}

int BlockAccess::insert(int relId, Attribute *record) {

    RelCatEntry relCatEntry;

    int ret = RelCacheTable::getRelCatEntry(relId, &relCatEntry);

    if(ret != SUCCESS){
        return ret;
    }

    int blockNum = relCatEntry.firstBlk;

    RecId rec_id = {-1, -1};

    int numOfSlots = relCatEntry.numSlotsPerBlk;
    int numOfAttributes = relCatEntry.numAttrs;

    int prevBlockNum = -1;

    while(blockNum != -1){

        RecBuffer block(blockNum);

        HeadInfo head;

        ret = block.getHeader(&head);

        if(ret != SUCCESS){
            return ret;
        }

        unsigned char slotMap[numOfSlots];

        ret = block.getSlotMap(slotMap);

        if(ret != SUCCESS){
            return ret;
        }

        int freeSlot = -1;

        for(int i = 0; i < numOfSlots; i++){

            if(slotMap[i] == SLOT_UNOCCUPIED){
                freeSlot = i;
                break;
            }
        }

        if(freeSlot != -1){

            rec_id.block = blockNum;
            rec_id.slot = freeSlot;

            break;
        }

        prevBlockNum = blockNum;
        blockNum = head.rblock;
    }

    if(rec_id.block == -1){

        if(relId == RELCAT_RELID){
            return E_MAXRELATIONS;
        }

        RecBuffer newBlock;

        int newBlockNum = newBlock.getBlockNum();

        if(newBlockNum == E_DISKFULL){
            return E_DISKFULL;
        }

        rec_id.block = newBlockNum;
        rec_id.slot = 0;

        HeadInfo head;

        head.blockType = REC;
        head.pblock = -1;

        if(prevBlockNum == -1){
            head.lblock = -1;
        }
        else{
            head.lblock = prevBlockNum;
        }

        head.rblock = -1;
        head.numEntries = 0;
        head.numSlots = numOfSlots;
        head.numAttrs = numOfAttributes;

        ret = newBlock.setHeader(&head);

        if(ret != SUCCESS){
            return ret;
        }

        unsigned char slotMap[numOfSlots];

        for(int i = 0; i < numOfSlots; i++){
            slotMap[i] = SLOT_UNOCCUPIED;
        }

        ret = newBlock.setSlotMap(slotMap);

        if(ret != SUCCESS){
            return ret;
        }

        if(prevBlockNum != -1){

            RecBuffer prevBlock(prevBlockNum);

            HeadInfo prevHead;

            ret = prevBlock.getHeader(&prevHead);

            if(ret != SUCCESS){
                return ret;
            }

            prevHead.rblock = rec_id.block;

            ret = prevBlock.setHeader(&prevHead);

            if(ret != SUCCESS){
                return ret;
            }
        }
        else{

            relCatEntry.firstBlk = rec_id.block;

            ret = RelCacheTable::setRelCatEntry(relId, &relCatEntry);

            if(ret != SUCCESS){
                return ret;
            }
        }

        relCatEntry.lastBlk = rec_id.block;

        ret = RelCacheTable::setRelCatEntry(relId, &relCatEntry);

        if(ret != SUCCESS){
            return ret;
        }
    }

    RecBuffer block(rec_id.block);

    ret = block.setRecord(record, rec_id.slot);

    if(ret != SUCCESS){
        return ret;
    }

    unsigned char slotMap[numOfSlots];

    ret = block.getSlotMap(slotMap);

    if(ret != SUCCESS){
        return ret;
    }

    slotMap[rec_id.slot] = SLOT_OCCUPIED;

    ret = block.setSlotMap(slotMap);

    if(ret != SUCCESS){
        return ret;
    }

    HeadInfo head;

    ret = block.getHeader(&head);

    if(ret != SUCCESS){
        return ret;
    }

    head.numEntries++;

    ret = block.setHeader(&head);

    if(ret != SUCCESS){
        return ret;
    }

    relCatEntry.numRecs++;

    ret = RelCacheTable::setRelCatEntry(relId, &relCatEntry);

    if(ret != SUCCESS){
        return ret;
    }

    return SUCCESS;
}