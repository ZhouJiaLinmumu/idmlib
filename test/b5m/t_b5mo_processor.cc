#include <boost/test/unit_test.hpp>
#include <boost/lexical_cast.hpp>
#include <idmlib/b5m/b5mo_processor.h>
#include <idmlib/b5m/b5mp_processor2.h>
#include "TestResources.h"

namespace idmlib { namespace b5m {
struct MapperData
{
    MapperData(){}
    MapperData(const std::string& p1, const std::string& p2, const std::string& p3)
    :source(p1), original(p2), category(p3)
    {
    }
    std::string source;
    std::string original;
    std::string category;
};

void SetProperty(Document& doc, const std::string& name, const std::string& value)
{
    std::string svalue = value;
    if(name=="DOCID" || name=="uuid")
    {
        while(svalue.length()<32)
        {
            svalue = "0"+svalue;
        }
    }
    doc.property(name) = str_to_propstr(svalue);
}
ScdDocument GenDoc(SCD_TYPE type, const std::string& docid, const std::string& title, const std::string& source, const std::string& original, const std::string& isbn)
{
    ScdDocument doc;
    doc.type = type;
    SetProperty(doc, "DOCID", docid);
    SetProperty(doc, "Title", title);
    SetProperty(doc, "Source", source);
    SetProperty(doc, "OriginalCategory", original);
    if(!isbn.empty())
    {
        std::string attrib = "ISBN:"+isbn;
        SetProperty(doc, "Attribute", attrib);
    }
    return doc;
}

ScdDocument GenODoc(SCD_TYPE type, const std::string& docid, const std::string& title, const std::string& source, const std::string& original, const std::string& isbn, const std::string& pid)
{
    ScdDocument doc;
    doc.type = type;
    SetProperty(doc, "DOCID", docid);
    SetProperty(doc, "Title", title);
    SetProperty(doc, "Source", source);
    SetProperty(doc, "Category", original);
    if(!isbn.empty())
    {
        std::string attrib = "ISBN:"+isbn;
        SetProperty(doc, "Attribute", attrib);
    }
    if(!pid.empty())
    {
        SetProperty(doc, "uuid", pid);
    }
    return doc;
}
ScdDocument GenPDoc(SCD_TYPE type, const std::string& docid, const std::string& title, const std::string& source, const std::string& original, int64_t itemcount)
{
    ScdDocument doc;
    doc.type = type;
    SetProperty(doc, "DOCID", docid);
    SetProperty(doc, "Title", title);
    SetProperty(doc, "Source", source);
    SetProperty(doc, "Category", original);
    if(itemcount>0)
    {
        SetProperty(doc, "itemcount", boost::lexical_cast<std::string>(itemcount));
    }
    //if(!isbn.empty())
    //{
        //std::string attrib = "ISBN:"+isbn;
        //SetProperty(doc, "Attribute", attrib);
    //}
    return doc;
}

struct TestItem
{
    std::vector<ScdDocument> input_docs;
    std::vector<MapperData> omapper;
    std::vector<ScdDocument> odocs;
    std::vector<ScdDocument> pdocs;

};

class ScdTester
{
public:
    ScdTester(): step_(0), mdb_("./mdb")
    {
        //mdb_ = boost::filesystem::absolute(boost::filesystem::path(mdb_)).string();
        B5MHelper::PrepareEmptyDir(mdb_);
    }

    ~ScdTester()
    {
        //boost::filesystem::remove_all(mdb_);
    }

