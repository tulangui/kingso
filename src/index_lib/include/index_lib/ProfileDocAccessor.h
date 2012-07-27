/*********************************************************************
 * $Author: pujian $
 *
 * $LastChangedBy: pujian $
 *
 * $Revision: 2577 $
 *
 * $LastChangedDate: 2011-03-09 09:50:55 +0800 (Wed, 09 Mar 2011) $
 *
 * $Id: ProfileDocAccessor.h 2577 2011-03-09 01:50:55Z pujian $
 *
 * $Brief: Profile数据的读写操作接口类 $
 ********************************************************************/
#ifndef KINGSO_INDEXLIB_PROFILEDOCACCESSOR_H_
#define KINGSO_INDEXLIB_PROFILEDOCACCESSOR_H_

#include <stdio.h>
#include <stdlib.h>

#include "util/HashMap.hpp"
#include "IndexLib.h"
#include "ProfileStruct.h"
#include "ProfileMultiValueIterator.h"
#include "ProfileMMapFile.h"
#include "ProfileEncodeFile.h"

#ifdef DEBUG   //////////////////// DEBUG BEGIN /////////////////////

// getInt8/INT16/INT32/INT64共用的查询逻辑
#define GET_INT_LOGIC(FIELD_TYPE, DATATYPE, INVALID_RETURN) \
            if (unlikely(doc_id >= _profileResc->docNum)) { \
                TERR("INDEXLIB: ProfileDocAccessor get int value error, invalid doc id[%u]!", doc_id); \
                return INVALID_RETURN; \
            } \
            if (unlikely(fieldPtr == NULL || fieldPtr->type != FIELD_TYPE)) { \
                TERR("INDEXLIB: ProfileDocAccessor get int value error, invalid argument!"); \
                return INVALID_RETURN; \
            } \
            uint32_t                segPos   = doc_id >> PROFILE_SEG_SIZE_BIT_NUM; \
            uint32_t                inSegPos = doc_id &  PROFILE_SEG_SIZE_BIT_MASK; \
            AttributeGroupResource *pGroup   = _profileResc->groupArr[fieldPtr->groupID]; \
            char                   *pAttrPtr = pGroup->segAddressArr[segPos] \
                                              + (pGroup->unitSpace * inSegPos) + fieldPtr->offset; \
            if (fieldPtr->storeType == MT_ONLY) { \
                DATATYPE *pValue = (DATATYPE *)pAttrPtr; \
                return *pValue; \
            } \
            uint32_t attrValue = *((uint32_t *)pAttrPtr); \
            if (fieldPtr->isBitRecord) { \
                if (!fieldPtr->isEncoded) { \
                    TERR("INDEXLIB: ProfileManager get int value error, bit field should be encode file field!"); \
                    return INVALID_RETURN; \
                } \
                attrValue = (attrValue & fieldPtr->bitRecordPtr->read_mask) >> fieldPtr->bitRecordPtr->bits_move; \
            } \
            DATATYPE *pValue = (DATATYPE *)fieldPtr->encodeFilePtr->getSingleBaseValuePtr(attrValue); \
            return *pValue; \

// getUInt8/UInt16/UInt32/UInt64共用的查询逻辑
#define GET_UINT_LOGIC(FIELD_TYPE, DATATYPE, INVALID_RETURN) \
            if (unlikely(doc_id >= _profileResc->docNum)) { \
                TERR("INDEXLIB: ProfileDocAccessor get int value error, invalid doc id[%u]!", doc_id); \
                return INVALID_RETURN; \
            } \
            if (unlikely(fieldPtr == NULL || fieldPtr->type != FIELD_TYPE)) { \
                TERR("INDEXLIB: ProfileDocAccessor get int value error, invalid argument!"); \
                return INVALID_RETURN; \
            } \
            uint32_t                segPos   = doc_id >> PROFILE_SEG_SIZE_BIT_NUM; \
            uint32_t                inSegPos = doc_id &  PROFILE_SEG_SIZE_BIT_MASK; \
            AttributeGroupResource *pGroup   = _profileResc->groupArr[fieldPtr->groupID]; \
            char                   *pAttrPtr = pGroup->segAddressArr[segPos] \
                                              + (pGroup->unitSpace * inSegPos) + fieldPtr->offset; \
            if (fieldPtr->storeType == MT_ONLY) { \
                DATATYPE *pValue = (DATATYPE*)pAttrPtr; \
                return *pValue; \
            } \
            uint32_t attrValue = *((uint32_t *)pAttrPtr); \
            if (fieldPtr->isBitRecord) { \
                attrValue = (attrValue & fieldPtr->bitRecordPtr->read_mask) >> fieldPtr->bitRecordPtr->bits_move; \
            } \
            if (likely(!fieldPtr->isEncoded)) { \
                return attrValue; \
            } \
            DATATYPE *pValue = (DATATYPE *)fieldPtr->encodeFilePtr->getSingleBaseValuePtr(attrValue); \
            return *pValue; \

// getRepeatedInt8/16/32/64共用的查询逻辑
#define GET_REPEATED_INT_LOGIC(FIELD_TYPE, DATATYPE, INVALID_RETURN) \
            if (unlikely(doc_id >= _profileResc->docNum)) { \
                TERR("INDEXLIB: ProfileDocAccessor get Repeated int error, invalid doc id[%u]!", doc_id); \
                return INVALID_RETURN; \
            } \
            if (unlikely(fieldPtr == NULL || fieldPtr->type != FIELD_TYPE)) { \
                TERR("INDEXLIB: ProfileDocAccessor get Repeated int error, invalid argument!"); \
                return INVALID_RETURN; \
            } \
            if (unlikely(!fieldPtr->isEncoded || fieldPtr->encodeFilePtr == NULL)) { \
                TERR("INDEXLIB: ProfileDocAccessor get Repeated int error, field should be valid encodefile field!");\
                return INVALID_RETURN; \
            } \
            uint32_t                segPos   = doc_id >> PROFILE_SEG_SIZE_BIT_NUM; \
            uint32_t                inSegPos = doc_id &  PROFILE_SEG_SIZE_BIT_MASK; \
            AttributeGroupResource *pGroup   = _profileResc->groupArr[fieldPtr->groupID]; \
            char *pEncodeValue = (char *)(pGroup->segAddressArr[segPos] \
                                          + (pGroup->unitSpace * inSegPos) + fieldPtr->offset); \
            DATATYPE *pValue = NULL; \
            if (unlikely(fieldPtr->storeType != MT_ONLY)) { \
                pValue = (DATATYPE *)fieldPtr->encodeFilePtr->getMultiEncodeValuePtr(*((uint32_t *)pEncodeValue)); \
            } \
            else { \
                pValue = (DATATYPE *)pEncodeValue; \
            } \
            return pValue[index]; \


// getRepeatedUInt8/16/32/64共用的查询逻辑
#define GET_REPEATED_UINT_LOGIC(FIELD_TYPE, DATATYPE, INVALID_RETURN) \
            if (unlikely(doc_id >= _profileResc->docNum)) { \
                TERR("INDEXLIB: ProfileDocAccessor get Repeated int error, invalid doc id[%u]!", doc_id); \
                return INVALID_RETURN; \
            } \
            if (unlikely(fieldPtr == NULL || fieldPtr->type != FIELD_TYPE)) { \
                TERR("INDEXLIB: ProfileDocAccessor get Repeated int error, invalid argument!"); \
                return INVALID_RETURN; \
            } \
            if (unlikely(!fieldPtr->isEncoded || fieldPtr->encodeFilePtr == NULL)) { \
                TERR("INDEXLIB: ProfileDocAccessor get Repeated int error, field should be valid encodefile field!");\
                return INVALID_RETURN; \
            } \
            uint32_t                segPos   = doc_id >> PROFILE_SEG_SIZE_BIT_NUM; \
            uint32_t                inSegPos = doc_id &  PROFILE_SEG_SIZE_BIT_MASK; \
            AttributeGroupResource *pGroup   = _profileResc->groupArr[fieldPtr->groupID]; \
            char *pEncodeValue = (char *)(pGroup->segAddressArr[segPos] \
                                          + (pGroup->unitSpace * inSegPos) + fieldPtr->offset); \
            DATATYPE *pValue = NULL; \
            if (unlikely(fieldPtr->storeType != MT_ONLY)) { \
                pValue = (DATATYPE *)fieldPtr->encodeFilePtr->getMultiEncodeValuePtr(*((uint32_t *)pEncodeValue)); \
            } \
            else { \
                pValue = (DATATYPE *)pEncodeValue; \
            } \
            return pValue[index]; \

