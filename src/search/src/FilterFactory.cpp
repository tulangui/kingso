#include "FilterFactory.h"

#include "SearchDef.h"
#include "IntAttrFilter.h"
#include "UIntAttrFilter.h"
#include "FloatAttrFilter.h"
#include "DoubleAttrFilter.h"
#include "StringAttrFilter.h"
#include "DeleteMapFilter.h"
#include "BitAttrFilter.h"
#include "PromotionAttrFilter.h"
#include "ZKGroupAttrFilter.h"
#include "UBitAttrFilter.h"
#include "ProvCityAttrFilter.h"
#include "BitmapIndexFilter.h"
#include "LatLngAttrFilter.h"

#include "util/MemPool.h"
#include "commdef/commdef.h"

using namespace index_lib;

namespace search {


FilterFactory::FilterFactory()
{
    _profile_accessor = NULL;
    _p_detele_map     = NULL;
}

FilterFactory::~FilterFactory()
{
}

bool FilterFactory::init(ProfileDocAccessor      *profile_accessor, 
                         ProvCityManager         *provcity_manager, 
                         DocIdManager::DeleteMap *p_delete_map) 
{
    if(!profile_accessor || !provcity_manager) {
        return false;
    }
    _profile_accessor = profile_accessor;
    _provcity_manager = provcity_manager;
    _p_detele_map     = p_delete_map;
    return true;
}

int FilterFactory::create(Context             *p_context, 
                          FilterInterfaceType  filter_type, 
                          FilterInterface     *&p_filter_interface, 
                          const char          *field_name, 
                          bool                 is_negative)
{
    int ret            = KS_SUCCESS;
    p_filter_interface = NULL;
    MemPool *mempool   = p_context->getMemPool();

    switch(filter_type)
    {
        case DT_DELMAP : {
            p_filter_interface = NEW(mempool, DeleteMapFilter)(_p_detele_map);
            if(p_filter_interface == NULL) {
                TERR2MSG(p_context, "Out of memory in FilterFactory::create");
                ret = KS_ENOMEM;
            }
            break;
        }
        case DT_BITMAP : {
            p_filter_interface = NEW(mempool, BitmapIndexFilter)
                                    ((const uint64_t*)field_name, is_negative);
            if(p_filter_interface == NULL) {
                TERR2MSG(p_context, "Out of memory in FilterFactory::create");
                ret = KS_ENOMEM;
            }
            break;
        }
        case DT_PROFILE : {
            if(_profile_accessor == NULL || field_name == NULL || field_name[0] == '\0') {
                ret = KS_EINVAL;
                break;
            }
            const ProfileField *profile_field = _profile_accessor->getProfileField(field_name);
            if(profile_field == NULL) {
                ret = KS_EINVAL;
                break;
            }
            PF_DATA_TYPE  type       = _profile_accessor->getFieldDataType(profile_field);
            PF_FIELD_FLAG field_flag = _profile_accessor->getFieldFlag(profile_field);
            if(field_flag == F_PROVCITY) {
                p_filter_interface = NEW(mempool, ProvCityAttrFilter)
                                        (_profile_accessor, _provcity_manager, 
                                         field_name,        is_negative);
                if(p_filter_interface == NULL) {
                    TERR2MSG(p_context, "Out of memory in FilterFactory::create");
                    ret = KS_ENOMEM;
                }
                break;
            }

            if(field_flag == F_LATLNG_DIST) {
                p_filter_interface = NEW(mempool, LatLngAttrFilter)
                    (_profile_accessor, field_name, is_negative);
                if (p_filter_interface == NULL) {
                    TERR2MSG(p_context, "Out of memory in FilterFactory::create");
                    ret = KS_ENOMEM;
                }
                break;
            }

            switch(type) {
                case DT_INT8 :
                case DT_INT16 :
                case DT_INT32 :
                case DT_INT64 :
                    if(field_flag == F_PROMOTION) {
                        p_filter_interface = NEW(mempool, PromotionAttrFilter)(_profile_accessor, field_name, is_negative);
                    } else if(field_flag == F_ZK_GROUP) {
                        p_filter_interface = NEW(mempool, ZKGroupAttrFilter)(_profile_accessor, field_name, is_negative);
                    } else {
                        if(field_flag == F_FILTER_BIT) {
                        p_filter_interface = NEW(mempool, BitAttrFilter)
                                                (_profile_accessor, field_name, is_negative);
                        } else {
                        p_filter_interface = NEW(mempool, IntAttrFilter)
                                                (_profile_accessor, field_name, is_negative);   
                        }
                    }
                    if(p_filter_interface == NULL) {
                        TERR2MSG(p_context, "Out of memory in FilterFactory::create");
                        ret = KS_ENOMEM;
                    }
                    break;
                case DT_UINT8 :
                case DT_UINT16 :
                case DT_UINT32 :
                case DT_UINT64 :
                    if(field_flag == F_PROMOTION) {
                        p_filter_interface = NEW(mempool, PromotionAttrFilter)(_profile_accessor, field_name, is_negative);
                    } else if(field_flag == F_ZK_GROUP) {
                        p_filter_interface = NEW(mempool, ZKGroupAttrFilter)(_profile_accessor, field_name, is_negative);
                    }else {
                        if(field_flag == F_FILTER_BIT) {
                            p_filter_interface = NEW(mempool, UBitAttrFilter)
                                (_profile_accessor, field_name, is_negative);
                        } else {
                            p_filter_interface = NEW(mempool, UIntAttrFilter)
                                (_profile_accessor, field_name, is_negative);
                        }
                    }
                    if(p_filter_interface == NULL) {
                        TERR2MSG(p_context, "Out of memory in FilterFactory::create");
                        ret = KS_ENOMEM;
                    }
                    break;
                case DT_FLOAT :
                    p_filter_interface = NEW(mempool, FloatAttrFilter)
                                            (_profile_accessor, field_name, is_negative);
                    if(p_filter_interface == NULL) {
                        TERR2MSG(p_context, "Out of memory in FilterFactory::create");
                        ret =  KS_ENOMEM;
                    }
                    break;
                case DT_DOUBLE :
                    p_filter_interface = NEW(mempool, DoubleAttrFilter)
                                            (_profile_accessor, field_name, is_negative);
                    if(p_filter_interface == NULL) {
                        TERR2MSG(p_context, "Out of memory in FilterFactory::create");
                        ret = KS_ENOMEM;
                    }
                    break;
                case DT_STRING :
                    p_filter_interface = NEW(mempool, StringAttrFilter)
                                            (_profile_accessor, field_name, is_negative);
                    if(p_filter_interface == NULL) {
                        TERR2MSG(p_context, "Out of memory in FilterFactory::create");
                        ret = KS_ENOMEM;
                    }
                    break;
                
                default :
                    ret = KS_EINVAL;
                    break;
            }
        }
        default : {
            ret = KS_EINVAL;
            break;
        }
    }

    return ret;
}

}
