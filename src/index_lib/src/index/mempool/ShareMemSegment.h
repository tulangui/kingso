/*********************************************************************
 * $Author: pujian $
 *
 * $LastChangedBy: pujian $
 *
 * $Revision: 2577 $
 *
 * $LastChangedDate: 2011-03-09 09:50:55 +0800 (Wed, 09 Mar 2011) $
 *
 * $Id: ShareMemSegment.h 2577 2011-03-09 01:50:55Z pujian $
 *
 * $Brief: share memory segment $
 ********************************************************************/

#ifndef KINGSO_SHM_SHAREMEMSEGMENT_H_
#define KINGSO_SHM_SHAREMEMSEGMENT_H_

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <string.h>
#include <error.h>
#include <errno.h>

namespace index_mem
{

class ShareMemSegment
{ 
    public:
        ShareMemSegment();
        ~ShareMemSegment();

        int createShareMem(const char * path, int pos, size_t pageSize);
        int loadShareMem(const char * path, int pos);
        int detachShareMem();
        int freeShareMem();

        inline size_t size()
        {
            return _size;
        }

        inline void * addr()
        {
            return _pAddress;
        }

        inline int getShmId()
        {
            return _segId;
        }

    private:
        size_t _size;
        int    _segId;
        void * _pAddress;
};

}

#endif //KINGSO_SHM_SHAREMEMSEGMENT_H_
