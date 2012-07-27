#include "AttrFilter.h"

using namespace index_lib;

namespace search {

AttrFilter::AttrFilter(ProfileDocAccessor *profile_accessor, 
                       const char         *field_name, 
                       bool                is_negative)
{
    _profile_accessor       = profile_accessor;
    _profile_field          = _profile_accessor->getProfileField(field_name);
    _zk_time_pf             = _profile_accessor->getProfileField("zk_time");
    _is_negative            = is_negative;
    _value_count            = 0;
    _range_count            = 0;
    _discount_time          = 0;
    _is_promotion = false;
}

AttrFilter::~AttrFilter()
{

}

void AttrFilter::addFiltValue(const char *value)
{

}

bool AttrFilter::isPromotionType()
{
    return _is_promotion;
}

bool AttrFilter::isLatLngType()
{
    if (unlikely(_profile_field == NULL)) {
        return false;
    }

    return (_profile_field->flag == F_LATLNG_DIST);
}

void AttrFilter::addFiltRange(const Range<char*> value)
{

}

void AttrFilter::setDiscount(const char* discount_field, time_t discount_time)
{
    PF_DATA_TYPE type;
    if(discount_field) {
        _zk_rate_pf = _profile_accessor->getProfileField(discount_field);
        if(_zk_rate_pf) {
            type = _profile_accessor->getFieldDataType(_zk_rate_pf);
            if(type == DT_INT16 || type == DT_INT8 || type == DT_INT32 || type == DT_UINT8
                    || type == DT_UINT16 || type == DT_UINT32) {
                _discount_time = discount_time;
            } else {
                _zk_rate_pf = NULL;
            }
        }
    }
}

int16_t AttrFilter::getDiscount(uint32_t doc_id)
{
    PF_DATA_TYPE time_type;
    PF_DATA_TYPE rate_type;
    time_t  begin_time      = 0;
    time_t  end_time        = 0;
    int64_t time_value      = 0;
    int64_t rate_value      = 0;
    int32_t multi_time_num  = 0;
    int32_t multi_rate_num  = 0;
    int16_t ret             = DISCOUNT_BASE;
    if(!_zk_time_pf || !_zk_rate_pf) {
        return ret;
    }
    multi_rate_num  = _profile_accessor->getFieldMultiValNum(_zk_rate_pf);
    multi_time_num  = _profile_accessor->getFieldMultiValNum(_zk_time_pf);
    time_type       = _profile_accessor->getFieldDataType(_zk_time_pf);
    if(multi_rate_num != multi_time_num) {
        return ret;
    }
    if(multi_rate_num == 1) {        //单值字段
        rate_value = _profile_accessor->getInt(doc_id, _zk_rate_pf);
        if(time_type == DT_INT64) {
            time_value = _profile_accessor->getInt(doc_id, _zk_time_pf);
        } else {
            time_value = _profile_accessor->getUInt(doc_id, _zk_time_pf);
        }
        begin_time = (time_t)(time_value >> 32);
        end_time = (time_t) (time_value & 0xFFFFFFFF);
        if(begin_time <= _discount_time && _discount_time < end_time) {
            ret = (int16_t)(rate_value);
        }
    } else { 
        rate_type = _zk_rate_pf->type;
        _profile_accessor->getRepeatedValue(doc_id, _zk_rate_pf, _zk_rate_iter);
        _profile_accessor->getRepeatedValue(doc_id, _zk_time_pf, _zk_time_iter);
        int32_t time_cnt = _zk_time_iter.getValueNum();
        int32_t rate_cnt = _zk_rate_iter.getValueNum();
        if(time_cnt != rate_cnt) {
            return ret;
        }
        for(int32_t j = 0; j < time_cnt ; j++) {
            if(rate_type == DT_INT8) {
                rate_value = _zk_rate_iter.nextInt8();
            } else if(rate_type == DT_INT16) {
                rate_value = _zk_rate_iter.nextInt16();
            } else if(rate_type == DT_INT32) {
                rate_value = _zk_rate_iter.nextInt32();
            } else {
                return ret;
            }
            if(time_type == DT_INT64) {
                time_value = _zk_time_iter.nextInt64();
            } else {
                time_value = _zk_time_iter.nextUInt64();
            }
            begin_time = (time_t)(time_value >> 32);
            end_time   = (time_t)(time_value & 0xFFFFFFFF);
            if(begin_time <= _discount_time && _discount_time < end_time) {
                ret = (int16_t)(rate_value);
                break;
            }
        }
    }

    return ret;
}

int16_t AttrFilter::getZKGroup(uint32_t doc_id)
{
    PF_DATA_TYPE time_type;
    PF_DATA_TYPE rate_type;
    int16_t ret             = INVAILD_ZKGROUP;
    time_t  begin_time      = 0;
    time_t  end_time        = 0;
    int64_t time_value      = 0;
    int64_t rate_value      = 0;
    int32_t multi_time_num  = 0;
    int32_t multi_rate_num  = 0;
    if(!_zk_time_pf || !_zk_rate_pf) {
        return INVAILD_ZKGROUP;
    }
    multi_rate_num  = _profile_accessor->getFieldMultiValNum(_zk_rate_pf);
    multi_time_num  = _profile_accessor->getFieldMultiValNum(_zk_time_pf);
    time_type       = _profile_accessor->getFieldDataType(_zk_time_pf);
    if(multi_rate_num != multi_time_num) {
        return INVAILD_ZKGROUP;
    }
    rate_type = _zk_rate_pf->type;
    _profile_accessor->getRepeatedValue(doc_id, _zk_rate_pf, _zk_rate_iter);
    _profile_accessor->getRepeatedValue(doc_id, _zk_time_pf, _zk_time_iter);
    int32_t time_cnt = _zk_time_iter.getValueNum();
    int32_t rate_cnt = _zk_rate_iter.getValueNum();
    if(time_cnt != rate_cnt) {
        return INVAILD_ZKGROUP;
    }
    for(int32_t j = 0; j < time_cnt ; j++) {
        if(rate_type == DT_INT8) {
            rate_value = _zk_rate_iter.nextInt8();
        } else if(rate_type == DT_UINT8) {
            rate_value = _zk_rate_iter.nextUInt8();
        } else if(rate_type == DT_INT16) {
            rate_value = _zk_rate_iter.nextInt16();
        } else if(rate_type == DT_INT32) {
            rate_value = _zk_rate_iter.nextInt32();
        } else {
            return INVAILD_ZKGROUP;
        }
        if(time_type == DT_INT64) {
            time_value = _zk_time_iter.nextInt64();
        } else if(time_type == DT_UINT64) {
            time_value = _zk_time_iter.nextUInt64();
        } else {
            return INVAILD_ZKGROUP;
        }
        begin_time = (time_t)(time_value >> 32);
        end_time   = (time_t)(time_value & 0xFFFFFFFF);
        if(begin_time <= _discount_time && _discount_time < end_time) {
            ret = (int16_t)(rate_value);
            break;
        }
    }

    return ret;
}
}
