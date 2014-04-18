#ifndef IDMLIB_B5M_COMMENTDB_H_
#define IDMLIB_B5M_COMMENTDB_H_

#include <string>
#include <vector>
#include "b5m_types.h"
#include "b5m_helper.h"
#include <types.h>
#include <am/sequence_file/ssfr.h>
#include <util/DynamicBloomFilter.h>
#include <glog/logging.h>

NS_IDMLIB_B5M_BEGIN

class CommentDb
{
public:
    typedef uint128_t KeyType;
    //typedef izenelib::util::BloomFilter<KeyType, KeyType> FilterType;
    typedef izenelib::util::DynamicBloomFilter<KeyType, KeyType> FilterType;
    CommentDb(const std::string& path)
    : path_(path)
      , filter_path_(path+"/filter")
      , count_path_(path+"/count")
      , filter_(NULL), count_(0), is_open_(false), has_modify_(false)
    {
    }

    ~CommentDb()
    {
        if(filter_!=NULL)
        {
            delete filter_;
        }
    }

    bool is_open() const
    {
        return is_open_;
    }

    bool open()
    {
        if(is_open_) return true;
        boost::filesystem::create_directories(path_);
        //filter_ = new FilterType(100000000, 0.000000001);
        filter_ = new FilterType(100000000, 0.000000001, 100000000);
        if(boost::filesystem::exists(filter_path_))
        {
            std::ifstream ifs(filter_path_.c_str());
            filter_->load(ifs);
            ifs.close();
        }
        izenelib::am::ssf::Util<>::Load(count_path_, count_);
        is_open_ = true;
        return true;
    }


    void Insert(const KeyType& key)
    {
        filter_->Insert(key);
        count_++;
        has_modify_ = true;
    }

    bool Get(const KeyType& key) const
    {
        return filter_->Get(key);
    }

    inline uint64_t Count() const
    {
        return count_;
    }



    bool flush()
    {
        LOG(INFO)<<"try flush cdb.."<<std::endl;
        if(has_modify_)
        {
            LOG(INFO)<<"cdb count "<<Count()<<std::endl;
            boost::filesystem::remove_all(filter_path_);
            std::ofstream ofs(filter_path_.c_str());
            filter_->save(ofs);
            ofs.close();
            izenelib::am::ssf::Util<>::Save(count_path_, count_);
            has_modify_ = false;
        }
        else
        {
            LOG(INFO)<<"cdb not change"<<std::endl;
        }
        return true;
    }

private:

private:

    std::string path_;
    std::string filter_path_;
    std::string count_path_;
    FilterType* filter_;
    uint64_t count_;
    bool is_open_;
    bool has_modify_;
};
NS_IDMLIB_B5M_END

#endif