// get varint编码的int32/int64/uint32/uint64对应的逻辑
#define GET_REPEATED_VARINT_LOGIC(ENCODE_FUNC) \
            uint32_t                segPos   = doc_id >> PROFILE_SEG_SIZE_BIT_NUM; \
            uint32_t                inSegPos = doc_id &  PROFILE_SEG_SIZE_BIT_MASK; \
            AttributeGroupResource *pGroup   = _profileResc->groupArr[fieldPtr->groupID]; \
            uint32_t *pEncodeValue = (uint32_t *)(pGroup->segAddressArr[segPos] + (pGroup->unitSpace * inSegPos) \
                                                  + fieldPtr->offset); \
            return fieldPtr->encodeFilePtr->ENCODE_FUNC(*pEncodeValue, index); \

#else  ///////////////// NOT DEBUG  ///////////////////

// getInt8/INT16/INT32/INT64共用的查询逻辑
#define GET_INT_LOGIC(FIELD_TYPE, DATATYPE, INVALID_RETURN) \
            uint32_t                segPos   = doc_id >> PROFILE_SEG_SIZE_BIT_NUM; \
            uint32_t                inSegPos = doc_id &  PROFILE_SEG_SIZE_BIT_MASK; \
            AttributeGroupResource *pGroup   = _profileResc->groupArr[fieldPtr->groupID]; \
            char                   *pAttrPtr = pGroup->segAddressArr[segPos] \
                                              + (pGroup->unitSpace * inSegPos) + fieldPtr->offset; \
            if (fieldPtr->storeType == MT_ONLY) { \
                DATATYPE *pValue = (DATATYPE *)pAttrPtr; \
                return *pValue; \
            } \
            uint32_t attrValue = *((uint32_t *)pAttrPtr); \
            if (fieldPtr->isBitRecord) { \
                attrValue = (attrValue & fieldPtr->bitRecordPtr->read_mask) >> fieldPtr->bitRecordPtr->bits_move; \
            } \
            DATATYPE *pValue = (DATATYPE *)fieldPtr->encodeFilePtr->getSingleBaseValuePtr(attrValue); \
            return *pValue; \

// getUInt8/UInt16/UInt32/UInt64共用的查询逻辑
#define GET_UINT_LOGIC(FIELD_TYPE, DATATYPE, INVALID_RETURN) \
            uint32_t                segPos   = doc_id >> PROFILE_SEG_SIZE_BIT_NUM; \
            uint32_t                inSegPos = doc_id &  PROFILE_SEG_SIZE_BIT_MASK; \
            AttributeGroupResource *pGroup   = _profileResc->groupArr[fieldPtr->groupID]; \
            char                   *pAttrPtr = pGroup->segAddressArr[segPos] \
                                              + (pGroup->unitSpace * inSegPos) + fieldPtr->offset; \
            if (fieldPtr->storeType == MT_ONLY) { \
                DATATYPE *pValue = (DATATYPE*)pAttrPtr; \
                return *pValue; \
            } \
            uint32_t attrValue = *((uint32_t *)pAttrPtr); \
            if (fieldPtr->isBitRecord) { \
                attrValue = (attrValue & fieldPtr->bitRecordPtr->read_mask) >> fieldPtr->bitRecordPtr->bits_move; \
            } \
            if (likely(!fieldPtr->isEncoded)) { \
                return attrValue; \
            } \
            DATATYPE *pValue = (DATATYPE *)fieldPtr->encodeFilePtr->getSingleBaseValuePtr(attrValue); \
            return *pValue; \

// getRepeatedInt8/16/32/64共用的查询逻辑
#define GET_REPEATED_INT_LOGIC(FIELD_TYPE, DATATYPE, INVALID_RETURN) \
            uint32_t                segPos   = doc_id >> PROFILE_SEG_SIZE_BIT_NUM; \
            uint32_t                inSegPos = doc_id &  PROFILE_SEG_SIZE_BIT_MASK; \
            AttributeGroupResource *pGroup = _profileResc->groupArr[fieldPtr->groupID]; \
            char *pEncodeValue = (char *)(pGroup->segAddressArr[segPos] \
                                          + (pGroup->unitSpace * inSegPos) + fieldPtr->offset); \
            DATATYPE *pValue = NULL; \
            if (unlikely(fieldPtr->storeType != MT_ONLY)) { \
                pValue = (DATATYPE *)fieldPtr->encodeFilePtr->getMultiEncodeValuePtr(*((uint32_t *)pEncodeValue)); \
            } \
            else { \
                pValue = (DATATYPE *)pEncodeValue; \
            } \
            return pValue[index]; \

// getRepeatedUInt8/16/32/64共用的查询逻辑
#define GET_REPEATED_UINT_LOGIC(FIELD_TYPE, DATATYPE, INVALID_RETURN) \
            uint32_t                segPos   = doc_id >> PROFILE_SEG_SIZE_BIT_NUM; \
            uint32_t                inSegPos = doc_id &  PROFILE_SEG_SIZE_BIT_MASK; \
            AttributeGroupResource *pGroup = _profileResc->groupArr[fieldPtr->groupID]; \
            char *pEncodeValue = (char *)(pGroup->segAddressArr[segPos] \
                                          + (pGroup->unitSpace * inSegPos) + fieldPtr->offset); \
            DATATYPE *pValue = NULL; \
            if (unlikely(fieldPtr->storeType != MT_ONLY)) { \
                pValue = (DATATYPE *)fieldPtr->encodeFilePtr->getMultiEncodeValuePtr(*((uint32_t *)pEncodeValue)); \
            } \
            else { \
                pValue = (DATATYPE *)pEncodeValue; \
            } \
            return pValue[index]; \

// get varint编码的int32/int64/uint32/uint64对应的逻辑
#define GET_REPEATED_VARINT_LOGIC(ENCODE_FUNC) \
            uint32_t                segPos   = doc_id >> PROFILE_SEG_SIZE_BIT_NUM; \
            uint32_t                inSegPos = doc_id &  PROFILE_SEG_SIZE_BIT_MASK; \
            AttributeGroupResource *pGroup   = _profileResc->groupArr[fieldPtr->groupID]; \
            uint32_t *pEncodeValue = (uint32_t *)(pGroup->segAddressArr[segPos] + (pGroup->unitSpace * inSegPos) \
                                                  + fieldPtr->offset); \
            return fieldPtr->encodeFilePtr->ENCODE_FUNC(*pEncodeValue, index); \

#endif ////////////////  END DEBUG  ///////////////////

namespace index_lib
{

/**
 * ProfileDocAccessor 类的定义
 */
class ProfileDocAccessor
{
public:
    /**
     * 类的构造函数
     * @param  pResc   ProfileResource结构体指针，内部描述了Profile的属性分组资源情况
     */
    ProfileDocAccessor(ProfileResource *pResc);

    /**
     * 类的析构函数
     */
    ~ProfileDocAccessor();

    /**
     * 向DocAccessor添加可以通过内部trie树查询的ProfileField
     * @param  fieldPtr    ProfileField对象指针
     * @return             0,OK;other,ERROR
     */
    int addProfileField(const ProfileField * fieldPtr);

    /**
     * 从DocAccessor查询字段对应的ProfileField对象指针
     * @param  name        目标字段名
     * @return             Not NULL,字段的ProfileField指针; NULL,ERROR
     */
    inline const ProfileField* getProfileField(const char* name)
    {
        const ProfileField * pField = NULL;
        util::HashMap<const char*, const ProfileField *>::iterator it = _fieldMap.find(name);
        if(it != _fieldMap.end())
        {
            pField = it->value;
        }
        return pField;
    }

    /**
     * 查询目标字段对应的空值结果
     * @param  fieldPtr    ProfileField对象指针（需要外部保证该指针不为空）
     * @return             EmptyValue类型的空值结构
     */
    inline EmptyValue getFieldEmptyValue(const ProfileField * fieldPtr) const
    {
        return fieldPtr->defaultEmpty;
    }

    /**
     * 获得目标字段的多值属性情况（单值、多值、变长）
     * @param  fieldPtr    目标字段的ProfileField指针（需要外部保证该指针不为空）
     * @return             目标字段的多值标识(0:变长，1:单值, N:多值)
     */
    inline uint32_t         getFieldMultiValNum(const ProfileField *fieldPtr) const
    {
        return fieldPtr->multiValNum;
    }

