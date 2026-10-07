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


int BlockAccess::search(int relId, Attribute *record,char attrName[ATTR_SIZE],Attribute attrVal, int op) {

    RecId recId;

    recId = linearSearch(relId,attrName,attrVal,op);

    if(recId.block == -1 && recId.slot == -1) {
        return E_NOTFOUND;
    }

    RecBuffer recBuffer(recId.block);

    int ret = recBuffer.getRecord(record, recId.slot);

    if(ret != SUCCESS) {
        return ret;
    }

    return SUCCESS;
}

int BlockAccess::deleteRelation(char relName[ATTR_SIZE]) {
    // Check catalog relations
    if (strcmp(relName, RELCAT_RELNAME) == 0 || strcmp(relName, ATTRCAT_RELNAME) == 0) {
        return E_NOTPERMITTED;
    }

    // Reset relation catalog search
    int ret = RelCacheTable::resetSearchIndex(RELCAT_RELID);
    if (ret != SUCCESS)
        return ret;

    Attribute relNameAttr;
    strcpy(relNameAttr.sVal, relName);

    // Find relation
    RecId relCatRecId = linearSearch(RELCAT_RELID,(char *)RELCAT_ATTR_RELNAME,relNameAttr,EQ);

    if (relCatRecId.block == -1 && relCatRecId.slot == -1)
        return E_RELNOTEXIST;

    Attribute relCatEntryRecord[RELCAT_NO_ATTRS];
    RecBuffer relCatBlock(relCatRecId.block);

    ret = relCatBlock.getRecord(relCatEntryRecord, relCatRecId.slot);
    if (ret != SUCCESS)
        return ret;

    int firstBlock = (int)relCatEntryRecord[RELCAT_FIRST_BLOCK_INDEX].nVal;
    int numAttrs = (int)relCatEntryRecord[RELCAT_NO_ATTRIBUTES_INDEX].nVal;

    // Delete record blocks
    int currentBlock = firstBlock;
    while (currentBlock != -1) {
        BlockBuffer blockBuffer(currentBlock);
        HeadInfo head;

        ret = blockBuffer.getHeader(&head);
        if (ret != SUCCESS)
            return ret;

        int nextBlock = head.rblock;
        blockBuffer.releaseBlock();
        currentBlock = nextBlock;
    }

    // Reset attribute catalog search
    ret = RelCacheTable::resetSearchIndex(ATTRCAT_RELID);
    if (ret != SUCCESS)
        return ret;

    int numberOfAttributesDeleted = 0;

    while (true) {
        RecId attrCatRecId = linearSearch(
            ATTRCAT_RELID,
            (char *)ATTRCAT_ATTR_RELNAME,
            relNameAttr,
            EQ
        );

        if (attrCatRecId.block == -1 && attrCatRecId.slot == -1)
            break;

        numberOfAttributesDeleted++;

        RecBuffer attrCatBlock(attrCatRecId.block);
        HeadInfo header;

        ret = attrCatBlock.getHeader(&header);
        if (ret != SUCCESS)
            return ret;

        Attribute attrCatRecord[ATTRCAT_NO_ATTRS];
        ret = attrCatBlock.getRecord(attrCatRecord, attrCatRecId.slot);
        if (ret != SUCCESS)
            return ret;

        int rootBlock = (int)attrCatRecord[ATTRCAT_ROOT_BLOCK_INDEX].nVal;

        // Free catalog slot
        unsigned char slotMap[header.numSlots];

        ret = attrCatBlock.getSlotMap(slotMap);
        if (ret != SUCCESS)
            return ret;

        slotMap[attrCatRecId.slot] = SLOT_UNOCCUPIED;

        ret = attrCatBlock.setSlotMap(slotMap);
        if (ret != SUCCESS)
            return ret;

        header.numEntries--;

        ret = attrCatBlock.setHeader(&header);
        if (ret != SUCCESS)
            return ret;

        // Remove empty block
        if (header.numEntries == 0) {
            int leftBlock = header.lblock;
            int rightBlock = header.rblock;

            if (leftBlock != -1) {
                RecBuffer leftBlockBuffer(leftBlock);
                HeadInfo leftHeader;

                ret = leftBlockBuffer.getHeader(&leftHeader);
                if (ret != SUCCESS)
                    return ret;

                leftHeader.rblock = rightBlock;

                ret = leftBlockBuffer.setHeader(&leftHeader);
                if (ret != SUCCESS)
                    return ret;
            }

            if (rightBlock != -1) {
                RecBuffer rightBlockBuffer(rightBlock);
                HeadInfo rightHeader;

                ret = rightBlockBuffer.getHeader(&rightHeader);
                if (ret != SUCCESS)
                    return ret;

                rightHeader.lblock = leftBlock;

                ret = rightBlockBuffer.setHeader(&rightHeader);
                if (ret != SUCCESS)
                    return ret;
            }
            else {
                RelCatEntry attrCatRelEntry;

                ret = RelCacheTable::getRelCatEntry(
                    ATTRCAT_RELID,
                    &attrCatRelEntry
                );
                if (ret != SUCCESS)
                    return ret;

                attrCatRelEntry.lastBlk = leftBlock;

                ret = RelCacheTable::setRelCatEntry(
                    ATTRCAT_RELID,
                    &attrCatRelEntry
                );
                if (ret != SUCCESS)
                    return ret;
            }

            attrCatBlock.releaseBlock();
        }

        // Indexing not implemented
        if (rootBlock != -1) {
            // BPlusTree::bPlusDestroy(rootBlock);
        }
    }

    // Delete relation catalog entry
    HeadInfo relCatHeader;

    ret = relCatBlock.getHeader(&relCatHeader);
    if (ret != SUCCESS)
        return ret;

    relCatHeader.numEntries--;

    ret = relCatBlock.setHeader(&relCatHeader);
    if (ret != SUCCESS)
        return ret;

    unsigned char relCatSlotMap[relCatHeader.numSlots];

    ret = relCatBlock.getSlotMap(relCatSlotMap);
    if (ret != SUCCESS)
        return ret;

    relCatSlotMap[relCatRecId.slot] = SLOT_UNOCCUPIED;

    ret = relCatBlock.setSlotMap(relCatSlotMap);
    if (ret != SUCCESS)
        return ret;

    // Update relation catalog cache
    RelCatEntry relCatEntry;

    ret = RelCacheTable::getRelCatEntry(
        RELCAT_RELID,
        &relCatEntry
    );
    if (ret != SUCCESS)
        return ret;

    relCatEntry.numRecs--;

    ret = RelCacheTable::setRelCatEntry(
        RELCAT_RELID,
        &relCatEntry
    );
    if (ret != SUCCESS)
        return ret;

    // Update attribute catalog cache
    RelCatEntry attrCatEntry;

    ret = RelCacheTable::getRelCatEntry(
        ATTRCAT_RELID,
        &attrCatEntry
    );
    if (ret != SUCCESS)
        return ret;

    attrCatEntry.numRecs -= numberOfAttributesDeleted;

    ret = RelCacheTable::setRelCatEntry(
        ATTRCAT_RELID,
        &attrCatEntry
    );
    if (ret != SUCCESS)
        return ret;

    return SUCCESS;
}