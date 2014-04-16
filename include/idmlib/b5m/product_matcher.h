#ifndef IDMLIB_B5M_PRODUCTMATCHER_H_
#define IDMLIB_B5M_PRODUCTMATCHER_H_
#include "ngram_synonym.h"
#include "b5m_helper.h"
#include "b5m_types.h"
#include "brand_manager.h"
#include "category_psm.h"
#include <boost/regex.hpp>
#include <boost/atomic.hpp>
#include <boost/unordered_map.hpp>
#include <boost/unordered_set.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/serialization/set.hpp>
#include <boost/operators.hpp>
#include <ir/id_manager/IDManager.h>
#include <util/singleton.h>
#include <am/sequence_file/ssfr.h>
#include <am/leveldb/Table.h>
#include <idmlib/util/idm_analyzer.h>
#include <idmlib/util/idm_id_manager.h>
#include <sf1common/Document.h>
#include <sf1common/ScdDocument.h>
#include <idmlib/keyphrase-extraction/kpe_task.h>
#include <idmlib/similarity/string_similarity.h>

NS_IDMLIB_B5M_BEGIN

using izenelib::util::UString;

class ProductMatcher {


public:
    typedef idmlib::sim::StringSimilarity::Object SimObject;
    typedef idmlib::util::IDMTerm Term;
    typedef std::vector<term_t> TermList;
    typedef std::vector<TermList> Suffixes;
    typedef boost::unordered_set<TermList> KeywordSet;
    typedef std::vector<Term> ATermList;//analyze term list
    //typedef uint32_t position_t;
    //typedef std::pair<position_t, position_t>  Position;
    //typedef std::vector<Position> Positions;
    typedef boost::unordered_map<std::string, std::string> Back2Front;
    typedef std::vector<std::pair<uint32_t, uint32_t> > FrequentValue;
    typedef boost::unordered_map<TermList, FrequentValue> NgramFrequent;
    typedef boost::unordered_map<TermList, UString> KeywordText;
    typedef std::vector<std::pair<uint32_t, double> > FeatureVector;
    typedef std::vector<double> NFeatureVector;

    struct Position
    {
        Position(): begin(0), end(0)
        {
        }

        Position(uint32_t b, uint32_t e) : begin(b), end(e)
        {
        }

        uint32_t begin;
        uint32_t end;

