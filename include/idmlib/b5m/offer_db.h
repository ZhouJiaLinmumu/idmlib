#ifndef IDMLIB_B5M_OFFERDB_H_
#define IDMLIB_B5M_OFFERDB_H_

#include <string>
#include <vector>
#include "b5m_types.h"
#include "b5m_helper.h"
#include <types.h>
#include <am/tc/BTree.h>
#include <am/tc/Hash.h>
#include <am/leveldb/Table.h>
#include <am/succinct/fujimap/fujimap.hpp>
#include <glog/logging.h>
#include <boost/unordered_map.hpp>

NS_IDMLIB_B5M_BEGIN

class OfferDb
{
public:
    typedef uint128_t KeyType;
    typedef uint128_t ValueType;
    typedef uint32_t FlagType;
    //typedef boost::unordered_map<KeyType, ValueType, uint128_hash > DbType;
    typedef izenelib::am::succinct::fujimap::Fujimap<KeyType, ValueType> DbType;
    typedef izenelib::am::succinct::fujimap::Fujimap<KeyType, FlagType> FlagDbType;
    OfferDb(const std::string& path)
    : path_(path)
      , db_path_(path+"/db"), text_path_(path+"/text"), tmp_path_(path+"/tmp"), buffer_path_(path+"/buffer")
      , flag_db_path_(path+"/flag_db"), flag_tmp_path_(path+"/flag_tmp")
      , db_(NULL), flag_db_(NULL)
      , is_open_(false), has_modify_(false), lazy_(false)
    {
    }

    ~OfferDb()
    {
        if(db_!=NULL)
        {
            delete db_;
        }
        if(flag_db_!=NULL)
        {
            delete flag_db_;
        }
        text_.close();
    }

    void set_lazy_mode()
    {
        lazy_ = true;
    }

    bool is_open() const
    {
        return is_open_;
    }

    bool open()
    {
        if(is_open_) return true;
        boost::filesystem::create_directories(path_);
        if(boost::filesystem::exists(tmp_path_))
        {
            boost::filesystem::remove_all(tmp_path_);
        }
        if(boost::filesystem::exists(buffer_path_))
        {
            boost::filesystem::remove_all(buffer_path_);
        }
        db_ = new DbType(tmp_path_.c_str());
        db_->initFP(32);
        db_->initTmpN(30000000);
        if(boost::filesystem::exists(flag_tmp_path_))
        {
            boost::filesystem::remove_all(flag_tmp_path_);
        }
        flag_db_ = new FlagDbType(flag_tmp_path_.c_str());
        flag_db_->initFP(32);
        flag_db_->initTmpN(30000000);
        if(boost::filesystem::exists(db_path_))
        {
            db_->load(db_path_.c_str());
        }
        else if(boost::filesystem::exists(text_path_))
        {
            LOG(INFO)<<"db empty, loading text"<<std::endl;
            load_text(text_path_, false);
            if(!flush()) return false;
        }
        if(boost::filesystem::exists(flag_db_path_))
        {
            flag_db_->load(flag_db_path_.c_str());
        }
        text_.open(text_path_.c_str(), std::ios::out | std::ios::app);
        is_open_ = true;
        return true;
    }

    bool load_text(const std::string& path, bool text = true)
    {
        LOG(INFO)<<"loading text "<<path<<std::endl;
        std::ifstream ifs(path.c_str());
        std::string line;
        uint32_t i = 0;
        while( getline(ifs,line))
        {
            ++i;
            if(i%100000==0)
            {
                LOG(INFO)<<"loading "<<i<<" item"<<std::endl;
            }
            boost::algorithm::trim(line);
            std::vector<std::string> vec;
            boost::algorithm::split(vec, line, boost::is_any_of(","));
            if(vec.size()<2) continue;
            const std::string& soid = vec[0];
            const std::string& spid = vec[1];
            KeyType ioid = B5MHelper::StringToUint128(soid);
            ValueType ipid = B5MHelper::StringToUint128(spid);
            insert_(ioid, ipid, text);
        }
        ifs.close();
        return true;
    }

