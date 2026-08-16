#include "AttrCacheTable.h"

#include <cstring>

AttrCacheEntry* AttrCacheTable::attrCache[MAX_OPEN];

void AttrCacheTable::recordToAttrCatEntry(union Attribute record[ATTRCAT_NO_ATTRS],AttrCatEntry* attrcatentry){

    strcpy(attrcatentry->relName,record[ATTRCAT_REL_NAME_INDEX].sVal);
    strcpy(attrcatentry->attrName,record[ATTRCAT_ATTR_NAME_INDEX].sVal);
    attrcatentry->attrType = (int)record[ATTRCAT_ATTR_TYPE_INDEX].nVal;
    attrcatentry->rootBlock = (int)record[ATTRCAT_ROOT_BLOCK_INDEX].nVal;
    attrcatentry->primaryFlag = (bool)record[ATTRCAT_PRIMARY_FLAG_INDEX].nVal;
    attrcatentry->offset = (int)record[ATTRCAT_OFFSET_INDEX].nVal;

}

int AttrCacheTable::getAttrCatEntry(int relId, int attrOffset, AttrCatEntry* attrCacheBuf){

    if(relId < 0 || relId >= MAX_OPEN){
        return E_OUTOFBOUND;
    }

    if(attrCache[relId] == nullptr){
        return E_RELNOTOPEN;
    }

    for(AttrCacheEntry* i = attrCache[relId];i != nullptr;i = i->next){

        if(i->attrCatEntry.offset == attrOffset){

            *attrCacheBuf = i->attrCatEntry;
            return SUCCESS;

        }
    }

    return E_ATTRNOTEXIST;

}