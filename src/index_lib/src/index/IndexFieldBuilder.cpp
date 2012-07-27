#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "Common.h"
#include "idx_dict.h"
#include "index_struct.h"
#include "IndexFieldBuilder.h"

INDEX_LIB_BEGIN

#define IDX_GET_BASE(doc_id)  ((doc_id) >> 16)
#define IDX_GET_DIFF(doc_id)  ((doc_id) &  0x0000FFFF)
#define COMMON_FILE_MODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)

IndexFieldBuilder::IndexFieldBuilder(const char * fieldName, int maxOccNum, int maxDocNum,
        const char * encodeFile) : _indexTermFactory(maxOccNum)
{
    _maxOccNum = maxOccNum;
    _maxDocNum = maxDocNum;
    // 长度限制由调用接口来把关
    strcpy(_fieldName, fieldName);
    if (NULL == encodeFile) {
        _encodeFile[0] = 0x0;
    } else {
        strcpy(_encodeFile, encodeFile);
    }
    _pIndexBuilder = NULL;
    _travelPos = -1;                // 遍历一级索引的当前位置
    _pMemPool = NULL;
    _fpInvertList = NULL;           // 倒排索引文件句柄
    _fpBitMapList = NULL;           // bitmap索引文件句柄
    _inverted_file_num = MAX_IDX2_FD;

    if(_maxOccNum > 1) {
        _pLineParse = new (std::nothrow) IndexMultiOccParse;
    } else if(_maxOccNum == 1) {
        _pLineParse = new (std::nothrow) IndexOneOccParse;
    } else {
        _pLineParse = new (std::nothrow) IndexNoneOccParse;
    }
    _pParseInstance = _pLineParse;

    _cIndexTermInfo.maxOccNum     = maxOccNum;             // field本身限制的最多支持的occ数量
    _cIndexTermInfo.maxDocNum     = maxDocNum;
    _cIndexTermInfo.delRepFlag    = 0;                     // 是否排重信息，0: 不排重， 1： 排重
    _cIndexTermInfo.bitmapLen     = (maxDocNum+63) / 64;   // bitmap索引长度(如果存在), uint64 数组长度
    _cIndexTermInfo.not_orig_flag = 0;

    _rebuildFlag = false;
}


IndexFieldBuilder::~IndexFieldBuilder()
{
    if (_pIndexBuilder) {
        delete _pIndexBuilder;
        _pIndexBuilder = NULL;
    }
    if (_pParseInstance) {
        delete _pParseInstance;
        _pParseInstance = NULL;
    }
    if(_pMemPool) {
        delete _pMemPool;
        _pMemPool = NULL;
    }
}

/*
 * func : fill a index_builder_t struct
 *
 * args : path, DIR of index files
 *        max_doc_id, max doc_id
 *        hashsize, buckets num of hash table
 *        nodesize, initial capacity of hash table
 *
 * ret  : false, error
 *      : true, success
 */
int IndexFieldBuilder::open(const char * path)
{
    if(NULL == _pLineParse) {
        return KS_ENOMEM;
    }

    const unsigned int max_doc_id = _maxDocNum;
    const int hashsize = (1<<20) + 1;
    const int nodesize = (1<<19);
    if ( !path || hashsize <= 0 || nodesize <= 0 ) {
        TERR("parameter error: path %s, hashsize %d, nodesize %d",
                path, hashsize, nodesize);
        return -1;
    }
    if(strlen(path) >= PATH_MAX) {
        TERR("path over len, %s", path);
        return -2;
    }
    strcpy(_path, path);
    _pIndexBuilder = new (std::nothrow) index_builder_t;
    if (NULL == _pIndexBuilder) {
        TERR("alloc index_builder_t error");
        return -3;
    }

    char filename[PATH_MAX];

    index_builder_t* pbuilder = (index_builder_t*)_pIndexBuilder;
    // save path and field_name
    snprintf(pbuilder->path, sizeof(pbuilder->path), "%s", path);
    snprintf(pbuilder->field_name, sizeof(pbuilder->field_name), "%s", _fieldName);

    // init index hash dict
    pbuilder->bitmap_fd   = -1;
    for (unsigned int i=0; i<MAX_IDX2_FD; i++) {
        pbuilder->inverted_fd[i]   = -1;
    }

    pbuilder->inverted_dict = idx_dict_create(hashsize, nodesize);
    if (!pbuilder->inverted_dict) {
        TERR("create hash dict (%d, %d) for inverted index failed. field_name %s",
                hashsize, nodesize, pbuilder->field_name);
        return -4;
    }

    pbuilder->bitmap_dict = idx_dict_create(1021, 512);
    if (!pbuilder->bitmap_dict) {
        TERR("create hash dict for bitmap index failed. field_name %s",
                pbuilder->field_name);
        return -5;
    }

    // open index2 fd
    snprintf(filename, sizeof(filename), "%s/%s.%s.0",
            path, pbuilder->field_name, INVERTED_IDX2_INFIX);
    pbuilder->inverted_fd[0] =
        ::open(filename, O_CREAT|O_WRONLY|O_TRUNC|O_APPEND, COMMON_FILE_MODE);
    if (pbuilder->inverted_fd[0] < 0) {
        TERR("open file %s failed. errno %d", filename, errno);
        return -6;
    }
    pbuilder->inverted_fd_num = 1;
    pbuilder->cur_size        = 0;

    snprintf(filename, sizeof(filename), "%s/%s.%s",
            path, pbuilder->field_name, BITMAP_IDX2_INFIX);
    pbuilder->bitmap_fd =
        ::open(filename, O_CREAT|O_WRONLY|O_TRUNC|O_APPEND, COMMON_FILE_MODE);
    if (pbuilder->bitmap_fd < 0) {
        TERR("open file %s failed. errno %d", filename, errno);
        return -7;
    }
    pbuilder->bitmap_size = (max_doc_id / 64 + (max_doc_id % 64 > 0)) * 8;

    // alloc buffers
    pbuilder->raw_buf_size = (max_doc_id + 1) * sizeof(idx2_nzip_unit_t);
    pbuilder->raw_buf      = (char*)malloc(pbuilder->raw_buf_size);
    if (!pbuilder->raw_buf) {
        TERR("alloc raw buf with size %u failed. field_name %s",
                pbuilder->raw_buf_size, pbuilder->field_name);
        return -8;
    }

//    pbuilder->final_buf_size = ((max_doc_id>>16) + 2) * sizeof(idx2_zip_skip_unit_t)
//        + (max_doc_id + 1) * sizeof(idx2_zip_list_unit_t) + sizeof(idx2_zip_header_t);
    pbuilder->final_buf_size = ((max_doc_id + P4D_BLOCK_SIZE) / P4D_BLOCK_SIZE + 1) * sizeof(_idx2_p4d_skip_unit_t);
    pbuilder->final_buf_size += (max_doc_id + 1) * sizeof(int32_t) + sizeof(idx2_zip_header_t);  
    if(_maxOccNum > 0) {
        pbuilder->final_buf_size += (max_doc_id + 1) * sizeof(int8_t);
    }
    if (pbuilder->final_buf_size < pbuilder->bitmap_size) {
        pbuilder->final_buf_size = pbuilder->bitmap_size;
    }
    pbuilder->final_buf = (char*)malloc(pbuilder->final_buf_size);
    if (!pbuilder->final_buf) {
        TERR("alloc final buf with size %u failed",
                pbuilder->final_buf_size);
        return -9;
    }
    return 0;
}

