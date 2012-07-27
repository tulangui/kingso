#include "index_lib/ProfileDocAccessor.h"

// setInt8/16/32/64共用的设置逻辑
#define SET_INT_LOGIC(FIELD_TYPE, DATATYPE, OUT_FORMAT) \
            uint32_t                segPos   = doc_id >> PROFILE_SEG_SIZE_BIT_NUM; \
            uint32_t                inSegPos = doc_id &  PROFILE_SEG_SIZE_BIT_MASK; \
            AttributeGroupResource *pGroup   = _profileResc->groupArr[fieldPtr->groupID]; \
            if (!fieldPtr->isBitRecord) { \
                if (!fieldPtr->isEncoded) { \
                    DATATYPE *pValue = (DATATYPE*)(pGroup->segAddressArr[segPos] \
                                             + (pGroup->unitSpace * inSegPos) + fieldPtr->offset); \
                    *pValue = value; \
                } \
                else { \
                    uint32_t *pEncodeValue = (uint32_t*)(pGroup->segAddressArr[segPos] \
                                                         + (pGroup->unitSpace * inSegPos) + fieldPtr->offset); \
                    char buf[1024]; \
                    int len = snprintf(buf, 1024, OUT_FORMAT, value); \
                    if (unlikely(len >= 1024)) { \
                        len = 1023; \
                    } \
                    uint32_t encodeValue = fieldPtr->encodeFilePtr->getEncodeIdx(buf, len); \
                    if (encodeValue == INVALID_NODE_POS) { \
                        if (unlikely(!fieldPtr->encodeFilePtr->addEncode(buf, len, encodeValue))) { \
                            TERR("INDEXLIB: ProfileDocAccessor set int error, add new encode value error!"); \
                            return KS_EINTR; \
                        } \
                    } \
                    *pEncodeValue = encodeValue; \
                } \
            } \
            else { \
                uint32_t *pBitRecord = (uint32_t*)(pGroup->segAddressArr[segPos] + (pGroup->unitSpace * inSegPos) + fieldPtr->offset); \
                if (!fieldPtr->isEncoded) { \
                    TERR("INDEXLIB: ProfileDocAccessor set int error, bit field of int type must be encodefile field!"); \
                    return KS_EINTR; \
                } \
                else { \
                    char buf[1024]; \
                    int len = snprintf(buf, 1024, OUT_FORMAT, value); \
                    if (unlikely(len >= 1024)) { \
                        len = 1023; \
                    } \
                    uint32_t encodeValue = fieldPtr->encodeFilePtr->getEncodeIdx(buf, len); \
                    if (encodeValue == INVALID_NODE_POS) { \
                        if (unlikely(!fieldPtr->encodeFilePtr->addEncode(buf, len, encodeValue))) { \
                            TERR("INDEXLIB: ProfileDocAccessor set int error, add new encode value error!"); \
                            return KS_EINTR; \
                        } \
                    } \
                    if (unlikely(encodeValue > MAX_BITFIELD_VALUE(fieldPtr->bitRecordPtr->field_len))) { \
                        TERR("INDEXLIB: ProfileDocAccessor set int error, add invalid bit value(value:%u, len:%u)!", \
                                encodeValue, fieldPtr->bitRecordPtr->field_len); \
                        return KS_EFAILED; \
                    } \
                    (*pBitRecord) = ((*pBitRecord)&fieldPtr->bitRecordPtr->write_mask) \
                                    | (encodeValue << fieldPtr->bitRecordPtr->bits_move); \
                } \
            } \
            setSyncDocId(doc_id); \
            return 0; \

