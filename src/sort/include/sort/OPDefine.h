/*********************************************************************
 * $Author:  $
 *
 * $LastChangedBy: xiaoleng.nj $
 *
 * $Revision: 2577 $
 *
 * $LastChangedDate: 2011-03-23 09:50:55 +0800 (Wed, 09 Mar 2011) $
 *
 * $Id: OPDefine.h 2577 2011-03-09 01:50:55Z  $
 *
 * $Brief: ini file parser $
 ********************************************************************/
#ifndef __OP_NAME_DEFINE__H__
#define __OP_NAME_DEFINE__H__

//#define DEBUG 1
#define MAX_DOC_COUNT 10000000
#define EXTERNALTYPE_BASESORT "IBaseSort"
//#define EXTERNALTYPE_SCOREREWRITER "IScoreRewriter"
#define EXTERNALTYPE_PHASE2MLR "IRelevanceScoring"

#define INNEROBJNAME_SORTPREPARE "SortPrepare"
#define INNEROBJNAME_DATAREADER "DataReader"
#define INNEROBJNAME_SORTFILLER	"SortFiller"

#define INNEROBJNAME_FIELD_ADD_WEIGHT "TaobaoWeightBlender"
#define INNEROBJNAME_HEAP_SORT_2D "HeapSort2D"
#define INNEROBJNAME_UNIQUE_SORT "SimpleUniqueSort"
#define INNEROBJNAME_UNIQUE_II_SORT "UniqIISort"
#define INNEROBJNAME_FIRST_AUDITION "FirstAudition"
#define DEFAULT_MERGER_EXEC_BRANCH  "*"
#define INNEROBJNAME_HEAPSORT "HeapSort"
#define INNEROBJNAME_RELE_SORT "ReleSort"
#define INNEROBJNAME_DEFAULT_RULE_SORT "DefaultRuleSort"
#define INNEROBJNAME_UNIQ_RANK_SORT "UNIQRANK"

#define INNEROBJNAME_MERGERPREPARER "MergerPreparer"
#define INNEROBJNAME_MERGESCOREANDGROUP	"MergerGroup"
#define INNEROBJNAME_VSACMERGER "VSAContainerMerger"
#define UNIQRANK "uniqrank"


#define OBJNAME_FIRSTAUDISCORE	"FirstAudiScoreSort"
#define OBJNAME_RELEMLRSCORE	"ReleScoreSort"
#define SCORE_REWRITER "__ScoreRewriter__"


/********** following is vsa name define. *****************/

// 以 '~' 开头的字段表示VSA中预先分配空间，但不提前预读，在排序完成后再填充
#define MNAME_COEFP "coefp"
#define MNAME_PRIMARYVSA "primaryVSA"
#define MNAME_DOCID "docid"
//#define MNAME_NID_DELAY "~nid"
#define MNAME_NID "nid"
//#define MNAME_NID MNAME_DOCID
#define MNAME_DEF_SORT_FIELD "ends"
#define MNAME_ENDS "ends"
#define MNAME_PFL "__profile__"

#define MNAME_RELESCORE "relevance"
#define MNAME_LEVELFIELD "dist_level"
#define MNAME_NGROUP "_nGroup"
#define MNAME_FILTER_DOC_NUM "__filter_docs_num__"
#define MNAME_MATCHWEIGHT "__matchweight"

#define MNAME_CHAIN_DESC "__processor_chain_desc__"
#define MNAME_DEBUG_INFO_DOC "__DebugInfoDoc__"
#define MNAME_DEBUG_INFO_QUERY "__DebugInfoQuery__"

#define MNAME_MLROPTIMIZECONF "__mlroptimizeconf__"
#define MNAME_MLROPTIMIZEARRAY "__mlroptimizearray__"

#define MNAME_UNIQKEY "__uniqkey__"
#define MNAME_UNIQVAL "__uniqval__"
#define MNAME_CONTAINER_COUNT "__container_count__"
#define MNAME_CLUSTERARR "__clusters__"
#define MNAME_UNIQVSA "__uniqvsa__"
#define MNAME_UNIQ_NULL_COUNT "__uniq_null_count__"
#define TOP_HEAPSORT_2D_NUMBERS "top_heapsort_2d_num"
#define MNAME_CLUSTERNAME "__clustername__"
#define MNAME_SERVERID "__serverid__"
#define MNAME_RESULT_POS "__result_pos__"

#define MNAME_UNIQII_VSA_IDX "__uniq_ii_vsa_idx__"
#define MNAME_UNIQII_VSA_INFO "__uniq_ii_vsa_info__"
#define MNAME_UNIQII_KEY "__uniqkey__"
#define MNAME_UNIQII_CNT "__uniqval__"
#define MNAME_UNIQII_STAT_SIZE "__uniq_ii_stat_size__"
#define MNAME_UNIQII_STAT "__uniq_ii_stat__"
#define MNAME_UNIQII_INFO_SIZE "__uniq_ii_info_size__"
#define MNAME_UNIQII_INFO_START "__uniq_ii_info_start__"
#define MNAME_UNIQII_SEID "__uniq_ii_seid__"
#define MNAME_UNIQII_MEID "__uniq_ii_meid__"
#define UNIQII_PARAM "uniqii_param"

#define NAME_PREPROCESS_DEFSORT "__preprocess_defsort__"
#define NAME_PREPROCESS_DEFSORT_NUMBERS "__preprocess_defsort_num__"

#define MAX_UNIQINFO_NUM 20             //用于UniqII,Uniq Info最大个数
#define MAX_UNIQINFO_CONTENT_NUM 12     //用于UniqII,指定content类型返回时，每条结果的宝贝最大展示个数
#define MAX_FIELD_NAME_LENGTH 128
#define INVALID_ENDS 0x7fffffff 
#define MNAME_DOC_FOUND "ndoc_found"
#define MNAME_ESDOC_FOUND "n_esdoc_found"
#define MNAME_STR_PS "str_ps"
#define MNAME_STR_SS "str_ss"
#define MNAME_ES_FACTOR "n_es_factor"
#define MNAME_UNIQII_INFO_ID "uniqII_info_id"
#define MNAME_UNIQII_INFO_NID "uniqII_info_nid"
#define MNAME_UNIQII_INFO_PS "uniqII_info_ps"
#define MNAME_UNIQII_INFO_SS "uniqII_info_ss"

#define MAX_RESORT_FIELD_NUM 24
#endif
