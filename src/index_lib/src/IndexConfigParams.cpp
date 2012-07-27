#include "index_lib/IndexConfigParams.h"

namespace index_lib
{

IndexConfigParams IndexConfigParams::_obj;

IndexConfigParams::IndexConfigParams()
{
    _flag = false;
}

IndexConfigParams::~IndexConfigParams()
{
}

IndexConfigParams *
IndexConfigParams::getInstance()
{
    return &_obj;
}

void
IndexConfigParams::setMemLock()
{
    _flag = true;
}

bool
IndexConfigParams::isMemLock()
{
    return _flag;
}

}