    /**
     * 获得目标字段的数据类型
     * @param  fieldPtr    目标字段的ProfileField指针（需要外部保证该指针不为空）
     * @return             目标字段的数据类型枚举值
     */
    inline PF_DATA_TYPE    getFieldDataType   (const ProfileField *fieldPtr) const
    {
        return fieldPtr->type;
    }

    /**
     * 获得目标字段的字段Flag标识
     * @param  fieldPtr    目标字段的ProfileField指针（需要外部保证该指针不为空）
     * @return             目标字段的FieldFlag类型枚举值
     */
    inline PF_FIELD_FLAG   getFieldFlag       (const ProfileField *fieldPtr) const
    {
        return fieldPtr->flag;
    }

    /**
     * 获得目标字段的字段名
     * @param  fieldPtr    目标字段的ProfileField指针（需要外部保证该指针不为空）
     * @return             目标字段的字段名
     */
    inline const char*     getFieldName       (const ProfileField *fieldPtr) const
    {
        return fieldPtr->name;
    }

    /**
     * 获得目标doc的目标字段的字段值个数
     * @param  doc_id      目标doc的docID（需要外部保证查询的docid不超过max_docid）
     * @param  fieldPtr      目标字段的ProfileField指针（需要外部保证该指针不为空）
     * @return             目标doc特定字段的字段值个数
     */
    inline uint32_t    getValueNum   (uint32_t doc_id, const ProfileField *fieldPtr) const
    {
        #ifdef DEBUG
        if (unlikely(doc_id >= _profileResc->docNum)) {
            TERR("INDEXLIB: ProfileDocAccessor getValueNum error, invalid doc id[%u]!", doc_id);
            return 0;
        }
        if (unlikely(fieldPtr == NULL)) {
            TERR("INDEXLIB: ProfileDocAccessor getValueNum error, invalid argument(NULL pointer)!");
            return 0;
        }
        #endif

        if (fieldPtr->multiValNum != 0) {
            // 单值字段
            return fieldPtr->multiValNum;
        }
        // 编码字段
        uint32_t                segPos       = doc_id >> PROFILE_SEG_SIZE_BIT_NUM;
        uint32_t                inSegPos     = doc_id &  PROFILE_SEG_SIZE_BIT_MASK;
        AttributeGroupResource *pGroup       = _profileResc->groupArr[fieldPtr->groupID];
        uint32_t               *pEncodeValue = (uint32_t *)(pGroup->segAddressArr[segPos]
                                                            + (pGroup->unitSpace * inSegPos)
                                                            + fieldPtr->offset);

        return fieldPtr->encodeFilePtr->getMultiEncodeValueNum(*pEncodeValue);
    }


    /////////////////////////////    获取字段值    //////////////////////////////
    /**
     * 获得目标doc的目标单值字段的字段值(DT_INT8/16/32/64类型)
     * @param  doc_id      目标doc的docID（需要外部保证查询的docid不超过max_docid）
     * @param  fieldPtr      目标字段的ProfileField指针（需要外部保证该指针不为空）
     * @return             目标字段的字段值
     */
    inline int64_t     getInt   (uint32_t doc_id, const ProfileField *fieldPtr) const
    {
        #ifdef DEBUG
        if (unlikely(doc_id >= _profileResc->docNum)) {
            TERR("INDEXLIB: ProfileDocAccessor getInt error, invalid doc id[%u]!", doc_id);
            return INVALID_INT64;
        }
        if (unlikely(fieldPtr == NULL)) {
            TERR("INDEXLIB: ProfileDocAccessor getInt error, invalid argument(NULL pointer)!");
            return INVALID_INT64;
        }
        #endif

        uint32_t                segPos   = doc_id >> PROFILE_SEG_SIZE_BIT_NUM;
        uint32_t                inSegPos = doc_id &  PROFILE_SEG_SIZE_BIT_MASK;
        AttributeGroupResource *pGroup   = _profileResc->groupArr[fieldPtr->groupID];
        char                   *pAttrPtr = pGroup->segAddressArr[segPos] + (pGroup->unitSpace * inSegPos)
                                           + fieldPtr->offset;

        if (fieldPtr->type == DT_INT32) {
            if (fieldPtr->storeType == MT_ONLY) {
                // 非bit字段，非编码字段
                int32_t *pValue = (int32_t *)pAttrPtr;
                return (*pValue);
            }

            #ifdef DEBUG
            if (unlikely(!fieldPtr->isEncoded)) {
                // 是bit字段，不是编码字段
                TERR("INDEXLIB: ProfileDocAccessor getInt error, bit field of int type must be encodefile field!");
                return INVALID_INT64;
            }
            #endif

            uint32_t attrValue = *((uint32_t *)pAttrPtr);
            if (fieldPtr->isBitRecord) {
                attrValue = (attrValue & fieldPtr->bitRecordPtr->read_mask) >> fieldPtr->bitRecordPtr->bits_move;
            }
            int32_t *pValue = (int32_t *)fieldPtr->encodeFilePtr->getSingleBaseValuePtr(attrValue);
            return *pValue;
        }
        else if (fieldPtr->type == DT_INT64) {
            if (fieldPtr->storeType == MT_ONLY) {
                int64_t *pValue = (int64_t *)pAttrPtr;
                return (*pValue);
            }

            #ifdef DEBUG
            if (unlikely(!fieldPtr->isEncoded)) {
                // 字段是bit字段，不是编码字段
                TERR("INDEXLIB: ProfileDocAccessor getInt error, bit field of int type must be encodefile field!");
                return INVALID_INT64;
            }
            #endif

            uint32_t attrValue = *((uint32_t *)pAttrPtr);
            if (fieldPtr->isBitRecord) {
                attrValue = (attrValue & fieldPtr->bitRecordPtr->read_mask) >> fieldPtr->bitRecordPtr->bits_move;
            }
            int64_t *pValue = (int64_t*)fieldPtr->encodeFilePtr->getSingleBaseValuePtr(attrValue);
            return *pValue;
        }
        else if (fieldPtr->type == DT_INT8) {
            if (fieldPtr->storeType == MT_ONLY) {
                int8_t  *pValue = (int8_t *)pAttrPtr;
                return (*pValue);
            }

            #ifdef DEBUG
            if (unlikely(!fieldPtr->isEncoded)) {
                // 字段是bit字段，不是编码字段
                TERR("INDEXLIB: ProfileDocAccessor getInt error, bit field of int type must be encodefile field!");
                return INVALID_INT64;
            }
            #endif

            uint32_t attrValue = *((uint32_t *)pAttrPtr);
            if (fieldPtr->isBitRecord) {
                attrValue = (attrValue & fieldPtr->bitRecordPtr->read_mask) >> fieldPtr->bitRecordPtr->bits_move;
            }
            int8_t *pValue = (int8_t*)fieldPtr->encodeFilePtr->getSingleBaseValuePtr(attrValue);
            return *pValue;
        }
        else if (fieldPtr->type == DT_INT16) {
            if (fieldPtr->storeType == MT_ONLY) {
                int16_t *pValue = (int16_t *)pAttrPtr;
                return (*pValue);
            }

            #ifdef DEBUG
            if (unlikely(!fieldPtr->isEncoded)) {
                // 字段是bit字段，不是编码字段
                TERR("INDEXLIB: ProfileDocAccessor getInt error, bit field of int type must be encodefile field!");
                return INVALID_INT64;
            }
            #endif

            uint32_t attrValue = *((uint32_t *)pAttrPtr);
            if (fieldPtr->isBitRecord) {
                attrValue = (attrValue & fieldPtr->bitRecordPtr->read_mask) >> fieldPtr->bitRecordPtr->bits_move;
            }
            int16_t *pValue = (int16_t*)fieldPtr->encodeFilePtr->getSingleBaseValuePtr(attrValue);
            return *pValue;
        }
        #ifdef DEBUG
        TERR("INDEXLIB: ProfileDocAccessor getInt error, wrong type [%d]!", fieldPtr->type);
        #endif

        return INVALID_INT64;
    }