        bool Combine(const Position& ap)
        {
            if( (begin<=ap.end&&begin>=ap.begin) || (ap.begin<=end&&ap.begin>=begin)) //overlap
            {
                begin = std::min(begin, ap.begin);
                end = std::max(end, ap.end);
                return true;
            }
            return false;
        }
        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive & ar, const unsigned int version)
        {
            ar & begin & end;
        }
    };

    struct WeightType
    {
        WeightType()
        :cweight(0.0), aweight(0.0), tweight(0.0), kweight(1.0)
         , paweight(0.0), paratio(0.0), type_match(false), brand_match(false)
         , price_diff(0.0)
        {
        }
        double cweight;
        double aweight;
        double tweight;
        double kweight;
        double paweight;
        double paratio;
        bool type_match;
        bool brand_match;
        double price_diff;
        //friend class boost::serialization::access;
        //template<class Archive>
        //void serialize(Archive & ar, const unsigned int version)
        //{
            //ar & cweight & aweight & tweight & kweight & paweight;
        //}

        inline double sum() const
        {
            return (cweight+aweight+tweight)*kweight;
        }

        bool operator<(const WeightType& another) const
        {
            return sum()<another.sum();
        }
        friend std::ostream& operator<<(std::ostream& out, const WeightType& wt)
        {
            out<<"cweight:"<<wt.cweight<<",aweight:"<<wt.aweight<<",tweight:"<<wt.tweight<<",kweight:"<<wt.kweight<<",paweight:"<<wt.paweight<<",paratio:"<<wt.paratio<<",typematch:"<<wt.type_match<<",brandmatch:"<<wt.brand_match<<",pricediff:"<<wt.price_diff;
            return out;
        }
    };

    struct ResultVectorItem
    {
        uint32_t cid;
        uint32_t spu_id;
        double score;
        WeightType weight;
        //double paweight;
        bool is_given_category;
        bool operator<(const ResultVectorItem& another) const
        {
            if(score==another.score)
            {
                return is_given_category;
            }
            return score>another.score;
        }
    };

    template<class K, class V>
    class dmap : public boost::unordered_map<K,V>
    {
        //typedef std::map<K,V> BaseType;
        typedef boost::unordered_map<K,V> BaseType;
    public:
        dmap():BaseType()
        {
        }
        dmap(const V& default_value):d_(default_value)
        {
        }

        V& operator[] (const K& k)
        {
            return (*((this->insert(make_pair(k,d_))).first)).second;
        }


    private:
        V d_;

    };
    template<class K, class V>
    class stdmap : public std::map<K,V>
    {
        typedef std::map<K,V> BaseType;
    public:
        stdmap():BaseType()
        {
        }
        stdmap(const V& default_value):d_(default_value)
        {
        }

        V& operator[] (const K& k)
        {
            return (*((this->insert(make_pair(k,d_))).first)).second;
        }

        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive & ar, const unsigned int version)
        {
            ar & *(dynamic_cast<BaseType*>(this));
        }

    private:
        V d_;

    };

    typedef dmap<std::string, double> StringToScore;
    struct CategoryNameApp
      : boost::less_than_comparable<CategoryNameApp>
       //,boost::equality_comparable<CategoryNameApp>
    {
        uint32_t cid;
        uint32_t depth;
        bool is_complete;
        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive & ar, const unsigned int version)
        {
            ar & cid & depth & is_complete;
        }

        bool operator<(const CategoryNameApp& another) const
        {
            return cid<another.cid;
            //if(cid<another.cid) return true;
            //else if(cid>another.cid) return false;
            //else if(depth<another.depth) return true;
            //else if(depth>another.depth) return false;
            //return false;
        }

        //bool operator==(const CategoryNameApp& another) const
        //{
            //return cid==another.cid&&depth==another.depth;
        //}
    };
    struct AttributeApp
      : boost::less_than_comparable<AttributeApp> ,boost::equality_comparable<AttributeApp>
    {
        uint32_t spu_id;
        std::string attribute_name;
        bool is_optional;
        double lprop; //length proportion

        //bool is_complete;
        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive & ar, const unsigned int version)
        {
            ar & spu_id & attribute_name & is_optional & lprop;
        }
        bool operator<(const AttributeApp& another) const
        {
            if(spu_id<another.spu_id) return true;
            else if(spu_id>another.spu_id) return false;
            return attribute_name<another.attribute_name;
        }

        bool operator==(const AttributeApp& another) const
        {
            return spu_id==another.spu_id&&attribute_name==another.attribute_name;
            //return spu_id==another.spu_id;
        }
    };

    struct OfferCategoryApp
    {
        uint32_t cid;
        uint32_t count;
        bool operator<(const OfferCategoryApp& another) const
        {
            return cid<another.cid;
        }
        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive & ar, const unsigned int version)
        {
            ar & cid & count;
        }
    };

    //struct FuzzyApp
    //{
        //uint32_t spu_id;
        //std::string attribute_name;
        //TermList term_list;
        //Position pos;
        //Position tpos; //runtime property
        //friend class boost::serialization::access;
        //template<class Archive>
        //void serialize(Archive & ar, const unsigned int version)
        //{
            //ar & spu_id & attribute_name & term_list & pos;
        //}

        //static bool PositionCompare(const FuzzyApp& a1, const FuzzyApp& a2)
        //{
            //if(a1.pos.begin<a2.pos.begin) return true;
            //else if(a1.pos.begin>a2.pos.begin) return false;
            //else return a1.pos.end<a2.pos.end;

        //}
    //};

    //struct FuzzyPositions
    //{
        //std::vector<Position> positions;
        //std::vector<Position> tpositions;
    //};

    //struct FuzzyValue
    //{
        //std::vector<FuzzyApp> fuzzy_apps;
        //friend class boost::serialization::access;
        //template<class Archive>
        //void serialize(Archive & ar, const unsigned int version)
        //{
            //ar & fuzzy_apps;
        //}
    //};

    struct TermIndexItem
      : boost::less_than_comparable<TermIndexItem> ,boost::equality_comparable<TermIndexItem>
    {
        uint32_t keyword_index;
        uint32_t pos;
        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive & ar, const unsigned int version)
        {
            ar & keyword_index & pos;
        }
        bool operator<(const TermIndexItem& another) const
        {
            if(keyword_index!=another.keyword_index) return keyword_index<another.keyword_index;
            else return pos<another.pos;
        }

        bool operator==(const TermIndexItem& another) const
        {
            return keyword_index==another.keyword_index && pos==another.pos;
        }
    };

    struct TermIndexValue
    {
        std::vector<TermIndexItem> items;
        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive & ar, const unsigned int version)
        {
            ar & items;
        }

        void flush()
        {
            std::sort(items.begin(), items.end());
            items.erase( std::unique(items.begin(), items.end()), items.end());
        }
    };

    struct TermIndex
    {
        typedef std::vector<ATermList> Forward;
        typedef boost::unordered_map<term_t, TermIndexValue> Invert;
        typedef boost::unordered_set<TermList> Set;

        Forward forward;
        Invert invert;
        Set set;//not serialized
        void clear()
        {
            forward.clear();
            invert.clear();
            set.clear();
        }
        void flush()
        {
            for(Invert::iterator it=invert.begin();it!=invert.end();it++)
            {
                it->second.flush();
            }
        }
        friend class boost::serialization::access;
        template<class Archive>
        void save(Archive & ar, const unsigned int version) const
        {
            ar & forward;
            std::map<typename Invert::key_type, typename Invert::mapped_type> m(invert.begin(), invert.end());
            ar & m;
        }
        template<class Archive>
        void load(Archive & ar, const unsigned int version)
        {
            ar & forward;
            std::map<typename Invert::key_type, typename Invert::mapped_type> m;
            ar & m;
            invert.insert(m.begin(), m.end());
        }
        BOOST_SERIALIZATION_SPLIT_MEMBER()
    };

    typedef boost::unordered_map<cid_t, TermIndex> TermIndexMap;

    struct FuzzyApp
    {
        std::vector<std::pair<uint32_t, uint32_t> > positions;
        uint32_t ilength;
        uint32_t count;
        //std::vector<uint32_t> kpos;
        //std::vector<uint32_t> tpos;
    };


    //typedef std::map<TermList, FuzzyValue> FuzzyTrie;
    //struct SpuTitleApp
      //: boost::less_than_comparable<SpuTitleApp> ,boost::equality_comparable<SpuTitleApp>
    //{
        //uint32_t spu_id;
        //uint32_t pstart;
        //uint32_t pend;
        //friend class boost::serialization::access;
        //template<class Archive>
        //void serialize(Archive & ar, const unsigned int version)
        //{
            //ar & spu_id & pstart & pend;
        //}
        //bool operator<(const SpuTitleApp& another) const
        //{
            //if(spu_id<another.spu_id) return true;
            //else if(spu_id>another.spu_id) return false;
            //else
            //{
                //return pstart<another.pstart;
            //}
        //}

        //bool operator==(const SpuTitleApp& another) const
        //{
            //return spu_id==another.spu_id&&pstart==another.pstart;
        //}
    //};
    typedef stdmap<std::string, uint32_t> KeywordTypeApp;
    typedef boost::unordered_map<cid_t, double> CategoryContributor;
    struct SpuContributorValue
    {
        SpuContributorValue()
        : paweight(0.0), model_match(false), brand_match(false)
        {
        }
        //double cweight;
        //double aweight;
        //double tweight;
        //double kweight;
        double paweight;
        //double lenweight;
        bool model_match;
        bool brand_match;
        boost::unordered_map<std::string, double> attribute_lenweight;
        double GetLenWeight() const
        {
            double lw = 0.0;
            for(boost::unordered_map<std::string, double>::const_iterator it = attribute_lenweight.begin(); it!=attribute_lenweight.end(); ++it)
            {
                lw += it->second;
            }
            return lw;
        }
        //bool IsAttributeFound(const std::string& a) const
        //{
            //return attribute_found.find(a)!=attribute_found.end();
        //}
        //double price_diff;
        //friend class boost::serialization::access;
        //template<class Archive>
        //void serialize(Archive & ar, const unsigned int version)
        //{
            //ar & cweight & aweight & tweight & kweight & paweight;
        //}

        //inline double sum() const
        //{
            //return (cweight+aweight+tweight)*kweight;
        //}

        //bool operator<(const WeightType& another) const
        //{
            //return sum()<another.sum();
        //}
        //friend std::ostream& operator<<(std::ostream& out, const WeightType& wt)
        //{
            //out<<"cweight:"<<wt.cweight<<",aweight:"<<wt.aweight<<",tweight:"<<wt.tweight<<",kweight:"<<wt.kweight<<",paweight:"<<wt.paweight<<",paratio:"<<wt.paratio<<",typematch:"<<wt.type_match<<",brandmatch:"<<wt.brand_match<<",pricediff:"<<wt.price_diff;
            //return out;
        //}
    };
    typedef boost::unordered_map<uint32_t, SpuContributorValue> SpuContributor;
    struct SpuMatchCandidate
    {
        uint32_t spuid;
        uint32_t cid_index;
        double paweight;
        double lenweight;
        double price_diff;
        bool operator<(const SpuMatchCandidate& another) const
        {
            if(cid_index!=another.cid_index) return cid_index<another.cid_index;
            if(paweight!=another.paweight) return paweight>another.paweight;
            if(lenweight!=another.lenweight) return lenweight>another.lenweight;
            return price_diff<another.price_diff;
        }
    };
    struct KeywordTag
    {
        KeywordTag();
        uint32_t id;
        TermList term_list;
        UString text;
        std::vector<CategoryNameApp> category_name_apps;
        std::vector<AttributeApp> attribute_apps;
        std::vector<OfferCategoryApp> offer_category_apps;
        //std::vector<SpuTitleApp> spu_title_apps;
        KeywordTypeApp type_app;
        //double cweight; //not serialized, runtime property
        //double aweight;
        double kweight;
        uint8_t ngram;
        std::vector<Position> positions;

        void PositionMerge(const Position& pos);

        static bool WeightCompare(const KeywordTag& k1, const KeywordTag& k2)
        {
            return k1.kweight>k2.kweight;
        }

        template<class T>
        void SortAndUnique(std::vector<T>& vec)
        {
            std::sort(vec.begin(), vec.end());
            vec.erase(std::unique(vec.begin(), vec.end()), vec.end());
        }
        void Flush();
        void Append(const KeywordTag& another, bool is_complete);
        bool Combine(const KeywordTag& another);
        bool IsAttribSynonym(const KeywordTag& another) const;
        bool IsBrand() const;
        bool IsModel() const;
        bool IsCategoryKeyword() const;
        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive & ar, const unsigned int version)
        {
            ar & id & term_list & text & category_name_apps & attribute_apps & offer_category_apps & type_app & kweight;
        }
    };

    typedef std::vector<KeywordTag> KeywordVector;
    typedef std::map<std::vector<term_t>, KeywordTag> TrieType;
    struct FuzzyKey
    {
        FuzzyKey(){}
        FuzzyKey(uint32_t pcid, const std::vector<uint32_t>& pname_ids, uint32_t pdid, const std::vector<uint32_t>& pvalue_ids)
        : cid(pcid), name_ids(pname_ids), did(pdid), value_ids(pvalue_ids)
        {
        }

        std::vector<uint32_t> GenKey() const
        {
            std::vector<uint32_t> key(1, cid);
            key.insert(key.end(), name_ids.begin(), name_ids.end());
            key.push_back(0);
            key.push_back(did);
            key.insert(key.end(), value_ids.begin(), value_ids.end());
            return key;
        }

        void Parse(const std::vector<uint32_t>& key)
        {
            cid = key[0];
            uint32_t i=1;
            for(;i<key.size();i++)
            {
                if(key[i]==0) break;
                name_ids.push_back(key[i]);
            }
            ++i;
            did = key[i++];
            for(;i<key.size();i++)
            {
                value_ids.push_back(key[i]);
            }
        }
        uint32_t cid;
        std::vector<uint32_t> name_ids;
        uint32_t did;//direction id;
        std::vector<uint32_t> value_ids;
    };
    typedef uint32_t PidType;
    typedef std::map<std::string, uint32_t> CategoryIndex;
    typedef std::map<std::string, uint32_t> ProductIndex;
    typedef std::vector<uint32_t> IdList;
    typedef std::map<uint32_t, IdList> IdToIdList;
    typedef boost::unordered_map<uint32_t, UString> IdManager;

    ProductMatcher();
    ~ProductMatcher();
    bool IsOpen() const;
    bool Open(const std::string& path);
    void ForceOpen()
    {
        is_open_ = true;
    }
    bool OpenPsm(const std::string& path);
    //static void Clear(const std::string& path, int mode=3);
    static std::string GetVersion(const std::string& path);
    static std::string GetAVersion(const std::string& path);
    static std::string GetRVersion(const std::string& path);
    bool Index(const std::string& path, const std::string& scd_path, int mode, int thread_num);
    bool IndexPost(const std::string& path, const std::string& scd_path, int thread_num);
    void Test(const std::string& scd_path);
    bool OutputCategoryMap(const std::string& scd_path, const std::string& output_file);
    bool DoMatch(const std::string& scd_path, const std::string& output_file="");
    bool FuzzyDiff(const std::string& scd_path, const std::string& output_file="");
    bool Process(const Document& doc, Product& result_product, bool use_fuzzy = false, bool use_psm=true);
    bool Process(const Document& doc, uint32_t limit, std::vector<Product>& result_products, bool use_fuzzy = false, bool use_psm=true);
    void GetFrontendCategory(const UString& text, uint32_t limit, std::vector<UString>& results);
    static bool GetIsbnAttribute(const Document& doc, std::string& isbn);
    static bool ProcessBookStatic(const Document& doc, Product& result_product);
    bool ProcessBook(const Document& doc, Product& result_product);
    bool GetProduct(const std::string& pid, Product& product);
    static void ParseAttributes(const UString& ustr, std::vector<Attribute>& attributes);
    static void MergeAttributes(std::vector<Attribute>& eattributes, const std::vector<Attribute>& attributes);
    static UString AttributesText(const std::vector<Attribute>& attributes);
    //return true if this is a complete match, else false: to return parent nodes
    bool GetFrontendCategory(UString& backend, UString& frontend) const;
    bool GetKeyword(const UString& text, KeywordTag& tag);
    void GetKeywords(const ATermList& term_list, KeywordVector& keyword_vector, bool bfuzzy = false, cid_t cid=0);
    void GetKeywords(const UString& text, KeywordVector& keywords);
    void ExtractKeywordsFromPage(const UString& text, std::list<std::pair<UString, std::pair<uint32_t, uint32_t> > >&res_ca, std::list<std::pair<UString, std::pair<uint32_t, uint32_t> > >&res_brand, std::list<std::pair<UString, std::pair<uint32_t, uint32_t> > >&res_model);
    void ExtractKeywordsFromPage(const UString& text, std::list<std::pair<UString, std::pair<uint32_t, uint32_t> > >& res);
    void ExtractKeywords(const UString& text, KeywordVector& keywords);
    void GetSearchKeywords(const UString& text, std::list<std::pair<UString, double> >& hits, std::list<std::pair<UString, double> >& left_hits, std::list<UString>& left);
    bool GetSynonymSet(const UString& pattern, std::vector<UString>& synonym_set, int& setid);
    bool GetSynonymId(const UString& pattern, int& setid);
    void UpdateSynonym(const std::string& dict_path);
    void PendingFinish(const boost::function<void (ScdDocument&)>& func, int thread_num=1);

    
    void SetCmaPath(const std::string& path)
    { cma_path_ = path; }
    bool IsIndexDone() const;

    void SetUsePriceSim(bool sim)
    { use_price_sim_ = sim; }

    void SetUsePsm(bool b)
    {
        use_psm_ = b;
    }

    
    ilplib::knlp::AttributeNormalize* GetAttributeNormalize() const
    {
        if(psm_==NULL) return NULL;
        return psm_->GetAttributeNormalize();
    }

    //if given category empty, do SPU matching only
    void SetMatcherOnly(bool m)
    { matcher_only_ = m; }

    void SetCategoryMaxDepth(uint16_t d)
    {
        category_max_depth_ = d;
        LOG(INFO)<<"set category max depth to "<<d<<std::endl;
    }

    const Product& GetProduct(uint32_t spu_id) const
    {
        return products_[spu_id];
    }

    const Category& GetCategory(uint32_t cid) const
    {
        return category_list_[cid];
    }

    static void CategoryDepthCut(std::string& category, uint16_t d);
    bool IsBrand(const std::string& scategory, const KeywordTag& k) const
    {
        return IsBrand_(scategory, k);
    }

