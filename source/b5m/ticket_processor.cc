#include <idmlib/b5m/ticket_processor.h>
#include <idmlib/b5m/b5m_types.h>
#include <idmlib/b5m/b5m_helper.h>
#include <idmlib/b5m/product_db.h>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <glog/logging.h>
#include <sf1common/ScdParser.h>
#include <sf1common/Document.h>
#include <sf1common/split_ustr.h>
#include <idmlib/b5m/product_term_analyzer.h>

//#define TICKET_DEBUG

using namespace idmlib::b5m;

TicketProcessor::TicketProcessor(const B5mM& b5mm): b5mm_(b5mm)
{
}

bool TicketProcessor::Generate(const std::string& mdb_instance)
{
    SetCmaPath(b5mm_.cma_path);
    const std::string& scd_path = b5mm_.scd_path;
    namespace bfs = boost::filesystem;
    std::vector<std::string> scd_list;
    B5MHelper::GetIUScdList(scd_path, scd_list);
    if(scd_list.empty()) return false;
    typedef boost::unordered_map<std::string, std::string> MatchResult;
    MatchResult match_result;


    {
        ProductTermAnalyzer analyzer(cma_path_);
        std::string work_dir = mdb_instance+"/work_dir";

        boost::filesystem::create_directories(work_dir);
        std::string dd_container = work_dir +"/dd_container";
        std::string group_table_file = work_dir +"/group_table";
        GroupTableType group_table(group_table_file);
        group_table.Load();

        DDType dd(dd_container, &group_table);
        dd.SetFixK(12);
        if(!dd.Open())
        {
            std::cout<<"DD open failed"<<std::endl;
            return false;
        }

        for(uint32_t i=0;i<scd_list.size();i++)
        {
            std::string scd_file = scd_list[i];
            LOG(INFO)<<"DD Processing "<<scd_file<<std::endl;
            ScdParser parser(izenelib::util::UString::UTF_8);
            parser.load(scd_file);
            //int scd_type = ScdParser::checkSCDType(scd_file);
            uint32_t n=0;
            for( ScdParser::iterator doc_iter = parser.begin();
              doc_iter!= parser.end(); ++doc_iter, ++n)
            {
                if(n%10000==0)
                {
                    LOG(INFO)<<"Find Documents "<<n<<std::endl;
                }
                SCDDoc& scddoc = *(*doc_iter);
                SCDDoc::iterator p = scddoc.begin();
                Document doc;
                for(; p!=scddoc.end(); ++p)
                {
                    const std::string& property_name = p->first;
                    doc.property(property_name) = p->second;
                }
                std::string soid;
                std::string category;
                std::string city;
                std::string name;
                std::string sprice;
                std::string address;
                std::string time;
                doc.getString("DOCID", soid);
                doc.getString("Category", category);
                doc.getString("PlayCity", city);
                doc.getString("PlayName", name);
                doc.getString("PlayAddress", address);
                doc.getString("PlayTime", time);
                if(soid.empty()||category.empty()||city.empty()||address.empty()||time.empty()||name.empty())
                {
                    continue;
                }
                const DocIdType& id = soid;

                TicketProcessorAttach attach;
                UString sid_str = propstr_to_ustr(category);
                sid_str.append(UString("|", UString::UTF_8));
                sid_str.append(propstr_to_ustr(city));
                attach.sid = izenelib::util::HashFunction<izenelib::util::UString>::generateHash32(sid_str);
                boost::algorithm::split(attach.time_array, time, boost::is_any_of(",;"));
                std::sort(attach.time_array.begin(), attach.time_array.end());

                UString text = propstr_to_ustr(name);
                //text.append(UString(" ", UString::UTF_8));
                //text.append(address);
                //text.append(UString(" ", UString::UTF_8));
                //text.append(category);
                //text.append(UString(" ", UString::UTF_8));
                //text.append(category);
                //text.append(UString(" ", UString::UTF_8));
                //text.append(category);
                //text.append(UString(" ", UString::UTF_8));
                //text.append(city);
                //text.append(UString(" ", UString::UTF_8));
                //text.append(city);
                //text.append(UString(" ", UString::UTF_8));
                //text.append(city);
                std::vector<std::pair<std::string, double> > doc_vector;
                analyzer.Analyze(text, doc_vector);

                if( doc_vector.empty() )
                {
                    continue;
                }
                dd.InsertDoc(id, doc_vector, attach);
#ifdef TICKET_DEBUG
                LOG(INFO)<<"insert id "<<id<<std::endl;
                for(uint32_t i=0;i<doc_vector.size();i++)
                {
                    LOG(INFO)<<doc_vector[i].first<<","<<doc_vector[i].second<<std::endl;
                }
#endif
            }
        }
        dd.RunDdAnalysis();
        const std::vector<std::vector<std::string> >& group_info = group_table.GetGroupInfo();
        for(uint32_t gid=0;gid<group_info.size();gid++)
        {
            const std::vector<std::string>& in_group = group_info[gid];
            if(in_group.empty()) continue;
            std::string pid = in_group[0];
            for(uint32_t i=0;i<in_group.size();i++)
            {
                match_result[in_group[i]] = pid;
            }
        }
    }
    LOG(INFO)<<"match result size "<<match_result.size()<<std::endl;
    //std::string b5mo_path = B5MHelper::GetB5moPath(mdb_instance);
    const std::string& b5mo_path = b5mm_.b5mo_path;
    B5MHelper::PrepareEmptyDir(b5mo_path);
    ScdWriter writer(b5mo_path, UPDATE_SCD);

    for(uint32_t i=0;i<scd_list.size();i++)
    {
        std::string scd_file = scd_list[i];
        LOG(INFO)<<"Processing "<<scd_file<<std::endl;
        ScdParser parser(izenelib::util::UString::UTF_8);
        parser.load(scd_file);
        //int scd_type = ScdParser::checkSCDType(scd_file);
        uint32_t n=0;
        for( ScdParser::iterator doc_iter = parser.begin();
          doc_iter!= parser.end(); ++doc_iter, ++n)
        {
            if(n%10000==0)
            {
                LOG(INFO)<<"Find Documents "<<n<<std::endl;
            }
            Document doc;
            SCDDoc& scddoc = *(*doc_iter);
            SCDDoc::iterator p = scddoc.begin();
            for(; p!=scddoc.end(); ++p)
            {
                const std::string& property_name = p->first;
                doc.property(property_name) = p->second;
            }
            Document::doc_prop_value_strtype oid;
            std::string soid;
            doc.getProperty("DOCID", oid);
            soid = propstr_to_str(oid);
            std::string spid;
            MatchResult::const_iterator it = match_result.find(soid);
            if(it==match_result.end())
            {
                spid = soid;
            }
            else
            {
                spid = it->second;
            }
            doc.property("uuid") = str_to_propstr(spid, UString::UTF_8);
            writer.Append(doc);
        }
    }
    writer.Close();
    PairwiseScdMerger merger(b5mo_path);
    std::size_t odoc_count = ScdParser::getScdDocCount(b5mo_path);
    LOG(INFO)<<"ticket o doc count "<<odoc_count<<std::endl;
    uint32_t m = odoc_count/2400000+1;
    merger.SetM(m);
    merger.SetMProperty("uuid");
    merger.SetOutputer(boost::bind( &TicketProcessor::B5moOutput_, this, _1, _2));
    merger.SetMEnd(boost::bind( &TicketProcessor::POutputAll_, this));
    //std::string p_output_dir = B5MHelper::GetB5mpPath(mdb_instance);
    const std::string& p_output_dir = b5mm_.b5mp_path;
    B5MHelper::PrepareEmptyDir(p_output_dir);
    pwriter_.reset(new ScdWriter(p_output_dir, UPDATE_SCD));
    merger.Run();
    pwriter_->Close();
    return true;

}