    bool load_text(const std::string& path, boost::unordered_set<uint128_t>& pid_changed_oid, bool text = true)
    {
        LOG(INFO)<<"loading text "<<path<<std::endl;
        std::ifstream ifs(path.c_str());
        std::string line;
        uint32_t i = 0;
        while( getline(ifs,line))
        {
            ++i;
            if(i%100000==0)
            {
                LOG(INFO)<<"loading "<<i<<" item"<<std::endl;
            }
            boost::algorithm::trim(line);
            std::vector<std::string> vec;
            boost::algorithm::split(vec, line, boost::is_any_of(","));
            if(vec.size()<3) continue;
            const std::string& soid = vec[0];
            const std::string& spid = vec[1];
            const std::string& old_spid = vec[2];
            KeyType ioid = B5MHelper::StringToUint128(soid);
            ValueType ipid = B5MHelper::StringToUint128(spid);
            insert_(ioid, ipid, text);
            if(!old_spid.empty())
            {
                ValueType old_ipid = B5MHelper::StringToUint128(old_spid);
                if(old_ipid!=ipid)
                {
                    pid_changed_oid.insert(ioid);
                }
            }
        }
        ifs.close();
        return true;
    }

    bool insert(const KeyType& key, const ValueType& value)
    {
        if(!lazy_)
        {
            boost::unique_lock<boost::shared_mutex> lock(mutex_);
            return insert_(key, value, false);
        }
        else
        {
            boost::unique_lock<boost::mutex> lock(lazy_mutex_);
            return lazy_insert_(key, value);
        }
    }

    bool insert(const std::string& soid, const std::string& spid)
    {
        return insert(B5MHelper::StringToUint128(soid), B5MHelper::StringToUint128(spid));
    }

    void insert_flag(const std::string& soid, const FlagType& flag)
    {
        boost::unique_lock<boost::shared_mutex> lock(mutex_);
        flag_db_->setInteger(B5MHelper::StringToUint128(soid), flag);
        has_modify_ = true;
    }

    void clear_all_flag()
    {
        boost::unique_lock<boost::shared_mutex> lock(mutex_);
        if(flag_db_!=NULL)
        {
            delete flag_db_;
        }
        if(boost::filesystem::exists(flag_tmp_path_))
        {
            boost::filesystem::remove_all(flag_tmp_path_);
        }
        if(boost::filesystem::exists(flag_db_path_))
        {
            boost::filesystem::remove_all(flag_db_path_);
        }
        flag_db_ = new FlagDbType(flag_tmp_path_.c_str());
        flag_db_->initFP(32);
        flag_db_->initTmpN(30000000);
        has_modify_ = true;
    }

    bool get(const KeyType& key, ValueType& value)
    {
        //boost::unique_lock<boost::shared_mutex> lock(mutex_);
        boost::shared_lock<boost::shared_mutex> lock(mutex_, boost::defer_lock_t());
        if(!lazy_) lock.lock();
        value = db_->getInteger(key);
        if(value==(ValueType)izenelib::am::succinct::fujimap::NOTFOUND)
        {
            return false;
        }
        return true;
    }

    bool get(const std::string& soid, std::string& spid)
    {
        uint128_t pid;
        if(!get(B5MHelper::StringToUint128(soid), pid)) return false;
        spid = B5MHelper::Uint128ToString(pid);
        //if(soid=="418e05ac51300093a5175aa74d231f6f"||soid=="013a1fa3904e7f72a250b4ed8c9b2c68")
        //{
        //    std::cerr<<"[ODEBUG]"<<soid<<","<<spid<<std::endl;
        //}
        return true;
    }

