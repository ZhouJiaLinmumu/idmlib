#include <idmlib/b5m/product_db.h>
#include <idmlib/b5m/product_matcher.h>
#include <sf1common/JsonDocument.h>
#include <sf1common/split_ustr.h>
#include <sstream>
using namespace idmlib::b5m;

B5mpDocGenerator::B5mpDocGenerator()
{
    sub_doc_props_.insert("DOCID");
    sub_doc_props_.insert("Title");
    sub_doc_props_.insert("Url");
    sub_doc_props_.insert("Price");
    sub_doc_props_.insert("Picture");
    sub_doc_props_.insert("OriginalPicture");
    sub_doc_props_.insert("Source");
    sub_doc_props_.insert("OriginalCategory");
    sub_doc_props_.insert("PicPrpt");
    sub_doc_props_.insert("SalesAmount");
    sub_doc_props_.insert("isFreeDelivery");
    sub_doc_props_.insert("isCOD");
    sub_doc_props_.insert("isGenuine");
    //sub_doc_props_.clear();
    default_source_weight_ = 1;
    subdoc_weighter_.insert(std::make_pair("卓越亚马逊", 10));
    subdoc_weighter_.insert(std::make_pair("京东商城", 9));
    subdoc_weighter_.insert(std::make_pair("天猫", 8));
    subdoc_weighter_.insert(std::make_pair("淘宝网", 0));
    pic_weighter_.insert(std::make_pair("卓越亚马逊", 10));
    pic_weighter_.insert(std::make_pair("京东商城", 9));
    pic_weighter_.insert(std::make_pair("天猫", 0));
    pic_weighter_.insert(std::make_pair("淘宝网", 0));
}
void B5mpDocGenerator::Gen(const std::vector<ScdDocument>& odocs, ScdDocument& pdoc)
{
    pdoc.type=UPDATE_SCD;
    int64_t itemcount=0;
    std::set<std::string> psource;
    std::set<std::string> pcountry;
    std::vector<Document> subdocs;
    ProductPrice pprice;
    Document::doc_prop_value_strtype pdate;
    Document::doc_prop_value_strtype spu_title;
    Document::doc_prop_value_strtype spu_pic;
    Document::doc_prop_value_strtype spu_url;
    Document::doc_prop_value_strtype url;
    ProductPrice min_price;
    //std::vector<ProductMatcher::Attribute> pattributes;
    bool independent=true;
    Document::doc_prop_value_strtype pid;
    odocs.front().getProperty("uuid", pid);
    std::size_t sales_amount=0;
    std::size_t sales_amount_number = 0;
    std::size_t comment_count=0;
    std::size_t comment_count_number = 0;
    std::vector<Attribute> attributes;
    //std::cerr<<"b5mp gen "<<pid<<","<<odocs.size()<<std::endl;
    for(uint32_t i=0;i<odocs.size();i++)
    {
        const ScdDocument& doc=odocs[i];
        Document::doc_prop_value_strtype oid;
        doc.getProperty("DOCID", oid);
        if(oid!=pid) independent=false;
        if(spu_title.empty())
        {
            doc.getProperty(B5MHelper::GetSPTPropertyName(), spu_title);
            if(!spu_title.empty())
            {
                independent = false;
            }
        }
        if(doc.type==NOT_SCD||doc.type==DELETE_SCD)
        {
            continue;
        }
        if(!sub_doc_props_.empty())
        {
            Document subdoc;
            for(ScdDocument::property_const_iterator it=doc.propertyBegin();it!=doc.propertyEnd();++it)
            {
                if(sub_doc_props_.find(it->first)!=sub_doc_props_.end())
                {
                    subdoc.property(it->first) = it->second;
                }
            }
            if(subdoc.getPropertySize()>0)
            {
                subdocs.push_back(subdoc);
            }
            SelectSubDocs_(subdocs);
        }
        doc.getProperty(B5MHelper::GetSPPicPropertyName(), spu_pic);
        doc.getProperty(B5MHelper::GetSPUrlPropertyName(), spu_url);
        pdoc.merge(doc);
        itemcount+=1;
        Document::doc_prop_value_strtype usource;
        Document::doc_prop_value_strtype uprice;
        //Document::doc_prop_value_strtype uattribute;
        Document::doc_prop_value_strtype udate;
        Document::doc_prop_value_strtype uurl;
        doc.getProperty("Source", usource);
        doc.getProperty("Price", uprice);
        //doc.getProperty("Attribute", uattribute);
        doc.getProperty("DATE", udate);
        doc.getProperty("Url", uurl);

        UString uattrib;
        doc.getString("Attribute", uattrib);
        std::vector<Attribute> av;
        ProductMatcher::ParseAttributes(uattrib, av);
        ProductMatcher::MergeAttributes(attributes, av);

        if(udate.compare(pdate)>0) pdate=udate;
        ProductPrice price;
        price.Parse(uprice);
        pprice+=price;
        if(!min_price.Valid()) 
        {
            url = uurl;
            min_price = price;
        }
        else if(price.Valid())
        {
            if(price.value.first<min_price.value.first)
            {
                min_price=price;
                url = uurl;
            }
        }
        if(usource.length()>0)
        {
            std::string ssource = propstr_to_str(usource);
            psource.insert(ssource);
        }
        std::string country;
        doc.getString("Country", country);
        if(!country.empty()) pcountry.insert(country);
        std::string samount;
        doc.getString(B5MHelper::GetSalesAmountPropertyName(), samount);
        if(!samount.empty())
        {
            try {
                std::size_t m = boost::lexical_cast<std::size_t>(samount);
                sales_amount += m;
                sales_amount_number+=1;
            }
            catch(std::exception& ex)
            {
                //std::cerr<<"sales amount error: ["<<samount<<"] on oid "<<oid<<std::endl;
            }
        }
        std::string ccount;
        doc.getString(B5MHelper::GetCommentCountPropertyName(), ccount);
        if(!ccount.empty())
        {
            try {
                std::size_t m = boost::lexical_cast<std::size_t>(ccount);
                comment_count += m;
                comment_count_number+=1;
            }
            catch(std::exception& ex)
            {
                //std::cerr<<"comment count error: ["<<ccount<<"] on oid "<<oid<<std::endl;
            }
        }
    }
    UString uattrib = ProductMatcher::AttributesText(attributes);
    if(!uattrib.empty())
    {
        pdoc.property("Attribute") = ustr_to_propstr(uattrib);
    }
    pdoc.property("Url") = url;
    if(!spu_title.empty())
    {
        independent=false;
        pdoc.property("Title") = spu_title;
    }
    if(!spu_pic.empty())
    {
        pdoc.property("Picture") = spu_pic;
    }
    else
    {
        std::pair<std::string, int> select_pic("", -1);
        for(uint32_t i=0;i<odocs.size();i++)
        {
            const ScdDocument& doc=odocs[i];
            if(doc.type==NOT_SCD||doc.type==DELETE_SCD)
            {
                continue;
            }
            std::string pic;
            doc.getString("Picture", pic);
            if(pic.empty()) continue;
            std::string source;
            doc.getString("Source", source);
            if(source.empty()) continue;
            int weight = default_source_weight_;
            boost::unordered_map<std::string, int>::const_iterator it = pic_weighter_.find(source);
            if(it!=pic_weighter_.end())
            {
                weight = it->second;
            }
            if(weight>select_pic.second)
            {
                select_pic.first = pic;
                select_pic.second = weight;
            }
        }
        if(!select_pic.first.empty())
        {
            pdoc.property("Picture") = str_to_propstr(select_pic.first);
        }

    }
    if(!spu_url.empty())
    {
        pdoc.property("Url") = spu_url;
    }
    pdoc.eraseProperty(B5MHelper::GetSPTPropertyName());
    pdoc.eraseProperty(B5MHelper::GetSPPicPropertyName());
    pdoc.eraseProperty(B5MHelper::GetSPUrlPropertyName());
    pdoc.eraseProperty("uuid");
    if(!subdocs.empty()&&!independent)
    {
        izenelib::util::UString text;
        JsonDocument::ToJsonText(subdocs, text);
        pdoc.property(B5MHelper::GetSubDocsPropertyName()) = ustr_to_propstr(text);
    }

    pdoc.property("DOCID") = pid;
    pdoc.property("itemcount") = itemcount;
    pdoc.property("independent") = (int64_t)independent;
    if(!pdate.empty()) pdoc.property("DATE") = pdate;
    pdoc.property("Price") = pprice.ToPropString();
    std::string ssource;
    for(std::set<std::string>::const_iterator it=psource.begin();it!=psource.end();++it)
    {
        if(!ssource.empty()) ssource+=",";
        ssource+=*it;
    }
    if(!ssource.empty()) pdoc.property("Source") = str_to_propstr(ssource, UString::UTF_8);
    std::string scountry;
    for(std::set<std::string>::const_iterator it=pcountry.begin();it!=pcountry.end();++it)
    {
        if(!scountry.empty()) scountry+=",";
        scountry+=*it;
    }
    if(!scountry.empty()) pdoc.property("Country") = str_to_propstr(scountry, UString::UTF_8);
    if(sales_amount_number>0)
    {
        //pdoc.property(B5MHelper::GetSalesAmountPropertyName()) = (int64_t)(sales_amount/sales_amount_number);
        pdoc.property(B5MHelper::GetSalesAmountPropertyName()) = (int64_t)sales_amount;
    }
    if(comment_count_number>0)
    {
        //pdoc.property(B5MHelper::GetCommentCountPropertyName()) = (int64_t)(comment_count/comment_count_number);
        pdoc.property(B5MHelper::GetCommentCountPropertyName()) = (int64_t)comment_count;
    }
    //if(!pattributes.empty()) pdoc.property("Attribute") = ProductMatcher::AttributesText(pattributes); 
    if(itemcount==0)
    {
        pdoc.type=DELETE_SCD;
        pdoc.clearExceptDOCID();
    }
}

