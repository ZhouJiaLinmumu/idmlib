/**
 * @file MemWikiIndex.h
 * @author Zhongxia Li
 * @date Jun 2, 2011
 * @brief
 */
#ifndef MEM_WIKI_INDEX_H_
#define MEM_WIKI_INDEX_H_

#include <idmlib/idm_types.h>

#include "WikiIndex.h"
#include "SparseVector.h"

#include <fstream>
#include <iostream>
#include <boost/filesystem.hpp>
#include <boost/serialization/access.hpp>
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/vector.hpp>

//#define MEMWIKI_INDEX_TEST

NS_IDMLIB_SSP_BEGIN

/**
 * Memory resident inverted index
 * @brief To ensure the performance of ESA, the inverted index of Wikipeida should be
 * kept in memory, in case the heavy i/o overhead of a disk resident index.
 * The memory resident inverted index can be entirely saved to (or loaded from) disk.
 *
 */
class MemWikiIndex : public WikiIndex
{
public:
    MemWikiIndex(const string& indexDir = "./esa/wiki")
            : WikiIndex(indexDir)
    {

    }

    virtual ~MemWikiIndex() {}
public:
    void insertDocument(WikiDoc& wikiDoc)
    {
        docCount_ ++;
        curDoc_ = wikiDoc.docid;

        gatherTF_(wikiDoc);

        updateInvertIndex_();
    }

    void postProcess()
    {
        calcWeight_();

#ifdef MEMWIKI_INDEX_TEST
        printWikiIndex();
#endif

        flush();
        clear();
    }

    bool load()
    {
        clear();

        return loadIndex_();

#ifdef MEMWIKI_INDEX_TEST
        printWikiIndex();
#endif
    }

public:
    SparseVectorType& getInvertedList(uint32_t termid)
    {
        invertedlists_iterator_t ret = invertedLists_.find(termid);
        if (ret != invertedLists_.end())
        {
            return *(ret->second);
        }

        return NullInvertedList_;
    }

private:
    void updateInvertIndex_()
    {
        uint32_t termid;
        float w;
        for (termtf_map_iterator_t iter = term_tf_map_.begin(); iter != term_tf_map_.end(); iter++)
        {
            termid = iter->first;
            w = iter->second; // tf

            invertedlists_iterator_t ret = invertedLists_.find(termid);
            if (ret == invertedLists_.end())
            {
                // term has not been indexed yet
                boost::shared_ptr<InvertedList> invertList(new InvertedList(termid));
                invertList->insertItem(curDoc_, w);
                invertedLists_.insert(invertedlists_t::value_type(termid, invertList));
            }
            else
            {
                // term has been indexed
                ret->second->insertItem(curDoc_, w);
            }
        }
    }

    /**
     * Calculate term-concept weights over all wiki index using TFIDF scheme.
     */
    void calcWeight_()
    {
        /// todo filt
        float idf;
        for (invertedlists_iterator_t it = invertedLists_.begin(); it != invertedLists_.end(); it++)
        {
            idf = std::log((float)docCount_ / it->second->len);

            InvertedList::list_iter_t iit;
            for (iit = it->second->list.begin(); iit != it->second->list.end(); )
            {
                iit->value *= idf; // tf * idf
                if (iit->value > thresholdWegt_)
                {
                    iit++;
                }
                else
                {
                    // remove those concepts whose weights for a given word are too low
                    iit = it->second->list.erase(iit);
                    it->second->len --;
                }
            }

            if (it->second->len <= 0)
            {
                it->second.reset();
            }
        }
    }

    void flush()
    {
        // save whole inverted index (just 1 barrel)
        // todo: if wikipedia corpus is large, we save each barrel as a sub inverted index which fit memory size.
        // each sub index can be used to calculate a partial interpretation vector in esa.

        std::cout<<"Saving Wikipedia index (term "<<invertedLists_.size()<<",concept "<<docCount_<<"): "<<dataFile_<<endl;
        std::ofstream fout(dataFile_.c_str(), ios::out);
        if (!fout.is_open())
        {
            std::cout <<"Failed to creatd: "<<dataFile_<<endl;
            return;
        }

        boost::archive::text_oarchive oa(fout);

        size_t count = invertedLists_.size(); //xxx some lists are null
        oa << count;

        for (invertedlists_iterator_t it = invertedLists_.begin(); it != invertedLists_.end(); it++)
        {
            if (it->second)
                oa << *it->second;
        }

        fout.close();
    }

    void clear()
    {
        init();

        invertedLists_.clear();
    }

    bool loadIndex_()
    {
        std::ifstream fin(dataFile_.c_str());
        if (!fin.is_open())
        {
            std::cout <<"Failed to open: "<<dataFile_<<endl;
            return false;
        }
        cout <<dataFile_<<endl;

        boost::archive::text_iarchive ia(fin);

        // read list count
        size_t count;
        ia >> count;

        for (size_t i = 0; i < count; i++)
        {
            boost::shared_ptr<InvertedList> invertedList(new InvertedList());
            try
            {
                ia >> *invertedList;
            }
            catch (std::exception& ex)
            {
                break;
            }
            invertedLists_.insert(invertedlists_t::value_type(invertedList->rowid, invertedList));
        }
        fin.close();

        return true;
    }

public:
    /// for test
    void printWikiIndex()
    {
        cout << "---- Wiki Index -----" <<endl;
        for (invertedlists_iterator_t it = invertedLists_.begin(); it != invertedLists_.end(); it++)
        {
            if (!it->second)
                continue;
            cout << "["<<it->second->rowid << " " << it->second->len << "] --> ";
            InvertedList::list_iter_t iit;
            for (iit = it->second->list.begin(); iit != it->second->list.end(); iit++)
            {
                cout << "(" << iit->itemid <<","<<iit->value<<") ";
            }
            cout << endl;
        }
    }

    /*
    private:
        struct InvertItem
        {
            uint32_t conceptId;
            float weight;

            InvertItem() {}
            InvertItem(uint32_t conceptId, float w)
            : conceptId(conceptId), weight(w) {}

            friend class boost::serialization::access;
            template<class Archive>
            void serialize(Archive & ar, const unsigned int version)
            {
                ar & conceptId;
                ar & weight;
            }
        };

        struct InvertedList
        {
            uint32_t termid;
            uint32_t len; ///< list length, or Document Frequence(DF).
            std::vector<InvertItem> list;

            InvertedList() {}
            InvertedList(uint32_t termid, size_t size=0)
            : termid(termid), len(0)
            {
                //list.resize(size); //xxx
            }

            void insertDoc(uint32_t docid, float w)
            {
                len ++;
                list.push_back(InvertItem(docid, w));
            }

            friend class boost::serialization::access;
            template<class Archive>
            void serialize(Archive & ar, const unsigned int version)
            {
                ar & termid;
                ar & len;
                ar & list;
            }
        };
    */

private:
    static const float thresholdWegt_ ; // 0.015 ~ 0.02 ?

    typedef SparseVectorItemType InvertItem;
    typedef SparseVectorType InvertedList;

    typedef rde::hash_map<uint32_t, boost::shared_ptr<InvertedList> > invertedlists_t;
    typedef rde::hash_map<uint32_t, boost::shared_ptr<InvertedList> >::iterator invertedlists_iterator_t;

    invertedlists_t invertedLists_;
    SparseVectorType NullInvertedList_; // an empty inverted list
};

NS_IDMLIB_SSP_END

#endif /* MEM_WIKI_INDEX_H_ */
