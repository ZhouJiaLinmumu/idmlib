#include <idmlib/b5m/product_price.h>

#include <sstream>
#include <boost/operators.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/lexical_cast.hpp>
#include <glog/logging.h>

using namespace idmlib::b5m;

ProductPrice::ProductPrice() : value(-1.0, -1.0)
{
}

ProductPrice::ProductPrice(ProductPriceType a, ProductPriceType b) : value(a,b)
{
    Check_();
}

ProductPrice& ProductPrice::operator+=(const ProductPrice& a)
{
    if (!Valid())
    {
        if (a.Valid())
        {
            value = a.value;
        }
    }
    else
    {
        if (a.Valid())
        {
            value.first = std::min(value.first, a.value.first);
            value.second = std::max(value.second, a.value.second);
        }
    }
    return *this;
}

bool ProductPrice::operator==(const ProductPrice& b) const
{
    return Valid() && b.Valid() && value == b.value;
}

bool ProductPrice::GetMid(ProductPriceType& mid) const
{
    if (!Valid()) return false;
    mid = (value.first + value.second) / 2;
    return true;
}

bool ProductPrice::Convert_(const std::string& str, ProductPriceType& p)
{
    try
    {
        p = boost::lexical_cast<ProductPriceType>(str);
    }
    catch(std::exception& ex)
    {
        return false;
    }
    return true;
}

bool ProductPrice::Convert_(ProductPriceType p, std::stringstream& os)
{
    char buffer[50];
    int stat = sprintf(buffer, "%.2f", p);
    if (stat == 0 || stat == EOF) return false;
    os << buffer;
    return true;
}

bool ProductPrice::Parse(const izenelib::util::UString& ustr)
{
    std::string str;
    ustr.convertString(str, izenelib::util::UString::UTF_8);
    return Parse(str);
}

bool ProductPrice::Parse(const std::string& str)
{
    ProductPriceType p;
    if (Convert_(str, p))
    {
        value.first = p;
        value.second = p;
    }
    else
    {
        static const char sep[] = {'-', '~', ','};
        static const uint32_t len = sizeof(sep) / sizeof(char);
        bool found = false;
        for (uint32_t i = 0; i < len; i++)
        {
            if (checkSeparatorType_(str, sep[i]) )
            {
                found = true;
                if (!split_float_(str, sep[i]))
                {
                    Reset_();
                    return false;
                }
                break;
            }
        }
        if (!found)
        {
            Reset_();
            return false;
        }
    }
    return true;
}
ProductPrice::ProductPriceType ProductPrice::ParseValue(const izenelib::util::UString& ustr)
{
    std::string str;
    ustr.convertString(str, izenelib::util::UString::UTF_8);
    return ParseValue(str);
}

ProductPrice::ProductPriceType ProductPrice::ParseValue(const std::string& str)
{
    ProductPrice pprice;
    if(!pprice.Parse(str))
    {
        return -1.0;
    }
    ProductPriceType price;
    pprice.GetMid(price);
    return price;
}

ProductPrice::ProductPriceType ProductPrice::ParseDocPrice(const Document& doc, const std::string& price_property_name)
{
    std::string sprice;
    doc.getString(price_property_name, sprice);
    return ProductPrice::ParseValue(sprice);
}

std::string ProductPrice::ToString() const
{
    if (!Valid()) return "";
    //LOG(INFO)<<value.first<<"-"<<value.second;
    std::stringstream ss;
    if (value.first == value.second)
    {
        if (!Convert_(value.first, ss)) return "";
    }
    else
    {
        if (!Convert_(value.first, ss)) return "";
        ss << "-";
        if (!Convert_(value.second, ss)) return "";
    }
    //LOG(INFO)<<ss.str()<<std::endl;
    return ss.str();
}

Document::doc_prop_value_strtype ProductPrice::ToPropString() const
{
    return str_to_propstr(ToString(), izenelib::util::UString::UTF_8);
}

bool ProductPrice::Valid() const
{
    return value.first >= 0.0 && value.second >= 0.0;
}

bool ProductPrice::Positive() const
{
    return value.first>0.0&&value.second>0.0;
}

void ProductPrice::Check_()
{
    if (value.first > value.second)
    {
        //invalid
        std::swap(value.first, value.second);
    }
}

void ProductPrice::Reset_()
{
    value.first = -1.0;
    value.second = -1.0;
}

bool ProductPrice::checkSeparatorType_(const std::string& propertyValueStr, char separator)
{
    size_t n = propertyValueStr.find(separator, 0);
    if (n != std::string::npos)
        return true;
    return false;
}

bool ProductPrice::split_float_(const std::string& str, char sep)
{
    std::size_t n = 0, nOld = 0;
    std::vector<ProductPriceType> value_list;
    while (n != std::string::npos)
    {
        n = str.find(sep, n);
        if (n != std::string::npos)
        {
            if (n != nOld)
            {
                std::string tmpStr = str.substr(nOld, n - nOld);
                boost::algorithm::trim(tmpStr);
                ProductPriceType p;
                if (!Convert_(tmpStr, p)) return false;
                value_list.push_back(p);
            }
            nOld = ++n;
        }
    }

    std::string tmpStr = str.substr(nOld, str.length() - nOld);
    boost::algorithm::trim(tmpStr);
    ProductPriceType p;
    if (!Convert_(tmpStr, p)) return false;
    value_list.push_back(p);
    if (value_list.size() >= 2)
    {
        value.first = std::min(value_list[0], value_list[1]);
        value.second = std::max(value_list[0], value_list[1]);
        return true;
    }
    else
    {
        return false;
    }
}