// setUInt8/16/32/64共用的设置逻辑
#define SET_UINT_LOGIC(FIELD_TYPE, DATATYPE, OUT_FORMAT) \
            uint32_t                segPos   = doc_id >> PROFILE_SEG_SIZE_BIT_NUM; \
            uint32_t                inSegPos = doc_id &  PROFILE_SEG_SIZE_BIT_MASK; \
            AttributeGroupResource *pGroup   = _profileResc->groupArr[fieldPtr->groupID]; \
            if (!fieldPtr->isBitRecord) { \
                if (!fieldPtr->isEncoded) { \
                    DATATYPE *pValue = (DATATYPE*)(pGroup->segAddressArr[segPos] \
                                             + (pGroup->unitSpace * inSegPos) + fieldPtr->offset); \
                    *pValue = value; \
                } \
                else { \
                    uint32_t *pEncodeValue = (uint32_t*)(pGroup->segAddressArr[segPos] \
                                                         + (pGroup->unitSpace * inSegPos) + fieldPtr->offset); \
                    char buf[1024]; \
                    int len = snprintf(buf, 1024, OUT_FORMAT, value); \
                    if (unlikely(len >= 1024)) { \
                        len = 1023; \
                    } \
                    uint32_t encodeValue = fieldPtr->encodeFilePtr->getEncodeIdx(buf, len); \
                    if (encodeValue == INVALID_NODE_POS) { \
                        if (unlikely(!fieldPtr->encodeFilePtr->addEncode(buf, len, encodeValue))) { \
                            TERR("INDEXLIB: ProfileDocAccessor set uint error, add new encode value error!"); \
                            return KS_EINTR; \
                        } \
                    } \
                    *pEncodeValue = encodeValue; \
                } \
            } \
            else { \
                uint32_t *pBitRecord = (uint32_t*)(pGroup->segAddressArr[segPos] + (pGroup->unitSpace * inSegPos) + fieldPtr->offset); \
                if (!fieldPtr->isEncoded) { \
                    uint32_t bitValue = (uint32_t)value; \
                    if (unlikely(bitValue > MAX_BITFIELD_VALUE(fieldPtr->bitRecordPtr->field_len))) { \
                        TERR("INDEXLIB: ProfileDocAccessor set uint error, add invalid bit value(value:%u, len:%u)!", \
                                bitValue, fieldPtr->bitRecordPtr->field_len); \
                        return KS_EFAILED; \
                    } \
                    (*pBitRecord) = ((*pBitRecord)&fieldPtr->bitRecordPtr->write_mask) \
                                    | (bitValue << fieldPtr->bitRecordPtr->bits_move); \
                } \
                else { \
                    char buf[1024]; \
                    int len = snprintf(buf, 1024, OUT_FORMAT, value); \
                    if (unlikely(len >= 1024)) { \
                        len = 1023; \
                    } \
                    uint32_t encodeValue = fieldPtr->encodeFilePtr->getEncodeIdx(buf, len); \
                    if (encodeValue == INVALID_NODE_POS) { \
                        if (unlikely(!fieldPtr->encodeFilePtr->addEncode(buf, len, encodeValue))) { \
                            TERR("INDEXLIB: ProfileDocAccessor set uint error, add new encode value error!"); \
                            return KS_EINTR; \
                        } \
                    } \
                    if (unlikely(encodeValue > MAX_BITFIELD_VALUE(fieldPtr->bitRecordPtr->field_len))) { \
                        TERR("INDEXLIB: ProfileDocAccessor set uint error, add invalid bit value(value:%u, len:%u)!", \
                                encodeValue, fieldPtr->bitRecordPtr->field_len); \
                        return KS_EFAILED; \
                    } \
                    (*pBitRecord) = ((*pBitRecord)&fieldPtr->bitRecordPtr->write_mask) \
                                    | (encodeValue << fieldPtr->bitRecordPtr->bits_move); \
                } \
            } \
            setSyncDocId(doc_id); \
            return 0; \


// 设置变长多值字段(Int8/16/32/64,UInt8/16/32/64,Float,Double,String)的共用设置逻辑
#define SET_REPEATED_VALUE_LOGIC(FIELD_TYPE) \
            if (unlikely(!fieldPtr->isEncoded || fieldPtr->encodeFilePtr == NULL)) { \
                TERR("INDEXLIB: ProfileDocAccessor setRepeatedValue error, field should be valid encodefile field!"); \
                return KS_EINVAL; \
            } \
            uint32_t                segPos   = doc_id >> PROFILE_SEG_SIZE_BIT_NUM; \
            uint32_t                inSegPos = doc_id &  PROFILE_SEG_SIZE_BIT_MASK; \
            AttributeGroupResource *pGroup   = _profileResc->groupArr[fieldPtr->groupID]; \
            int                     len      = strlen(value); \
            uint32_t encodeValue = fieldPtr->encodeFilePtr->getEncodeIdx(value, len); \
            if (encodeValue == INVALID_NODE_POS) { \
                if (unlikely(!fieldPtr->encodeFilePtr->addEncode(value, len, encodeValue, delim))) { \
                    TERR("INDEXLIB: ProfileDocAccessor setRepeatedValue error, add new encode value error!"); \
                    return KS_EINTR; \
                } \
            } \
            if (fieldPtr->isBitRecord) { \
                if (unlikely(encodeValue > MAX_BITFIELD_VALUE(fieldPtr->bitRecordPtr->field_len))) { \
                    TERR("INDEXLIB: ProfileDocAccessor setRepeatedValue error, add invalid bit value(value:%u, len:%u)!", \
                            encodeValue, fieldPtr->bitRecordPtr->field_len); \
                    return KS_EFAILED; \
                } \
                uint32_t *pBitRecord = (uint32_t*)(pGroup->segAddressArr[segPos] + (pGroup->unitSpace * inSegPos) + fieldPtr->offset); \
                (*pBitRecord) = ((*pBitRecord)&fieldPtr->bitRecordPtr->write_mask) \
                                | (encodeValue << fieldPtr->bitRecordPtr->bits_move); \
            } \
            else { \
                uint32_t *pEncodeValue = (uint32_t*)(pGroup->segAddressArr[segPos] \
                                                     + (pGroup->unitSpace * inSegPos) + fieldPtr->offset); \
                *pEncodeValue = encodeValue; \
            } \
            setSyncDocId(doc_id); \
            return 0; \

