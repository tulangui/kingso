#include "index_lib/ProfileMultiValueIterator.h"
#include <stdlib.h>

namespace index_lib
{

ProfileMultiValueIterator::ProfileMultiValueIterator()
{
    _num        = 0;
    _pos        = -1;
    _targetAddr = NULL;
    _compress   = false;
}

ProfileMultiValueIterator::~ProfileMultiValueIterator()
{
}

}