    /**
     * 获得目标doc的目标单值字段的字段值(DT_UINT8/16/32/64类型)
     * @param  doc_id      目标doc的docID（需要外部保证查询的docid不超过max_docid）
     * @param  fieldPtr      目标字段的ProfileField指针（需要外部保证该指针不为空）
     * @return             目标字段的字段值
     */
    inline uint64_t    getUInt  (uint32_t doc_id, const ProfileField *fieldPtr) const
    {
        #ifdef DEBUG
        if (unlikely(doc_id >= _profileResc->docNum)) {
            TERR("INDEXLIB: ProfileDocAccessor getUInt error, invalid doc id[%u]!", doc_id);
            return INVALID_UINT64;
        }

        if (unlikely(fieldPtr == NULL)) {
            TERR("INDEXLIB: ProfileDocAccessor getUInt error, invalid argument(NULL pointer)!");
            return INVALID_UINT64;
        }
        #endif

        uint32_t                segPos   = doc_id >> PROFILE_SEG_SIZE_BIT_NUM;
        uint32_t                inSegPos = doc_id &  PROFILE_SEG_SIZE_BIT_MASK;
        AttributeGroupResource *pGroup   = _profileResc->groupArr[fieldPtr->groupID];
        char                   *pAttrPtr = pGroup->segAddressArr[segPos] + (pGroup->unitSpace * inSegPos)
                                           + fieldPtr->offset;

        if (fieldPtr->type == DT_UINT32) {
            if (fieldPtr->storeType == MT_ONLY) {
                uint32_t *pValue = (uint32_t *)pAttrPtr;
                return (*pValue);
            }

            uint32_t attrValue = *((uint32_t *)pAttrPtr);
            if (fieldPtr->isBitRecord) {
                attrValue = (attrValue & fieldPtr->bitRecordPtr->read_mask) >> fieldPtr->bitRecordPtr->bits_move;
            }
            if (likely(!fieldPtr->isEncoded)) {
                return attrValue;
            }
            uint32_t *pValue = (uint32_t*)fieldPtr->encodeFilePtr->getSingleBaseValuePtr(attrValue);
            return *pValue;
        }
        else if (fieldPtr->type == DT_UINT64) {
            if (fieldPtr->storeType == MT_ONLY) {
                uint64_t *pValue = (uint64_t *)pAttrPtr;
                return (*pValue);
            }

            uint32_t attrValue = *((uint32_t *)pAttrPtr);
            if (fieldPtr->isBitRecord) {
                attrValue = (attrValue & fieldPtr->bitRecordPtr->read_mask) >> fieldPtr->bitRecordPtr->bits_move;
            }
            if (likely(!fieldPtr->isEncoded)) {
                return attrValue;
            }
            uint64_t *pValue = (uint64_t*)fieldPtr->encodeFilePtr->getSingleBaseValuePtr(attrValue);
            return *pValue;
        }
        else if (fieldPtr->type == DT_UINT8) {
            if (fieldPtr->storeType == MT_ONLY) {
                uint8_t *pValue = (uint8_t *)pAttrPtr;
                return (*pValue);
            }

            uint32_t attrValue = *((uint32_t *)pAttrPtr);
            if (fieldPtr->isBitRecord) {
                attrValue = (attrValue & fieldPtr->bitRecordPtr->read_mask) >> fieldPtr->bitRecordPtr->bits_move;
            }
            if (likely(!fieldPtr->isEncoded)) {
                return attrValue;
            }
            uint8_t *pValue = (uint8_t*)fieldPtr->encodeFilePtr->getSingleBaseValuePtr(attrValue);
            return *pValue;
        }
        else if (fieldPtr->type == DT_UINT16) {
            if (fieldPtr->storeType == MT_ONLY) {
                uint16_t *pValue = (uint16_t *)pAttrPtr;
                return (*pValue);
            }

            uint32_t attrValue = *((uint32_t *)pAttrPtr);
            if (fieldPtr->isBitRecord) {
                attrValue = (attrValue & fieldPtr->bitRecordPtr->read_mask) >> fieldPtr->bitRecordPtr->bits_move;
            }
            if (likely(!fieldPtr->isEncoded)) {
                return attrValue;
            }
            uint16_t *pValue = (uint16_t*)fieldPtr->encodeFilePtr->getSingleBaseValuePtr(attrValue);
            return *pValue;
        }
        #ifdef DEBUG
        TERR("INDEXLIB: ProfileDocAccessor getUInt error, wrong type [%d]!", fieldPtr->type);
        #endif

        return INVALID_UINT64;
    }

    /**
     * 获得目标doc的目标单值字段的字段值(DT_FLOAT类型字段)
     * @param  doc_id      目标doc的docID（需要外部保证查询的docid不超过max_docid）
     * @param  fieldPtr      目标字段的ProfileField指针（需要外部保证fieldPtr不为空）
     * @return             目标字段的字段值
     */
    inline float       getFloat (uint32_t doc_id, const ProfileField *fieldPtr) const
    {
        #ifdef DEBUG
        if (unlikely(doc_id >= _profileResc->docNum)) {
            TERR("INDEXLIB: ProfileDocAccessor getFloat error, invalid doc id[%u]!", doc_id);
            return INVALID_FLOAT;
        }

        if (unlikely(fieldPtr == NULL || fieldPtr->type != DT_FLOAT)) {
            TERR("INDEXLIB: ProfileDocAccessor getFloat error, invalid argument!");
            return INVALID_FLOAT;
        }
        #endif

        uint32_t                segPos   = doc_id >> PROFILE_SEG_SIZE_BIT_NUM;
        uint32_t                inSegPos = doc_id &  PROFILE_SEG_SIZE_BIT_MASK;
        AttributeGroupResource *pGroup   = _profileResc->groupArr[fieldPtr->groupID];
        char                   *pAttrPtr = pGroup->segAddressArr[segPos] + (pGroup->unitSpace * inSegPos) + fieldPtr->offset;
        if (fieldPtr->storeType == MT_ONLY) {
            float *pValue = (float *)pAttrPtr;
            return *pValue;
        }

        uint32_t attrValue = *((uint32_t *)pAttrPtr);
        if (fieldPtr->isBitRecord) {
            #ifdef DEBUG
            if (!fieldPtr->isEncoded) {
                // 字段是bit字段，不是编码字段
                TERR("INDEXLIB: ProfileManager getFloat error, bit field of float type should be encode file field!");
                return INVALID_FLOAT;
            }
            #endif
            attrValue = (attrValue & fieldPtr->bitRecordPtr->read_mask) >> fieldPtr->bitRecordPtr->bits_move;
        }
        float *pValue = (float *)fieldPtr->encodeFilePtr->getSingleBaseValuePtr(attrValue);
        return *pValue;
    }

    /**
     * 获得目标doc的目标单值字段的字段值(DT_DOUBLE类型字段)
     * @param  doc_id      目标doc的docID（需要外部保证查询的docid不超过max_docid）
     * @param  fieldPtr      目标字段的ProfileField指针（需要外部保证fieldPtr不为空）
     * @return             目标字段的字段值
     */
    inline double      getDouble(uint32_t doc_id, const ProfileField *fieldPtr) const
    {
        #ifdef DEBUG
        if (unlikely(doc_id >= _profileResc->docNum)) {
            TERR("INDEXLIB: ProfileDocAccessor getDouble error, invalid doc id[%u]!", doc_id);
            return INVALID_DOUBLE;
        }
        if (unlikely(fieldPtr == NULL || fieldPtr->type != DT_DOUBLE)) {
            TERR("INDEXLIB: ProfileDocAccessor getDouble error, invalid argument!");
            return INVALID_DOUBLE;
        }
        #endif

        uint32_t                segPos   = doc_id >> PROFILE_SEG_SIZE_BIT_NUM;
        uint32_t                inSegPos = doc_id &  PROFILE_SEG_SIZE_BIT_MASK;
        AttributeGroupResource *pGroup   = _profileResc->groupArr[fieldPtr->groupID];
        char                   *pAttrPtr = pGroup->segAddressArr[segPos] + (pGroup->unitSpace * inSegPos)
                                           + fieldPtr->offset;
        if (fieldPtr->storeType == MT_ONLY) {
            double *pValue = (double *)pAttrPtr;
            return *pValue;
        }

        uint32_t attrValue = *((uint32_t *)pAttrPtr);
        if (fieldPtr->isBitRecord) {
            #ifdef DEBUG
            if (!fieldPtr->isEncoded) {
                // 字段是bit字段，不是编码字段
                TERR("INDEXLIB: ProfileManager getDouble error, bit field of double type should be encode file field!");
                return INVALID_DOUBLE;
            }
            #endif
            attrValue = (attrValue & fieldPtr->bitRecordPtr->read_mask) >> fieldPtr->bitRecordPtr->bits_move;
        }
        double *pValue = (double *)fieldPtr->encodeFilePtr->getSingleBaseValuePtr(attrValue);
        return *pValue;
    }

