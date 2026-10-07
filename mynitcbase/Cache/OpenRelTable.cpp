#include "OpenRelTable.h"
#include <cstdlib>
#include <cstring>

OpenRelTableMetaInfo OpenRelTable::tableMetaInfo[MAX_OPEN];

OpenRelTable::OpenRelTable(){
    for(int i = 0;i < MAX_OPEN;i++){
        RelCacheTable::relCache[i] = nullptr;
        AttrCacheTable::attrCache[i] = nullptr;

        tableMetaInfo[i].free = true;
    }

    RecBuffer relCatBlock(RELCAT_BLOCK);
    Attribute relCatRecord[RELCAT_NO_ATTRS];

    relCatBlock.getRecord(relCatRecord,RELCAT_SLOTNUM_FOR_RELCAT);

    RelCacheEntry relCacheEntry;

    RelCacheTable::recordToRelCatEntry(relCatRecord,&relCacheEntry.relCatEntry);

    relCacheEntry.recId.block = RELCAT_BLOCK;
    relCacheEntry.recId.slot = RELCAT_SLOTNUM_FOR_RELCAT;

    RelCacheTable::relCache[RELCAT_RELID] = (struct RelCacheEntry*)malloc(sizeof(RelCacheEntry));
    *(RelCacheTable::relCache[RELCAT_RELID]) = relCacheEntry;

    relCatBlock.getRecord(relCatRecord,RELCAT_SLOTNUM_FOR_ATTRCAT);

    RelCacheTable::recordToRelCatEntry(relCatRecord,&relCacheEntry.relCatEntry);

    relCacheEntry.recId.block = RELCAT_BLOCK;
    relCacheEntry.recId.slot = RELCAT_SLOTNUM_FOR_ATTRCAT;

    RelCacheTable::relCache[ATTRCAT_RELID] = (RelCacheEntry*)malloc(sizeof(RelCacheEntry));
    *(RelCacheTable::relCache[ATTRCAT_RELID]) = relCacheEntry;

    RecBuffer attrCatBlock(ATTRCAT_BLOCK);
    Attribute attrCatRecord[ATTRCAT_NO_ATTRS];

    AttrCacheEntry *head = nullptr;
    AttrCacheEntry *tail = nullptr;

    for(int i = 0;i < RELCAT_NO_ATTRS;i++){
        attrCatBlock.getRecord(attrCatRecord,i);

        AttrCacheEntry *node = (AttrCacheEntry*)malloc(sizeof(AttrCacheEntry));
        AttrCacheTable::recordToAttrCatEntry(attrCatRecord,&node->attrCatEntry);

        node->dirty = false;
        node->recId.block = ATTRCAT_BLOCK;
        node->recId.slot = i;

        node->next = nullptr;

        if(head == nullptr){
            head = node;
            tail = head;
        }else{
            tail->next = node;
            tail = node;
        }
    }

    AttrCacheTable::attrCache[RELCAT_RELID] = head;

    head = nullptr;
    tail = nullptr;

    for(int i = ATTRCAT_NO_ATTRS;i < ATTRCAT_NO_ATTRS + RELCAT_NO_ATTRS;i++){
        attrCatBlock.getRecord(attrCatRecord,i);

        AttrCacheEntry *node = (AttrCacheEntry*)malloc(sizeof(AttrCacheEntry));
        AttrCacheTable::recordToAttrCatEntry(attrCatRecord,&node->attrCatEntry);

        node->dirty = false;
        node->recId.block = ATTRCAT_BLOCK;
        node->recId.slot = i;

        node->next = nullptr;

        if(head == nullptr){
            head = node;
            tail = head;
        }else{
            tail->next = node;
            tail = node;
        }
    }
    
    AttrCacheTable::attrCache[ATTRCAT_RELID] = head;

    

    OpenRelTable::tableMetaInfo[RELCAT_RELID].free = false;
    strcpy(OpenRelTable::tableMetaInfo[RELCAT_RELID].relName,RELCAT_RELNAME);

    OpenRelTable::tableMetaInfo[ATTRCAT_RELID].free = false;
    strcpy(OpenRelTable::tableMetaInfo[ATTRCAT_RELID].relName,ATTRCAT_RELNAME);

}

int OpenRelTable::openRel(char relName[ATTR_SIZE]){
    int relId = getRelId(relName);
    if(relId >= 0){
        return relId;
    }

    relId = getFreeOpenRelTableEntry();
 
    if(relId < 0){
        return relId;
    }

    Attribute relNameAttr;
    strcpy(relNameAttr.sVal,relName);

    RelCacheTable::resetSearchIndex(RELCAT_RELID);

    RecId relRecId = BlockAccess::linearSearch(RELCAT_RELID,(char *)RELCAT_ATTR_RELNAME,relNameAttr,EQ);

    if(relRecId.block == -1 && relRecId.slot == -1){
        return E_RELNOTEXIST;
    }

    RecBuffer relBlock(relRecId.block);

    Attribute record[RELCAT_NO_ATTRS];
    relBlock.getRecord(record,relRecId.slot);

    RelCatEntry relCatEntry;
    RelCacheTable::recordToRelCatEntry(record,&relCatEntry);

    RelCacheEntry *relCacheEntry = (RelCacheEntry *)malloc(sizeof(RelCacheEntry));

    relCacheEntry->relCatEntry = relCatEntry;
    relCacheEntry->dirty = false;
    relCacheEntry->recId = relRecId;

    relCacheEntry->searchIndex.block = -1;
    relCacheEntry->searchIndex.slot = -1;

    RelCacheTable::relCache[relId] = relCacheEntry;

    RelCacheTable::resetSearchIndex(ATTRCAT_RELID);

    Attribute attrSearch;
    strcpy(attrSearch.sVal,relName);

    AttrCacheEntry *head = nullptr;
    AttrCacheEntry *tail = nullptr;

    while(true){
        RecId attrRecId = BlockAccess::linearSearch(ATTRCAT_RELID,(char *)ATTRCAT_ATTR_RELNAME,attrSearch,EQ);

        if(attrRecId.block == -1 && attrRecId.slot == -1){
            break;
        }

        RecBuffer attrBlock(attrRecId.block);

        Attribute attrRecord[ATTRCAT_NO_ATTRS];

        attrBlock.getRecord(attrRecord,attrRecId.slot);

        AttrCatEntry attrCatEntry;

        AttrCacheTable::recordToAttrCatEntry(attrRecord,&attrCatEntry);

        AttrCacheEntry* node = (AttrCacheEntry*)malloc(sizeof(AttrCacheEntry));
        node->attrCatEntry = attrCatEntry;
        node->dirty = false;
        node->recId = attrRecId;
        node->next = nullptr;

        if(head == nullptr){
            head = node;
            tail = node;
        }else{
            tail->next = node;
            tail = node;
        }
    }

    AttrCacheTable::attrCache[relId] = head;

    tableMetaInfo[relId].free = false;
    strcpy(tableMetaInfo[relId].relName,relName);

    return relId;

    }

