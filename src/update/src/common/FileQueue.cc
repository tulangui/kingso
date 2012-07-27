#include "FileQueue.h"
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <malloc.h>
#include <stdio.h>
#include <sys/mman.h>
#include <stdlib.h>
#include "util/Log.h"
#include "util/ThreadLock.h"

using namespace std;

namespace update {

#define INFO_FILE_EXNAME "inf"
typedef struct
{
    uint64_t node_count;
    uint64_t min_seq;
    uint64_t max_seq;
    uint64_t last_index_fno : 16;
    uint64_t last_data_fno : 16;
}
FileQueueChunkInfo;

#define INDEX_FILE_EXNAME "idx"
typedef struct
{
    uint64_t seq : 64;
    uint64_t fno : 16;
    uint64_t off : 31;
    uint64_t len : 31;

    char cmpr;              //压缩标志
    uint8_t action;         //操作类型，因排重操作可能与消息体中的类型不一致
    uint64_t nid;           //宝贝id
    uint64_t distribute;    //分发id
}
FileQueueChunkIndex;
static uint32_t PERCHUNKINDEX_NODE_MAX = (((((uint32_t)1)<<31)-1)/sizeof(FileQueueChunkIndex));

#define DATA_FILE_EXNAME "dat"
static uint32_t PERDATA_LENGTH_MAX = ((((uint32_t)1)<<31)-1);

struct MMapInfo {
    int   fd;     // 文件句柄
    int   ref;    // 引用次数
    void* mem;    // 内存地址
};

class FileQueueChunk
{
    public:
        FileQueueChunk();
        ~FileQueueChunk();
    private:
        /*  
         * 禁止拷贝构造函数和赋值操作
         */
        FileQueueChunk(const FileQueueChunk&);
        FileQueueChunk& operator=(const FileQueueChunk&);
    public:
        int open(const char* dir);
        void close();
        int get(StorableMessage& msg, uint64_t seq);
        int get(StorableMessage& msg, FileQueueSelect& sel);
        
        int set(const StorableMessage& msg);
        uint64_t getMinSeq();
        uint64_t getMaxSeq();
    private:
        int loadInfo();
        int loadReadIndex(uint16_t fno);
        int loadWriteIndex(uint16_t fno);
        int openReadData(uint16_t fno);
        int openWriteData(uint16_t fno);
        void closeInfo();
        void closeReadIndex();
        void closeWriteIndex();
        void closeReadData();
        void closeWriteData();
    private:
        int _iffd;      // 整体信息句柄
        int _wifd;      // 写索引句柄
        int _rdfd;      // 读数据文件句柄
        int _wrfd;      // 写数据文件句柄
        FileQueueChunkInfo* _info;
        FileQueueChunkIndex* _rdIndex;
        FileQueueChunkIndex* _wrIndex;
        uint16_t _rifno;
        uint16_t _wifno;
        uint16_t _rdfno;
        uint16_t _wrfno;