int IndexFieldBuilder::reopen(const char * path)
{
    if(NULL == _pLineParse) {
        return KS_ENOMEM;
    }

    _rebuildFlag = true;

    if(strlen(path) >= PATH_MAX) {
        TERR("path over len, %s", path);
        return -2;
    }
    strcpy(_path, path);
    _pIndexBuilder = new (std::nothrow) index_builder_t;
    if (NULL == _pIndexBuilder) {
        TERR("alloc index_builder_t error");
        return -3;
    }

    char filename[PATH_MAX];

    index_builder_t* pbuilder = (index_builder_t*)_pIndexBuilder;
    // save path and field_name
    snprintf(pbuilder->path, sizeof(pbuilder->path), "%s", path);
    snprintf(pbuilder->field_name, sizeof(pbuilder->field_name), "%s", _fieldName);

    // init index hash dict
    pbuilder->bitmap_fd   = -1;
    for (unsigned int i=0; i<MAX_IDX2_FD; i++) {
        pbuilder->inverted_fd[i]   = -1;
    }

    // 加载上次的一级索引文件
    snprintf(filename, sizeof(filename), "%s.%s",
            pbuilder->field_name, INVERTED_IDX1_INFIX);
    pbuilder->inverted_dict = idx_dict_load(pbuilder->path, filename);
    if (!pbuilder->inverted_dict) {
        TERR("load idx_dict failed. field_name %s", filename);
        return -4;
    }

    // 加载上次的bitmap一级索引
    snprintf(filename, sizeof(filename), "%s.%s",
            pbuilder->field_name, BITMAP_IDX1_INFIX);
    pbuilder->bitmap_dict = idx_dict_load(pbuilder->path, filename);
    if (!pbuilder->bitmap_dict) {
        TERR("load bitmap idx_dict failed. field_name %s", filename);
        return -5;
    }

    // open index2 fd
    struct stat st;
    pbuilder->inverted_fd_num = 0;
    for(uint32_t i = 0; i < MAX_IDX2_FD; i++) {
        snprintf(filename, sizeof(filename), "%s/%s.%s.%u",
                path, pbuilder->field_name, INVERTED_IDX2_INFIX, i);
        if(-1 == stat(filename, &st) || 0 == st.st_size) {
            break;
        }
        pbuilder->inverted_fd_num++;
    }
    if(pbuilder->inverted_fd_num > 0) {
        pbuilder->inverted_fd_num--;
    }
    if (pbuilder->inverted_fd_num >= MAX_IDX2_FD) {
        TLOG("num of inverted idx2 files is %d now!", MAX_IDX2_FD);
        return -1;
    }

    // 打开最后一个文件
    snprintf(filename, sizeof(filename), "%s/%s.%s.%u",
            path, pbuilder->field_name, INVERTED_IDX2_INFIX, pbuilder->inverted_fd_num);
    if(-1 == stat(filename, &st) || 0 == st.st_size) { // 不存在，写打开
        pbuilder->inverted_fd[pbuilder->inverted_fd_num] =
            ::open(filename, O_CREAT|O_WRONLY|O_TRUNC|O_APPEND, COMMON_FILE_MODE);
        pbuilder->cur_size = 0;
    } else { // 以追加方式打开
        pbuilder->inverted_fd[pbuilder->inverted_fd_num] =
            ::open(filename, O_WRONLY|O_APPEND, COMMON_FILE_MODE);
        pbuilder->cur_size = st.st_size;
    }
    if (pbuilder->inverted_fd[pbuilder->inverted_fd_num] < 0) {
        TERR("open file %s failed. errno %d", filename, errno);
        return -6;
    }
    pbuilder->inverted_fd_num++;

    snprintf(filename, sizeof(filename), "%s/%s.%s",
            path, pbuilder->field_name, BITMAP_IDX2_INFIX);
    pbuilder->bitmap_fd =
        ::open(filename, O_WRONLY|O_APPEND, COMMON_FILE_MODE);
    if (pbuilder->bitmap_fd < 0) {
        TERR("open file %s failed. errno %d", filename, errno);
        return -7;
    }

    const unsigned int max_doc_id = _maxDocNum;
    pbuilder->bitmap_size = (max_doc_id / 64 + (max_doc_id % 64 > 0)) * 8;

    // alloc buffers
    pbuilder->raw_buf_size = (max_doc_id + 1) * sizeof(idx2_nzip_unit_t);
    pbuilder->raw_buf      = (char*)malloc(pbuilder->raw_buf_size);
    if (!pbuilder->raw_buf) {
        TERR("alloc raw buf with size %u failed. field_name %s",
                pbuilder->raw_buf_size, pbuilder->field_name);
        return -8;
    }

//    pbuilder->final_buf_size = ((max_doc_id>>16) + 2) * sizeof(idx2_zip_skip_unit_t)
//        + (max_doc_id + 1) * sizeof(idx2_zip_list_unit_t) + sizeof(idx2_zip_header_t);
    pbuilder->final_buf_size = ((max_doc_id + P4D_BLOCK_SIZE) / P4D_BLOCK_SIZE + 1) * sizeof(_idx2_p4d_skip_unit_t);
    pbuilder->final_buf_size += (max_doc_id + 1) * sizeof(int32_t) + sizeof(idx2_zip_header_t);  
    if(_maxOccNum > 0) {
        pbuilder->final_buf_size += (max_doc_id + 1) * sizeof(int8_t);
    }
    if (pbuilder->final_buf_size < pbuilder->bitmap_size) {
        pbuilder->final_buf_size = pbuilder->bitmap_size;
    }
    pbuilder->final_buf = (char*)malloc(pbuilder->final_buf_size);
    if (!pbuilder->final_buf) {
        TERR("alloc final buf with size %u failed",
                pbuilder->final_buf_size);
        return -9;
    }
    return 0;
}

/*
 * func : close and free an index_builder_t struct
 *
 * args : pbuilder, pointer of an index_builder_t struct
 *
 */