    void Test(TestItem& item)
    {
        int step = ++step_;
        int mode = 0;
        if(step==1) mode = 1;
        std::cerr<<"test step "<<step<<", mode "<<mode<<std::endl;
        string input_dir="./input_scd";
        B5MHelper::PrepareEmptyDir(input_dir);
        {
            ScdTypeWriter writer(input_dir);
            for(uint32_t i=0;i<item.input_docs.size();i++)
            {
                writer.Append(item.input_docs[i]);
            }
            writer.Close();
        }
        std::string mdb_instance = mdb_+"/"+mdb_ts(step);
        std::cerr<<"mdb dir "<<mdb_instance<<std::endl;
        B5mM b5mm;
        b5mm.path = mdb_instance;
        //set b5mm value
        b5mm.mode = mode;
        b5mm.thread_num = 2;
        b5mm.scd_path = input_dir;
        b5mm.Gen();
        processor_.reset(new B5moProcessor(b5mm));
        boost::filesystem::create_directories(mdb_instance);
        std::string omapper_path = B5MHelper::GetOMapperPath(mdb_instance);
        if(!item.omapper.empty())
        {
            boost::filesystem::create_directories(omapper_path);
            std::string omapperdata_path = omapper_path+"/data";
            std::ofstream ofs(omapperdata_path.c_str());
            for(uint32_t i=0;i<item.omapper.size();i++)
            {
                MapperData& mdata = item.omapper[i];
                ofs<<mdata.source<<","<<mdata.original<<":"<<mdata.category<<std::endl;
            }
            ofs.close();
        }
        BOOST_CHECK(processor_->Generate(mdb_instance, last_mdb_instance_));
        B5mpProcessor2 pprocessor(b5mm);
        BOOST_CHECK(pprocessor.Generate(mdb_instance, last_mdb_instance_));
        std::vector<ScdDocument> odocs;
        GetAllDocs(B5MHelper::GetB5moPath(mdb_instance), odocs);
        std::cerr<<"start to do batch test on b5mo"<<std::endl;
        DocBatchTest(odocs, item.odocs);
        std::vector<ScdDocument> pdocs;
        GetAllDocs(B5MHelper::GetB5mpPath(mdb_instance), pdocs);
        std::cerr<<"start to do batch test on b5mp"<<std::endl;
        DocBatchTest(pdocs, item.pdocs);




        last_mdb_instance_ = mdb_instance;

    }

    static bool DocidCompare(const ScdDocument& doc1, const ScdDocument& doc2)
    {
        std::string s1;
        std::string s2;
        doc1.getString("DOCID", s1);
        doc2.getString("DOCID", s2);
        return s1<s2;
    }

    static void GetAllDocs(const std::string& dir, std::vector<ScdDocument>& docs)
    {
        std::vector<std::string> scd_list;
        ScdParser::getScdList(dir, scd_list);
        for(uint32_t s=0;s<scd_list.size();s++)
        {
            std::string scd_file = scd_list[s];
            ScdParser parser(izenelib::util::UString::UTF_8);
            parser.load(scd_file);
            SCD_TYPE scd_type = ScdParser::checkSCDType(scd_file);
            for( ScdParser::iterator doc_iter = parser.begin();
              doc_iter!= parser.end(); ++doc_iter)
            {
                SCDDoc& scddoc = *(*doc_iter);
                SCDDoc::iterator p = scddoc.begin();
                ScdDocument doc;
                doc.type = scd_type;
                for(; p!=scddoc.end(); ++p)
                {
                    const std::string& property_name = p->first;
                    doc.property(property_name) = p->second;
                }
                docs.push_back(doc);
            }
        }
    }

    static void DocBatchTest(std::vector<ScdDocument>& my,std::vector<ScdDocument>& ref)
    {
        std::stable_sort(my.begin(), my.end(), DocidCompare);
        std::stable_sort(ref.begin(), ref.end(), DocidCompare);
        BOOST_CHECK_EQUAL(my.size(), ref.size());
        typedef std::vector<ScdDocument>::iterator iterator;
        for(uint32_t i=0;i<my.size();i++)
        {
            ScdDocument& m = my[i];
            std::string mid;
            m.getString("DOCID", mid);
            std::pair<iterator, iterator> result = std::equal_range(ref.begin(), ref.end(), m, DocidCompare);
            bool match = false;
            if(result.first<result.second)
            {
                for(iterator it = result.first;it<result.second;++it)
                {
                    if(DocTest(m, *it))
                    {
                        match = true;
                        break;
                    }
                }
            }
            else
            {
                std::cerr<<"doc not found in ref for "<<mid<<std::endl;
            }
            BOOST_CHECK_MESSAGE(match, mid);
        }
    }