        char _dir[PATH_MAX+1];
        bool _opened;
    public:
        // 只读索引mmap表
        static util::Mutex _mmapLock;
        static std::map<std::string, MMapInfo> _mmapTable; 
};
util::Mutex FileQueueChunk::_mmapLock;
std::map<std::string, MMapInfo> FileQueueChunk::_mmapTable;

FileQueueChunk::FileQueueChunk()
{
    _iffd = _wifd = _rdfd = _wrfd = -1;
    _info = 0;
    _rdIndex = _wrIndex = 0;
    _rifno = _wifno = _rdfno = _wrfno = 0;
    _dir[0] = 0;
    _opened = false;
}

FileQueueChunk::~FileQueueChunk()
{
    close();
}

int FileQueueChunk::open(const char* dir)
{
    struct stat dir_stat;

    if(!dir){
        TWARN("FileQueueChunk open argument error, return!");
        return -1;
    }
    if(access(dir, F_OK)){
        if(mkdir(dir, 0777)){
            TWARN("mkdir error, return![dir:%s]", dir);
            return -1;
        }
    }
    else{
        if(stat(dir, &dir_stat) || !S_ISDIR(dir_stat.st_mode)){
            TWARN("%s is not dir, return!", dir);
            return -1;
        }
    }

    strncpy(_dir, dir, PATH_MAX);
    _dir[PATH_MAX] = 0;

    if(loadInfo()){
        TWARN("loadInfo error, return!");
        return -1;
    }
    if(loadWriteIndex(_info->last_index_fno)){
        TWARN("loadWriteIndex error, return!");
        closeInfo();
        return -1;
    }
    if(loadReadIndex(0)){
        TWARN("loadReadIndex error, return!");
        closeInfo();
        closeWriteIndex();
        return -1;
    }
    if(openWriteData(_info->last_data_fno)){
        TWARN("openWriteData error, return!");
        closeInfo();
        closeWriteIndex();
        closeReadIndex();
        return -1;
    }
    if(openReadData(0)){
        TWARN("openReadData error, return!");
        closeInfo();
        closeWriteIndex();
        closeReadIndex();
        closeWriteData();
        return -1;
    }

    _opened = true;

    return 0;
}

void FileQueueChunk::close()
{
    if(_opened){
        closeInfo();
        closeReadIndex();
        closeWriteIndex();
        closeReadData();
        closeWriteData();
        _opened = false;
    }
}

int FileQueueChunk::get(StorableMessage& msg, uint64_t seq)
{
    uint64_t seq_idx = 0;
    uint16_t idx_fno = 0;
    uint32_t idx_idx = 0;
    uint16_t dat_fno = 0;
    uint32_t dat_off = 0;
    uint32_t dat_len = 0;

    if(!_opened){
        TWARN("FileQueueChunk %s not open, reutrn!", _dir);
        return -1;
    }
    if(seq<_info->min_seq){
        TWARN("sequence %lu to small, min is %lu, reutrn!", seq, _info->min_seq);
        return -1;
    }
    if(seq>_info->max_seq){
        return 0;
    }
    
    seq_idx = seq-_info->min_seq;
    idx_fno = seq_idx/PERCHUNKINDEX_NODE_MAX;
    idx_idx = seq_idx%PERCHUNKINDEX_NODE_MAX;

    if(idx_fno!=_rifno){
        TNOTE("switch index file when get, seq=%lu", seq);
        closeReadIndex();
        if(loadReadIndex(idx_fno)){
            TWARN("loadReadIndex %d error, return!", idx_fno);
            return -1;
        }
    }
    if(_rdIndex[idx_idx].seq!=seq){
        TWARN("index seq=%lu, not match input seq=%lu, return!", _rdIndex[idx_idx].seq, seq);
        return -1;
    }
    dat_fno = _rdIndex[idx_idx].fno;
    dat_off = _rdIndex[idx_idx].off;
    dat_len = _rdIndex[idx_idx].len;

    if(msg.max<dat_len){
        char* mem = (char*)malloc(dat_len);
        if(!mem){
            TWARN("malloc msg  buffer error, return!");
            return -1;
        }
        if(msg.ptr){
            msg.freeFn(msg.ptr);
        }
        msg.ptr = mem;
        msg.max = dat_len;
        msg.freeFn = ::free;
    }
    if(dat_fno!=_rdfno){
        TNOTE("switch data file when get, seq=%lu", seq);
        closeReadData();
        if(openReadData(dat_fno)){
            TWARN("openReadData %d error, return!", dat_fno);
            return -1;
        }
    }
    if(pread(_rdfd, msg.ptr, dat_len, dat_off)!=dat_len){
        TWARN("pread error, return!");
        return -1;
    }
    msg.size = dat_len;
    msg.seq = seq;
    msg.action = _rdIndex[idx_idx].action;
    msg.cmpr = _rdIndex[idx_idx].cmpr;
    msg.nid = _rdIndex[idx_idx].nid;
    msg.distribute = _rdIndex[idx_idx].distribute;
    return 1;
}

int FileQueueChunk::get(StorableMessage& msg, FileQueueSelect& sel)
{
    uint64_t seq_idx = 0;
    uint16_t idx_fno = 0;
    uint32_t idx_idx = 0;
    uint16_t dat_fno = 0;
    uint32_t dat_off = 0;
    uint32_t dat_len = 0;

    if(!_opened){
        TWARN("FileQueueChunk %s not open, reutrn!", _dir);
        return -1;
    }

    if(sel.seq < _info->min_seq){
        TWARN("sequence %lu to small, min is %lu, reutrn!", sel.seq, _info->min_seq);
        return -1;
    }

    while(sel.seq <= _info->max_seq) {
        seq_idx = sel.seq-_info->min_seq;
        idx_fno = seq_idx/PERCHUNKINDEX_NODE_MAX;
        idx_idx = seq_idx%PERCHUNKINDEX_NODE_MAX;

        if(idx_fno!=_rifno){
            TNOTE("switch index file when get, seq=%lu", sel.seq);
            closeReadIndex();
            if(loadReadIndex(idx_fno)){
                TWARN("loadReadIndex %d error, return!", idx_fno);
                return -1;
            }
        }
        if(_rdIndex[idx_idx].seq != sel.seq) {
            TWARN("index seq=%lu, not match input seq=%lu, return!", _rdIndex[idx_idx].seq, sel.seq);
            return -1;
        }
        if(_rdIndex[idx_idx].distribute % sel.colNum == sel.colNo) {
            break;
        }
        sel.seq++;
    }
    if(sel.seq > _info->max_seq) {// 找到最后，没找到
        return 0;
    }
    dat_fno = _rdIndex[idx_idx].fno;
    dat_off = _rdIndex[idx_idx].off;
    dat_len = _rdIndex[idx_idx].len;

    if(msg.max<dat_len){
        char* mem = (char*)malloc(dat_len);
        if(!mem){
            TWARN("malloc msg  buffer error, return!");
            return -1;
        }
        if(msg.ptr){
            msg.freeFn(msg.ptr);
        }
        msg.ptr = mem;
        msg.max = dat_len;
        msg.freeFn = ::free;
    }
    if(dat_fno!=_rdfno){
        TNOTE("switch data file when get, seq=%lu", sel.seq);
        closeReadData();
        if(openReadData(dat_fno)){
            TWARN("openReadData %d error, return!", dat_fno);
            return -1;
        }
    }
    if(pread(_rdfd, msg.ptr, dat_len, dat_off)!=dat_len){
        TWARN("pread error, return!");
        return -1;
    }
    msg.size = dat_len;
    msg.seq = sel.seq;
    msg.action = _rdIndex[idx_idx].action;
    msg.cmpr = _rdIndex[idx_idx].cmpr;
    msg.nid = _rdIndex[idx_idx].nid;
    msg.distribute = _rdIndex[idx_idx].distribute;
    return 1;
}

int FileQueueChunk::set(const StorableMessage& msg)
{
    struct stat wrstat;
    uint64_t seq_idx = 0;
    uint16_t idx_fno = 0;
    uint32_t idx_idx = 0;
    uint16_t dat_fno = 0;
    uint32_t dat_off = 0;
    uint32_t dat_len = 0;

    if(!_opened){
        TWARN("FileQueueChunk %s not open, return!", _dir);
        return -1;
    }
    if(!msg.ptr || msg.size==0 || (_info->max_seq>0 && msg.seq!=_info->max_seq+1)){
        TWARN("msg content error, size=%d, seq=%lu/%lu return!", msg.size, msg.seq, _info->max_seq+1);
        return -1;
    }

    if(_info->min_seq==0){
        seq_idx = 0;
    }
    else{
        seq_idx = msg.seq-_info->min_seq;
    }
    idx_fno = seq_idx/PERCHUNKINDEX_NODE_MAX;
    idx_idx = seq_idx%PERCHUNKINDEX_NODE_MAX;
    if(idx_fno!=_wifno){
        TNOTE("switch index file when set, seq=%lu, cnt=%lu", msg.seq, _info->node_count);
        closeWriteIndex();
        if(loadWriteIndex(idx_fno)){
            TWARN("loadWriteIndex %d error, return!", idx_fno);
            return -1;
        }
        if(_wifno>_info->last_index_fno){
            _info->last_index_fno = _wifno;
        }
    }

    if(fstat(_wrfd, &wrstat)){
        TWARN("fstat erorr, return!");
        return -1;
    }
    dat_fno = _wrfno;
    dat_off = wrstat.st_size;
    dat_len = msg.size;
    if(dat_off+dat_len>PERDATA_LENGTH_MAX){
        TNOTE("switch data file when set, seq=%lu, cnt=%lu", msg.seq, _info->node_count);
        closeWriteData();
        if(openWriteData(_wrfno+1)){
            TWARN("openWriteData %d error, return!", _wrfno+1);
            return -1;
        }
        if(_wrfno>_info->last_data_fno){
            _info->last_data_fno = _wrfno;
        }
        dat_fno = _wrfno;
        dat_off = 0;
    }
    if(pwrite(_wrfd, msg.ptr, dat_len, dat_off)!=dat_len){
        TWARN("pwrite error, return!");
        return -1;
    }

    _wrIndex[idx_idx].seq = msg.seq;
    _wrIndex[idx_idx].fno = dat_fno;
    _wrIndex[idx_idx].off = dat_off;
    _wrIndex[idx_idx].len = dat_len;
    _wrIndex[idx_idx].action = msg.action;
    _wrIndex[idx_idx].cmpr   = msg.cmpr;
    _wrIndex[idx_idx].nid = msg.nid;
    _wrIndex[idx_idx].distribute = msg.distribute;

    _info->node_count ++;
    _info->max_seq = msg.seq;
    if(_info->min_seq==0){
        _info->min_seq = msg.seq;
    }

    return 1;
}

uint64_t FileQueueChunk::getMinSeq()
{
    if(_opened && _info){
        return _info->min_seq;
    }
    else{
        return 0;
    }
}

uint64_t FileQueueChunk::getMaxSeq()
{
    if(_opened && _info){
        return _info->max_seq;
    }
    else{
        return 0;
    }
}

int FileQueueChunk::loadInfo()
{
    char strbuf[PATH_MAX+1] = {0};
    int fd = -1;
    void* mem = 0;
    struct stat ifstat;

    snprintf(strbuf, PATH_MAX+1, "%s/%05u.%s", _dir, 0, INFO_FILE_EXNAME);
    if(access(strbuf, F_OK)){
        fd = ::open(strbuf, O_RDWR|O_CREAT, 0666);
        if(fd<0){
            TWARN("open %s error, return!", strbuf);
            return -1;
        }
        if(ftruncate(fd, sizeof(FileQueueChunkInfo))){
            TWARN("ftruncate %s error, return!", strbuf);
            ::close(fd);
            return -1;
        }
        mem = mmap(0, sizeof(FileQueueChunkInfo), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
        if(mem==MAP_FAILED){
            TWARN("mmap %s error, return!", strbuf);
            ::close(fd);
            return -1;
        }
        bzero(mem, sizeof(FileQueueChunkInfo));
    }
    else{
        fd = ::open(strbuf, O_RDWR);
        if(fd<0){
            TWARN("open %s error, return!", strbuf);
            return -1;
        }
        if(fstat(fd, &ifstat)){
            TWARN("fstat %s error, return!", strbuf);
            ::close(fd);
            return -1;
        }
        if(ifstat.st_size!=sizeof(FileQueueChunkInfo)){
            TWARN("%s size error, return!", strbuf);
            ::close(fd);
            return -1;
        }
        mem = mmap(0, sizeof(FileQueueChunkInfo), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
        if(mem==MAP_FAILED){
            TWARN("mmap %s error, return!", strbuf);
            ::close(fd);
            return -1;
        }
    }

    _info = (FileQueueChunkInfo*)mem;
    _iffd = fd;

    return 0;
}

int FileQueueChunk::loadReadIndex(uint16_t fno)
{
    char strbuf[PATH_MAX+1];
    MMapInfo info;
    std::map<std::string, MMapInfo>::iterator it;
    snprintf(strbuf, PATH_MAX+1, "%s/%05u.%s", _dir, fno, INDEX_FILE_EXNAME);
    
    FileQueueChunk::_mmapLock.lock();
    it = _mmapTable.find(strbuf);
    if(it != _mmapTable.end()) {
        _rdIndex = (FileQueueChunkIndex*)it->second.mem;
        _rifno = fno;
        it->second.ref++;
        _mmapLock.unlock();
        return 0; 
    } else {
        int fd = ::open(strbuf, O_RDONLY);
        if(fd<0){
            _mmapLock.unlock();
            TWARN("open %s error, return!", strbuf);
            return -1;
        }
    
        struct stat ristat;
        if(fstat(fd, &ristat)){
            _mmapLock.unlock();
            TWARN("fstat %s error, return!", strbuf);
            ::close(fd);
            return -1;
        }
        if(ristat.st_size != (off_t)sizeof(FileQueueChunkIndex)*PERCHUNKINDEX_NODE_MAX){
            _mmapLock.unlock();
            TWARN(" %s size error, return!", strbuf);
            ::close(fd);
            return -1;
        }
        void* mem = mmap(0, sizeof(FileQueueChunkIndex)*PERCHUNKINDEX_NODE_MAX, PROT_READ, MAP_SHARED, fd, 0);
        if(mem==MAP_FAILED){
            _mmapLock.unlock();
            TWARN("mmap %s error, return!", strbuf);
            ::close(fd);
            return -1;
        }
        
        info.mem = mem;
        info.ref = 1;
        info.fd  = fd;
        std::pair<std::map<std::string,MMapInfo>::iterator,bool> ret;
        ret = _mmapTable.insert(std::make_pair<std::string,MMapInfo>(strbuf, info));
        if(ret.second == false) {
            _mmapLock.unlock();
            TWARN("mmap %s, insert table error, return!", strbuf);
            ::close(fd);
            return -1;
        }
        _mmapLock.unlock();
        _rdIndex = (FileQueueChunkIndex*)mem;
        _rifno = fno;
    }

    return 0;
}

int FileQueueChunk::loadWriteIndex(uint16_t fno)
{
    char strbuf[PATH_MAX+1] = {0};
    int fd = -1;
    void* mem = 0;
    struct stat wistat;

    snprintf(strbuf, PATH_MAX+1, "%s/%05u.%s", _dir, fno, INDEX_FILE_EXNAME);
    if(access(strbuf, F_OK)){
        fd = ::open(strbuf, O_RDWR|O_CREAT, 0666);
        if(fd<0){
            TWARN("open %s error, return!", strbuf);
            return -1;
        }
        if(ftruncate(fd, sizeof(FileQueueChunkIndex)*PERCHUNKINDEX_NODE_MAX)){
            TWARN("ftruncate %s error, return!", strbuf);
            ::close(fd);
            return -1;
        }
        mem = mmap(0, sizeof(FileQueueChunkIndex)*PERCHUNKINDEX_NODE_MAX, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
        if(mem==MAP_FAILED){
            TWARN("mmap %s error, return!", strbuf);
            ::close(fd);
            return -1;
        }
    }
    else{
        fd = ::open(strbuf, O_RDWR);
        if(fd<0){
            TWARN("open %s error, return!", strbuf);
            return -1;
        }
        if(fstat(fd, &wistat)){
            TWARN("fstat %s error, return!", strbuf);
            ::close(fd);
            return -1;
        }
        if(wistat.st_size != (off_t)sizeof(FileQueueChunkIndex)*PERCHUNKINDEX_NODE_MAX){
            TWARN("%s size error, return!", strbuf);
            ::close(fd);
            return -1;
        }
        mem = mmap(0, sizeof(FileQueueChunkIndex)*PERCHUNKINDEX_NODE_MAX, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
        if(mem==MAP_FAILED){
            TWARN("mmap %s error, return!", strbuf);
            ::close(fd);
            return -1;
        }
    }

    _wrIndex = (FileQueueChunkIndex*)mem;
    _wifno = fno;
    _wifd = fd;

    return 0;
}

int FileQueueChunk::openReadData(uint16_t fno)
{
    char strbuf[PATH_MAX+1] = {0};
    int fd = -1;
    snprintf(strbuf, PATH_MAX+1, "%s/%05u.%s", _dir, fno, DATA_FILE_EXNAME);
    fd = ::open(strbuf, O_RDONLY);
    if(fd<0){
        TWARN("open %s error, return!", strbuf);
        return -1;
    }
    _rdfd = fd;
    _rdfno = fno;
    return 0;
}

int FileQueueChunk::openWriteData(uint16_t fno)
{
    char strbuf[PATH_MAX+1] = {0};
    int fd = -1;
    snprintf(strbuf, PATH_MAX+1, "%s/%05u.%s", _dir, fno, DATA_FILE_EXNAME);
    fd = ::open(strbuf, O_WRONLY|O_CREAT, 0666);
    if(fd<0){
        TWARN("open %s error, return!", strbuf);
        return -1;
    }
    _wrfd = fd;
    _wrfno = fno;
    return 0;
}

void FileQueueChunk::closeInfo()
{
    if(_info){
        munmap(_info, sizeof(FileQueueChunkInfo));
        _info = 0;
    }
    if(_iffd>=0){
        ::close(_iffd);
        _iffd = -1;
    }
}

void FileQueueChunk::closeReadIndex()
{
    if(NULL == _rdIndex) return;
    
    char strbuf[PATH_MAX+1];
    std::map<std::string, MMapInfo>::iterator it;
    snprintf(strbuf, PATH_MAX+1, "%s/%05u.%s", _dir, _rifno, INDEX_FILE_EXNAME);
    
    _mmapLock.lock();
    it = _mmapTable.find(strbuf);
    if(it == _mmapTable.end()) {
        _mmapLock.unlock();
        TERR("must find at table, but not find %s", strbuf);
        return;
    }
    it->second.ref--;
    if(it->second.ref <= 0) {
        if(it->second.ref < 0) {
            TERR("ref can't less 0");
            return;
        }
        munmap(_rdIndex, sizeof(FileQueueChunkIndex)*PERCHUNKINDEX_NODE_MAX);
        ::close(it->second.fd);
        _rdIndex = NULL;
        _mmapTable.erase(it);
    }
    _mmapLock.unlock();
}

void FileQueueChunk::closeWriteIndex()
{
    if(_wrIndex){
        munmap(_wrIndex, sizeof(FileQueueChunkIndex)*PERCHUNKINDEX_NODE_MAX);
        _wrIndex = 0;
    }
    if(_wifd>=0){
        ::close(_wifd);
        _wifd = -1;
    }
}

void FileQueueChunk::closeReadData()
{
    if(_rdfd>=0){
        ::close(_rdfd);
        _rdfd = -1;
    }
}

void FileQueueChunk::closeWriteData()
{
    if(_wrfd){
        ::close(_wrfd);
        _wrfd = -1;
    }
}


#define FILE_QUEUE_INFO_NAME "info"
#define FILE_QUEUE_INDEX_NAME "index"
static uint32_t PERINDEX_NODE_MAX = (((((uint32_t)1)<<31)-1)/sizeof(FileQueueIndex));

static int loadMMap(void*& ptr, int& fd, const char* path, const uint32_t length, bool clean_new)
{
    if(access(path, F_OK)){
        fd = ::open(path, O_RDWR|O_CREAT, 0666);
        if(fd<0){
            TWARN("open %s error, return!", path);
            return -1;
        }
        if(ftruncate(fd, length)){
            TWARN("ftruncate %s error, return!", path);
            ::close(fd);
            fd = -1;
            return -1;
        }
        ptr = mmap(0, length, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
        if(ptr==MAP_FAILED){
            TWARN("mmap %s error, return!", path);
            ::close(fd);
            fd = -1;
            ptr = 0;
            return -1;
        }
        if(clean_new){
            bzero(ptr, length);
        }
    }
    else{
        fd = ::open(path, O_RDWR);
        if(fd<0){
            TWARN("open %s error, return!", path);
            return -1;
        }
        ptr = mmap(0, length, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
        if(ptr==MAP_FAILED){
            TWARN("mmap %s error, return!", path);
            ::close(fd);
            fd = -1;
            ptr = 0;
            return -1;
        }
    }
    return 0;
}

static void closeMMap(void* ptr, int fd, const uint32_t length)
{
    munmap(ptr, length);
    ::close(fd);
}

static uint32_t getNow()
{
    time_t now = time(0);
    struct tm tm_now;
    localtime_r(&now, &tm_now);
    return ((1900+tm_now.tm_year)*10000+(1+tm_now.tm_mon)*100+tm_now.tm_mday);
}

static uint32_t getYes()
{
    time_t yes = time(0)-86400;
    struct tm tm_yes;
    localtime_r(&yes, &tm_yes);
    return ((1900+tm_yes.tm_year)*10000+(1+tm_yes.tm_mon)*100+tm_yes.tm_mday);
}

pthread_mutex_t FileQueue::_switch_lock = PTHREAD_MUTEX_INITIALIZER;
FileQueue::FileQueue()
{
    _info = 0;
    _index = 0;
    _iffd = _idfd = 0;
    _inow = _iyes = 0;
    _yeschunk = _todchunk = 0;
    _yesindex = _todindex = 0;
    _read_seq = 0;
    _path[0] = 0;
    _opened = false;
    pthread_spin_init(&_lock, PTHREAD_PROCESS_PRIVATE);
    _info_lock.l_type = F_WRLCK;
    _info_lock.l_start = 0;
    _info_lock.l_whence = SEEK_SET;
    _info_lock.l_len = sizeof(FileQueueInfo);
    _info_unlock.l_type = F_UNLCK;
    _info_unlock.l_start = 0;
    _info_unlock.l_whence = SEEK_SET;
    _info_unlock.l_len = sizeof(FileQueueInfo);
}

FileQueue::~FileQueue()
{
    close();
    pthread_spin_destroy(&_lock);
}

int FileQueue::open(const char* path)
{
    char strbuf[PATH_MAX+1] = {0};
    struct stat dir_stat;

    if(!path){
        TWARN("argument path is null, return!");
        return -1;
    }
    if(access(path, F_OK)){
        if(mkdir(path, 0777)){
            TWARN("mkdir error, return![dir:%s]", path);
            return -1;
        }
    }
    else{
        if(stat(path, &dir_stat) || !S_ISDIR(dir_stat.st_mode)){
            TWARN("%s is not dir, return!", path);
            return -1;
        }
    }

    strncpy(_path, path, PATH_MAX);
    _path[PATH_MAX] = 0;

    snprintf(strbuf, PATH_MAX+1, "%s/%s", _path, FILE_QUEUE_INFO_NAME);
    if(loadMMap((void*&)_info, _iffd, strbuf, sizeof(FileQueueInfo), true)){
        TWARN("loadMMap %s error, return!", strbuf);
        return -1;
    }
    snprintf(strbuf, PATH_MAX+1, "%s/%s", _path, FILE_QUEUE_INDEX_NAME);
    if(loadMMap((void*&)_index, _idfd, strbuf, sizeof(FileQueueIndex)*PERINDEX_NODE_MAX, false)){
        TWARN("loadMMap %s error, return!", strbuf);
        closeMMap(_info, _iffd, sizeof(FileQueueInfo));
        _info = 0;
        _iffd = -1;
        return -1;
    }

    _opened = true;

    return 0;
}

int FileQueue::close()
{
    if(_opened){
        if(_info && _iffd>=0){
            closeMMap(_info, _iffd, sizeof(FileQueueInfo));
        }
        if(_index && _idfd>=0){
            closeMMap(_index, _idfd, sizeof(FileQueueIndex)*PERINDEX_NODE_MAX);
        }
        if(_todchunk){
            delete _todchunk;
        }
        if(_yeschunk){
            delete _yeschunk;
        }
        _info = 0;
        _index = 0;
        _iffd = _idfd = -1;
        _inow = _iyes = 0;
        _todchunk = _yeschunk = 0;
        _todindex = _yesindex = 0;
        _read_seq = 0;
        _path[0] = 0;
        _opened = false;
    }
    return 0;
}


int FileQueue::push(StorableMessage& msg)
{
    if(!_opened){
        TWARN("FileQueue %s not open, return!", _path);
        return -1;
    }
    pthread_spin_lock(&(_lock));
    if(switchChunk()){
        TWARN("switchChunk error, return!");
        pthread_spin_unlock(&(_lock));
        return -1;
    }
    msg.seq = _info->write_seq+1;
    if(_todchunk->set(msg)==-1){
        TWARN("set today chunk error, return!");
        pthread_spin_unlock(&(_lock));
        return -1;
    }
    _info->write_seq ++;
    if(_todindex->fseq==0){
        _todindex->fseq = _info->write_seq;
    }
    _todindex->lseq = _info->write_seq;
    pthread_spin_unlock(&(_lock));
    return 1;
}

int FileQueue::pop(StorableMessage& msg)
{
    int ret = 0;
    if(!_opened){
        TWARN("FileQueue %s not open, return!", _path);
        return -1;
    }
    pthread_spin_lock(&(_lock));
    if(switchChunk()){
        TWARN("switchChunk error, return!");
        pthread_spin_unlock(&(_lock));
        return -1;
    }
    if(_yeschunk){
        if(_read_seq==0){
            _read_seq = _todchunk->getMinSeq();
        }    
        else if(_read_seq<_yeschunk->getMinSeq()){
            _read_seq = _yeschunk->getMinSeq();
        }    
    }    
    else{
        if(_read_seq<_todchunk->getMinSeq()){
            _read_seq = _todchunk->getMinSeq();
        }    
    }    
    if(_read_seq==0 || _read_seq>_info->write_seq){
        pthread_spin_unlock(&(_lock));
        return 0;
    }    
    if(_yeschunk){
        if(_read_seq <= _yeschunk->getMaxSeq()){
            ret = _yeschunk->get(msg, _read_seq);
        }    
        else{
            ret = _todchunk->get(msg, _read_seq);
        }    
    }    
    else{
        ret = _todchunk->get(msg, _read_seq);
    }
    if(ret==1){
        _read_seq ++;
    }
    pthread_spin_unlock(&(_lock));
    return ret;
}

int FileQueue::pop(StorableMessage& msg, FileQueueSelect& sel)
{
    int ret = 0;
    if(!_opened){
        TWARN("FileQueue %s not open, return!", _path);
        return -1;
    }
    pthread_spin_lock(&(_lock));
    if(switchChunk()){
        TWARN("switchChunk error, return!");
        pthread_spin_unlock(&(_lock));
        return -1;
    }
    if(_yeschunk){
        if(_read_seq==0){
            _read_seq = _todchunk->getMinSeq();
        }    
        else if(_read_seq<_yeschunk->getMinSeq()){
            _read_seq = _yeschunk->getMinSeq();
        }    
    }    
    else{
        if(_read_seq<_todchunk->getMinSeq()){
            _read_seq = _todchunk->getMinSeq();
        }    
    }    
    if(_read_seq==0 || _read_seq>_info->write_seq){
        pthread_spin_unlock(&(_lock));
        return 0;
    }    
    
    sel.seq = _read_seq;
    if(_yeschunk) {
        if(_read_seq <= _yeschunk->getMaxSeq()){
            ret = _yeschunk->get(msg, sel);
        } else {
            ret = _todchunk->get(msg, sel);
        }    
    } else { 
        ret = _todchunk->get(msg, sel);
    }
    if(ret > 0) {
        _read_seq = sel.seq + 1;
    } else {
        _read_seq = sel.seq;
    }
    pthread_spin_unlock(&(_lock));
    return ret;
}

/* 查看队列头一条消息 */
int FileQueue::touch(StorableMessage& msg)
{
    int ret = 0;
    if(!_opened){
        TWARN("FileQueue %s not open, return!", _path);
        return -1;
    }
    pthread_spin_lock(&(_lock));
    if(switchChunk()){
        TWARN("switchChunk error, return!");
        pthread_spin_unlock(&(_lock));
        return -1; 
    }   
    if(_yeschunk){
        if(_read_seq==0){
            _read_seq = _todchunk->getMinSeq();
        }    
        else if(_read_seq<_yeschunk->getMinSeq()){
            _read_seq = _yeschunk->getMinSeq();
        }    
    }    
    else{
        if(_read_seq<_todchunk->getMinSeq()){
            _read_seq = _todchunk->getMinSeq();
        }    
    }    
    if(_read_seq==0 || _read_seq>_info->write_seq){
        pthread_spin_unlock(&(_lock));
        return 0;
    }    
    if(_yeschunk){
        if(_read_seq<_todchunk->getMinSeq()){
            ret = _yeschunk->get(msg, _read_seq);
        }    
        else{
            ret = _todchunk->get(msg, _read_seq);
        }    
    }    
    else{
        ret = _todchunk->get(msg, _read_seq);
    }   
    pthread_spin_unlock(&(_lock));
    return ret;
}

/* 翻过队列头一条消息 */
int FileQueue::next()
{
    if(!_opened){
        TWARN("FileQueue %s not open, return!", _path);
        return -1;
    }
    pthread_spin_lock(&(_lock));
    _read_seq ++;
    pthread_spin_unlock(&(_lock));
    return 1;
}

uint64_t FileQueue::reset(const uint64_t seq)
{
    if(!_opened){
        TWARN("FileQueue %s not open, return!", _path);
        return 0;
    }
    pthread_spin_lock(&(_lock));
    if(switchChunk()){
        TWARN("switchChunk error, return!");
        pthread_spin_unlock(&(_lock));
        return 0;
    }
    if(seq==0){
        _read_seq = _todchunk->getMinSeq();
    }
    else if(_yeschunk){
        if(seq<_yeschunk->getMinSeq()){
            _read_seq = _yeschunk->getMinSeq();
        }
        else{
            _read_seq = seq;
        }
    }
    else{
        if(seq<_todchunk->getMinSeq()){
            _read_seq = _todchunk->getMinSeq();
        }
        else{
            _read_seq = seq;
        }
    }
    pthread_spin_unlock(&(_lock));
    return _read_seq;
}

const uint64_t FileQueue::tail() const
{
    if(_opened && _info){
        return _info->write_seq;
    }
    else{
        return 0;
    }
}

const char* FileQueue::path() const
{
    if(_opened){
        return _path;
    }
    else{
        return 0;
    }
}

/* 获取队列中的数据数量 */
const uint64_t FileQueue::count() const
{
    if(_opened){
        if(_info->write_seq==_read_seq==0){
            return 0;
        }
        else{
            return (_info->write_seq-_read_seq+1);
        }
    }
    else{
        return 0;
    }
}

int FileQueue::switchChunk()
{
    bool isadd = false;
    struct stat dir_stat;
    if(!_todchunk || _inow!=getNow()){
        char strbuf[PATH_MAX+1] = {0}; 
        pthread_mutex_lock(&_switch_lock);
        fcntl(_iffd, F_SETLKW, &_info_lock);
        if(_yeschunk){
            delete _yeschunk;
            _iyes = 0;
            _yeschunk = 0;
            _yesindex = 0;
        }
        if(_todchunk){
            _iyes = _inow;
            _yeschunk = _todchunk;
            _yesindex = _todindex;
        }
        else{
            _iyes = getYes();
            snprintf(strbuf, PATH_MAX+1, "%s/%d/", _path, _iyes);
            if(access(strbuf, F_OK)==0 && stat(strbuf, &dir_stat)==0 && S_ISDIR(dir_stat.st_mode)){
                if(_info->index_count<2 || _iyes!=_index[_info->index_count-2].date){
                }
                else{
                    _yeschunk = new FileQueueChunk();
                    if(_yeschunk && _yeschunk->open(strbuf)){
                        delete _yeschunk;
                        _iyes = 0;
                        _yeschunk = 0;
                    }
                }
            }
            else{
                _iyes = 0;
            }
            if(_yeschunk){
                _yesindex = &(_index[_info->index_count-2]);
            }
        }
        _inow = getNow();
        _todchunk = 0; 
        _todindex = 0;
        snprintf(strbuf, PATH_MAX+1, "%s/%d/", _path, _inow);
        if(access(strbuf, F_OK) || stat(strbuf, &dir_stat) || !S_ISDIR(dir_stat.st_mode)){
            unlink(strbuf);
            if(_info->index_count==PERINDEX_NODE_MAX){
                TWARN("index_count=%lu get max, return!", _info->index_count);
                _inow = 0;
                fcntl(_iffd, F_SETLKW, &_info_unlock);
                pthread_mutex_unlock(&_switch_lock);
                return -1;
            }    
            isadd = true;
        }    
        else{
            if(_inow!=_index[_info->index_count-1].date){
                TWARN("_inow=%u, not match last index date=%lu, return!", _inow, _index[_info->index_count-1].date);
                _inow = 0;
                fcntl(_iffd, F_SETLKW, &_info_unlock);
                pthread_mutex_unlock(&_switch_lock);
                return -1;
            }    
        }    
        _todchunk = new FileQueueChunk();
        if(!_todchunk){
            TWARN("new today chunk error, return!");
            _inow = 0;
            fcntl(_iffd, F_SETLKW, &_info_unlock);
            pthread_mutex_unlock(&_switch_lock);
            return -1;
        }    
        if(_todchunk->open(strbuf)){
            TWARN("open today chunk error, return![path=%s]", strbuf);
            delete _todchunk;
            _inow = 0;
            _todchunk = 0; 
            fcntl(_iffd, F_SETLKW, &_info_unlock);
            pthread_mutex_unlock(&_switch_lock);
            return -1;
        }    
        if(isadd){
            _index[_info->index_count].date = _inow;
            _index[_info->index_count].fseq = 0; 
            _index[_info->index_count].lseq = 0; 
            _info->index_count ++;
        }    
        _todindex = &(_index[_info->index_count-1]);
        fcntl(_iffd, F_SETLKW, &_info_unlock);
        pthread_mutex_unlock(&_switch_lock);
    }
    return 0;
}

}
