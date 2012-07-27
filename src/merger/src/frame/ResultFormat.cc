#include "ResultFormat.h"
#include "Document.h"
#include "statistic/StatisticResult.h"
#include "sort/SortResult.h"
#include "util/Log.h"
#include "DebugInfo.h"
#include <stdlib.h>
#include <string.h>
#include "KSResult.pb.h"

const int32_t g_max_docsfound = 5000;

uint32_t genStatisticXmlOutPut(
        const statistic::StatisticResult &statisticResult,
        char *output, uint32_t out_size);

uint32_t genStatisticV3OutPut(
        const statistic::StatisticResult &statisticResult, 
        char *output, uint32_t out_size);

//internal interface, so little check here
void travelDocument(commdef::ClusterResult *pClusterResult,uint32_t &dirty, int &fieldCount, 
        int &totalfieldCount, uint32_t &fieldCount_offset, uint32_t *countarray)
{
    if (countarray == NULL) return;
    int tmpfieldCount = 0;
    const char *pTmp = NULL;
    for (int32_t i = 0; i < pClusterResult->_nDocsReturn; i++)
    {
        if (pClusterResult->_ppDocuments[i] == NULL)
        {
            dirty++;
            countarray[i] = 0;
            if (fieldCount == 0)
                fieldCount_offset++;
        }
        else
        {
            tmpfieldCount = pClusterResult->_ppDocuments[i]->getFieldCount();
            for(int32_t j = 0; j < tmpfieldCount; j++)
            {
                pTmp = pClusterResult->_ppDocuments[i]->getField(j)->getName();
            }
            countarray[i] = tmpfieldCount;
            if (unlikely(tmpfieldCount==0))
            {
                dirty++;
                if (fieldCount==0)
                    fieldCount_offset++;
                continue;
            }
            if (fieldCount == 0)
                fieldCount = tmpfieldCount;
            else if (tmpfieldCount != fieldCount)
            {
                dirty++;	
                continue;
            }
            totalfieldCount += fieldCount;
        }
    }
    return;
}

void * ResultFormat::alloc_memory(uint32_t size) {
    if(size == 0) return NULL;
    if(_heap) return NEW_VEC(_heap, char, size);
    return malloc(size);
}

void ResultFormat::free_memory(void * ptr) {
    if(ptr && !_heap) ::free(ptr);
}

int32_t ResultFormat::format(const result_container_t & container,
        char * & out_data, uint32_t & out_size) {
    uint32_t max_size;
    max_size = 1048576L; //default 1024L * 1024L = 1MB
    while(true) {
        out_data = (char*)alloc_memory(max_size);
        if(unlikely(!out_data)) return KS_ENOMEM;
        out_size = doFormat(container, out_data, max_size);
        if(out_size < max_size) break;
        free_memory(out_data);
        max_size = max_size * 2;
    }
    return KS_SUCCESS;
}

/***********************ResultFormatDefault**************************/
/**************************ResultFormatV3**************************/

uint32_t ResultFormatV3::doFormat(const result_container_t & container,
        char * out_data, uint32_t out_size) {
    static const char * ERRSTR = VERSION_3_0""STOP_STR""QUERY_FAILURE""STOP_STR"0"STOP_STR""DOC_END_STR;
    static const uint32_t ERRLEN = strlen(ERRSTR);
    char * ptr = out_data ? out_data : SG_TMP;
    const char *pTmp = NULL;
    uint32_t left = out_size;
    uint32_t total = 0;
    uint32_t len, i, n, dirty = 0;
    uint32_t fieldCount_offset = 0;
    int fieldCount = 0;
    int tmpfieldCount = 0;
    int totalfieldCount = 0;
    int j = 0;
    commdef::ClusterResult * pClusterResult = container.pClusterResult;
    if(unlikely(!pClusterResult)) {
        if(out_size < ERRLEN) return out_size + 1;
        if(out_data) memcpy(out_data, ERRSTR, ERRLEN);
        return ERRLEN;
    }
    DebugInfo *pDebugInfo = pClusterResult->_pDebugInfo;
    //Modified DocsFound
    if (pClusterResult->_nDocsFound < g_max_docsfound) {
        pClusterResult->_nEstimateDocsFound = pClusterResult->_nDocsFound;
    }
    uint32_t *_fieldcountarray = new uint32_t [pClusterResult->_nDocsReturn];
    memset((char *)_fieldcountarray, 0x0, sizeof(uint32_t)*pClusterResult->_nDocsReturn);
    if (pClusterResult->_ppDocuments != NULL ) {
        travelDocument(pClusterResult,dirty,fieldCount,totalfieldCount,fieldCount_offset,_fieldcountarray); 
    }
    len = snprintf(ptr, left, "%s%c%s%c%d%c",
            VERSION_3_0, STOP, QUERY_SUCCESS, STOP, container.cost, STOP);
    if(len + total > out_size)
    {
        delete [] _fieldcountarray;
        return out_size+1;	
    }
    MOVE_CURSOR(ptr, left, total, len);
    len = snprintf(ptr, left, "%s%c%d%c%d%c%d%c%d%c%d%c%d%c%d%c",
            SECTION_CONSTANT, STOP, 
            6, STOP, 
            pClusterResult->_nEstimateDocsFound, STOP, 
            pClusterResult->_nDocsSearch, STOP, 
            pClusterResult->_nDocsRestrict, STOP, 
            pClusterResult->_nDocsReturn - dirty, STOP,
            1, STOP, 
            1, STOP);
    if(len + total > out_size)
    {
        delete [] _fieldcountarray;
        return out_size+1;
    }
    MOVE_CURSOR(ptr, left, total, len);
    char *echoPtr = NULL;
    char *basePtr = NULL;
    len = snprintf(ptr, left, "%s%c%d%c%d%c%s%c%d%c",
            SECTION_ECHOKEY, STOP, 3, STOP, 0, STOP, 
            (basePtr ? basePtr : ""), STOP,0,STOP);
    if(basePtr) free_memory(basePtr);
    if(len + total > out_size)
    {
        delete [] _fieldcountarray;
        return out_size + 1;
    }
    MOVE_CURSOR(ptr, left, total, len);
    //TODO gen statistic output
    int32_t statSize = genStatisticV3OutPut(*pClusterResult->_pStatisticResult, ptr, left);
    if(statSize + total > out_size) {
        delete [] _fieldcountarray;
        return out_size+1;
    }
    MOVE_CURSOR(ptr, left, total, statSize);

    if (pClusterResult->_nDocsReturn > dirty && pClusterResult->_ppDocuments != NULL) {
        n = 1 + totalfieldCount + fieldCount;
        int fieldCountAppend = fieldCount;
        len = snprintf(ptr, left, "%s%c%d%c%d%c", SECTION_DATA, STOP, n, STOP, fieldCountAppend, STOP);
        if(len + total > out_size)
        {
            delete [] _fieldcountarray;
            return out_size + 1;
        }
        MOVE_CURSOR(ptr, left, total, len);
        for(j=0; j < fieldCount; j++) {
            pTmp = pClusterResult->_ppDocuments[fieldCount_offset]->getField(j)->getName();
            len = snprintf(ptr, left, "%s%c", pTmp, STOP);
            if(len + total > out_size)
            {
                delete [] _fieldcountarray;
                return out_size + 1;
            }
            MOVE_CURSOR(ptr, left, total, len);
        }
        for( i = 0; i < pClusterResult->_nDocsReturn; i++ ) {
            long lRetTmp;
            Document *pDocument = pClusterResult->_ppDocuments[i];
            if(pDocument == NULL) continue;
            if (_fieldcountarray[i] != fieldCount) continue;
            for(j=0; j < fieldCount; j++) {
                pTmp = pDocument->getField(j)->getName();
                len = snprintf(ptr, left, "%s%c", 
                        pDocument->getField(j)->getValue(), STOP);
                if(len + total > out_size)
                {
                    delete [] _fieldcountarray;
                    return out_size + 1;
                }
                MOVE_CURSOR(ptr, left, total, len);
            }
        }//for
    }//if (pClusterResult->_ppDocuments != NULL)
    delete [] _fieldcountarray;
    if(left < 2) return out_size + 1;
        ptr[0] = STOP;
        ptr[1] = STOP;
        MOVE_CURSOR(ptr, left, total, 2);
        return total;
    }

    uint32_t ResultFormatXML::doFormat(const result_container_t & container,
            char * out_data, uint32_t out_size) {
        return _ref.doFormat(container, out_data, out_size);
    }