int IndexFieldBuilder::close()
{
    index_builder_t* pbuilder = (index_builder_t*)_pIndexBuilder;

    if (pbuilder->inverted_dict) {
        idx_dict_free(pbuilder->inverted_dict);
    }

    if (pbuilder->bitmap_dict) {
        idx_dict_free(pbuilder->bitmap_dict);
    }

    for (unsigned int i=0; i < MAX_IDX2_FD; i++) {
        if (pbuilder->inverted_fd[i] >= 0) {
            ::close(pbuilder->inverted_fd[i]);
        }
    }

    if (pbuilder->bitmap_fd >= 0) {
        ::close(pbuilder->bitmap_fd);
    }

    if (pbuilder->raw_buf) {
        free(pbuilder->raw_buf);
    }

    if (pbuilder->final_buf) {
        free(pbuilder->final_buf);
    }
    return 0;
}

/*
 * func : convert a text index line to binary index and write to files
 *
 * args : line, a line include a term's text index
 * args : lineLen, text index line len
 *
 * ret  : <=0, error
 *      : >0, success. the ret is the doc_id count of the term
 */
int IndexFieldBuilder::addTerm(const char * line, int lineLen)
{
    if(_pLineParse->init(line, lineLen) < 0) {
        return KS_EFAILED;
    }
    _pLineParse = _pParseInstance;
    return addTerm();
}
int IndexFieldBuilder::addTerm(const uint64_t termSign, const uint32_t* docList, uint32_t docNum)
{
    if(unlikely(_maxOccNum > 0)) {
        return KS_EFAILED;
    }
    IndexNoneOccParseNew parse;
    if(parse.init(termSign, docList, docNum) < 0) {
        return KS_EFAILED;
    }
    _pLineParse = &parse;
    return addTerm();
}
int IndexFieldBuilder::addTerm(const uint64_t termSign, const DocListUnit* docList, uint32_t docNum)
{
    if(unlikely(_maxOccNum < 0)) {
        return KS_EFAILED;
    }
    if(_maxOccNum == 1) {
        IndexOneOccParseNew parse;
        if(parse.init(termSign, docList, docNum) < 0) {
            return KS_EFAILED;
        }
        _pLineParse = &parse;
        return addTerm();
    }

    // 多occ情况下可能内存不足
    if((docNum + 1) * sizeof(idx2_nzip_unit_t) > _pIndexBuilder->raw_buf_size) {
        _pIndexBuilder->raw_buf_size = (docNum + 1) * sizeof(idx2_nzip_unit_t);
        free(_pIndexBuilder->raw_buf);
        _pIndexBuilder->raw_buf = (char*)malloc(_pIndexBuilder->raw_buf_size);

        int size = ((docNum + P4D_BLOCK_SIZE) / P4D_BLOCK_SIZE + 1) * sizeof(_idx2_p4d_skip_unit_t);
        size += (docNum + 1) * sizeof(int32_t) + sizeof(idx2_zip_header_t);
        if(size > _pIndexBuilder->final_buf_size) {
            free(_pIndexBuilder->final_buf);
            _pIndexBuilder->final_buf_size = size;
            _pIndexBuilder->final_buf = (char*)malloc(_pIndexBuilder->final_buf_size);
        }
    }

    IndexMultiOccParseNew parse;
    if(parse.init(termSign, docList, docNum) < 0) {
        return KS_EFAILED;
    }
    _pLineParse = &parse;
    return addTerm();
}

int IndexFieldBuilder::addTerm()
{
    index_builder_t * pbuilder = _pIndexBuilder;
    if ((NULL == pbuilder))	{
        TERR("parameter error: pbuilder %p", pbuilder);
        return -1;
    }

    int32_t             ret         = 0;
    unsigned int        base_num    = 0;
    unsigned int        doc_count   = 0;
    IDX_DISK_TYPE       disk_type   = TS_IDT_NOT_ZIP;
    idict_node_t        node        = {0L, 0, 0, 0, 0};
    full_idx1_unit_t  * pind1       = (full_idx1_unit_t*)(&node);

    pind1->term_sign  = _pLineParse->getSign();

    // convert text inverted index to binary inverted index(not-zipped format)
    uint32_t useLen = 0;
    ret = buildNZipIndex(&base_num, pbuilder->raw_buf, pbuilder->raw_buf_size, useLen);
    if (unlikely(ret<=0)) {
        TERR("build uncompressed index failed. ret %d, line sign=%lu",	ret, pind1->term_sign);
        return -1;
    }
    doc_count = ret;

    // determine which format will be store into index2 file
    disk_type = set_idt(doc_count, base_num, pbuilder->bitmap_size);

    // build and write zip/bitmap index2 according to disk_type
    switch(disk_type) {
        case TS_IDT_NOT_ZIP:
        {
            ret = abandonOverflow(pbuilder->raw_buf, doc_count, base_num, useLen);
            if(ret < 0) {
                TERR("abandon overflow(unzip) error, sign=%lu", node.sign);
                return -1;
            } else if(ret > 0) {
                TERR("idx2 docnum over bit level(unzip), lastNum=%u, line sign=%lu",
                        doc_count, pind1->term_sign);
            }
            // add index1 node
            pind1->file_num      = pbuilder->inverted_fd_num - 1;
            pind1->pos           = pbuilder->cur_size;
            pind1->zip_flag      = 0;
            pind1->doc_count     = doc_count;
            pind1->len           = useLen;
            pind1->not_orig_flag = ( false == _rebuildFlag ) ? 0 : 1;

            ret = idx_dict_add(pbuilder->inverted_dict, &node);
            if (unlikely(ret<0)) {
                TERR("add token %lu to idx1 dict failed. ret %d",
                        node.sign, ret);
                return -1;
            }

            // write index2
            ret = write(pbuilder->inverted_fd[pind1->file_num],
                    pbuilder->raw_buf, pind1->len);
            if (unlikely((unsigned)ret != pind1->len)) {
                TERR("write NOT_ZIP index of token %lu to file %d failed. ret %d",
                        pind1->term_sign, pind1->file_num, ret);
                return -1;
            }

            break;
        }
        case TS_IDT_BITMAP:
        case TS_IDT_ZIP_BITMAP:
            {
                bitmap_idx1_unit_t * pBitMapIdx = (bitmap_idx1_unit_t*)pind1;

                // build bitmap index
                memset(pbuilder->final_buf, 0, pbuilder->bitmap_size);
                buildBitmapIndex(pbuilder->raw_buf, doc_count, pbuilder->final_buf, pBitMapIdx->max_docId);

                // add index1 node to bitmap_dict
                pBitMapIdx->doc_count     = doc_count;
                pBitMapIdx->pos           = lseek(pbuilder->bitmap_fd, 0, SEEK_CUR);
                pBitMapIdx->not_orig_flag = ( false == _rebuildFlag ) ? 0 : 1;

                ret = idx_dict_add(pbuilder->bitmap_dict, &node);
                if (unlikely(ret<0)) {
                    TERR("add token %lu to bitmap idx1 dict failed. ret %d",
                            node.sign, ret);
                    return -1;
                }

                // write bitmap index
                ret = write(pbuilder->bitmap_fd, pbuilder->final_buf, pbuilder->bitmap_size);
                if (unlikely((unsigned)ret != pbuilder->bitmap_size)) {
                    TERR("write BITMAP index of token %lu failed. ret %d",
                            pind1->term_sign, ret);
                    return -1;
                }

                // 1. TS_IDT_ZIP_BITMAP no break, go on to build & write zip index
                // 2. TS_IDT_BITMAP break，不在保留 zip index
                if(disk_type == TS_IDT_BITMAP) break;
            }
        case TS_IDT_ZIP:
            {
                ret = abandonOverflow(pbuilder->raw_buf, doc_count, base_num, useLen);
                if(ret < 0) {
                    TERR("abandon overflow(zip) error, sign=%lu", node.sign);
                    return -1;
                } else if(ret > 0) {
                    TERR("idx2 docnum over bit level(zip), lastNum=%u, line sign=%lu",
                            doc_count, pind1->term_sign);
                }
                ret = buildP4DIndex(pbuilder->raw_buf, doc_count, pbuilder->final_buf);
                // add index1 node
                pind1->file_num      = pbuilder->inverted_fd_num - 1;
                pind1->pos           = pbuilder->cur_size;
                pind1->doc_count     = doc_count;
                pind1->len           = ret;
                pind1->zip_flag      = 2;
                pind1->not_orig_flag = ( false == _rebuildFlag ) ? 0 : 1;

                ret = idx_dict_add(pbuilder->inverted_dict, &node);
                if (unlikely(ret<0)) {
                    TERR("add token %lu to idx1 dict failed. ret %d",
                            node.sign, ret);
                    return -1;
                }

                // write index2
                ret = write(pbuilder->inverted_fd[pind1->file_num],
                        pbuilder->final_buf, pind1->len);
                if (unlikely((unsigned)ret != pind1->len)) {
                    TERR("write ZIP index of token %lu to file %d failed. ret %d",
                            pind1->term_sign, pind1->file_num, ret);
                    return -1;
                }

                break;
            }
        default:
            {
                TERR("wrong IDX_DISK_TYPE %d of token %lu",
                        disk_type, pind1->term_sign);
                return -1;
            }
    }

    // update cur_size
    if(disk_type != TS_IDT_BITMAP) {
        ret = update_cur_size(pind1->len);
        if(unlikely(ret<0)) {
            TERR("update file_size of file_no %d failed", pind1->file_num);
            return -1;
        }
    }
    return doc_count;
}


