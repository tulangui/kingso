#include "IndexFieldInc.h"
#include "index_struct.h"
#include "IndexTermActualize.h"
#include "index_lib/DocIdManager.h"
#include "IndexFieldSyncManager.h"

INDEX_LIB_BEGIN

IndexFieldInc::IndexFieldInc(int32_t maxOccNum, uint32_t fullDocNum, index_mem::ShareMemPool & pool) : _indexTermFactory(maxOccNum), _pool(pool)
{
    _maxOccNum      = maxOccNum;
    _fullDocNum     = fullDocNum;
    _invertedDict   = NULL;            // 1级倒排索引hash表
    _bitmapDict     = NULL;            // 全量bitmap索引hash表
    _pIncMemManager = NULL;            // 增量内存管理
    _docIdMgr       = NULL;

    _travelPos      = 0;
    _travelDict     = NULL;
}

IndexFieldInc::~IndexFieldInc()
{
  if(_pIncMemManager) {
    delete _pIncMemManager;
    _pIncMemManager = NULL;
  }
  if(_invertedDict) {
    IndexFieldSyncManager * pSyncMgr = (IndexFieldSyncManager *) _invertedDict->syncMgr;
    SAFE_DELETE(pSyncMgr);
    idx_dict_free(_invertedDict);
    _invertedDict = NULL;
  }
  if(_bitmapDict) {
      IndexFieldSyncManager * pSyncMgr = (IndexFieldSyncManager *) _bitmapDict->syncMgr;
      SAFE_DELETE(pSyncMgr);
      idx_dict_free(_bitmapDict);
      _bitmapDict = NULL;
  }
}

int IndexFieldInc::open(const char * path, const char* fieldName, bool incLoad, bool sync, int32_t incSkipCnt, int32_t incBitmapSize)
{
    if(strlen(path) >= PATH_MAX || strlen(fieldName) >= MAX_FIELD_NAME_LEN) {
        TERR("IndexFieldInc open error, path=%s,field=%s", path, fieldName);
        return KS_EINVAL;
    }
    strcpy(_idxPath, path);
    strcpy(_fieldName, fieldName);

    _maxIncSkipCnt     = incSkipCnt;
    _maxIncBitmapSize  = incBitmapSize;

    // 创建二级索引内存
    char fileName[PATH_MAX];
    snprintf(fileName, PATH_MAX, "%s/%s.%s", _idxPath, _fieldName, INC_IND2_POSTFIX);
    _pIncMemManager = new (std::nothrow) IndexIncMemManager(fileName, _pool);
    if(NULL == _pIncMemManager) {
        return KS_ENOMEM;
    }

    // 一级索引管理
    snprintf(fileName, sizeof(fileName), "%s.inc.%s", _fieldName, INVERTED_IDX1_INFIX);
    _invertedDict = idx_dict_load(path, fileName);
    if (NULL == _invertedDict && incLoad) {
        // 检查一级索引/二级索引加载匹配情况
        TERR("IndexFieldInc open error, load dict [%s] failed!", fileName);
        return KS_ENOMEM;
    }

    if (_invertedDict && !incLoad) {
        // 检查一级索引/二级索引加载匹配情况
        TERR("IndexFieldInc open error, dict [%s] should not exist!", fileName);
        return KS_ENOMEM;
    }

    if (NULL == _invertedDict) {
        _invertedDict = idx_dict_create( 393241, 24593 );
        IndexFieldSyncManager * pSyncMgr = NULL;
        if ( sync ) {
            pSyncMgr = new (std::nothrow) IndexFieldSyncManager(_invertedDict, path, fileName);
            if (pSyncMgr) {
                pSyncMgr->putIdxHashSize();
                pSyncMgr->putIdxBlockPos();
                pSyncMgr->putIdxHashTable();
                pSyncMgr->syncToDisk();
            }
        }
        _invertedDict->syncMgr = pSyncMgr;
    }
    else {
        IndexFieldSyncManager * pSyncMgr = NULL;
        if ( sync ) {
            pSyncMgr = new (std::nothrow) IndexFieldSyncManager(_invertedDict, path, fileName);
        }
        _invertedDict->syncMgr = pSyncMgr;
    }

    if (NULL == _invertedDict) {
        return KS_ENOMEM;
    }

    // 获取全量bitmap信息，用于增量bitmap参照
    char bitmapFileName[PATH_MAX];
    snprintf(bitmapFileName, sizeof(bitmapFileName), "%s.inc.%s", fieldName, BITMAP_IDX1_INFIX);
    _bitmapDict = idx_dict_load(path, bitmapFileName);
    if(_bitmapDict == NULL) {
        char fullBitmapFileName[PATH_MAX];
        snprintf(fullBitmapFileName, sizeof(fullBitmapFileName), "%s.%s", fieldName, BITMAP_IDX1_INFIX);
        _bitmapDict = idx_dict_load(path, fullBitmapFileName);
        if(_bitmapDict)
        {
            uint32_t off = 0;
            uint32_t pos = 0;
            idict_node_t* node = idx_dict_first(_bitmapDict, &pos);
            while(node)
            {
                ((bitmap_idx1_unit_t*)node)->doc_count = 0;
                if(_pIncMemManager->makeNewPage(_maxIncBitmapSize + 8, off) < 0)
                {
                    idx_dict_free(_bitmapDict);
                    _bitmapDict = NULL;
                    break;
                }
                char *buf = _pIncMemManager->offset2Addr(off);
                memset(buf, 0, _maxIncBitmapSize);
                ((bitmap_idx1_unit_t*)node)->pos = off;
                node = idx_dict_next(_bitmapDict, &pos);
            }

            IndexFieldSyncManager * pSyncMgr = NULL;
            if ( sync ) {
                pSyncMgr = new (std::nothrow) IndexFieldSyncManager(_bitmapDict, path, bitmapFileName);
                if (pSyncMgr) {
                    pSyncMgr->putIdxHashSize();
                    pSyncMgr->putIdxBlockPos();
                    pSyncMgr->putIdxHashTable();
                    pSyncMgr->putIdxAllBlock();
                    pSyncMgr->syncToDisk();
                }
            }
            _bitmapDict->syncMgr = pSyncMgr;
        }
    }
    else {
        IndexFieldSyncManager * pSyncMgr = NULL;
        if ( sync ) {
            pSyncMgr = new (std::nothrow) IndexFieldSyncManager(_bitmapDict, path, bitmapFileName);
        }
        _bitmapDict->syncMgr = pSyncMgr;
    }

    _docIdMgr = DocIdManager::getInstance();
    return 0;
}