bool B5mpDocGenerator::SubDocCompare_(const Document& x, const Document& y)
{
    std::string xstr;
    std::string ystr;
    x.getString("Price", xstr);
    y.getString("Price", ystr);
    ProductPrice xprice;
    xprice.Parse(xstr);
    ProductPrice yprice;
    yprice.Parse(ystr);
    if(!xprice.Positive()) return false;
    if(!yprice.Positive()) return true;
    return xprice.Min()<yprice.Min();

}
bool B5mpDocGenerator::ReversePriceCompare_(const Document& x, const Document& y)
{
    std::string xstr;
    std::string ystr;
    x.getString("Price", xstr);
    y.getString("Price", ystr);
    ProductPrice xprice;
    xprice.Parse(xstr);
    ProductPrice yprice;
    yprice.Parse(ystr);
    if(!xprice.Positive()) return true;
    if(!yprice.Positive()) return false;
    return xprice.Min()>yprice.Min();
}

void B5mpDocGenerator::SelectSubDocs_(std::vector<Document>& subdocs) const
{
    static const uint32_t max = 10u;
    if(subdocs.size()<=max) return;
    //std::sort(subdocs.begin(), subdocs.end(), SubDocCompare_);
    //subdocs.resize(max);
    boost::unordered_map<std::string, SubDocSelector> source_map;
    for(uint32_t i=0;i<subdocs.size();i++)
    {
        const Document& doc = subdocs[i];
        std::string source;
        doc.getString("Source", source);
        source_map[source].docs.push_back(doc);
    }
    std::vector<SubDocSelector> list;
    for(boost::unordered_map<std::string, SubDocSelector>::iterator it = source_map.begin();it!=source_map.end();++it)
    {
        boost::unordered_map<std::string, int>::const_iterator wit = subdoc_weighter_.find(it->first);
        int weight = default_source_weight_;
        if(wit!=subdoc_weighter_.end())
        {
            weight = wit->second;
        }
        it->second.weight = weight;
        list.push_back(it->second);
    }
    std::sort(list.begin(), list.end());
    for(uint32_t i=0;i<list.size();i++)
    {
        std::sort(list[i].docs.begin(), list[i].docs.end(), ReversePriceCompare_);
    }
    std::vector<Document> new_subdocs;
    while(true)
    {
        if(new_subdocs.size()==max) break;
        bool find = false;
        for(uint32_t i=0;i<list.size();i++)
        {
            if(new_subdocs.size()==max) break;
            SubDocSelector& s = list[i];
            if(s.docs.empty()) continue;
            new_subdocs.push_back(s.docs.back());
            s.docs.resize(s.docs.size()-1);
            find = true;
        }
        if(!find) break;
    }
    subdocs.swap(new_subdocs);
}

