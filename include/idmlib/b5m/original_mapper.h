#ifndef IDMLIB_B5M_ORIGINALMAPPER_H_
#define IDMLIB_B5M_ORIGINALMAPPER_H_
#include "b5m_helper.h"
#include "b5m_types.h"

NS_IDMLIB_B5M_BEGIN

class OriginalMapper {
public:
    //static std::string GetVersion(const std::string& path)
    //{
    //}
    typedef std::pair<std::string, std::string> Key;
    typedef std::string Value;
    typedef boost::unordered_map<Key, Value> Mapper;
    OriginalMapper(): is_open_(false)
    {
    }
    bool IsOpen() const
    {
        return is_open_;
    }
    bool Open(const std::string& path)
    {
        std::string file = path+"/data";
        return OpenFile(file);
    }
    bool OpenFile(const std::string& file)
    {
        std::string line;
        std::ifstream ifs(file.c_str());
        while(getline(ifs, line))
        {
            boost::algorithm::trim(line);
            Key key;
            Value value;
            if(!Parse_(line, key, value)) continue;
            mapper_[key] = value;
        }
        is_open_ = true;
        LOG(INFO)<<"Omapper size "<<mapper_.size()<<std::endl;
        return true;
    }

    bool Diff(const std::string& new_path)
    {
        Mapper new_mapper;
        std::string file = new_path+"/data";
        std::string line;
        std::ifstream ifs(file.c_str());
        while(getline(ifs, line))
        {
            boost::algorithm::trim(line);
            Key key;
            Value value;
            if(!Parse_(line, key, value)) continue;
            bool diff = false;
            Mapper::const_iterator it = mapper_.find(key);
            if(it==mapper_.end())
            {
                //LOG(INFO)<<"omapper key not found : "<<key.first<<","<<key.second<<std::endl;
                diff = true;
            }
            else
            {
                if(it->second!=value)
                {
                    //LOG(INFO)<<"omapper key not match for key "<<key.first<<","<<key.second<<" : "<<it->second<<","<<value<<std::endl;
                    diff = true;
                }
            }
            if(diff)
            {
                new_mapper[key] = value;
            }
            else
            {
                new_mapper.erase(key);
            }
        }
        ifs.close();
        mapper_.swap(new_mapper);
        return true;
    }

    bool Map(const std::string& source, const std::string& original, std::string& category) const
    {
        std::pair<std::string, std::string> key(source, original);
        Mapper::const_iterator it = mapper_.find(key);
        if(it==mapper_.end()) return false;
        else
        {
            category = it->second;
            return true;
        }
    }

    std::size_t Size() const
    {
        return mapper_.size();
    }

    void Show() const
    {
        for(Mapper::const_iterator it = mapper_.begin();it!=mapper_.end();++it)
        {
            std::cerr<<it->first.first<<","<<it->first.second<<","<<it->second<<std::endl;
        }
    }


private:
    static bool Parse_(const std::string& input, Key& key, Value& value)
    {
        std::vector<std::string> vec;
        boost::algorithm::split(vec, input, boost::algorithm::is_any_of(":"));
        if(vec.size()!=2) return false;
        value = vec[1];
        std::string key_str = vec[0];
        vec.clear();
        boost::algorithm::split(vec, key_str, boost::algorithm::is_any_of(","));
        if(vec.size()!=2) return false;
        key.first = vec[0];
        key.second = vec[1];
        return true;
    }
private:
    bool is_open_;
    Mapper mapper_;

};

NS_IDMLIB_B5M_END
#endif