#include "mxml.h"
#include <string.h>
#include <set>
#ifdef UNIT_TEST
#include <stdio.h>
#include <iostream>
#endif //UNIT_TEST

    //==========================ResultFormatXMLConfig========================

    class ResultFormatXMLConfig {
        public:
            ResultFormatXMLConfig();
            ~ResultFormatXMLConfig();
        public:
                int32_t parse(const char * pXmlConfigFile);
                bool getOutputMetainfo(void) { return m_bOutputMetainfo; }
                void setOutputMetainfo(bool bOutput) { m_bOutputMetainfo = bOutput; }
                bool getOutputEchoKey(void) { return m_bOutputEchoKey; }
                void setOutputEchoKey(bool bOutput) { m_bOutputEchoKey = bOutput; }
                void addKey2Metainfo(const char *pKey);
                bool isConfigedInMetainfo(const char *pKey);
                bool getOutputExtKeySet(void) { return m_bOutputExtKeySet; }
                void setOutputExtKeySet(bool bOutput) { m_bOutputExtKeySet = bOutput; }
                void addKey2ExtKeySet(const char *pKey);
                bool isConfigedInExtKeySet(const char *pKey);
                bool getOutputStat(void) { return m_bOutputStat; }
                void setOutputStat(bool bOutput) { m_bOutputStat = bOutput; }
                bool getOutputData(void) { return m_bOutputData; }
                void setOutputData(bool bOutput) { m_bOutputData = bOutput; }
                bool getOutputRank(void) { return m_bOutputRank; }
                void setOutputRank(bool bOutput) { m_bOutputRank = bOutput; }
                bool getOutputUniq(void) { return m_bOutputUniq; }
                bool setOutputUniq(bool bOutput) { m_bOutputUniq = bOutput; }
                const char * getName() { return m_szXmlName; }
#ifdef UNIT_TEST
                void unittestDumpContent(void) {
                    std::set<std::string>::iterator it;
                    fprintf(stderr, "xml config name:%s\n", m_szXmlName);
                    std::cout << "m_bOutputMetainfo:" << m_bOutputMetainfo << std::endl;
                    std::cout << ">>key in metainfo<<" << std::endl;
                    for(it = m_metaInfo.begin(); it != m_metaInfo.end(); it++) {
                        fprintf(stderr, "\t%s\n", (*it).c_str());
                    }
                    std::cout << "m_bOutputExtKeySet:" << m_bOutputExtKeySet << std::endl;
                    std::cout << ">>key in extkeyset<<" << std::endl;
                    for(it = m_extKeySet.begin(); it != m_extKeySet.end(); it++) {
                        fprintf(stderr, "\t%s\n", (*it).c_str());
                    }
                    std::cout << "m_bOutputStat:" << m_bOutputStat << std::endl;
                    std::cout << "m_bOutputData:" << m_bOutputData << std::endl;
                    std::cout << "m_bOutputUniq:" << m_bOutputUniq << std::endl;
                    std::cout << "m_bOutputRank" << m_bOutputRank << std::endl;
                    std::cout.flush();
                }
#endif
            private:
                char *m_szXmlName;			
                bool m_bOutputMetainfo;     
                std::set<std::string> m_metaInfo;
                bool m_bOutputExtKeySet;     
                std::set<std::string> m_extKeySet;
                bool m_bOutputStat;         
                bool m_bOutputEchoKey;      
                bool m_bOutputData;         
                bool m_bOutputRank;         
                bool m_bOutputUniq;         
        };

        ResultFormatXMLConfig::ResultFormatXMLConfig() 
            : m_szXmlName(NULL),
            m_bOutputMetainfo(true),
            m_bOutputExtKeySet(true),
            m_bOutputStat(true),
            m_bOutputData(true),
            m_bOutputRank(true),
            m_bOutputUniq(true) {
            }

        ResultFormatXMLConfig::~ResultFormatXMLConfig() {
            if(m_szXmlName) ::free(m_szXmlName);
        }

        int32_t ResultFormatXMLConfig::parse(const char *pXmlConfigFile) {
            static const char *XMLCONFIG = "xmlconfig";
            FILE *fp = NULL;
            const char *szAttr = NULL;
            mxml_node_t *pXMLTree = NULL;
            mxml_node_t *pXmlConfigNode = NULL;
            if ((fp = fopen(pXmlConfigFile, "r")) == NULL) {
                return KS_EFAILED;
            }
            pXMLTree = mxmlLoadFile(NULL, fp, MXML_NO_CALLBACK);
            fclose(fp);
            if(pXMLTree == NULL) {
                return KS_EFAILED;
            }
            pXmlConfigNode = mxmlFindElement(pXMLTree, pXMLTree, 
                    XMLCONFIG, NULL, NULL, MXML_DESCEND);
            szAttr = mxmlElementGetAttr(pXmlConfigNode, "name");
            if (szAttr == NULL) {
            } else if (strlen(szAttr) == 0) {
            } else {
                m_szXmlName = strdup(szAttr);
                if(unlikely(!m_szXmlName)) {
                    if(pXMLTree) mxmlDelete(pXMLTree);
                    return KS_ENOMEM;
                }
                int32_t nMetainfoFieldSerial = 0;
                mxml_node_t *pMetaInfoNode = 
                    mxmlFindElement(pXmlConfigNode, pXmlConfigNode, 
                            "metainfo", NULL, NULL, MXML_DESCEND);
                szAttr = mxmlElementGetAttr(pMetaInfoNode, "output");
                if (szAttr != NULL && 
                        (strcasecmp("n", szAttr) == 0 || 
                         strcasecmp("no", szAttr) == 0)) {
                    setOutputMetainfo(false);
                }
                mxml_node_t *pFieldNode = 
                    mxmlFindElement(pMetaInfoNode, pXmlConfigNode, 
                            "field", NULL, NULL, MXML_DESCEND);
                while(pFieldNode != NULL&&pFieldNode->parent == pMetaInfoNode) {
                    nMetainfoFieldSerial++;
                    szAttr = mxmlElementGetAttr(pFieldNode, "name");
                    if (szAttr == NULL) {
                    } else if (strlen(szAttr) == 0) {
                        pFieldNode = mxmlFindElement(pFieldNode, pXmlConfigNode, 
                                "field", NULL, NULL, MXML_DESCEND);
                        continue;
                    }
                    addKey2Metainfo(szAttr);
                    pFieldNode = mxmlFindElement(pFieldNode, pXmlConfigNode, 
                            "field", NULL, NULL, MXML_DESCEND);
                }
                mxml_node_t *pEchoKeyNode = 
                    mxmlFindElement(pXmlConfigNode, pXmlConfigNode, 
                            "echokey", NULL, NULL, MXML_DESCEND);
                szAttr = mxmlElementGetAttr(pEchoKeyNode, "output");
                if (szAttr != NULL && 
                        (strcasecmp("n", szAttr) == 0 ||
                         strcasecmp("no", szAttr) == 0)) { 
                    setOutputEchoKey(false);
                } else {
                    setOutputEchoKey(true);
                }
                mxml_node_t *pExtKeySetNode = 
                    mxmlFindElement(pXmlConfigNode, pXmlConfigNode, 
                            "extkeyset", NULL, NULL, MXML_DESCEND);
                szAttr = mxmlElementGetAttr(pExtKeySetNode, "output");
                if (szAttr != NULL && 
                        (strcasecmp("n", szAttr) == 0 ||  
                         strcasecmp("no", szAttr) == 0)) {
                    setOutputExtKeySet(false);
                }
                int32_t nExtKeySetFieldSerial = 0;
                pFieldNode = mxmlFindElement(pExtKeySetNode, pXmlConfigNode, 
                        "field", NULL, NULL, MXML_DESCEND);
                while (pFieldNode != NULL&&pFieldNode->parent == pExtKeySetNode) {
                    nExtKeySetFieldSerial++;
                    szAttr = mxmlElementGetAttr(pFieldNode, "name");
                    if (szAttr == NULL) {
                        pFieldNode = mxmlFindElement(pFieldNode, pXmlConfigNode, 
                                "field", NULL, NULL, MXML_DESCEND);
                        continue;
                    }
                    addKey2ExtKeySet(szAttr);
                    pFieldNode = mxmlFindElement(pFieldNode, pXmlConfigNode, 
                            "field", NULL, NULL, MXML_DESCEND);
                }
                mxml_node_t *pStatNode = 
                    mxmlFindElement(pXmlConfigNode, pXmlConfigNode, 
                            "statset", NULL, NULL, MXML_DESCEND);
                szAttr = mxmlElementGetAttr(pStatNode, "output");
                if (szAttr != NULL && 
                        (strcasecmp("n", szAttr) == 0 ||  
                         strcasecmp("no", szAttr) == 0)) {
                    setOutputStat(false);
                }
                mxml_node_t *pDataNode = 
                    mxmlFindElement(pXmlConfigNode, pXmlConfigNode, 
                            "docele", NULL, NULL, MXML_DESCEND);
                szAttr = mxmlElementGetAttr(pDataNode, "output");
                if (szAttr != NULL && 
                        (strcasecmp("n", szAttr) == 0 ||  
                         strcasecmp("no", szAttr) == 0)) {
                    setOutputData(false);
                }
                szAttr = mxmlElementGetAttr(pDataNode, "outputrank");
                if (szAttr != NULL && 
                        (strcasecmp("n", szAttr) == 0 ||  
                         strcasecmp("no", szAttr) == 0)) {
                    setOutputRank(false);
                }
                szAttr = mxmlElementGetAttr(pDataNode, "outputuniq");
                if (szAttr != NULL && 
                        (strcasecmp("n", szAttr) == 0 ||  
                         strcasecmp("no", szAttr) == 0)) {
                    setOutputUniq(false);
                }
            }
            if (pXMLTree) mxmlDelete(pXMLTree);
            return KS_SUCCESS;
        }

        void ResultFormatXMLConfig::addKey2Metainfo(const char *pKey) {
            m_metaInfo.insert(pKey);
        }

        bool ResultFormatXMLConfig::isConfigedInMetainfo(const char *pKey) {
            return m_metaInfo.find(std::string(pKey)) != m_metaInfo.end();
        }

        void ResultFormatXMLConfig::addKey2ExtKeySet(const char *pKey) {
            m_extKeySet.insert(pKey);
        }

        bool ResultFormatXMLConfig::isConfigedInExtKeySet(const char *pKey) {
            return m_extKeySet.find(std::string(pKey)) != m_extKeySet.end();
        }

        //==========================ResultFormatXMLRefg========================

        ResultFormatXMLRef::ResultFormatXMLRef() : _conf(NULL) {
        }

        ResultFormatXMLRef::~ResultFormatXMLRef() {
            if(_conf) delete _conf;
        }

        const char * ResultFormatXMLRef::getName() {
            return _conf ? _conf->getName() : NULL;
        }

        int32_t ResultFormatXMLRef::initialize(const char * path) {
            int32_t ret;
            if(_conf) {
                TWARN("this result format has been inited yet.");
                return KS_SUCCESS;
            }
            _conf = new (std::nothrow) ResultFormatXMLConfig();
            if(unlikely(!_conf)) return KS_ENOMEM;
            ret = _conf->parse(path);
            if(unlikely(ret != KS_SUCCESS)) {
                delete _conf;
                _conf = NULL;
                TERR("init xml result format by `%s' failed.", 
                        SAFE_STRING(path));
                return ret;
            }
#ifdef UNIT_TEST
            _conf->unittestDumpContent();
#endif //UNIT_TEST
            return KS_SUCCESS;
        }

        inline long Utf8Width(const char *szStr, merger::ESupportCoding eEncoding) {
            if (eEncoding == merger::sc_GBK) {
                if (szStr[0] < 0 && szStr[1])
                    return 2;
                return 1;
            }
            else if (eEncoding == merger::sc_UTF8) {
                if ((szStr[0] & 0x80) == 0)
                    return 1;
                unsigned char ch = 0x80;
                long nWidth, i;
                for (nWidth = 0; nWidth < 8 && (ch & szStr[0]); nWidth++, ch >>= 1);
                for (i = 0; i < nWidth && szStr[i] < 0; i++);
                return i;
            }
            return 1;
        }

        char *ResultFormatXMLRef::convertRestrictedChar(const char *pchStr, merger::ESupportCoding eEncodingType)
        {
            long lStrlen = strlen(pchStr);
            char *pWrite = (char *)malloc(lStrlen*6+1);
            char *pchWriteBak = pWrite;
            const char *pchStart = pchStr;
            const char *pchEnd = pchStart + lStrlen;
            long lLen = 0;
            while(pchStart < pchEnd) {
                //<!ENTITY lt     "&#38;#60;">
                //<!ENTITY gt     "&#62;">
                //<!ENTITY amp    "&#38;#38;">
                //<!ENTITY apos   "&#39;">
                //<!ENTITY quot   "&#34;">
                switch (*pchStart) {
                    case 38:
                    case 60:
                    case 62:
                    case 39:
                    case 34:
                        lLen = sprintf(pWrite, "&#x%x;", *pchStart);
                        pWrite += lLen;
                        pchStart++;
                        break;
                    case 0:
                        pchStart++;
                        break;
                    default:
                        lLen = Utf8Width(pchStart, eEncodingType);
                        if (lLen == 1)
                            *pWrite++ = *pchStart++;
                        else {
                            memcpy(pWrite, pchStart, lLen);
                            pWrite += lLen;
                            pchStart += lLen;
                        }
                }
            }
            *pWrite = '\0';
            return pchWriteBak;
        }

        uint32_t ResultFormatXMLRef::doFormat(const result_container_t & container,
                char * out_data, uint32_t out_size) {
            static const char * ERRSTR =
                "<?xml version=\"1.0\" encoding=\"GB2312\" standalone=\"yes\" ?>\n\
                <resultset xmlns=\"\">\n\
                <version>"VERSION_4_0"</version>\n\
                <status>error</status>\n\
                <metainfo>\n\
                <errmsg>ERROR</errmsg>\n\
                </metainfo>\n\
                </resultset>\n";
            static const uint32_t ERRLEN = strlen(ERRSTR);
            commdef::ClusterResult *pClusterResult = container.pClusterResult;
            if(unlikely(!pClusterResult)) {
                if(out_size < ERRLEN) return out_size + 1;
                if(out_data) memcpy(out_data, ERRSTR, ERRLEN);
                return ERRLEN;
            }
            DebugInfo *pDebugInfo = pClusterResult->_pDebugInfo;
            //Modified DocsFound
            if (pClusterResult->_nDocsFound < g_max_docsfound) {
                pClusterResult->_nEstimateDocsFound = pClusterResult->_nDocsFound;
            }
            int32_t len = 0, i, j;
            uint32_t dirty = 0;
            int fieldCount = 0;
            int totalfieldCount = 0;
            uint32_t fieldCount_offset = 0;
            const char *pTmp;
            uint32_t total_size, left;
            char * ptr;
            total_size = 0;
            left = out_size;
            ptr = out_data ? out_data : SG_TMP;
            uint32_t *_fieldcountarray = new uint32_t [pClusterResult->_nDocsReturn];
            memset((char *)_fieldcountarray, 0x0, sizeof(uint32_t)*pClusterResult->_nDocsReturn);
            if(pClusterResult->_ppDocuments != NULL) {
                travelDocument(pClusterResult,dirty,fieldCount,totalfieldCount,fieldCount_offset,_fieldcountarray);
            }
            delete [] _fieldcountarray;
            len = snprintf(ptr, left, 
                    "<?xml version=\"1.0\" encoding=\"GBK\" standalone=\"yes\" ?>\n\
                    <resultset xmlns=\"\">\n\
                    <version>%s</version>\n\
                    <status>ok</status>\n", VERSION_4_0);
            if(len + total_size > out_size) return out_size + 1;
            MOVE_CURSOR(ptr, left, total_size, len);
            if(!_conf) {
                len = snprintf(ptr, left,  "<metainfo>\n\
                        <miele key=\"searched\" val=\"%d\" />\n\
                        <miele key=\"returned\" val=\"%d\" />\n\
                        <miele key=\"found\" val=\"%d\" />\n\
                        <miele key=\"restrict\" val=\"%d\" />\n\
                        <miele key=\"prevpage\" val=\"%d\" />\n\
                        <miele key=\"nextpage\" val=\"%d\" />\n\
                        <miele key=\"time\" val=\"%.5d\" />\n\
                        </metainfo>\n",
                        pClusterResult->_nDocsSearch,
                        pClusterResult->_nDocsReturn - dirty,
                        pClusterResult->_nEstimateDocsFound,
                        pClusterResult->_nDocsRestrict,
                        1,
                        1,
                        container.cost);
                if(len + total_size > out_size) return out_size + 1;
                MOVE_CURSOR(ptr, left, total_size, len);
            } else if(_conf->getOutputMetainfo()) {
                len = snprintf(ptr, left, "<metainfo>\n");
                if(len + total_size > out_size) return out_size + 1;
                MOVE_CURSOR(ptr, left, total_size, len);
                if(_conf->isConfigedInMetainfo("searched")) {
                    len = snprintf(ptr, left, 
                            "<miele key=\"searched\" val=\"%d\" />\n", 
                            pClusterResult->_nDocsSearch);
                    if(len + total_size > out_size) return out_size + 1;
                    MOVE_CURSOR(ptr, left, total_size, len);
                }
                if(_conf->isConfigedInMetainfo("returned")) {
                    len = snprintf(ptr, left, 
                            "<miele key=\"returned\" val=\"%d\" />\n", 
                            pClusterResult->_nDocsReturn - dirty);
                    if(len + total_size > out_size) return out_size + 1;
                    MOVE_CURSOR(ptr, left, total_size, len);
                }
                if(_conf->isConfigedInMetainfo("found")) {
                    len = snprintf(ptr, left, 
                            "<miele key=\"found\" val=\"%d\" />\n", 
                            pClusterResult->_nEstimateDocsFound);
                    if(len + total_size > out_size) return out_size + 1;
                    MOVE_CURSOR(ptr, left, total_size, len);
                }
                if(_conf->isConfigedInMetainfo("prevpage")) {
                    len = snprintf(ptr, left, 
                            "<miele key=\"prevpage\" val=\"%d\" />\n", 
                            1);
                    if(len + total_size > out_size) return out_size + 1;
                    MOVE_CURSOR(ptr, left, total_size, len);
                }
                if(_conf->isConfigedInMetainfo("nextpage")) {
                    len = snprintf(ptr, left, 
                            "<miele key=\"nextpage\" val=\"%d\" />\n", 
                            1);
                    if(len + total_size > out_size) return out_size + 1;
                    MOVE_CURSOR(ptr, left, total_size, len);
                }
                if(_conf->isConfigedInMetainfo("time")) {
                    len = snprintf(ptr, left, 
                            "<miele key=\"time\" val=\"%.5d\" />\n", 
                            container.cost);
                    if(len + total_size > out_size) return out_size + 1;
                    MOVE_CURSOR(ptr, left, total_size, len);
                }
                len = snprintf(ptr, left, "</metainfo>\n");
                if(len + total_size > out_size) return out_size + 1;
                MOVE_CURSOR(ptr, left, total_size, len);
            }
            if(pClusterResult->_pEchoKeys && (!_conf || _conf->getOutputEchoKey())) {
                len = snprintf(ptr, left, 
                        "<echokeyset count=\"%d\">\n", 
                        0);
                if(len + total_size > out_size) return out_size + 1;
                MOVE_CURSOR(ptr, left, total_size, len);
                len = snprintf(ptr, left, "</echokeyset>\n");
                if(len + total_size > out_size) return out_size + 1;
                MOVE_CURSOR(ptr, left, total_size, len);
            }
            if (!_conf || _conf->getOutputExtKeySet()) {
                bool bRelation = 
                    _conf ? _conf->isConfigedInExtKeySet("relationset") : true;
                bool bSyn = _conf ? _conf->isConfigedInExtKeySet("synonymset") : true;
            }
            if(!_conf || _conf->getOutputStat()) {
                if (pClusterResult->_pStatisticResult != NULL) {
                    int32_t statSize = genStatisticXmlOutPut(*pClusterResult->_pStatisticResult, ptr, left);
                    if(statSize + total_size > out_size) return out_size + 1;
                    MOVE_CURSOR(ptr, left, total_size, statSize);
                }
            }
            if((!_conf || _conf->getOutputData()) &&
                    (pClusterResult->_nDocsReturn - dirty > 0) && 
                    (pClusterResult->_ppDocuments != NULL)) {
                len = snprintf(ptr, left, "<docset>\n");
                if(len + total_size > out_size) return out_size + 1;
                MOVE_CURSOR(ptr, left, total_size, len);
                for(i = 0; i < pClusterResult->_nDocsReturn; i++) {
                    if(pClusterResult->_ppDocuments[i] == NULL) continue;
                    len = snprintf(ptr, left, "<docele>\n");
                    if(len + total_size > out_size) return out_size + 1;
                    MOVE_CURSOR(ptr, left, total_size, len);
                    Document * pDocument = pClusterResult->_ppDocuments[i];
                    fieldCount = pDocument->getFieldCount();
                    for(int j = 0; j < fieldCount; j++) {
                        pTmp = pDocument->getField(j)->getName();
                        char *pEncodeStr = convertRestrictedChar(pDocument->getField(j)->getValue(), merger::sc_UTF8);
                        if(strlen(pEncodeStr) <= 0)
                            len = snprintf(ptr, left, "<docitem key=\"%s\" />\n",
                                    pDocument->getField(j)->getName());
                        else
                            len = snprintf(ptr, left, 
                                    "<docitem key=\"%s\">%s</docitem>\n",
                                    pDocument->getField(j)->getName(), 
                                    pEncodeStr);
                        if(pEncodeStr)  ::free(pEncodeStr);
                        if(len + total_size > out_size) return out_size + 1;
                        MOVE_CURSOR(ptr, left, total_size, len);
                    }
                    if(!_conf || _conf->getOutputRank()) {
                        int64_t * pFirstRanks = pClusterResult->_szFirstRanks;
                        int64_t * pSecondRanks = pClusterResult->_szSecondRanks;
                        int32_t * pDistCounts = pClusterResult->_szDistCounts;
                        int32_t * pUnionRelates = pClusterResult->_szUnionRelates;
                        unsigned char * pOnlineStatus = pClusterResult->_pOnlineStatus;
                        len = snprintf(ptr, left,  
                                "<docitem key=\"primary_rank_score\">%lld</docitem>\n\
                                <docitem key=\"second_rank_score\">%lld</docitem>\n\
                                <docitem key=\"distinct_count\">%d</docitem>\n\
                                <docitem key=\"online_status\">%d</docitem>\n\
                                <docitem key=\"sum_value\">%d</docitem>\n",
                                (pFirstRanks == NULL ? 0 : pFirstRanks[i]),
                                (pSecondRanks == NULL ? 0 : pSecondRanks[i]),
                                (pDistCounts == NULL ? 0 : pDistCounts[i]),
                                0,
                                0);
                        if(len + total_size > out_size) return out_size + 1;
                        MOVE_CURSOR(ptr, left, total_size, len);
                    }
                    len = snprintf(ptr, left, "</docele>\n");
                    if(len + total_size > out_size) return out_size + 1;
                    MOVE_CURSOR(ptr, left, total_size, len);
                }
                len = snprintf(ptr, left, "</docset>\n");
                if(len + total_size > out_size) return out_size + 1;
                MOVE_CURSOR(ptr, left, total_size, len);
            }
            static const char * RSENDTAG = "</resultset>\n";
            static const uint32_t RSENDTAG_LEN = strlen(RSENDTAG);
            if(left < RSENDTAG_LEN) return out_size + 1;
            memcpy(ptr, RSENDTAG, RSENDTAG_LEN);
            MOVE_CURSOR(ptr, left, total_size, RSENDTAG_LEN);
            return total_size;
        }

