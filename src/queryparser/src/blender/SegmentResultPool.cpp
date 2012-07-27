#include "SegmentResultPool.h"
#include "util/common.h"
#include "util/Log.h"

namespace queryparser{

    SegmentResultPool::SegmentResultPool()
        : _p_tokenizer(NULL), _count(0)
    {

    }

    SegmentResultPool::~SegmentResultPool()
    {
        for(int32_t i = 0; i < _count; ++i)
        {
            _p_tokenizer->ReleaseSegResult(_seg_results[i]);
            _seg_results[i] = NULL;
        }
        _count = 0;
        pthread_spin_destroy(&_lock);
        _p_tokenizer = NULL;
    }

    int32_t SegmentResultPool::init(ws::AliTokenizer* p_tokenizer)
    {
        if(unlikely(p_tokenizer == NULL))
        {
            TERR("QUERYPARSER: can't find tokenizer.");
            return -1;
        }
        _p_tokenizer = p_tokenizer;
        for(int32_t i = 0; i < MAX_SEGMENT_REUSLT_NUM; ++i)
        {
            _seg_results[i] = NULL;
        } 
        if(pthread_spin_init(&_lock, PTHREAD_PROCESS_PRIVATE) != 0)
        {
			TERR("QUERYPARSER: pthread_spin_lock init error.");
			return -1;
        }

        return 0;
    }

    ws::SegResult *SegmentResultPool::pop()
    {
        ws::SegResult *p_seg_result = NULL;
        pthread_spin_lock(&_lock);
        if(_count > 0)
        {
            p_seg_result = _seg_results[--_count];
        }
        pthread_spin_unlock(&_lock);
        if(p_seg_result == NULL)
        {
            p_seg_result = _p_tokenizer->CreateSegResult();
        }
        return p_seg_result; 
    }

    int32_t SegmentResultPool::push(ws::SegResult *p_seg_result)
    {
        pthread_spin_lock(&_lock);
        if(_count < MAX_SEGMENT_REUSLT_NUM)
        {
            _seg_results[_count++] = p_seg_result;
            p_seg_result = NULL;
        }
        pthread_spin_unlock(&_lock);
        if(p_seg_result)
        {
            _p_tokenizer->ReleaseSegResult(p_seg_result);
        }
        return 0;
    }
}
