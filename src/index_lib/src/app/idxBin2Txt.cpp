#include "index_lib/IndexLib.h"
#include "index_lib/IndexBuilder.h"

using namespace index_lib;

int main(int argc, char** argv)
{
    char* outpath = NULL;
    if(argc == 2) {
        fprintf(stdout, "%s index_path=%s\n", argv[0], argv[1]);
        outpath = "./";
    } else if(argc == 3) {
        fprintf(stdout, "%s index_path=%s out_path=%s\n", argv[0], argv[1], argv[2]);
        outpath = argv[2];
    } else {
        fprintf(stdout, "%s index_path [out_path]\n", argv[0]);
        return -1;
    }

    IndexBuilder* pbuilder = IndexBuilder::getInstance();
    if(pbuilder->reopen(argv[1]) < 0) {
        fprintf(stderr, "open indexlib %s error\n", argv[1]);
        return -1;
    }

    int32_t travelPos = -1;
    int32_t maxOccNum = 0;
    const char* fieldName = pbuilder->getNextField(travelPos, maxOccNum);
    
    while(fieldName) {
        char filename[PATH_MAX];
        snprintf(filename, PATH_MAX, "%s/%s.txt", outpath, fieldName); 
        FILE* fp = fopen(filename, "wb");
        if(NULL == fp) {
            fprintf(stderr, "open %s error\n", filename);
            return -1;
        }

        uint64_t termSign;
        const IndexTermInfo* pTermInfo = pbuilder->getFirstTermInfo(fieldName, termSign);
        while(pTermInfo) {
            IndexTerm * pIndexTerm = pbuilder->getTerm(fieldName, termSign);
            if(NULL == pIndexTerm) {
                fprintf(stderr, "getTerm %s %lu error\n", fieldName, termSign);
                return -1;
            }
            fprintf(fp, "term:%lu docNum:%d\n", termSign, pTermInfo->docNum);

            uint32_t docId;
            if(pTermInfo->maxOccNum > 0) {
                while((docId = pIndexTerm->next()) < INVALID_DOCID) {
                    int32_t count;
                    uint8_t* pocc = pIndexTerm->getOcc(count);
                    for(int32_t i = 0; i < count; i++) {
                        fprintf(fp, "%u(%u) ", docId, pocc[i]);
                    }
                }
            } else {
                while((docId = pIndexTerm->next()) < INVALID_DOCID) {
                    fprintf(fp, "%u ", docId);
                }
            }

            fprintf(fp, "\n");
            pTermInfo = pbuilder->getNextTermInfo(fieldName, termSign);
        }
        
        fclose(fp);
        fprintf(stdout, "finish:%s\n", fieldName);
        fieldName = pbuilder->getNextField(travelPos, maxOccNum);
    }

    pbuilder->close();
    return 0;
}