int IndexFieldInc::close()
{
    _pIncMemManager->close();
    return 0;
}

int IndexFieldInc::dump(bool flag)
{
    char filename[PATH_MAX];
    snprintf(filename, PATH_MAX, "%s.%s", _fieldName, INC_IND1_INFIX);

    int ret = idx_dict_save(_invertedDict, _idxPath, filename);
    if (ret<0) {
        TERR("idx_dict_save to %s/%s failed.", _idxPath, filename);
        return -1;
    }
    if(!flag) { // 不需要导出文本索引
        return 0;
    }

    idx_dict_t* pdict = idx_dict_load(_idxPath, filename);
    if(NULL == pdict) {
        TERR("idx_dict_load to %s/%s failed.", _idxPath, filename);
        return -1;
    }
    ret = idx2Txt(pdict);
    idx_dict_free(pdict);
    if(ret < 0) {
        TERR("idx2Txt to %s/%s failed.", _idxPath, filename);
        return -1;
    }

    return 0;
}

IndexTerm * IndexFieldInc::getTerm(MemPool *pMemPool, uint64_t termSign)
{
  idict_node_t * pNode = idx_dict_find(_invertedDict, termSign);
  if (NULL == pNode) {
    return NULL;
  }

  uint64_t *pBitmap = NULL;
  if(_bitmapDict)
  {
      idict_node_t * pNodeBitmap = idx_dict_find(_bitmapDict, termSign);
      if(pNodeBitmap)
      {
          bitmap_idx1_unit_t* pIncBitmap = (bitmap_idx1_unit_t*)pNodeBitmap;
          pBitmap = (uint64_t*)(_pIncMemManager->offset2Addr(pIncBitmap->pos));
          pBitmap -= _fullDocNum / 64;                // 增量bitmap位计算需要按照全量doc数做偏移
      }
  }

  inc_idx1_unit_t* pInc = (inc_idx1_unit_t*)pNode;
  inc_idx2_header_t *pIncHead = (inc_idx2_header_t*)_pIncMemManager->offset2Addr(pInc->pos);
  char* invertList = (char*)(pIncHead + 1);

  return _indexTermFactory.make(pMemPool, pIncHead->zipFlag+1, pBitmap, invertList, pInc->doc_count,
          DocIdManager::getInstance()->getDocIdCount());
}