OpenRelTable::~OpenRelTable(){
    for(int i = 2;i < MAX_OPEN;i++){
        if(!tableMetaInfo[i].free){
            OpenRelTable::closeRel(i);
        }
    }

    if(RelCacheTable::relCache[ATTRCAT_RELID]->dirty) {

        Attribute record[RELCAT_NO_ATTRS];

        RelCacheTable::relCatEntryToRecord(
            &RelCacheTable::relCache[ATTRCAT_RELID]->relCatEntry,
            record
        );

        RecId recId = RelCacheTable::relCache[ATTRCAT_RELID]->recId;

        RecBuffer relCatBlock(recId.block);

        relCatBlock.setRecord(record, recId.slot);
    }

    free(RelCacheTable::relCache[ATTRCAT_RELID]);
    RelCacheTable::relCache[ATTRCAT_RELID] = nullptr;

    if(RelCacheTable::relCache[RELCAT_RELID]->dirty) {

        Attribute record[RELCAT_NO_ATTRS];

        RelCacheTable::relCatEntryToRecord(
            &RelCacheTable::relCache[RELCAT_RELID]->relCatEntry,
            record
        );

        RecId recId = RelCacheTable::relCache[RELCAT_RELID]->recId;

        RecBuffer relCatBlock(recId.block);

        relCatBlock.setRecord(record, recId.slot);
    }

    free(RelCacheTable::relCache[RELCAT_RELID]);
    RelCacheTable::relCache[RELCAT_RELID] = nullptr;

    AttrCacheEntry *curr = AttrCacheTable::attrCache[RELCAT_RELID];
    
    while(curr != nullptr){
        AttrCacheEntry *next = curr->next;
        free(curr);
        curr = next;
    }

    curr = AttrCacheTable::attrCache[ATTRCAT_RELID];

    while(curr != nullptr){
        AttrCacheEntry *next = curr->next;
        free(curr);
        curr = next;
    }

    AttrCacheTable::attrCache[RELCAT_RELID] = nullptr;
    AttrCacheTable::attrCache[ATTRCAT_RELID] = nullptr;
    
} 

int OpenRelTable::closeRel(int relId){
    // confirm that rel-id fits the following conditions
    //     2 <=relId < MAX_OPEN
    //     does not correspond to a free slot

    if(relId == RELCAT_RELID || relId == ATTRCAT_RELID){
        return E_NOTPERMITTED;
    }

    if(relId < 2 || relId >= MAX_OPEN){
        return E_OUTOFBOUND;
    }

    if(tableMetaInfo[relId].free){
        return E_RELNOTOPEN;
    }

    /****** Releasing the Relation Cache entry of the relation ******/

    if(RelCacheTable::relCache[relId]->dirty)
    {
        RelCacheEntry *relCacheEntry = RelCacheTable::relCache[relId];

        Attribute record[RELCAT_NO_ATTRS];

        RelCacheTable::relCatEntryToRecord(
            &relCacheEntry->relCatEntry,
            record
        );

        RecId recId = relCacheEntry->recId;

        RecBuffer relCatBlock(recId.block);

        int ret = relCatBlock.setRecord(record, recId.slot);

        if(ret != SUCCESS){
            return ret;
        }
    }

    free(RelCacheTable::relCache[relId]);
    RelCacheTable::relCache[relId] = nullptr;

    /****** Releasing the Attribute Cache entry of the relation ******/

    AttrCacheEntry* curr = AttrCacheTable::attrCache[relId];

    while(curr != nullptr){
        AttrCacheEntry* next = curr->next;
        free(curr);
        curr = next;
    }

    AttrCacheTable::attrCache[relId] = nullptr;

    /****** Set the Open Relation Table entry of the relation as free ******/

    tableMetaInfo[relId].free = true;

    return SUCCESS;
}

int OpenRelTable::getFreeOpenRelTableEntry(){
    for(int i = 0;i < MAX_OPEN;i++){
        if(tableMetaInfo[i].free){
            return i;
        }
    }

    return E_CACHEFULL;
}

int OpenRelTable::getRelId(char relName[ATTR_SIZE]) {

    for(int i = 0; i < MAX_OPEN; i++) {

        if(RelCacheTable::relCache[i] == nullptr)
            continue;

        if(tableMetaInfo[i].free == false && strcmp(RelCacheTable::relCache[i]->relCatEntry.relName,relName) == 0) {
            return i;
        }
    }

    return E_RELNOTOPEN;
}