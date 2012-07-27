/*********************************************************************
 * $Author: pujian $
 *
 * $LastChangedBy: pujian $
 *
 * $Revision: 2577 $
 *
 * $LastChangedDate: 2011-03-09 09:50:55 +0800 (Wed, 09 Mar 2011) $
 *
 * $Id: ProfileSyncManager.h 2577 2011-03-09 01:50:55Z pujian $
 *
 * $Brief: ini file parser $
 ********************************************************************/

#ifndef PROFILE_PROFILESYNCMANAGER_H_
#define PROFILE_PROFILESYNCMANAGER_H_

#include "IndexLib.h"
#include <vector>

using namespace std;

#define INIT_SYNC_NUM 64
#define STEP_SYNC_NUM 8

namespace index_lib
{

class ProfileMMapFile;

struct ProfileSyncInfo
{
    public:
        char * pMemAddr;         // value address in memory
        size_t len;              // value length
        ProfileMMapFile * pFile; // file pointer 

    public:
        explicit ProfileSyncInfo():pMemAddr(NULL),len(0),pFile(NULL) { }
};

class ProfileSyncManager
{ 
    public:
        ProfileSyncManager(uint32_t init = INIT_SYNC_NUM);
        ~ProfileSyncManager();

        int putSyncInfo(char * pMemAddr, size_t len, ProfileMMapFile * file);
        void syncToDisk();
        void setDocId(uint32_t docid);
        inline uint32_t getDocId() { return _docId; }

    protected:
        bool expand(uint32_t step = STEP_SYNC_NUM);

    private:
        vector <ProfileSyncInfo *> _syncInfoVector;   // SyncInfo
        uint32_t                   _usedNum;          // SyncInfo vector中的个数
        uint32_t                   _docId;            // SyncInfo vector中对应的docid
};

}


#endif //PROFILE_PROFILESYNCMANAGER_H_