    /**
     * 获得目标doc的目标单值字段的字段值(DT_STRING类型字段)
     * @param  doc_id      目标doc的docID（需要外部保证查询的docid不超过max_docid）
     * @param  fieldPtr      目标字段的ProfileField指针（需要外部保证fieldPtr不为空）
     * @return             目标字段的字段值
     */
    inline const char* getString(uint32_t doc_id, const ProfileField *fieldPtr) const
    {
        #ifdef DEBUG
        if (unlikely(doc_id >= _profileResc->docNum)) {
            TERR("INDEXLIB: ProfileDocAccessor getString error, invalid doc id[%u]!", doc_id);
            return NULL;
        }

        if (unlikely(fieldPtr == NULL || fieldPtr->type != DT_STRING)) {
            TERR("INDEXLIB: ProfileDocAccessor getString error, invalid argument!");
            return NULL;
        }

        if (unlikely(!fieldPtr->isEncoded || fieldPtr->encodeFilePtr == NULL)) {
            TERR("INDEXLIB: ProfileDocAccessor getString error, field should be valid encode file field!");
            return NULL;
        }
        #endif

        uint32_t                segPos   = doc_id >> PROFILE_SEG_SIZE_BIT_NUM;
        uint32_t                inSegPos = doc_id &  PROFILE_SEG_SIZE_BIT_MASK;
        AttributeGroupResource *pGroup   = _profileResc->groupArr[fieldPtr->groupID];
        char                   *pAttrPtr = pGroup->segAddressArr[segPos] + (pGroup->unitSpace * inSegPos)
                                           + fieldPtr->offset;

        uint32_t attrValue = *((uint32_t *)pAttrPtr);
        if (fieldPtr->isBitRecord) {
            // 是bit字段
            attrValue = (attrValue & fieldPtr->bitRecordPtr->read_mask) >> fieldPtr->bitRecordPtr->bits_move;
        }
        return fieldPtr->encodeFilePtr->getSingleStrValuePtr(attrValue);
    }

    /**
     * 获得目标doc的目标非单值(多值/变长)字段的字段值(DT_INT8/16/32/64类型)
     * @param  doc_id      目标doc的docID（需要外部保证查询的docid不超过max_docid）
     * @param  fieldPtr      目标字段的ProfileField指针（需要外部保证ProfileField指针不为空）
     * @param  index       字段值下标,从0开始（需要外部保证查询的index不超过实际字段值个数范围）
     * @return             目标字段的字段值
     */
    inline int64_t     getRepeatedInt   (uint32_t doc_id, const ProfileField *fieldPtr, uint32_t index = 0) const
    {
        #ifdef DEBUG
        if (unlikely(doc_id >= _profileResc->docNum)) {
            TERR("INDEXLIB: ProfileDocAccessor getRepeatedInt error, invalid doc id[%u]!", doc_id);
            return INVALID_INT64;
        }
        if (unlikely(fieldPtr == NULL)) {
            TERR("INDEXLIB: ProfileDocAccessor getRepeatedInt error, invalid argument(NULL pointer)!");
            return INVALID_INT64;
        }
        if (unlikely(!fieldPtr->isEncoded || fieldPtr->encodeFilePtr == NULL)) {
            TERR("INDEXLIB: ProfileDocAccessor getRepeatedInt error, field should be valid encode file field!");
            return INVALID_INT64;
        }
        #endif

        uint32_t                segPos   = doc_id >> PROFILE_SEG_SIZE_BIT_NUM;
        uint32_t                inSegPos = doc_id &  PROFILE_SEG_SIZE_BIT_MASK;
        AttributeGroupResource *pGroup   = _profileResc->groupArr[fieldPtr->groupID];
        uint32_t *pEncodeValue = (uint32_t*)(pGroup->segAddressArr[segPos] + (pGroup->unitSpace * inSegPos)
                                             + fieldPtr->offset);
        if (fieldPtr->type == DT_INT32) {
            if (!fieldPtr->isCompress) {
                int32_t *pValue = NULL;
                if (unlikely(fieldPtr->storeType == MT_ONLY)) {
                    pValue = (int32_t *)pEncodeValue;
                }
                else {
                    pValue = (int32_t *)fieldPtr->encodeFilePtr->getMultiEncodeValuePtr(*pEncodeValue);
                }
                return pValue[index];
            }
            return fieldPtr->encodeFilePtr->getEncodeVarInt32(*pEncodeValue, index);
        }
        if (fieldPtr->type == DT_INT64) {
            if (!fieldPtr->isCompress) {
                int64_t *pValue = NULL;
                if (unlikely(fieldPtr->storeType == MT_ONLY)) {
                    pValue = (int64_t *)pEncodeValue;
                }
                else {
                    pValue = (int64_t *)fieldPtr->encodeFilePtr->getMultiEncodeValuePtr(*pEncodeValue);
                }
                return pValue[index];
            }
            return fieldPtr->encodeFilePtr->getEncodeVarInt64(*pEncodeValue, index);
        }
        if (fieldPtr->type == DT_INT8) {
            int8_t *pValue = NULL;
            if (unlikely(fieldPtr->storeType == MT_ONLY)) {
                pValue = (int8_t *)pEncodeValue;
            }
            else {
                pValue = (int8_t *)fieldPtr->encodeFilePtr->getMultiEncodeValuePtr(*pEncodeValue);
            }
            return pValue[index];
        }
        if (fieldPtr->type == DT_INT16) {
            int16_t *pValue = NULL;
            if (unlikely(fieldPtr->storeType == MT_ONLY)) {
                pValue = (int16_t *)pEncodeValue;
            }
            else {
                pValue = (int16_t *)fieldPtr->encodeFilePtr->getMultiEncodeValuePtr(*pEncodeValue);
            }
            return pValue[index];
        }
        #ifdef DEBUG
        TERR("INDEXLIB: ProfileDocAccessor getRepeatedInt error, wrong type [%d]!", fieldPtr->type);
        #endif

        return INVALID_INT64;
    }

    /**
     * 获得目标doc的目标非单值(多值/变长)字段的字段值(DT_UINT8/16/32/64类型)
     * @param  doc_id      目标doc的docID（需要外部保证查询的docid不超过max_docid）
     * @param  fieldPtr      目标字段的ProfileField指针（需要外部保证ProfileField指针不为空）
     * @param  index       字段值下标,从0开始（需要外部保证查询的index不超过实际字段值个数范围）
     * @return             目标字段的字段值
     */
    inline uint64_t    getRepeatedUInt  (uint32_t doc_id, const ProfileField *fieldPtr, uint32_t index = 0) const
    {
        #ifdef DEBUG
        if (unlikely(doc_id >= _profileResc->docNum)) {
            TERR("INDEXLIB: ProfileDocAccessor getRepeatedUInt error, invalid doc id[%u]!", doc_id);
            return INVALID_UINT64;
        }
        if (unlikely(fieldPtr == NULL)) {
            TERR("INDEXLIB: ProfileDocAccessor getRepeatedUInt error, invalid argument(NULL pointer)!");
            return INVALID_UINT64;
        }
        if (unlikely(!fieldPtr->isEncoded || fieldPtr->encodeFilePtr == NULL)) {
            TERR("INDEXLIB: ProfileDocAccessor getRepeatedUInt error, field should be valid encode file field!");
            return INVALID_UINT64;
        }
        #endif

        uint32_t                segPos   = doc_id >> PROFILE_SEG_SIZE_BIT_NUM;
        uint32_t                inSegPos = doc_id &  PROFILE_SEG_SIZE_BIT_MASK;
        AttributeGroupResource *pGroup   = _profileResc->groupArr[fieldPtr->groupID];
        uint32_t *pEncodeValue = (uint32_t*)(pGroup->segAddressArr[segPos] + (pGroup->unitSpace * inSegPos)
                                             + fieldPtr->offset);
        if (fieldPtr->type ==  DT_UINT32) {
            if (!fieldPtr->isCompress) {
                uint32_t *pValue = NULL;
                if (unlikely(fieldPtr->storeType == MT_ONLY)) {
                    pValue = (uint32_t *)pEncodeValue;
                }
                else {
                    pValue = (uint32_t *)fieldPtr->encodeFilePtr->getMultiEncodeValuePtr(*pEncodeValue);
                }
                return pValue[index];
            }
            return fieldPtr->encodeFilePtr->getEncodeVarUInt32(*pEncodeValue, index);
        }
        if (fieldPtr->type ==  DT_UINT64) {
            if (!fieldPtr->isCompress) {
                uint64_t *pValue = NULL;
                if (unlikely(fieldPtr->storeType == MT_ONLY)) {
                    pValue = (uint64_t *)pEncodeValue;
                }
                else {
                    pValue = (uint64_t *)fieldPtr->encodeFilePtr->getMultiEncodeValuePtr(*pEncodeValue);
                }
                return pValue[index];
            }
            return fieldPtr->encodeFilePtr->getEncodeVarUInt64(*pEncodeValue, index);
        }
        if (fieldPtr->type == DT_UINT8) {
            uint8_t *pValue = NULL;
            if (unlikely(fieldPtr->storeType == MT_ONLY)) {
                pValue = (uint8_t *)pEncodeValue;
            }
            else {
                pValue = (uint8_t *)fieldPtr->encodeFilePtr->getMultiEncodeValuePtr(*pEncodeValue);
            }
            return pValue[index];
        }
        if (fieldPtr->type == DT_UINT16) {
            uint16_t *pValue = NULL;
            if (unlikely(fieldPtr->storeType == MT_ONLY)) {
                pValue = (uint16_t *)pEncodeValue;
            }
            else {
                pValue = (uint16_t *)fieldPtr->encodeFilePtr->getMultiEncodeValuePtr(*pEncodeValue);
            }
            return pValue[index];
        }
        #ifdef DEBUG
        TERR("INDEXLIB: ProfileDocAccessor getRepeatedUInt error, wrong type [%d]!", fieldPtr->type);
        #endif
        return INVALID_UINT64;
    }

