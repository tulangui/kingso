/** ProfileEncodeFile.cpp
 *******************************************************************
 * $Author: pujian $
 *
 * $LastChangedBy: pujian $
 *
 * $Revision:  $
 *
 * $LastChangedDate: 2011-03-16 09:50:55 +0800 (Wed, 16 Mar 2011) $
 *
 * $Id: ProfileEncodeFile.cpp  2011-03-16 01:50:55 pujian $
 *
 * $Brief: 编码表类的实现 $
 *******************************************************************
*/
#include "index_lib/ProfileEncodeFile.h"

namespace index_lib
{

static KV32 strtokv32(const char * str) {
    KV32 kv;
    char *pdelim = NULL;
    pdelim = strchr(str, PROFILE_KV_DELIM);
    if (unlikely( pdelim == NULL )) {
        TWARN("INDEXLIB: Profile strtokv32 error, value [%s] not fit format!", str);
        kv.key   = INVALID_INT32;
        kv.value = INVALID_INT32;
        return kv;
    }

    kv.key   = strtol(str,      NULL, 10);
    kv.value = strtol(pdelim+1, NULL, 10);
    return kv;
}

ProfileEncodeFile::ProfileEncodeFile(PF_DATA_TYPE type, const char* path, const char* name, EmptyValue empty, bool single)
{
    // 存储编码表对应的路径和名称
    snprintf(_encodePath, PATH_MAX, "%s", path);
    snprintf(_encodeName, MAX_FIELD_NAME_LEN, "%s", name);

    // 初始化编码表内部成员变量
    _updateOverlap  = true;
    _emptyValue     = empty;
    _pHashTable     = NULL;
    _dataType       = type;
    _loadFlag       = false;
    _single         = single;
    _curEncodeValue = 0;
    _pVarintWrapper = NULL;
    _unitSize       = getDataTypeSize(type);
    _unitSizeDegree = getUnitSizeDegree(_unitSize);

    // 为编码表通用cnt文件空间进行变量初始化
    _pCntFile = NULL;
    for (uint32_t pos = 0; pos < MAX_ENCODE_SEG_NUM; pos++) {
        _cntFileAddr [pos] = NULL;
        _pNewCntFile [pos] = NULL;
    }
    _curContentSize   = 0;
    _totalContentSize = 0;
    _cntNewFileNum    = 0;
    _cntSegNum        = 0;
    _cntDegree        = 0;
    _cntMask          = 0;

    // 为单值string类型使用的ext文件空间进行变量初始化
    _pSingleStrExtFile = NULL;
    for (uint32_t pos = 0; pos < MAX_ENCODE_SEG_NUM; pos++) {
        _extFileAddr   [pos] = NULL;
        _pNewStrExtFile[pos] = NULL;
    }
    _extContentSize      = 0;
    _totalExtContentSize = 0;
    _extNewFileNum       = 0;
    _extSegNum           = 0;
    _extDegree           = 0;
    _extMask             = 0;

    // 初始化同步管理器为NULL
    _syncMgr             = NULL;
    _desc_fd             = -1;
}

ProfileEncodeFile::~ProfileEncodeFile()
{
    // 释放编码表资源
    freeResource();
}

void ProfileEncodeFile::freeResource()
{
    // 重置编码值和数据存储大小
    _curEncodeValue      = 0;
    _curContentSize      = 0;
    _totalContentSize    = 0;
    _extContentSize      = 0;
    _totalExtContentSize = 0;

    SAFE_DELETE( _pCntFile );          // 释放全量cnt文件的mmap映射对象

    // 释放新增cnt分段文件对应的mmap映射对象
    for (uint32_t pos = 0; pos < _cntNewFileNum; pos++)
    {
        SAFE_DELETE( _pNewCntFile[pos] );
    }
    _cntNewFileNum  = 0;
    _cntSegNum      = 0;
    _cntDegree      = 0;
    _cntMask        = 0;

    SAFE_DELETE( _pSingleStrExtFile ); // 释放全量single string ext文件的mmap映射对象

    // 释放新增ext分段文件对应的mmap映射对象
    for (uint32_t pos = 0; pos < _extNewFileNum; pos++)
    {
        SAFE_DELETE( _pNewStrExtFile[pos] );
    }
    _extNewFileNum     = 0;
    _extSegNum         = 0;
    _extDegree         = 0;
    _extMask           = 0;


    SAFE_DELETE( _pHashTable );        // 释放哈希表
    SAFE_DELETE( _pVarintWrapper );    // 释放varint压缩包装对象

    // 释放描述文件句柄
    if (_desc_fd > 0) {
        ::close(_desc_fd);
        _desc_fd = -1;
    }
}

/**
 * 根据字段类型获取cnt文件单元的长度
 */
size_t ProfileEncodeFile::getDataTypeSize(PF_DATA_TYPE type)
{
    if (type == DT_INT8  || type == DT_UINT8)  return 1;
    if (type == DT_INT16 || type == DT_UINT16) return 2;
    if (type == DT_INT32 || type == DT_UINT32) return 4;
    if (type == DT_INT64 || type == DT_UINT64) return 8;
    if (type == DT_FLOAT)                      return sizeof(float);
    if (type == DT_DOUBLE)                     return sizeof(double);
    if (type == DT_KV32)                       return sizeof(KV32);

    return 1;
}


/**
 * 创建编码表(用于建库过程)
 */
bool    ProfileEncodeFile::createEncodeFile(bool encode, bool overlap, uint32_t hashsize, uint32_t blocknum)
{
    _loadFlag = false;         // 设置加载标识为false，加载编码表时为true
    freeResource();            // 释放原有资源，如果存在的话
    setEncodeUnitSize(encode); // 计算并设置字段存储单元大小

    // 计算映射文件的初始化空间大小
    bool   ret        = false;
    char   szEncodeCntFile[PATH_MAX];
    char   szEncodeExtFile[PATH_MAX];

    // 拼接文件名
    snprintf(szEncodeCntFile,  PATH_MAX, "%s/%s%s", _encodePath, _encodeName, ENCODE_CNT_SUFFIX);
    snprintf(szEncodeExtFile,  PATH_MAX, "%s/%s%s", _encodePath, _encodeName, ENCODE_EXT_SUFFIX);

    do {
        // 创建哈希表
        _pHashTable = new ProfileHashTable();
        if (unlikely(_pHashTable == NULL
                     || !_pHashTable->initHashTable(hashsize, sizeof(uint32_t))))
        {
            break;
        }

        // 创建编码表通用的cnt文件对应的内存映射文件
        _pCntFile  = new ProfileMMapFile(szEncodeCntFile);
        if (unlikely(_pCntFile == NULL)) {
            break;
        }
        size_t cntFileLen = blocknum * _unitSize;
        ret = _pCntFile->open(false, false, cntFileLen);
        if (unlikely(!ret)) {
            break;
        }
        _totalContentSize = cntFileLen;

        // 如果是单值字符串类型，则创建单值字符串类型需要的扩展文件
        if (_single && (_dataType == DT_STRING)) {
            _pSingleStrExtFile = new ProfileMMapFile(szEncodeExtFile);
            if (unlikely(_pSingleStrExtFile == NULL)) {
                break;
            }
            size_t extFileLen = blocknum;
            ret = _pSingleStrExtFile->open(false, false, extFileLen);
            if (unlikely(!ret)) {
                break;
            }
            _totalExtContentSize = extFileLen;
        }

        _updateOverlap = overlap;
        return true;

    }while(0);

    // 创建失败，释放资源
    SAFE_DELETE( _pHashTable );
    SAFE_DELETE( _pCntFile );
    SAFE_DELETE( _pSingleStrExtFile );
    SAFE_DELETE( _pVarintWrapper );

    return false;
}

bool    ProfileEncodeFile::loadEncodeFile(bool update)
{
    _loadFlag = true;      // 设置加载标识为true（加载编码表情况）
    freeResource();        // 释放原有资源

    // 拼接文件名
    char szEncodeIdxFileName[PATH_MAX];   // .idx文件
    char szEncodeCntFile    [PATH_MAX];   // .cnt文件
    char szEncodeDescFile   [PATH_MAX];   // .desc文件
    char szEncodeExtFile    [PATH_MAX];   // .ext文件(单值字符串类型专用)
    snprintf(szEncodeCntFile,     PATH_MAX, "%s/%s%s", _encodePath, _encodeName, ENCODE_CNT_SUFFIX);
    snprintf(szEncodeDescFile,    PATH_MAX, "%s/%s%s", _encodePath, _encodeName, ENCODE_DESC_SUFFIX);
    snprintf(szEncodeIdxFileName, PATH_MAX, "%s%s",    _encodeName, ENCODE_IDX_SUFFIX);
    snprintf(szEncodeExtFile,     PATH_MAX, "%s/%s%s", _encodePath, _encodeName, ENCODE_EXT_SUFFIX);

    bool ret = false;
    do {
        // 加载.desc编码表描述文件
        if (unlikely(!loadEncodeDescFile(szEncodeDescFile))) {
            break;
        }

        // 加载哈希表.idx文件
        if (_updateOverlap) {
            _pHashTable = new ProfileHashTable();
            if (unlikely(_pHashTable == NULL
                        || !_pHashTable->loadHashTable(_encodePath, szEncodeIdxFileName)))
            {
                break;
            }
        }
        else {
            _pHashTable = NULL;
        }

        // 加载字段值存储文件.cnt文件
        _pCntFile  = new ProfileMMapFile(szEncodeCntFile);
        if (unlikely(_pCntFile == NULL)) {
            break;
        }
        ret = _pCntFile->open(false, true);
        if (unlikely(!ret)) {
            break;
        }

        // 如果是单值字符串类型，则加载扩展文件，加载扩展存储文件.ext文件
        if (_single && _dataType == DT_STRING) {
            _pSingleStrExtFile = new ProfileMMapFile(szEncodeExtFile);
            if (unlikely(_pSingleStrExtFile == NULL)) {
                break;
            }
            ret = _pSingleStrExtFile->open(false, true);
            if (unlikely(!ret)) {
                break;
            }
        }

        // 将加载的编码表文件进行分段计算
        if (unlikely(!loadEncodeSegInfo(update))) {
            break;
        }
        return true;

    }while(0);

    // 加载过程失败，释放资源
    SAFE_DELETE( _pHashTable );
    SAFE_DELETE( _pCntFile );
    SAFE_DELETE( _pSingleStrExtFile );
    SAFE_DELETE( _pVarintWrapper );

    return false;
}




/**
 * 加载字段编码文件,用于重新修改
 * @return  true加载成功；false 加载失败
 */
bool  ProfileEncodeFile::loadEncodeFileForModify()
{
    _loadFlag = false;                                               // 设置加载标识为true（加载编码表情况）
    freeResource();                                                  // 释放原有资源

    char szIdxFileName[ PATH_MAX ] = {0};                            // .idx文件
    char szCntFile    [ PATH_MAX ] = {0};                            // .cnt文件
    char szDescFile   [ PATH_MAX ] = {0};                            // .desc文件
    char szExtFile    [ PATH_MAX ] = {0};                            // .ext文件(单值字符串类型专用)

    snprintf( szCntFile,     PATH_MAX, "%s/%s%s", _encodePath, _encodeName, ENCODE_CNT_SUFFIX);
    snprintf( szDescFile,    PATH_MAX, "%s/%s%s", _encodePath, _encodeName, ENCODE_DESC_SUFFIX);
    snprintf( szIdxFileName, PATH_MAX, "%s%s",                 _encodeName, ENCODE_IDX_SUFFIX);
    snprintf( szExtFile,     PATH_MAX, "%s/%s%s", _encodePath, _encodeName, ENCODE_EXT_SUFFIX);

    if ( !loadEncodeDescFile( szDescFile ) )  goto failed;           // 加载.desc编码表描述文件

    if ( true ==  _updateOverlap )                                   // 有hash表
    {
        _pHashTable = new ProfileHashTable();                        // 加载哈希表.idx文件

        if ( _pHashTable == NULL )  goto failed;
        if ( !_pHashTable->loadHashTable( _encodePath, szIdxFileName ) )
            goto failed;
    }
    else
    {
        _pHashTable = NULL;                                          // 没有hash表
    }

    _pCntFile  = new ProfileMMapFile( szCntFile );                   // 加载字段值存储文件.cnt文件

    if ( _pCntFile == NULL )                  goto failed;
    if ( ! _pCntFile->open( false, false ) )  goto failed;

    _pCntFile->preload();
    _totalContentSize = _pCntFile->getLength();

    if ( _single && _dataType == DT_STRING )                         // 单值字符串类型，加载扩展存储文件.ext文件
    {
        _pSingleStrExtFile = new ProfileMMapFile( szExtFile );

        if ( _pSingleStrExtFile == NULL )                   goto failed;
        if ( ! _pSingleStrExtFile->open( false, false ) )   goto failed;
    }

    return true;

failed:
    // 加载过程失败，释放资源
    SAFE_DELETE( _pHashTable );
    SAFE_DELETE( _pCntFile );
    SAFE_DELETE( _pSingleStrExtFile );
    SAFE_DELETE( _pVarintWrapper );

    return false;
}





/**
 * 将编码表数据持久化到硬盘
 */
bool    ProfileEncodeFile::flushDescFile(bool sync)
{
    char szEncodeDescFile    [PATH_MAX];
    // 拼接文件名
    snprintf(szEncodeDescFile,    PATH_MAX, "%s/%s%s", _encodePath, _encodeName, ENCODE_DESC_SUFFIX);

    do {
        // 打开描述信息文件
        if (_desc_fd == -1) {
            _desc_fd = ::open(szEncodeDescFile, O_RDWR|O_CREAT, 0644);
            if (unlikely(_desc_fd == -1)) {
                break;
            }
        }
        else {
            lseek(_desc_fd, 0, SEEK_SET);
        }

        encode_desc_cnt_info   cnt_desc;   // .desc文件持久化数据的结构体1
        encode_desc_ext_info   ext_desc;   // .desc文件持久化数据的结构体2
        encode_desc_meta_info  meta_desc;  // .desc文件持久化数据的结构体3

        cnt_desc.cur_encode_value = _curEncodeValue;
        cnt_desc.cur_content_size = _curContentSize;
        cnt_desc.cntNewFileNum    = _cntNewFileNum;
        cnt_desc.cntSegNum        = _cntSegNum;

        ext_desc.cur_extend_size  = _extContentSize;
        ext_desc.extNewFileNum    = _extNewFileNum;
        ext_desc.extSegNum        = _extSegNum;

        meta_desc.type            = _dataType;
        meta_desc.unit_size       = _unitSize;
        meta_desc.update_overlap  = _updateOverlap;
        meta_desc.cntDegree       = _cntDegree;
        meta_desc.cntMask         = _cntMask;
        meta_desc.extDegree       = _extDegree;
        meta_desc.extMask         = _extMask;
        if (_pVarintWrapper == NULL) {
            meta_desc.varintEncode = false;
        }
        else {
            meta_desc.varintEncode = true;
        }

        // 写入描述信息
        size_t rnum = ::write(_desc_fd, &cnt_desc, sizeof(encode_desc_cnt_info));
        if (unlikely(rnum != sizeof(encode_desc_cnt_info))) {
            break;
        }

        if (sync || (_dataType == DT_STRING && _single)) {
            rnum = ::write(_desc_fd, &ext_desc, sizeof(encode_desc_ext_info));
            if (unlikely(rnum != sizeof(encode_desc_ext_info))) {
                break;
            }
        }

        if (sync) {
            rnum = ::write(_desc_fd, &meta_desc, sizeof(encode_desc_meta_info));
            if (unlikely(rnum != sizeof(encode_desc_meta_info))) {
                break;
            }
        }
        return true;

    }while(0);

    // dump过程失败，关闭描述文件
    if (_desc_fd > 0) {
        close(_desc_fd);
        _desc_fd = -1;
    }
    return false;
}


/**
 * 将编码表数据持久化到硬盘
 */
bool    ProfileEncodeFile::dumpEncodeFile()
{
    FILE * fd = NULL;
    char szEncodeIdxFileName [PATH_MAX];
    char szEncodeDescFile    [PATH_MAX];
    // 拼接文件名
    snprintf(szEncodeDescFile,    PATH_MAX, "%s/%s%s", _encodePath, _encodeName, ENCODE_DESC_SUFFIX);
    snprintf(szEncodeIdxFileName, PATH_MAX, "%s%s",    _encodeName, ENCODE_IDX_SUFFIX);

    do {
        // 打开描述信息文件
        fd = fopen(szEncodeDescFile, "w");
        if (unlikely(fd == NULL)) {
            break;
        }

        encode_desc_cnt_info   cnt_desc;   // .desc文件持久化数据的结构体1
        encode_desc_ext_info   ext_desc;   // .desc文件持久化数据的结构体2
        encode_desc_meta_info  meta_desc;  // .desc文件持久化数据的结构体3

        cnt_desc.cur_encode_value = _curEncodeValue;
        cnt_desc.cur_content_size = _curContentSize;
        cnt_desc.cntNewFileNum    = _cntNewFileNum;
        cnt_desc.cntSegNum        = _cntSegNum;

        ext_desc.cur_extend_size  = _extContentSize;
        ext_desc.extNewFileNum    = _extNewFileNum;
        ext_desc.extSegNum        = _extSegNum;

        meta_desc.type            = _dataType;
        meta_desc.unit_size       = _unitSize;
        meta_desc.update_overlap  = _updateOverlap;
        meta_desc.cntDegree       = _cntDegree;
        meta_desc.cntMask         = _cntMask;
        meta_desc.extDegree       = _extDegree;
        meta_desc.extMask         = _extMask;
        if (_pVarintWrapper == NULL) {
            meta_desc.varintEncode = false;
        }
        else {
            meta_desc.varintEncode = true;
        }

        // 写入描述信息
        size_t rnum = fwrite(&cnt_desc, sizeof(encode_desc_cnt_info), 1, fd);
        if (unlikely(rnum != 1)) {
            break;
        }
        rnum = fwrite(&ext_desc, sizeof(encode_desc_ext_info), 1, fd);
        if (unlikely(rnum != 1)) {
            break;
        }
        rnum = fwrite(&meta_desc, sizeof(encode_desc_meta_info), 1, fd);
        if (unlikely(rnum != 1)) {
            break;
        }

        // dump哈希表
        if (unlikely(_updateOverlap
                    && (_pHashTable == NULL
                        || !_pHashTable->dumpHashTable(_encodePath, szEncodeIdxFileName, true))))
        {
            break;
        }
        // dump字段值实际内容
        if (unlikely(_pCntFile == NULL || !_pCntFile->flush(true))) {
            break;
        }

        // dump ext文件内容
        if (_single && _dataType == DT_STRING) {
            if (unlikely(_pSingleStrExtFile == NULL || !_pSingleStrExtFile->flush(true))) {
                break;
            }
        }

        if (!_loadFlag) {
            // 初始建库过程，建库结束resize .cnt文件（字段值实际内容大小）
            size_t cntFileSize = _curContentSize;
            if (cntFileSize != 0) {
                if (unlikely(!_pCntFile->resize(cntFileSize))) {
                    break;
                }
            }
            // 单值字符串字段，resize .ext文件
            if (_single && _dataType == DT_STRING) {
                size_t extFileSize = _extContentSize;
                if (extFileSize != 0) {
                    if (unlikely(!_pSingleStrExtFile->resize(extFileSize))) {
                        break;
                    }
                }
            }
        }
        else {
            // 持久化增量添加的.cnt分段文件
            for (uint32_t pos = 0; pos < _cntNewFileNum; pos++) {
                if (unlikely(_pNewCntFile[pos] == NULL)) {
                    TERR("Dump EncodeFile: %s/%s%s_%u error!", _encodePath, _encodeName, ENCODE_CNT_SUFFIX, pos);
                    continue;
                }
                _pNewCntFile[pos]->flush(true);
            }

            // 持久化增量添加的.ext分段文件
            for (uint32_t pos = 0; pos < _extNewFileNum; pos++) {
                if (unlikely(_pNewStrExtFile[pos] == NULL)) {
                    TERR("Dump EncodeFile: %s/%s%s_%u error!", _encodePath, _encodeName, ENCODE_EXT_SUFFIX, pos);
                    continue;
                }
                _pNewStrExtFile[pos]->flush(true);
            }
        }

        // 关闭描述文件
        fclose(fd);
        fd = NULL;
        return true;

    }while(0);

    // dump过程失败，关闭描述文件
    if (fd != NULL) {
        fclose(fd);
        fd = NULL;
    }
    return false;
}

/**
 * 向编码表中添加新的字段值
 */
bool    ProfileEncodeFile::addEncode(const char* str, uint32_t len, uint32_t &offset, char delim)
{
    // 有效性检查
    if (unlikely(str == NULL || _pCntFile == NULL
            || _curEncodeValue == INVALID_ENCODEVALUE_NUM ))
    {
        return false;
    }

    if ( false == _single )                      // 添加多值编码字段
    {
        return addMultiEncodeValue(str, len, delim, offset);
    }
    else                                         // 添加单值编码字段
    {
        return addSingleEncodeValue(str, len, offset);
    }

    return false;
}


/**
 * 向编码表中添加单值编码字段的字段值
 */
bool  ProfileEncodeFile::addSingleEncodeValue(const char* str, uint32_t len, uint32_t &offset)
{
    // 计算字符串哈希值(如果需要排重)
    uint64_t sign    = 0;
    if (_pHashTable != NULL) {
        sign = ProfileHashTable::hash(str, len);
        // 查找字符串对应的编码值是否存在
        uint32_t *pValue = (uint32_t *)_pHashTable->findValuePtr(sign);
        if (pValue != NULL) {
            return false;
        }
    }

    if (likely(_dataType != DT_STRING)) {
        // 添加单值基本类型字段值
        return addSingleBaseTypeEncode(sign, str, len, offset);
    }
    else {
        // 添加单值字符串类型的字段值
        return addSingleStringTypeEncode(sign, str, len, offset);
    }
    return false;
}

/**
 * 向编码表中添加多值字段的字段值
 */
bool  ProfileEncodeFile::addMultiEncodeValue(const char* str, uint32_t len, char delim, uint32_t &offset)
{
    uint64_t sign = 0;                                               // 计算字符串哈希值

    if ( _pHashTable != NULL )                                       // 查找字符串对应的编码值是否存在
    {
        sign = ProfileHashTable::hash(str, len);

        uint32_t * pValue = (uint32_t *)_pHashTable->findValuePtr( sign );

        if ( pValue != NULL )  return false;                         // 已经存在，啥也不用干了
    }

    if (unlikely( str[0] == '\0') )                                  // 添加空值情况
    {
        uint8_t         * pCntAddr    = NULL;                        // 写入索引的内存起始位置
        ProfileMMapFile * pTargetFile = NULL;

        if ( !_loadFlag )
        {
            if (unlikely( _curContentSize + sizeof(uint16_t) > _totalContentSize ))    // 扩展一下文件
            {
                if ( _pCntFile->makeNewPage(_totalContentSize / 5) == 0 )
                    return false;

                _totalContentSize = _pCntFile->getLength();
            }

            pCntAddr = (uint8_t *)_pCntFile->safeOffset2Addr(_curContentSize);
        }
        else
        {
            while (unlikely( _curContentSize + sizeof(uint16_t) > _totalContentSize ))
            {
                if ( !newContentSegmentFile() )
                    return false;
            }

            uint32_t  segPos    = _curContentSize >> _cntDegree;
            uint32_t  inSegPos  = _curContentSize &  _cntMask;

            pCntAddr    = (uint8_t *)(_cntFileAddr[segPos] + inSegPos);
            pTargetFile = getMMapFileBySegNum(segPos);
        }

        *(uint16_t *)pCntAddr = 0;                                               // 设置字段值个数对应的元数据

        uint32_t curOffset = (uint32_t)(_curContentSize >> 1);

        // 添加字段读取偏移信息到哈希表中
        if ( _pHashTable != NULL && _pHashTable->addValuePtr( sign, &curOffset ) != 0 )
            return false;

        offset = curOffset;
        size_t wlen = resetCurContentSize(sizeof(uint16_t));
        putSyncInfo((char *)pCntAddr, wlen, pTargetFile);

        // 设置编码表元数据
        ++_curEncodeValue;
        return true;
    }

    if ( _pVarintWrapper == NULL )                                   // 不需要varint压缩的情况
    {
        return addNoVarintEncode( sign, str, len, delim, offset );
    }
    else
    {                                                                // 需要varint压缩的情况
        return addVarintEncode( sign, str, len, delim, offset );
    }
}



/**
 * 解析并添加单值基本类型(float/double/int/uint)字段值，被addSingleEncodeValue调用
 * @param  sign      字段值对应的哈希值
 * @param  str       字段值的字符串
 * @param  len       字段值的字符串长度
 * @param  offset    编码存储的偏移位置(单元位置)
 * @return           true, OK; false, ERROR
 */
bool  ProfileEncodeFile::addSingleBaseTypeEncode(int64_t sign, const char* str, uint32_t len, uint32_t &offset)
{
    char * pCntAddr = NULL;          //字段值起始地址
    ProfileMMapFile * pFile = NULL;
    if (!_loadFlag) {
        if (unlikely((_curContentSize + _unitSize) > _totalContentSize)) {
            if (_pCntFile->makeNewPage(_totalContentSize / 5) == 0) {
                return false;
            }
            _totalContentSize = _pCntFile->getLength();
        }
        pCntAddr = _pCntFile->safeOffset2Addr(_curContentSize);
    }
    else {
        while (unlikely((_curContentSize + _unitSize) > _totalContentSize)) {
            if (!newContentSegmentFile()) {
                return false;
            }
        }
        uint32_t segPos          = _curContentSize >> _cntDegree;
        uint32_t inSegPos        = _curContentSize &  _cntMask;
        pCntAddr = _cntFileAddr[segPos] + inSegPos;
        pFile    = getMMapFileBySegNum(segPos);
    }

    switch(_dataType) {
        case DT_INT32:
            {
                if (unlikely(len == 0)) {
                    *((int32_t *)pCntAddr) = _emptyValue.EV_INT32;
                }
                else {
                    *((int32_t *)pCntAddr) = strtol(str, NULL, 10);
                }
            }
            break;

        case DT_UINT32:
            {
                if (unlikely(len == 0)) {
                    *((uint32_t *)pCntAddr) = _emptyValue.EV_UINT32;
                }
                else {
                    *((uint32_t *)pCntAddr) = strtoul(str, NULL, 10);
                }
            }
            break;

        case DT_INT64:
            {
                if (unlikely(len == 0)) {
                    *((int64_t *)pCntAddr) = _emptyValue.EV_INT64;
                }
                else {
                    *((int64_t *)pCntAddr) = strtoll(str, NULL, 10);
                }
            }
            break;

        case DT_UINT64:
            {
                if (unlikely(len == 0)) {
                    *((uint64_t *)pCntAddr) = _emptyValue.EV_UINT64;
                }
                else {
                    *((uint64_t *)pCntAddr) = strtoull(str, NULL, 10);
                }
            }
            break;

        case DT_FLOAT:
            {
                if (unlikely(len == 0)) {
                    *((float *)pCntAddr) = _emptyValue.EV_FLOAT;
                }
                else {
                    *((float *)pCntAddr) = strtof(str, NULL);
                }
            }
            break;

        case DT_DOUBLE:
            {
                if (unlikely(len == 0)) {
                    *((double *)pCntAddr) = _emptyValue.EV_DOUBLE;
                }
                else {
                    *((double *)pCntAddr) = strtod(str, NULL);
                }
            }
            break;

        case DT_INT16:
            {
                if (unlikely(len == 0)) {
                    *((int16_t *)pCntAddr) = _emptyValue.EV_INT16;
                }
                else {
                    *((int16_t *)pCntAddr) = (int16_t)strtol(str, NULL, 10);
                }
            }
            break;

        case DT_UINT16:
            {
                if (unlikely(len == 0)) {
                    *((uint16_t *)pCntAddr) = _emptyValue.EV_UINT16;
                }
                else {
                    *((uint16_t *)pCntAddr) = (uint16_t)strtoul(str, NULL, 10);
                }
            }
            break;

        case DT_INT8:
            {
                if (unlikely(len == 0)) {
                    *((int8_t *)pCntAddr) = _emptyValue.EV_INT8;
                }
                else {
                    *((int8_t *)pCntAddr) = (int8_t)strtol(str, NULL, 10);
                }
            }
            break;

        case DT_UINT8:
            {
                if (unlikely(len == 0)) {
                    *((uint8_t *)pCntAddr) = _emptyValue.EV_UINT8;
                }
                else {
                    *((uint8_t *)pCntAddr) = (uint8_t)strtoul(str, NULL, 10);
                }
            }
            break;

        default:
            return false;
    }

    // 添加单值字段的查询编码值到哈希表中
    uint32_t curOffset = _curContentSize >> _unitSizeDegree;
    if (_pHashTable != NULL && _pHashTable->addValuePtr(sign, &curOffset) != 0) {
        return false;
    }
    offset = curOffset;

    // 设置编码表元数据
    _curContentSize += _unitSize;
    ++_curEncodeValue;
    putSyncInfo((char *)pCntAddr, _unitSize, pFile);
    return true;
}

/**
 * 解析并添加单值字符串类型(DT_STRING)字段值，被addSingleEncodeValue调用
 * @param  sign      字段值对应的哈希值
 * @param  str       字段值的字符串
 * @param  len       字段值的字符串长度
 * @param  offset    编码存储的偏移位置(单元位置)
 * @return           true, OK; false, ERROR
 */
bool  ProfileEncodeFile::addSingleStringTypeEncode(int64_t sign, const char* str, uint32_t len, uint32_t &offset)
{
    SingleString *pCntAddr = NULL;          //字段值起始地址
    char         *pExtAddr = NULL;          //字段扩展存储地址

    ProfileMMapFile *pTargetCntFile = NULL;
    ProfileMMapFile *pTargetExtFile = NULL;

    if (!_loadFlag) {
        // 建库过程
        if (unlikely((_curContentSize + _unitSize) > _totalContentSize)) {
            // 如果cnt文件空间不足，扩展20%
            if (_pCntFile->makeNewPage(_totalContentSize / 5) == 0) {
                return false;
            }
            _totalContentSize = _pCntFile->getLength();
        }
        pCntAddr = (SingleString *)_pCntFile->safeOffset2Addr(_curContentSize);

        if (unlikely(len >= 7)) {  // 如果字符串长度>=7，则查找ext文件地址
            if (unlikely((_extContentSize + len) >= _totalExtContentSize)) {
                if (_pSingleStrExtFile->makeNewPage(_totalExtContentSize / 5) == 0) {
                    return false;
                }
                _totalExtContentSize = _pSingleStrExtFile->getLength();
            }
            pExtAddr = _pSingleStrExtFile->safeOffset2Addr(_extContentSize);
        }
    }
    else {
        // 加载索引更新过程
        while (unlikely((_curContentSize + _unitSize) > _totalContentSize)) {
            if (!newContentSegmentFile()) {
                return false;
            }
        }
        uint32_t segPos   = _curContentSize >> _cntDegree;
        uint32_t inSegPos = _curContentSize &  _cntMask;
        pCntAddr = (SingleString *)(_cntFileAddr[segPos] + inSegPos);
        pTargetCntFile = getMMapFileBySegNum(segPos);

        if (unlikely(len >= 7)) {
            if (unlikely((_extContentSize + len) >= _totalExtContentSize)) {
                if (!newExtendSegmentFile()) {
                    return false;
                }
            }
            uint32_t extSegPos   = _extContentSize >> _extDegree;
            uint32_t extInSegPos = _extContentSize &  _extMask;
            pExtAddr = _extFileAddr[extSegPos] + extInSegPos;
            pTargetExtFile = getMMapFileBySegNum(extSegPos, false);
        }
    }

    memset(pCntAddr, 0, _unitSize);
    if (len == 0) {
        //空值
        pCntAddr->flag = 0;
    }
    else if (len < 7) {
        //字符串存储在cnt文件中
        pCntAddr->flag = 1;
        snprintf(pCntAddr->payload.str, 7, "%s", str);
    }
    else {
        //字符串存储在ext文件中，cnt文件中存储偏移量
        pCntAddr->flag = 2;
        memcpy(pExtAddr, str, (len + 1));
        pCntAddr->payload.offset = _extContentSize;
        _extContentSize += (len + 1);
        putSyncInfo((char *)pExtAddr, len + 1, pTargetExtFile);
    }
    putSyncInfo((char *)pCntAddr, _unitSize, pTargetCntFile);

    // 添加单值字符串对应的编码值到哈希表中
    uint32_t curOffset = _curContentSize >> _unitSizeDegree;
    if (_pHashTable != NULL && _pHashTable->addValuePtr(sign, &curOffset) != 0) {
        return false;
    }
    offset = curOffset;

    // 设置编码表元数据
    _curContentSize += _unitSize;
    ++_curEncodeValue;
    return true;
}

/**
 * 解析并添加字段值，不进行varint压缩
 * @param  sign      字段值对应的哈希值
 * @param  str       字段值的字符串
 * @param  len       字段值的字符串长度
 * @param  delem     字段值的分隔符
 * @param  offset    字段值的偏移量
 * @return           true, OK; false, ERROR
 */
bool  ProfileEncodeFile::addNoVarintEncode(int64_t sign, const char* str, uint32_t len, char delim, uint32_t &offset)
{
    uint32_t    num      = 0;             //字段值个数
    const char* begin    = str;           //单个字段值的起始位置
    const char* end      = NULL;          //单个字段值的终止位置
    uint16_t    str_len  = 0;             //单个字段值的字符串长度
    char        vbuf [MAX_VALUE_STR_LEN]; //字段值临时存储buffer
    int         incr_len = 0;             //新添加字段内容占用的空间大小
    char        valueArr[MAX_PROFILE_ORGINSTR_LEN];   //原始值buffer

    // 解析字段值原始字符串，将字符串转换为二进制保存在valueArr中
    end = strchr(begin, delim);
    if (_dataType == DT_INT32 || _dataType == DT_UINT32) {
        uint32_t *pValueArr = (uint32_t *)valueArr;
        while(end != NULL) {
            // 非最后一个字段值
            if (unlikely(num == MAX_ENCODE_VALUE_NUM)) {
                TWARN("INDEXLIB: ProfileEncodeFile addNoVarintEncode error, value [%s] num over %u !", str, MAX_ENCODE_VALUE_NUM);
                break;
            }
            str_len = end - begin;
            if (str_len != 0) {
                // 数值类型，进行转换再存储
                snprintf(vbuf, ((str_len+1) > MAX_VALUE_STR_LEN)?MAX_VALUE_STR_LEN:(str_len+1), "%s", begin);
                pValueArr[num] = strtoul(vbuf, NULL, 10);
                ++num;
            }
            begin = end + 1;
            end   = strchr(begin, delim);
        }

        // 最后一个字段值
        str_len = len - (begin - str);
        if (str_len != 0 && num < MAX_ENCODE_VALUE_NUM) {
            // 数值类型
            snprintf(vbuf, ((str_len+1) >= MAX_VALUE_STR_LEN)?MAX_VALUE_STR_LEN:(str_len+1), "%s", begin);
            pValueArr[num] = strtoul(vbuf, NULL, 10);
            ++num;
        }
        incr_len = num * 4;
    }
    else if (_dataType == DT_INT64 || _dataType == DT_UINT64) {
        uint64_t *pValueArr = (uint64_t *)valueArr;
        while(end != NULL) {
            // 非最后一个字段值
            if (unlikely(num == MAX_ENCODE_VALUE_NUM)) {
                TWARN("INDEXLIB: ProfileEncodeFile addNoVarintEncode error, value [%s] num over %u !", str, MAX_ENCODE_VALUE_NUM);
                break;
            }
            str_len = end - begin;
            if (str_len != 0) {
                // 数值类型，进行转换再存储
                snprintf(vbuf, ((str_len+1) > MAX_VALUE_STR_LEN)?MAX_VALUE_STR_LEN:(str_len+1), "%s", begin);
                pValueArr[num] = strtoull(vbuf, NULL, 10);
                ++num;
            }
            begin = end + 1;
            end   = strchr(begin, delim);
        }

        // 最后一个字段值
        str_len = len - (begin - str);
        if (str_len != 0 && num < MAX_ENCODE_VALUE_NUM) {
            // 数值类型
            snprintf(vbuf, ((str_len+1) >= MAX_VALUE_STR_LEN)?MAX_VALUE_STR_LEN:(str_len+1), "%s", begin);
            pValueArr[num] = strtoull(vbuf, NULL, 10);
            ++num;
        }
        incr_len = 8 * num;
    }
    else if (_dataType == DT_KV32) {
        KV32 *pValueArr = (KV32 *)valueArr;
        while(end != NULL) {
            // 非最后一个字段值
            if (unlikely(num == MAX_ENCODE_VALUE_NUM)) {
                TWARN("INDEXLIB: ProfileEncodeFile addNoVarintEncode error, value [%s] num over %u !", str, MAX_ENCODE_VALUE_NUM);
                break;
            }
            str_len = end - begin;
            if (str_len != 0) {
                // 数值类型，进行转换再存储
                snprintf(vbuf, ((str_len+1) > MAX_VALUE_STR_LEN)?MAX_VALUE_STR_LEN:(str_len+1), "%s", begin);
                pValueArr[num] = strtokv32(vbuf);
                ++num;
            }
            begin = end + 1;
            end   = strchr(begin, delim);
        }

        // 最后一个字段值
        str_len = len - (begin - str);
        if (str_len != 0 && num < MAX_ENCODE_VALUE_NUM) {
            // 数值类型
            snprintf(vbuf, ((str_len+1) >= MAX_VALUE_STR_LEN)?MAX_VALUE_STR_LEN:(str_len+1), "%s", begin);
            pValueArr[num] = strtokv32(vbuf);
            ++num;
        }
        incr_len = num * _unitSize;
    }
    else if (_dataType == DT_FLOAT) {
        float *pValueArr = (float *)valueArr;
        while(end != NULL) {
            // 非最后一个字段值
            if (unlikely(num == MAX_ENCODE_VALUE_NUM)) {
                TWARN("INDEXLIB: ProfileEncodeFile addNoVarintEncode error, value [%s] num over %u !", str, MAX_ENCODE_VALUE_NUM);
                break;
            }
            str_len = end - begin;
            if (str_len != 0) {
                // 数值类型，进行转换再存储
                snprintf(vbuf, ((str_len+1) > MAX_VALUE_STR_LEN)?MAX_VALUE_STR_LEN:(str_len+1), "%s", begin);
                pValueArr[num] = strtof(vbuf, NULL);
                ++num;
            }
            begin = end + 1;
            end   = strchr(begin, delim);
        }

        // 最后一个字段值
        str_len = len - (begin - str);
        if (str_len != 0 && num < MAX_ENCODE_VALUE_NUM) {
            // 数值类型
            snprintf(vbuf, ((str_len+1) >= MAX_VALUE_STR_LEN)?MAX_VALUE_STR_LEN:(str_len+1), "%s", begin);
            pValueArr[num] = strtof(vbuf, NULL);
            ++num;
        }
        incr_len = num * _unitSize;
    }
    else if (_dataType == DT_INT16 || _dataType == DT_UINT16) {
        uint16_t *pValueArr = (uint16_t *)valueArr;
        while(end != NULL) {
            // 非最后一个字段值
            if (unlikely(num == MAX_ENCODE_VALUE_NUM)) {
                TWARN("INDEXLIB: ProfileEncodeFile addNoVarintEncode error, value [%s] num over %u !", str, MAX_ENCODE_VALUE_NUM);
                break;
            }
            str_len = end - begin;
            if (str_len != 0) {
                // 数值类型，进行转换再存储
                snprintf(vbuf, ((str_len+1) > MAX_VALUE_STR_LEN)?MAX_VALUE_STR_LEN:(str_len+1), "%s", begin);
                pValueArr[num] = (uint16_t)strtoul(vbuf, NULL, 10);
                ++num;
            }
            begin = end + 1;
            end   = strchr(begin, delim);
        }

        // 最后一个字段值
        str_len = len - (begin - str);
        if (str_len != 0 && num < MAX_ENCODE_VALUE_NUM) {
            // 数值类型
            snprintf(vbuf, ((str_len+1) >= MAX_VALUE_STR_LEN)?MAX_VALUE_STR_LEN:(str_len+1), "%s", begin);
            pValueArr[num] = (uint16_t)strtoul(vbuf, NULL, 10);
            ++num;
        }
        incr_len = 2 * num;
    }
    else if (_dataType == DT_INT8 || _dataType == DT_UINT8) {
        uint8_t *pValueArr = (uint8_t *)valueArr;
        while(end != NULL) {
            // 非最后一个字段值
            if (unlikely(num == MAX_ENCODE_VALUE_NUM)) {
                TWARN("INDEXLIB: ProfileEncodeFile addNoVarintEncode error, value [%s] num over %u !", str, MAX_ENCODE_VALUE_NUM);
                break;
            }
            str_len = end - begin;
            if (str_len != 0) {
                // 数值类型，进行转换再存储
                snprintf(vbuf, ((str_len+1) > MAX_VALUE_STR_LEN)?MAX_VALUE_STR_LEN:(str_len+1), "%s", begin);
                pValueArr[num] = (uint8_t)strtoul(vbuf, NULL, 10);
                ++num;
            }
            begin = end + 1;
            end   = strchr(begin, delim);
        }

        // 最后一个字段值
        str_len = len - (begin - str);
        if (str_len != 0 && num < MAX_ENCODE_VALUE_NUM) {
            // 数值类型
            snprintf(vbuf, ((str_len+1) >= MAX_VALUE_STR_LEN)?MAX_VALUE_STR_LEN:(str_len+1), "%s", begin);
            pValueArr[num] = (uint8_t)strtoul(vbuf, NULL, 10);
            ++num;
        }
        incr_len = num;
    }
    else if (_dataType == DT_DOUBLE) {
        double *pValueArr = (double *)valueArr;
        while(end != NULL) {
            // 非最后一个字段值
            if (unlikely(num == MAX_ENCODE_VALUE_NUM)) {
                TWARN("INDEXLIB: ProfileEncodeFile addNoVarintEncode error, value [%s] num over %u !", str, MAX_ENCODE_VALUE_NUM);
                break;
            }
            str_len = end - begin;
            if (str_len != 0) {
                // 数值类型，进行转换再存储
                snprintf(vbuf, ((str_len+1) > MAX_VALUE_STR_LEN)?MAX_VALUE_STR_LEN:(str_len+1), "%s", begin);
                pValueArr[num] = strtod(vbuf, NULL);
                ++num;
            }
            begin = end + 1;
            end   = strchr(begin, delim);
        }

        // 最后一个字段值
        str_len = len - (begin - str);
        if (str_len != 0 && num < MAX_ENCODE_VALUE_NUM) {
            // 数值类型
            snprintf(vbuf, ((str_len+1) >= MAX_VALUE_STR_LEN)?MAX_VALUE_STR_LEN:(str_len+1), "%s", begin);
            pValueArr[num] = strtod(vbuf, NULL);
            ++num;
        }
        incr_len = num * sizeof(double);
    }
    else if (_dataType == DT_STRING) {
        uint32_t str_size = 0;
        while(end != NULL) {
            // 非最后一个字段值
            if (unlikely(num == MAX_ENCODE_VALUE_NUM)) {
                TWARN("INDEXLIB: ProfileEncodeFile addNoVarintEncode error, value [%s] num over %u !", str, MAX_ENCODE_VALUE_NUM);
                break;
            }
            str_len = end - begin;
            if (str_len != 0) {
                // 数值类型，进行转换再存储
                if (unlikely((str_size + sizeof(str_len) + str_len + 1) > sizeof(valueArr))) {
                    TERR("ProfileEncodeFile parse string type error, buffer not big enough!");
                    break;
                }
                memcpy(valueArr + str_size, &str_len, sizeof(str_len));
                snprintf(valueArr + str_size + sizeof(str_len), (str_len+1), "%s", begin);
                str_size += (sizeof(str_len) + str_len + 1);
                ++num;
            }
            begin = end + 1;
            end   = strchr(begin, delim);
        }

        // 最后一个字段值
        str_len = len - (begin - str);
        if (str_len != 0 && num < MAX_ENCODE_VALUE_NUM && (str_size + sizeof(str_len) + str_len + 1) <= sizeof(valueArr)) {
            // 数值类型
            memcpy(valueArr + str_size, &str_len, sizeof(str_len));
            snprintf(valueArr + str_size + sizeof(str_len), (str_len+1), "%s", begin);
            str_size += (sizeof(str_len) + str_len + 1);
            ++num;
        }
        incr_len = str_size;
    }
    else {
        TERR("ProfileEncodeFile add novarint encode error: unknown type!");
        return false;
    }

    if (num == 0) {
        return false;
    }

    char * pCntAddr = NULL;          //字段值在索引中写入的起始地址
    ProfileMMapFile * pTargetFile = NULL;
    if (!_loadFlag) {
        if (unlikely((_curContentSize + incr_len + sizeof(uint16_t)) > _totalContentSize)) {
            if (_pCntFile->makeNewPage(_totalContentSize / 5) == 0) {
                return false;
            }
            _totalContentSize = _pCntFile->getLength();
        }
        pCntAddr = _pCntFile->safeOffset2Addr(_curContentSize);
    }
    else {
        while (unlikely((_curContentSize + incr_len + sizeof(uint16_t)) > _totalContentSize)) {
            if (!newContentSegmentFile()) {
                return false;
            }
        }
        uint32_t segPos   = _curContentSize >> _cntDegree;
        uint32_t inSegPos = _curContentSize &  _cntMask;
        pCntAddr    = _cntFileAddr[segPos] + inSegPos;
        pTargetFile = getMMapFileBySegNum(segPos);
    }
    *(uint16_t*)pCntAddr = (uint16_t)num;      // 写入字段值个数信息
    memcpy(pCntAddr + sizeof(uint16_t), valueArr, incr_len);  // 写入字段值内容

    // 向哈希表中添加多值字段的读取偏移信息
    uint32_t curOffset = (uint32_t)(_curContentSize >> 1);
    if (_pHashTable != NULL && _pHashTable->addValuePtr(sign, &curOffset) != 0) {
        return false;
    }
    offset = curOffset;

    // 设置编码表元数据
    size_t wlen = resetCurContentSize(sizeof(uint16_t) + incr_len);
    putSyncInfo((char *)pCntAddr, wlen, pTargetFile);
    ++_curEncodeValue;
    return true;
}

/**
 * 解析并添加字段值，进行varint压缩
 * @param  sign      字段值对应的哈希值
 * @param  str       字段值的字符串
 * @param  len       字段值的字符串长度
 * @param  delem     字段值的分隔符
 * @param  offset    字段值的偏移量
 * @return           true, OK; false, ERROR
 */
bool   ProfileEncodeFile::addVarintEncode(int64_t sign, const char* str, uint32_t len, char delim, uint32_t &offset)
{
    uint32_t    num      = 0;             //字段值个数
    const char* begin    = str;           //单个字段值的起始位置
    const char* end      = NULL;          //单个字段值的终止位置
    uint16_t    str_len  = 0;             //单个字段值的字符串长度
    char        vbuf [MAX_VALUE_STR_LEN]; //字段值临时存储buffer
    int         incr_len = 0;             //新添加字段内容占用的空间大小

    char        valueArr[MAX_ENCODE_VALUE_NUM * 8];   //原始值buffer
    uint8_t     buf[MAX_ENCODE_VALUE_NUM * 10];   //编码值buffer

    // 解析字段值原始字符串，将字符串转换为二进制保存在valueArr中
    end = strchr(begin, delim);
    if (_dataType == DT_INT32 || _dataType == DT_UINT32) {
        uint32_t *pValueArr = (uint32_t *)valueArr;
        while(end != NULL) {
            // 非最后一个字段值
            if (unlikely(num == MAX_ENCODE_VALUE_NUM)) {
                TWARN("INDEXLIB: ProfileEncodeFile addNoVarintEncode error, value [%s] num over %u !", str, MAX_ENCODE_VALUE_NUM);
                break;
            }
            str_len = end - begin;
            if (str_len != 0) {
                // 数值类型，进行转换再存储
                snprintf(vbuf, ((str_len+1) > MAX_VALUE_STR_LEN)?MAX_VALUE_STR_LEN:(str_len+1), "%s", begin);
                pValueArr[num] = strtoul(vbuf, NULL, 10);
                ++num;
            }
            begin = end + 1;
            end   = strchr(begin, delim);
        }

        // 最后一个字段值
        str_len = len - (begin - str);
        if (str_len != 0 && num < MAX_ENCODE_VALUE_NUM) {
            // 数值类型
            snprintf(vbuf, ((str_len+1) >= MAX_VALUE_STR_LEN)?MAX_VALUE_STR_LEN:(str_len+1), "%s", begin);
            pValueArr[num] = strtoul(vbuf, NULL, 10);
            ++num;
        }
    }
    else {
        uint64_t *pValueArr = (uint64_t *)valueArr;
        while(end != NULL) {
            // 非最后一个字段值
            if (unlikely(num == MAX_ENCODE_VALUE_NUM)) {
                TWARN("INDEXLIB: ProfileEncodeFile addNoVarintEncode error, value [%s] num over %u !", str, MAX_ENCODE_VALUE_NUM);
                break;
            }
            str_len = end - begin;
            if (str_len != 0) {
                // 数值类型，进行转换再存储
                snprintf(vbuf, ((str_len+1) > MAX_VALUE_STR_LEN)?MAX_VALUE_STR_LEN:(str_len+1), "%s", begin);
                pValueArr[num] = strtoull(vbuf, NULL, 10);
                ++num;
            }
            begin = end + 1;
            end   = strchr(begin, delim);
        }

        // 最后一个字段值
        str_len = len - (begin - str);
        if (str_len != 0 && num < MAX_ENCODE_VALUE_NUM) {
            // 数值类型
            snprintf(vbuf, ((str_len+1) >= MAX_VALUE_STR_LEN)?MAX_VALUE_STR_LEN:(str_len+1), "%s", begin);
            pValueArr[num] = strtoull(vbuf, NULL, 10);
            ++num;
        }
    }

    if (num == 0) {
        return false;
    }

    // 进行varint编码
    if (_dataType == DT_INT32) {
        int32_t *pValueArr = (int32_t *)valueArr;
        incr_len = _pVarintWrapper->encode(pValueArr, num, buf, (MAX_ENCODE_VALUE_NUM * 10));
    }
    else if (_dataType == DT_UINT32) {
        uint32_t *pValueArr = (uint32_t *)valueArr;
        incr_len = _pVarintWrapper->encode(pValueArr, num, buf, (MAX_ENCODE_VALUE_NUM * 10));
    }
    else if (_dataType == DT_INT64) {
        int64_t *pValueArr = (int64_t *)valueArr;
        incr_len = _pVarintWrapper->encode(pValueArr, num, buf, (MAX_ENCODE_VALUE_NUM * 10));
    }
    else if (_dataType == DT_UINT64) {
        uint64_t *pValueArr = (uint64_t *)valueArr;
        incr_len = _pVarintWrapper->encode(pValueArr, num, buf, (MAX_ENCODE_VALUE_NUM * 10));
    }

    if (incr_len < 0) {
        fprintf(stderr, "ProfileEncodeFile addEncode error, varint encode failed!\n");
        return false;
    }

    /************************ 写入到cnt文件中 *********************/
    char            * pCntAddr    = NULL;          //字段值写入索引内存的起始地址
    ProfileMMapFile * pTargetFile = NULL;
    if (!_loadFlag) {
        if (unlikely((_curContentSize + incr_len + sizeof(uint16_t)) > _totalContentSize)) {
            if (_pCntFile->makeNewPage(_totalContentSize / 5) == 0) {
                return false;
            }
            _totalContentSize = _pCntFile->getLength();
        }
        pCntAddr = _pCntFile->safeOffset2Addr(_curContentSize);
    }
    else {
        while (unlikely((_curContentSize + incr_len + sizeof(uint16_t)) > _totalContentSize)) {
            if (!newContentSegmentFile()) {
                return false;
            }
        }
        uint32_t segPos          = _curContentSize >> _cntDegree;
        uint32_t inSegPos        = _curContentSize &  _cntMask;
        pCntAddr    = _cntFileAddr[segPos] + inSegPos;
        pTargetFile = getMMapFileBySegNum(segPos);
    }
    *(uint16_t*)pCntAddr = (uint16_t)num;                          // 写入字段个数
    memcpy(pCntAddr + sizeof(uint16_t), buf, incr_len);  // 写入字段值

    // 将字段值对应的位移偏移信息写入哈希表
    uint32_t curOffset = (uint32_t)(_curContentSize >> 1);
    if (_pHashTable != NULL && _pHashTable->addValuePtr(sign, &curOffset) != 0) {
        return false;
    }
    offset = curOffset;

    // 设置编码表元数据
    size_t wlen = resetCurContentSize(sizeof(uint16_t) + incr_len);
    putSyncInfo((char *)pCntAddr, wlen, pTargetFile);
    ++_curEncodeValue;
    return true;
}

/**
 * 根据总数大小，自动计算适合的分段个数和分段掩码，并计算实际大小差值
 * @param   num       目标计算的大小
 * @param   segNum    分段个数
 * @param   rmask     分段掩码（用于代替取模运算）
 * @param   diff      分段总大小和实际大小的差值
 * @return            分段对应的指数
 */
uint32_t ProfileEncodeFile::getBaseDegree(size_t num, uint32_t &segNum, uint32_t &rmask, size_t &diff)
{
    uint32_t baseNum = (uint32_t)(num / BASE_DEGREE_TIMES);
    uint32_t degree  = BASE_DEGREE_MIN;
    while(pow(2, (degree+1)) <= baseNum) {
        degree++;
    }

    segNum = (uint32_t)ceil(num / pow(2, degree));
    if (segNum > (BASE_DEGREE_TIMES * 1.5f)) {
        degree++;
        segNum = (uint32_t)ceil(num /pow(2, degree));
    }

    if (segNum == 0) {
        segNum = 1;
    }

    rmask = ((uint32_t) 1 << degree) - 1;
    diff = ((size_t) 1 << degree) * segNum - num;
    return degree;
}

/**
 * 添加新的cnt段文件
 */
bool    ProfileEncodeFile::newContentSegmentFile()
{
    if (unlikely(_cntSegNum >= MAX_ENCODE_SEG_NUM)) {
        TERR("ProfileEncodeFile add new cnt segment file failed！Too many seg file [%u]", _cntSegNum);
        return false;
    }

    char szNewCntFile[PATH_MAX];
    snprintf(szNewCntFile, PATH_MAX, "%s/%s%s_%u", _encodePath, _encodeName, ENCODE_CNT_SUFFIX, _cntNewFileNum);
    size_t newCntLen = (size_t)1 << _cntDegree;

    ProfileMMapFile *pNewCntFile = new ProfileMMapFile(szNewCntFile);
    if (unlikely(pNewCntFile == NULL)) {
        TERR("ProfileEncodeFile add new cnt segment file [%s] failed!", szNewCntFile);
        return false;
    }
    if (!pNewCntFile->open(false, true, newCntLen)) {
        TERR("ProfileEncodeFile add new cnt segment file [%s] failed!", szNewCntFile);
        delete pNewCntFile;
        return false;
    }
    _pNewCntFile[_cntNewFileNum++] = pNewCntFile;
    _cntFileAddr[_cntSegNum++]     = pNewCntFile->getBaseAddr();
    _curContentSize                = ((size_t)1 << _cntDegree) * (_cntSegNum - 1);
    _totalContentSize              = ((size_t)1 << _cntDegree) * _cntSegNum;
    return true;
}

/**
 * 添加新的ext段文件
 */
bool    ProfileEncodeFile::newExtendSegmentFile()
{
    if (unlikely(_extSegNum >= MAX_ENCODE_SEG_NUM)) {
        TERR("ProfileEncodeFile add new ext segment file failed！Too many seg file [%u]", _extSegNum);
        return false;
    }

    char szNewExtFile[PATH_MAX];
    snprintf(szNewExtFile, PATH_MAX, "%s/%s%s_%u", _encodePath, _encodeName, ENCODE_EXT_SUFFIX, _extNewFileNum);
    size_t newExtLen = (size_t)1 << _extDegree;

    ProfileMMapFile *pNewExtFile = new ProfileMMapFile(szNewExtFile);
    if (unlikely(pNewExtFile == NULL)) {
        TERR("ProfileEncodeFile add new ext segment file [%s] failed!", szNewExtFile);
        return false;
    }
    if (!pNewExtFile->open(false, true, newExtLen)) {
        TERR("ProfileEncodeFile add new ext segment file [%s] failed!", szNewExtFile);
        delete pNewExtFile;
        return false;
    }
    _pNewStrExtFile[_extNewFileNum++] = pNewExtFile;
    _extFileAddr[_extSegNum++]        = pNewExtFile->getBaseAddr();
    _extContentSize                   = ((size_t)1 << _extDegree) * (_extSegNum - 1);
    _totalExtContentSize              = ((size_t)1 << _extDegree) * _extSegNum;
    return true;
}

/**
 * 根据cnt文件单元长度获取对应的2的幂次
 */
uint32_t ProfileEncodeFile::getUnitSizeDegree(size_t size)
{
    if (size == 1) {
        return 0;
    }

    if (size == 2) {
        return 1;
    }

    if (size == 4) {
        return 2;
    }

    if (size == 8) {
        return 3;
    }

    if (size == 16) {
        return 4;
    }

    return 0;
}

/**
 * 根据编码表元数据，设置编码表的unitSize信息
 */
void  ProfileEncodeFile::setEncodeUnitSize(bool encode)
{
    if (!_single && encode
        && (_dataType == DT_INT32 || _dataType == DT_UINT32
           || _dataType == DT_INT64 || _dataType == DT_UINT64))
    {   //varint压缩存储，则cnt文件单元长度为1byte
        _unitSize       = 1;
        _pVarintWrapper = new ProfileVarintWrapper();
    }
    else {  //无varint压缩过程
        if (_single && (_dataType == DT_STRING)) {
            _unitSize   = 8;                           // 单值的字符串字段
        }
        else {
            _unitSize   = getDataTypeSize(_dataType);  // 存储二进制数据
        }

        if (_pVarintWrapper != NULL) {
            delete _pVarintWrapper;
            _pVarintWrapper = NULL;
        }
    }
    _unitSizeDegree  = getUnitSizeDegree(_unitSize);   // 计算unitSize对应的位移长度
}


/**
 * 加载编码表相关的.desc文件
 * @param  szFileName    desc文件名
 * @return               true,加载成功；false,加载失败
 */
bool  ProfileEncodeFile::loadEncodeDescFile(const char *szFileName)
{
    if (szFileName == NULL) {
        return false;
    }

    FILE *fd = NULL;
    // 读取编码字段描述文件
    fd = fopen(szFileName, "r");
    if (unlikely(fd == NULL)) {
        return false;
    }

    encode_desc_cnt_info   cnt_desc;   // .desc文件持久化数据的结构体1
    encode_desc_ext_info   ext_desc;   // .desc文件持久化数据的结构体2
    encode_desc_meta_info  meta_desc;  // .desc文件持久化数据的结构体3

    size_t rnum = fread(&cnt_desc, sizeof(encode_desc_cnt_info), 1, fd);
    if (unlikely(rnum != 1)) {
        fclose(fd);
        return false;
    }
    rnum = fread(&ext_desc, sizeof(encode_desc_ext_info), 1, fd);
    if (unlikely(rnum != 1)) {
        fclose(fd);
        return false;
    }
    rnum = fread(&meta_desc, sizeof(encode_desc_meta_info), 1, fd);
    if (unlikely(rnum != 1)) {
        fclose(fd);
        return false;
    }
    if (unlikely(meta_desc.type != _dataType)) {
        fclose(fd);
        return false;
    }

    // 设置字段编码描述信息
    _curEncodeValue = cnt_desc.cur_encode_value;
    _curContentSize = cnt_desc.cur_content_size;
    _cntNewFileNum  = cnt_desc.cntNewFileNum;
    _cntSegNum      = cnt_desc.cntSegNum;

    _extContentSize = ext_desc.cur_extend_size;
    _extNewFileNum  = ext_desc.extNewFileNum;
    _extSegNum      = ext_desc.extSegNum;

    _updateOverlap  = meta_desc.update_overlap;
    _dataType       = meta_desc.type;
    _unitSize       = meta_desc.unit_size;
    _unitSizeDegree = getUnitSizeDegree(_unitSize);
    _cntDegree      = meta_desc.cntDegree;
    _cntMask        = meta_desc.cntMask;
    _extDegree      = meta_desc.extDegree;
    _extMask        = meta_desc.extMask;

    if (meta_desc.varintEncode) {
        if (_pVarintWrapper == NULL) {
            _pVarintWrapper = new ProfileVarintWrapper();
        }
    }
    fclose(fd);
    return true;
}

/**
 * 完成编码表的分段加载逻辑(初始加载索引/增量持久化后重新加载等情况)
 * @param  update     是否支持更新
 * @return            true；OK; false；ERROR
 */
bool  ProfileEncodeFile::loadEncodeSegInfo(bool update)
{
    bool     loadSeg   = false;   // 是否需要加载多个段文件(用于加载更新后dump的索引情况)
    size_t   cnt_diff  = 0;       // cnt文件在首次带更新加载情况下需要补足的空间单元大小
    size_t   ext_diff  = 0;       // single string类型的ext文件在首次带更新加载情况下需要补足的空间单元大小

    if (_cntSegNum == 0) {
        // 建库后首次加载索引情况，首先计算内部分段信息
        _cntDegree  = getBaseDegree(_curContentSize, _cntSegNum,  _cntMask,  cnt_diff);
        if (_single && _dataType == DT_STRING) {
            _extDegree = getBaseDegree(_extContentSize, _extSegNum, _extMask, ext_diff);
        }
        loadSeg = false;     //不需要加载分段文件
    }
    else {
        // 索引更新后将段信息dump到索引中
        loadSeg = true;
    }

    // 为初始cnt文件和ext文件扩展空间(保证为段长的整数倍)
    if (update && !loadSeg) {
        // 如果当前空间小于预留，则分配buffer
        if (unlikely(cnt_diff != 0 && (cnt_diff + _curContentSize) > _pCntFile->getLength()
                     && _pCntFile->makeNewPage(cnt_diff) == 0))
        {
            return false;
        }

        if (_single && _dataType == DT_STRING) {
            if (unlikely(ext_diff != 0 && (ext_diff + _extContentSize) > _pSingleStrExtFile->getLength()
                        && _pSingleStrExtFile->makeNewPage(ext_diff) == 0)) {
                return false;
            }
        }
    }
    _totalContentSize = _pCntFile->getLength();
    if (_single && _dataType == DT_STRING) {
        _totalExtContentSize = _pSingleStrExtFile->getLength();
    }

    // 预加载内存映射文件
    _pCntFile->preload();
    if (_single && _dataType == DT_STRING) {
        _pSingleStrExtFile->preload();
    }

    // 按照计算的段长进行地址切分
    for (uint32_t pos = 0; pos < (_cntSegNum - _cntNewFileNum); pos++) {
        _cntFileAddr[pos]  = _pCntFile->offset2Addr(pos * ((size_t)1 << _cntDegree));
    }

    for (uint32_t pos = 0; pos < (_extSegNum - _extNewFileNum); pos++) {
        _extFileAddr[pos]  = _pSingleStrExtFile->offset2Addr(pos * ((size_t)1 << _extDegree));
    }

    if (loadSeg) {
        // 如果存在增量分段文件，加载分段文件
        char segFileName[PATH_MAX];
        ProfileMMapFile *pSegFile = NULL;
        for(uint32_t pos = 0; pos < _cntNewFileNum; pos++) {
            // 依次加载cnt的新增段文件
            snprintf(segFileName, PATH_MAX, "%s/%s%s_%u", _encodePath, _encodeName, ENCODE_CNT_SUFFIX, pos);
            pSegFile  = new ProfileMMapFile(segFileName);
            if (!pSegFile->open(false, true)) {
                TERR("ProfileEncodeFile open %s failed!", segFileName);
                return false;
            }
            pSegFile->preload();
            _cntFileAddr[_cntSegNum - _cntNewFileNum + pos] = pSegFile->getBaseAddr();
            _pNewCntFile[pos] = pSegFile;
            _totalContentSize += pSegFile->getLength();
        }

        for(uint32_t pos = 0; pos < _extNewFileNum; pos++) {
            // 依次加载ext的新增段文件
            snprintf(segFileName, PATH_MAX, "%s/%s%s_%u", _encodePath, _encodeName, ENCODE_EXT_SUFFIX, pos);
            pSegFile  = new ProfileMMapFile(segFileName);
            if (!pSegFile->open(false, true)) {
                TERR("ProfileEncodeFile open %s failed!", segFileName);
                return false;
            }
            pSegFile->preload();
            _extFileAddr[_extSegNum - _extNewFileNum + pos] = pSegFile->getBaseAddr();
            _pNewStrExtFile[pos] = pSegFile;
            _totalExtContentSize += pSegFile->getLength();
        }
    }

    if (!loadSeg) {
        // 重写desc文件
        flushDescFile();
    }
    return true;
}

void ProfileEncodeFile::setSyncManager(ProfileSyncManager * syncMgr)
{
    _syncMgr = syncMgr;
}

/**
 * 添加同步数据到同步管理器中
 */
void ProfileEncodeFile::putSyncInfo(char * addr, uint32_t len, ProfileMMapFile * file)
{
    if (_loadFlag && _syncMgr) {
        _syncMgr->putSyncInfo(addr, len, file);
    }
}

/**
 * 根据逻辑段号获得对应的MMapFile
 */
ProfileMMapFile * ProfileEncodeFile::getMMapFileBySegNum(uint32_t segNum, bool cnt)
{
    if (cnt) {
        if (segNum < (_cntSegNum - _cntNewFileNum)) {
            return _pCntFile;
        }
        else {
            return _pNewCntFile[segNum + _cntNewFileNum - _cntSegNum];
        }
    }
    else {
        if (segNum < (_extSegNum - _extNewFileNum)) {
            return _pSingleStrExtFile;
        }
        else {
            return _pNewStrExtFile[segNum + _extNewFileNum - _extSegNum];
        }
    }
}



/**
 * 将2个编码表合并起来
 *
 * @param    pEncode    待合并的编码表对象
 * @param    signMap    合并后，被合并掉的编码表中的 旧编码值 和 新编码值的对应关系
 *
 * @return   添加成功返回0；否则返回-1。
 */
int ProfileEncodeFile::merge( ProfileEncodeFile * pEncode, hashmap_t signMap  )
{
    if ( NULL      == pEncode )                 return -1;
    if ( NULL      == signMap )                 return -1;
    if ( _dataType != pEncode->getDataType() )  return -1;
    if ( true      == pEncode->isSingle() )     return 0;            // 目前没有实际用处，先不实现
    if ( DT_STRING == _dataType )               return 0;            // 目前没有实际用处，先不实现

    uint32_t           pos        = 0;
    char               valArrNuf[ MAX_ENCODE_VALUE_NUM * 8 ] = {0};
    ProfileHashNode  * pNode      = NULL;                            // 将要被合并的编码表 节点
    ProfileHashNode  * tmpNode    = NULL;

    pNode = pEncode->_pHashTable->firstNode( pos );
    while ( pNode )
    {
        tmpNode = _pHashTable->findNode( pNode->sign );
        if ( NULL != tmpNode )                                       // 查看一下是否在当前encode中已经存在
        {
            hashmap_insert( signMap, (void *) (*(uint32_t *)pNode->payload ),
                                     (void *) (*(uint32_t *)tmpNode->payload) );

            pNode = pEncode->_pHashTable->nextNode( pos );           // 下一个节点
            continue;
        }

        uint32_t   payload   = *(uint32_t *)pNode->payload;          // 假的偏移量，需要运算后得到真正的偏移
        uint32_t   valNum    = 0;                                    // 在旧的encode文件中的值的个数
        uint8_t  * valOffset = 0;                                    // 在旧的encode文件中的真正的地址
        uint32_t   valLen    = 0;                                    // 在旧的encode文件中的值的byte数
        char     * pCntAddr  = NULL;                                 // 需要copy到的当前encode文件目的地址

        valOffset  = (uint8_t *)pEncode->_pCntFile->safeOffset2Addr( payload << 1 );
        valNum     = *((uint16_t *)valOffset);
        valOffset += sizeof( uint16_t );

        // 计算出需要copy的字符串长度
        if ( true == pEncode->isEncoded() )
        {
            switch ( pEncode->getDataType() )                        // Wrapper是个无状态的工具类，都一样
            {
                case DT_INT32:
                    valLen = _pVarintWrapper->decode( valOffset, valNum,
                                   (int32_t *)valArrNuf, MAX_ENCODE_VALUE_NUM );
                    break;
                case DT_UINT32:
                    valLen = _pVarintWrapper->decode( valOffset, valNum,
                                   (uint32_t *)valArrNuf, MAX_ENCODE_VALUE_NUM );
                    break;
                case DT_INT64:
                    valLen = _pVarintWrapper->decode( valOffset, valNum,
                                   (int64_t *)valArrNuf, MAX_ENCODE_VALUE_NUM );
                    break;
                case DT_UINT64:
                    valLen = _pVarintWrapper->decode( valOffset, valNum,
                                   (uint64_t *)valArrNuf, MAX_ENCODE_VALUE_NUM );
                    break;
                default:
                    break;
            }
        }
        else
        {
            valLen = valNum * getUnitSize();
        }

        // 根据需要 看看 是否需要新分配内存
        if (unlikely( (_curContentSize + sizeof(uint16_t) + valLen) >= _totalContentSize ))
        {
            if ( (_totalContentSize / 5) > sizeof(uint16_t) + valLen )
            {
                if ( _pCntFile->makeNewPage(_totalContentSize / 5) == 0 )
                    return -1;
            }
            else
            {
                if ( _pCntFile->makeNewPage( 100 * (sizeof(uint16_t) + valLen) ) == 0 )
                    return -1;
            }

            _totalContentSize = _pCntFile->getLength();
        }

        // 新文件的可写地址
        pCntAddr = _pCntFile->safeOffset2Addr( _curContentSize );

        // 开始真正的 写入 值得个数  和 值得内存
        *(uint16_t *)pCntAddr = (uint16_t)valNum;
        memcpy( pCntAddr + sizeof(uint16_t), valOffset, valLen );     // 从旧文件中copy到新文件中

        // 加入到当前encode文件的hash表中 , 把旧的payload 和 新的payload 存下来
        hashmap_insert( signMap, (void *) payload,
                                 (void *) ((uint32_t)( _curContentSize >> 1 )) );

        payload = (uint32_t)( _curContentSize >> 1 );                // 把实际的偏移量除以2，得到假偏移
        _pHashTable->addValuePtr( pNode->sign, &payload );           // 加到新hash表中去

        // 重置一下下一个可以写入的位置。 一定是2的倍数
        resetCurContentSize( sizeof(uint16_t) + valLen );             // 重置内部的 实际偏移量，得到下一次添加时可用的起始位置
        ++_curEncodeValue;

        pNode = pEncode->_pHashTable->nextNode( pos );               // 旧encode的下一个节点
    }

    return 0;
}



#define KVVAL_TO_STR_BUF( DATA_TYPE, FMT)                                                       \
    addOffset = (sizeof(uint16_t) + valNum * getUnitSize());                     \
    for( uint32_t i = 0; i < valNum; ++i )                                       \
    {                                                                            \
        bufLen += snprintf( valueBuf + bufLen, sizeof(valueBuf) - bufLen,          \
                FMT, ((DATA_TYPE *)valOffset)[i].key, ((DATA_TYPE *)valOffset)[i].value); \
    }



#define VAL_TO_STR_BUF( DATA_TYPE , FMT )                                      \
    addOffset = (sizeof(uint16_t) + valNum * getUnitSize() );                   \
                                                                               \
    for ( uint32_t i = 0; i < valNum; ++i )                                    \
    {                                                                          \
        bufLen += snprintf( valueBuf + bufLen, sizeof(valueBuf) - bufLen,      \
                            FMT, ((DATA_TYPE *) valOffset)[ i ] );             \
    }



#define DECODE_VAL_TO_STR_BUF( DATA_TYPE , FMT )                               \
    if ( _pVarintWrapper != NULL )                                             \
    {                                                                          \
        addOffset  = (sizeof(uint16_t));                                        \
        addOffset += _pVarintWrapper->decode( valOffset, valNum,               \
                            (DATA_TYPE *) valArrBuf, MAX_ENCODE_VALUE_NUM );   \
                                                                               \
        for ( uint32_t i = 0; i < valNum; ++i )                                \
        {                                                                      \
            bufLen += snprintf( valueBuf + bufLen, sizeof(valueBuf) - bufLen,  \
                                FMT, ((DATA_TYPE *) valArrBuf)[ i ] );         \
        }                                                                      \
    }                                                                          \
    else                                                                       \
    {                                                                          \
        addOffset = (sizeof(uint16_t) + valNum * getUnitSize() );               \
                                                                               \
        for ( uint32_t i = 0; i < valNum; ++i )                                \
        {                                                                      \
            bufLen += snprintf( valueBuf + bufLen, sizeof(valueBuf) - bufLen,  \
                                FMT, ((DATA_TYPE *) valOffset)[ i ] );         \
        }                                                                      \
    }                                                                          \
                                                                               \


/**
 * 重建hashtable，
 * 因为updateOverlap==false的字段没有idx文件
 * 需要初始化一个hashtable , 并根据content中的内容填充hashtable
 *
 * @return  成功返回true，失败返回false。
 */
bool ProfileEncodeFile::rebuildHashTable( uint32_t hashsize )
{
    if ( NULL      != _pHashTable )   return true;                   // 已经有了好建个啥
    if ( true      == isSingle() )    return true;                   // 目前没有实际用处，先不实现
    if ( DT_STRING == _dataType )     return true;                   // 目前没有实际用处，先不实现
    if ( DT_FLOAT  == _dataType )     return true;                   // 不能精确还原,目前没有实际用处，先不实现
    if ( DT_DOUBLE == _dataType )     return true;                   // 不能精确还原,目前没有实际用处，先不实现

    _pHashTable = new ProfileHashTable();                            // 创建哈希表
    if ( NULL == _pHashTable ||
            !_pHashTable->initHashTable( hashsize, sizeof(uint32_t)) )
    {
        SAFE_DELETE( _pHashTable );
        return false;
    }

    /** ========= 根据实际的数据 计算签名，取出偏移。 放入hash表中   ============== */
    uint64_t   offset                               = 0;           // 真正偏移量
    char       valueBuf[ MAX_PROFILE_ORGINSTR_LEN ] = {0};         // buffer

    for ( ; offset < _curContentSize; )
    {
        uint32_t   valNum    = 0;                                    // 值的个数
        uint32_t   bufLen    = 0;                                    // buf中被使用掉的字节数
        uint8_t  * valOffset = 0;                                    // 真正的地址
        uint32_t   addOffset = 0;
        char       valArrBuf[ MAX_ENCODE_VALUE_NUM * 8];

        valOffset  = (uint8_t *)_pCntFile->safeOffset2Addr( offset );
        valNum     = *((uint16_t *)valOffset);
        valOffset += sizeof( uint16_t );

        if ( valNum > 0 )
        {
            switch ( _dataType )
            {
                case DT_INT8:
                    VAL_TO_STR_BUF( int8_t, "%d " );
                    break;
                case DT_UINT8:
                    VAL_TO_STR_BUF( uint8_t, "%u " );
                    break;
                case DT_INT16:
                    VAL_TO_STR_BUF( int16_t, "%d " );
                    break;
                case DT_UINT16:
                    VAL_TO_STR_BUF( uint16_t, "%u " );
                    break;
                case DT_KV32:
                    KVVAL_TO_STR_BUF(KV32, "%d:%d " );
                    break;
                case DT_INT32:
                    DECODE_VAL_TO_STR_BUF( int32_t, "%d " );
                    break;
                case DT_UINT32:
                    DECODE_VAL_TO_STR_BUF( uint32_t, "%u " );
                    break;
                case DT_INT64:
                    DECODE_VAL_TO_STR_BUF( int64_t, "%ld " );
                    break;
                case DT_UINT64:
                    DECODE_VAL_TO_STR_BUF( uint64_t, "%lu " );
                    break;
                default:
                    break;
            }

            valueBuf[ bufLen - 1 ]  = '\0';                          // 替换掉最后的空格
            bufLen                 -= 1;
        }
        else
        {
            valueBuf[ 0 ] = '\0';
            bufLen        = 0;
            addOffset     = sizeof(uint16_t);
        }

        uint64_t  sign    = ProfileHashTable::hash( valueBuf, bufLen );
        uint32_t  payload = offset >> 1;                             // 假的偏移信息

        if ( NULL == _pHashTable->findNode( sign ) )                 // 添加到hash表中去
        {
            if ( _pHashTable->addValuePtr( sign, &payload ) != 0)
                return false;
        }
        else
        {
            TERR( "failed! find same node in %s ", _encodeName );
        }

        if ( addOffset % 2 == 0 )                                    // 真偏移一定2的倍数
        {
            offset += addOffset;
        }
        else
        {
            offset += ( addOffset + 1 );
        }
    }

    return true;
}



void ProfileEncodeFile::print_test( )
{
    if ( true      == isSingle() )    return ;                   // 目前没有实际用处，先不实现
    if ( DT_STRING == _dataType )     return ;                   // 目前没有实际用处，先不实现
    if ( DT_FLOAT  == _dataType )     return ;                   // 不能精确还原,目前没有实际用处，先不实现
    if ( DT_DOUBLE == _dataType )     return ;                   // 不能精确还原,目前没有实际用处，先不实现

    /** ========= 根据实际的数据 计算签名，取出偏移。 放入hash表中   ============== */
    uint64_t   offset                               = 0;           // 真正偏移量
    char       valueBuf[ MAX_PROFILE_ORGINSTR_LEN ] = {0};         // buffer

    for ( ; offset < _curContentSize; )
    {
        uint32_t   valNum    = 0;                                    // 值的个数
        uint32_t   bufLen    = 0;                                    // buf中被使用掉的字节数
        uint8_t  * valOffset = 0;                                    // 真正的地址
        uint32_t   addOffset = 0;
        char       valArrBuf[ MAX_ENCODE_VALUE_NUM * 8];

        valOffset  = (uint8_t *)_pCntFile->safeOffset2Addr( offset );
        valNum     = *((uint16_t *)valOffset);
        valOffset += sizeof( uint16_t );

        if ( valNum > 0 )
        {
            switch ( _dataType )
            {
                case DT_INT8:
                    VAL_TO_STR_BUF( int8_t, "%d " );
                    break;
                case DT_UINT8:
                    VAL_TO_STR_BUF( uint8_t, "%u " );
                    break;
                case DT_INT16:
                    VAL_TO_STR_BUF( int16_t, "%d " );
                    break;
                case DT_UINT16:
                    VAL_TO_STR_BUF( uint16_t, "%u " );
                    break;
                case DT_KV32:
                    KVVAL_TO_STR_BUF(KV32, "%d:%d " );
                    break;
                case DT_INT32:
                    DECODE_VAL_TO_STR_BUF( int32_t, "%d " );
                    break;
                case DT_UINT32:
                    DECODE_VAL_TO_STR_BUF( uint32_t, "%u " );
                    break;
                case DT_INT64:
                    DECODE_VAL_TO_STR_BUF( int64_t, "%ld " );
                    break;
                case DT_UINT64:
                    DECODE_VAL_TO_STR_BUF( uint64_t, "%lu " );
                    break;
                default:
                    break;
            }

            valueBuf[ bufLen - 1 ]  = '\0';                          // 替换掉最后的空格
            bufLen                 -= 1;
        }
        else
        {
            valueBuf[ 0 ] = '\0';
            bufLen        = 0;
            addOffset     = sizeof(uint16_t);
        }

        fprintf( stdout, "num:%u  val:%s\n", valNum, valueBuf );

        if ( addOffset % 2 == 0 )                                    // 真偏移一定2的倍数
        {
            offset += addOffset;
        }
        else
        {
            offset += ( addOffset + 1 );
        }
    }

}





#define PARSE_STR_TO_VAL_ARR( DATA_TYPE , CONVERT_FUNC )                                \
                                                                                        \
    DATA_TYPE * pValueArr = (DATA_TYPE * )outBuf;                                       \
                                                                                        \
    do                                                                                  \
    {                                                                                   \
        if ( NULL != end )                                                              \
        {                                                                               \
            itemLen = end - begin;                                                      \
        }                                                                               \
        else                                                                            \
        {                                                                               \
            itemLen = strLen - ( begin - str );                                         \
        }                                                                               \
                                                                                        \
        if ( itemLen != 0 )                                                             \
        {                                                                               \
            if (unlikely( outLen >= outBufLen ))                                        \
            {                                                                           \
                TWARN("error, value [%s] num over %u !", str, outBufLen);               \
                break;                                                                  \
            }                                                                           \
                                                                                        \
            if (unlikely( (itemLen + 1) > MAX_VALUE_STR_LEN ))                          \
            {                                                                           \
                TWARN("error, value len [%s] over %u !", itemLen, MAX_VALUE_STR_LEN );  \
                break;                                                                  \
            }                                                                           \
                                                                                        \
            snprintf( vbuf, (itemLen + 1), "%s", begin );                               \
            pValueArr[ itemNum ] = ( DATA_TYPE ) CONVERT_FUNC;                          \
                                                                                        \
            ++itemNum;                                                                  \
            outLen += sizeof( DATA_TYPE );                                              \
        }                                                                               \
                                                                                        \
        begin  = end + 1;                                                               \
        end    = strchr( begin, delim );                                                \
    } while ( end != NULL );



/**
 * 将输入的字符串，解析成值的数组
 *
 * @return       true:成功; false:失败
 */
/***
bool ProfileEncodeFile::parseStr2Val( const char * str,              // 单个宝贝字段的取值字符串
                                      uint32_t     strLen,           // 取值字符串的长度
                                      char         delim = ' ',      // 取值字符串中字段值的分隔符号(默认空格)
                                      char       * outBuf,           // 用于保存输出的buffer
                                      uint32_t     outBufLen,        // 输入的buffer长度
                                      int        & outLen,           // 输出到buffer中的总共的byte数
                                      uint32_t   & itemNum           // 输出的元素得个数
                                    )
{
    if (unlikely( NULL == str    ))   return false;
    if (unlikely( NULL == outBuf ))   return false;
    if (unlikely( 0    == strLen ))   return false;

    const char * begin                    = str;                     // 单个字段值的起始位置
    const char * end                      = NULL;                    // 单个字段值的终止位置
    uint16_t     itemLen                  = 0;                       // 单个字段值的字符串长度
    char         vbuf [MAX_VALUE_STR_LEN];                           // 单个字段值临时存储buffer
                 outLen                   = 0;
                 itemNum                  = 0;

    end = strchr( begin, delim );                                    // 查找第一个分隔符

    if ( _dataType == DT_INT32 || _dataType == DT_UINT32 )
    {
        PARSE_STR_TO_VAL_ARR( uint32_t , strtoul( vbuf, NULL, 10 ) );
    }
    else if (_dataType == DT_INT64 || _dataType == DT_UINT64)
    {
        PARSE_STR_TO_VAL_ARR( uint64_t , strtoull( vbuf, NULL, 10 ) );
    }
    else if (_dataType == DT_FLOAT)
    {
        PARSE_STR_TO_VAL_ARR( float , strtof( vbuf, NULL ) );
    }
    else if (_dataType == DT_INT16 || _dataType == DT_UINT16)
    {
        PARSE_STR_TO_VAL_ARR( uint16_t , strtoul( vbuf, NULL, 10 ) );
    }
    else if (_dataType == DT_INT8 || _dataType == DT_UINT8)
    {
        PARSE_STR_TO_VAL_ARR( uint8_t , strtoul( vbuf, NULL, 10 ) );
    }
    else if (_dataType == DT_DOUBLE)
    {
        PARSE_STR_TO_VAL_ARR( double , strtod( vbuf, NULL ) );
    }
    else if (_dataType == DT_STRING)
    {
        do
        {
            if ( NULL != end )
            {
                itemLen = end - begin;
            }
            else
            {
                itemLen = strLen - ( begin - str );
            }

            if ( itemLen != 0 )
            {
                if (unlikely( (outLen + sizeof(uint16_t) + itemLen + 1) > outBufLen ) )
                {
                    TERR("parse string type error, buffer not big enough!");
                    break;
                }

                // 输出单个item字符串长度  和 字符串本身
                memcpy( outBuf + outLen, &itemLen, sizeof(uint16_t) );
                snprintf( outBuf + outLen + sizeof(uint16_t), (itemLen + 1), "%s", begin);

                outLen += (sizeof(uint16_t) + itemLen + 1);
                ++itemNum;
            }

            begin = end + 1;
            end   = strchr( begin, delim );
        } while ( end != NULL );
    }
    else
    {
        TERR("add novarint encode error: unknown type!");
        return false;
    }

    return true;
}
*/

}
