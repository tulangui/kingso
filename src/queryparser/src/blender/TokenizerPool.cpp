#include "TokenizerPool.h"
#include "util/common.h"
#include "util/Log.h"

namespace queryparser {

TokenizerPool::TokenizerPool()
    : _count(0)
{

}

TokenizerPool::~TokenizerPool()
{
    for(int32_t i = 0; i < _count; ++i)
    {
        delete _tokenizer_stack[i];
        _tokenizer_stack[i] = NULL;
    }
    _count = 0;
    pthread_spin_destroy(&_lock);
}

int32_t TokenizerPool::init(const char *filename, const char* ws_type)
{
    if(_factory.init(filename, ws_type, 1) < 0)
    {
        TERR("QUERYPARSER: init tokenizer factory failed.");
        return -1;
    }
    
    int len = strlen(ws_type);
    if(len > MAX_WORDSPLIT_LEN-1)
    {
        TWARN("QUERYPARSER: tokenizer type overlength length:%d",len);
        len = MAX_WORDSPLIT_LEN-1;           
    }
    memcpy(_ws_type, ws_type, len);
    _ws_type[len] = '\0';
    
    for(int32_t i = 0; i < MAX_TOKENIZER_NUM; ++i)
    {
        _tokenizer_stack[i] = NULL;
    } 
    if(pthread_spin_init(&_lock, PTHREAD_PROCESS_PRIVATE) != 0)
    {
    		TERR("QUERYPARSER: pthread_spin_lock init error.");
    		return -1;
    }

    return 0;
}

ITokenizer *TokenizerPool::pop()
{
    ITokenizer * p_tokenizer = NULL;
    pthread_spin_lock(&_lock);
    if(_count > 0)
    {
        p_tokenizer = _tokenizer_stack[--_count];
    }
    pthread_spin_unlock(&_lock);
    if(p_tokenizer == NULL)
    {
        p_tokenizer = _factory.getTokenizer(_ws_type);
        if(unlikely(p_tokenizer == NULL))
        {
            TERR("QUERYPARSER: init tokenizer failed.");
        }
    }
    return p_tokenizer; 
}

int32_t TokenizerPool::push(ITokenizer * p_tokenizer)
{
    pthread_spin_lock(&_lock);
    if(_count < MAX_TOKENIZER_NUM)
    {
        _tokenizer_stack[_count++] = p_tokenizer;
        p_tokenizer = NULL;
    }
    pthread_spin_unlock(&_lock);
    if(p_tokenizer)
    {
        delete p_tokenizer;
        p_tokenizer = NULL;
    }
    return 0;
}

}