int IndexFieldInc::addTerm(uint64_t termSign, uint32_t docId, uint8_t firstOcc)
{
    // 检查并删除idx_dict中超时的buffer空间
    idx_dict_free_oldblock(_bitmapDict);
    idx_dict_free_oldblock(_invertedDict);

	if(unlikely((_fullDocNum + 8*_maxIncBitmapSize) <= docId))
	{
		TWARN("inc reach max doc count limit!");
		return KS_SUCCESS;
	}
    if(_bitmapDict)
    {
        idict_node_t* pNodeBitmap = idx_dict_find(_bitmapDict, termSign);
        if(pNodeBitmap)
        {
            bitmap_idx1_unit_t* pIncBitmap = (bitmap_idx1_unit_t*)pNodeBitmap;
            uint64_t * pBitmap =  (uint64_t*)(_pIncMemManager->offset2Addr(pIncBitmap->pos));
            pBitmap -= _fullDocNum / 64;
            pBitmap[docId / 64] |= bit_mask_tab[docId % 64];
            pIncBitmap->doc_count++;
            pIncBitmap->max_docId = docId;
            if (_bitmapDict->syncMgr) {
                IndexFieldSyncManager * pSyncMgr = (IndexFieldSyncManager *) _bitmapDict->syncMgr;
                pSyncMgr->putIdxDictNode(pNodeBitmap);
                pSyncMgr->syncToDisk();
            }
        }
    }
    inc_idx2_header_t* head = NULL;

    // 确定新的2级索引单元的写入位置
    idict_node_t* pNode = idx_dict_find(_invertedDict, termSign);

    if(pNode == NULL) { // 没有这个token, 申请一个新的block
        int32_t pageSize = getPageSize(0, 0);
        uint32_t off  = 0;
        if(_pIncMemManager->makeNewPage(pageSize, off) < 0) {
            return KS_ENOMEM;
        }
        char* buf = _pIncMemManager->offset2Addr(off);
        head = (inc_idx2_header_t*)buf;
        head->pageSize = _pIncMemManager->size(off);
        head->endOff = sizeof(inc_idx2_header_t);
        head->zipFlag = 0;

        idict_node_t node = {0L, 0, 0, 0, 0};
        node.sign = termSign;
        ((inc_idx1_unit_t*)&node)->pos = off;
        if (0 != idx_dict_add(_invertedDict, &node)) {
            TWARN("add token %lu to idx1 dict failed", node.sign);
            return KS_EFAILED;
        }
        pNode = idx_dict_find(_invertedDict, termSign);
    }

    if(NULL == pNode) {
        TWARN("add token %lu to idx1 dict failed", termSign);
        return KS_EFAILED;
    }

    // 获取头信息，定位到空白内存处
    inc_idx1_unit_t* pind1 = (inc_idx1_unit_t*)pNode;
    char* begin = _pIncMemManager->offset2Addr(pind1->pos);
    head = (inc_idx2_header_t*)begin;
    char* end = begin + head->pageSize;
//    begin = _pIncMemManager->offset2Addr(head->endOff);
    begin += head->endOff;

    uint32_t docCount = getDocNumByEndOff(head->endOff, head->zipFlag);
    if (unlikely(docCount != pind1->doc_count)) {
        TWARN("doc_count[%u] in idx1 mismatch idx2_header[%u]!", pind1->doc_count, docCount);
        pind1->doc_count = docCount;
    }

    /*
    if (head->zipFlag == 1) {
        idx2_zip_header_t *zipHeader = (idx2_zip_header_t*)(head + 1);
        idx2_zip_skip_unit_t *skip = (idx2_zip_skip_unit_t*)(zipHeader + 1);

        if (skip[zipHeader->block_num - 1].off > (docCount - 1)) {
            TWARN("reduce block_num!");
            zipHeader->block_num--;
            skip[zipHeader->block_num].off         = docCount;
            skip[zipHeader->block_num].doc_id_base = IDX_GET_BASE(INVALID_DOCID);
        }
    }
    */

    // 扩容
    uint32_t len = 0;
    if(head->zipFlag == 1)
    {
        len = (_maxOccNum > 0) ? sizeof(idx2_zip_list_unit_t) : sizeof(uint16_t);
        if(begin + (len<<1) > end)
        {
            int32_t ret = expend(head, pind1, begin, end);
            if(ret != KS_SUCCESS)
                return ret;
        }
    }
    else
    {
        len = (_maxOccNum > 0) ? sizeof(idx2_nzip_unit_t) : sizeof(uint32_t);
        if(begin + (len<<1) > end)
        {
            int32_t ret = expend(head, pind1, begin, end);
            if(ret != KS_SUCCESS)
                return ret;
        }
    }
    // 增加一篇文档
    if(head->zipFlag == 1)
    {
        uint16_t base = IDX_GET_BASE(docId);
        uint16_t diff = IDX_GET_DIFF(docId);
        idx2_zip_header_t *zipHeader = (idx2_zip_header_t*)(head + 1);
        idx2_zip_skip_unit_t *skip = (idx2_zip_skip_unit_t*)(zipHeader + 1);

        if(_maxOccNum == 0)
        {
            *(uint16_t*)(begin+sizeof(uint16_t)) = IDX_GET_DIFF(INVALID_DOCID);;
            *(uint16_t*)begin = diff;
            begin += sizeof(uint16_t);
            zipHeader->doc_list_len += sizeof(uint16_t);
        }
        else
        {
            ((idx2_zip_list_unit_t*)(begin+sizeof(idx2_zip_list_unit_t)))->doc_id_diff = IDX_GET_DIFF(INVALID_DOCID);
            ((idx2_zip_list_unit_t*)begin)->doc_id_diff = diff;
            ((idx2_zip_list_unit_t*)begin)->occ = firstOcc;
            begin += sizeof(idx2_zip_list_unit_t);
            zipHeader->doc_list_len += sizeof(idx2_zip_list_unit_t);
        }

        if(zipHeader->block_num == 0 || base > skip[zipHeader->block_num-1].doc_id_base)
        {
            if(unlikely(zipHeader->block_num == _maxIncSkipCnt))
            {
                TWARN("inc reach max doc count limit!");
                return KS_SUCCESS;
            }
            skip[zipHeader->block_num+1].doc_id_base = IDX_GET_BASE(INVALID_DOCID);
            skip[zipHeader->block_num+1].off = pind1->doc_count + 1;
            skip[zipHeader->block_num].doc_id_base = base;
            skip[zipHeader->block_num].off = pind1->doc_count;
            zipHeader->block_num++;
        }
        else
        {
            skip[zipHeader->block_num].off = pind1->doc_count + 1;
        }

        pind1->doc_count++;
    }
    else
    {
        if(_maxOccNum == 0)
        {
            *(uint32_t*)(begin+sizeof(uint32_t)) = INVALID_DOCID;
            *(uint32_t*)begin = docId;
            pind1->doc_count++;
            begin += sizeof(uint32_t);
        }
        else
        {
            ((idx2_nzip_unit_t*)(begin+sizeof(idx2_nzip_unit_t)))->doc_id = INVALID_DOCID;
            ((idx2_nzip_unit_t*)begin)->doc_id = docId;
            ((idx2_nzip_unit_t*)begin)->occ    = firstOcc;
            pind1->doc_count++;
            begin += sizeof(idx2_nzip_unit_t);
        }
    }

    char* p1 = _pIncMemManager->offset2Addr(pind1->pos);
    head->endOff = begin - p1;

    if (_invertedDict->syncMgr)
    {
        IndexFieldSyncManager * pSyncMgr = (IndexFieldSyncManager *) _invertedDict->syncMgr;
        pSyncMgr->putIdxDictNode(pNode);
        pSyncMgr->syncToDisk();
    }

    return 1;
}

