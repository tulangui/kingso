#include "IndexTermParse.h" 

INDEX_LIB_BEGIN

IndexTermParse::IndexTermParse()
{
}

IndexTermParse::~IndexTermParse()
{
}
  
int IndexTermParse::init(const char* line, int lineLen)
{
  if (NULL == line || lineLen <= 0) {
    return -1;
  }

  _begin = line;
  _end = line + lineLen;

  const char* pre = _begin;
  char* cur = NULL;

	// read term sign
  _sign = strtoull(pre, &cur, 10);
  if (unlikely(pre == cur)) {
    return -1;
  }
  
  _begin = cur;
  return 0;
}

IndexNoneOccParse::IndexNoneOccParse()
{
}

IndexNoneOccParse::~IndexNoneOccParse()
{
}

int IndexNoneOccParse::init(const char* line, int lineLen)
{
  _node = INVALID_DOCID;
  _preDocId = 0;
  return IndexTermParse::init(line, lineLen);
}

int32_t IndexNoneOccParse::next(char*& outbuf, uint32_t& docId)
{
	const char* pre = _begin;
	char* cur = NULL;

  // read doc-id
  do {
    docId = strtoul(pre, &cur, 10);
    if (unlikely(pre == cur) || cur >= _end) { // 解析到行尾，退出循环
      docId = _preDocId;
      return 0;
    }
    pre = cur;

    // read occ
    strtoul(pre, &cur, 10);
    if (unlikely(pre == cur) || cur > _end) {
      return -1;
    }
    pre = cur;
  } while(_node == docId);
  
  _begin = cur;
  _node = docId;
  outbuf = (char*)&_node;

  if(docId < _preDocId) {
    return -1;
  }
  _preDocId = docId;
  return sizeof(_node);		
}

int32_t IndexNoneOccParse::invalid(char*& outbuf, uint32_t& invaldDocId)
{
  outbuf = (char*)&_node;
  _node = INVALID_DOCID;
  invaldDocId = _node;
  return sizeof(_node);
}

int32_t IndexNoneOccParse::getPageSize(int32_t exitSize)
{
  if(exitSize <= 0) {
    return sizeof(_node) * 32;
  }
  return (exitSize<<1);
}

IndexOneOccParse::IndexOneOccParse()
{
}

IndexOneOccParse::~IndexOneOccParse()
{
}

int IndexOneOccParse::init(const char* line, int lineLen)
{
  _node.doc_id = INVALID_DOCID;
  _node.occ = 0;
  _preDocId = 0;
  return IndexTermParse::init(line, lineLen);
}

int32_t IndexOneOccParse::next(char*& outbuf, uint32_t& docId)
{
	const char* pre = _begin;
	char* cur = NULL;

  // read doc-id
  do {
    docId = strtoul(pre, &cur, 10);
    if (unlikely(pre == cur) || cur >= _end) { // 解析到行尾，退出循环
      docId = _preDocId;
      return 0;
    }
    pre = cur;

    // read occ
    uint32_t occ = strtoul(pre, &cur, 10);
    if(occ >= (1<<(sizeof(_node.occ)<<3))) {
      return -1;
    }
    _node.occ = occ;
    if (unlikely(pre == cur) || cur > _end) {
      return -1;
    }
    pre = cur;
  } while(_node.doc_id == docId);
  
  _begin = cur;
  _node.doc_id = docId;
  outbuf = (char*)&_node;
  
  if(docId < _preDocId) {
    return -1;
  }
  _preDocId = docId;
  return sizeof(_node);		
}

int32_t IndexOneOccParse::invalid(char*& outbuf, uint32_t& invaldDocId)
{
  outbuf = (char*)&_node;
  _node.doc_id = INVALID_DOCID;
  _node.occ = 0;
  return sizeof(_node);
}

int32_t IndexOneOccParse::getPageSize(int32_t exitSize)
{
  if(exitSize <= 0) {
    return sizeof(_node) * 32;
  }
  return (exitSize<<1);
}

IndexMultiOccParse::IndexMultiOccParse()
{
}

IndexMultiOccParse::~IndexMultiOccParse()
{
}

int IndexMultiOccParse::init(const char* line, int lineLen)
{
  _node.doc_id = INVALID_DOCID;
  _node.occ = 0;
  _preDocId = 0;
  return IndexTermParse::init(line, lineLen);
}

int32_t IndexMultiOccParse::next(char*& outbuf, uint32_t& docId)
{
	const char* pre = _begin;
	char* cur = NULL;

  // read doc-id
  do {
    docId = strtoul(pre, &cur, 10);
    if (unlikely(pre == cur) || cur >= _end) { // 解析到行尾，退出循环
      docId = _preDocId;
      return 0;
    }
    pre = cur;

    // read occ
    uint32_t occ = strtoul(pre, &cur, 10);
    if(occ >= (1<<(sizeof(_node.occ)<<3)) ) {
      return -1;
    }
    _node.occ = occ;
    if (unlikely(pre == cur) || cur > _end) {
      return -1;
    }
    pre = cur;
  } while(0); // 一次循环即可
  
  _begin = cur;
  _node.doc_id = docId;
  outbuf = (char*)&_node;
  
  if(docId < _preDocId) {
    return -1;
  }
  _preDocId = docId;
  return sizeof(_node);		
}

