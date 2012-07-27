#ifndef SRC_XMLREADER_H_
#define SRC_XMLREADER_H_

#include <zlib.h>
#include "util/HashMap.hpp"
#include "util/charsetfunc.h"
#include "ks_build/ks_build.h"
#include "update/update_def.h"
#include "commdef/DocMessage.pb.h"

namespace update {

class XmlReader
{ 
public:
    XmlReader(ksb_conf_t * conf);   // 不支持多线程
    ~XmlReader();

    /*
     * filename NULL, 代表只做签名计算
     */
    int init(const char* filename, bool needUtf8);
    
    /*
     * 每次读取一篇文档(完成到utf8转换,如果需要)
     */
    DocMessage* next(); 
   
    /*
     * 计算文档签名信息
     */
    int sign(DocMessage* msg, unsigned char buf[16]);

private:
    int initFieldTable(ksb_conf_t * conf, util::HashMap<char*, int>& field);

private:
    bool _need_utf8;                            // 需要转换为utf8
    ksb_conf_t * _conf;                         // 配置文件
    char* _fields_name[KSB_FIELD_MAX*3];
    util::HashMap<char*, int> _field_table;     // 有效字段表

    int _maxLen;      // 最大行长度
    int _docCount; 

    char* _line;        // 行信息
    char* _changeBuf;    // 编码转换临时buf   
    code_conv_t _conv_t;// 转换句柄

    FILE* _fp;
    gzFile _gzfp;

    DocMessage _msg;     // 原始msg
    DocMessage _msgNew;  // 编码转换后的msg
};

}

#endif //SRC_XMLREADER_H_