int32_t IndexFieldInc::idx2Txt(idx_dict_t* pdict)
{
    if(NULL == pdict) {
        return 0;
    }

    char filename[PATH_MAX];
    snprintf(filename, PATH_MAX, "%s/%s.txt", _idxPath, _fieldName);

    FILE* fp = fopen(filename, "wb");
    if(NULL == fp) {
        return -1;
    }

    unsigned int pos = 0;
    MemPool cMemPool;
    idict_node_t* pNode = idx_dict_first(pdict, &pos);
    DocIdManager* pDoc = DocIdManager::getInstance();
    DocIdManager::DeleteMap* del = pDoc->getDeleteMap();

    int maxNum = 0, num = 0;
    DocListUnit* list = NULL;

    while(pNode) {
        IndexTerm* pIndexTerm = getTerm(&cMemPool, pNode->sign);
        if(NULL == pIndexTerm) {
            TERR("get %s %lu error\n", _fieldName, pNode->sign);
            fclose(fp);
            return -1;
        }
        const IndexTermInfo* pTermInfo = pIndexTerm->getTermInfo();
        if(maxNum < pTermInfo->docNum) {
            if(list) delete [] list;
            maxNum = pTermInfo->docNum;
            list = new DocListUnit[maxNum];
        }

        uint32_t docId;
        if(pTermInfo->maxOccNum > 0) {
            num = 0;
            while((docId = pIndexTerm->next()) < INVALID_DOCID) {
                if(del->isDel(docId)) continue;
                int32_t count;
                uint8_t* pocc = pIndexTerm->getOcc(count);
                for(int32_t i = 0; i < count; i++, num++) {
                    list[num].doc_id = docId;
                    list[num].occ = pocc[i];
                }
            }
        } else {
            num = 0;
            while((docId = pIndexTerm->next()) < INVALID_DOCID) {
                if(del->isDel(docId)) continue;
                list[num].doc_id = docId;
                list[num++].occ = 0;
            }
        }

        if(num > 0) {
            fprintf(fp, "term:%lu docNum:%d\n", pNode->sign, num);
            if(pTermInfo->maxOccNum > 0) {
                for(int32_t i = 0; i < num; i++) {
                    fprintf(fp, "%lu(%u) ", pDoc->getNid(list[i].doc_id), list[i].occ);
                }
            } else {
                for(int32_t i = 0; i < num; i++) {
                    fprintf(fp, "%lu ", pDoc->getNid(list[i].doc_id));
                }
            }
        fprintf(fp, "\n");
        }
        
        cMemPool.reset();
        pNode = idx_dict_next(pdict, &pos);
    }
    fclose(fp);
    if (list)  delete [] list;

    return 0;
}