    bool get_flag(const std::string& key, FlagType& flag)
    {
        //boost::unique_lock<boost::shared_mutex> lock(mutex_);
        flag = flag_db_->getInteger(B5MHelper::StringToUint128(key));
        if(flag==(FlagType)izenelib::am::succinct::fujimap::NOTFOUND)
        {
            return false;
        }
        return true;
    }

    //bool erase(const KeyType& key)
    //{
        //has_modify_ = true;
        //return db_.erase(key)>0;
    //}

    //bool erase(const std::string& soid)
    //{
        //return erase(B5MHelper::StringToUint128(soid));
    //}

    bool flush()
    {
        boost::unique_lock<boost::shared_mutex> lock(mutex_);
        boost::unique_lock<boost::mutex> lock2(lazy_mutex_);
        LOG(INFO)<<"try flush odb.."<<std::endl;
        if(text_.is_open())
        {
            text_.flush();
        }
        if(has_modify_)
        {
            if(lazy_)
            {
                LOG(INFO)<<"lazy mode"<<std::endl;
                if(buffer_ofs_.is_open())
                {
                    LOG(INFO)<<"try insert buffer"<<std::endl;
                    buffer_ofs_.close();
                    std::ifstream ifs(buffer_path_.c_str());
                    std::string line;
                    std::size_t count=0;
                    while(getline(ifs, line))
                    {
                        if(line.length()<64) continue;
                        std::string skey = line.substr(0, 32);
                        std::string svalue = line.substr(32, 32);
                        insert_(B5MHelper::StringToUint128(skey), B5MHelper::StringToUint128(svalue), false);
                        ++count;
                    }
                    ifs.close();
                    has_modify_ = false;
                    LOG(INFO)<<"inserted "<<count<<" buffer into odb"<<std::endl;
                }
                if(boost::filesystem::exists(buffer_path_))
                {
                    boost::filesystem::remove_all(buffer_path_);
                }
            }
            LOG(INFO)<<"building fujimap.."<<std::endl;
            if(db_->build()==-1)
            {
                LOG(ERROR)<<"fujimap build error"<<std::endl;
                return false;
            }
            boost::filesystem::remove_all(db_path_);
            db_->save(db_path_.c_str());
            if(flag_db_->build()==-1)
            {
                LOG(ERROR)<<"fujimap build error"<<std::endl;
                return false;
            }
            boost::filesystem::remove_all(flag_db_path_);
            flag_db_->save(flag_db_path_.c_str());
            has_modify_ = false;
        }
        else
        {
            LOG(INFO)<<"db not change"<<std::endl;
        }
        return true;
    }

private:
    bool insert_(const KeyType& key, const ValueType& value, bool text)
    {
        ValueType evalue = db_->getInteger(key);
        if(evalue==value)
        {
            return false;
        }
        db_->setInteger(key, value);
        has_modify_ = true;
        if(text)
        {
            text_<<B5MHelper::Uint128ToString(key)<<","<<B5MHelper::Uint128ToString(value)<<std::endl;
        }
        return true;
    }
    bool lazy_insert_(const KeyType& key, const ValueType& value)
    {
        std::string skey = B5MHelper::Uint128ToString(key);
        std::string svalue = B5MHelper::Uint128ToString(value);
        std::string line = skey+svalue;
        if(!buffer_ofs_.is_open())
        {
            buffer_ofs_.open(buffer_path_.c_str());
        }
        buffer_ofs_<<line<<std::endl;
        if(buffer_ofs_.fail()) return false;
        has_modify_ = true;
        return true;
    }

private:

    std::string path_;
    std::string db_path_;
    std::string text_path_;
    std::string tmp_path_;
    std::string buffer_path_;
    std::string flag_db_path_;
    std::string flag_tmp_path_;
    DbType* db_;
    FlagDbType* flag_db_;
    std::ofstream text_;
    std::ofstream buffer_ofs_;
    bool is_open_;
    bool has_modify_;
    bool lazy_;
    boost::shared_mutex mutex_;
    boost::mutex lazy_mutex_;
};
NS_IDMLIB_B5M_END

#endif

