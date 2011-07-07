#ifndef IDMLIB_RESYS_ITEM_COVISITATION_H
#define IDMLIB_RESYS_ITEM_COVISITATION_H

#include "ItemRescorer.h"
#include <idmlib/idm_types.h>

#include <am/matrix/matrix_db.h>

#include <util/ThreadModel.h>
#include <util/PriorityQueue.h>

#include <boost/shared_ptr.hpp>

#include <string>
#include <vector>
#include <list>
#include <map>
#include <algorithm>

#include <sys/time.h>

using namespace std;

NS_IDMLIB_RESYS_BEGIN

typedef uint32_t ItemType;

struct CoVisitTimeFreq :public ::izenelib::util::pod_tag
{
    CoVisitTimeFreq():freq(0),timestamp(0){}

    uint32_t freq;
    int64_t timestamp;

    void update()
    {
        freq += 1;
        timestamp = (int64_t)izenelib::util::Timestamp::now();
    }
};

struct CoVisitFreq :public ::izenelib::util::pod_tag
{
    CoVisitFreq():freq(0){}

    uint32_t freq;

    void update()
    {
        freq += 1;
    }
};

template<typename CoVisitation>
struct CoVisitationQueueItem
{
    CoVisitationQueueItem()  {}
    CoVisitationQueueItem(ItemType item, CoVisitation covisit)
        :itemId(item),covisitation(covisit){}
    ItemType itemId;
    CoVisitation covisitation;
};

template<typename CoVisitation>
class CoVisitationQueue : public izenelib::util::PriorityQueue<CoVisitationQueueItem<CoVisitation> >
{
public:
    CoVisitationQueue(size_t size)
    {
        this->initialize(size);
    }
protected:
    bool lessThan(CoVisitationQueueItem<CoVisitation> o1, CoVisitationQueueItem<CoVisitation> o2)
    {
        return (o1.covisitation.freq < o2.covisitation.freq);
    }
};

template<typename CoVisitation>
class ItemCoVisitation
{
    typedef izenelib::am::MatrixDB<ItemType, CoVisitation > MatrixDBType;

public:
    typedef typename MatrixDBType::row_type RowType;
    typedef typename MatrixDBType::iterator iterator; // first is ItemType, second is RowType

    ItemCoVisitation(
          const std::string& homePath, 
          size_t cache_size = 1024*1024
          )
        : db_(cache_size, homePath)
    {
    }

    ~ItemCoVisitation()
    {
        dump();
    }

    /**
     * Update items visited by one user.
     * @param oldItems the items visited before
     * @param newItems the new visited items
     *
     * As the covisit matrix records the number of users who have visited both item i and item j,
     * the @c visit function has below pre-conditions:
     * @pre each items in @p oldItems should be unique
     * @pre each items in @p newItems should be unique
     * @pre there should be no items contained in both @p oldItems and @p newItems
     *
     * for performance reason, it has below post-condition:
     * @post when @c visit function returns, the items in @p newItems would be moved to the end of @p oldItems,
     *       and @p newItems would be empty.
     */
    void visit(std::list<ItemType>& oldItems, std::list<ItemType>& newItems)
    {
        izenelib::util::ScopedWriteLock<izenelib::util::ReadWriteLock> lock(lock_);

        std::list<ItemType>::const_iterator iter;
        if (oldItems.empty())
        {
            // update new*new pairs
            for(iter = newItems.begin(); iter != newItems.end(); ++iter)
                updateCoVisation(*iter, newItems);

            // for post condition
            oldItems.swap(newItems);
        }
        else
        {
            // update old*new pairs
            for(iter = oldItems.begin(); iter != oldItems.end(); ++iter)
                updateCoVisation(*iter, newItems);

            // move new items into oldItems
            std::list<ItemType>::const_iterator newItemIt = oldItems.end();
            --newItemIt;
            oldItems.splice(oldItems.end(), newItems);
            ++newItemIt;

            // update new*total pairs
            for(iter = newItemIt; iter != oldItems.end(); ++iter)
                updateCoVisation(*iter, oldItems);
        }
    }

    void getCoVisitation(size_t howmany, ItemType item, std::vector<ItemType>& results, ItemRescorer* rescorer = NULL)
    {
        izenelib::util::ScopedReadLock<izenelib::util::ReadWriteLock> lock(lock_);

        boost::shared_ptr<const RowType> rowdata = db_.row(item);
        CoVisitationQueue<CoVisitation> queue(howmany);
        typename RowType::const_iterator iter = rowdata->begin();
        for(;iter != rowdata->end(); ++iter)
        {
            // escape the input item
            if (iter->first != item
                && (!rescorer || !rescorer->isFiltered(iter->first)))
            {
                queue.insert(CoVisitationQueueItem<CoVisitation>(iter->first,iter->second));
            }
        }
        results.resize(queue.size());
        for(std::vector<ItemType>::reverse_iterator rit = results.rbegin(); rit != results.rend(); ++rit)
            *rit = queue.pop().itemId;
    }

    void getCoVisitation(size_t howmany, ItemType item, std::vector<ItemType>& results, int64_t timestamp, ItemRescorer* rescorer = NULL)
    {
        izenelib::util::ScopedReadLock<izenelib::util::ReadWriteLock> lock(lock_);

        boost::shared_ptr<const RowType> rowdata = db_.row(item);

        CoVisitationQueue<CoVisitation> queue(howmany);
        typename RowType::const_iterator iter = rowdata->begin();
        for(;iter != rowdata->end(); ++iter)
        {
            // escape the input item
            if (iter->timestamp >= timestamp && iter->first != item
                && (!rescorer || !rescorer->isFiltered(iter->first)))
            {
                queue.insert(CoVisitationQueueItem<CoVisitation>(iter->first,iter->second));
            }
        }
        results.resize(queue.size());
        for(std::vector<ItemType>::reverse_iterator rit = results.rbegin(); rit != results.rend(); ++rit)
            *rit = queue.pop().itemId;
    }

    uint32_t coeff(ItemType row, ItemType col)
    {
        //we do not use read lock here because coeff is called only after writing operation is finished
        //izenelib::util::ScopedReadLock<izenelib::util::ReadWriteLock> lock(lock_);
        return db_.coeff(row,col).freq;
    }

    void dump()
    {
        db_.dump();
    }

    void status(std::ostream& ostream)
    {
        db_.status(ostream);
    }

    iterator begin()
    {
        return db_.begin();
    }

    iterator end()
    {
        return db_.end();
    }

private:
    void updateCoVisation(ItemType item, const std::list<ItemType>& coItems)
    {
        if(coItems.empty()) return;
        db_.row_incre(item, coItems);
    }

private:
    MatrixDBType db_;
    izenelib::util::ReadWriteLock lock_;
};


NS_IDMLIB_RESYS_END

#endif

