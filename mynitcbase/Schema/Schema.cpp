#include "Schema.h"

#include <cmath>
#include <cstring>

int Schema::openRel(char relName[ATTR_SIZE]){
    int ret = OpenRelTable::openRel(relName);

    if(ret >= 0){
        return SUCCESS;
    }

    return ret;
}

int Schema::renameRel(char oldRelName[ATTR_SIZE], char newRelName[ATTR_SIZE]){

    if(strcmp(oldRelName,ATTRCAT_RELNAME) == 0 || strcmp(oldRelName,RELCAT_RELNAME) == 0 || strcmp(newRelName,ATTRCAT_RELNAME) == 0 || strcmp(newRelName,RELCAT_RELNAME) == 0){
    
        return E_NOTPERMITTED;
    
    }
    
    int ret = OpenRelTable::getRelId(oldRelName);
    
    if(ret != E_RELNOTOPEN){
        return E_RELOPEN;
    }

    ret = BlockAccess::renameRelation(oldRelName,newRelName);

    return ret;
}

int Schema::renameAttr(char *relName, char *oldAttrName, char *newAttrName){

    if(strcmp(oldAttrName,ATTRCAT_RELNAME) == 0 || strcmp(oldAttrName,RELCAT_RELNAME) == 0 || strcmp(newAttrName,ATTRCAT_RELNAME) == 0 || strcmp(newAttrName,RELCAT_RELNAME) == 0){
    
        return E_NOTPERMITTED;
    
    }

    int ret = OpenRelTable::getRelId(oldAttrName);

    if(ret != E_RELNOTOPEN){
        return E_RELOPEN;
    }

    ret = BlockAccess::renameAttribute(relName,oldAttrName,newAttrName);

    return ret;
}


int Schema::closeRel(char relName[ATTR_SIZE]){
    if(strcmp(relName,"RELCAT") == 0 || strcmp(relName,"ATTRCAT") == 0){
        return E_NOTPERMITTED;
    }

    int relId = OpenRelTable::getRelId(relName);

    if(relId == E_RELNOTOPEN){
        return E_RELNOTOPEN;
    }

    return OpenRelTable::closeRel(relId);
}

int Schema::createRel(char relName[], int nAttrs, char attrs[][ATTR_SIZE], int attrtype[]){
    Attribute relNameAsAttribute;
    strcpy(relNameAsAttribute.sVal,relName);

    RecId targetRelId;

    int retVal = RelCacheTable::resetSearchIndex(RELCAT_RELID);

    if(retVal != SUCCESS){
        return retVal;
    }

    char attrName[ATTR_SIZE];
    strcpy(attrName, "RelName");

    targetRelId = BlockAccess::linearSearch(RELCAT_RELID,attrName,relNameAsAttribute,EQ);

    if(targetRelId.block != -1 && targetRelId.slot != -1){
        return E_RELEXIST;
    }

    for(int i = 0;i < nAttrs;i++){
        for(int j = i + 1;j < nAttrs;j++){
            if(strcmp(attrs[i],attrs[j]) == 0){
                return E_DUPLICATEATTR;
            }
        }
    }

    Attribute relCatRecord[RELCAT_NO_ATTRS];

    strcpy(relCatRecord[RELCAT_REL_NAME_INDEX].sVal,relName);
    relCatRecord[RELCAT_NO_ATTRIBUTES_INDEX].nVal = nAttrs;
    relCatRecord[RELCAT_NO_RECORDS_INDEX].nVal = 0;
    relCatRecord[RELCAT_FIRST_BLOCK_INDEX].nVal = -1;
    relCatRecord[RELCAT_LAST_BLOCK_INDEX].nVal = -1;
    relCatRecord[RELCAT_NO_SLOTS_PER_BLOCK_INDEX].nVal = 2016 / (16 * nAttrs + 1);

    retVal = BlockAccess::insert(RELCAT_RELID,relCatRecord);

    if(retVal != SUCCESS){
        return retVal;
    }

     for (int i = 0; i < nAttrs; i++) {

        Attribute attrCatRecord[ATTRCAT_NO_ATTRS];

        strcpy( attrCatRecord[ATTRCAT_REL_NAME_INDEX].sVal, relName);

        strcpy( attrCatRecord[ATTRCAT_ATTR_NAME_INDEX].sVal, attrs[i]);

        attrCatRecord[ATTRCAT_ATTR_TYPE_INDEX].nVal = attrtype[i];
        attrCatRecord[ATTRCAT_PRIMARY_FLAG_INDEX].nVal = -1;
        attrCatRecord[ATTRCAT_ROOT_BLOCK_INDEX].nVal = -1;
        attrCatRecord[ATTRCAT_OFFSET_INDEX].nVal = i;

        retVal = BlockAccess::insert( ATTRCAT_RELID, attrCatRecord);

        if (retVal != SUCCESS) {
            Schema::deleteRel(relName);
            return E_DISKFULL;
        }
    }

    return SUCCESS;
}

int Schema::deleteRel(char *relName){
    // Catalogs cannot be deleted
    if (strcmp(relName, RELCAT_RELNAME) == 0 ||strcmp(relName, ATTRCAT_RELNAME) == 0) {
        return E_NOTPERMITTED;
    }

    int relId = OpenRelTable::getRelId(relName);

    // Relation must be closed
    if (relId >= 0 && relId < MAX_OPEN)
        return E_RELOPEN;

    return BlockAccess::deleteRelation(relName);
}