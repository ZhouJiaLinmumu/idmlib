#ifndef IDMLIB_B5M_PSMINDEXER_H_
#define IDMLIB_B5M_PSMINDEXER_H_

#include "psm_helper.h"
#include <string>
#include <vector>
#include "product_price.h"
#include "b5m_types.h"
#include "b5m_helper.h"
#include <types.h>
#include <am/succinct/fujimap/fujimap.hpp>
#include <glog/logging.h>
#include <boost/unordered_map.hpp>
#include <idmlib/duplicate-detection/dup_detector.h>
#include <idmlib/duplicate-detection/psm.h>

NS_IDMLIB_B5M_BEGIN

class PsmIndexer
{
    struct CacheType
    {
        CacheType(){}
        CacheType(const std::string& p1, const std::vector<std::pair<std::string, double> >& p2, const PsmAttach& p3)
        :key(p1), doc_vector(p2), attach(p3)
        {
        }
        std::string key;
        std::vector<std::pair<std::string, double> > doc_vector;
        PsmAttach attach;
    };
public:
    typedef idmlib::dd::DupDetector<uint32_t, uint32_t, PsmAttach> DDType;
    typedef DDType::GroupTableType GroupTableType;
    typedef idmlib::dd::PSM<64, 3, 24, std::string, std::string, PsmAttach> PsmType;
    PsmIndexer(const std::string& cma_path);
    void SetPsmK(uint32_t k) {psmk_ = k;}
    
    bool Index(const std::string& scd_path, const std::string& output_path, bool test = false);
    bool DoMatch(const std::string& scd_path, const std::string& knowledge_dir);

private:
    std::string cma_path_;
    uint32_t psmk_;
};

NS_IDMLIB_B5M_END

#endif

