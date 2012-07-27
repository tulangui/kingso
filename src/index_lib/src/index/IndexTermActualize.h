/*
 * IndexTerm.h
 *
 *  Created on: 2011-3-4
 *      Author: yewang@taobao.com clm971910@gmail.com
 *
 *      Desc  : 这是一个基类。  根据索引的类型和压缩类型 ， 生成各种子类
 *              数据成员： 字段名  字段签名 对应的倒排链的指针  bitmap的指针
 *                    索引包含的doc数， 检索过程中的临时位置
 */

#ifndef _INDEXTERMACTUALIZE_H_
#define _INDEXTERMACTUALIZE_H_


#include "index_struct.h"
#include "index_lib/IndexLib.h"
#include "index_lib/IndexTerm.h"
#include "util/MemPool.h"
#include "IndexCompression.h"

namespace index_lib
{


/**
 * 非压缩 one occ
 */
class IndexUnZipOneOccTerm : public IndexTerm
{
public:
    IndexUnZipOneOccTerm();
    virtual ~IndexUnZipOneOccTerm();

    inline uint32_t seek(uint32_t docId);

    inline uint8_t* const getOcc(int32_t &count);

    inline uint32_t next();
    inline uint32_t next(int32_t step);

    inline uint32_t setpos(uint32_t docId);

    int init(uint32_t docNum, uint32_t maxDocNum, uint32_t maxOccNum, uint32_t not_orig_flag);


private:
    idx2_nzip_unit_t* _first;
    idx2_nzip_unit_t* _begin;
    idx2_nzip_unit_t* _end;
    idx2_nzip_unit_t* _beginNext;

    bool _nextFirstFlag;  // next 操作时第一次标志
};

/**
 * 非压缩 one occ + skip read
 */
class IndexUnZipOneOccTermOpt : public IndexTerm
{
public:
    IndexUnZipOneOccTermOpt();
    virtual ~IndexUnZipOneOccTermOpt();

    inline uint32_t seek(uint32_t docId);

    inline uint8_t* const getOcc(int32_t &count);

    inline uint32_t next();
    inline uint32_t next(int32_t step);

    inline uint32_t setpos(uint32_t docId);

    int init(uint32_t docNum, uint32_t maxDocNum,
            uint32_t maxOccNum, uint32_t not_orig_flag);

private:
    idx2_nzip_unit_t* _first;
    idx2_nzip_unit_t* _begin;
    idx2_nzip_unit_t* _end;
    idx2_nzip_unit_t* _beginNext;

    bool _nextFirstFlag;  // next 操作时第一次标志
};

/**
 * 非压缩  有多个occ
 */
class IndexUnZipMulOccTerm : public IndexUnZipOneOccTerm
{
public:
    IndexUnZipMulOccTerm();
    virtual ~IndexUnZipMulOccTerm();

    inline uint32_t seek(uint32_t docId);
    inline uint8_t* const getOcc(int32_t &count);

    inline uint32_t next();
    inline uint32_t next(int32_t step);

    inline uint32_t setpos(uint32_t docId);

    int init(uint32_t docNum, uint32_t maxDocNum,
            uint32_t maxOccNum, MemPool* pMemPool, uint32_t not_orig_flag);

private:
    idx2_nzip_unit_t* _first;
    idx2_nzip_unit_t* _begin;
    idx2_nzip_unit_t* _end;
    idx2_nzip_unit_t* _beginNext;

    uint8_t*          _pOcc;       // occ buffer
    uint32_t          _lastDocId;  // next 操作使用
};


/**
 * 非压缩  无occ
 */
class IndexUnZipNullOccTerm : public IndexTerm
{
public:
    IndexUnZipNullOccTerm();
    virtual ~IndexUnZipNullOccTerm();

    inline uint32_t seek(uint32_t docId);

    inline uint32_t next();
    inline uint32_t next(int32_t step);

    inline uint32_t setpos(uint32_t docId);

    int init(uint32_t docNum, uint32_t maxDocNum, uint32_t not_orig_flag);
private:
    uint32_t* _begin;   // docList begin
    uint32_t* _end;     // docList end

    uint32_t* _beginNext;
};

/**
 * 非压缩  无occ + skip read
 */
class IndexUnZipNullOccTermOpt : public IndexTerm
{
public:
    IndexUnZipNullOccTermOpt();
    virtual ~IndexUnZipNullOccTermOpt();