int32_t IndexFieldInc::expend(inc_idx2_header_t *&head, inc_idx1_unit_t *&pind, char *&begin, char *&end)
{
    int32_t  ret         = 0;
    uint32_t valid_count = 0;
    if(head->zipFlag == 0)
    {
        uint32_t baseNum = 0;
        uint32_t preBase = IDX_GET_BASE(INVALID_DOCID);
        if(_maxOccNum == 0)
        {
            uint32_t* p = (uint32_t*)(head + 1);
            for(uint32_t i = 0; i < pind->doc_count; i++)
            {
                if ( _docIdMgr->isDocIdDelUseLess( p[i] ) )  continue;

                if (preBase != IDX_GET_BASE(p[i]))
                {
                    preBase = IDX_GET_BASE(p[i]);
                    baseNum++;
                }
                valid_count++;
            }
            uint32_t zipLen = sizeof(idx2_zip_header_t) + (baseNum + 1) * sizeof(idx2_zip_skip_unit_t) + valid_count * sizeof(uint16_t);
            if(zipLen <= valid_count*sizeof(uint32_t))
            {
                ret = nzip2zip(head, pind, begin, end, baseNum, valid_count);
            }
            else
            {
                ret = nzip2nzip(head, pind, begin, end, valid_count);
            }
        }
        else
        {
            idx2_nzip_unit_t *p = (idx2_nzip_unit_t*)(head + 1);
            for(uint32_t i = 0; i < pind->doc_count; i++)
            {
                if ( _docIdMgr->isDocIdDelUseLess( p[i].doc_id) ) continue;

                if (preBase != IDX_GET_BASE(p[i].doc_id))
                {
                    preBase = IDX_GET_BASE(p[i].doc_id);
                    baseNum++;
                }
                valid_count++;
            }
            uint32_t zipLen = sizeof(idx2_zip_header_t) + (baseNum + 1) * sizeof(idx2_zip_skip_unit_t) + valid_count * sizeof(idx2_zip_list_unit_t);
            if(zipLen <= valid_count*sizeof(idx2_nzip_unit_t))
            {
                ret = nzip2zip(head, pind, begin, end, baseNum, valid_count);
            }
            else
            {
                ret = nzip2nzip(head, pind, begin, end, valid_count);
            }
        }
    }
    else
    {
        idx2_zip_header_t * pheader = (idx2_zip_header_t *)(head + 1);
        idx2_zip_skip_unit_t *pskip = (idx2_zip_skip_unit_t*)(pheader + 1);
        uint32_t baseId     = 0;
        uint32_t docId      = 0;

        if (_maxOccNum > 0) {
            idx2_zip_list_unit_t *plist = (idx2_zip_list_unit_t*)(pskip + _maxIncSkipCnt + 1);
            for(unsigned int i = 0; i < pheader->block_num; i++)
            {
                baseId = (pskip[i].doc_id_base << 16);
                for(unsigned int j = pskip[i].off; j < pskip[i+1].off; j++) {
                    docId = baseId + plist[j].doc_id_diff;

                    if ( _docIdMgr->isDocIdDelUseLess( docId ))  continue;

                    ++valid_count;
                }
            }
        }
        else {
            uint16_t *plist = (uint16_t *)(pskip + _maxIncSkipCnt + 1);
            for(unsigned int i = 0; i < pheader->block_num; i++)
            {
                baseId = (pskip[i].doc_id_base << 16);
                for(unsigned int j = pskip[i].off; j < pskip[i+1].off; j++) {
                    docId = baseId + plist[j];

                    if ( _docIdMgr->isDocIdDelUseLess( docId ))  continue;

                    ++valid_count;
                }
            }
        }
        ret = zip2zip(head, pind, begin, end, valid_count);
    }

    return ret;
}

int32_t IndexFieldInc::nzip2zip(inc_idx2_header_t *&head, inc_idx1_unit_t *&pind, char *&begin, char *&end, uint32_t baseNum, uint32_t validCount)
{
    int32_t pageSize = getPageSize(validCount, 1);
    uint32_t off = 0;
    if(_pIncMemManager->makeNewPage(pageSize, off) < 0)
    {
        TWARN("inc makeNewPage error");
        return KS_ENOMEM;
    }
    char* p1 = _pIncMemManager->offset2Addr(off);
    inc_idx2_header_t* tHead = (inc_idx2_header_t*)p1;
    char* p2 = _pIncMemManager->offset2Addr(pind->pos);
    memcpy(p1, p2, sizeof(inc_idx2_header_t));

    char *zip = p1+sizeof(inc_idx2_header_t);
    char *nzip = p2+sizeof(inc_idx2_header_t);
    idx2_zip_header_t *pheader = (idx2_zip_header_t*)zip;
    idx2_zip_skip_unit_t *pskip = (idx2_zip_skip_unit_t*)(pheader + 1);
    pheader->skip_list_len = (_maxIncSkipCnt + 1) * sizeof(idx2_zip_skip_unit_t);
    pheader->block_num = baseNum;

    uint16_t pre_base = IDX_GET_BASE(INVALID_DOCID);
    uint16_t cur_base = 0;
    uint32_t doc_count = pind->doc_count;
    uint32_t doc_pos  = 0;

    if(_maxOccNum > 0)
    { // has occ info
        idx2_nzip_unit_t     * notzip  = (idx2_nzip_unit_t*)nzip;
        pheader->doc_list_len  = (validCount +1) * sizeof(idx2_zip_list_unit_t);
        idx2_zip_list_unit_t * plist   = (idx2_zip_list_unit_t*)(pskip + _maxIncSkipCnt + 1);

        for (unsigned int i=0, j=0;i<doc_count;i++)
        {
            // build doc_list unit
            if ( _docIdMgr->isDocIdDelUseLess( notzip[i].doc_id ))  continue;

            plist[doc_pos].doc_id_diff = IDX_GET_DIFF(notzip[i].doc_id);
            plist[doc_pos].occ         = notzip[i].occ;

            // build skip_list unit
            cur_base = IDX_GET_BASE(notzip[i].doc_id);
            if (cur_base != pre_base)
            {
                pskip[j].doc_id_base = cur_base;
                pskip[j].off         = doc_pos;
                pre_base             = cur_base;
                j++;
            }
            ++doc_pos;
        }

        if (unlikely(doc_pos != validCount)) {
            TERR("inc expand error, count miss[%u:%u]", validCount, doc_pos);
            return KS_EINVAL;
        }

        // build end doc list
        plist[validCount].doc_id_diff = IDX_GET_DIFF(INVALID_DOCID);
        plist[validCount].occ = 0;
        begin = (char*)(plist + validCount);
    }
    else
    { // null occ info
        uint32_t * notzip  = (uint32_t*)nzip;
        pheader->doc_list_len  = (validCount + 1) * sizeof(uint16_t);
        uint16_t* plist   = (uint16_t*)(pskip + _maxIncSkipCnt + 1);

        for (unsigned int i=0, j=0;i<doc_count;i++)
        {
            if ( _docIdMgr->isDocIdDelUseLess( notzip[i] ))  continue;

            // build doc_list unit
            plist[doc_pos] = IDX_GET_DIFF((notzip[i]));

            // build skip_list unit
            cur_base = IDX_GET_BASE(notzip[i]);
            if (cur_base != pre_base)
            {
                pskip[j].doc_id_base = cur_base;
                pskip[j].off         = doc_pos;
                pre_base             = cur_base;
                j++;
            }
            ++doc_pos;
        }

        if (unlikely(doc_pos != validCount)) {
            TERR("inc expand error, count miss[%u:%u]", validCount, doc_pos);
            return KS_EINVAL;
        }
        // build end doc list
        plist[validCount] = IDX_GET_DIFF(INVALID_DOCID);
        begin = (char*)(plist + validCount);
    }

    // build auxillary block at the end of skiplist
    pskip[baseNum].doc_id_base = IDX_GET_BASE(INVALID_DOCID);
    pskip[baseNum].off         = validCount;

    tHead->pageSize = _pIncMemManager->size(off);
    tHead->endOff = begin - p1;
    tHead->zipFlag = 1;
    end = p1 + tHead->pageSize;
	_pIncMemManager->freeOldPage(head->pageSize, pind->pos);
    head = tHead;
    pind->pos = off;
    pind->doc_count = validCount;

    return KS_SUCCESS;
}