// 设置定长多值字段(INT8/16/32/64,UINT8/16/32/64)的共用逻辑
#define SET_CERTAIN_MULTI_INT_LOGIC(EMPTYVALUE, DATATYPE, TRANS_FUNC) \
            uint32_t                segPos   = doc_id >> PROFILE_SEG_SIZE_BIT_NUM; \
            uint32_t                inSegPos = doc_id &  PROFILE_SEG_SIZE_BIT_MASK; \
            AttributeGroupResource *pGroup   = _profileResc->groupArr[fieldPtr->groupID]; \
            DATATYPE *pValue = (DATATYPE*)(pGroup->segAddressArr[segPos] \
                                             + (pGroup->unitSpace * inSegPos) + fieldPtr->offset); \
            const char* begin = value; \
            for (uint32_t pos = 0; pos < fieldPtr->multiValNum; ++pos) { \
                if (unlikely( begin == NULL || !isdigit(*begin) )) { \
                    pValue[pos] = EMPTYVALUE; \
                    continue; \
                } \
                pValue[pos] = TRANS_FUNC(begin, NULL, 10); \
                begin = index(begin, delim); \
                if (begin != NULL) { \
                    begin = begin + 1; \
                } \
            } \
            setSyncDocId(doc_id); \
            return 0; \

// 设置定长多值字段(float/double)的共用逻辑
#define SET_CERTAIN_MULTI_FLOAT_LOGIC(EMPTYVALUE, DATATYPE, TRANS_FUNC) \
            uint32_t                segPos   = doc_id >> PROFILE_SEG_SIZE_BIT_NUM; \
            uint32_t                inSegPos = doc_id &  PROFILE_SEG_SIZE_BIT_MASK; \
            AttributeGroupResource *pGroup   = _profileResc->groupArr[fieldPtr->groupID]; \
            DATATYPE *pValue = (DATATYPE*)(pGroup->segAddressArr[segPos] \
                                             + (pGroup->unitSpace * inSegPos) + fieldPtr->offset); \
            const char* begin = value; \
            for (uint32_t pos = 0; pos < fieldPtr->multiValNum; ++pos) { \
                if (unlikely( begin == NULL || !isdigit(*begin) )) { \
                    pValue[pos] = EMPTYVALUE; \
                    continue; \
                } \
                pValue[pos] = TRANS_FUNC(begin, NULL); \
                begin = index(begin, delim); \
                if (begin != NULL) { \
                    begin = begin + 1; \
                } \
            } \
            setSyncDocId(doc_id); \
            return 0; \