ProductProperty::ProductProperty()
:itemcount(0), independent(false)
{
}

bool ProductProperty::Parse(const Document& doc)
{
    if(doc.getProperty("itemcount", itemcount))
    {
        if(!doc.getProperty("DOCID", productid))
        {
            LOG(ERROR)<<"parse doc error"<<std::endl;
            return false;
        }
        if(!doc.getProperty("OID", oid))
        {
            LOG(ERROR)<<"parse doc error"<<std::endl;
            return false;
        }
    }
    else if(doc.getProperty("uuid", productid)) 
    {
        itemcount = 1;
        if(!doc.getProperty("DOCID", oid))
        {
            LOG(ERROR)<<"parse doc error"<<std::endl;
            return false;
        }
    }
    else
    {
        LOG(ERROR)<<"parse doc error"<<std::endl;
        return false;
    }
    doc.getString("DATE", date);
    SetIndependent();
    //LOG(INFO)<<"find oid "<<oid<<std::endl;
    Document::doc_prop_value_strtype usource;
    Document::doc_prop_value_strtype uprice;
    Document::doc_prop_value_strtype uattribute;
    doc.getProperty("Source", usource);
    doc.getProperty("Price", uprice);
    doc.getProperty("Attribute", uattribute);
    price.Parse(uprice);
    if(usource.length()>0)
    {
        std::string ssource = propstr_to_str(usource);
        std::vector<std::string> s_vector;
        boost::algorithm::split( s_vector, ssource, boost::algorithm::is_any_of(",") );
        for(uint32_t i=0;i<s_vector.size();i++)
        {
            if(s_vector[i].empty()) continue;
            source.insert(s_vector[i]);
        }
    }
    std::vector<AttrPair> attrib_list;
    split_attr_pair(propstr_to_ustr(uattribute), attrib_list);
    for(std::size_t i=0;i<attrib_list.size();i++)
    {
        std::vector<izenelib::util::UString>& attrib_value_list = attrib_list[i].second;
        std::vector<std::string> attrib_value_str_list;
        //do duplicate remove
        boost::unordered_set<std::string> value_set;
        for(uint32_t j=0;j<attrib_value_list.size();j++)
        {
            std::string svalue;
            attrib_value_list[j].convertString(svalue, UString::UTF_8);
            if(value_set.find(svalue)!=value_set.end()) continue;
            attrib_value_str_list.push_back(svalue);
            value_set.insert(svalue);
        }
        izenelib::util::UString attrib_name = attrib_list[i].first;
        std::string sname;
        attrib_name.convertString(sname, UString::UTF_8);
        attribute[sname] = attrib_value_str_list;
    }
 
    return true;
}