int32_t IndexFieldInc::nzip2nzip(inc_idx2_header_t *&head, inc_idx1_unit_t *&pind, char *&begin, char *&end, uint32_t validCount)
{
    int32_t pageSize = getPageSize(validCount, 0);
    uint32_t off = 0;
    if(_pIncMemManager->makeNewPage(pageSize, off) < 0) {
        TWARN("inc makeNewPage error");
        return KS_ENOMEM;
    }

    char* p1 = _pIncMemManager->offset2Addr(off);
    inc_idx2_header_t* tHead = (inc_idx2_header_t*)p1;
    char* p2 = _pIncMemManager->offset2Addr(pind->pos);
    memcpy(p1, p2, sizeof(inc_idx2_header_t));

    char *newNzip = p1 + sizeof(inc_idx2_header_t);
    char *oldNzip = p2 + sizeof(inc_idx2_header_t);

    uint32_t doc_count = pind->doc_count;
    uint32_t doc_pos  = 0;

    if (_maxOccNum > 0) {
        idx2_nzip_unit_t * newNotzip = (idx2_nzip_unit_t*)newNzip;
        idx2_nzip_unit_t * oldNotzip = (idx2_nzip_unit_t*)oldNzip;

        for (unsigned int i=0;i<doc_count;i++)
        {
            // build doc_list unit
            if ( _docIdMgr->isDocIdDelUseLess( oldNotzip[i].doc_id ))  continue;

            newNotzip[doc_pos++] = oldNotzip[i];
        }

        if (unlikely(doc_pos != validCount)) {
            TERR("inc expand error, count miss[%u:%u]", validCount, doc_pos);
            return KS_EINVAL;
        }

        // build end doc list
        newNotzip[validCount].doc_id = INVALID_DOCID;
        newNotzip[validCount].occ    = 0;
        begin = (char*)(newNotzip + validCount);
    }
    else {
        uint32_t * newNotzip = (uint32_t *)newNzip;
        uint32_t * oldNotzip = (uint32_t *)oldNzip;

        for (unsigned int i=0;i<doc_count;i++)
        {
            // build doc_list unit
            if ( _docIdMgr->isDocIdDelUseLess( oldNotzip[i] ))  continue;

            newNotzip[doc_pos++] = oldNotzip[i];
        }

        if (unlikely(doc_pos != validCount)) {
            TERR("inc expand error, count miss[%u:%u]", validCount, doc_pos);
            return KS_EINVAL;
        }

        // build end doc list
        newNotzip[validCount] = INVALID_DOCID;
        begin = (char*)(newNotzip + validCount);
    }

    // build auxillary block at the end of skiplist
    tHead->pageSize = _pIncMemManager->size(off);
    tHead->endOff   = begin - p1;
    tHead->zipFlag  = 0;
    end = p1 + tHead->pageSize;

	_pIncMemManager->freeOldPage(head->pageSize, pind->pos);
    head = tHead;
    pind->pos = off;
    pind->doc_count = validCount;
    return KS_SUCCESS;
}

