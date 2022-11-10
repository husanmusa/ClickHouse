#include <Storages/MergeTree/RequestResponse.h>

#include <Core/ProtocolDefines.h>
#include <Common/SipHash.h>
#include "IO/VarInt.h"
#include <IO/WriteHelpers.h>
#include <IO/ReadHelpers.h>

#include <consistent_hashing.h>

namespace DB
{

namespace ErrorCodes
{
    extern const int UNKNOWN_PROTOCOL;
}

void ParallelReadRequest::serialize(WriteBuffer & out) const
{
    /// Must be the first
    writeIntBinary(DBMS_PARALLEL_REPLICAS_PROTOCOL_VERSION, out);

    writeIntBinary(replica_num, out);
    writeIntBinary(min_number_of_marks, out);
}


void ParallelReadRequest::describe(WriteBuffer & out) const
{
    String result;
    result += fmt::format("replica_num: {} \n", replica_num);
    result += fmt::format("min_num_of_marks: {} \n", min_number_of_marks);
    out.write(result.c_str(), result.size());
}

void ParallelReadRequest::deserialize(ReadBuffer & in)
{
    UInt64 version;
    readIntBinary(version, in);
    if (version != DBMS_PARALLEL_REPLICAS_PROTOCOL_VERSION)
        throw Exception(ErrorCodes::UNKNOWN_PROTOCOL, "Protocol versions for parallel reading \
            from replicas differ. Got: {}, supported version: {}",
            version, DBMS_PARALLEL_REPLICAS_PROTOCOL_VERSION);

    readIntBinary(replica_num, in);
    readIntBinary(min_number_of_marks, in);
}

void ParallelReadResponse::serialize(WriteBuffer & out) const
{
    /// Must be the first
    writeVarUInt(DBMS_PARALLEL_REPLICAS_PROTOCOL_VERSION, out);

    writeBoolText(finish, out);
    description.serialize(out);
}

void ParallelReadResponse::deserialize(ReadBuffer & in)
{
    UInt64 version;
    readIntBinary(version, in);
    if (version != DBMS_PARALLEL_REPLICAS_PROTOCOL_VERSION)
        throw Exception(ErrorCodes::UNKNOWN_PROTOCOL, "Protocol versions for parallel reading \
            from replicas differ. Got: {}, supported version: {}",
            version, DBMS_PARALLEL_REPLICAS_PROTOCOL_VERSION);

    readBoolText(finish, in);
    description.deserialize(in);
}


void InitialAllRangesAnnouncement::serialize(WriteBuffer & out) const
{
    description.serialize(out);
    writeIntBinary(replica_num, out);
}


void InitialAllRangesAnnouncement::describe(WriteBuffer & out)
{
    description.describe(out);
    auto result = fmt::format("----------\nReceived from {} replica\n", replica_num);
    out.write(result.c_str(), result.size());
}

void InitialAllRangesAnnouncement::deserialize(ReadBuffer & in)
{
    description.deserialize(in);
    readIntBinary(replica_num, in);
}

}