void ProductProperty::Set(Document& doc) const
{
    doc.property("DOCID") = productid;
    //LOG(INFO)<<"set oid "<<oid<<std::endl;
    doc.property("OID") = oid;
    if(!date.empty())
    {
        doc.property("DATE") = str_to_propstr(date, UString::UTF_8);
    }
    if(price.Valid())
    {
        doc.property("Price") = price.ToPropString();
    }
    izenelib::util::UString usource = GetSourceUString();
    if(!usource.empty())
    {
        doc.property("Source") = ustr_to_propstr(usource);
    }

    UString uattribute = GetAttributeUString();
    if(!uattribute.empty())
    {
        doc.property("Attribute") = ustr_to_propstr(uattribute);
    }
 
    doc.property("itemcount") = itemcount;
    if(independent)
    {
        doc.property("independent") = (int64_t)1;
    }
    else
    {
        doc.property("independent") = (int64_t)0;
    }
    doc.eraseProperty("uuid");
}

void ProductProperty::SetIndependent()
{
    if(itemcount==1 && productid==oid)
    {
        independent = true;
    }
    else
    {
        independent = false;
    }
}

std::string ProductProperty::GetSourceString() const
{
    std::string result;
    for(SourceType::const_iterator it = source.begin(); it!=source.end(); ++it)
    {
        if(!result.empty()) result+=",";
        result += *it;
    }
    return result;
}

