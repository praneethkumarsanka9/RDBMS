#include "OpenRelTable.h"
#include <cstdlib>
#include <cstring>

OpenRelTable::OpenRelTable(){
    for(int i = 0;i < MAX_OPEN;i++){
        RelCacheTable::relCache[i] = nullptr;
        AttrCacheTable::attrCache[i] = nullptr;
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

    int STUDENT = 2;

    relCatBlock.getRecord(relCatRecord,STUDENT);

    RelCacheTable::recordToRelCatEntry(relCatRecord,&relCacheEntry.relCatEntry);

    relCacheEntry.dirty = false;
    relCacheEntry.recId.block = RELCAT_BLOCK;
    relCacheEntry.recId.slot = STUDENT;
    
    RelCacheTable::relCache[STUDENT] = (RelCacheEntry*)malloc(sizeof(RelCacheEntry));
    *(RelCacheTable::relCache[STUDENT]) = relCacheEntry;

    head = nullptr;
    tail = nullptr;

    for(int i = 12;i < 18;i++){
        attrCatBlock.getRecord(attrCatRecord,i);

        AttrCacheEntry* node = (AttrCacheEntry*)malloc(sizeof(AttrCacheEntry));
        AttrCacheTable::recordToAttrCatEntry(attrCatRecord,&node->attrCatEntry);

        node->dirty = false;
        node->next = nullptr;
        node->recId.block = ATTRCAT_BLOCK;
        node->recId.slot = i;

        if(head == nullptr){
            head = node;
            tail = head;
        }else{
            tail->next = node;
            tail = node;
        }
    }

    AttrCacheTable::attrCache[STUDENT] = head;
}

OpenRelTable::~OpenRelTable(){
    for(int i = 0;i < MAX_OPEN;i++){
        if(RelCacheTable::relCache[i] != nullptr){
            free(RelCacheTable::relCache[i]);
            RelCacheTable::relCache[i] = nullptr;
        }

        AttrCacheEntry *curr = AttrCacheTable::attrCache[i];

        while(curr != nullptr){
            AttrCacheEntry *temp = curr;
            curr = curr->next;
            free(temp);
        }
        AttrCacheTable::attrCache[i] = nullptr;
    }
}