/**************************ResultFormatPB**************************/
#include "util/py_code.h"

uint32_t ResultFormatPB::doFormat(const result_container_t & container,
        char * out_data, uint32_t out_size) {
    commdef::ClusterResult* pClusterResult = container.pClusterResult;
    KSResult result;
    uint32_t serial_size = 0;

    if(!pClusterResult){
        result.set_status(KS_RESULT_EUNKNOWN);
        serial_size = result.ByteSize();
        if(serial_size<=out_size){
            if(!result.SerializeToArray(out_data, out_size)){
                return 0;
            }
        }
        return serial_size;
    }

    //Modified DocsFound
    if (pClusterResult->_nDocsFound < g_max_docsfound) {
        pClusterResult->_nEstimateDocsFound = pClusterResult->_nDocsFound;
    }
    //Fill header
    uint32_t dirty = 0;
    int fieldCount = 0;
    int totalfieldCount = 0;
    uint32_t fieldCount_offset = 0;
    uint32_t *_fieldcountarray = new uint32_t [pClusterResult->_nDocsReturn];
    if(!_fieldcountarray){
        result.set_status(KS_RESULT_EUNKNOWN);
        serial_size = result.ByteSize();
        if(serial_size<=out_size){
            if(!result.SerializeToArray(out_data, out_size)){
                return 0;
            }
        }
        return serial_size;
    }
    memset((char *)_fieldcountarray, 0x0, sizeof(uint32_t)*pClusterResult->_nDocsReturn);
    if(pClusterResult->_ppDocuments){
        travelDocument(pClusterResult, dirty, fieldCount, totalfieldCount, fieldCount_offset, _fieldcountarray);
    }
    delete [] _fieldcountarray;
    KSHeader* header = result.mutable_header();
    header->set_docs_return(pClusterResult->_nDocsReturn-dirty);
    header->set_docs_found(pClusterResult->_nEstimateDocsFound);
    header->set_docs_total(pClusterResult->_nDocsSearch);
    header->set_time_used(container.cost);
    header->set_docs_restrict(pClusterResult->_nDocsRestrict);

    // fill statistic
    char valbuf[64];
    int vallen = 0;
    statistic::StatisticResult* pStatisticResult = pClusterResult->_pStatisticResult;
    if(pStatisticResult && pStatisticResult->nCount){
        KSStatistics* stat = result.mutable_stat();
        for(int i=0; i<pStatisticResult->nCount; i++){
            statistic::StatisticResultItem* pResultItem = pStatisticResult->ppStatResItems[i];
            if(!pResultItem){
                continue;
            }
            KSStatistic* record = stat->add_records();
            record->set_field_name(pResultItem->pStatHeader->szFieldName);
            for(int j=0; j<pResultItem->nCount; j++){
                KSStatistic_Pair* pair = record->add_pairs();
                vallen = showVariantType(valbuf, 64, pResultItem->ppStatItems[j]->statKey.eFieldType, &pResultItem->ppStatItems[j]->statKey.uFieldValue);
                valbuf[vallen] = 0;
                pair->set_key(valbuf);
                pair->set_count(pResultItem->ppStatItems[j]->nCount);
            }
        }
    }

    // fill data
    if(pClusterResult->_nDocsReturn>dirty){
        KSData* data = result.mutable_data();
        Document* pDocument = pClusterResult->_ppDocuments[fieldCount_offset];
        for(int i=0; i<pDocument->getFieldCount(); i++){
            data->add_field_names(pDocument->getField(i)->getName());
        }
        for(int i=0; i<pClusterResult->_nDocsReturn; i++){
            pDocument = pClusterResult->_ppDocuments[i];
            if (pDocument == NULL) continue;
            KSDocument* doc = data->add_docs();
            for(int j=0; j<pDocument->getFieldCount(); j++){
                doc->add_field_values(pDocument->getField(j)->getValue());
            }
        }
    }
    // final result
    result.set_status(KS_RESULT_OK);
    serial_size = result.ByteSize();
    if(serial_size<=out_size){
        if(!result.SerializeToArray(out_data, out_size)){
            return 0;
        }
    }
    return serial_size;
}

        /********************ResultFormatFactory***************************/

        bool ResultFormatV3::accept(const char * name) {
            return name && 
                (strcmp(name, "v30") == 0 || 
                 strcmp(name, "v3") == 0 || 
                 strcmp(name, "v3.0") == 0 || 
                 strcmp(name, "3") == 0);
        }

        bool ResultFormatXML::accept(const char * name) {
            return name && 
                ((strlen(name) > 4 && memcmp("xml/", name, 4) == 0) ||
                 strcmp("xml", name) == 0);
        }

        bool ResultFormatPB::accept(const char * name) {
            return name &&
                (strcmp(name, "pb") == 0);
        }

        ResultFormatV3 ResultFormatFactory::s_v3(NULL);
        ResultFormatXMLRef ResultFormatFactory::s_xml_ref;
        ResultFormatXML ResultFormatFactory::s_xml(s_xml_ref, NULL);
        ResultFormatPB ResultFormatFactory::s_pb(NULL);

        ResultFormatFactory::~ResultFormatFactory() {
            std::map<std::string, ResultFormatXMLRef*>::iterator it;
            for(it = _mp_xml.begin(); it != _mp_xml.end(); it++) {
                if(it->second) delete it->second;
            }
            _mp_xml.clear();
        }

        ResultFormat * ResultFormatFactory::make(const char *name, MemPool *heap) {
            if(ResultFormatV3::accept(name)) {
                if(!heap) return &s_v3;
                return new (std::nothrow) ResultFormatV3(heap);
            } else if(ResultFormatXML::accept(name)) {
                if(strlen(name) < 4) {
                    if(likely(!heap)) return &s_xml;
                    return new (std::nothrow) ResultFormatXML(s_xml_ref, heap);
                }
                std::string key(name + 4);
                std::map<std::string, ResultFormatXMLRef*>::iterator it = 
                    _mp_xml.find(key);
                if(it == _mp_xml.end()) return NULL;
                return new (std::nothrow) 
                    ResultFormatXML((it->second ? *(it->second) : s_xml_ref), heap);
            } else if(ResultFormatPB::accept(name)) {
                if(!heap) return &s_pb;
                return new (std::nothrow) ResultFormatPB(heap);
            }
            return NULL;
        }

        void ResultFormatFactory::recycle(ResultFormat * p) {
            if(p &&
                    p != &s_v3 && 
                    p != &s_xml && 
                    p != &s_pb)
                delete p;
        }

        int32_t ResultFormatFactory::registerXML(const char * path) {
            int32_t ret;
            ResultFormatXMLRef * ref;
            const char * name;
            if(unlikely(!path || strlen(path) == 0)) return KS_EFAILED;
            ref = new (std::nothrow) ResultFormatXMLRef();
            if(unlikely(!ref)) return KS_ENOMEM;
            ret = ref->initialize(path);
            if(unlikely(ret != KS_SUCCESS)) {
                delete ref;
                return ret;
            }
            name = ref->getName();
            if(unlikely(!name || strlen(name) == 0)) {
                delete ref;
                return KS_EFAILED;
            }
            std::string key(name);
            if(unlikely(_mp_xml.find(key) != _mp_xml.end())) {
                TERR("redefine xml result format name[%s].", name);
                return KS_EFAILED;
            }
            TLOG("added result format 'xml/%s' by config file `%s'.",
                    name, path);
            _mp_xml.insert(std::make_pair(key, ref));
            return KS_SUCCESS;
        }

        uint32_t genStatisticXmlOutPut(const statistic::StatisticResult &statisticResult, char *output, uint32_t out_size)
        {
            char *ptr = output;
            uint32_t left = out_size;
            uint32_t total_size = 0;
            uint32_t len = 0;
            int32_t i = 0;
            if ( statisticResult.nCount > 0 )
            {
                len = snprintf(ptr, left, "<statset count=\"%d\">\n", statisticResult.nCount);
                if(len + total_size > out_size)
                {
                    return out_size + 1;
                }
                MOVE_CURSOR(ptr, left, total_size, len);
                for(i = 0; i < statisticResult.nCount; i++)
                {
                    statistic::StatisticResultItem *pResultItem = statisticResult.ppStatResItems[i];
                    if (NULL == pResultItem)
                    {
                        continue;
                    }
                    if (pResultItem->nCount <= 0)
                    {
                        len = snprintf(ptr, left, "<statele count=\"0\" key=\"%s\">\n</statele>\n",
                                pResultItem->pStatHeader->szFieldName);
                        if (len + total_size > out_size)
                        {
                            return out_size + 1;
                        }
                        MOVE_CURSOR(ptr, left, total_size, len);
                        continue;
                    }
                    len = snprintf(ptr, left, "<statele count=\"%d\" key=\"%s\">\n", pResultItem->nCount, pResultItem->pStatHeader->szFieldName);
                    if (len + total_size > out_size)
                    {
                        return out_size + 1;
                    }
                    MOVE_CURSOR(ptr, left, total_size, len);
                    for (int32_t i=0; i<pResultItem->nCount; i++)
                    {
                        if (pResultItem->ppStatItems[i]->nCount < 0)
                            break;
                        statistic::StatisticItem *pItem = pResultItem->ppStatItems[i];
                        statistic::StatisticHeader *pHead = pResultItem->pStatHeader;
                        len = snprintf(ptr, left, "<statitem count=\"%d\" statid=\"",
                                pItem->nCount);
                        if(len + total_size > out_size) {
                            return out_size + 1;
                        }
                        MOVE_CURSOR(ptr, left, total_size, len);
                        len = showVariantType(ptr, left, pItem->statKey.eFieldType,
                                &(pItem->statKey.uFieldValue)); 
                        if(len + total_size > out_size) {
                            return out_size + 1;
                        }
                        MOVE_CURSOR(ptr, left, total_size, len);
                        len = snprintf(ptr, left, "\"");
                        if(len + total_size > out_size) {
                            return out_size + 1;
                        }
                        MOVE_CURSOR(ptr, left, total_size, len);
                        if (pHead->szSumFieldName != NULL
                                && (pHead->sumFieldType == DT_INT32
                                    || pHead->sumFieldType == DT_INT64
                                    || pHead->sumFieldType == DT_DOUBLE
                                    || pHead->sumFieldType == DT_INT8
                                    || pHead->sumFieldType == DT_INT16
                                    || pHead->sumFieldType == DT_FLOAT)) {
                            len = snprintf(ptr, left, " sumcnt=\"");
                            if(len + total_size > out_size) {
                                return out_size + 1;
                            }
                            MOVE_CURSOR(ptr, left, total_size, len);
                            len = showVariantType(ptr, left,
                                    pHead->sumFieldType, &(pItem->sumCount));
                            if(len + total_size > out_size) {
                                return out_size + 1;
                            }
                            MOVE_CURSOR(ptr, left, total_size, len);
                            len = snprintf(ptr, left, "\"");
                            if(len + total_size > out_size) {
                                return out_size + 1;
                            }
                            MOVE_CURSOR(ptr, left, total_size, len);
                        }
                        len = snprintf(ptr, left, " />\n");
                        if(len + total_size > out_size) {
                            return out_size + 1;
                        }
                        MOVE_CURSOR(ptr, left, total_size, len);
                    }
                    len = snprintf(ptr, left, "</statele>\n");
                    if(len + total_size > out_size) {
                        return out_size + 1;
                    }
                    MOVE_CURSOR(ptr, left, total_size, len);
                }
                len = snprintf(ptr, left, "</statset>\n");
                if(len + total_size > out_size) return out_size + 1;
                MOVE_CURSOR(ptr, left, total_size, len);
            }
            return total_size;
        }

        uint32_t genStatisticV3OutPut(const statistic::StatisticResult &statisticResult, char *output, uint32_t out_size)
        {
            char *ptr = output;
            uint32_t left = out_size;
            uint32_t total_size = 0;
            uint32_t len = 0;
            int32_t i = 0;
            int32_t n = 1;
            if ( statisticResult.nCount > 0 ) {
                for( i = 0; i < statisticResult.nCount; i++ ) {
                    statistic::StatisticResultItem *pResultItem = statisticResult.ppStatResItems[i];
                    if (NULL == pResultItem) {
                        continue;
                    }
                    n += (pResultItem->nCount + 1);
                }
                len = snprintf(ptr, left, "%s%c%d%c%d%c",
                        SECTION_STATINFO, STOP, n, STOP, 
                        statisticResult.nCount, STOP);
                if (len + total_size > out_size) {
                    return out_size + 1;
                }
                MOVE_CURSOR(ptr, left, total_size, len);
                for ( i = 0; i < statisticResult.nCount; i++ ) {
                    statistic::StatisticResultItem *pResultItem = statisticResult.ppStatResItems[i];
                    if (NULL == pResultItem)
                    {
                        continue;
                    }
                    len = snprintf(ptr, left, "%d%c", pResultItem->nCount, STOP);
                    if(len + total_size > out_size) {
                        return out_size + 1;
                    }
                    MOVE_CURSOR(ptr, left, total_size, len);
                    for (int32_t i=0; i<pResultItem->nCount; i++)
                    {
                        if (pResultItem->ppStatItems[i]->nCount < 0)
                            break;
                        statistic::StatisticItem *pItem = pResultItem->ppStatItems[i];
                        statistic::StatisticHeader *pHead = pResultItem->pStatHeader;

                        len = showVariantType(ptr, left, pItem->statKey.eFieldType,
                                &(pItem->statKey.uFieldValue)); 
                        if(len + total_size > out_size) {
                            return out_size + 1;
                        }
                        MOVE_CURSOR(ptr, left, total_size, len);
                        len = snprintf(ptr, left, ";%d;", pItem->nCount);
                        if(len + total_size > out_size) {
                            return out_size + 1;
                        }
                        MOVE_CURSOR(ptr, left, total_size, len);
                        if (pHead->szSumFieldName != NULL
                                && (pHead->sumFieldType == DT_INT32
                                    || pHead->sumFieldType == DT_INT64
                                    || pHead->sumFieldType == DT_DOUBLE
                                    || pHead->sumFieldType == DT_INT8
                                    || pHead->sumFieldType == DT_INT16
                                    || pHead->sumFieldType == DT_FLOAT)) {
                            len = showVariantType(ptr, left,
                                    pHead->sumFieldType, &(pItem->sumCount));
                            if(len + total_size > out_size) {
                                return out_size + 1;
                            }
                            MOVE_CURSOR(ptr, left, total_size, len);
                        }
                        len = snprintf(ptr, left, "%c", STOP);
                        if(len + total_size > out_size) {
                            return out_size + 1;
                        }
                        MOVE_CURSOR(ptr, left, total_size, len);
                    }
                }
            }
            return total_size;
        }
