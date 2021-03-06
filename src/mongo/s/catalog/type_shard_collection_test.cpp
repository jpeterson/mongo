/**
 *    Copyright (C) 2018-present MongoDB, Inc.
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the Server Side Public License, version 1,
 *    as published by MongoDB, Inc.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    Server Side Public License for more details.
 *
 *    You should have received a copy of the Server Side Public License
 *    along with this program. If not, see
 *    <http://www.mongodb.com/licensing/server-side-public-license>.
 *
 *    As a special exception, the copyright holders give permission to link the
 *    code of portions of this program with the OpenSSL library under certain
 *    conditions as described in each individual source file and distribute
 *    linked combinations including the program with the OpenSSL library. You
 *    must comply with the Server Side Public License in all respects for
 *    all of the code used other than as permitted herein. If you modify file(s)
 *    with this exception, you may extend this exception to your version of the
 *    file(s), but you are not obligated to do so. If you do not wish to do so,
 *    delete this exception statement from your version. If you delete this
 *    exception statement from all source files in the program, then also delete
 *    it in the license file.
 */

#include "mongo/platform/basic.h"

#include "mongo/s/catalog/type_shard_collection.h"

#include "mongo/bson/oid.h"
#include "mongo/unittest/unittest.h"
#include "mongo/util/time_support.h"

namespace mongo {
namespace {

using unittest::assertGet;

const NamespaceString kNss = NamespaceString("db.coll");
const BSONObj kKeyPattern = BSON("a" << 1);
const BSONObj kDefaultCollation = BSON("locale"
                                       << "fr_CA");

TEST(ShardCollectionType, FromBSONEmptyShardKeyFails) {
    BSONObjBuilder builder;
    builder.append(ShardCollectionType::kNssFieldName, kNss.ns());
    builder.append(ShardCollectionType::kEpochFieldName, OID::gen());
    builder.append(ShardCollectionType::kKeyPatternFieldName, BSONObj());
    builder.append(ShardCollectionType::kUniqueFieldName, true);

    StatusWith<ShardCollectionType> status = ShardCollectionType::fromBSON(builder.obj());
    ASSERT_EQ(status.getStatus().code(), ErrorCodes::ShardKeyNotFound);
}

TEST(ShardCollectionType, FromBSONEpochMatchesLastRefreshedCollectionVersionWhenBSONTimestamp) {
    OID epoch = OID::gen();

    BSONObjBuilder builder;
    builder.append(ShardCollectionType::kNssFieldName, kNss.ns());
    builder.append(ShardCollectionType::kEpochFieldName, epoch);
    builder.append(ShardCollectionType::kKeyPatternFieldName, kKeyPattern);
    builder.append(ShardCollectionType::kUniqueFieldName, true);
    builder.append(ShardCollectionType::kLastRefreshedCollectionVersionFieldName, Timestamp());

    ShardCollectionType shardCollType = assertGet(ShardCollectionType::fromBSON(builder.obj()));

    ASSERT_EQ(epoch, shardCollType.getLastRefreshedCollectionVersion()->epoch());
}

TEST(ShardCollectionType, FromBSONEpochMatchesLastRefreshedCollectionVersionWhenDate) {
    OID epoch = OID::gen();

    BSONObjBuilder builder;
    builder.append(ShardCollectionType::kNssFieldName, kNss.ns());
    builder.append(ShardCollectionType::kEpochFieldName, epoch);
    builder.append(ShardCollectionType::kKeyPatternFieldName, kKeyPattern);
    builder.append(ShardCollectionType::kUniqueFieldName, true);
    builder.append(ShardCollectionType::kLastRefreshedCollectionVersionFieldName, Date_t());

    ShardCollectionType shardCollType = assertGet(ShardCollectionType::fromBSON(builder.obj()));

    ASSERT_EQ(epoch, shardCollType.getLastRefreshedCollectionVersion()->epoch());
}

TEST(ShardCollectionType, ToBSONEmptyDefaultCollationNotIncluded) {
    ShardCollectionType shardCollType;
    shardCollType.setNss(kNss);
    shardCollType.setEpoch(OID::gen());
    shardCollType.setKeyPattern(kKeyPattern);
    shardCollType.setUnique(true);

    shardCollType.setDefaultCollation(BSONObj());
    BSONObj obj = shardCollType.toBSON();

    ASSERT_FALSE(obj.hasField(ShardCollectionType::kDefaultCollationFieldName));

    shardCollType.setDefaultCollation(kDefaultCollation);
    obj = shardCollType.toBSON();

    ASSERT_TRUE(obj.hasField(ShardCollectionType::kDefaultCollationFieldName));
}

TEST(ShardCollectionType, ReshardingFieldsIncluded) {
    ShardCollectionType shardCollType;
    shardCollType.setNss(kNss);
    shardCollType.setEpoch(OID::gen());
    shardCollType.setKeyPattern(kKeyPattern);
    shardCollType.setUnique(true);

    TypeCollectionReshardingFields reshardingFields;
    const auto reshardingUuid = UUID::gen();
    reshardingFields.setUuid(reshardingUuid);
    shardCollType.setReshardingFields(std::move(reshardingFields));

    BSONObj obj = shardCollType.toBSON();
    ASSERT(obj.hasField(ShardCollectionType::kReshardingFieldsFieldName));

    auto shardCollTypeFromBSON = assertGet(ShardCollectionType::fromBSON(obj));
    ASSERT(shardCollType.getReshardingFields());
    ASSERT_EQ(reshardingUuid, shardCollType.getReshardingFields()->getUuid());
}

}  // namespace
}  // namespace mongo
