#include "index_lib/ProfileSyncManager.h"
#include "index_lib/ProfileMMapFile.h"

namespace index_lib
{

ProfileSyncManager::ProfileSyncManager(uint32_t init)
{
    _usedNum = 0;
    _docId   = 0;
    ProfileSyncInfo * pInfo = NULL;
    for(uint32_t i = 0; i < init; i++) {
        pInfo = new ProfileSyncInfo();
        if (pInfo != NULL) {
            _syncInfoVector.push_back(pInfo);
        }
    }
}

ProfileSyncManager::~ProfileSyncManager()
{
    for(uint32_t i = 0; i < _syncInfoVector.size(); i++) {
        if( _syncInfoVector[i] ) {
            delete _syncInfoVector[i];
        }
    }
    _syncInfoVector.clear();
    _usedNum = 0;
}

int ProfileSyncManager::putSyncInfo(char * pMemAddr, size_t len, ProfileMMapFile * file)
{
    if (unlikely(pMemAddr == NULL || file == NULL)) {
        return -1;
    }

    if (unlikely(_usedNum >= _syncInfoVector.size())) {
        if (!expand()) {
            return -1;
        }
    }

    ProfileSyncInfo * pInfo = _syncInfoVector[_usedNum];
    pInfo->pMemAddr = pMemAddr;
    pInfo->len      = len;
    pInfo->pFile    = file;

    ++_usedNum;
    return 0;
}

void ProfileSyncManager::syncToDisk()
{
    ProfileSyncInfo * pInfo = NULL;
    for(uint32_t i = 0; i < _usedNum; i++) {
        pInfo = _syncInfoVector[i];
        pInfo->pFile->syncToDisk(pInfo->pMemAddr, pInfo->len);
    }
    _usedNum = 0;
}

bool ProfileSyncManager::expand(uint32_t step)
{
    ProfileSyncInfo * pInfo = NULL;
    for(uint32_t i = 0; i < step; i++) {
        pInfo = new ProfileSyncInfo();
        if (pInfo != NULL) {
            _syncInfoVector.push_back(pInfo);
        }
        else {
            return false;
        }
    }
    return true;
}

void ProfileSyncManager::setDocId(uint32_t docid)
{
    _docId = docid;
}

}


