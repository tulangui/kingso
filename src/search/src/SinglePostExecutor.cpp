#include "SinglePostExecutor.h"

using namespace index_lib;

namespace search {

SinglePostExecutor::SinglePostExecutor()
{

}

SinglePostExecutor::~SinglePostExecutor()
{
}

void SinglePostExecutor::print(char *buf)
{
    strcat(buf, "SINGLE:");
    strcat(buf, _token);
    strcat(buf, ";");
}


}
