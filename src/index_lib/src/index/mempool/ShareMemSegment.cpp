#include "ShareMemSegment.h"

namespace index_mem
{

ShareMemSegment::ShareMemSegment()
{
    _size     = 0;
    _segId    = -1;
    _pAddress = NULL;
}

ShareMemSegment::~ShareMemSegment()
{
}

int 
ShareMemSegment::createShareMem(const char * path, int pos, size_t size)
{
    int ret = 0;
    struct shmid_ds stat;
    key_t key;

    do {
        key = ftok(path, pos);
        if (key == -1) {
            fprintf(stderr, "createShareMem:ftok failed:%s,%d, error info:%s\n", path, pos, strerror(errno));
            ret = -1;
            break;
        }

        _segId = shmget(key, size, IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
        if (_segId == -1) {
            fprintf(stderr, "createShareMem:shmget failed! error info:%s\n", strerror(errno));
            ret = -1;
            break;
        }

        if (shmctl(_segId, IPC_STAT, &stat) == -1) {
            fprintf(stderr, "createShareMem:shmctl failed! error info:%s\n", strerror(errno));
            ret = -1;
            break;
        }
        _size = stat.shm_segsz;

        _pAddress = shmat(_segId, NULL, 0);
        if (_pAddress == (void *)(-1)) {
            fprintf(stderr, "createShareMem:shmat failed! error info:%s\n", strerror(errno));
            ret = -1;
            break;
        }

    }while(0);

    if (ret == -1) {
        _segId    = -1;
        _size     = 0;
        _pAddress = NULL;
    }
    return ret;
}


int
ShareMemSegment::loadShareMem(const char * path, int pos)
{
    int ret = 0;
    struct shmid_ds stat;
    key_t key;

    do {
        key = ftok(path, pos);
        if (key == -1) {
            fprintf(stderr, "loadShareMem:ftok failed:%s,%d, error info:%s\n", path, pos, strerror(errno));
            ret = -1;
            break;
        }

        _segId = shmget(key, getpagesize(), S_IRUSR | S_IWUSR);
        if (_segId == -1) {
            //fprintf(stderr, "loadShareMem:shmget failed! error info:%s\n", strerror(errno));
            ret = -1;
            break;
        }

        if (shmctl(_segId, IPC_STAT, &stat) == -1) {
            fprintf(stderr, "loadShareMem:shmctl failed! error info:%s\n", strerror(errno));
            ret = -1;
            break;
        }
        _size = stat.shm_segsz;

        _pAddress = shmat(_segId, NULL, 0);
        if (_pAddress == (void *)(-1)) {
            fprintf(stderr, "loadShareMem:shmat failed! error info:%s\n", strerror(errno));
            ret = -1;
            break;
        }

    }while(0);

    if (ret == -1) {
        _segId    = -1;
        _size     = 0;
        _pAddress = NULL;
    }
    return ret;
}

int
ShareMemSegment::detachShareMem()
{
    int ret = 0;
    if (_segId != -1 && _pAddress != NULL) {
        if (shmdt(_pAddress) == -1) {
            fprintf(stderr, "freeShareMem:shmdt failed! error info:%s\n", strerror(errno));
            ret = -1;
        }            
    }

    _pAddress = NULL;
    return ret;
}

int
ShareMemSegment::freeShareMem()
{
    int ret = 0;
    if (_segId != -1) {
        detachShareMem();
        if ( (ret = shmctl(_segId, IPC_RMID, 0)) == -1) {
            fprintf(stderr, "freeShareMem:shmctl failed! error info:%s\n", strerror(errno));
        }
    }

    _size   = 0;
    _segId  = -1;
    return ret;
}

}