/*
 * func : dump index1 hash tables and close all index2 fds
 *
 * args : pbuilder, pointer of index_builder_t struct
 *
 * ret  : <0, error
 *      : 0, success
 */
int IndexFieldBuilder::dump()
{
    index_builder_t* pbuilder = (index_builder_t*)_pIndexBuilder;
    if (!pbuilder) {
        TERR("parameter error: pbuilder is NULL");
        return -1;
    }

    int  ret = 0;
    char filename[PATH_MAX];

    snprintf(filename, sizeof(filename), "%s.%s",
            pbuilder->field_name, INVERTED_IDX1_INFIX);
    ret = idx_dict_save(pbuilder->inverted_dict, pbuilder->path, filename);
    if (ret<0) {
        TERR("idx_dict_save to %s/%s failed.", pbuilder->path, filename);
        return -1;
    }

    snprintf(filename, sizeof(filename), "%s.%s",
            pbuilder->field_name, BITMAP_IDX1_INFIX);
    ret = idx_dict_save(pbuilder->bitmap_dict, pbuilder->path, filename);
    if (ret<0) {
        TERR("idx_dict_save to %s/%s failed.", pbuilder->path, filename);
        return -1;
    }

    for(unsigned int i=0; i < pbuilder->inverted_fd_num; i++) {
        if (pbuilder->inverted_fd[i] >= 0) {
            ::close(pbuilder->inverted_fd[i]);
            pbuilder->inverted_fd[i] = -1;
        }
    }
    pbuilder->inverted_fd_num = 0;

    if (pbuilder->bitmap_fd >= 0) {
        off_t off = lseek(pbuilder->bitmap_fd, 0, SEEK_CUR);
        if(off > 0) {// 防止bitmap访问越界，在最后增加8个byte
            char buf[8] = {0};
            write(pbuilder->bitmap_fd, buf, 8);
        }
        ::close(pbuilder->bitmap_fd);
        pbuilder->bitmap_fd = -1;
    }

    return 0;
}

/*
 * func : 将一行文本倒排索引解析为非压缩格式的2进制倒排索引
 *
 * args : [input]  line, 以\0结尾的一行文本，存储一个token对应的2级倒排索引
 *        [output] sign, token的64位签名
 *        [output] base_num, 独立doc_id高16bit的个数
 *        [in/out] buf, 存储非压缩格式2进制倒排索引的buffer
 *        [input]  buf_len, 上一参数buf的长度
 *
 * ret  : <=0, error
 *      : >0,  2进制倒排索引链条中doc_id的个数
 */
int IndexFieldBuilder::build_nzip_index(const char *line, int lineLen, uint64_t *sign, uint32_t *base_num,
        char *buf, const uint32_t buf_len)
{
    unsigned int       count    = 0;
    const char       * pre      = line;
    char             * cur      = NULL;
    unsigned int       pre_id   = INVALID_DOCID;
    unsigned int       pre_base = IDX_GET_BASE(INVALID_DOCID);
    idx2_nzip_unit_t * pind2    = (idx2_nzip_unit_t*)buf;
    unsigned int       ind2_max = buf_len/sizeof(idx2_nzip_unit_t); // max_doc_id
    const char* end = line + lineLen;

    // read term sign
    *sign = strtoull(pre, &cur, 10);
    if (unlikely(pre == cur)) {
        TERR("analyse term_sign from line %s failed", line);
        return -1;
    }

    *base_num = 0;
    while (count < ind2_max) {
        // read doc-id
        pre           = cur;
        pind2->doc_id = strtoul(pre, &cur, 10);
        if (unlikely(pre == cur) || cur >= end) { // 解析到行尾，退出循环
            break;
        }

        // read occ
        pre           = cur;
        pind2->occ    = strtoul(pre, &cur, 10);
        if (unlikely(pre == cur) || cur >= end) {
            // 解析到doc_id但解析不到对应的occ，line格式有问题，退出
            TERR("get occ corresponding doc_id<%u> failed, line %s",
                    pind2->doc_id, line);
            return -1;
        }

        // 本次解析得到的pind2->doc_id跟前一个doc_id不同才存到2进制索引中
        // 即仅保存相同doc_id的第一个occ
        if (pre_id != pind2->doc_id) {
            pre_id = pind2->doc_id;  // pre_id now is current id
            pind2++;
            count++;

            // doc_id高16位同前一个doc_id高16位不同，则计数器加一
            if (pre_base != IDX_GET_BASE(pre_id)) {
                pre_base = IDX_GET_BASE(pre_id);
                (*base_num)++;
            }
        }
    }

    if (unlikely(pre_id >= ind2_max)) { // 已解析的id里面有没有超过max_doc_id的
        TERR("some of doc_ids are larger than max_doc_id (%u)%u in line %s",
                pre_id, ind2_max, line);
        return -1;
    }

    if (unlikely(count == ind2_max)) {
        pre = cur;
        pre_id = strtoul(pre, &cur, 10);
        if (pre != cur) { // 还有未被解析完的文本倒排索引
            TERR("too many doc_ids in line %s", line);
            return -1;
        }
    }

    // 最后写入不可能出现的最大docid，用作监视哨
    pind2->doc_id = INVALID_DOCID;
    pind2->occ    = 0;

    return count;
}