    inline uint32_t seek(uint32_t docId);

    inline uint32_t next();
    inline uint32_t next(int32_t step);

    inline uint32_t setpos(uint32_t docId);

    int init(uint32_t docNum, uint32_t maxDocNum, uint32_t not_orig_flag);
private:
    uint32_t* _begin;   // docList begin
    uint32_t* _end;     // docList end

    uint32_t* _beginNext;
};

/**
 * 压缩  one occ
 */
class IndexZipOneOccTerm : public IndexTerm
{
public:
    IndexZipOneOccTerm();
    virtual ~IndexZipOneOccTerm();

    inline uint32_t seek(uint32_t docId);

    inline uint8_t* const getOcc(int32_t &count);

    inline uint32_t next();
    inline uint32_t next(int32_t step);

    inline uint32_t setpos(uint32_t docId);

    int init(uint32_t docNum, uint32_t maxDocNum, uint32_t maxOccNum, uint32_t not_orig_flag);

private:
    idx2_zip_header_t* _head;         //

    idx2_zip_list_unit_t* _first;      // docList head
    idx2_zip_list_unit_t* _begin;     // docList begin
    idx2_zip_list_unit_t* _end;       // docList end
    idx2_zip_list_unit_t* _beginNext;

    idx2_zip_skip_unit_t* _pSkipBegin;// skipList begin
    idx2_zip_skip_unit_t* _pSkipEnd;  // skipList end

    bool _nextFirstFlag;              // next 操作时第一次标志
};


/**
 * 压缩  one occ
 */
class IndexP4DOneOccTerm : public IndexTerm
{
public:
    IndexP4DOneOccTerm();
    virtual ~IndexP4DOneOccTerm();

    inline uint32_t seek(uint32_t docId);

    inline uint8_t* const getOcc(int32_t &count);

    inline uint32_t next();

    int init(uint32_t docNum, uint32_t maxDocNum, uint32_t maxOccNum, uint32_t not_orig_flag);

private:
    idx2_zip_header_t* _head;         //

    uint8_t* _first;     // docList head
    uint32_t* _begin;     // docList begin
    uint32_t* _end;       // docList end

    idx2_p4d_skip_unit_t* _pSkipBegin;// skipList begin
    idx2_p4d_skip_unit_t* _pSkipEnd;  // skipList end

    IndexCompression _idxComp;
    uint32_t _curId;
    uint8_t *_occBegin;
    uint32_t _occPos;
    uint8_t _curOcc;

};


/**
 * 压缩  多occ
 */
class IndexZipMulOccTerm : public IndexTerm
{
public:
    IndexZipMulOccTerm();
    virtual ~IndexZipMulOccTerm();

    inline uint32_t seek(uint32_t docId);

    inline uint8_t* const getOcc(int32_t &count);

    inline uint32_t next();
    inline uint32_t next(int32_t step);

    inline uint32_t setpos(uint32_t docId);

    int init(uint32_t docNum, uint32_t maxDocNum, uint32_t maxOccNum, MemPool* pMemPool, uint32_t not_orig_flag);

private:
    idx2_zip_header_t* _head;         //

    idx2_zip_list_unit_t* _first;      // docList head
    idx2_zip_list_unit_t* _begin;     // docList begin
    idx2_zip_list_unit_t* _end;       // docList end
    idx2_zip_list_unit_t* _beginNext;

    idx2_zip_skip_unit_t* _pSkipBegin;// skipList begin
    idx2_zip_skip_unit_t* _pSkipEnd;  // skipList end

    uint8_t*              _pOcc;           // occ buffer
    uint32_t              _lastDocIdDiff;  // next 操作使用
};

class IndexP4DMulOccTerm : public IndexTerm
{
public:
    IndexP4DMulOccTerm();
    virtual ~IndexP4DMulOccTerm();

    inline uint32_t seek(uint32_t docId);

    inline uint8_t* const getOcc(int32_t &count);

    inline uint32_t next();

    int init(uint32_t docNum, uint32_t maxDocNum, uint32_t maxOccNum, uint32_t not_orig_flag);

private:
    idx2_zip_header_t* _head;         //

    uint8_t* _first;     // docList head
    uint32_t* _begin;     // docList begin
    uint32_t* _end;       // docList end