int32_t IndexFieldInc::zip2zip(inc_idx2_header_t *&head, inc_idx1_unit_t *&pind, char *&begin, char *&end, uint32_t validCount)
{
    int32_t pageSize = getPageSize(validCount, 1);
    uint32_t off = 0;
    if(_pIncMemManager->makeNewPage(pageSize, off) < 0) {
        TWARN("inc makeNewPage error");
        return KS_ENOMEM;
    }

    char* p1 = _pIncMemManager->offset2Addr(off);
    char* p2 = _pIncMemManager->offset2Addr(pind->pos);
    memcpy(p1, p2, sizeof(inc_idx2_header_t));

    char *newZip = p1 + sizeof(inc_idx2_header_t);
    char *oldZip = p2 + sizeof(inc_idx2_header_t);

    inc_idx2_header_t * tHead = (inc_idx2_header_t *)p1;

    idx2_zip_header_t    *pNewHeader = (idx2_zip_header_t*)newZip;
    pNewHeader->skip_list_len = (_maxIncSkipCnt + 1) * sizeof(idx2_zip_skip_unit_t);

    idx2_zip_header_t    *pOldHeader = (idx2_zip_header_t*)oldZip;
    idx2_zip_skip_unit_t *pOldSkip   = (idx2_zip_skip_unit_t*)(pOldHeader + 1);
    idx2_zip_skip_unit_t *pNewSkip   = (idx2_zip_skip_unit_t*)(pNewHeader + 1);

    uint32_t baseNum    = pOldHeader->block_num;

    uint32_t baseId     = 0;
    uint32_t docId      = 0;
    uint32_t baseDocCnt = 0;

    uint32_t listPos    = 0;    // 扩展后的doclist个数
    uint32_t basePos    = 0;    // 扩展后的base block个数
    uint32_t beginPos   = INVALID_DOCID;

    if (_maxOccNum > 0) {
        idx2_zip_list_unit_t * pOldList = (idx2_zip_list_unit_t*)(pOldSkip + _maxIncSkipCnt + 1);
        idx2_zip_list_unit_t * pNewList = (idx2_zip_list_unit_t*)(pNewSkip + _maxIncSkipCnt + 1);

        for(unsigned int i = 0; i < baseNum; i++)
        {
            baseDocCnt = 0;
            beginPos   = INVALID_DOCID;
            baseId     = (pOldSkip[i].doc_id_base << 16);
            for(unsigned int j = pOldSkip[i].off; j < pOldSkip[i+1].off; j++) {
                docId = baseId + pOldList[j].doc_id_diff;

                if ( _docIdMgr->isDocIdDelUseLess( docId ))  continue;

                if (beginPos == INVALID_DOCID) {
                    beginPos = listPos;
                }
                pNewList[listPos++] = pOldList[j];
                baseDocCnt++;
            }

            if (baseDocCnt == 0) {
                continue;
            }
            pNewSkip[basePos].doc_id_base = pOldSkip[i].doc_id_base;
            pNewSkip[basePos].off         = beginPos;
            basePos++;
        }
        pNewList[listPos].doc_id_diff = IDX_GET_DIFF(INVALID_DOCID);
        pNewList[listPos].occ         = 0;
        begin = (char *)(pNewList + listPos);
        pNewHeader->doc_list_len = (listPos + 1) * sizeof(idx2_zip_list_unit_t);
    }
    else {
        uint16_t * pOldList = (uint16_t *)(pOldSkip + _maxIncSkipCnt + 1);
        uint16_t * pNewList = (uint16_t *)(pNewSkip + _maxIncSkipCnt + 1);
        for(unsigned int i = 0; i < baseNum; i++)
        {
            baseDocCnt = 0;
            beginPos   = INVALID_DOCID;
            baseId = (pOldSkip[i].doc_id_base << 16);
            for(unsigned int j = pOldSkip[i].off; j < pOldSkip[i+1].off; j++) {
                docId = baseId + pOldList[j];

                if ( _docIdMgr->isDocIdDelUseLess( docId ))  continue;

                if (beginPos == INVALID_DOCID) {
                    beginPos = listPos;
                }
                pNewList[listPos++] = pOldList[j];
                baseDocCnt++;
            }

            if (baseDocCnt == 0) {
                continue;
            }
            pNewSkip[basePos].doc_id_base = pOldSkip[i].doc_id_base;
            pNewSkip[basePos].off         = beginPos;
            basePos++;
        }
        pNewList[listPos] = IDX_GET_DIFF(INVALID_DOCID);
        begin = (char *)(pNewList + listPos);
        pNewHeader->doc_list_len = (listPos + 1) * sizeof(uint16_t);
    }

    pNewSkip[basePos].doc_id_base = IDX_GET_BASE(INVALID_DOCID);
    pNewSkip[basePos].off         = listPos;
    pNewHeader->block_num = basePos;

    tHead->pageSize = _pIncMemManager->size(off);
    tHead->endOff   = begin - p1;
    tHead->zipFlag  = 1;
    end = p1 + tHead->pageSize;

	_pIncMemManager->freeOldPage(head->pageSize, pind->pos);
    head = tHead;
    pind->pos = off;
    pind->doc_count = listPos;

    return KS_SUCCESS;
}