izenelib::util::UString ProductProperty::GetSourceUString() const
{
    return izenelib::util::UString(GetSourceString(), izenelib::util::UString::UTF_8);
}

izenelib::util::UString ProductProperty::GetAttributeUString() const
{
    std::string result;
    for(AttributeType::const_iterator oit = attribute.begin(); oit!=attribute.end(); ++oit)
    {
        if(!result.empty())
        {
            result += ",";
        }
        result += oit->first+":";
        std::string svalue;
        const std::vector<std::string>& value_list = oit->second;
        for(uint32_t i=0;i<value_list.size();i++)
        {
            if(!svalue.empty())
            {
                svalue+="|";
            }
            svalue+=value_list[i];
        }
        result+=svalue;
    }
    return izenelib::util::UString(result, izenelib::util::UString::UTF_8);
}

ProductProperty& ProductProperty::operator+=(const ProductProperty& other)
{
    if(productid.empty()) productid = other.productid;
    if(oid.empty() || other.productid==other.oid) oid = other.oid;
    if(other.date>date) date = other.date; //use latest date
    price += other.price;
    for(SourceType::const_iterator oit = other.source.begin(); oit!=other.source.end(); ++oit)
    {
        source.insert(*oit);
    }
    for(AttributeType::const_iterator oit = other.attribute.begin(); oit!=other.attribute.end(); ++oit)
    {
        attribute[oit->first] = oit->second;
    }
    itemcount+=other.itemcount;
    SetIndependent();
    return *this;
}

//ProductProperty& ProductProperty::operator-=(const ProductProperty& other)
//{
    //for(SourceMap::const_iterator oit = other.source.begin(); oit!=other.source.end(); ++oit)
    //{
        //SourceMap::iterator it = source.find(oit->first);
        //if(it==source.end())
        //{
            //source.insert(std::make_pair(oit->first, -1*oit->second));
            ////invalid data
            ////LOG(WARNING)<<"invalid data"<<std::endl;
        //}
        //else
        //{
            //it->second -= oit->second;
            //LOG(INFO)<<oit->first<<" reduced from "<<it->second+oit->second<<" to "<<it->second<<std::endl;
            ////if(it->second<0)
            ////{
                //////invalid data
                ////LOG(WARNING)<<"invalid data"<<std::endl;
                ////it->second = 0;
            ////}
        //}
    //}
    //itemcount -= 1;

    //return *this;
//}


std::string ProductProperty::ToString() const
{
    std::string spid;
    spid = propstr_to_str(productid);
    std::stringstream ss;
    ss<<"pid:"<<spid<<",DATE:"<<date<<",itemcount:"<<itemcount<<",price:"<<price.ToString()<<",source:";
    for(SourceType::const_iterator it = source.begin(); it!=source.end(); ++it)
    {
        ss<<"["<<*it<<"]";
    }
    return ss.str();
}