int IndexFieldBuilder::buildNZipIndex(uint32_t *base_num, char *buf, const uint32_t buf_len, uint32_t& useBufLen)
{
    int32_t count = 0;
    char* outbuf = NULL;    //
    int32_t outlen = -1;    // read term 返回值,一个doc单元占用的内存大小
    uint32_t docId = 0;     // 返回的docId
    uint32_t preBase = IDX_GET_BASE(INVALID_DOCID);

    char* begin = buf;
    char* end = buf + buf_len;

    *base_num = 0;
    while ((outlen = _pLineParse->next(outbuf, docId)) > 0) {
        if(begin + outlen > end) {
            TERR("not alloc enough mem or some of doc_ids are larger than max_doc_id, \
                    sign=%lu", _pLineParse->getSign());
            return KS_ENOMEM;
        }
        memcpy(begin, outbuf, outlen);
        begin += outlen;

        // doc_id高16位同前一个doc_id高16位不同，则计数器加一
        if (preBase != IDX_GET_BASE(docId)) {
            preBase = IDX_GET_BASE(docId);
            (*base_num)++;
        }
        count++;
    }
    if(outlen < 0) {
        TERR("parse line failed, sign = %lu", _pLineParse->getSign());
        return -1;
    }
    if (unlikely(docId >= (uint32_t)_maxDocNum)) { // 已解析的id里面有没有超过max_doc_id的
        TERR("some of doc_ids are larger than max_doc_id (%u)%u in line %lu",
                docId, _maxDocNum, _pLineParse->getSign());
        return -1;
    }

    outlen = _pLineParse->invalid(outbuf, docId);
    if(begin + outlen > end) {
        TERR("please malloc more memory for watch dog, sign=%lu",
                _pLineParse->getSign());
        return KS_ENOMEM;
    }
    // 最后写入不可能出现的最大docid，用作监视哨
    memcpy(begin, outbuf, outlen);
    begin += outlen;
    useBufLen = begin - buf;

    return count;
}

/*
 * func : 根据非压缩格式的2进制倒排索引构建压缩格式的2进制倒排索引
 *
 * args : [input]  nzip, 存储非压缩格式2级倒排索引的buffer
 *        [input]  doc_count, doc_id个数
 *        [input]  base_num, 独立doc_id高16bit的个数
 *        [in/out] zip, 存储压缩格式2进制倒排索引的buffer
 *
 * ret  : <=0, error
 *      : >0, 压缩格式2进制倒排索引的长度（以字节为单位）
 */
int IndexFieldBuilder::build_zip_index(const char *nzip, const unsigned int doc_count,
        const unsigned int base_num, char *zip)
{
    idx2_nzip_unit_t     * notzip  = (idx2_nzip_unit_t*)nzip;
    idx2_zip_header_t    * pheader = (idx2_zip_header_t*)zip;
    idx2_zip_skip_unit_t * pskip   = (idx2_zip_skip_unit_t*)(pheader + 1);
    idx2_zip_list_unit_t * plist   = (idx2_zip_list_unit_t*)(pskip + base_num + 1);

    // build header
    pheader->skip_list_len = (base_num + 1) * sizeof(idx2_zip_skip_unit_t);
    pheader->doc_list_len  = (doc_count +1) * sizeof(idx2_zip_list_unit_t);
    pheader->block_num     = base_num;

    // build zip index2 body
    uint16_t pre_base = IDX_GET_BASE(INVALID_DOCID);
    uint16_t cur_base = 0;

    for (unsigned int i=0, j=0;i<doc_count;i++) {
        // build doc_list unit
        plist[i].doc_id_diff = IDX_GET_DIFF(notzip[i].doc_id);
        plist[i].occ         = notzip[i].occ;

        // build skip_list unit
        cur_base = IDX_GET_BASE(notzip[i].doc_id);
        if (cur_base != pre_base) {
            pskip[j].doc_id_base = cur_base;
            pskip[j].off         = i;
            pre_base             = cur_base;
            j++;
        }
    }

    // build end doc list
    plist[doc_count].doc_id_diff = IDX_GET_DIFF(INVALID_DOCID);
    plist[doc_count].occ = 0;

    // build auxillary block at the end of skiplist
    pskip[base_num].doc_id_base = IDX_GET_BASE(INVALID_DOCID);
    pskip[base_num].off         = doc_count;

    return sizeof(idx2_zip_header_t) + pheader->skip_list_len + pheader->doc_list_len;
}

