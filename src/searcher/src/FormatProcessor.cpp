#include "FormatProcessor.h"
        
int32_t FormatProcessor::frmt_pbp(result_container_t &container, char** output, uint32_t* outlen)
{
    ResultFormat* rf = _result_format_factory.make("pb", _mem_pool);
    char* fmt_out = 0;
    int fmt_len = 0;
    int ret = 0;
    
    if(!output || !outlen){
        TWARN("argument error, return!");
        return -1;
    }
    if(!rf){
        TWARN("make ResultFormat error, return!");
        return -1;
    }

    // 格式化输出成pb
    ret = rf->format(container, fmt_out, (uint32_t&)fmt_len);
    if(ret<0 || !fmt_out || fmt_len<=0){
        TWARN("format output error, return!");
        _result_format_factory.recycle(rf);
        return -1;
    }
    *output = (char *)malloc(fmt_len+1);
    memcpy(*output, fmt_out, fmt_len);
    *output[fmt_len] = '\0';
    _result_format_factory.recycle(rf);

    return 0;
}
int32_t FormatProcessor::frmt_xml(result_container_t &container, char** output, uint32_t* outlen)
{
    ResultFormat* rf = _result_format_factory.make("xml", _mem_pool);
    char* fmt_out = 0;
    int fmt_len = 0;
    int ret = 0;
    
    if(!output || !outlen){
        TWARN("argument error, return!");
        return -1;
    }
    if(!rf){
        TWARN("make ResultFormat error, return!");
        return -1;
    }

    // 格式化输出成xml
    ret = rf->format(container, fmt_out, (uint32_t&)fmt_len);
    if(ret<0 || !fmt_out || fmt_len<=0){
        TWARN("format output error, return!");
        _result_format_factory.recycle(rf);
        return -1;
    }

    fmt_out[fmt_len] = 0;    
    TDEBUG("utf8_format=%s", fmt_out);

    // 转码成gbk
    *output = (char *)malloc(fmt_len+1);
    if(!*output){
        TWARN("new output buffer error, return!");
        _result_format_factory.recycle(rf);
        return -1;
    }
    *outlen = py_utf8_to_gbk(fmt_out, fmt_len, *output, fmt_len+1);
    TDEBUG("gbk_outlen=%d, gbk_strlen=%d", *outlen, strlen(*output));
    TDEBUG("gbk_output=%s", *output);

    _result_format_factory.recycle(rf);

    return 0;
}

int32_t FormatProcessor::frmt_v30(result_container_t &container, char** output, uint32_t* outlen)
{
    ResultFormat* rf = _result_format_factory.make("v30", _mem_pool);
    char* fmt_out = 0;
    int fmt_len = 0;
    int ret = 0;
    
    if(!output || !outlen){
        TWARN("argument error, return!");
        return -1;
    }
    if(!rf){
        TWARN("make ResultFormat error, return!");
        return -1;
    }

    // 格式化输出成v3
    ret = rf->format(container, fmt_out, (uint32_t&)fmt_len);
    if(ret<0 || !fmt_out || fmt_len<=0){
        TWARN("format output error, return!");
        _result_format_factory.recycle(rf);
        return -1;
    }

    fmt_out[fmt_len] = 0;    
    TDEBUG("utf8_format=%s", fmt_out);

    // 转码成gbk
    *output = (char *)malloc(fmt_len+1);
    if(!*output){
        TWARN("new output buffer error, return!");
        _result_format_factory.recycle(rf);
        return -1;
    }
    *outlen = py_utf8_to_gbk(fmt_out, fmt_len, *output, fmt_len+1);
    TDEBUG("gbk_outlen=%d, gbk_strlen=%d", *outlen, strlen(*output));
    TDEBUG("gbk_output=%s", *output);

    _result_format_factory.recycle(rf);

    return 0;
}

int32_t FormatProcessor::frmt_ksp(result_container_t &container, char** output, uint32_t* outlen)
{
    return -1;
}

 

