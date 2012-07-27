#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "util/Log.h"
#include "SignDict.h"

#define INC_FINGER_FILE  "incFinger.dat"
#define DEFAULT_UPDATE_NUM (100<<20)

UPDATE_BEGIN

SignDict SignDict::_signDictInstance;

SignDict::SignDict()
{
    _fullNum = 0;
    _updateNum = 0;
    for(int i = 0; i < HASH_DICT_NUM; i++) {
        _fd[i] = 0;
        _hashTable[i] = NULL;
    }
}

SignDict::~SignDict()
{
    for(int i = 0; i < HASH_DICT_NUM; i++) {
        if(_fd[i]) close(_fd[i]);
        if(_hashTable[i]) delete _hashTable[i];
    }
}

int SignDict::init(const char* dictPath)
{
    const int eNum = DEFAULT_UPDATE_NUM / HASH_DICT_NUM;
    for(int i = 0; i < HASH_DICT_NUM; i++) {
        _hashTable[i] = new util::HashMap<int64_t, Finger>(eNum);
    }

    struct stat st;
    char filename[PATH_MAX];
    struct dirent* file = NULL;

    // 加载全量指纹信息
    DIR* dir = opendir(dictPath);
    if(dir == NULL) {
        TERR("open dir %s error", dictPath);
        return -1;
    }

    int readNum = 0;
    while((file = readdir(dir))) {
        snprintf(filename, PATH_MAX, "%s/%s", dictPath, file->d_name);
        lstat(filename, &st);
        if(S_ISDIR(st.st_mode)) continue;
        
        int len = strlen(file->d_name);
        if(len < 5 || strcmp(file->d_name + len - 5, ".sign")) continue;

        int ret = loadSign(filename);
        if(ret < 0) {
            closedir(dir);
            TERR("load sign %s error", filename);
            return -1;
        }
        readNum += ret;
    }
    closedir(dir);
    
    _fullNum = 0;
    for(int i = 0; i < HASH_DICT_NUM; i++) {
        _fullNum += _hashTable[i]->size();
    }
    if(_fullNum == readNum) {
        TLOG("success load full finger %d", _fullNum);
    } else {
        TWARN("load full finger %d, but true %d", readNum, _fullNum);
    }

    // 加载增量指纹信息
    readNum = 0;
    for(int i = 0; i < HASH_DICT_NUM; i++) {
        snprintf(filename, PATH_MAX, "%s/%.2d_%s", dictPath, i, INC_FINGER_FILE);
        if(lstat(filename, &st) == 0 && S_ISREG(st.st_mode)) {
            int ret = loadSign(filename);
            if(ret < 0) {
                TERR("load update finger file %s error", filename);
                return -1;
            }
            readNum += ret;
        }
    }
    for(int i = 0; i < HASH_DICT_NUM; i++) {
        _updateNum += _hashTable[i]->size();
    }
    _updateNum -= _fullNum;
    TLOG("success load inc finger %d, true %d", readNum, _updateNum);
    
    // 打开增量记录文件
    for(int i = 0; i < HASH_DICT_NUM; i++) {
        snprintf(filename, PATH_MAX, "%s/%.2d_%s", dictPath, i, INC_FINGER_FILE);
        if(lstat(filename, &st)) {
            int fd = open(filename, O_CREAT, S_IRWXU|S_IRWXG|S_IRWXO);
            close(fd);
        }
        if(lstat(filename, &st)) {
            TERR("stat update file %s error", filename);
            return -1;
        }
        _fd[i] = open(filename, O_RDWR, S_IRWXU|S_IRWXG|S_IRWXO);
        if(_fd[i] <= 0) {
            TERR("open update file %s error", filename);
            return -1;
        }
        if(st.st_size % sizeof(SignInfo)) {// 上次未完全写完，应该极少发生
            st.st_size = st.st_size / sizeof(SignInfo) * sizeof(SignInfo);
            lseek(_fd[i], st.st_size, SEEK_SET);
        }
    }
    return 0;
}

int SignDict::check(int64_t nid, unsigned char sign[16])
{
    int ret = -1;
    int no = nid % HASH_DICT_NUM;
    util::HashMap<int64_t, Finger>::iterator it;

    _hashLock[no].lock();
    it = _hashTable[no]->find(nid);
    if(it == _hashTable[no]->end()) { // 全新的
       if(_hashTable[no]->insert(nid, *(Finger*)sign) == false) {
           TWARN("insert new nid %lu failed", nid);
       }
    } else if(it->value.high != (*(Finger*)sign).high || it->value.low != (*(Finger*)sign).low) {
        it->value = *(Finger*)sign;
    } else {
        ret = 0; 
    }
    if(ret) {
        SignInfo info;
        info.nid = nid;
        info.finger = *(Finger*)sign;
        if(write(_fd[no], &info, sizeof(SignInfo)) != sizeof(SignInfo)) {
            TERR("write update info error, nid=%lu", nid);
            return -1;
        }
    }
    _hashLock[no].unlock();

    return ret;
}

int SignDict::loadSign(const char* filename)
{
    SignInfo info;
    int readNum = 0;
    util::HashMap<int64_t, Finger>::iterator it;

    FILE* fp = fopen(filename, "rb");
    if(NULL == fp) {
        return 0;
    }
    while(fread(&info, sizeof(SignInfo), 1, fp)) {
        readNum++;
        int64_t nid = info.nid;
        if(_hashTable[nid%HASH_DICT_NUM]->insert(nid, info.finger) == false) {
            it = _hashTable[nid%HASH_DICT_NUM]->find(nid);
            if(it == _hashTable[nid%HASH_DICT_NUM]->end()) {
                fclose(fp);
                return -1;
            }
            it->value = info.finger;
        }
    }

    fclose(fp);
    return readNum;
}

UPDATE_END