private:
    static void SetIndexDone_(const std::string& path, bool b);
    static bool IsIndexDone_(const std::string& path);
    void Init_();
    double PriceSim_(double offerp, double spup) const;
    bool IsValuePriceSim_(double op, double p) const;
    bool IsPriceSim_(const ProductPrice& op, const ProductPrice& p) const;
    ProductPrice GetProductPriceRange_(const ProductPrice& p, const std::vector<ProductPrice>& offer_prices) const;
    ProductPrice GetProductPriceRange_(const ProductPrice& p) const;
    double PriceDiff_(double op, double p) const;
    double PriceDiff_(const ProductPrice& op, const ProductPrice& p) const;
    void AnalyzeNoSymbol_(const izenelib::util::UString& text, std::vector<Term>& result);
    void Analyze_(const izenelib::util::UString& text, std::vector<Term>& result);
    //void AnalyzeCR_(const izenelib::util::UString& text, std::vector<izenelib::util::UString>& result);

    //void AnalyzeImpl_(idmlib::util::IDMAnalyzer* analyzer, const izenelib::util::UString& text, std::vector<izenelib::util::UString>& result);


    void GetPsmKeywords_(const KeywordVector& keywords, const std::string& scategory, std::vector<std::string>& brands, std::vector<std::string>& ckeywords) const;
    void IndexOffer_(const std::string& offer_scd, int thread_num);
    void OfferProcess_(ScdDocument& doc);
    void PostProcess_(ScdDocument& doc);
    void IndexFuzzy_();
    void ProcessFuzzy_(const std::pair<uint32_t, uint32_t>& range);

    void GenSuffixes_(const std::vector<term_t>& term_list, Suffixes& suffixes);
    void GenSuffixes_(const std::string& text, Suffixes& suffixes);
    void ConstructSuffixTrie_(TrieType& trie);
    term_t GetTerm_(const UString& text);
    term_t GetTerm_(const std::string& text);
    std::string GetText_(const TermList& tl, const std::string& s = "") const;
    std::string GetText_(const term_t& term) const;
    void GetTerms_(const std::string& text, std::vector<term_t>& term_list);
    void GetTerms_(const UString& text, std::vector<term_t>& term_list);
    void GetTermsString_(const UString& text, std::string& str);//analyzing first, then combine
    bool NeedFuzzy_(const std::string& value);
    void ConstructKeywords_();
    void AddKeyword_(const UString& text);
    void ConstructKeywordTrie_(const TrieType& suffix_trie);
    void GetFuzzyKeywords_(const ATermList& term_list, KeywordVector& keyword_vector, cid_t cid);
    void SearchKeywordsFilter_(std::vector<KeywordTag>& keywords);
    bool EqualOrIsParent_(uint32_t parent, uint32_t child) const;
    void GenCategoryContributor_(const KeywordTag& tag, CategoryContributor& cc);
    void MergeCategoryContributor_(CategoryContributor& cc, const CategoryContributor& cc2);
    void GenSpuContributor_(const KeywordTag& tag, SpuContributor& sc);
    void Compute_(const Document& doc, const std::vector<Term>& term_list, KeywordVector& keyword_vector, uint32_t limit, std::vector<Product>& p);
    void Compute2_(const Document& doc, const std::vector<Term>& term_list, KeywordVector& keywords, uint32_t limit, std::vector<Product>& result_products, std::string& why);
    void ComputeT_(const Document& doc, const std::vector<Term>& term_list, KeywordVector& keywords, uint32_t limit, std::vector<Product>& result_products);
    uint32_t GetCidBySpuId_(uint32_t spu_id);
    uint32_t GetCidByMaxDepth_(uint32_t cid);
    cid_t GetCid_(const UString& category) const;

    bool IsSpuMatch_(const Product& p, const SpuContributorValue& scv, double query_len) const;
    bool SpuMatched_(const WeightType& weight, const Product& p) const;
    int SelectKeyword_(const KeywordTag& tag1, const KeywordTag& tag2) const;
    bool IsFuzzyMatched_(const ATermList& keyword, const FuzzyApp& app) const;

    cid_t GetLevelCid_(const std::string& scategory, uint32_t level) const;
    void GenFeatureVector_(const std::vector<KeywordTag>& keywords, FeatureVector& feature_vector) const;
    void FeatureVectorAdd_(FeatureVector& o, const FeatureVector& a) const;
    void FeatureVectorNorm_(FeatureVector& v) const;
    void GenFeatureVector_(const std::vector<KeywordTag>& keywords, NFeatureVector& feature_vector) const;
    void FeatureVectorAdd_(NFeatureVector& o, const NFeatureVector& a) const;
    void FeatureVectorAdd_(NFeatureVector& o, const std::vector<KeywordTag>& keywords) const;
    void FeatureVectorNorm_(NFeatureVector& v) const;
    void FeatureVectorConvert_(const NFeatureVector& n, FeatureVector& v) const;
    double Cosine_(const FeatureVector& v1, const FeatureVector& v2) const;

    static uint32_t TextLength_(const std::string& text)
    {
        UString u(text, UString::UTF_8);
        return u.length();
    }

    template<class K, class V>
    void GetSortedVector_(const std::map<K, V>& map, std::vector<std::pair<V, K> >& vec, uint32_t max=0)
    {
        for(typename std::map<K,V>::const_iterator it = map.begin();it!=map.end();it++)
        {
            vec.push_back(std::make_pair(it->second, it->first));
        }
        std::sort(vec.begin(), vec.end(), std::greater<std::pair<V, K> >());
        if(max>0&&vec.size()>max)
        {
            vec.resize(max);
        }
    }

    template<class T>
    void SortVectorDesc_(std::vector<T>& vec)
    {
        std::sort(vec.begin(), vec.end(), std::greater<T>());
    }

    template<class T>
    static bool StartsWith_(const std::vector<T>& v1, const std::vector<T>& v2)
    {
        if(v2.size()>v1.size()) return false;
        for(uint32_t i=0;i<v2.size();i++)
        {
            if(v2[i]!=v1[i]) return false;
        }
        return true;
    }

    bool IsSymbol_(const Term& term) const
    {
        if(term.tag == idmlib::util::IDMTermTag::SYMBOL)
        {
            if(!IsTextSymbol_(term.id)) return true;
        }
        return false;
        //return symbol_terms_.find(term)!=symbol_terms_.end();
    }

    //bool IsSymbol_(term_t term) const
    //{
        //return symbol_terms_.find(term)!=symbol_terms_.end();
    //}

    bool IsConnectSymbol_(term_t term) const
    {
        return connect_symbols_.find(term)!=connect_symbols_.end();
    }

    bool IsTextSymbol_(term_t term) const
    {
        return text_symbols_.find(term)!=text_symbols_.end();
    }

    bool IsBlankSplit_(const UString& t1, const UString& t2) const;

    void SetProductsId_()
    {
        for(uint32_t i=0;i<products_.size();i++)
        {
            products_[i].id = i;
        }
    }

    void SetProductsPrice_(bool use_offer);

    bool IsBrand_(const std::string& scategory, const KeywordTag& k) const;


