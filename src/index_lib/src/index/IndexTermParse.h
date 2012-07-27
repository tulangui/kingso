/*********************************************************************
 * $Author: xiaojie.lgx $
 *
 * $LastChangedBy: xiaojie.lgx $
 *
 * $Revision: 2577 $
 *
 * $LastChangedDate: 2011-03-09 09:50:55 +0800 (Wed, 09 Mar 2011) $
 *
 * $Id: IndexTermParse.h 2577 2011-03-09 01:50:55Z xiaojie.lgx $
 *
 * $Brief: ini file parser $
 ********************************************************************/

#ifndef INDEX_INDEXTERM_PARSE_H_
#define INDEX_INDEXTERM_PARSE_H_

#include "Common.h"
#include "index_struct.h"
#include "index_lib/IndexBuilder.h"

INDEX_LIB_BEGIN


class IndexTermParse
{ 
public:
  IndexTermParse();
  virtual ~IndexTermParse();
  
  virtual int init(const char* line, int lineLen);
  uint64_t getSign() {return _sign;}
  void setSign(uint64_t sign) {_sign = sign;}

  //  < 0 失败， 0 正常结束 > 0 outsize
  virtual int32_t next(char*& outbuf, uint32_t& docId) = 0;
  virtual int32_t invalid(char*& outbuf, uint32_t& invaldDocId) = 0;

  // 及时更新使用
  virtual int32_t getPageSize(int32_t exitSize) = 0;

protected:
  const char* _begin;
  const char* _end;
  uint32_t _preDocId;   // 上一个docid
private:
  uint64_t _sign;      // 64位签名  
};


class IndexNoneOccParse : public IndexTermParse
{
public:
  IndexNoneOccParse();
  virtual ~IndexNoneOccParse();
  
  int init(const char* line, int lineLen);
  int32_t next(char*& outbuf, uint32_t& docId);
  int32_t invalid(char*& outbuf, uint32_t& invaldDocId);
  
  // 及时更新使用
  int32_t getPageSize(int32_t exitSize); 
private:
  uint32_t _node;
};

class IndexOneOccParse : public IndexTermParse
{
  public:
    IndexOneOccParse();
    virtual ~IndexOneOccParse();
  
    int init(const char* line, int lineLen);
    int32_t next(char*& outbuf, uint32_t& docId);
    int32_t invalid(char*& outbuf, uint32_t& invaldDocId);

    // 及时更新使用
    int32_t getPageSize(int32_t exitSize);
  private:
    idx2_nzip_unit_t _node; // one occ 二级索引结构
};

class IndexMultiOccParse : public IndexTermParse
{
  public:
    IndexMultiOccParse();
    virtual ~IndexMultiOccParse();
  
    int init(const char* line, int lineLen);
    int32_t next(char*& outbuf, uint32_t& docId);
    int32_t invalid(char*& outbuf, uint32_t& invaldDocId);

    // 及时更新使用
    int32_t getPageSize(int32_t exitSize);
  private:
    idx2_nzip_unit_t _node; // multi occ 二级索引结构, 同one occ结构相同
};

/////////////////////////////////////////////////
class IndexNoneOccParseNew : public IndexTermParse
{
public:
  IndexNoneOccParseNew();
  virtual ~IndexNoneOccParseNew();
  
  int init(const uint64_t termSign, const uint32_t* docList, uint32_t docNum);
  int32_t next(char*& outbuf, uint32_t& docId);
  int32_t invalid(char*& outbuf, uint32_t& invaldDocId);
  
  // 及时更新使用
  int32_t getPageSize(int32_t exitSize); 
private:
  uint32_t _node;
  const uint32_t* _begin;
  const uint32_t* _end;
};

class IndexOneOccParseNew : public IndexTermParse
{
  public:
    IndexOneOccParseNew();
    virtual ~IndexOneOccParseNew();
  
    int init(const uint64_t termSign, const DocListUnit* docList, uint32_t docNum);
    int32_t next(char*& outbuf, uint32_t& docId);
    int32_t invalid(char*& outbuf, uint32_t& invaldDocId);

    // 及时更新使用
    int32_t getPageSize(int32_t exitSize);
  private:
    idx2_nzip_unit_t _node; // one occ 二级索引结构
    const DocListUnit* _begin;    // one occ 二级索引结构
    const DocListUnit* _end;      // one occ 二级索引结构
};

class IndexMultiOccParseNew : public IndexTermParse
{
  public:
    IndexMultiOccParseNew();
    virtual ~IndexMultiOccParseNew();
  
    int init(const uint64_t termSign, const DocListUnit* docList, uint32_t docNum);
    int32_t next(char*& outbuf, uint32_t& docId);
    int32_t invalid(char*& outbuf, uint32_t& invaldDocId);

    // 及时更新使用
    int32_t getPageSize(int32_t exitSize);
  private:
    idx2_nzip_unit_t _node;    // multi occ 二级索引结构, 同one occ结构相同
    const DocListUnit* _begin;       // one occ 二级索引结构
    const DocListUnit* _end;         // one occ 二级索引结构
};

INDEX_LIB_END
 
#endif //INDEX_INDEXTERMPARSE_H_