    static bool DocTest(const ScdDocument& m, const ScdDocument& r)
    {
        if(m.type!=r.type) 
        {
            std::cerr<<"doc type not match"<<std::endl;
            return false;
        }
        for(ScdDocument::property_const_iterator it = r.propertyBegin(); it!=r.propertyEnd();++it)
        {
            ScdDocument::property_const_iterator it2 = m.findProperty(it->first);
            if(it2==m.propertyEnd()) 
            {
                if(it->second.get<Document::str_type>().empty()) continue;
                std::cerr<<"can not find property "<<it->first<<std::endl;
                return false;
            }
            Document::str_type pstr1 = it->second.get<Document::str_type>();
            Document::str_type pstr2 = it2->second.get<Document::str_type>();
            std::string str1 = propstr_to_str(pstr1);
            std::string str2 = propstr_to_str(pstr2);
            if(it->first=="Source")
            {
                std::vector<std::string> vec1;
                std::vector<std::string> vec2;
                boost::algorithm::split(vec1, str1, boost::algorithm::is_any_of(","));
                boost::algorithm::split(vec2, str2, boost::algorithm::is_any_of(","));
                std::sort(vec1.begin(), vec1.end());
                std::sort(vec2.begin(), vec2.end());
                if(vec1!=vec2) 
                {
                    std::cerr<<"Source not match"<<std::endl;
                    return false;
                }
            }
            else
            {
                std::vector<std::string> vec1;
                std::vector<std::string> vec2;
                boost::algorithm::split(vec1, str1, boost::algorithm::is_any_of("|"));
                boost::algorithm::split(vec2, str2, boost::algorithm::is_any_of("|"));
                if(vec1.empty()) vec1.push_back("");
                if(vec2.empty()) vec2.push_back("");
                bool find_equal = false;
                for(uint32_t i=0;i<vec1.size();i++)
                {
                    for(uint32_t j=0;j<vec2.size();j++)
                    {
                        if(vec1[i]==vec2[j])
                        {
                            find_equal = true;
                            break;
                        }
                    }
                    if(find_equal) break;
                }
                if(!find_equal) 
                {
                    std::cerr<<"property "<<it->first<<" not match: "<<it->second<<","<<it2->second<<std::endl;
                    return false;
                }
            }
        }
        return true;
    }