private:
    std::string path_;
    bool is_open_;
    std::string cma_path_;
    bool use_price_sim_;
    bool matcher_only_;
    uint16_t category_max_depth_;
    bool use_ngram_;
    idmlib::sim::StringSimilarity string_similarity_;
    IdManager id_manager_;
    idmlib::util::IDMAnalyzer* analyzer_;
    idmlib::util::IDMAnalyzer* char_analyzer_;
    idmlib::util::IDMAnalyzer* chars_analyzer_;
    std::string test_docid_;
    //boost::unordered_set<term_t> symbol_terms_;
    boost::unordered_set<term_t> text_symbols_;
    std::string left_bracket_;
    std::string right_bracket_;
    std::string place_holder_;
    std::string blank_;
    term_t left_bracket_term_;
    term_t right_bracket_term_;
    term_t place_holder_term_;
    term_t blank_term_;
    boost::unordered_set<term_t> connect_symbols_;
    std::vector<Product> products_;
    std::vector<std::string> keywords_thirdparty_;
    KeywordSet not_keywords_;
    std::vector<Category> category_list_;
    std::vector<IdList> cid_to_pids_;
    CategoryIndex category_index_;
    ProductIndex product_index_;
    KeywordSet keyword_set_;
    TrieType trie_;
    KeywordText keyword_text_;//only use in training, not stemmed
    //FuzzyTrie ftrie_;
    //TermIndex term_index_;
    TermIndexMap term_index_map_;
    Back2Front back2front_;
    KeywordVector all_keywords_; //not serialized
    boost::regex type_regex_;
    boost::regex vol_regex_;
    std::string book_category_;

    std::map<string, size_t> synonym_map_;
    std::vector<std::vector<string> > synonym_dict_;
    izenelib::util::ReadWriteLock lock_;
    boost::mutex offer_mutex_;

    std::vector<FeatureVector> feature_vectors_;
    std::vector<NFeatureVector> nfeature_vectors_;
    std::vector<uint32_t> fv_count_;
    std::vector<std::vector<uint32_t> > oca_;
    boost::unordered_map<cid_t, uint32_t> first_level_category_;
    //NgramFrequent nf_;
    bool use_psm_;
    //std::vector<CategoryPsm*> psms_;
    CategoryPsm* psm_;
    BrandManager* brand_manager_;
    boost::unordered_map<uint128_t, uint128_t> psm_result_;
    boost::unordered_map<std::string, std::vector<ProductPrice> > spu_offer_prices_;
    bool offer_prices_finish_;

    const static double optional_weight_ = 0.2;
    const static std::string AVERSION;

};

class ProductMatcherInstance : public izenelib::util::Singleton<ProductMatcher>
{
};
NS_IDMLIB_B5M_END

#endif