int32_t IndexMultiOccParse::invalid(char*& outbuf, uint32_t& invaldDocId)
{
  outbuf = (char*)&_node;
  _node.doc_id = INVALID_DOCID;
  _node.occ = 0;
  return sizeof(_node);
}

int32_t IndexMultiOccParse::getPageSize(int32_t exitSize)
{
  if(exitSize <= 0) {
    return sizeof(_node) * 32;
  }
  return (exitSize<<1);
}

////////////////////////////////////////////
IndexNoneOccParseNew::IndexNoneOccParseNew()
{
}

IndexNoneOccParseNew::~IndexNoneOccParseNew()
{
}

int IndexNoneOccParseNew::init(const uint64_t termSign, const uint32_t* docList, uint32_t docNum)
{
  _node = INVALID_DOCID;
  _preDocId = 0;
  
  setSign(termSign);
  _begin = docList;
  _end = docList + docNum;
  return 0; 
}

int32_t IndexNoneOccParseNew::next(char*& outbuf, uint32_t& docId)
{
  if(_begin >= _end) {
    return 0;
  }

  docId = *_begin++; 
  outbuf = (char*)&_node;

  if(docId <= _preDocId) {
    if(_node != INVALID_DOCID || docId != _preDocId) {
      return -1;
    }
  }
  _node = docId;
  _preDocId = docId;
  return sizeof(_node);		
}

int32_t IndexNoneOccParseNew::invalid(char*& outbuf, uint32_t& invaldDocId)
{
  outbuf = (char*)&_node;
  _node = INVALID_DOCID;
  invaldDocId = _node;
  return sizeof(_node);
}

int32_t IndexNoneOccParseNew::getPageSize(int32_t exitSize)
{
  if(exitSize <= 0) {
    return sizeof(_node) * 32;
  }
  return (exitSize<<1);
}

IndexOneOccParseNew::IndexOneOccParseNew()
{
}

IndexOneOccParseNew::~IndexOneOccParseNew()
{
}

int IndexOneOccParseNew::init(const uint64_t termSign, const DocListUnit* docList, uint32_t docNum)
{
  _node.doc_id = INVALID_DOCID;
  _node.occ = 0;
  _preDocId = 0;

  setSign(termSign);
  _begin = docList;  // one occ 二级索引结构
  _end = docList + docNum; 
  return 0;
}

int32_t IndexOneOccParseNew::next(char*& outbuf, uint32_t& docId)
{
  if(_begin >= _end) {
    return 0;
  }

  docId = _begin->doc_id;
  _node.doc_id = docId;
  _node.occ = _begin->occ;

  do {
    _begin++;
  } while(_begin < _end && docId == _begin->doc_id);

  outbuf = (char*)&_node;
  if(docId < _preDocId) {
    return -1;
  }
  _preDocId = docId;
  return sizeof(_node);		
}

int32_t IndexOneOccParseNew::invalid(char*& outbuf, uint32_t& invaldDocId)
{
  outbuf = (char*)&_node;
  _node.doc_id = INVALID_DOCID;
  _node.occ = 0;
  return sizeof(_node);
}

int32_t IndexOneOccParseNew::getPageSize(int32_t exitSize)
{
  if(exitSize <= 0) {
    return sizeof(_node) * 32;
  }
  return (exitSize<<1);
}

IndexMultiOccParseNew::IndexMultiOccParseNew()
{
}

IndexMultiOccParseNew::~IndexMultiOccParseNew()
{
}

int IndexMultiOccParseNew::init(const uint64_t termSign, const DocListUnit* docList, uint32_t docNum)
{
  _node.doc_id = INVALID_DOCID;
  _node.occ = 0;
  _preDocId = 0;
  setSign(termSign);
  
  _begin = docList;  // one occ 二级索引结构
  _end = docList + docNum; 
  return 0;
}

int32_t IndexMultiOccParseNew::next(char*& outbuf, uint32_t& docId)
{
  if(_begin >= _end) {
    return 0;
  }

  // read doc-id
  docId = _begin->doc_id;
  _node.occ = _begin->occ;
  
  _begin++;
  _node.doc_id = docId;
  outbuf = (char*)&_node;
  
  if(docId < _preDocId) {
    return -1;
  }
  _preDocId = docId;
  return sizeof(_node);		
}

int32_t IndexMultiOccParseNew::invalid(char*& outbuf, uint32_t& invaldDocId)
{
  outbuf = (char*)&_node;
  _node.doc_id = INVALID_DOCID;
  _node.occ = 0;
  return sizeof(_node);
}

int32_t IndexMultiOccParseNew::getPageSize(int32_t exitSize)
{
  if(exitSize <= 0) {
    return sizeof(_node) * 32;
  }
  return (exitSize<<1);
}
INDEX_LIB_END
 
