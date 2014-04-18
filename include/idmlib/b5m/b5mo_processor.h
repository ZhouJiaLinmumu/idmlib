#ifndef IDMLIB_B5M_B5MOPROCESSOR_H_
#define IDMLIB_B5M_B5MOPROCESSOR_H_

#include <sf1common/ScdParser.h>
#include <string>
#include <vector>
#include "b5m_types.h"
#include "offer_db.h"
//#include "brand_db.h"
#include "product_matcher.h"
#include "b5mo_sorter.h"
#include "original_mapper.h"
#include "matcher_status.h"
#include "b5m_m.h"
#include <knlp/attr_normalize.h>
#include <boost/atomic.hpp>
//#include <sf1r-net/RpcServerConnectionConfig.h>

NS_IDMLIB_B5M_BEGIN
class RpcServerConnectionConfig;
class B5moProcessor {
    struct LastOMapperItem {
        OriginalMapper* last_omapper;
        ScdTypeWriter* writer;
        std::string text;
    };
public:
    B5moProcessor(const B5mM& b5mm);
    ~B5moProcessor();

    void LoadMobileSource(const std::string& file);
    void SetHumanMatchFile(const std::string& file) {human_match_file_ = file;}

    void Process(ScdDocument& doc);

    bool Generate(const std::string& mdb_instance, const std::string& last_mdb_instance);

private:

    void OMapperChange_(LastOMapperItem& item);

    void EstimateSA_(ScdDocument& doc);
    void ProcessIU_(ScdDocument& doc, bool force_match = false);
    void ProcessIUProduct_(ScdDocument& doc, Product& p, const std::string& old_spid="");
    void PendingProcess_(ScdDocument& doc);

    bool OMap_(const OriginalMapper& omapper, Document& doc) const;

private:
    B5mM b5mm_;
    OfferDb* odb_;
    ProductMatcher* matcher_;
    std::string ts_;
    std::string last_ts_;
    B5moSorter* sorter_;
    OriginalMapper* omapper_;
    int mode_;
    std::string human_match_file_;
    boost::unordered_set<std::string> mobile_source_;
    //sf1r::RpcServerConnectionConfig* img_server_cfg_;
    boost::shared_ptr<ScdTypeWriter> writer_;
    std::ofstream match_ofs_;
    std::ofstream cmatch_ofs_;
    boost::unordered_set<uint128_t> changed_match_;
    boost::shared_mutex mutex_;
    ilplib::knlp::AttributeNormalize* attr_;
    MatcherStatus status_;
    //boost::atomic<uint32_t> stat1_;
    //boost::atomic<uint32_t> stat2_;
    //boost::atomic<uint32_t> stat3_;
};

NS_IDMLIB_B5M_END

#endif

