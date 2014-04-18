#ifndef IDMLIB_B5M_BRANDMANAGER_H_
#define IDMLIB_B5M_BRANDMANAGER_H_

#include <string>
#include <vector>
#include "b5m_types.h"
#include "b5m_helper.h"
#include <types.h>
#include <glog/logging.h>
#include <boost/unordered_map.hpp>
#include <boost/regex.hpp>

NS_IDMLIB_B5M_BEGIN

class BrandManager
{
    typedef std::set<std::string> CategorySet;
    typedef std::map<std::string, CategorySet> Map;
public:
    BrandManager();
    ~BrandManager();
    bool Load(const std::string& path);
    bool Save(const std::string& path);
    void LoadBrandErrorFile(const std::string& file);
    void Insert(const std::string& category, const std::string& title, const std::string& brand);
    bool IsBrand(const std::string& category, const std::string& brand) const;
    bool IsErrorBrand(const std::string& brand) const;

private:
    Map map_;
    boost::mutex mutex_;
    boost::unordered_set<std::string> error_set_;
    std::vector<boost::regex> error_regexps_;
};
NS_IDMLIB_B5M_END

#endif

