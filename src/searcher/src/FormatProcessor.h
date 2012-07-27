
#ifndef _FRAMEWORK_APP_SEARCHER_FORMAT_PROCESSOR_H_
#define _FRAMEWORK_APP_SEARCHER_FORMAT_PROCESSOR_H_

#include "ResultFormat.h"
#include "util/py_code.h"

//format type for service http 
enum outfmt_type
{
    KS_OFMT_PBP,
    KS_OFMT_XML,
    KS_OFMT_V30,
    KS_OFMT_KSP,
};

class FormatProcessor {
    public:
        FormatProcessor(){}

        bool init(MemPool *mem_pool)
        {
            _mem_pool = mem_pool;
        }
    public:
        inline int32_t frmt(outfmt_type type, result_container_t &container, char** output, uint32_t* outlen)
        {
            switch(type){
                case KS_OFMT_PBP:
                    return frmt_pbp(container, output, outlen);
                    break;
                case KS_OFMT_XML:
                    return frmt_xml(container, output, outlen);
                    break;
                case KS_OFMT_V30:
                    return frmt_v30(container, output, outlen);
                    break;
                case KS_OFMT_KSP:
                    return frmt_ksp(container, output, outlen);
                    break;
                default:
                    TWARN("FORMATRESULT: format type undefined!");    
            }
            return -1;
        }
    private:
        int32_t frmt_xml(result_container_t &container, char** output, uint32_t* outlen);
        int32_t frmt_pbp(result_container_t &container, char** output, uint32_t* outlen);
        int32_t frmt_v30(result_container_t &container, char** output, uint32_t* outlen);
        int32_t frmt_ksp(result_container_t &container, char** output, uint32_t* outlen);
    private:
        MemPool *_mem_pool;
        ResultFormatFactory _result_format_factory; 
};

#endif