    /**
     * 获得目标doc的目标非单值(多值/变长)字段的字段值(DT_FLOAT类型)
     * @param  doc_id      目标doc的docID（需要外部保证查询的docid不超过max_docid）
     * @param  fieldPtr      目标字段的ProfileField指针（需要外部保证ProfileField指针不为空）
     * @param  index       字段值下标,从0开始（需要外部保证查询的index不超过实际字段值个数范围）
     * @return             目标字段的字段值
     */
    inline float       getRepeatedFloat (uint32_t doc_id, const ProfileField *fieldPtr, uint32_t index = 0) const
    {
        #ifdef DEBUG
        if (unlikely(doc_id >= _profileResc->docNum)) {
            TERR("INDEXLIB: ProfileDocAccessor getRepeatedFloat error, invalid doc id[%u]!", doc_id);
            return INVALID_FLOAT;
        }
        if (unlikely(fieldPtr == NULL || fieldPtr->type != DT_FLOAT)) {
            TERR("INDEXLIB: ProfileDocAccessor getRepeatedFloat error, invalid argument!");
            return INVALID_FLOAT;
        }
        if (unlikely(!fieldPtr->isEncoded || fieldPtr->encodeFilePtr == NULL)) {
            TERR("INDEXLIB: ProfileDocAccessor getRepeatedFloat error, field should be valid encode file field!");
            return INVALID_FLOAT;
        }
        #endif

        uint32_t                segPos   = doc_id >> PROFILE_SEG_SIZE_BIT_NUM;
        uint32_t                inSegPos = doc_id &  PROFILE_SEG_SIZE_BIT_MASK;
        AttributeGroupResource *pGroup   = _profileResc->groupArr[fieldPtr->groupID];
        uint32_t *pEncodeValue = (uint32_t*)(pGroup->segAddressArr[segPos] + (pGroup->unitSpace * inSegPos)
                                             + fieldPtr->offset);
        float *pValue = NULL;
        if (unlikely(fieldPtr->storeType == MT_ONLY)) {
            pValue = (float *)pEncodeValue;
        }
        else {
            pValue = (float *)fieldPtr->encodeFilePtr->getMultiEncodeValuePtr(*pEncodeValue);
        }
        return pValue[index];
    }

    /**
     * 获得目标doc的目标非单值(多值/变长)字段的字段值(DT_DOUBLE类型)
     * @param  doc_id      目标doc的docID（需要外部保证查询的docid不超过max_docid）
     * @param  fieldPtr      目标字段的ProfileField指针（需要外部保证ProfileField指针不为空）
     * @param  index       字段值下标,从0开始（需要外部保证查询的index不超过实际字段值个数范围）
     * @return             目标字段的字段值
     */
    inline double      getRepeatedDouble(uint32_t doc_id, const ProfileField *fieldPtr, uint32_t index = 0) const
    {
        #ifdef DEBUG
        if (unlikely(doc_id >= _profileResc->docNum)) {
            TERR("INDEXLIB: ProfileDocAccessor getRepeatedDouble error, invalid doc id[%u]!", doc_id);
            return INVALID_DOUBLE;
        }
        if (unlikely(fieldPtr == NULL || fieldPtr->type != DT_DOUBLE)) {
            TERR("INDEXLIB: ProfileDocAccessor getRepeatedDouble error, invalid argument!");
            return INVALID_DOUBLE;
        }
        if (unlikely(!fieldPtr->isEncoded || fieldPtr->encodeFilePtr == NULL)) {
            TERR("INDEXLIB: ProfileDocAccessor getRepeatedDouble error, field should be valid encode file field!");
            return INVALID_DOUBLE;
        }
        #endif

        uint32_t                segPos   = doc_id >> PROFILE_SEG_SIZE_BIT_NUM;
        uint32_t                inSegPos = doc_id &  PROFILE_SEG_SIZE_BIT_MASK;
        AttributeGroupResource *pGroup   = _profileResc->groupArr[fieldPtr->groupID];
        uint32_t *pEncodeValue = (uint32_t*)(pGroup->segAddressArr[segPos] + (pGroup->unitSpace * inSegPos)
                                             + fieldPtr->offset);
        double *pValue = NULL;
        if (unlikely(fieldPtr->storeType == MT_ONLY)) {
            pValue = (double *)pEncodeValue;
        }
        else {
            pValue = (double *)fieldPtr->encodeFilePtr->getMultiEncodeValuePtr(*pEncodeValue);
        }
        return pValue[index];
    }

    /**
     * 获得目标doc的目标非单值(多值/变长)字段的字段值(DT_STRING类型)
     * @param  doc_id      目标doc的docID（需要外部保证查询的docid不超过max_docid）
     * @param  fieldPtr      目标字段的ProfileField指针（需要外部保证ProfileField指针不为空）
     * @param  index       字段值下标,从0开始（需要外部保证查询的index不超过实际字段值个数范围）
     * @return             目标字段的字段值
     */
    inline const char* getRepeatedString(uint32_t doc_id, const ProfileField *fieldPtr, uint32_t index = 0) const
    {
        #ifdef DEBUG
        if (unlikely(doc_id >= _profileResc->docNum)) {
            TERR("INDEXLIB: ProfileDocAccessor getRepeatedString error, invalid doc id[%u]!", doc_id);
            return NULL;
        }
        if (unlikely(fieldPtr == NULL || fieldPtr->type != DT_STRING)) {
            TERR("INDEXLIB: ProfileDocAccessor getRepeatedString error, invalid argument!");
            return NULL;
        }
        if (unlikely(!fieldPtr->isEncoded || fieldPtr->encodeFilePtr == NULL)) {
            TERR("INDEXLIB: ProfileDocAccessor getRepeatedString error, field should be valid encode file field!");
            return NULL;
        }
        #endif

        uint32_t                segPos   = doc_id >> PROFILE_SEG_SIZE_BIT_NUM;
        uint32_t                inSegPos = doc_id &  PROFILE_SEG_SIZE_BIT_MASK;
        AttributeGroupResource *pGroup   = _profileResc->groupArr[fieldPtr->groupID];
        uint32_t *pEncodeValue = (uint32_t*)(pGroup->segAddressArr[segPos] + (pGroup->unitSpace * inSegPos)
                                             + fieldPtr->offset);

        char *pBase = fieldPtr->encodeFilePtr->getMultiEncodeValuePtr(*pEncodeValue);
        register uint32_t bSize  = 0;
        uint16_t *pSize = NULL;

        uint32_t pos = 0;
        while(pos < index) {
            pSize  = (uint16_t*)(pBase + bSize);
            bSize += (3 + (*pSize));
            ++pos;
        }

        pSize = (uint16_t*)(pBase + bSize);
        bSize += 2;
        return (pBase + bSize);
    }

    /**
     * 整型字段值的细化类型访问接口
     * 由 getInt/setInt 细化展开
     */
    inline int8_t   getInt8  (uint32_t doc_id, const ProfileField *fieldPtr) const
    {
        GET_INT_LOGIC(DT_INT8, int8_t, INVALID_INT8)
    }

    inline uint8_t  getUInt8 (uint32_t doc_id, const ProfileField *fieldPtr) const
    {
        GET_UINT_LOGIC(DT_UINT8, uint8_t, INVALID_UINT8)
    }

