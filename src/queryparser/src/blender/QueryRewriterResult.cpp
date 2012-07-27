#include "queryparser/QueryRewriterResult.h"
#include <stddef.h>

namespace queryparser {

    QueryRewriterResult::QueryRewriterResult()
        : _rew_query(NULL), _rew_query_len(0)
    {
    
    }

    QueryRewriterResult::~QueryRewriterResult()
    {
        _rew_query = NULL;
        _rew_query_len = 0;
    }

    const char *QueryRewriterResult::getRewriteQuery()
    {
        return _rew_query;
    }

    int32_t QueryRewriterResult::getRewriteQueryLen()
    {
       return _rew_query_len;
    }

    void QueryRewriterResult::setRewriteQuery(char *rew_query,
                                              int32_t len)
    {
        _rew_query = rew_query;
        _rew_query_len = len;
    }
    
    void QueryRewriterResult::setRewriteQueryLen(int32_t len)
    {
        _rew_query_len = len;
    }

}
