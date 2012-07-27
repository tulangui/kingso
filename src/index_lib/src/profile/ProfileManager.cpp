#include <string.h>
#include "index_lib/ProfileManager.h"
#include "index_lib/DocIdManager.h"
#include "index_lib/ProfileSyncManager.h"

namespace index_lib {

ProfileManager* ProfileManager::_managerObj = NULL;

/** 释放ProfileManager类的单一实例对象 */
void ProfileManager::freeInstance()
{
    // test only, output add docs
    //_managerObj->outputIncDocInfo();
    SAFE_DELETE(_managerObj);
}

/** constructor */
ProfileManager::ProfileManager():_docAccessor(&_profileResource)
{
    memset(_profileFieldArr, 0, sizeof(ProfileField *)       * MAX_PROFILE_FIELD_NUM);
    memset(_docHeaderArr,    0, sizeof(const ProfileField *) * MAX_PROFILE_FIELD_NUM);
    _fieldNum                 = 0;
    _headerNum                = 0;
    _profilePath [0]          = '\0';
    _dictPath [0]             = '\0';
    _profileResource.docNum   = 0;
    _profileResource.groupNum = 0;
    _profileResource.syncMgr  = NULL;
    _pDictManager             = NULL;
    _nidPos                   = -1;
    memset(_profileResource.groupArr, 0, sizeof(AttributeGroupResource *) * MAX_PROFILE_FIELD_NUM);
    _buildDocNum              = 0;
    _docCntFd                 = -1;
    _sync                     = false;
}

/** destructor */
ProfileManager::~ProfileManager()
{
    // dump data as needed, can be removed
    //dump();

    // free ProfileField array
    for(uint32_t pos = 0; pos < _fieldNum; ++pos) {
        SAFE_FREE(_profileFieldArr[pos]->bitRecordPtr);
        SAFE_DELETE(_profileFieldArr[pos]->encodeFilePtr);
        SAFE_FREE(_profileFieldArr[pos]);
    }

    // free ProfileResource
    for(uint32_t pos = 0; pos < _profileResource.groupNum; ++pos) {
        for(uint32_t segPos = 0; segPos < _profileResource.groupArr[pos]->segNum; ++segPos) {
            SAFE_DELETE(_profileResource.groupArr[pos]->mmapFileArr[segPos]);
            _profileResource.groupArr[pos]->segAddressArr[segPos] = NULL;
        }
        SAFE_FREE(_profileResource.groupArr[pos]);
    }

    SAFE_FREE(_profileResource.syncMgr);

    if (_docCntFd > 0) {
        close(_docCntFd);
        _docCntFd = -1;
    }
}



/** 为新profile生成dict文件 */
int ProfileManager::regenDocidDict()
{
    _pDictManager = DocIdManager::getInstance();
    _pDictManager->reset();

    const ProfileField * pField = _docAccessor.getProfileField( NID_FIELD_NAME );

    uint32_t oldDocId = INVALID_DOCID;
    uint32_t docId    = 0;

    for ( uint32_t i = 0; i < _profileResource.docNum; ++i )
    {
        int64_t nid = _docAccessor.getInt64( i, pField );

        if (unlikely( docId != _pDictManager->addNid( nid, false ) ))
        {
            oldDocId = _pDictManager->getDocId( nid );              // 容错功能
            if ( INVALID_DOCID != oldDocId )
                _pDictManager->delDocId( oldDocId );                 // 删掉旧的那个

            if ( docId != _pDictManager->addNid( nid, false ) )
            {
                TERR(" add nid [%ld] get docId [%u]!", nid, docId );
                return KS_EFAILED;
            }
        }

        ++docId;
    }

    return KS_SUCCESS;
}







/**
 * 将小profile合并起来
 *
 * @param  pflMgr  要被合并的小段索引
 *
 * @return         成功返回0，否则返回-1。
 */
int ProfileManager::merge( ProfileManager * pflMgr )
{
    if ( NULL == pflMgr )   return -1;

    uint32_t oldDocNum   = getProfileDocNum();                       // 老的doc个数，存下来有用
    uint32_t copyDocNum  = pflMgr->getProfileDocNum();               // 需要被copy的doc个数

    if ( copyDocNum <= 0 )  return 0;                                //

    rebuildHashTable();                                              // 怕没有，确保一下
    pflMgr->rebuildHashTable();                                      // 理论上外部应该会提前建的

    for ( uint32_t i = 0; i < pflMgr->getProfileDocNum(); ++i )
    {
        newDocSegment( false );                                      // 内部会把对应的新块 及变量维护好
    }

    /********************* 循环所有的group, copy 各个内存块   **********************/
    for ( uint8_t i = 0; i < _profileResource.groupNum; ++i )
    {
        uint32_t  targetSegIndex   = 0;                              // 目标segment的索引
        uint64_t  targetDocOffset  = 0;                              // 目标segment的可写偏移
        uint64_t  targetLeftDocNum = 0;                              // 目标segment的剩余可写doc数

        uint32_t  srcSegIndex      = 0;                              // 源segment的索引
        uint64_t  srcDocOffset     = 0;                              // 源segment的可读偏移
        uint64_t  srcLeftDocNum    = 0;                              // 源segment的剩余可读doc数

        targetSegIndex   = oldDocNum >> PROFILE_SEG_SIZE_BIT_NUM;    // 目标 相关值的初始化
        targetDocOffset  = oldDocNum %  DOCNUM_PER_SEGMENT;
        targetLeftDocNum = DOCNUM_PER_SEGMENT - targetDocOffset;

        if ( 0 == copyDocNum % DOCNUM_PER_SEGMENT )                  // 源 相关值的初始化
        {
            srcLeftDocNum = DOCNUM_PER_SEGMENT;
        }
        else
        {
            if ( copyDocNum > DOCNUM_PER_SEGMENT )
            {
                srcLeftDocNum = DOCNUM_PER_SEGMENT;
            }
            else
            {
                srcLeftDocNum = copyDocNum;
            }
        }

        AttributeGroupResource * targetGroup = _profileResource.groupArr[ i ];
        AttributeGroupResource * srcGroup    = pflMgr->_profileResource.groupArr[ i ];
        uint32_t                 unitSpace   = targetGroup->unitSpace;

        while ( 1 )                                                  // 开始copy
        {
            if ( targetLeftDocNum >= srcLeftDocNum )                 // 空间还够, 都copy过去
            {
                memcpy( targetGroup->segAddressArr[ targetSegIndex ] + targetDocOffset * unitSpace,
                        srcGroup->segAddressArr[ srcSegIndex ] + srcDocOffset * unitSpace,
                        srcLeftDocNum * unitSpace );

                if ( srcSegIndex == srcGroup->segNum - 1 ) break;    // 最后一个copy完了

                if ( targetLeftDocNum == srcLeftDocNum )             // 目标：下一个segment
                {
                    targetSegIndex   += 1;                           // 目标： 判断写入segment的 索引
                    targetDocOffset   = 0;                           // 目标： 计算出在这个可写的segment中的可写位置
                    targetLeftDocNum  = DOCNUM_PER_SEGMENT;          // 目标： 计算出可以写入的doc个数
                }
                else
                {
                    targetDocOffset  += srcLeftDocNum;               // 目标： 计算出在这个可写的segment中的可写位置
                    targetLeftDocNum -= srcLeftDocNum;               // 目标： 计算出可以写入的doc个数
                }

                srcSegIndex      += 1;                               // 源: 计算读取的segment的索引
                srcDocOffset      = 0;                               // 源: 计算读取的segment的读取位置

                if ( srcSegIndex < srcGroup->segNum - 1 )            // 还没有到最后一个segment
                {
                    srcLeftDocNum = DOCNUM_PER_SEGMENT;              // 源: 计算读取的doc个数
                }
                else                                                 // 最后一个segment了
                {
                    srcLeftDocNum = copyDocNum % DOCNUM_PER_SEGMENT;
                }
            }
            else                                                     // 空间不够, 先copy一部分
            {
                memcpy( targetGroup->segAddressArr[ targetSegIndex ] + targetDocOffset * unitSpace,
                        srcGroup->segAddressArr[ srcSegIndex ] + srcDocOffset * unitSpace,
                        targetLeftDocNum * unitSpace );

                targetSegIndex   += 1;                               // 目标： 判断写入segment的 索引
                targetDocOffset   = 0;                               // 目标： 计算出在这个可写的segment中的可写位置
                srcDocOffset     += targetLeftDocNum;                // 源: 计算读取的segment的读取位置
                srcLeftDocNum    -= targetLeftDocNum;                // 源: 计算读取的doc个数

                targetLeftDocNum  = DOCNUM_PER_SEGMENT;              // 目标： 计算出可以写入的doc个数
            }

            if ( 0 == srcLeftDocNum ) break;
        }
    }

    /************************ 循环   合并编码表   修改编码值 ***********************/
    for ( uint32_t i = 0; i < _fieldNum; ++i )
    {
              ProfileField * pflField = _profileFieldArr[ i ];
        const ProfileField * tmpField = pflMgr->getProfileFieldByIndex( i );

        if ( (NULL == pflField) || (NULL == tmpField) ) continue;
        if ( false == pflField->isEncoded )             continue;
        if ( false == tmpField->isEncoded )             continue;

        hashmap_t signMap = hashmap_create( 3145739, HM_FLAG_POOL, hashmap_hashInt, hashmap_cmpInt );
        if ( NULL == signMap )
        {
            TERR( "field:%s create hash map failed", pflField->name );
            return -1;
        }

        if ( 0 != pflField->encodeFilePtr->merge( tmpField->encodeFilePtr, signMap ) )
        {
            TERR( "merge field:%s encode file failed", pflField->name );
            return -1;
        }

        // 合并成功，循环新加入的所有的doc， 现在要更新主表中记录的 pos值，
        for ( uint32_t docId = 0; docId < copyDocNum; ++docId )
        {
            // 1. 从旧表中取 编码值, 2, 查hash表， 得到新值    3. 更新到新主表中去
            uint32_t tmpVal = pflMgr->_docAccessor.getFieldEncodeValue( docId, tmpField );
            uint32_t ecdVal = (uint64_t)(hashmap_find( signMap, (void *) tmpVal ));

            _docAccessor.setFieldEncodeValue( docId + oldDocNum , pflField, ecdVal );
        }

        // 释放掉 hash_map
        hashmap_destroy( signMap, NULL, NULL );
        signMap = NULL;
    }

    return 0;
}



/**
 * 对所有因为updateOverlap==false的字段没有idx文件， 重建hashtable，
 *
 * @return  成功返回true，失败返回false。
 */
bool ProfileManager::rebuildHashTable()
{
    if ( _fieldNum        <= 0    )  return true;
    if ( _profileFieldArr == NULL )  return true;

    for ( uint32_t i = 0; i < _fieldNum; ++i )
    {
        ProfileField * pflField = _profileFieldArr[ i ];

        if ( NULL  == pflField )                 continue;
        if ( false == pflField->isEncoded )      continue;
        if ( NULL  == pflField->encodeFilePtr )  continue;
        if ( true  == pflField->encodeFilePtr->isUpdateOverlap() )
            continue;

        if ( false == pflField->encodeFilePtr->rebuildHashTable() )
        {
            TERR( "rebuild field:%s HashTable failed", pflField->name );
            return false;
        }
    }

    return true;
}



/** 设置Profile数据的存取路径 */
int ProfileManager::setProfilePath (const char *path)
{
    if (unlikely(path == NULL)) {
        TERR("INDEXLIB: ProfileManager set profile path error!");
        return KS_EINVAL;
    }

    snprintf(_profilePath, PATH_MAX, "%s", path);
    return 0;
}

/** 设置DocIdDict数据的存取路径 */
int ProfileManager::setDocIdDictPath (const char *path)
{
    if (unlikely(path == NULL)) {
        TERR("INDEXLIB: ProfileManager set docId dict path error!");
        return KS_EINVAL;
    }

    snprintf(_dictPath, PATH_MAX, "%s", path);
    return 0;
}


/** 添加一组BitRecord字段 */
int ProfileManager::addBitRecordField(uint32_t num, char **fieldNameArr, char **fieldAliasArr, uint32_t *lenArr,
                                     PF_DATA_TYPE *dataTypeArr, bool *encodeFlagArr, PF_FIELD_FLAG *fieldFlagArr,
                                     EmptyValue *defaultArr, uint32_t groupPos)
{
    // check args
    if (unlikely(num > MAX_BITFIELD_NUM || (num + _fieldNum) > MAX_PROFILE_FIELD_NUM
                 || num == 0 || fieldNameArr == NULL || fieldAliasArr == NULL || lenArr == NULL
                 || dataTypeArr == NULL || encodeFlagArr == NULL || fieldFlagArr == NULL || defaultArr == NULL))
    {
        TERR("INDEXLIB: ProfileManager add BitRecordField error, check arguments failed!");
        return KS_EINVAL;
    }

    // check field name
    for (uint32_t pos = 0; pos < num; ++pos) {
        if (unlikely(fieldNameArr[pos] == NULL)) {
            TERR("INDEXLIB: ProfileManager addBitRecordField error, invalid field name!");
            return KS_EINVAL;
        }
    }

    uint32_t posArray[MAX_BITFIELD_NUM];    // 排序输出字段下标对应的buffer
    bool ret = false;

    // 对bit字段进行排序，按照字段长度从大到小排序，将字段下标输出到posArray数组
    ret = bitFieldSortByLen(lenArr, num, posArray, MAX_BITFIELD_NUM);
    if (unlikely(!ret)) {
        TERR("INDEXLIB: ProfileManager addBitRecordField error, field length sort failed!");
        return KS_EFAILED;
    }

    // 检查bit字段长度是否超标
    if (unlikely(lenArr[posArray[0]] >= INVALID_BITFIELD_LEN || lenArr[posArray[num - 1]] == 0 )) {
        TERR("INDEXLIB: ProfileManager addBitRecordField error, only support bit field length between 1~31!");
        return KS_EINVAL;
    }

    // 分配bit字段描述和profile字段描述结构体空间
    uint32_t             startPos   = _fieldNum;
    ProfileField        *fieldPtr   = NULL;
    BitRecordDescriptor *bitDescPtr = NULL;
    for (uint32_t pos = 0; pos < num; ++pos) {
        bitDescPtr = (BitRecordDescriptor *)calloc(1, sizeof(BitRecordDescriptor));
        if (unlikely(bitDescPtr == NULL)) {
            TERR("INDEXLIB: ProfileManager addBitRecoreField error, alloc BitRecordDescriptor failed!");
            return KS_ENOMEM;
        }

        fieldPtr  = (ProfileField *)calloc(1, sizeof(ProfileField));
        if (unlikely(fieldPtr == NULL)) {
            TERR("INDEXLIB: ProfileManager addBitRecordField error, alloc ProfileField for bitRecord failed!");
            SAFE_FREE(bitDescPtr);
            return KS_ENOMEM;
        }
        fieldPtr->bitRecordPtr        = bitDescPtr;
        _profileFieldArr[_fieldNum] = fieldPtr;
        ++_fieldNum;
    }

    uint32_t inPayloadPos[MAX_BITFIELD_NUM] = {0};    //存储每个payload已使用空间的buffer
    uint32_t maxPayloadPos = 0;                       //目标字段所属payload空间的位置
    // 为每个bit字段进行bit字段描述信息的计算
    for (uint32_t pos = 0; pos < num; ++pos) {
        uint32_t matchPayloadPos = 0;
        uint32_t fieldPos        = posArray[pos];
        bool     match           = false;

        // 为当前字段寻找匹配的payload位置(每个payload是一个uint32_t大小空间)
        for(uint32_t payloadPos = 0; payloadPos <= maxPayloadPos; ++payloadPos) {
            if ((inPayloadPos[payloadPos] + lenArr[fieldPos]) <= 32) {
                // 在每个payload空间中进行空间计算
                match           = true;
                matchPayloadPos = payloadPos;
                break;
            }
        }

        if (!match) {
            // 如果都不匹配则需要分配新的payload空间
            if ((maxPayloadPos + 1) >= MAX_BITFIELD_NUM) {
                TERR("INDEXLIB: ProfileManager addBitRecordField error, BitRecord cannot support payload over 1024 Bytes!");
                return KS_EINTR;
            }
            matchPayloadPos = ++maxPayloadPos;
        }

        // 设置bit字段具体描述信息
        _profileFieldArr[startPos + fieldPos]->bitRecordPtr->field_len  = lenArr[fieldPos];
        _profileFieldArr[startPos + fieldPos]->bitRecordPtr->u32_offset = matchPayloadPos;
        _profileFieldArr[startPos + fieldPos]->bitRecordPtr->bits_move  = 32 - inPayloadPos[matchPayloadPos] - lenArr[fieldPos];
        _profileFieldArr[startPos + fieldPos]->bitRecordPtr->read_mask  = getReadMask ( 32 - inPayloadPos[matchPayloadPos], lenArr[fieldPos]);
        _profileFieldArr[startPos + fieldPos]->bitRecordPtr->write_mask = getWriteMask( 32 - inPayloadPos[matchPayloadPos], lenArr[fieldPos]);

        // 重设payload对应的使用空间
        inPayloadPos[matchPayloadPos] += lenArr[fieldPos];
    }

    // 检查group结构，不存在则创建
    if (unlikely(groupPos >= MAX_PROFILE_FIELD_NUM)) {
        TERR("INDEXLIB: ProfileManager addBitRecordField error, groupID [%u] over limitation!", groupPos);
        return KS_EINTR;
    }

    if (_profileResource.groupArr[groupPos] == NULL) {
        AttributeGroupResource * pGroupResc = (AttributeGroupResource *)calloc(1, sizeof(AttributeGroupResource));
        if (pGroupResc == NULL) {
            TERR("INDEXLIB: ProfileManager addBitRecordField error, alloc new AttributeGroupResource failed!");
            return KS_ENOMEM;
        }
        _profileResource.groupArr[groupPos] = pGroupResc;
        if ((groupPos + 1) > _profileResource.groupNum) {
            _profileResource.groupNum = groupPos + 1;
        }
    }

    // 构造每个字段对应的ProfileField结构体指针
    for(uint32_t pos = 0; pos < num; ++pos) {
        uint32_t fieldPos = posArray[pos];
        if (dataTypeArr[fieldPos] != DT_UINT8 && dataTypeArr[fieldPos] != DT_UINT16
            && dataTypeArr[fieldPos] != DT_UINT32 && dataTypeArr[fieldPos] != DT_UINT64)
        {
            // 负数、浮点数、字符串转化为编码表
            encodeFlagArr[fieldPos] = true;
        }

        snprintf(_profileFieldArr[startPos + fieldPos]->name, MAX_FIELD_NAME_LEN, "%s", fieldNameArr[fieldPos]);
        if (fieldAliasArr[fieldPos] == NULL) {
            _profileFieldArr[startPos + fieldPos]->alias[0] = '\0';
        }
        else {
            snprintf(_profileFieldArr[startPos + fieldPos]->alias, MAX_FIELD_NAME_LEN, "%s", fieldAliasArr[fieldPos]);
        }
        _profileFieldArr[startPos + fieldPos]->type          = dataTypeArr [fieldPos];
        _profileFieldArr[startPos + fieldPos]->flag          = fieldFlagArr[fieldPos];
        _profileFieldArr[startPos + fieldPos]->multiValNum   = 1;
        _profileFieldArr[startPos + fieldPos]->offset        = _profileResource.groupArr[groupPos]->unitSpace
                        + (_profileFieldArr[startPos + fieldPos]->bitRecordPtr->u32_offset * sizeof(uint32_t));
        _profileFieldArr[startPos + fieldPos]->unitLen       = sizeof(uint32_t) * (maxPayloadPos + 1);
        _profileFieldArr[startPos + fieldPos]->groupID       = groupPos;
        _profileFieldArr[startPos + fieldPos]->isBitRecord   = true;
        _profileFieldArr[startPos + fieldPos]->isEncoded     = encodeFlagArr[fieldPos];
        _profileFieldArr[startPos + fieldPos]->isCompress    = false;
        _profileFieldArr[startPos + fieldPos]->defaultEmpty  = defaultArr[fieldPos];

        // 设置字段存储类型标签
        if (_profileFieldArr[startPos + fieldPos]->isEncoded) {
            if (_profileFieldArr[startPos + fieldPos]->isBitRecord) {
                _profileFieldArr[startPos + fieldPos]->storeType = MT_BIT_ENCODE;
            }
            else {
                _profileFieldArr[startPos + fieldPos]->storeType = MT_ENCODE;
            }
        }
        else {
            if (_profileFieldArr[startPos + fieldPos]->isBitRecord) {
                _profileFieldArr[startPos + fieldPos]->storeType = MT_BIT;
            }
            else {
                _profileFieldArr[startPos + fieldPos]->storeType = MT_ONLY;
            }
        }

        if (!encodeFlagArr[fieldPos]) {
            _profileFieldArr[startPos + fieldPos]->encodeFilePtr = NULL;
        }
        else {
            // 创建并打开编码表文件
            ProfileEncodeFile *pEncodeFile = new ProfileEncodeFile(dataTypeArr[fieldPos], _profilePath, fieldNameArr[fieldPos], defaultArr[fieldPos], true);
            if (unlikely(pEncodeFile == NULL
                         || !pEncodeFile->createEncodeFile(false)))
            {
                TERR("INDEXLIB: ProfileManager addBitRecordField error, create encodefile [%s] failed!", fieldNameArr[fieldPos]);
                return KS_ENOMEM;
            }
            _profileFieldArr[startPos + fieldPos]->encodeFilePtr = pEncodeFile;
        }
    }

    // 将ProfileField插入到trie树中
    for (uint32_t pos = 0; pos < num; ++pos) {
        if(_docAccessor.addProfileField(_profileFieldArr[startPos + pos])!= 0) {
            TERR("INDEXLIB: ProfileManager addBitRecordField error, insert ProfileField [%s] failed!", _profileFieldArr[startPos + pos]->name);
            return KS_EINTR;
        }
    }

    _profileResource.groupArr[groupPos]->unitSpace += (sizeof(uint32_t) * (maxPayloadPos + 1));
    return 0;
}

/** 添加非BitRecord字段 */
int ProfileManager::addField(const char *fieldName, const char * fieldAlias, PF_DATA_TYPE dataType, uint32_t multiValNum, bool encodeFlag, PF_FIELD_FLAG fieldFlag, bool compress, EmptyValue defaultValue, uint32_t groupPos, bool overlap)
{
    if (unlikely(fieldName == NULL || multiValNum > MAX_ENCODE_VALUE_NUM
                || _fieldNum >= MAX_PROFILE_FIELD_NUM))
    {
        TERR("INDEXLIB: ProfileManager addField error, invalid arguments!");
        return KS_EINVAL;
    }

    // get groupID of target field
    if (unlikely(groupPos >= MAX_PROFILE_FIELD_NUM)) {
        TERR("INDEXLIB: ProfileManager addField error, groupID [%u] over limitation!", groupPos);
        return KS_EINTR;
    }
    // if group Resource not exist, alloc new space to create new group
    if (_profileResource.groupArr[groupPos] == NULL) {
        AttributeGroupResource * pGroupResc = (AttributeGroupResource *)calloc(1, sizeof(AttributeGroupResource));
        if (pGroupResc == NULL) {
            TERR("INDEXLIB: ProfileManager addField error, alloc new AttributeGroupResource failed!");
            return KS_ENOMEM;
        }
        _profileResource.groupArr[groupPos] = pGroupResc;
        if ((groupPos + 1) > _profileResource.groupNum) {
            _profileResource.groupNum = groupPos + 1;
        }
    }

    if (compress && multiValNum == 0 &&
        (dataType == DT_INT32 || dataType == DT_UINT32 || dataType == DT_INT64 || dataType == DT_UINT64)) {
        compress = true;
    }
    else {
        compress = false;
    }

    // get unit size in main table
    ProfileEncodeFile *pEncodeFile = NULL;
    uint32_t           unitSize = 0;
    if ( dataType == DT_STRING || multiValNum == 0 || (multiValNum == 1 && encodeFlag) ) {
        unitSize    = sizeof(uint32_t);
        encodeFlag  = true;

        // create encode file
        pEncodeFile = new ProfileEncodeFile(dataType, _profilePath, fieldName, defaultValue, (multiValNum == 1)?true:false);
        if (unlikely(pEncodeFile == NULL)) {
            TERR("INDEXLIB: ProfileManager addField error, alloc ProfileEncodeFile failed!");
            return KS_ENOMEM;
        }

        if(unlikely(!pEncodeFile->createEncodeFile(compress, overlap))) {
            TERR("INDEXLIB: ProfileManager addField error, create ProfileEncodeFile"
                    " [name %s, default size] failed!", fieldName);
            return KS_ENOMEM;
        }
    }
    else {
        unitSize = getDataTypeSize(dataType);
        if (unlikely(unitSize == 0)) {
            TERR("INDEXLIB: ProfileManager addField error, get unit size of [%s] target type failed!", fieldName);
            return KS_EINVAL;
        }
    }

    // create ProfileField structure
    ProfileField *pNewFieldPtr = NULL;
    pNewFieldPtr = (ProfileField *)malloc(sizeof(ProfileField));
    if (unlikely(pNewFieldPtr == NULL)) {
        TERR("INDEXLIB: ProfileManager addField error, alloc ProfileField failed!");
        return KS_ENOMEM;
    }

    // set internal value of ProfileField
    snprintf(pNewFieldPtr->name, MAX_FIELD_NAME_LEN, "%s", fieldName);
    if (fieldAlias == NULL) {
        pNewFieldPtr->alias[0] = '\0';
    }
    else {
        snprintf(pNewFieldPtr->alias, MAX_FIELD_NAME_LEN, "%s", fieldAlias);
    }
    pNewFieldPtr->type          = dataType;
    pNewFieldPtr->flag          = fieldFlag;
    pNewFieldPtr->multiValNum   = multiValNum;
    pNewFieldPtr->offset        = _profileResource.groupArr[groupPos]->unitSpace;
    pNewFieldPtr->unitLen       = unitSize;
    pNewFieldPtr->groupID       = groupPos;
    pNewFieldPtr->isBitRecord   = false;
    pNewFieldPtr->isEncoded     = encodeFlag;
    pNewFieldPtr->isCompress    = compress;
    pNewFieldPtr->bitRecordPtr  = NULL;
    pNewFieldPtr->encodeFilePtr = pEncodeFile;
    pNewFieldPtr->defaultEmpty  = defaultValue;
    if (pNewFieldPtr->isEncoded) {
        if (pNewFieldPtr->isBitRecord) {
            pNewFieldPtr->storeType = MT_BIT_ENCODE;
        }
        else {
            pNewFieldPtr->storeType = MT_ENCODE;
        }
    }
    else {
        if (pNewFieldPtr->isBitRecord) {
            pNewFieldPtr->storeType = MT_BIT;
        }
        else {
            pNewFieldPtr->storeType = MT_ONLY;
        }
    }

    // add to trie tree
    if(_docAccessor.addProfileField(pNewFieldPtr) != 0) {
        TERR("INDEXLIB: ProfileManager addField error, add field [%s] to trietree failed!", pNewFieldPtr->name);
        SAFE_FREE(pNewFieldPtr);
        return KS_EINTR;
    }

    // set new size to target group
    if (multiValNum > 1) {
        _profileResource.groupArr[groupPos]->unitSpace += (unitSize * multiValNum);
    }
    else {
        _profileResource.groupArr[groupPos]->unitSpace += unitSize;
    }

    // add to field array
    _profileFieldArr[_fieldNum] = pNewFieldPtr;
    ++_fieldNum;
    return 0;
}

/** 建库过程设置profile文本索引的doc header */
int ProfileManager::setDocHeader(const char *header)
{
    if (unlikely(header == NULL)) {
        TERR("INDEXLIB: ProfileManager setDocHeader error, invalid header!");
        return KS_EINVAL;
    }

    if (unlikely(strstr(header, PROFILE_HEADER_DOCID_STR) == NULL)) {
        TERR("INDEXLIB: ProfileManager setDocHeader error, %s must be set first!", PROFILE_HEADER_DOCID_STR);
        return KS_EINVAL;
    }

    // reset doc header
    _headerNum = 0;

    char        buf[MAX_FIELD_NAME_LEN];
    const char *begin = header + strlen(PROFILE_HEADER_DOCID_STR);
    const char *end   = NULL;
    int         len   = 0;

    const  ProfileField *fieldPtr = NULL;
    while(*begin == '\0' || (end = strchr(begin, '\01')) != NULL) {
        len = end - begin;
        if (len == 0) {
            TERR("INDEXLIB: ProfileManager setDocHeader error, get empty title name!");
            return KS_EINVAL;
        }

        snprintf(buf, ((len + 1) >= MAX_FIELD_NAME_LEN)?MAX_FIELD_NAME_LEN:(len + 1), "%s", begin);
        fieldPtr = _docAccessor.getProfileField(buf);
        if (unlikely(fieldPtr == NULL)) {
            TERR("INDEXLIB: ProfileManager setDocHeader error, invalid field [%s]!", buf);
            return KS_ENOENT;
        }

        _docHeaderArr[_headerNum] = fieldPtr;
        ++_headerNum;
        if (unlikely(_headerNum > _fieldNum || _headerNum > MAX_PROFILE_FIELD_NUM)) {
            TERR("INDEXLIB: ProfileManager setDocHeader error, invalid header num [%u]!", _headerNum);
            return KS_EINTR;
        }
        begin = end + 1;
    }

    if (*begin != '\0') {
        fieldPtr = _docAccessor.getProfileField(begin);
        if (unlikely(fieldPtr == NULL)) {
            TERR("INDEXLIB: ProfileManager setDocHeader error, invalid field [%s]!", begin);
            return KS_ENOENT;
        }

        _docHeaderArr[_headerNum] = fieldPtr;
        ++_headerNum;
        if (unlikely(_headerNum > _fieldNum || _headerNum > MAX_PROFILE_FIELD_NUM)) {
            TERR("INDEXLIB: ProfileManager setDocHeader error, invalid header num [%u]!", _headerNum);
            return KS_EINTR;
        }
    }

    // 标记nid字段的位置，供DocIdManager建DocIdDict使用
    _pDictManager = DocIdManager::getInstance();
    if (_pDictManager == NULL) {
        TERR("INDEXLIB: ProfileManager get instance of DocIdManager error!");
        return -1;
    }

    for(uint32_t pos = 0; pos < _headerNum; ++pos) {
        if ( strcmp(_docHeaderArr[pos]->name, NID_FIELD_NAME) == 0 ) {
            _nidPos = (int)(pos + 1);
        }
    }
    if (_nidPos == -1) {
        TERR("INDEXLIB: ProfileManager setDocHeader error, not find  <%s> field!", NID_FIELD_NAME);
        return KS_EINTR;
    }

    return 0;
}

/** 添加一条profile doc对应的文本索引数据入索引库 */
int ProfileManager::addDoc(char *line)
{
    int ret = 0;
    if (unlikely(line == NULL)) {
        TERR("INDEXLIB: ProfileManager addDoc error, line is NULL!");
        return KS_EINVAL;
    }

    char       *fieldPos[MAX_PROFILE_FIELD_NUM];
    uint32_t    fieldNum  = 0;
    char       *begin     = line;
    char       *end       = NULL;
    int64_t     nid       = 0;
    uint32_t    docId     = 0;

    fieldPos[fieldNum] = begin;
    ++fieldNum;
    while((end = strchr(begin, '\01')) != NULL) {
        *end   = '\0';
        begin  = end + 1;
        fieldPos[fieldNum] = begin;
        ++fieldNum;
    }

    //read and check doc id
    docId = (uint32_t)strtoul(fieldPos[0], NULL, 10);
    if (unlikely(_profileResource.docNum != docId)) {
        TERR("INDEXLIB: ProfileManager addDoc error, docId [%u] dismatch, should be %u!",
            docId, _profileResource.docNum);
        return KS_EINVAL;
    }

    // create space for new doc
    if (unlikely((ret = newDocSegment(false)) != 0)) {
        TERR("INDEXLIB: ProfileManager addDoc error, addNewDocSegment failed!");
        return ret;
    }

    // check field_number
    if (unlikely(fieldNum != (_headerNum + 1))) {
        TERR("INDEXLIB: ProfileManager addDoc error, field num dismatch [docId:%s]!", fieldPos[0]);
        return KS_EINVAL;
    }

    // set Nid to DocIdManager
    if (unlikely(strlen(fieldPos[_nidPos]) == 0 || ! isdigit(fieldPos[_nidPos][0]))) {
        TERR("INDEXLIB: ProfileManager addDoc error, invalid nid [%s] of docId [%u]!",
                fieldPos[_nidPos], docId);
        return KS_EINVAL;
    }
    nid = (int64_t)strtoll(fieldPos[_nidPos], NULL, 10);
    if (unlikely(docId != _pDictManager->addNid(nid, false))) {
        uint32_t oldDoc = _pDictManager->getDocId(nid);
        if (INVALID_DOCID != oldDoc) {
            _pDictManager->delDocId(oldDoc);
        }

        if (unlikely(docId != _pDictManager->addNid( nid, false))) {
            TERR("INDEXLIB: ProfileManager addDoc error, add nid [%ld] of doc [%u] error!", nid, docId);
            return KS_EINVAL;
        }
        TWARN("INDEXLIB: ProfileManager addDoc error, nid [%ld] duplicate! Old doc [%u] is deleted, using new doc [%u]!", nid, oldDoc, docId);
    }

    // set value for each field
    const ProfileField * pField = NULL;
    for(uint32_t pos = 1; pos < fieldNum; ++pos) {
        pField = _docHeaderArr[pos - 1];
        if (_docAccessor.setFieldValue(docId, pField, fieldPos[pos]) != 0) {
            TERR("INDEXLIB: ProfileManager addDoc error, set docId: [%u] Field [%s] Value [%s] failed!",
                    docId, pField->name, fieldPos[pos]);
        }
    }

    // flush last mmap file
    if (_profileResource.docNum & PROFILE_SEG_SIZE_BIT_MASK == 0) {
        uint32_t segPos = (_profileResource.docNum >> PROFILE_SEG_SIZE_BIT_NUM - 1);
        for (uint32_t groupPos = 0; groupPos < _profileResource.groupNum; ++groupPos) {
            if (_profileResource.groupArr[groupPos]->mmapFileArr[segPos]->flush(true)) {
                TERR("INDEXLIB: ProfileManager addDoc error, dump [groupPos:%u, segPos:%u] mmapfile failed!",
                      groupPos, segPos);
            }
            SAFE_DELETE(_profileResource.groupArr[groupPos]->mmapFileArr[segPos]);
            _profileResource.groupArr[groupPos]->segAddressArr[segPos] = NULL;
        }
    }
    return ret;
}

/** 加载Profile索引数据 */
int ProfileManager::load(bool update)
{
    int ret = 0;
    do {
        if (update && _sync) {
            _profileResource.syncMgr = new ProfileSyncManager();
            if (_profileResource.syncMgr == NULL) {
                TERR("INDEXLIB: ProfileManager load error, create profile sync manager obj failed!");
                break;
            }
        }

        // load profile描述文件
        ret = loadProfileDesc();
        if (ret != 0) {
            TERR("INDEXLIB: ProfileManager load error, load profile description file failed!");
            break;
        }

        // load profile属性分组的主表信息
        ret = loadProfileAttributeGroup( false );
        if (ret != 0) {
            TERR("INDEXLIB: ProfileManager load error, load attribute group failed!");
            break;
        }

        // load profile中的所有编码表数据
        ret = loadProfileEncodeFile(update);
        if (ret != 0) {
            TERR("INDEXLIB: ProfileManager load error, load encode files failed!");
            break;
        }
    }while(0);
    return ret;
}




/** 加载Profile索引数据 , 用于修改 */
int ProfileManager::loadForModify()
{
    int ret = 0;

    ret = loadProfileDesc();                               // load profile描述文件
    if (ret != 0)
    {
        TERR( "load profile description file failed!" );
        return ret;
    }

    ret = loadProfileAttributeGroup( true );               // load profile属性分组的主表信息
    if (ret != 0)
    {
        TERR( "load attribute group failed!" );
        return ret;
    }

    ProfileEncodeFile * pEncode = NULL;
    ProfileField      * pField  = NULL;

    for( uint32_t pos = 0; pos < _fieldNum; ++pos )        // load profile中的所有编码表数据
    {
        pField = _profileFieldArr[ pos ];

        if ( false == pField->isEncoded )  continue;       // 没有有编码表

        if ( 1 == pField->multiValNum )
        {
            pEncode = new ProfileEncodeFile( pField->type, _profilePath, pField->name,
                                             pField->defaultEmpty, true );
        }
        else
        {
            pEncode = new ProfileEncodeFile( pField->type, _profilePath, pField->name,
                                             pField->defaultEmpty, false );
        }

        if ( pEncode == NULL )
        {
            TERR( "allocate EncodeFile failed!" );
            return KS_ENOMEM;
        }

        if ( !pEncode->loadEncodeFileForModify() )
        {
            TERR( "load EncodeFile [%s] failed!", pField->name );
            return KS_EINTR;
        }

        pField->encodeFilePtr = pEncode;
    }

    return 0;
}





/** dump索引数据到硬盘文件 */
int ProfileManager::dump()
{
    int ret = 0;
    do {
        // dump profile描述文件
        ret = dumpProfileDesc(true);
        if (ret != 0) {
            TERR("INDEXLIB: ProfileManager dump error, dump profile description file failed!");
            break;
        }

        // dump profile属性分组的主表信息
        ret = dumpProfileAttributeGroup(true);
        if (ret != 0) {
            TERR("INDEXLIB: ProfileManager dump error, dump attribute group data failed!");
            break;
        }

        // dump profile中的所有编码表数据
        ret = dumpProfileEncodeFile(true);
        if (ret != 0) {
            TERR("INDEXLIB: ProfileManager dump error, dump encode files failed!");
            break;
        }

        // dump DocId dict数据
        ret = dumpDocIdDict();
        if (ret != 0) {
            TERR("INDEXLIB: ProfileManager dump error, dump docId dict files failed!");
            break;
        }
    }while(0);
    return ret;
}

/**
 * flush增量索引数据到硬盘
 * @return     0, OK; 负数，ERROR
 */
int ProfileManager::flush()
{
    if ( !_sync ) { return 0;}

    int ret = 0;
    do {
        // flush profile属性分组的主表信息(将主表的doc数据空间加入到sync mgr中）
        ret = dumpProfileAttributeGroup(false);
        if (ret != 0) {
            TERR("INDEXLIB: ProfileManager flush error, flush attribute group data failed!");
            break;
        }

        // 更新sync Manager队列中的更新数据
        if (_profileResource.syncMgr) {
            _profileResource.syncMgr->syncToDisk();
        }

        // flush profile中的所有编码表数据(将编码表描述信息写入到描述文件中）
        ret = dumpProfileEncodeFile(false);
        if (ret != 0) {
            TERR("INDEXLIB: ProfileManager flush error, flush encode files failed!");
            break;
        }

        // flush profile描述文件
        ret = dumpProfileDesc(false);
        if (ret != 0) {
            TERR("INDEXLIB: ProfileManager flush error, flush profile description file failed!");
            break;
        }
    }while(0);
    return ret;
}


/** 创建Profile主表数据对应的新Doc节点（如果segment文件装不下则创建新的segment) */
int ProfileManager::newDocSegment(bool update)
{
    uint32_t segNum = (uint32_t)ceil((double(_profileResource.docNum + 1))/DOCNUM_PER_SEGMENT);
    if (unlikely(segNum > MAX_SEGMENT_NUM)) {
        TERR("INDEXLIB: ProfileManager newDocSegment error, invalid segment num [segNum %u]", segNum);
        return KS_EINTR;
    }

    char buf[PATH_MAX];
    for(uint32_t gPos = 0; gPos < _profileResource.groupNum; ++gPos) {
        uint32_t groupSegNum = _profileResource.groupArr[gPos]->segNum;
        while (groupSegNum < segNum) {
            snprintf(buf, PATH_MAX, "%s/%s_%d%s_%d", _profilePath, PROFILE_MTFILE_PREFIX,
                     gPos, PROFILE_MTFILE_SUFFIX, groupSegNum);

            ProfileMMapFile *mFile = NULL;
            mFile = new ProfileMMapFile(buf);
            if (unlikely(mFile == NULL
                         || !mFile->open(false, update, _profileResource.groupArr[gPos]->unitSpace * DOCNUM_PER_SEGMENT)
                         || mFile->safeOffset2Addr(0) == NULL))
            {
                SAFE_DELETE(mFile);
                TERR("INDEXLIB: ProfileManager newDocSegment error, create new Segment MMapFile failed!");
                return KS_ENOMEM;
            }

            _profileResource.groupArr[gPos]->mmapFileArr  [groupSegNum] = mFile;
            _profileResource.groupArr[gPos]->segAddressArr[groupSegNum] = mFile->safeOffset2Addr(0);
            groupSegNum = ++(_profileResource.groupArr[gPos]->segNum);
        }
    }
    ++(_profileResource.docNum);
    return 0;
}

uint32_t ProfileManager::getDataTypeSize(PF_DATA_TYPE type)
{
    uint32_t uSize = 0;
    switch(type) {
        case DT_INT8:
        case DT_UINT8:
            uSize = 1;
            break;

        case DT_INT16:
        case DT_UINT16:
            uSize = 2;
            break;

        case DT_INT32:
        case DT_UINT32:
            uSize = 4;
            break;

        case DT_INT64:
        case DT_UINT64:
            uSize = 8;
            break;

        case DT_KV32:
            uSize = sizeof(KV32);
            break;

        case DT_FLOAT:
            uSize = sizeof(float);
            break;

        case DT_DOUBLE:
            uSize = sizeof(double);
            break;

        default:
            uSize = 0;
    }
    return uSize;
}

/** Bit字段排序功能
 * args : pFieldLen, pointer of field length array [input]
 *        FieldNum,  field number                  [input]
 *        pPosArray, pointer of sorted field pos array  [output]
 *        arraySize, size of pos array             [input]
 * ret  : true, OK
 *        false, ERROR
 */
bool ProfileManager::bitFieldSortByLen(uint32_t *pFieldLen, uint32_t FieldNum, uint32_t *pPosArray, uint32_t arraySize)
{
    // args check
    if (unlikely(pFieldLen == NULL || pPosArray == NULL || arraySize < FieldNum)) {
        return false;
    }

    for(uint32_t pos = 0; pos < FieldNum; ++pos) {
        bool     init   = true;   // 标识是否是查找剩余未输出位置的初始状态
        uint32_t maxPos = 0;
        for(uint32_t i = 0; i < FieldNum; ++i) {
            //查找i对应的位置是否已经输出到排序数组中
            bool exist = false;    // 标识i对应位置是否在pPosArray中存在
            for(uint32_t m = 0; m < pos; ++m) {
                if (pPosArray[m] == i) {
                    exist = true;
                    break;
                }
            }

            if(exist) {
                //如果i已经输出过，则选择下一个位置继续判断
                continue;
            }

            if (init) {
                //i是首个未输出的字段位置
                maxPos = i;
                init   = false;
            }
            else if (pFieldLen[i] > pFieldLen[maxPos]) {
                //后续未输出的位置i，且位置i的length值较大
                maxPos = i;
            }
        }

        if (!init) {
            //将剩余位置中较大的位置输出到排序数组
            pPosArray[pos] = maxPos;
        }
    }
    return true;
}


/* calculate read mask value */
uint32_t ProfileManager::getReadMask(uint32_t start, uint32_t len)
{
    if (unlikely(start > 32 || start < len)) {
        return 0xFFFFFFFF;
    }
    uint32_t res = 0x00000000;
    // set certern bits to 1
    for(uint32_t pos = 1; pos <= len; ++pos) {
        res = res | (1 << (start - pos));
    }
    return res;
}


/* calculate write mask value */
uint32_t ProfileManager::getWriteMask(uint32_t start, uint32_t len)
{
    if (unlikely(start > 32 || start < len)) {
        return 0x00000000;
    }
    uint32_t res = getReadMask(start,len);
    // get opposite bits to readMask
    return (~res);
}


/** dump Profile增量过程中的doc count信息 */
int  ProfileManager::dumpProfileDocCount(const char * fileName)
{
    if (fileName == NULL) {
        TWARN("INDEXLIB: ProfileManager dump profile doc count error, fileName is NULL!");
        return KS_EINVAL;
    }
    char buf[20];
    int len = snprintf(buf, 20, "%u", _profileResource.docNum);

    if(unlikely(_docCntFd == -1)) {
        _docCntFd = ::open(fileName, O_RDWR|O_CREAT, 0644);
        if (_docCntFd == -1) {
            TWARN("INDEXLIB: ProfileManager dump profile doc count [%s] error, %s", fileName, strerror(errno));
            return KS_EIO;
        }
    }

    if (lseek(_docCntFd, 0, SEEK_SET) == -1) {
        TWARN("INDEXLIB: ProfileManager dump profile doc count [%s] error, %s", fileName, strerror(errno));
        ::close(_docCntFd);
        return KS_EIO;
    }

    int wnum = ::write(_docCntFd, buf, len);
    if (unlikely(wnum != len)) {
        TERR("INDEXLIB: ProfileManager dump profile descriptor error, write docNum to %s error!", fileName);
        ::close(_docCntFd);
        return KS_EIO;
    }
    return 0;
}

/** dump Profile描述信息 */
int  ProfileManager::dumpProfileDesc(bool sync)
{
    int ret = 0;
    // 写入描述文件
    char descFile[PATH_MAX];
    if (!sync) {
        snprintf(descFile, PATH_MAX, "%s/%s", _profilePath, PROFILE_MTFILE_DOCCOUNT);
        return dumpProfileDocCount(descFile);
    }
    snprintf(descFile, PATH_MAX, "%s/%s", _profilePath, PROFILE_MTFILE_DESC);

    FILE   *fd   = NULL;
    size_t wnum  = 0;
    do {
        // open file
        fd = fopen(descFile, "w");
        if (unlikely(fd == NULL)) {
            TERR("INDEXLIB: ProfileManager dump profile descriptor error, open %s error!", PROFILE_MTFILE_DESC);
            ret = KS_EIO;
            break;
        }

        // write version information
        wnum = fwrite(&MAIN_VERSION, sizeof(uint32_t), 1, fd);
        if (unlikely(wnum != 1)) {
            TERR("INDEXLIB: ProfileManager dump profile descriptor error, write MAIN_VERSION to %s error!", PROFILE_MTFILE_DESC);
            ret = KS_EIO;
            break;
        }

        wnum = fwrite(&SUB_VERSION, sizeof(uint32_t), 1, fd);
        if (unlikely(wnum != 1)) {
            TERR("INDEXLIB: ProfileManager dump profile descriptor error, write SUB_VERSION to %s error!", PROFILE_MTFILE_DESC);
            ret = KS_EIO;
            break;
        }

        wnum = fwrite(&FIX_VERSION, sizeof(uint32_t), 1, fd);
        if (unlikely(wnum != 1)) {
            TERR("INDEXLIB: ProfileManager dump profile descriptor error, write FIX_VERSION to %s error!", PROFILE_MTFILE_DESC);
            ret = KS_EIO;
            break;
        }

        // write doc number
        wnum = fwrite(&_profileResource.docNum, sizeof(uint32_t), 1, fd);
        if (unlikely(wnum != 1)) {
            TERR("INDEXLIB: ProfileManager dump profile descriptor error, write docNum to %s error!", PROFILE_MTFILE_DESC);
            ret = KS_EIO;
            break;
        }

        // write group number
        wnum = fwrite(&_profileResource.groupNum, sizeof(uint32_t), 1, fd);
        if (unlikely(wnum != 1)) {
            TERR("INDEXLIB: ProfileManager dump profile descriptor error, write groupNum to %s error!", PROFILE_MTFILE_DESC);
            ret = KS_EIO;
            break;
        }

        // write each group size
        for(uint32_t pos = 0; pos < _profileResource.groupNum; ++pos) {
            uint32_t groupSize = _profileResource.groupArr[pos]->unitSpace;
            wnum = fwrite(&groupSize, sizeof(uint32_t), 1, fd);
            if (unlikely(wnum != 1)) {
                TERR("INDEXLIB: ProfileManager dump profile descriptor error, write group[%u] unitsize to %s error!", pos, PROFILE_MTFILE_DESC);
                ret = KS_EIO;
                break;
            }
        }
        if (ret != 0) {
            break;
        }

        // write field number
        wnum = fwrite(&_fieldNum, sizeof(uint32_t), 1, fd);
        if (unlikely(wnum != 1)) {
            TERR("INDEXLIB: ProfileManager dump profile descriptor error, write field number to %s error!", PROFILE_MTFILE_DESC);
            ret = KS_EIO;
            break;
        }

        // write each field description information
        for(uint32_t pos = 0; pos < _fieldNum; ++pos) {
            // write ProfileField structure
            wnum = fwrite(_profileFieldArr[pos], sizeof(ProfileField), 1, fd);
            if (unlikely(wnum != 1)) {
                TERR("INDEXLIB: ProfileManager dump profile descriptor error, write profile field [pos:%u] information to %s error!",
                    pos, PROFILE_MTFILE_DESC);
                ret = KS_EIO;
                break;
            }

            // write BitRecordDescriptor structure if needed
            if (_profileFieldArr[pos]->isBitRecord) {
                if (unlikely(_profileFieldArr[pos]->bitRecordPtr == NULL)) {
                    TERR("INDEXLIB: ProfileManager dump profile descriptor error, write field [%s] bitRecord information error!",
                        _profileFieldArr[pos]->name);
                    ret = KS_ENOENT;
                    break;
                }
                wnum = fwrite(_profileFieldArr[pos]->bitRecordPtr, sizeof(BitRecordDescriptor), 1, fd);
                if (unlikely(wnum != 1)) {
                    TERR("INDEXLIB: ProfileManager dump profile descriptor error, write bitRecord [fieldpos:%u] information error!", pos);
                    ret = KS_EIO;
                    break;
                }
            }
        }
        if (ret != 0) {
            break;
        }

    }while(0);

    if (fd != NULL) {
        fclose(fd);
        fd = NULL;
    }
    return ret;
}

/** dump Profile主表信息 */
int  ProfileManager::dumpProfileAttributeGroup(bool sync)
{
    int ret = 0;

    // 写入主表数据
    uint32_t                groupNum      = _profileResource.groupNum;
    AttributeGroupResource *groupResource = NULL;
    char                   *pDocAddr      = NULL;
    ProfileMMapFile        *mFile         = NULL;

    if (!sync && _profileResource.syncMgr) {
        uint32_t docid    = _profileResource.syncMgr->getDocId();
        uint32_t segPos   = docid >> PROFILE_SEG_SIZE_BIT_NUM;
        uint32_t inSegPos = docid &  PROFILE_SEG_SIZE_BIT_MASK;
        for(uint32_t pos = 0; pos < groupNum; ++pos) {
            if ((groupResource = _profileResource.groupArr[pos]) == NULL) {
                continue;
            }
            pDocAddr = groupResource->segAddressArr[segPos] + (groupResource->unitSpace * inSegPos);
            mFile    = groupResource->mmapFileArr  [segPos];
            _profileResource.syncMgr->putSyncInfo(pDocAddr, groupResource->unitSpace, mFile);
        }
    }
    else {
        for(uint32_t pos = 0; pos < groupNum; ++pos) {
            if ((groupResource = _profileResource.groupArr[pos]) == NULL) {
                TERR("INDEXLIB: ProfileManager dump group resource error, get invalid group [groupID:%u]!",pos);
                ret = KS_ENOENT;
                continue;
            }
            for(uint32_t segPos = 0; segPos < groupResource->segNum; ++segPos) {
                if ((mFile = groupResource->mmapFileArr[segPos]) == NULL) {
                    if (segPos == (groupResource->segNum - 1)) {
                        TERR("INDEXLIB: ProfileManager dump group resource error, last mmapfile is NULL!");
                        ret = KS_ENOENT;
                    }
                    continue;
                }
                if (!mFile->flush(sync)) {
                    TERR("INDEXLIB: ProfileManager dump group resource error, mmapfile flush segment [groupPos:%u, segPos:%u] data failed!", pos, segPos);
                    ret = KS_EIO;
                }
            }
        }
    }
    return ret;
}

/** dump 编码表数据      */
int  ProfileManager::dumpProfileEncodeFile(bool sync)
{
    int ret = 0;
    // 写入编码表数据
    ProfileEncodeFile *pEncodeFile = NULL;
    bool               isEncoded   = false;
    for(uint32_t pos = 0; pos < _fieldNum; ++pos) {
        isEncoded = _profileFieldArr[pos]->isEncoded;
        if (isEncoded) {
            pEncodeFile = _profileFieldArr[pos]->encodeFilePtr;
            if (pEncodeFile == NULL) {
                TERR("INDEXLIB: ProfileManager dump error, encodeFilePtr for [%s] not exist!",
                     _profileFieldArr[pos]->name);
                continue;
            }
            if (sync) {
                if (!pEncodeFile->dumpEncodeFile()) {
                    TERR("INDEXLIB: ProfileManager dump error, dump encode file for [%s] failed!", _profileFieldArr[pos]->name);
                    ret = KS_EIO;
                }
            }
            else {
                if (!pEncodeFile->flushDescFile(false)) {
                    TERR("INDEXLIB: ProfileManager flush error, flush encode desc file for [%s] failed!", _profileFieldArr[pos]->name);
                    ret = KS_EIO;
                }
            }
        }
    }
    return ret;
}

/** dump docId dict数据      */
int  ProfileManager::dumpDocIdDict()
{
    int ret = 0;
    if (_pDictManager != NULL && !_pDictManager->dump(_dictPath)) {
        ret = -1;
    }
    return ret;
}

/** load Profile增量过程中的doc count信息 */
int  ProfileManager::loadProfileDocCount(const char * fileName, uint32_t &doc_num)
{
    if (fileName == NULL) {
        return -1;
    }

    char buf[20];
    FILE * fd = NULL;
    fd = fopen(fileName, "r");
    if (unlikely(fd == NULL)) {
        return -1;
    }

    size_t rnum = fread(buf, 1, 20, fd);
    if (rnum == 20) {
        TERR("Profile load DocCount error!");
        fclose(fd);
        return -1;
    }
    buf[rnum] = '\0';
    doc_num = strtoul(buf, NULL, 10);
    fclose(fd);
    return 0;
}

/** load Profile描述信息 */
int  ProfileManager::loadProfileDesc()
{
    int ret = 0;
    // 读入描述文件
    char descFile[PATH_MAX];
    snprintf(descFile, PATH_MAX, "%s/%s", _profilePath, PROFILE_MTFILE_DESC);

    char countFile[PATH_MAX];
    snprintf(countFile, PATH_MAX, "%s/%s", _profilePath, PROFILE_MTFILE_DOCCOUNT);

    FILE      *fd       = NULL;
    size_t    rnum      = 0;
    uint32_t  u32_value = 0;
    uint32_t  versionArr[3];

    do {
        // open file
        fd = fopen(descFile, "r");
        if (unlikely(fd == NULL)) {
            TERR("INDEXLIB: ProfileManager load error, open %s failed!", PROFILE_MTFILE_DESC);
            ret = KS_EIO;
            break;
        }

        // read version information
        rnum = fread(versionArr, sizeof(uint32_t), 3, fd);
        if (unlikely(rnum != 3)) {
            TERR("INDEXLIB: ProfileManager load error, read version information from %s failed!", PROFILE_MTFILE_DESC);
            ret = KS_EIO;
            break;
        }

        // check version information
        if (unlikely(!checkProfileVersion(versionArr[0], versionArr[1], versionArr[2]))) {
            TERR("INDEXLIB: ProfileManager load error, data version[%u.%u.%u] and code version[%u.%u.%u] mismatch!",
                    versionArr[0], versionArr[1], versionArr[2], MAIN_VERSION, SUB_VERSION, FIX_VERSION);
            ret = KS_EINTR;
            break;
        }

        // read doc number
        rnum = fread(&u32_value, sizeof(uint32_t), 1, fd);
        if (unlikely(rnum != 1)) {
            TERR("INDEXLIB: ProfileManager load error, read docNum from %s failed!", PROFILE_MTFILE_DESC);
            ret = KS_EIO;
            break;
        }
        _buildDocNum            = u32_value;
        _profileResource.docNum = u32_value;
        loadProfileDocCount(countFile, _profileResource.docNum);
        uint32_t segNum         = (uint32_t)ceil((double(_profileResource.docNum))/DOCNUM_PER_SEGMENT);

        // read group number
        rnum = fread(&u32_value, sizeof(uint32_t), 1, fd);
        if (unlikely(rnum != 1)) {
            TERR("INDEXLIB: ProfileManager load error, read groupNum from %s failed!", PROFILE_MTFILE_DESC);
            ret = KS_EIO;
            break;
        }
        _profileResource.groupNum = u32_value;
        for (uint32_t gPos = 0; gPos < _profileResource.groupNum; ++gPos) {
            _profileResource.groupArr[gPos] = (AttributeGroupResource *)calloc(1, sizeof(AttributeGroupResource));
            if (unlikely(_profileResource.groupArr[gPos] == NULL)) {
                TERR("INDEXLIB: ProfileManager load error, alloc space for AttributeGroupResource failed!");
                ret = KS_ENOMEM;
                break;
            }
            _profileResource.groupArr[gPos]->segNum = segNum;
        }

        // read each group size
        for(uint32_t pos = 0; pos < _profileResource.groupNum; ++pos) {
            rnum = fread(&u32_value, sizeof(uint32_t), 1, fd);
            if (unlikely(rnum != 1)) {
                TERR("INDEXLIB: ProfileManager load error, read group[%u] unitsize from %s failed!", pos, PROFILE_MTFILE_DESC);
                ret = KS_EIO;
                break;
            }
            _profileResource.groupArr[pos]->unitSpace = u32_value;
        }
        if (ret != 0) {
            break;
        }

        // read field number
        rnum = fread(&u32_value, sizeof(uint32_t), 1, fd);
        if (unlikely(rnum != 1)) {
            TERR("INDEXLIB: ProfileManager load error, read field number from %s failed!", PROFILE_MTFILE_DESC);
            ret = KS_EIO;
            break;
        }
        _fieldNum = u32_value;

        ProfileField *fieldPtr = NULL;
        // read each field description information
        for(uint32_t pos = 0; pos < _fieldNum; ++pos) {
            // read ProfileField structure
            fieldPtr = (ProfileField *)malloc(sizeof(ProfileField));
            if (unlikely(fieldPtr == NULL)) {
                SAFE_FREE(fieldPtr);
                TERR("INDEXLIB: ProfileManager load error, alloc ProfileField failed!");
                ret = KS_ENOMEM;
                break;
            }
            rnum = fread(fieldPtr, sizeof(ProfileField), 1, fd);
            if (unlikely(rnum != 1)) {
                SAFE_FREE(fieldPtr);
                TERR("INDEXLIB: ProfileManager load error, read profile field [pos: %u] information from  %s failed!",
                    pos, PROFILE_MTFILE_DESC);
                ret = KS_EIO;
                break;
            }
            fieldPtr->bitRecordPtr  = NULL;
            fieldPtr->encodeFilePtr = NULL;
            // set field to array
            _profileFieldArr[pos] = fieldPtr;

            // read BitRecordDescriptor structure if needed
            if (fieldPtr->isBitRecord) {
                BitRecordDescriptor *bitRecordPtr = (BitRecordDescriptor *)malloc(sizeof(BitRecordDescriptor));
                if (unlikely(bitRecordPtr == NULL)) {
                    SAFE_FREE(bitRecordPtr);
                    TERR("INDEXLIB: ProfileManager load error, alloc BitRecordDescriptor failed!");
                    ret = KS_ENOMEM;
                    break;
                }

                rnum = fread(bitRecordPtr, sizeof(BitRecordDescriptor), 1, fd);
                if (unlikely(rnum != 1)) {
                    SAFE_FREE(bitRecordPtr);
                    TERR("INDEXLIB: ProfileManager load error, read bitRecord [fieldpos:%u] information failed!", pos);
                    ret = KS_EIO;
                    break;
                }
                fieldPtr->bitRecordPtr = bitRecordPtr;
            }

            // add field to trie tree
            if(unlikely(_docAccessor.addProfileField(fieldPtr) != 0)) {
                TERR("INDEXLIB: ProfileManager load error, add field [%s] to trie tree failed!", fieldPtr->name);
                ret = KS_EINTR;
                break;
            }
        }
        if (ret != 0) {
            break;
        }
    }while(0);

    if (fd != NULL) {
        fclose(fd);
    }
    return ret;
}

/** load Profile主表信息 */
int  ProfileManager::loadProfileAttributeGroup( bool canModify )
{
    int      ret    = 0;
    char     buf[PATH_MAX];

    AttributeGroupResource *pGroupResc = NULL;
    // load each attribute group
    for(uint32_t pos = 0; pos < _profileResource.groupNum; ++pos) {
        pGroupResc = _profileResource.groupArr[pos];
        if (unlikely(pGroupResc == NULL)) {
            TERR("INDEXLIB: ProfileManager load error, target group resource [pos:%u] not exist!", pos);
            ret = KS_ENOENT;
            break;
        }

        ProfileMMapFile *mFile = NULL;
        // load each segment file
        for(uint32_t segPos = 0; segPos < pGroupResc->segNum; ++segPos) {
            snprintf(buf, PATH_MAX, "%s/%s_%d%s_%d", _profilePath, PROFILE_MTFILE_PREFIX,
                     pos, PROFILE_MTFILE_SUFFIX, segPos);

            mFile = new ProfileMMapFile(buf);
            if (unlikely(mFile == NULL || !mFile->open(false, !canModify )
                         || mFile->safeOffset2Addr(0) == NULL))
            {
                SAFE_DELETE(mFile);
                TERR("INDEXLIB: ProfileManager load error, load segment file:%s failed!", buf);
                ret =  KS_ENOMEM;
                break;
            }
            mFile->preload();
            pGroupResc->mmapFileArr  [segPos] = mFile;
            pGroupResc->segAddressArr[segPos] = mFile->safeOffset2Addr(0);
        }

        if (ret != 0) {
            break;
        }
    }
    return ret;
}

/** load 编码表数据      */
int  ProfileManager::loadProfileEncodeFile(bool update)
{
    ProfileEncodeFile *pEncodeFile = NULL;
    // load encode file for each field
    for(uint32_t pos = 0; pos < _fieldNum; ++pos) {
        if(_profileFieldArr[pos]->isEncoded) {
            pEncodeFile = new ProfileEncodeFile(_profileFieldArr[pos]->type, _profilePath, _profileFieldArr[pos]->name,
                            _profileFieldArr[pos]->defaultEmpty, (_profileFieldArr[pos]->multiValNum == 1)?true:false);
            if (unlikely(pEncodeFile == NULL)) {
                TERR("INDEXLIB: ProfileManager load EncodeFile error, allocate EncodeFile failed!");
                return KS_ENOMEM;
            }

            if (unlikely(!pEncodeFile->loadEncodeFile(update))) {
                TERR("INDEXLIB: ProfileManager load EncodeFile error, load EncodeFile [%s] failed!", _profileFieldArr[pos]->name);
                return KS_EINTR;
            }
            _profileFieldArr[pos]->encodeFilePtr = pEncodeFile;
            if (update) {
                pEncodeFile->setSyncManager(_profileResource.syncMgr);
            }
        }
    }
    return 0;
}

/**
 * 检查profile加载数据和加载代码是否一致
 * @param   mVersion   主版本号
 * @param   sVersion   次版本号
 * @param   fVersion   bugfix版本号
 * @return             true,版本一致; false,版本不一致
 */
bool  ProfileManager::checkProfileVersion(uint32_t mVersion, uint32_t sVersion, uint32_t fVersion)
{
    if (mVersion != MAIN_VERSION) {
        return false;
    }
    if (sVersion != SUB_VERSION) {
        return false;
    }
    if (fVersion != FIX_VERSION) {
        return false;
    }
    return true;
}

/**
 * 打印版本信息
 */
void  ProfileManager::printProfileVersion()
{
    printf("version: %u.%u.%u\n", MAIN_VERSION, SUB_VERSION, FIX_VERSION);
}

/**
 *  供QA测试使用，上线前需要注销该函数的调用
 *  输出新增doc到外部文件profile_inc.txt
 */
void  ProfileManager::outputIncDocInfo()
{
    if (_buildDocNum == 0) {
        return;
    }

    FILE *fd = NULL;
    int8_t   value_int8 = 0;
    int16_t  value_int16 = 0;
    int32_t  value_int32 = 0;
    int64_t  value_int64 = 0;
    uint8_t  value_uint8 = 0;
    uint16_t value_uint16 = 0;
    uint32_t value_uint32 = 0;
    uint64_t value_uint64 = 0;
    float    value_float = 0;
    double   value_double = 0;
    const char* value_str = NULL;

    fd = fopen("profile_inc.txt", "w");
    if (fd == NULL) {
        printf("open output file error!\n");
        return;
    }

    ProfileManager *pManager = ProfileManager::getInstance();
    ProfileDocAccessor *pDocAccessor = ProfileManager::getDocAccessor();

    uint32_t field_num = 0;
    uint32_t doc_num  = 0;
    uint32_t valueNum = 0;
    const ProfileField * pFieldArr[256] = {0};
    ProfileMultiValueIterator iterator;

    field_num = pManager->getProfileFieldNum();
    fprintf(fd, "vdocid");
    for(uint32_t pos = 0; pos < field_num; pos++) {
        pFieldArr[pos] = pManager->getProfileFieldByIndex(pos);
        fprintf(fd, "%c%s", '\01', pFieldArr[pos]->name);
    }
    fprintf(fd, "\n");

    doc_num = pManager->getProfileDocNum();
    for(uint32_t docNum = _buildDocNum; docNum < doc_num; docNum++) {
        fprintf(fd, "%u", docNum);
        for (uint32_t fnum = 0; fnum < field_num; fnum++) {
            if (pFieldArr[fnum]->multiValNum == 1) {
                switch(pFieldArr[fnum]->type) {
                    case DT_INT8:{
                                     value_int8 = pDocAccessor->getInt8(docNum, pFieldArr[fnum]);
                                     fprintf(fd, "%c%d", '\01', value_int8);
                                 }
                                 break;


                    case DT_INT16:{
                                      value_int16 = pDocAccessor->getInt16(docNum, pFieldArr[fnum]);
                                      fprintf(fd, "%c%d", '\01', value_int16);
                                  }
                                  break;


                    case DT_INT32:{
                                      value_int32 = pDocAccessor->getInt32(docNum, pFieldArr[fnum]);
                                      fprintf(fd, "%c%d", '\01', value_int32);
                                  }
                                  break;


                    case DT_INT64: {
                                       value_int64 = pDocAccessor->getInt64(docNum, pFieldArr[fnum]);
                                       fprintf(fd, "%c%ld", '\01', value_int64);
                                   }
                                   break;

                    case DT_UINT8:{
                                      value_uint8 = pDocAccessor->getUInt8(docNum, pFieldArr[fnum]);
                                      fprintf(fd, "%c%u", '\01', value_uint8);
                                  }
                                  break;

                    case DT_UINT16:{
                                       value_uint16 = pDocAccessor->getUInt16(docNum, pFieldArr[fnum]);
                                       fprintf(fd, "%c%u", '\01', value_uint16);
                                   }
                                   break;


                    case DT_UINT32:{
                                       value_uint32 = pDocAccessor->getUInt32(docNum, pFieldArr[fnum]);
                                       fprintf(fd, "%c%u", '\01', value_uint32);
                                   }
                                   break;

                    case DT_UINT64:{
                                       value_uint64 = pDocAccessor->getUInt64(docNum, pFieldArr[fnum]);
                                       fprintf(fd, "%c%lu", '\01', value_uint64);
                                   }
                                   break;

                    case DT_FLOAT: {
                                       value_float = pDocAccessor->getFloat(docNum, pFieldArr[fnum]);
                                       fprintf(fd, "%c%f", '\01', value_float);
                                   }
                                   break;

                    case DT_DOUBLE: {
                                        value_double = pDocAccessor->getDouble(docNum, pFieldArr[fnum]);
                                        fprintf(fd, "%c%lf", '\01', value_double);
                                    }
                                    break;

                    case DT_STRING: {
                                        value_str = pDocAccessor->getString(docNum, pFieldArr[fnum]);
                                        fprintf(fd, "%c%s", '\01', value_str);
                                    }
                                    break;

                    default:
                                    printf("invalid type\n");
                }
            }
            else {
                fprintf(fd, "%c", '\01');
                pDocAccessor->getRepeatedValue(docNum, pFieldArr[fnum], iterator);
                valueNum = iterator.getValueNum();
                for (uint32_t valuePos = 0; valuePos < valueNum; valuePos++) {
                    switch(pFieldArr[fnum]->type) {
                        case DT_INT8:
                            value_int8 = iterator.nextInt8();
                            fprintf(fd, "%d ", value_int8);
                            break;

                        case DT_INT16:
                            value_int16 = iterator.nextInt16();
                            fprintf(fd, "%d ", value_int16);
                            break;

                        case DT_INT32:
                            value_int32 = iterator.nextInt32();
                            fprintf(fd, "%d ", value_int32);
                            break;

                        case DT_INT64:
                            value_int64 = iterator.nextInt64();
                            fprintf(fd, "%ld ", value_int64);
                            break;

                        case DT_UINT8:
                            value_uint8 = iterator.nextUInt8();
                            fprintf(fd, "%u ", value_uint8);
                            break;

                        case DT_UINT16:
                            value_uint16 = iterator.nextUInt16();
                            fprintf(fd, "%u ", value_uint16);
                            break;

                        case DT_UINT32:
                            value_uint32 = iterator.nextUInt32();
                            fprintf(fd, "%u ", value_uint32);
                            break;

                        case DT_UINT64:
                            value_uint64 = iterator.nextUInt64();
                            fprintf(fd, "%lu ", value_uint64);
                            break;

                        case DT_FLOAT:
                            value_float = iterator.nextFloat();
                            fprintf(fd, "%f ", value_float);
                            break;

                        case DT_DOUBLE:
                            value_double = iterator.nextDouble();
                            fprintf(fd, "%lf ", value_double);
                            break;

                        case DT_STRING:
                            value_str = iterator.nextString();
                            fprintf(fd, "%s ", value_str);
                            break;

                        default:
                            printf("invalid type\n");
                    } // switch
                } // for
            } // else
        } //for
        fprintf(fd, "\n");
    } // for
    fclose(fd);
}

}