int IndexFieldBuilder::buildZipIndex(const char *nzip, const unsigned int doc_count,
        const unsigned int base_num, char *zip)
{
    idx2_zip_header_t    * pheader = (idx2_zip_header_t*)zip;
    idx2_zip_skip_unit_t * pskip   = (idx2_zip_skip_unit_t*)(pheader + 1);

    // build header
    pheader->skip_list_len = (base_num + 1) * sizeof(idx2_zip_skip_unit_t);
    pheader->block_num     = base_num;

    // build zip index2 body
    uint16_t pre_base = IDX_GET_BASE(INVALID_DOCID);
    uint16_t cur_base = 0;

    if(_maxOccNum > 0) { // has occ info
        idx2_nzip_unit_t     * notzip  = (idx2_nzip_unit_t*)nzip;
        pheader->doc_list_len  = (doc_count +1) * sizeof(idx2_zip_list_unit_t);
        idx2_zip_list_unit_t * plist   = (idx2_zip_list_unit_t*)(pskip + base_num + 1);

        for (unsigned int i=0, j=0;i<doc_count;i++) {
            // build doc_list unit
            plist[i].doc_id_diff = IDX_GET_DIFF(notzip[i].doc_id);
            plist[i].occ         = notzip[i].occ;

            // build skip_list unit
            cur_base = IDX_GET_BASE(notzip[i].doc_id);
            if (cur_base != pre_base) {
                pskip[j].doc_id_base = cur_base;
                pskip[j].off         = i;
                pre_base             = cur_base;
                j++;
            }
        }
        // build end doc list
        plist[doc_count].doc_id_diff = IDX_GET_DIFF(INVALID_DOCID);
        plist[doc_count].occ = 0;
    } else { // null occ info
        uint32_t * notzip  = (uint32_t*)nzip;
        pheader->doc_list_len  = (doc_count + 1) * sizeof(uint16_t);
        uint16_t* plist   = (uint16_t*)(pskip + base_num + 1);

        for (unsigned int i=0, j=0;i<doc_count;i++) {
            // build doc_list unit
            plist[i] = IDX_GET_DIFF((notzip[i]));

            // build skip_list unit
            cur_base = IDX_GET_BASE(notzip[i]);
            if (cur_base != pre_base) {
                pskip[j].doc_id_base = cur_base;
                pskip[j].off         = i;
                pre_base             = cur_base;
                j++;
            }
        }
        // build end doc list
        plist[doc_count] = IDX_GET_DIFF(INVALID_DOCID);
    }

    // build auxillary block at the end of skiplist
    pskip[base_num].doc_id_base = IDX_GET_BASE(INVALID_DOCID);
    pskip[base_num].off         = doc_count;

    return sizeof(idx2_zip_header_t) + pheader->skip_list_len + pheader->doc_list_len;
}

int IndexFieldBuilder::buildP4DIndex(const char *nzip, const unsigned int doc_count, char *zip)
{
    int ret = 0;
    int nBlockNum = doc_count / P4D_BLOCK_SIZE;
    if(doc_count % P4D_BLOCK_SIZE != 0)
        nBlockNum++;

    idx2_zip_header_t    * pheader = (idx2_zip_header_t*)zip;
    idx2_p4d_skip_unit_t * pskip   = (idx2_p4d_skip_unit_t*)(pheader + 1);

    // build header
    pheader->skip_list_len = nBlockNum * sizeof(idx2_p4d_skip_unit_t);
    pheader->block_num     = nBlockNum;
    uint8_t *pIdList = (uint8_t*)(pskip + nBlockNum);
    uint32_t nWriteSize = 0;

    // build zip index2 body
    if(_maxOccNum > 0)
    { // has occ info
        uint8_t *pIdPos = pIdList;
        idx2_nzip_unit_t *notzip  = (idx2_nzip_unit_t*)nzip;
        uint32_t ids[P4D_BLOCK_SIZE];
        int k = 0;
        uint32_t j = 0;
        while(j < doc_count)
        {
            int i = 0;
            while(i < P4D_BLOCK_SIZE && j < doc_count)
            {
                ids[i] = notzip[j].doc_id;
                i++;
                j++;
            }
            pskip[k].off = nWriteSize;
            pskip[k].doc_id_base = ids[i-1];

            _idxComp.clearEncode();
            uint8_t *buf = _idxComp.compression(ids, i);
            uint32_t nEncodeSize = _idxComp.getCompSize();
            memcpy(pIdPos, buf, nEncodeSize);

            nWriteSize += nEncodeSize;
            k++;
            pIdPos += nEncodeSize;
        }
        pheader->doc_list_len = pIdPos - pIdList;
        for(uint32_t i = 0; i < doc_count; i++)
        {
            pIdPos[i] = notzip[i].occ;
        }
        ret = sizeof(idx2_zip_header_t) + pheader->skip_list_len + pheader->doc_list_len + doc_count * sizeof(uint8_t);
    }
    else
    {
        uint8_t *pIdPos = pIdList;
        uint32_t *notzip  = (uint32_t*)nzip;
        uint32_t ids[P4D_BLOCK_SIZE];
        int k = 0;
        uint32_t j = 0;
        while(j < doc_count)
        {
            int i = 0;
            while(i < P4D_BLOCK_SIZE && j < doc_count)
            {
                ids[i] = notzip[j];
                i++;
                j++;
            }
            pskip[k].off = nWriteSize;
            pskip[k].doc_id_base = ids[i-1];

            _idxComp.clearEncode();
            uint8_t *buf = _idxComp.compression(ids, i);
            int nEncodeSize = _idxComp.getCompSize();
            memcpy(pIdPos, buf, nEncodeSize);

            nWriteSize += nEncodeSize;
            k++;
            pIdPos += nEncodeSize;
        }
        pheader->doc_list_len = pIdPos - pIdList;
        ret = sizeof(idx2_zip_header_t) + pheader->skip_list_len + pheader->doc_list_len;
    }

    return ret;
}
/*
 * func : 根据非压缩格式的2进制倒排索引构建bitmap索引
 *
 * args : [input]  nzip, 存储非压缩格式2级倒排索引的buffer
 *        [input]  doc_count, doc_id个数
 *        [in/out] final_buf, 存储bitmap索引的buffer
 *
 */
void IndexFieldBuilder::build_bitmap_index(const char *nzip, const unsigned int doc_count,
        char *final_buf)
{
    idx2_nzip_unit_t *list   = (idx2_nzip_unit_t*)nzip;
    uint64_t         *bitmap = (uint64_t*)final_buf;

    for(unsigned int i=0;i<doc_count;i++)
    {
        bitmap[list[i].doc_id / 64] |= bit_mask_tab[list[i].doc_id % 64];
    }
}
void IndexFieldBuilder::buildBitmapIndex(const char *nzip, const unsigned int doc_count,
        char *final_buf, uint32_t& maxDocId)
{
    uint64_t *bitmap = (uint64_t*)final_buf;

    if(_maxOccNum > 0) {
        idx2_nzip_unit_t *list   = (idx2_nzip_unit_t*)nzip;
        for(unsigned int i=0;i<doc_count;i++)
        {
            bitmap[list[i].doc_id / 64] |= bit_mask_tab[list[i].doc_id % 64];
        }
        maxDocId = list[doc_count-1].doc_id;
    } else {
        uint32_t *list   = (uint32_t*)nzip;
        for(unsigned int i=0;i<doc_count;i++)
        {
            bitmap[list[i] / 64] |= bit_mask_tab[list[i] % 64];
        }
        maxDocId = list[doc_count-1];
    }
}
/*
 * func : 根据输入参数确定2进制索引的存储格式
 *
 * args : [input]  doc_count, doc_id个数
 *        [input]  base_num, 独立doc_id高16bit的个数
 *        [input]  bitmap_size, bitmap索引的大小
 *
 * ret  : 2进制索引的存储格式
 */
