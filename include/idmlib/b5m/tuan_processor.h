#ifndef IDMLIB_B5M_TUANPROCESSOR_H
#define IDMLIB_B5M_TUANPROCESSOR_H 
#include <util/ustring/UString.h>
#include <idmlib/duplicate-detection/dup_detector.h>
#include "product_price.h"
#include "b5m_helper.h"
#include "b5m_types.h"
#include "b5m_m.h"
#include <boost/unordered_map.hpp>
#include <sf1common/ScdWriter.h>
#include <sf1common/PairwiseScdMerger.h>
NS_IDMLIB_B5M_BEGIN

class TuanProcessor{
    typedef izenelib::util::UString UString;

    class TuanProcessorAttach
    {
    public:
        uint32_t sid; //sid should be same
        ProductPrice price;
        std::vector<std::string> area_array;//sorted, should be at least one same

        template<class Archive>
        void serialize(Archive& ar, const unsigned int version)
        {
            ar & sid & price & area_array;
        }

        bool dd(const TuanProcessorAttach& other) const
        {
            if(sid!=other.sid) return false;
            return true;
            double mid1;
            double mid2;
            if(!price.GetMid(mid1)) return false;
            if(!other.price.GetMid(mid2)) return false;
            double max = std::max(mid1, mid2);
            double min = std::min(mid1, mid2);
            if(min<=0.0) return false;
            double ratio = max/min;
            if(ratio>2.0) return false;
            if(area_array.empty()&&other.area_array.empty()) return true;
            bool found = false;
            uint32_t i=0,j=0;
            while(i<area_array.size()&&j<other.area_array.size())
            {
                if(area_array[i]==other.area_array[j])
                {
                    found = true;
                    break;
                }
                else if(area_array[i]<other.area_array[j])
                {
                    ++i;
                }
                else
                {
                    ++j;
                }
            }
            if(!found) return false;
            return true;
        }
    };
    typedef std::string DocIdType;

    struct ValueType
    {
        std::string soid;
        UString title;

        bool operator<(const ValueType& other) const
        {
            return title.length()<other.title.length();
        }
        
    };


    typedef idmlib::dd::DupDetector<DocIdType, uint32_t, TuanProcessorAttach> DDType;
    typedef DDType::GroupTableType GroupTableType;
    typedef PairwiseScdMerger::ValueType SValueType;
    typedef boost::unordered_map<uint128_t, SValueType> CacheType;

public:
    TuanProcessor(const B5mM& b5mm);
    bool Generate(const std::string& mdb_instance);

    void SetCmaPath(const std::string& path)
    { cma_path_ = path; }

private:

    void POutputAll_();
    void B5moOutput_(SValueType& value, int status);
    uint128_t GetPid_(const Document& doc);
    uint128_t GetOid_(const Document& doc);
    void ProductMerge_(SValueType& value, const SValueType& another_value);

private:
    B5mM b5mm_;
    std::string cma_path_;
    CacheType cache_;
    boost::shared_ptr<ScdWriter> pwriter_;
};

NS_IDMLIB_B5M_END

#endif