    inline int16_t  getInt16 (uint32_t doc_id, const ProfileField *fieldPtr) const
    {
        GET_INT_LOGIC(DT_INT16, int16_t, INVALID_INT16)
    }

    inline uint16_t getUInt16(uint32_t doc_id, const ProfileField *fieldPtr) const
    {
        GET_UINT_LOGIC(DT_UINT16, uint16_t, INVALID_UINT16)
    }

    inline int32_t  getInt32 (uint32_t doc_id, const ProfileField *fieldPtr) const
    {
        GET_INT_LOGIC(DT_INT32, int32_t, INVALID_INT32)
    }

    inline uint32_t getUInt32(uint32_t doc_id, const ProfileField *fieldPtr) const
    {
        GET_UINT_LOGIC(DT_UINT32, uint32_t, INVALID_UINT32)
    }

    inline int64_t  getInt64 (uint32_t doc_id, const ProfileField *fieldPtr) const
    {
        GET_INT_LOGIC(DT_INT64, int64_t, INVALID_INT64)
    }

    inline uint64_t getUInt64(uint32_t doc_id, const ProfileField *fieldPtr) const
    {
        GET_UINT_LOGIC(DT_UINT64, uint64_t, INVALID_UINT64)
    }

    inline KV32 getKV32(uint32_t doc_id, const ProfileField *fieldPtr) const
    {
        KV32 kv;
        kv.key   = INVALID_INT32;
        kv.value = INVALID_INT32;
        GET_INT_LOGIC(DT_KV32, KV32, kv)
    }

    /**
     * 整型字段值的细化类型访问接口
     * 由 getRepeatedInt/setRepeatedInt 细化展开
     */
    inline int8_t   getRepeatedInt8  (uint32_t doc_id, const ProfileField *fieldPtr, uint32_t index = 0) const
    {
        GET_REPEATED_INT_LOGIC(DT_INT8, int8_t, INVALID_INT8)
    }

    inline uint8_t  getRepeatedUInt8 (uint32_t doc_id, const ProfileField *fieldPtr, uint32_t index = 0) const
    {
        GET_REPEATED_UINT_LOGIC(DT_UINT8, uint8_t, INVALID_UINT8)
    }

    inline int16_t  getRepeatedInt16 (uint32_t doc_id, const ProfileField *fieldPtr, uint32_t index = 0) const
    {
        GET_REPEATED_INT_LOGIC(DT_INT16, int16_t, INVALID_INT16)
    }

    inline uint16_t getRepeatedUInt16(uint32_t doc_id, const ProfileField *fieldPtr, uint32_t index = 0) const
    {
        GET_REPEATED_UINT_LOGIC(DT_UINT16, uint16_t, INVALID_UINT16)
    }

    inline int32_t  getRepeatedInt32 (uint32_t doc_id, const ProfileField *fieldPtr, uint32_t index = 0) const
    {
        if (!fieldPtr->isCompress) {
             GET_REPEATED_INT_LOGIC(DT_INT32, int32_t, INVALID_INT32)
        }
        GET_REPEATED_VARINT_LOGIC(getEncodeVarInt32)
    }

    inline uint32_t getRepeatedUInt32(uint32_t doc_id, const ProfileField *fieldPtr, uint32_t index = 0) const
    {
        if (!fieldPtr->isCompress) {
            GET_REPEATED_UINT_LOGIC(DT_UINT32, uint32_t, INVALID_UINT32)
        }
        GET_REPEATED_VARINT_LOGIC(getEncodeVarUInt32)
    }

    inline int64_t  getRepeatedInt64 (uint32_t doc_id, const ProfileField *fieldPtr, uint32_t index = 0) const
    {
        if (!fieldPtr->isCompress) {
            GET_REPEATED_INT_LOGIC(DT_INT64, int64_t, INVALID_INT64)
        }
        GET_REPEATED_VARINT_LOGIC(getEncodeVarInt64)
    }

    inline uint64_t getRepeatedUInt64(uint32_t doc_id, const ProfileField *fieldPtr, uint32_t index = 0) const
    {
        if (!fieldPtr->isCompress) {
            GET_REPEATED_UINT_LOGIC(DT_UINT64, uint64_t, INVALID_UINT64)
        }
        GET_REPEATED_VARINT_LOGIC(getEncodeVarUInt64)
    }

    /**
     * 获取多值字段的多值结果迭代器信息
     * @param  doc_id     目标doc对应的doc_id
     * @param  fieldPtr   目标字段的字段信息结构体指针
     * @param  iterater   获取到的字段多值信息保存到的迭代器对象
     * @return            0, OK; -1, ERROR
     */
    inline int getRepeatedValue(uint32_t doc_id, const ProfileField *fieldPtr, ProfileMultiValueIterator &iterator)
    {
        #ifdef DEBUG
        if (unlikely(doc_id >= _profileResc->docNum)) {
            TERR("INDEXLIB: ProfileDocAccessor getRepeatedValue error, invalid doc id[%u]!", doc_id);
                return KS_EINVAL;
        }
        if (unlikely(fieldPtr == NULL)) {
            TERR("INDEXLIB: ProfileDocAccessor getRepeatedValue error, invalid argument!");
                return KS_EINVAL;
        }
        #endif

        uint32_t                segPos   = doc_id >> PROFILE_SEG_SIZE_BIT_NUM;
        uint32_t                inSegPos = doc_id &  PROFILE_SEG_SIZE_BIT_MASK;
        AttributeGroupResource *pGroup   = _profileResc->groupArr[fieldPtr->groupID];
        uint32_t *pEncodeValue = (uint32_t*)(pGroup->segAddressArr[segPos]
                                             + (pGroup->unitSpace * inSegPos) + fieldPtr->offset);

        uint32_t num   = 0;
        char     *addr = NULL;
        if (unlikely(fieldPtr->storeType == MT_ONLY)) {
            num  = fieldPtr->multiValNum;
            addr = (char *)pEncodeValue;
        }
        else {
            addr = fieldPtr->encodeFilePtr->getMultiEncodeValueInfo(*pEncodeValue, num);
        }
        iterator.setMetaData(num, addr, fieldPtr->isCompress);
        return 0;
    }

    /**
     * 获取多值字段的字段值起始地址
     * @param  doc_id    目标docid
     * @param  fieldPtr  目标字段结构体
     * @return           字段值的起始地址位置
     */
    inline const char* getRepeatedValuePtr(uint32_t doc_id, const ProfileField *fieldPtr) const
    {
        #ifdef DEBUG
        if (unlikely(doc_id >= _profileResc->docNum)) {
            TERR("INDEXLIB: ProfileDocAccessor getRepeatedValuePtr error, invalid doc id[%u]!", doc_id);
                return NULL;
        }
        if (unlikely(fieldPtr == NULL)) {
            TERR("INDEXLIB: ProfileDocAccessor getRepeatedValuePtr error, invalid argument!");
                return NULL;
        }
        #endif

        uint32_t                segPos   = doc_id >> PROFILE_SEG_SIZE_BIT_NUM;
        uint32_t                inSegPos = doc_id &  PROFILE_SEG_SIZE_BIT_MASK;
        AttributeGroupResource *pGroup   = _profileResc->groupArr[fieldPtr->groupID];
        uint32_t *pEncodeValue = (uint32_t*)(pGroup->segAddressArr[segPos]
                                             + (pGroup->unitSpace * inSegPos) + fieldPtr->offset);
        if (unlikely(fieldPtr->storeType == MT_ONLY)) {
            return (char*)pEncodeValue;
        }
        return fieldPtr->encodeFilePtr->getMultiEncodeValuePtr(*pEncodeValue);
    }