    idx2_p4d_skip_unit_t* _pSkipBegin;// skipList begin
    idx2_p4d_skip_unit_t* _pSkipEnd;  // skipList end

    IndexCompression _idxComp;
    uint32_t _curId;
    uint8_t *_occBegin;
    uint32_t _occPos;
    uint8_t *_curOcc;
    int32_t _occCount;

};

/**
 * 压缩  无occ
 */
class IndexZipNullOccTerm : public IndexTerm
{
public:
    IndexZipNullOccTerm();
    virtual ~IndexZipNullOccTerm();

    inline uint32_t seek(uint32_t docId);

    inline uint32_t next();
    inline uint32_t next(int32_t step);

    inline uint32_t setpos(uint32_t docId);

    int init(uint32_t docNum, uint32_t maxDocNum, uint32_t not_orig_flag);

private:
    idx2_zip_header_t* _head;         //

    uint16_t* _first;     // docList head
    uint16_t* _begin;     // docList begin
    uint16_t* _end;       // docList end
    uint16_t* _beginNext;

    idx2_zip_skip_unit_t* _pSkipBegin;// skipList begin
    idx2_zip_skip_unit_t* _pSkipEnd;  // skipList end
};

/**
 * 压缩  无occ
 */
class IndexP4DNullOccTerm : public IndexTerm
{
public:
    IndexP4DNullOccTerm();
    virtual ~IndexP4DNullOccTerm();

    inline uint32_t seek(uint32_t docId);

    inline uint32_t next();

    int init(uint32_t docNum, uint32_t maxDocNum, uint32_t not_orig_flag);

private:
    idx2_zip_header_t* _head;         //

    uint8_t* _first;     // docList head
    uint32_t* _begin;     // docList begin
    uint32_t* _end;       // docList end

    idx2_p4d_skip_unit_t* _pSkipBegin;// skipList begin
    idx2_p4d_skip_unit_t* _pSkipEnd;  // skipList end

    IndexCompression _idxComp;
    uint32_t _curId;
};


/*
 * bitmap
 */
class IndexBitmapTerm : public IndexTerm
{
public:
    IndexBitmapTerm();
    virtual ~IndexBitmapTerm();

    inline uint32_t seek(uint32_t docId);

    inline uint32_t next();
    inline uint32_t next(int32_t step);

    inline uint32_t setpos(uint32_t docId);

    int init(uint32_t docNum, uint32_t maxDocNum, uint32_t maxDocId, uint32_t not_orig_flag);

private:
    uint32_t _maxDocId;
    uint32_t _lastDocId;
    uint8_t* _begin;
    uint8_t* _end;
};

/**
 * 非压缩    无occ 内部排重
 */
//class IndexUnZipNullOccEncodeTerm : public IndexTerm
//{
//public:
//    IndexUnZipNullOccEncodeTerm();
//    virtual ~IndexUnZipNullOccEncodeTerm();
//
//    inline uint32_t seek(uint32_t docId);
//    inline uint32_t seek(uint32_t docId, uint32_t &firstOcc);
//    inline uint32_t seek(uint32_t docId, uint8_t **occList, uint32_t &occNum);
//
//    inline uint32_t next();
//    inline uint32_t next(uint32_t &firstOcc);
//    inline uint32_t next(uint8_t **occList, uint32_t &occNum);
//
//    inline uint32_t expand();
//    inline uint32_t expand(uint32_t docId);
//};
//
//
///**
// * 压缩    无occ 内部排重
// */
//class IndexZipNullOccEncodeTerm : public IndexTerm
//{
//public:
//    IndexZipNullOccEncodeTerm();
//    virtual ~IndexZipNullOccEncodeTerm();
//
//    inline uint32_t seek(uint32_t docId);
//    inline uint32_t seek(uint32_t docId, uint32_t &firstOcc);
//    inline uint32_t seek(uint32_t docId, uint8_t **occList, uint32_t &occNum);
//
//    inline uint32_t next();
//    inline uint32_t next(uint32_t &firstOcc);
//    inline uint32_t next(uint8_t **occList, uint32_t &occNum);
//
//    inline uint32_t expand();
//    inline uint32_t expand(uint32_t docId);
//};

}



#endif /* INDEXTERM_H_ */
