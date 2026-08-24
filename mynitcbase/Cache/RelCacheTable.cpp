#include "RelCacheTable.h"

#include <cstring>

RelCacheEntry* RelCacheTable::relCache[MAX_OPEN];

void RelCacheTable::recordToRelCatEntry(union Attribute record[RELCAT_NO_ATTRS],RelCatEntry* relcatentry){
    
    strcpy(relcatentry->relName, record[RELCAT_REL_NAME_INDEX].sVal);
    relcatentry->numAttrs = (int)record[RELCAT_NO_ATTRIBUTES_INDEX].nVal;
    relcatentry->firstBlk = (int)record[RELCAT_FIRST_BLOCK_INDEX].nVal;
    relcatentry->lastBlk = (int)record[RELCAT_LAST_BLOCK_INDEX].nVal;
    relcatentry->numRecs = (int)record[RELCAT_NO_RECORDS_INDEX].nVal;
    relcatentry->numSlotsPerBlk = (int)record[RELCAT_NO_SLOTS_PER_BLOCK_INDEX].nVal;

}

int RelCacheTable::getRelCatEntry(int relId,RelCatEntry* relcatentry){
    
    if(relId < 0 || relId >= MAX_OPEN){
        return E_OUTOFBOUND;
    }

    if(relCache[relId] == nullptr){
        return E_RELNOTOPEN;
    }

    *relcatentry = relCache[relId]->relCatEntry;

    return SUCCESS;
}

int RelCacheTable::getSearchIndex(int relId, RecId* searchIndex){
    
    if(relId < 0 || relId >= MAX_OPEN){
        return E_OUTOFBOUND;
    }
    
    if(relCache[relId] == nullptr){
        return E_RELNOTOPEN;
    }
    
    *searchIndex = relCache[relId]->searchIndex;
    
    return SUCCESS;
}

int RelCacheTable::setSearchIndex(int relId, RecId* searchIndex){
    
    if(relId < 0 || relId >= MAX_OPEN){
        return E_OUTOFBOUND;
    }

    if(relCache[relId] == nullptr){
        return E_RELNOTOPEN;
    }
    
    relCache[relId]->searchIndex = *searchIndex;
    
    return SUCCESS;
}

int RelCacheTable::resetSearchIndex(int relId){
    
    relCache[relId]->searchIndex = {-1,-1};
    
    return SUCCESS;
}