namespace index_lib {


/**
 * 类的构造函数
 *
 * @param  pResc   ProfileResource结构体指针，内部描述了Profile的属性分组资源情况
 */
ProfileDocAccessor::ProfileDocAccessor(ProfileResource *pResc)
{
    _profileResc = pResc;
}


/**
 * 类的析构函数
 */
ProfileDocAccessor::~ProfileDocAccessor()
{
    _fieldMap.clear();
}


/**
 * 向DocAccessor添加可以通过内部trie树查询的ProfileField
 *
 * @param  fieldPtr    ProfileField对象指针
 *
 * @return  0,OK;other,ERROR
 */
int ProfileDocAccessor::addProfileField(const ProfileField * fieldPtr)
{
    if (!_fieldMap.insert(fieldPtr->name, fieldPtr)) {
        TERR("IndexLib: Profile add field name [%s] to hashmap failed!", fieldPtr->name);
        return KS_EFAILED;
    }

    if ( fieldPtr->alias[0] != '\0' && !_fieldMap.insert(fieldPtr->alias, fieldPtr)) {
        TERR("IndexLib: Profile add field alias [%s] to hashmap failed!", fieldPtr->alias);
        return KS_EFAILED;
    }
    return 0;
}

/**
 * 设置目标doc的目标单值字段的字段值
 *
 * @param  doc_id      目标doc的docID
 * @param  fieldPtr    目标字段的ProfileField指针
 * @param  value       目标字段的字段值
 *
 * @return    0, OK; other,ERROR
 */
int ProfileDocAccessor::setFloat (uint32_t doc_id, const ProfileField *fieldPtr, float      value)
{
    uint32_t                segPos   = doc_id >> PROFILE_SEG_SIZE_BIT_NUM;
    uint32_t                inSegPos = doc_id &  PROFILE_SEG_SIZE_BIT_MASK;
    AttributeGroupResource *pGroup   = _profileResc->groupArr[fieldPtr->groupID];

    if (!fieldPtr->isBitRecord) {
        // 字段不是bit字段
        if (!fieldPtr->isEncoded) {
            // 字段不是bit字段，且不是编码表字段
            float *pValue = (float*)(pGroup->segAddressArr[segPos]
                    + (pGroup->unitSpace * inSegPos) + fieldPtr->offset);
            *pValue = value;
        }
        else {
            // 字段不是bit字段，是编码表字段
            uint32_t *pEncodeValue = (uint32_t*)(pGroup->segAddressArr[segPos]
                    + (pGroup->unitSpace * inSegPos) + fieldPtr->offset);
            char buf[1024];
            int len = snprintf(buf, 1024, "%f", value);
            if (unlikely(len >= 1024)) {
                len = 1023;
            }

            uint32_t encodeValue = fieldPtr->encodeFilePtr->getEncodeIdx(buf, len);
            if (encodeValue == INVALID_NODE_POS) {
                if (unlikely(!fieldPtr->encodeFilePtr->addEncode(buf, len, encodeValue))) {
                    TERR("INDEXLIB: ProfileDocAccessor setFloat error, add new encode value error!");
                    return KS_EINTR;
                }
            }

            *pEncodeValue = encodeValue;
        }
    }
    else {
        // 字段是bit字段
        uint32_t *pBitRecord = (uint32_t*)(pGroup->segAddressArr[segPos] + (pGroup->unitSpace * inSegPos) + fieldPtr->offset);
        if (!fieldPtr->isEncoded) {
            // 字段是bit字段，不是编码字段
            TERR("INDEXLIB: ProfileDocAccessor setFloat error, bit field of int type must be encodefile field!");
            return KS_EINTR;
        }
        else {
            char buf[1024];
            int len = snprintf(buf, 1024, "%f", value);
            if (unlikely(len >= 1024)) {
                len = 1023;
            }

            uint32_t encodeValue = fieldPtr->encodeFilePtr->getEncodeIdx(buf, len);
            if (encodeValue == INVALID_NODE_POS) {
                if (unlikely(!fieldPtr->encodeFilePtr->addEncode(buf, len, encodeValue))) {
                    TERR("INDEXLIB: ProfileDocAccessor setFloat error, add new encode value error!");
                    return KS_EINTR;
                }
            }

            if (unlikely(encodeValue > MAX_BITFIELD_VALUE(fieldPtr->bitRecordPtr->field_len))) {
                TERR("INDEXLIB: ProfileDocAccessor setFloat error, add invalid bit value(value:%u, len:%u)!",
                        encodeValue, fieldPtr->bitRecordPtr->field_len);
                return KS_EFAILED;
            }
            (*pBitRecord) = ((*pBitRecord)&fieldPtr->bitRecordPtr->write_mask)
                | (encodeValue << fieldPtr->bitRecordPtr->bits_move);
        }
    }
    setSyncDocId(doc_id);
    return 0;
}

int ProfileDocAccessor::setDouble(uint32_t doc_id, const ProfileField *fieldPtr, double     value)
{
    uint32_t                segPos   = doc_id >> PROFILE_SEG_SIZE_BIT_NUM;
    uint32_t                inSegPos = doc_id &  PROFILE_SEG_SIZE_BIT_MASK;
    AttributeGroupResource *pGroup   = _profileResc->groupArr[fieldPtr->groupID];

    if (!fieldPtr->isBitRecord) {
        // 字段不是bit字段
        if (!fieldPtr->isEncoded) {
            // 字段不是bit字段，且不是编码表字段
            double *pValue = (double*)(pGroup->segAddressArr[segPos]
                    + (pGroup->unitSpace * inSegPos) + fieldPtr->offset);
            *pValue = value;
        }
        else {
            // 字段不是bit字段，是编码表字段
            uint32_t *pEncodeValue = (uint32_t*)(pGroup->segAddressArr[segPos]
                    + (pGroup->unitSpace * inSegPos) + fieldPtr->offset);
            char buf[1024];
            int len = snprintf(buf, 1024, "%lf", value);
            if (unlikely(len >= 1024)) {
                len = 1023;
            }

            uint32_t encodeValue = fieldPtr->encodeFilePtr->getEncodeIdx(buf, len);
            if (encodeValue == INVALID_NODE_POS) {
                if (unlikely(!fieldPtr->encodeFilePtr->addEncode(buf, len, encodeValue))) {
                    TERR("INDEXLIB: ProfileDocAccessor setDouble error, add new encode value error!");
                    return KS_EINTR;
                }
            }

            *pEncodeValue = encodeValue;
        }
    }
    else {
        // 字段是bit字段
        uint32_t *pBitRecord = (uint32_t*)(pGroup->segAddressArr[segPos] + (pGroup->unitSpace * inSegPos) + fieldPtr->offset);
        if (!fieldPtr->isEncoded) {
            // 字段是bit字段，不是编码字段
            TERR("INDEXLIB: ProfileDocAccessor setDouble error, bit field of int type must be encodefile field!");
            return KS_EINTR;
        }
        else {
            char buf[1024];
            int len = snprintf(buf, 1024, "%lf", value);
            if (unlikely(len >= 1024)) {
                len = 1023;
            }

            uint32_t encodeValue = fieldPtr->encodeFilePtr->getEncodeIdx(buf, len);
            if (encodeValue == INVALID_NODE_POS) {
                if (unlikely(!fieldPtr->encodeFilePtr->addEncode(buf, len, encodeValue))) {
                    TERR("INDEXLIB: ProfileDocAccessor setDouble error, add new encode value error!");
                    return KS_EINTR;
                }
            }

            if (unlikely(encodeValue > MAX_BITFIELD_VALUE(fieldPtr->bitRecordPtr->field_len))) {
                TERR("INDEXLIB: ProfileDocAccessor setDouble error, add invalid bit value(value:%u, len:%u)!",
                        encodeValue, fieldPtr->bitRecordPtr->field_len);
                return KS_EFAILED;
            }
            (*pBitRecord) = ((*pBitRecord)&fieldPtr->bitRecordPtr->write_mask)
                | (encodeValue << fieldPtr->bitRecordPtr->bits_move);
        }
    }
    setSyncDocId(doc_id);
    return 0;
}

int ProfileDocAccessor::setString(uint32_t doc_id, const ProfileField *fieldPtr, const char *value)
{
    if (unlikely(!fieldPtr->isEncoded || fieldPtr->encodeFilePtr == NULL)) {
        TERR("INDEXLIB: ProfileDocAccessor setString error, field should be valid encode file field!");
        return KS_EINVAL;
    }

    uint32_t                segPos   = doc_id >> PROFILE_SEG_SIZE_BIT_NUM;
    uint32_t                inSegPos = doc_id &  PROFILE_SEG_SIZE_BIT_MASK;
    AttributeGroupResource *pGroup   = _profileResc->groupArr[fieldPtr->groupID];
    int                     len      = strlen(value);

    uint32_t encodeValue = fieldPtr->encodeFilePtr->getEncodeIdx(value, len);
    if (encodeValue == INVALID_NODE_POS) {
        if (unlikely(!fieldPtr->encodeFilePtr->addEncode(value, len, encodeValue))) {
            TERR("INDEXLIB: ProfileDocAccessor setString error, add new encode value error!");
            return KS_EINTR;
        }
    }

    if (fieldPtr->isBitRecord) {
        // 是bit字段
        if (unlikely(encodeValue > MAX_BITFIELD_VALUE(fieldPtr->bitRecordPtr->field_len))) {
            TERR("INDEXLIB: ProfileDocAccessor setString error, add invalid bit value(value:%u, len:%u)!",
                    encodeValue, fieldPtr->bitRecordPtr->field_len);
            return KS_EFAILED;
        }

        uint32_t *pBitRecord = (uint32_t*)(pGroup->segAddressArr[segPos] + (pGroup->unitSpace * inSegPos) + fieldPtr->offset);

        (*pBitRecord) = ((*pBitRecord)&fieldPtr->bitRecordPtr->write_mask)
            | (encodeValue << fieldPtr->bitRecordPtr->bits_move);
    }
    else {
        // 不是bit字段
        uint32_t *pEncodeValue = (uint32_t*)(pGroup->segAddressArr[segPos]
                + (pGroup->unitSpace * inSegPos) + fieldPtr->offset);
        *pEncodeValue = encodeValue;
    }
    setSyncDocId(doc_id);
    return 0;
}

/**
 * 设置目标doc的对应字段值
 * @param  doc_id      目标doc的docID
 * @param  fieldPtr    目标字段的ProfileField指针
 * @param  value       目标字段的字符串形式字段值
 * @param  delim       多值字段的字段值分隔符
 * @return             0, OK; other,ERROR
 */
int ProfileDocAccessor::setFieldValue(uint32_t doc_id, const ProfileField *fieldPtr, const char* valueStr, char delim)
{
    if (unlikely(doc_id >= _profileResc->docNum)) {
        TERR("INDEXLIB: ProfileDocAccessor set field value error, invalid doc id[%u]", doc_id);
            return KS_EINVAL;
    }
    if (unlikely(fieldPtr == NULL || valueStr == NULL)) { 
        TERR("INDEXLIB: ProfileDocAccessor set field value error, invalid argument!");
            return KS_EINVAL;
    }

    int ret = 0;
    switch(getFieldDataType(fieldPtr))
    {
        case DT_UINT32:
            {
                if(getFieldMultiValNum(fieldPtr) == 1) {
                    uint32_t value = (uint32_t)strtoul(valueStr, NULL, 10);
                    if (*valueStr == '\0') {
                        value = fieldPtr->defaultEmpty.EV_UINT32;
                    }
                    ret = setUInt32(doc_id, fieldPtr, value);
                }
                else {
                    // add repeated
                    ret = setRepeatedUInt32(doc_id, fieldPtr, valueStr, delim);
                }
            }
            break;

        case DT_INT32:
            {
                if(getFieldMultiValNum(fieldPtr) == 1) {
                    int32_t value = (int32_t)strtol(valueStr, NULL, 10);
                    if (*valueStr == '\0') {
                        value = fieldPtr->defaultEmpty.EV_INT32;
                    }
                    ret = setInt32(doc_id, fieldPtr, value);
                }
                else {
                    // add repeated
                    ret = setRepeatedInt32(doc_id, fieldPtr, valueStr, delim);
                }
            }
            break;

        case DT_UINT64:
            {
                if(getFieldMultiValNum(fieldPtr) == 1) {
                    uint64_t value = (uint64_t)strtoull(valueStr, NULL, 10);
                    if (*valueStr == '\0') {
                        value = fieldPtr->defaultEmpty.EV_UINT64;
                    }
                    ret = setUInt64(doc_id, fieldPtr, value);
                }
                else {
                    // add repeated
                    ret = setRepeatedUInt64(doc_id, fieldPtr, valueStr, delim);
                }
            }
            break;

        case DT_INT64:
            {
                if(getFieldMultiValNum(fieldPtr) == 1) {
                    int64_t value = (int64_t)strtoll(valueStr, NULL, 10);
                    if (*valueStr == '\0') {
                        value = fieldPtr->defaultEmpty.EV_INT64;
                    }
                    ret = setInt64(doc_id, fieldPtr, value);
                }
                else {
                    // add repeated
                    ret = setRepeatedInt64(doc_id, fieldPtr, valueStr, delim);
                }
            }
            break;

        case DT_FLOAT:
            {
                if(getFieldMultiValNum(fieldPtr) == 1) {
                    float value = (float)strtof(valueStr, NULL);
                    if (*valueStr == '\0') {
                        value = fieldPtr->defaultEmpty.EV_FLOAT;
                    }
                    ret = setFloat(doc_id, fieldPtr, value);
                }
                else {
                    // add repeated
                    ret = setRepeatedFloat(doc_id, fieldPtr, valueStr, delim);
                }
            }
            break;

        case DT_STRING:
            {
                if(getFieldMultiValNum(fieldPtr) == 1) {
                    ret = setString(doc_id, fieldPtr, valueStr);
                }
                else {
                    // add repeated
                    ret = setRepeatedString(doc_id, fieldPtr, valueStr, delim);
                }
            }
            break;

        case DT_DOUBLE:
            {
                if(getFieldMultiValNum(fieldPtr) == 1) {
                    double value = (double)strtod(valueStr, NULL);
                    if (*valueStr == '\0') {
                        value = fieldPtr->defaultEmpty.EV_DOUBLE;
                    }
                    ret = setDouble(doc_id, fieldPtr, value);
                }
                else {
                    // add repeated
                    ret = setRepeatedDouble(doc_id, fieldPtr, valueStr, delim);
                }
            }
            break;

        case DT_INT16:
            {
                if(getFieldMultiValNum(fieldPtr) == 1) {
                    int16_t value = (int16_t)strtol(valueStr, NULL, 10);
                    if (*valueStr == '\0') {
                        value = fieldPtr->defaultEmpty.EV_INT16;
                    }
                    ret = setInt16(doc_id, fieldPtr, value);
                }
                else {
                    // add repeated
                    ret = setRepeatedInt16(doc_id, fieldPtr, valueStr, delim);
                }
            }
            break;

        case DT_UINT16:
            {
                if(getFieldMultiValNum(fieldPtr) == 1) {
                    uint16_t value = (uint16_t)strtoul(valueStr, NULL, 10);
                    if (*valueStr == '\0') {
                        value = fieldPtr->defaultEmpty.EV_UINT16;
                    }
                    ret = setUInt16(doc_id, fieldPtr, value);
                }
                else {
                    // add repeated
                    ret = setRepeatedUInt16(doc_id, fieldPtr, valueStr, delim);
                }
            }
            break;

        case DT_INT8:
            {
                if(getFieldMultiValNum(fieldPtr) == 1) {
                    int8_t value = (int8_t)strtol(valueStr, NULL, 10);
                    if (*valueStr == '\0') {
                        value = fieldPtr->defaultEmpty.EV_INT8;
                    }
                    ret = setInt8(doc_id, fieldPtr, value);
                }
                else {
                    // add repeated
                    ret = setRepeatedInt8(doc_id, fieldPtr, valueStr, delim);
                }
            }
            break;

        case DT_UINT8:
            {
                if(getFieldMultiValNum(fieldPtr) == 1) {
                    uint8_t value = (uint8_t)strtoul(valueStr, NULL, 10);
                    if (*valueStr == '\0') {
                        value = fieldPtr->defaultEmpty.EV_UINT8;
                    }
                    ret = setUInt8(doc_id, fieldPtr, value);
                }
                else {
                    // add repeated
                    ret = setRepeatedUInt8(doc_id, fieldPtr, valueStr, delim);
                }
            }
            break;

        case DT_KV32:
            {
                if(getFieldMultiValNum(fieldPtr) == 1) {
                    KV32 kv;
                    char * delim = strchr(valueStr, PROFILE_KV_DELIM);
                    if (delim == NULL) {
                        kv.key = INVALID_INT32;
                        kv.value = INVALID_INT32;
                    }
                    else {
                        kv.key = strtol(valueStr, NULL, 10);
                        kv.value = strtol(delim + 1, NULL, 10);
                    }
                    ret = setKV32(doc_id, fieldPtr, kv);
                }
                else {
                    ret = setRepeatedKV32(doc_id, fieldPtr, valueStr, delim);
                }
            }
            break;

        default:
            ret = KS_EFAILED;
    }
    return ret;
}


int ProfileDocAccessor::setInt8  (uint32_t doc_id, const ProfileField *fieldPtr, int8_t   value)
{
    SET_INT_LOGIC(DT_INT8, int8_t, "%d");
}

int ProfileDocAccessor::setUInt8 (uint32_t doc_id, const ProfileField *fieldPtr, uint8_t  value)
{
    SET_UINT_LOGIC(DT_UINT8, uint8_t, "%u");
}

int ProfileDocAccessor::setInt16 (uint32_t doc_id, const ProfileField *fieldPtr, int16_t  value)
{
    SET_INT_LOGIC(DT_INT16, int16_t, "%d");
}

int ProfileDocAccessor::setUInt16(uint32_t doc_id, const ProfileField *fieldPtr, uint16_t value)
{
    SET_UINT_LOGIC(DT_UINT16, uint16_t, "%u");
}

int ProfileDocAccessor::setInt32 (uint32_t doc_id, const ProfileField *fieldPtr, int32_t  value)
{
    SET_INT_LOGIC(DT_INT32, int32_t, "%d");
}

int ProfileDocAccessor::setKV32 (uint32_t doc_id, const ProfileField *fieldPtr, KV32 value)
{
    uint32_t                segPos   = doc_id >> PROFILE_SEG_SIZE_BIT_NUM;
    uint32_t                inSegPos = doc_id &  PROFILE_SEG_SIZE_BIT_MASK;
    AttributeGroupResource *pGroup   = _profileResc->groupArr[fieldPtr->groupID];

    if (!fieldPtr->isEncoded) {
        // 字段不是bit字段，且不是编码表字段
        KV32 *pValue = (KV32 *)(pGroup->segAddressArr[segPos]
                + (pGroup->unitSpace * inSegPos) + fieldPtr->offset);
        *pValue = value;
    }
    else {
        // 字段不是bit字段，是编码表字段
        uint32_t *pEncodeValue = (uint32_t*)(pGroup->segAddressArr[segPos]
                + (pGroup->unitSpace * inSegPos) + fieldPtr->offset);
        char buf[1024];
        int len = snprintf(buf, 1024, "%d:%d", value.key, value.value);
        if (unlikely(len >= 1024)) {
            len = 1023;
        }

        uint32_t encodeValue = fieldPtr->encodeFilePtr->getEncodeIdx(buf, len);
        if (encodeValue == INVALID_NODE_POS) {
            if (unlikely(!fieldPtr->encodeFilePtr->addEncode(buf, len, encodeValue))) {
                TERR("INDEXLIB: ProfileDocAccessor setFloat error, add new encode value error!");
                return KS_EINTR;
            }
        }

        *pEncodeValue = encodeValue;
    }
    setSyncDocId(doc_id);
    return 0;
}

int ProfileDocAccessor::setUInt32(uint32_t doc_id, const ProfileField *fieldPtr, uint32_t value)
{
    SET_UINT_LOGIC(DT_UINT32, uint32_t, "%u");
}

int ProfileDocAccessor::setInt64 (uint32_t doc_id, const ProfileField *fieldPtr, int64_t  value)
{
    SET_INT_LOGIC(DT_INT64, int64_t, "%ld");
}

int ProfileDocAccessor::setUInt64(uint32_t doc_id, const ProfileField *fieldPtr, uint64_t value)
{
    SET_UINT_LOGIC(DT_UINT64, uint64_t, "%lu");
}

int ProfileDocAccessor::setRepeatedInt8  (uint32_t doc_id, const ProfileField *fieldPtr, const char *value, char delim)
{
    if (fieldPtr->multiValNum == 0) {
        SET_REPEATED_VALUE_LOGIC(DT_INT8)
    }
    else {
        SET_CERTAIN_MULTI_INT_LOGIC(fieldPtr->defaultEmpty.EV_INT8, int8_t, strtol)
    }
}

int ProfileDocAccessor::setRepeatedUInt8 (uint32_t doc_id, const ProfileField *fieldPtr, const char *value, char delim)
{
    if (fieldPtr->multiValNum == 0) {
        SET_REPEATED_VALUE_LOGIC(DT_UINT8)
    }
    else {
        SET_CERTAIN_MULTI_INT_LOGIC(fieldPtr->defaultEmpty.EV_UINT8, uint8_t, strtoul)
    }
}

int ProfileDocAccessor::setRepeatedInt16 (uint32_t doc_id, const ProfileField *fieldPtr, const char *value, char delim)
{
    if (fieldPtr->multiValNum == 0) {
        SET_REPEATED_VALUE_LOGIC(DT_INT16)
    }
    else {
        SET_CERTAIN_MULTI_INT_LOGIC(fieldPtr->defaultEmpty.EV_INT16, int16_t, strtol)
    }
}

int ProfileDocAccessor::setRepeatedUInt16(uint32_t doc_id, const ProfileField *fieldPtr, const char *value, char delim)
{
    if (fieldPtr->multiValNum == 0) {
        SET_REPEATED_VALUE_LOGIC(DT_UINT16)
    }
    else {
        SET_CERTAIN_MULTI_INT_LOGIC(fieldPtr->defaultEmpty.EV_UINT16, uint16_t, strtoul)
    }
}

int ProfileDocAccessor::setRepeatedInt32 (uint32_t doc_id, const ProfileField *fieldPtr, const char *value, char delim)
{
    if (fieldPtr->multiValNum == 0) {
        SET_REPEATED_VALUE_LOGIC(DT_INT32)
    }
    else {
        SET_CERTAIN_MULTI_INT_LOGIC(fieldPtr->defaultEmpty.EV_INT32, int32_t, strtol)
    }
}

int ProfileDocAccessor::setRepeatedUInt32(uint32_t doc_id, const ProfileField *fieldPtr, const char *value, char delim)
{
    if (fieldPtr->multiValNum == 0) {
        SET_REPEATED_VALUE_LOGIC(DT_UINT32)
    }
    else {
        SET_CERTAIN_MULTI_INT_LOGIC(fieldPtr->defaultEmpty.EV_UINT32, uint32_t, strtoul)
    }
}

int ProfileDocAccessor::setRepeatedInt64 (uint32_t doc_id, const ProfileField *fieldPtr, const char *value, char delim)
{
    if (fieldPtr->multiValNum == 0) {
        SET_REPEATED_VALUE_LOGIC(DT_INT64)
    }
    else {
        SET_CERTAIN_MULTI_INT_LOGIC(fieldPtr->defaultEmpty.EV_INT64, int64_t, strtoll)
    }
}

int ProfileDocAccessor::setRepeatedUInt64(uint32_t doc_id, const ProfileField *fieldPtr, const char *value, char delim)
{
    if (fieldPtr->multiValNum == 0) {
        SET_REPEATED_VALUE_LOGIC(DT_UINT64)
    }
    else {
        SET_CERTAIN_MULTI_INT_LOGIC(fieldPtr->defaultEmpty.EV_UINT64, uint64_t, strtoull)
    }
}

int ProfileDocAccessor::setRepeatedFloat (uint32_t doc_id, const ProfileField *fieldPtr, const char *value, char delim)
{
    if (fieldPtr->multiValNum == 0) {
        SET_REPEATED_VALUE_LOGIC(DT_FLOAT)
    }
    else {
        SET_CERTAIN_MULTI_FLOAT_LOGIC(fieldPtr->defaultEmpty.EV_FLOAT, float, strtof)
    }
}

int ProfileDocAccessor::setRepeatedDouble(uint32_t doc_id, const ProfileField *fieldPtr, const char *value, char delim)
{
    if (fieldPtr->multiValNum == 0) {
        SET_REPEATED_VALUE_LOGIC(DT_DOUBLE)
    }
    else {
        SET_CERTAIN_MULTI_FLOAT_LOGIC(fieldPtr->defaultEmpty.EV_DOUBLE, double, strtod)
    }
}

int ProfileDocAccessor::setRepeatedString(uint32_t doc_id, const ProfileField *fieldPtr, const char *value, char delim)
{
    SET_REPEATED_VALUE_LOGIC(DT_STRING)
}

int ProfileDocAccessor::setRepeatedKV32(uint32_t doc_id, const ProfileField *fieldPtr, const char *value, char delim)
{
    SET_REPEATED_VALUE_LOGIC(DT_KV32)
}

/**
 * 同步数据到硬盘
 */
void ProfileDocAccessor::setSyncDocId(uint32_t docid)
{
    if (_profileResc->syncMgr) {
        _profileResc->syncMgr->setDocId(docid);
    }
}


}

