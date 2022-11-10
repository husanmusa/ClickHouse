#pragma once

#include <Storages/MergeTree/MergeTreeBlockReadUtils.h>
#include <Storages/MergeTree/MergeTreeData.h>
#include <Storages/SelectQueryInfo.h>
#include <Storages/MergeTree/IMergeTreeReader.h>
#include <Storages/MergeTree/RequestResponse.h>

#include <Processors/ISource.h>


namespace DB
{

class IMergeTreeReader;
class UncompressedCache;
class MarkCache;
struct PrewhereExprInfo;


struct ParallelReadingExtension
{
    MergeTreeAllRangesCallback all_callback;
    MergeTreeReadTaskCallback callback;
    size_t count_participating_replicas{0};
    size_t number_of_current_replica{0};
    /// This is needed to estimate the number of bytes
    /// between a pair of marks to perform one request
    /// over the network for a 1Gb of data.
    Names colums_to_read;
};

/// Base class for MergeTreeThreadSelectProcessor and MergeTreeSelectProcessor
class MergeTreeBaseSelectProcessor : public ISource
{
public:
    MergeTreeBaseSelectProcessor(
        Block header,
        const MergeTreeData & storage_,
        const StorageSnapshotPtr & storage_snapshot_,
        const PrewhereInfoPtr & prewhere_info_,
        ExpressionActionsSettings actions_settings,
        UInt64 max_block_size_rows_,
        UInt64 preferred_block_size_bytes_,
        UInt64 preferred_max_column_in_block_size_bytes_,
        const MergeTreeReaderSettings & reader_settings_,
        bool use_uncompressed_cache_,
        const Names & virt_column_names_ = {});

    ~MergeTreeBaseSelectProcessor() override;

    static Block transformHeader(
        Block block, const PrewhereInfoPtr & prewhere_info, const DataTypePtr & partition_value_type, const Names & virtual_columns);

    static std::unique_ptr<MergeTreeBlockSizePredictor> getSizePredictor(
        const MergeTreeData::DataPartPtr & data_part,
        const MergeTreeReadTaskColumns & task_columns,
        const Block & sample_block);

protected:
    /// This struct allow to return block with no columns but with non-zero number of rows similar to Chunk
    struct BlockAndRowCount
    {
        Block block;
        size_t row_count = 0;
    };

    Chunk generate() final;

    /// Creates new this->task and return a flag whether it was successful or not
    virtual bool getNewTaskImpl() = 0;
    /// Creates new readers for a task it is needed. These methods are separate, because
    /// in case of parallel reading from replicas the whole task could be denied by a coodinator
    /// or it could modified somehow.
    virtual void finalizeNewTask() = 0;

    size_t estimateMaxBatchSizeForHugeRanges();

    /// Closes readers and unlock part locks
    virtual void finish() = 0;

    virtual BlockAndRowCount readFromPart();

    BlockAndRowCount readFromPartImpl();

    /// Used for filling header with no rows as well as block with data
    static void
    injectVirtualColumns(Block & block, size_t row_count, MergeTreeReadTask * task, const DataTypePtr & partition_value_type, const Names & virtual_columns);

    static std::unique_ptr<PrewhereExprInfo> getPrewhereActions(PrewhereInfoPtr prewhere_info, const ExpressionActionsSettings & actions_settings);

    static void initializeRangeReadersImpl(
         MergeTreeRangeReader & range_reader,
         std::deque<MergeTreeRangeReader> & pre_range_readers,
         PrewhereInfoPtr prewhere_info,
         const PrewhereExprInfo * prewhere_actions,
         IMergeTreeReader * reader,
         bool has_lightweight_delete,
         const MergeTreeReaderSettings & reader_settings,
         const std::vector<std::unique_ptr<IMergeTreeReader>> & pre_reader_for_step,
         const PrewhereExprStep & lightweight_delete_filter_step,
         const Names & non_const_virtual_column_names);

    /// Sets up data readers for each step of prewhere and where
    void initializeMergeTreeReadersForPart(
        MergeTreeData::DataPartPtr & data_part,
        const MergeTreeReadTaskColumns & task_columns, const StorageMetadataPtr & metadata_snapshot,
        const MarkRanges & mark_ranges, const IMergeTreeReader::ValueSizeMap & value_size_map,
        const ReadBufferFromFileBase::ProfileCallback & profile_callback);

    /// Sets up range readers corresponding to data readers
    void initializeRangeReaders(MergeTreeReadTask & task);

    const MergeTreeData & storage;
    StorageSnapshotPtr storage_snapshot;

    /// This step is added when the part has lightweight delete mask
    const PrewhereExprStep lightweight_delete_filter_step { nullptr, LightweightDeleteDescription::FILTER_COLUMN.name, true, true };
    PrewhereInfoPtr prewhere_info;
    std::unique_ptr<PrewhereExprInfo> prewhere_actions;

    UInt64 max_block_size_rows;
    UInt64 preferred_block_size_bytes;
    UInt64 preferred_max_column_in_block_size_bytes;

    MergeTreeReaderSettings reader_settings;

    bool use_uncompressed_cache;

    Names virt_column_names;

    /// These columns will be filled by the merge tree range reader
    Names non_const_virtual_column_names;

    DataTypePtr partition_value_type;

    /// This header is used for chunks from readFromPart().
    Block header_without_virtual_columns;

    std::shared_ptr<UncompressedCache> owned_uncompressed_cache;
    std::shared_ptr<MarkCache> owned_mark_cache;

    using MergeTreeReaderPtr = std::unique_ptr<IMergeTreeReader>;
    MergeTreeReaderPtr reader;
    std::vector<MergeTreeReaderPtr> pre_reader_for_step;

    MergeTreeReadTaskPtr task;
private:
    Poco::Logger * log = &Poco::Logger::get("MergeTreeBaseSelectProcessor");

    bool getNewTask();

};

}
