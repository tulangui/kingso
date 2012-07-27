#include "OccSinglePostExecutor.h"

using namespace index_lib;

namespace search {

OccSinglePostExecutor::OccSinglePostExecutor()
{

}

OccSinglePostExecutor::~OccSinglePostExecutor()
{
}

void OccSinglePostExecutor::print(char *buf)
{
    strcat(buf, "OccSINGLEPost:");
    strcat(buf, _token);
    strcat(buf, ";");
}


}
