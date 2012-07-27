#ifndef INDEX_TESTBITMAPNEXT_H_
#define INDEX_TESTBITMAPNEXT_H_

#include "IndexFieldReader.h"

namespace index_lib
{
class testBitmapNext
{ 
public:
  testBitmapNext();
  ~testBitmapNext();

  int init(const char* idxPath);

  // next operator
  int bitmapNext();
  int inverteNext();

  // seek operator
  int bitmapSeek();
  int inverteSeek();

  int inverteNormalSeek(int32_t step);
  int bitmapNormalSeek(int32_t step);
  int bitmapBestSeek();

private:
  int32_t _nodeNum;
  idict_node_t* _pBitmapNode;
  idict_node_t* _pInvertNode;
  uint32_t** _ppDocList;
  
  index_lib::IndexFieldReader _cReader;
};
}
#endif //INDEX_TESTBITMAPNEXT_H_