IDX_DISK_TYPE IndexFieldBuilder::set_idt(const unsigned int doc_count,
        const unsigned int base_num, const unsigned int bitmap_size)
{
    uint32_t notzip_len = 0;
    uint32_t zip_len = 0;

    if(_maxOccNum > 0) {
        notzip_len = doc_count * sizeof(idx2_nzip_unit_t);
        zip_len = sizeof(idx2_zip_header_t)
            + (base_num + 1) * sizeof(idx2_zip_skip_unit_t)
            + doc_count * sizeof(idx2_zip_list_unit_t);
    } else {
        notzip_len = doc_count * sizeof(uint32_t);
        zip_len = sizeof(idx2_zip_header_t)
            + (base_num + 1) * sizeof(idx2_zip_skip_unit_t)
            + doc_count * sizeof(uint16_t);
    }

    if(_maxOccNum <= 0) { // 无occ情况下，在压缩、非压缩、bitmap之间选择一个
        //IDX_DISK_TYPE ret = zip_len >= notzip_len ? TS_IDT_NOT_ZIP : TS_IDT_ZIP;
        IDX_DISK_TYPE ret = doc_count <= P4D_BLOCK_SIZE ? TS_IDT_NOT_ZIP : TS_IDT_ZIP;
        uint32_t minlen = zip_len >= notzip_len ? notzip_len : zip_len;
        if(minlen > bitmap_size)
            ret = TS_IDT_BITMAP;
        return ret;
    }

    if (doc_count <= P4D_BLOCK_SIZE) {
        return TS_IDT_NOT_ZIP;
    }

    if (zip_len < bitmap_size) {
        // 存为压缩格式比bitmap格式小，则不保存该token的bitmap索引
        return TS_IDT_ZIP;
    }

    return TS_IDT_ZIP_BITMAP;
}

/*
 * func : 更新当前2级倒排索引文件的已写入长度，
 *        若大于指定长度，则新开一个2级倒排索引文件
 *
 * args : [in/out] pbuilder, 建索引句柄
 *        [input]  len, 最近一次写入2级索引文件的索引长度
 *
 * ret  : 0, success
 *        <0, error
 */
int IndexFieldBuilder::update_cur_size(const unsigned int len)
{
    index_builder_t *pbuilder = (index_builder_t*)_pIndexBuilder;
    pbuilder->cur_size += len;
    if (pbuilder->cur_size < MAX_IDX2_FILE_LEN) {
        return 0;
    }

    // if current index2 file is large enough, open the next index2 file
    if (pbuilder->inverted_fd_num >= MAX_IDX2_FD) {
        TLOG("num of inverted idx2 files is %d now!", MAX_IDX2_FD);
        return -1;
    }

    char filename[PATH_MAX];
    snprintf(filename, sizeof(filename), "%s/%s.%s.%u", pbuilder->path,
            pbuilder->field_name, INVERTED_IDX2_INFIX, pbuilder->inverted_fd_num);

    pbuilder->inverted_fd[pbuilder->inverted_fd_num] =
        ::open(filename, O_CREAT|O_WRONLY|O_TRUNC|O_APPEND, COMMON_FILE_MODE);
    if (unlikely(pbuilder->inverted_fd[pbuilder->inverted_fd_num] < 0)) {
        TERR("open file %s failed. errno %d", filename, errno);
        return -1;
    }

    pbuilder->cur_size = 0;
    (pbuilder->inverted_fd_num)++;
    return 0;
}

int IndexFieldBuilder::abandonOverflow(char *buf, uint32_t& docCount, uint32_t& baseNum, uint32_t& useBufLen)
{
    if(docCount <= MAX_IDX2_DOC_NUM) {
        return 0;
    }

    // 有occ情况下，倒排单位大小
    int32_t nodeSize = sizeof(idx2_nzip_unit_t);
    // 无occ情况下，倒排单位大小
    if(_maxOccNum <= 0) {
        nodeSize = sizeof(uint32_t);
    }

    char* begin = buf + nodeSize * MAX_IDX2_DOC_NUM;
    char* end = buf + nodeSize * docCount;

    // 截断的最大保留个数
    int ret = docCount - MAX_IDX2_DOC_NUM;
    docCount = MAX_IDX2_DOC_NUM;

    // 回退base_num
    uint32_t preBase = IDX_GET_BASE(*(uint32_t*)(begin - nodeSize));
    while (begin < end) {
        // doc_id高16位同前一个doc_id高16位不同，则计数器加一
        if (preBase != IDX_GET_BASE(*(uint32_t*)begin)) {
            preBase = IDX_GET_BASE(*(uint32_t*)begin);
            baseNum--;
        }
        begin += nodeSize;
    }

    uint32_t docId;
    char* outbuf = NULL;
    int32_t outlen = _pLineParse->invalid(outbuf, docId);

    // 最后写入不可能出现的最大docid，用作监视哨
    begin = buf + nodeSize * MAX_IDX2_DOC_NUM;
    memcpy(begin, outbuf, outlen);
    begin += outlen;
    useBufLen = begin - buf;

    return ret;
}

// 顺序遍历所有的term
const IndexTermInfo* IndexFieldBuilder::getFirstTerm(uint64_t & termSign)
{
    full_idx1_unit_t* pIdxNode = NULL;
    bitmap_idx1_unit_t* pBitmapNode = NULL;

    _travelPos = -1;
    _pDictTravel = _pIndexBuilder->inverted_dict;

    // 获取一级索引的第一个节点
    pIdxNode = (full_idx1_unit_t*)idx_dict_first(_pDictTravel, &_travelPos);
    if(NULL == pIdxNode) {
        _pDictTravel = _pIndexBuilder->bitmap_dict;
        pBitmapNode = (bitmap_idx1_unit_t*)idx_dict_first(_pDictTravel, &_travelPos);
        if(NULL == pBitmapNode) {
            return NULL;
        }
        termSign = pBitmapNode->term_sign;

        _cIndexTermInfo.docNum        = pBitmapNode->doc_count;    // 2级索引链条中doc的数量
        _cIndexTermInfo.occNum        = pBitmapNode->doc_count;    // occ链表中含有的occ数量
        _cIndexTermInfo.zipType       = 0;
        _cIndexTermInfo.bitmapFlag    = 1;
        _cIndexTermInfo.not_orig_flag = pBitmapNode->not_orig_flag;
    } else {
        termSign = pIdxNode->term_sign;

        _cIndexTermInfo.docNum        = pIdxNode->doc_count;    // 2级索引链条中doc的数量
        _cIndexTermInfo.occNum        = pIdxNode->doc_count;    // occ链表中含有的occ数量
        _cIndexTermInfo.not_orig_flag = pIdxNode->not_orig_flag;

        if(pIdxNode->zip_flag) {
            _cIndexTermInfo.zipType = 2;
        } else {
            _cIndexTermInfo.zipType = 1;
        }
        // 到bitmap索引中查找是否存在bitmap索引
        pBitmapNode = (bitmap_idx1_unit_t*)idx_dict_find(_pIndexBuilder->bitmap_dict, pIdxNode->term_sign);
        if(NULL == pBitmapNode) {
            _cIndexTermInfo.bitmapFlag = 0;
        } else {
            _cIndexTermInfo.bitmapFlag = 1;
        }
    }
    return &_cIndexTermInfo;
}