/*
int32_t IndexFieldInc::otherExpend(inc_idx2_header_t *&head, inc_idx1_unit_t *&pind, char *&begin, char *&end, uint8_t toZip, uint32_t validCount)
{
    int32_t pageSize = getPageSize(pind->doc_count, toZip);
    uint32_t off = 0;
    if(_pIncMemManager->makeNewPage(pageSize, off) < 0) {
        TWARN("inc makeNewPage error");
        return KS_ENOMEM;
    }
    char* p1 = _pIncMemManager->offset2Addr(off);
    inc_idx2_header_t* tHead = (inc_idx2_header_t*)p1;
    char* p2 = _pIncMemManager->offset2Addr(pind->pos);

    // old 使用内存大小
    int32_t useSize = begin - p2;
    // 拷贝原来的内存
    memcpy(p1, p2, useSize);
    begin = p1 + useSize;
    end = p1 + pageSize;
    //
    tHead->pageSize = pageSize;
    tHead->endOff = useSize;
    _pIncMemManager->freeOldPage(head->pageSize, pind->pos);
    // 切换头信息
    head = tHead;
    pind->pos = off;
    pind->doc_count = validCount;

    return KS_SUCCESS;
}
*/

int32_t IndexFieldInc::getPageSize(uint32_t currDocCnt, uint8_t zipFlag)
{
    int32_t ret = 0;
    uint32_t docCnt = (currDocCnt == 0)? 32 : (uint32_t)((currDocCnt+1)*(1.3f));
    if(zipFlag == 0)
    {
        if(_maxOccNum == 0)
        {
            ret = sizeof(inc_idx2_header_t) + sizeof(uint32_t) * docCnt;
        }
        else
        {
            ret = sizeof(inc_idx2_header_t) + sizeof(idx2_nzip_unit_t) * docCnt;
        }
    }
    else
    {
        if(_maxOccNum == 0)
        {
            ret = sizeof(inc_idx2_header_t) + sizeof(idx2_zip_header_t) + sizeof(idx2_zip_skip_unit_t)*(_maxIncSkipCnt+1) + sizeof(uint16_t) * docCnt;
        }
        else
        {
            ret = sizeof(inc_idx2_header_t) + sizeof(idx2_zip_header_t) + sizeof(idx2_zip_skip_unit_t)*(_maxIncSkipCnt+1) + sizeof(idx2_zip_skip_unit_t) * docCnt;
        }
    }

    return ret;
}

uint32_t IndexFieldInc::getDocNumByEndOff(uint32_t endOff, int zipFlag)
{
    uint32_t docNum = 0;
    if (zipFlag == 0)
    {
        if (_maxOccNum == 0)
        {
            docNum = (endOff - sizeof(inc_idx2_header_t)) / sizeof(uint32_t);
        }
        else
        {
            docNum = (endOff - sizeof(inc_idx2_header_t)) / sizeof(idx2_nzip_unit_t);

        }
    }
    else {
        if (_maxOccNum == 0)
        {
            docNum = (endOff - sizeof(inc_idx2_header_t) - sizeof(idx2_zip_header_t) - sizeof(idx2_zip_skip_unit_t) * (_maxIncSkipCnt+1)) / sizeof(uint16_t);
        }
        else
        {
            docNum = (endOff - sizeof(inc_idx2_header_t) - sizeof(idx2_zip_header_t) - sizeof(idx2_zip_skip_unit_t) * (_maxIncSkipCnt+1)) / sizeof(idx2_zip_list_unit_t);
        }
    }
    return docNum;
}


/**
 * 遍历当前字段的所有term
 *
 * @param termSign    返回的当前term的签名
 *
 * @return 0：成功;   -1：失败
 */
int IndexFieldInc::getFirstTerm( uint64_t &termSign )
{
    idict_node_t * pNode = NULL;

    _travelDict = _invertedDict;

    pNode = idx_dict_first( _travelDict, &_travelPos );

    if ( pNode == NULL )
    {
        _travelDict = _bitmapDict;

        pNode = idx_dict_first( _travelDict, &_travelPos );

        if ( pNode == NULL ) return -1;          // 没有找到任何term
    }

    termSign = pNode->sign;
    return 0;
}


int IndexFieldInc::getNextTerm( uint64_t &termSign )
{
    idict_node_t * pNode = NULL;

    pNode = idx_dict_next( _travelDict, &_travelPos );

    if ( _travelDict == _invertedDict )
    {
        if ( pNode != NULL)
        {
            termSign = pNode->sign;
            return 0;
        }

        _travelDict = _bitmapDict;
        pNode = idx_dict_first( _travelDict, &_travelPos );
    }

    while ( pNode != NULL && idx_dict_find( _invertedDict, pNode->sign ) != NULL )
    {
        pNode = idx_dict_next( _travelDict, &_travelPos );
    }

    if ( pNode == NULL )  return -1;

    termSign = pNode->sign;
    return 0;
}

INDEX_LIB_END