    /**
     * 获取正确的ends值
     * @param  doc_id      目标doc的docID
     * @param  fieldPtr    目标ends字段的ProfileField指针
     * @param  curTime     当前时间
     * @param  validPos    有效ends值的位置下标(如果为空则返回-1)
     * @return             有效的ends时间值
     */
    int32_t getValidEndTime(uint32_t doc_id, const ProfileField *fieldPtr, int curTime, int &validPos)
    {
        uint32_t                segPos   = doc_id >> PROFILE_SEG_SIZE_BIT_NUM;
        uint32_t                inSegPos = doc_id &  PROFILE_SEG_SIZE_BIT_MASK;
        AttributeGroupResource *pGroup   = _profileResc->groupArr[fieldPtr->groupID];
        uint32_t *pEncodeValue = (uint32_t*)(pGroup->segAddressArr[segPos]
                                             + (pGroup->unitSpace * inSegPos) + fieldPtr->offset);

        validPos = -1;
        uint32_t  num   = 0;
        int32_t  *pEnds = NULL;
        if (likely(fieldPtr->storeType == MT_ONLY)) {
            num   = fieldPtr->multiValNum;
            pEnds = (int32_t *)pEncodeValue;
        }
        else {
            pEnds = (int32_t *)fieldPtr->encodeFilePtr->getMultiEncodeValueInfo(*pEncodeValue, num);
        }

        int32_t validEnd = fieldPtr->defaultEmpty.EV_INT32;
        for (uint32_t pos = 0; pos < num; pos++) {
            if (unlikely(pEnds[pos] == fieldPtr->defaultEmpty.EV_INT32)) {
                break;
            }
            validEnd = pEnds[pos];
            if (validEnd >= curTime) {
                validPos = pos;
                break;
            }
        }
        return validEnd;
    }

    //////////////////////////////  设置字段值  //////////////////////////////////
    /**
     * 设置目标doc的对应字段值
     * @param  doc_id      目标doc的docID
     * @param  fieldPtr    目标字段的ProfileField指针
     * @param  value       目标字段的字符串形式字段值
     * @param  delim       多值字段的字段值分隔符
     * @return             0, OK; other,ERROR
     */
    int  setFieldValue(uint32_t doc_id, const ProfileField *fieldPtr, const char* value, char delim = ' ');


    /**
     * 在主表中 设置目标doc经过编码后的值
     *
     * @param  doc_id         目标doc的docID
     * @param  fieldPtr       目标字段的ProfileField指针
     * @param  encodeValue    目标字段的字段值经过编码后得到的值
     * @return                0, OK; other,ERROR
     */
    int   setFieldEncodeValue ( uint32_t doc_id, const ProfileField *fieldPtr, uint32_t encodeValue)
    {
        #ifdef DEBUG
        if (unlikely(doc_id >= _profileResc->docNum))
        {
            TERR("error, invalid doc id[%u]!", doc_id);
            return NULL;
        }
        if (unlikely(fieldPtr == NULL))
        {
            TERR("error, invalid argument!");
            return NULL;
        }
        #endif

        uint32_t                 segPos   = doc_id >> PROFILE_SEG_SIZE_BIT_NUM;
        uint32_t                 inSegPos = doc_id &  PROFILE_SEG_SIZE_BIT_MASK;

        AttributeGroupResource * pGroup   = _profileResc->groupArr[ fieldPtr->groupID ];

        if (unlikely( fieldPtr->isBitRecord ))
        {
            uint32_t * pBitRecord = (uint32_t *)(pGroup->segAddressArr[segPos]
                                     + (pGroup->unitSpace * inSegPos) + fieldPtr->offset);

            if ( encodeValue > MAX_BITFIELD_VALUE( fieldPtr->bitRecordPtr->field_len ) )
            {
                TERR("error, add invalid value. more than max value");
                return KS_EFAILED;
            }

            (*pBitRecord) = ((*pBitRecord) & fieldPtr->bitRecordPtr->write_mask)
                               | (encodeValue << fieldPtr->bitRecordPtr->bits_move);

            return 0;
        }

        *((uint32_t * )(pGroup->segAddressArr[ segPos ]
             + (pGroup->unitSpace * inSegPos) + fieldPtr->offset)) = encodeValue ;

        return 0;
    }


    /**
     * 在主表中 获取  目标doc经过编码后的值
     *
     * @param  doc_id         目标doc的docID
     * @param  fieldPtr       目标字段的ProfileField指针
     * @return                编码后的值
     */
    uint32_t  getFieldEncodeValue ( uint32_t doc_id, const ProfileField * fieldPtr )
    {
        #ifdef DEBUG
        if (unlikely(doc_id >= _profileResc->docNum))
        {
            TERR("error, invalid doc id[%u]!", doc_id);
            return NULL;
        }
        if (unlikely(fieldPtr == NULL))
        {
            TERR("error, invalid argument!");
            return NULL;
        }
        #endif

        uint32_t                 segPos   = doc_id >> PROFILE_SEG_SIZE_BIT_NUM;
        uint32_t                 inSegPos = doc_id &  PROFILE_SEG_SIZE_BIT_MASK;

        AttributeGroupResource * pGroup   = _profileResc->groupArr[ fieldPtr->groupID ];
        uint32_t                 value    = *(uint32_t * )(pGroup->segAddressArr[ segPos ]
                                              + (pGroup->unitSpace * inSegPos) + fieldPtr->offset);

        if (unlikely( fieldPtr->isBitRecord ))
        {
            value = (value & fieldPtr->bitRecordPtr->read_mask) >> fieldPtr->bitRecordPtr->bits_move;
        }

        return value;
    }

private:

    /**
     * 设置当前更新管理器对应的docid
     */
    void setSyncDocId(uint32_t docid);

    /**
     * 设置目标doc的目标单值字段的字段值
     * @param  doc_id      目标doc的docID
     * @param  fieldPtr    目标字段的ProfileField指针
     * @param  value       目标字段的字段值
     * @return             0, OK; other,ERROR
     */
    int  setInt8  (uint32_t doc_id, const ProfileField *fieldPtr, int8_t   value);
    int  setUInt8 (uint32_t doc_id, const ProfileField *fieldPtr, uint8_t  value);
    int  setInt16 (uint32_t doc_id, const ProfileField *fieldPtr, int16_t  value);
    int  setUInt16(uint32_t doc_id, const ProfileField *fieldPtr, uint16_t value);
    int  setInt32 (uint32_t doc_id, const ProfileField *fieldPtr, int32_t  value);
    int  setUInt32(uint32_t doc_id, const ProfileField *fieldPtr, uint32_t value);
    int  setInt64 (uint32_t doc_id, const ProfileField *fieldPtr, int64_t  value);
    int  setUInt64(uint32_t doc_id, const ProfileField *fieldPtr, uint64_t value);
    int  setFloat (uint32_t doc_id, const ProfileField *fieldPtr, float      value);
    int  setDouble(uint32_t doc_id, const ProfileField *fieldPtr, double     value);
    int  setString(uint32_t doc_id, const ProfileField *fieldPtr, const char *value);
    int  setKV32  (uint32_t doc_id, const ProfileField *fieldPtr, KV32     value);

    /**
     * 设置目标doc的目标非单值(多值/变长)字段的字段值
     * @param  doc_id      目标doc的docID
     * @param  fieldPtr    目标字段的ProfileField指针
     * @param  value       目标字段的字段值的字符串形式
     * @param  delim       字符串形式字段值的分隔符
     * @return             0, OK; other,ERROR
     */
    int  setRepeatedInt8  (uint32_t doc_id, const ProfileField *fieldPtr, const char *value, char delim = ' ');
    int  setRepeatedUInt8 (uint32_t doc_id, const ProfileField *fieldPtr, const char *value, char delim = ' ');
    int  setRepeatedInt16 (uint32_t doc_id, const ProfileField *fieldPtr, const char *value, char delim = ' ');
    int  setRepeatedUInt16(uint32_t doc_id, const ProfileField *fieldPtr, const char *value, char delim = ' ');
    int  setRepeatedInt32 (uint32_t doc_id, const ProfileField *fieldPtr, const char *value, char delim = ' ');
    int  setRepeatedUInt32(uint32_t doc_id, const ProfileField *fieldPtr, const char *value, char delim = ' ');
    int  setRepeatedInt64 (uint32_t doc_id, const ProfileField *fieldPtr, const char *value, char delim = ' ');
    int  setRepeatedUInt64(uint32_t doc_id, const ProfileField *fieldPtr, const char *value, char delim = ' ');
    int  setRepeatedFloat (uint32_t doc_id, const ProfileField *fieldPtr, const char *value, char delim = ' ');
    int  setRepeatedDouble(uint32_t doc_id, const ProfileField *fieldPtr, const char *value, char delim = ' ');
    int  setRepeatedString(uint32_t doc_id, const ProfileField *fieldPtr, const char *value, char delim = ' ');
    int  setRepeatedKV32  (uint32_t doc_id, const ProfileField *fieldPtr, const char *value, char delim = ' ');

private:
    ProfileResource *_profileResc;                          // profile资源对象指针
    util::HashMap<const char*, const ProfileField*> _fieldMap;    // 记录ProfileField的映射表
};

}

#endif //KINGSO_INDEXLIB_PROFILEDOCACCESSOR_H_