void TicketProcessor::POutputAll_()
{
    for(CacheType::iterator it = cache_.begin();it!=cache_.end();it++)
    {
        SValueType& svalue = it->second;
        svalue.doc.eraseProperty("OID");
        pwriter_->Append(svalue.doc);
    }
    cache_.clear();
}

void TicketProcessor::B5moOutput_(SValueType& value, int status)
{
    uint128_t pid = GetPid_(value.doc);
    if(pid==0)
    {
        return;
    }
    SValueType& svalue = cache_[pid];
    ProductMerge_(svalue, value);
}

uint128_t TicketProcessor::GetPid_(const Document& doc)
{
    std::string spid;
    doc.getString("uuid", spid);
    if(spid.empty()) return 0;
    return B5MHelper::StringToUint128(spid);
}
uint128_t TicketProcessor::GetOid_(const Document& doc)
{
    std::string soid;
    doc.getString("DOCID", soid);
    if(soid.empty()) return 0;
    return B5MHelper::StringToUint128(soid);
}

void TicketProcessor::ProductMerge_(SValueType& value, const SValueType& another_value)
{
    //value is pdoc or empty, another_value is odoc
    ProductProperty pp;
    if(!value.empty())
    {
        pp.Parse(value.doc);
    }
    ProductProperty another;
    another.Parse(another_value.doc);
    pp += another;
    if(value.empty() || another.oid==another.productid)
    {
        value.doc.copyPropertiesFromDocument(another_value.doc, true);
    }
    else
    {
        const PropertyValue& docid_value = value.doc.property("DOCID");
        //override if empty property
        for (Document::property_const_iterator it = another_value.doc.propertyBegin(); it != another_value.doc.propertyEnd(); ++it)
        {
            if (!value.doc.hasProperty(it->first))
            {
                value.doc.property(it->first) = it->second;
            }
            else
            {
                PropertyValue& pvalue = value.doc.property(it->first);
                if(pvalue.which()==docid_value.which()) //is UString
                {
                    const PropertyValue::PropertyValueStrType& uvalue = pvalue.getPropertyStrValue();
                    if(uvalue.empty())
                    {
                        pvalue = it->second;
                    }
                }
            }
        }
    }
    value.type = UPDATE_SCD;
    pp.Set(value.doc);
}