    std::string mdb_ts(int step) const
    {
        std::string prefix = "2000010100000";
        return prefix+boost::lexical_cast<std::string>(step);
    }

private:
    boost::shared_ptr<B5moProcessor> processor_;
    int step_;
    std::string mdb_;
    std::string last_mdb_instance_;

};

string toString(UString us)
{
    string str;
    us.convertString(str, izenelib::util::UString::UTF_8);
    return str;
}

void ProcessVector(ProductMatcher &matcher,vector<Document> docvec)
{
    Product result_product;
    for(unsigned i=0; i<docvec.size(); i++)
    {
        cout<<i<<endl;
        matcher.Process(docvec[i], result_product);
    }
}


string getTitle(Document doc)
{
    Document::doc_prop_value_strtype utitle;
    doc.getProperty("Title", utitle);
    return propstr_to_str(utitle);
}

string getSource(Document doc)
{
    Document::doc_prop_value_strtype usource;
    doc.getProperty("Source", usource);
    return propstr_to_str(usource);
}

string get(Document doc,string prop)
{
    Document::doc_prop_value_strtype ustr;
    doc.getProperty(prop, ustr);
    return propstr_to_str(ustr);
}
void check(Document doc,string source,string price,string title,string category,string attrubute,string mobile)//,uint128_t docid,uint128_t uuid
{
    BOOST_CHECK_EQUAL(get(doc,"Source"),source);
    //  BOOST_CHECK_EQUAL(doc.property("DOCID")==docid,true);
    //  BOOST_CHECK_EQUAL(doc.property("uuid")==uuid,true);
    BOOST_CHECK_EQUAL(get(doc,"Price"),price);
    BOOST_CHECK_EQUAL(get(doc,"Title"),title);
    BOOST_CHECK_EQUAL(get(doc,"Category"),category);
    //BOOST_CHECK_EQUAL(get(doc,"Attribute"),attrubute);
    BOOST_CHECK_EQUAL(doc.hasProperty("mobile"),mobile.length()>0);

}


void show(Document doc)
{
    cout<<get(doc,"Source")<<" "<<doc.property("DOCID")<<" "<<doc.property("uuid")<<" "<<get(doc,"Price")<<"  "<<get(doc,"Title")<<"  "<<get(doc,"Category")<<" "<<get(doc,"Attribute")<<"  mobile "<<doc.property("mobile") <<endl;
}
void check(Product product,string fcategory,string scategory,string sbrand)
{
    BOOST_CHECK_EQUAL(product.scategory.find(scategory)==string::npos,false);
    BOOST_CHECK_EQUAL(product.fcategory.find(fcategory)==string::npos,false);
    BOOST_CHECK_EQUAL(product.sbrand,sbrand);
}


void show(Product product)
{
    cout<<"product"<<product.spid<<"title"<< product.stitle<<"attributes"<<product.GetAttributeValue()<<"frontcategory"<<product.fcategory<<"scategory"<<product.scategory<<"price"<<product.price<<"brand"<<product.sbrand<<endl;
}


BOOST_AUTO_TEST_SUITE(b5mo_processor_test)


BOOST_AUTO_TEST_CASE(b5mo_omap_test)
{
    ScdTester tester;
    {
        TestItem item;
        item.input_docs.push_back(
        GenDoc(UPDATE_SCD,"aaa", "TITLE1","AMAZON"
            , "手机",""));
        item.input_docs.push_back(
        GenDoc(UPDATE_SCD,"bbb", "TITLE2","JD"
            , "电视机",""));
        item.input_docs.push_back(
        GenDoc(UPDATE_SCD,"ccc", "BOOK1","DD"
            , "教材","101"));
        item.input_docs.push_back(
        GenDoc(UPDATE_SCD,"ddd", "BOOK2","DD"
            , "小说","103"));
        item.input_docs.push_back(
        GenDoc(UPDATE_SCD,"eee", "BOOK11","PUB"
            , "英语教材","101"));
        item.input_docs.push_back(
        GenDoc(UPDATE_SCD,"111", "BOOKA","PUB"
            , "书籍","999"));
        item.input_docs.push_back(
        GenDoc(UPDATE_SCD,"222", "BOOKB","京东商城"
            , "书籍","999"));
        item.input_docs.push_back(
        GenDoc(UPDATE_SCD,"333", "BOOKC","京东商城"
            , "书籍","999"));
        item.input_docs.push_back(
        GenDoc(UPDATE_SCD,"444", "BOOKD","京东商城"
            , "书籍","999"));
        item.input_docs.push_back(
        GenDoc(UPDATE_SCD,"555", "BOOKE","卓越亚马逊"
            , "书籍","999"));
        item.input_docs.push_back(
        GenDoc(UPDATE_SCD,"666", "BOOKF","DD"
            , "书籍","999"));
        item.omapper.push_back(MapperData("AMAZON", "手机", "手机数码>手机"));
        item.omapper.push_back(MapperData("JD", "电视机", "大家电>平板电视"));
        item.odocs.push_back(
        GenODoc(UPDATE_SCD,"aaa", "TITLE1","AMAZON"
            , "手机数码>手机>","", "aaa"));
        item.odocs.push_back(
        GenODoc(UPDATE_SCD,"bbb", "TITLE2","JD"
            , "大家电>平板电视>","", "bbb"));
        item.odocs.push_back(
        GenODoc(UPDATE_SCD,"ccc", "BOOK1","DD"
            , "","", B5MHelper::GetPidByIsbn("101")));
        item.odocs.push_back(
        GenODoc(UPDATE_SCD,"ddd", "BOOK2","DD"
            , "","", B5MHelper::GetPidByIsbn("103")));
        item.odocs.push_back(
        GenODoc(UPDATE_SCD,"eee", "BOOK11","PUB"
            , "","", B5MHelper::GetPidByIsbn("101")));
        item.odocs.push_back(
        GenODoc(UPDATE_SCD,"111", "BOOKA","PUB"
            , "","", B5MHelper::GetPidByIsbn("999")));
        item.odocs.push_back(
        GenODoc(UPDATE_SCD,"222", "BOOKB","京东商城"
            , "","", B5MHelper::GetPidByIsbn("999")));
        item.odocs.push_back(
        GenODoc(UPDATE_SCD,"333", "BOOKC","京东商城"
            , "","", B5MHelper::GetPidByIsbn("999")));
        item.odocs.push_back(
        GenODoc(UPDATE_SCD,"444", "BOOKD","京东商城"
            , "","", B5MHelper::GetPidByIsbn("999")));
        item.odocs.push_back(
        GenODoc(UPDATE_SCD,"555", "BOOKE","卓越亚马逊"
            , "","", B5MHelper::GetPidByIsbn("999")));
        item.odocs.push_back(
        GenODoc(UPDATE_SCD,"666", "BOOKF","DD"
            , "","", B5MHelper::GetPidByIsbn("999")));

        //item.pdocs.push_back(
        //GenPDoc(UPDATE_SCD,"aaa", "TITLE1","AMAZON"
        //    , "手机数码>手机>",1));
        //item.pdocs.push_back(
        //GenPDoc(UPDATE_SCD,"bbb", "TITLE2","JD"
        //    , "大家电>平板电视>",1));
        item.pdocs.push_back(
        GenPDoc(UPDATE_SCD,B5MHelper::GetPidByIsbn("101"), "BOOK1|BOOK11","DD,PUB"
            ,"",2));
        //item.pdocs.push_back(
        //GenPDoc(UPDATE_SCD,B5MHelper::GetPidByIsbn("103"), "BOOK2","DD"
        //    ,"",1));
        item.pdocs.push_back(
        GenPDoc(UPDATE_SCD,B5MHelper::GetPidByIsbn("999"), "BOOKA|BOOKB|BOOKC|BOOKD|BOOKE|BOOKF","PUB,京东商城,卓越亚马逊,DD"
            ,"",6));
        item.pdocs.insert(item.pdocs.end(), item.odocs.begin(), item.odocs.end());
        tester.Test(item);
    }
    {
        TestItem item;
        item.input_docs.push_back(
        GenDoc(UPDATE_SCD,"bbb", "TITLE3","JD"
            , "电视机",""));
        item.input_docs.push_back(
        GenDoc(UPDATE_SCD,"eee", "BOOK12","PUB"
            , "英语教材","109"));
        item.input_docs.push_back(
        GenDoc(DELETE_SCD,"111", "",""
            , "",""));
        item.omapper.push_back(MapperData("AMAZON", "手机", "手机数码>手机配件"));
        item.omapper.push_back(MapperData("JD", "电视机", "家电>大家电>平板电视"));
        item.omapper.push_back(MapperData("PUB", "英语教材", "化妆品"));
        item.odocs.push_back(
        GenODoc(UPDATE_SCD,"aaa", "TITLE1","AMAZON"
            , "手机数码>手机配件>","", "aaa"));
        //item.pdocs.push_back(
        //GenODoc(UPDATE_SCD,"aaa", "TITLE1","AMAZON"
        //    , "手机数码>手机配件>","", "aaa"));
        item.pdocs.push_back(
        GenODoc(UPDATE_SCD,"aaa", "",""
            , "手机数码>手机配件>","", ""));
        item.odocs.push_back(
        GenODoc(UPDATE_SCD,"bbb", "TITLE2","JD"
            , "家电>大家电>平板电视>","", "bbb"));
        item.odocs.push_back(
        GenODoc(UPDATE_SCD,"bbb", "TITLE3","JD"
            , "家电>大家电>平板电视>","", "bbb"));
        item.pdocs.push_back(
        GenODoc(UPDATE_SCD,"bbb", "TITLE3",""
            , "家电>大家电>平板电视>","", ""));
        item.odocs.push_back(
        GenODoc(UPDATE_SCD,"eee", "BOOK11","PUB"
            , "化妆品>","", "eee"));
        item.odocs.push_back(
        GenODoc(UPDATE_SCD,"eee", "BOOK12","PUB"
            , "化妆品>","", "eee"));
        item.pdocs.push_back(
        GenODoc(UPDATE_SCD,"eee", "BOOK12","PUB"
            , "化妆品>","", "eee"));
        item.odocs.push_back(
        GenODoc(DELETE_SCD,"111", "",""
            , "","", ""));
        item.pdocs.push_back(
        GenPDoc(DELETE_SCD,"111", "",""
            ,"",0));
        item.pdocs.push_back(
        GenPDoc(UPDATE_SCD,B5MHelper::GetPidByIsbn("999"), "","京东商城,卓越亚马逊,DD"
            ,"",5));

        //item.pdocs.push_back(
        //GenPDoc(UPDATE_SCD,"aaa", "",""
        //    , "手机数码>手机配件>",0));
        //item.pdocs.push_back(
        //GenPDoc(UPDATE_SCD,"bbb", "TITLE3",""
        //    , "家电>大家电>平板电视>",0));
        //item.pdocs.push_back(
        //GenPDoc(UPDATE_SCD,B5MHelper::GetPidByIsbn("101"), "BOOK1|","DD"
        //    ,"",1));
        item.pdocs.push_back(
        GenPDoc(DELETE_SCD,B5MHelper::GetPidByIsbn("101"), "",""
            ,"",0));
        //item.pdocs.push_back(
        //GenPDoc(UPDATE_SCD,"eee", "BOOK12","PUB"
        //    ,"化妆品>",1));
        tester.Test(item);
    }
    {
        TestItem item;
        item.input_docs.push_back(
        GenDoc(UPDATE_SCD,"aaa", "TITLE01","AMAZON"
            , "UNKNOW",""));
        item.input_docs.push_back(
        GenDoc(UPDATE_SCD,"eee", "BOOK12","PUB"
            , "英语","110"));
        item.input_docs.push_back(
        GenDoc(DELETE_SCD,"111", "",""
            , "",""));
        item.input_docs.push_back(
        GenDoc(DELETE_SCD,"222", "",""
            , "",""));
        item.omapper.push_back(MapperData("AMAZON", "手机", "手机数码>手机"));
        item.omapper.push_back(MapperData("JD", "电视机", "家电>大家电>电视"));
        item.omapper.push_back(MapperData("PUB", "英语教材", "化妆品"));
        item.odocs.push_back(
        GenODoc(UPDATE_SCD,"aaa", "TITLE1","AMAZON"
            , "手机数码>手机>","", "aaa"));
        item.odocs.push_back(
        GenODoc(UPDATE_SCD,"aaa", "TITLE01","AMAZON"
            , "","", "aaa"));
        item.pdocs.push_back(
        GenODoc(UPDATE_SCD,"aaa", "TITLE01",""
            , "","", ""));
        item.odocs.push_back(
        GenODoc(UPDATE_SCD,"bbb", "TITLE3","JD"
            , "家电>大家电>电视>","", "bbb"));
        item.pdocs.push_back(
        GenODoc(UPDATE_SCD,"bbb", "",""
            , "家电>大家电>电视>","", ""));
        item.odocs.push_back(
        GenODoc(UPDATE_SCD,"eee", "BOOK12","PUB"
            , "","", B5MHelper::GetPidByIsbn("110")));
        item.pdocs.push_back(
        GenODoc(UPDATE_SCD,"eee", "BOOK12","PUB"
            , "","", B5MHelper::GetPidByIsbn("110")));
        item.odocs.push_back(
        GenODoc(DELETE_SCD,"111", "",""
            , "","", ""));
        item.odocs.push_back(
        GenODoc(DELETE_SCD,"222", "",""
            , "","", ""));
        item.pdocs.push_back(
        GenPDoc(DELETE_SCD,"111", "",""
            ,"",0));
        item.pdocs.push_back(
        GenPDoc(DELETE_SCD,"222", "",""
            ,"",0));
        item.pdocs.push_back(
        GenPDoc(UPDATE_SCD,B5MHelper::GetPidByIsbn("999"), "",""
            ,"",4));

        //item.pdocs.push_back(
        //GenPDoc(UPDATE_SCD,"aaa", "TITLE01",""
        //    , "",0));
        //item.pdocs.push_back(
        //GenPDoc(UPDATE_SCD,"bbb", "",""
        //    , "家电>大家电>电视>",0));
        //item.pdocs.push_back(
        //GenPDoc(UPDATE_SCD,B5MHelper::GetPidByIsbn("110")
        //    , "BOOK12","PUB"
        //    ,"",1));
        //item.pdocs.push_back(
        //GenPDoc(DELETE_SCD,"eee", "",""
        //    ,"",0));
        tester.Test(item);
    }
}
} }

BOOST_AUTO_TEST_SUITE_END()


