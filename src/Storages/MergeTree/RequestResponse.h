#pragma once

#include <functional>
#include <optional>

#include <base/types.h>

#include <IO/WriteBuffer.h>
#include <IO/ReadBuffer.h>

#include <Storages/MergeTree/MarkRange.h>
#include <Storages/MergeTree/RangesInDataPart.h>


namespace DB
{

/// Represents a segment [left; right]
struct PartBlockRange
{
    Int64 begin;
    Int64 end;

    bool operator==(const PartBlockRange & rhs) const
    {
        return begin == rhs.begin && end == rhs.end;
    }
};

struct ParallelReadRequest
{
    size_t replica_num;
    size_t min_number_of_marks;

    void serialize(WriteBuffer & out) const;
    void describe(WriteBuffer & out) const;
    void deserialize(ReadBuffer & in);
};

struct ParallelReadResponse
{
    bool finish;
    RangesInDataPartsDescription description;

    void serialize(WriteBuffer & out) const;
    void deserialize(ReadBuffer & in);
};


struct InitialAllRangesAnnouncement
{
    RangesInDataPartsDescription description;
    size_t replica_num;

    void serialize(WriteBuffer & out) const;
    void deserialize(ReadBuffer & in);
};

using MergeTreeAllRangesCallback = std::function<void(InitialAllRangesAnnouncement)>;
using MergeTreeReadTaskCallback = std::function<std::optional<ParallelReadResponse>(ParallelReadRequest)>;


}