const IndexTermInfo* IndexFieldBuilder::getNextTerm(uint64_t & termSign)
{
    full_idx1_unit_t* pIdxNode = NULL;
    bitmap_idx1_unit_t* pBitmapNode = NULL;

    // 获取一级索引的下一个节点
    idict_node_t* pNode = idx_dict_next(_pDictTravel, &_travelPos);
    if(NULL == pNode) {
        // 为bitmap的一级索引，则遍历结束
        if(_pDictTravel == _pIndexBuilder->bitmap_dict) {
            return NULL;
        }
        // 为InvertList的一级索引，则开始遍历bitmap，查找单独bitmap存在的情况
        _pDictTravel =  _pIndexBuilder->bitmap_dict;
        pNode = idx_dict_first(_pDictTravel, &_travelPos);
        if(NULL == pNode) { // 没有bitmap索引
            return NULL;
        }
    }

    if(_pDictTravel == _pIndexBuilder->bitmap_dict) { // bitmap idx
        // 直到找到一个，invertlist里面没有的
        while(pNode && (pIdxNode = (full_idx1_unit_t*)idx_dict_find(_pIndexBuilder->inverted_dict, pNode->sign))) {
            pNode = idx_dict_next(_pDictTravel, &_travelPos);
        }
        if(NULL == pNode) { // 结束
            return NULL;
        }

        pBitmapNode = (bitmap_idx1_unit_t*)pNode;
        termSign = pBitmapNode->term_sign;
        _cIndexTermInfo.docNum = pBitmapNode->doc_count;    // 2级索引链条中doc的数量
        _cIndexTermInfo.occNum = pBitmapNode->doc_count;    // occ链表中含有的occ数量
        _cIndexTermInfo.zipType = 0;
        _cIndexTermInfo.bitmapFlag = 1;
        _cIndexTermInfo.not_orig_flag = pBitmapNode->not_orig_flag;
    } else {
        pIdxNode = (full_idx1_unit_t*)pNode;
        termSign = pIdxNode->term_sign;
        _cIndexTermInfo.docNum = pIdxNode->doc_count;    // 2级索引链条中doc的数量
        _cIndexTermInfo.occNum = pIdxNode->doc_count;    // occ链表中含有的occ数量
        _cIndexTermInfo.not_orig_flag = pIdxNode->not_orig_flag;

        if(pIdxNode->zip_flag) {
            _cIndexTermInfo.zipType = 2;
        } else {
            _cIndexTermInfo.zipType = 1;
        }
        // 到bitmap索引中查找是否存在bitmap索引
        pBitmapNode = (bitmap_idx1_unit_t*)idx_dict_find(_pIndexBuilder->bitmap_dict, pIdxNode->term_sign);
        if(NULL == pBitmapNode) {
            _cIndexTermInfo.bitmapFlag = 0;
        } else {
            _cIndexTermInfo.bitmapFlag = 1;
        }
    }
    return &_cIndexTermInfo;
}

IndexTerm * IndexFieldBuilder::getTerm(uint64_t termSign)
{
    full_idx1_unit_t* pIdxNode = NULL;
    bitmap_idx1_unit_t* pBitmapNode = NULL;

    pIdxNode = (full_idx1_unit_t*)idx_dict_find(_pIndexBuilder->inverted_dict, termSign);
    pBitmapNode = (bitmap_idx1_unit_t*)idx_dict_find(_pIndexBuilder->bitmap_dict, termSign);
    if(unlikely(NULL == pIdxNode && NULL == pBitmapNode)) {
        return NULL;
    }

    if(unlikely(NULL == _pMemPool)) {
        _pMemPool = new (std::nothrow)MemPool;
    } else {
        _pMemPool->reset();
    }

    index_builder_t* pbuilder = _pIndexBuilder;
    uint64_t* bitmap = NULL;
    if(pBitmapNode) {
        if(unlikely(NULL == _fpBitMapList)) {
            char filename[PATH_MAX];
            snprintf(filename, sizeof(filename), "%s/%s.%s",
                    pbuilder->path, pbuilder->field_name, BITMAP_IDX2_INFIX);
            _fpBitMapList = fopen(filename, "rb");
        }
        bitmap = (uint64_t*)pbuilder->raw_buf;
        fseek(_fpBitMapList, pBitmapNode->pos, SEEK_SET);
        fread(bitmap, pbuilder->bitmap_size, 1,  _fpBitMapList);
    }

    char* invertList = NULL;
    if(pIdxNode) {
        if(_inverted_file_num != pIdxNode->file_num) {
            if(_fpInvertList) fclose(_fpInvertList);
            _inverted_file_num = pIdxNode->file_num;

            char filename[PATH_MAX];
            snprintf(filename, sizeof(filename), "%s/%s.%s.%u",
                    pbuilder->path, pbuilder->field_name, INVERTED_IDX2_INFIX, pIdxNode->file_num);
            _fpInvertList = fopen(filename, "rb");
        }
        invertList = pbuilder->final_buf;
        fseek(_fpInvertList, pIdxNode->pos, SEEK_SET);
        fread(invertList, pIdxNode->len, 1, _fpInvertList);

        // 压缩类型，0:没有倒排链  1: 不压缩 2: 高位压缩 3: p4delta压缩
        if(pIdxNode->zip_flag == 2) {
            return _indexTermFactory.make(_pMemPool, 3,
                    bitmap, invertList, pIdxNode->doc_count, _maxDocNum, 0 , pIdxNode->not_orig_flag);
        } else if(pIdxNode->zip_flag == 1) {
            return _indexTermFactory.make(_pMemPool, 2,
                    bitmap, invertList, pIdxNode->doc_count, _maxDocNum, 0 , pIdxNode->not_orig_flag);
        } else {
            return _indexTermFactory.make(_pMemPool, 1,
                    bitmap, invertList, pIdxNode->doc_count, _maxDocNum, 0 , pIdxNode->not_orig_flag);
        }
    }

    // 只有bitmap索引
    return _indexTermFactory.make(_pMemPool, 0, bitmap, invertList, pBitmapNode->doc_count
                                  , _maxDocNum, pBitmapNode->max_docId, pBitmapNode->not_orig_flag);
}

INDEX_LIB_END

