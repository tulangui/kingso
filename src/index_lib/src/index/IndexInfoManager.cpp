//
#include <new>
#include <unistd.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "Common.h"
#include "util/MD5.h"
#include "util/errcode.h"
#include "util/idx_sign.h"
#include "index_lib/IndexLib.h"
#include "index_lib/IndexInfoManager.h"


INDEX_LIB_BEGIN

IndexInfoManager::IndexInfoManager()
{
    _fieldNum = 0;
    _maxDocNum = 0;
}

IndexInfoManager::~IndexInfoManager()
{

}

/**
 * 载入(保存)指定的路径下的倒排索引信息
 *
 * @param path 倒排索引信息文件路径(名称固定)
 *
 * @return < 0: error;  >= 0: success.
 */
int IndexInfoManager::load(const char* path)
{
    if (NULL == path || strlen(path) >= PATH_MAX) {
        return -1;
    }

    char fileName[PATH_MAX];
    snprintf(fileName, PATH_MAX, "%s/%s", path, INDEX_GLOBAL_INFO);

    struct stat st;
    if (stat(fileName, &st) || st.st_size <= 0) {
        return -1;
    }

    char* pGlobalInfoBuf = new (std::nothrow) char[st.st_size + 1];
    if (NULL == pGlobalInfoBuf) {
        return -1;
    }

    FILE* fp = fopen(fileName, "rb");
    if (NULL == fp || fread(pGlobalInfoBuf, st.st_size, 1, fp) != 1) {
        if (fp) fclose(fp);
        delete [] pGlobalInfoBuf;
        return -1;
    }
    fclose(fp);
    pGlobalInfoBuf[st.st_size] = 0;

    // 验证索引是否完整
    unsigned char md5Sign[16];
    util::MD5_CTX context;
    util::MD5Init(&context);
    util::MD5Update(&context, (const unsigned char*)pGlobalInfoBuf, st.st_size - 16);
    util::MD5Final(md5Sign, &context);
    if (memcmp(md5Sign, pGlobalInfoBuf + st.st_size - 16, 16)) {
        delete [] pGlobalInfoBuf;
        return -1;
    }

    char* p = pGlobalInfoBuf;
    int32_t fieldNum = *(int32_t*)p;  // fieldNum
    p += sizeof(int32_t);

    _maxDocNum = *(int32_t*)p;        // maxDocNum
    p += sizeof(int32_t);

    for (int32_t i = 0; i < fieldNum; i++) {
        char* fieldName = p;
        while (*p) {
            p++;
        }
        p++;      // 0x0
        uint32_t maxOccNum = *(int32_t*)p;  // maxOccNum
        p += sizeof(int32_t);

        _maxOccNum[_fieldNum] = maxOccNum;
        strcpy(_fieldName[_fieldNum], fieldName);

        _fieldNum++;
        if(_fieldNum >= MAX_INDEX_FIELD_NUM) {
            return -1;
        }
    }
    delete[] pGlobalInfoBuf;

    return _fieldNum;
}
int IndexInfoManager::save(const char* path)
{
    char* buf = new (std::nothrow) char[MAX_INDEX_FIELD_NUM * (PATH_MAX + sizeof(int32_t)) + 1024];
    if (NULL == buf) {
        return -1;
    }

    char* p = buf;
    *(uint32_t*)p = _fieldNum;
    p += sizeof(uint32_t);             // fieldNum
    *(int32_t*)p = _maxDocNum;         // maxDocNum
    p += sizeof(int32_t);

    for (int32_t i = 0; i < _fieldNum; i++)
    {
        p += sprintf(p, "%s", _fieldName[i]);     // fieldName
        *p++ = 0;

        *(int32_t*)p = _maxOccNum[i];
        p += sizeof(int32_t);                    // maxOccNum
    }

    util::MD5_CTX context;
    util::MD5Init(&context);
    util::MD5Update(&context, (const unsigned char*)buf, p - buf);
    util::MD5Final((unsigned char*)p, &context);
    p += 16;                          // md5 sign

    char fileName[PATH_MAX];
    snprintf(fileName, PATH_MAX, "%s/%s", path, INDEX_GLOBAL_INFO);
    FILE* fp = fopen(fileName, "wb");
    if (NULL == fp) {
        delete[] buf;
        return -1;
    }
    fwrite(buf, p - buf, 1, fp);
    fclose(fp);
    delete[] buf;

    return 0;
}

/**
 * 新加入一个字段信息
 *
 * @param fieldName 倒排字段名称
 * @param maxOccNum 字段的occ数量
 *
 * @return < 0: error; == 0:添加的字段重复 > 0: 当前字段数量
 */
int IndexInfoManager::addField(const char * fieldName, int maxOccNum)
{
    if ( NULL == fieldName )  return -1;

    if ( _fieldNum >= MAX_INDEX_FIELD_NUM
            || strlen( fieldName ) >= PATH_MAX - 1 )
    {
        return -1;
    }

    _maxOccNum[_fieldNum] = maxOccNum;
    strcpy(_fieldName[_fieldNum], fieldName);
    _fieldNum++;

    return _fieldNum;
}

/**
 * 设置(获取)Doc数量信息
 *
 * @param docNum doc数量信息
 *
 * @return < 0: error;  >= 0: success.
 */
int IndexInfoManager::setDocNum(int32_t docNum)
{
    _maxDocNum = docNum;

    return 1;
}


/**
 * 变量所有的字段信息
 *
 * @param pos    in/out 获取的字段序号，初始化为0，由函数内部修改
 * @param fieldName out 得到的字段名
 * @param maxOccNum out 对应字段的occ信息
 *
 * @return < 0: error;  >= 0: success.
 */
int IndexInfoManager::getNextField(uint32_t& pos, char*& fieldName, uint32_t& maxOccNum)
{
    if ( (int32_t)pos >= _fieldNum )  return -1;

    fieldName = _fieldName[pos];
    maxOccNum = _maxOccNum[pos];
    pos++;

    return pos;
}


INDEX_LIB_END
