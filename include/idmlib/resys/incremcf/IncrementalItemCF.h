#ifndef IDMLIB_RESYS_INCREMENTAL_ITEM_CF_H
#define IDMLIB_RESYS_INCREMENTAL_ITEM_CF_H

#include <idmlib/idm_types.h>

#include "UserRecStorage.h"
#include <idmlib/resys/ItemCF.h>
#include <idmlib/resys/ItemRescorer.h>
#include <idmlib/resys/RecommendItem.h>
#include <idmlib/resys/ItemCoVisitation.h>
#include <idmlib/resys/similarity/SimilarityMatrix.h>

#include <vector>
#include <set>
#include <string>

NS_IDMLIB_RESYS_BEGIN

class IncrementalItemCF : public ItemCF
{
public:
    IncrementalItemCF(
        const std::string& covisit_path, 
        size_t covisit_row_cache_size,
        const std::string& item_item_similarity_path,
        size_t similarity_row_cache_size, 
        const std::string& item_neighbor_path,
        size_t topK,
        const std::string& user_recommendItem_path,
        size_t max_items_stored_for_each_user = 1000
        );

    ~IncrementalItemCF();

public:
    /**
     * Update covisist and similarity matrix.
     * @param oldItems the items visited before
     * @param newItems the new visited items
     *
     * As the covisit matrix records the number of users who have visited both item i and item j,
     * it has below pre-conditions:
     * @pre each items in @p oldItems should be unique
     * @pre each items in @p newItems should be unique
     * @pre there should be no items contained in both @p oldItems and @p newItems
     *
     * for performance reason, it has below post-condition:
     * @post when this function returns, the items in @p newItems would be moved to the end of @p oldItems,
     *       and @p newItems would be empty.
     */
    void buildMatrix(
        std::list<uint32_t>& oldItems,
        std::list<uint32_t>& newItems
    );

    /**
     * Update covisist matrix.
     * @param oldItems the items visited before
     * @param newItems the new visited items
     *
     * for performance reason, it has below post-condition:
     * @post when this function returns, the items in @p newItems would be moved to the end of @p oldItems,
     *       and @p newItems would be empty.
     */
    void updateVisitMatrix(
        std::list<uint32_t>& oldItems,
        std::list<uint32_t>& newItems
    );

    /**
     * Rebuild the whole similarity matrix in batch mode.
     * For each row in @c covisitation_, to calculate:
     * 1. the similarity value for each column
     * 2. the top K nieghbors for the row
     */
    void buildSimMatrix();

    /**
     * Build recommend items for user.
     * @param userId the user id
     * @param visitItems the items already visited by the user
     * @param rescorer filter items
     */
    void buildUserRecItems(
        uint32_t userId,
        const std::set<uint32_t>& visitItems,
        ItemRescorer* rescorer = NULL
    );

    /**
     * Given @p userId, recommend @p recItems, which is built by @c buildUserRecItems() previously.
     * @param howMany the max number of items to recommend
     * @param userId the user id
     * @param recItems the recommend result
     * @param rescorer filter items
     */
    void getRecByUser(
        int howMany, 
        uint32_t userId,
        RecommendItemVec& recItems, 
        ItemRescorer* rescorer = NULL
    );

    /**
     * Given @p visitItems, recommend @p recItems.
     * @param howMany the max number of items to recommend
     * @param visitItems the items already visited by user
     * @param recItems the recommend result
     * @param rescorer filter items
     */
    void getRecByItem(
        int howMany,
        const std::vector<uint32_t>& visitItems,
        RecommendItemVec& recItems, 
        ItemRescorer* rescorer = NULL
    );

    void flush();

private:
    /**
     * Update similarity matrix.
     * @param rows the row numbers to update
     */
    void updateSimMatrix_(const std::list<uint32_t>& rows);

    /**
     * Calculate the value of similarity matrix[row][col].
     * @param row the row number of similarity matrix
     * @param col the column number of similarity matrix
     * @return the value for similarity matrix[row][col]
     */
    float calcSimValue_(
        uint32_t row,
        uint32_t col
    );

    /**
     * Given @p visitItems, recommend @p recItems.
     * @param howMany the max number of items to recommend
     * @param visitItems the items already visited by user
     * @param recItems the recommend result
     * @param rescorer filter items
     */
    void recommend_(
        int howMany, 
        const std::set<uint32_t>& visitItems,
        RecommendItemVec& recItems, 
        ItemRescorer* rescorer = NULL
    );

private:
    ItemCoVisitation<CoVisitFreq> covisitation_;
    SimilarityMatrix<uint32_t,float> similarity_;
    UserRecStorage userRecStorage_;
    size_t max_items_stored_for_each_user_;

    friend class ItemCFTest;
};

NS_IDMLIB_RESYS_END

#endif