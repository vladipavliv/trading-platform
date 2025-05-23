// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: metadata_messages.proto

#include "metadata_messages.pb.h"

#include <algorithm>

#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/extension_set.h>
#include <google/protobuf/wire_format_lite.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/generated_message_reflection.h>
#include <google/protobuf/reflection_ops.h>
#include <google/protobuf/wire_format.h>
// @@protoc_insertion_point(includes)
#include <google/protobuf/port_def.inc>

PROTOBUF_PRAGMA_INIT_SEG

namespace _pb = ::PROTOBUF_NAMESPACE_ID;
namespace _pbi = _pb::internal;

namespace hft {
namespace serialization {
namespace gen {
namespace proto {
namespace metadata {
PROTOBUF_CONSTEXPR OrderTimestamp::OrderTimestamp(
    ::_pbi::ConstantInitialized): _impl_{
    /*decltype(_impl_.order_id_)*/uint64_t{0u}
  , /*decltype(_impl_.created_)*/uint64_t{0u}
  , /*decltype(_impl_.fulfilled_)*/uint64_t{0u}
  , /*decltype(_impl_.notified_)*/uint64_t{0u}
  , /*decltype(_impl_._cached_size_)*/{}} {}
struct OrderTimestampDefaultTypeInternal {
  PROTOBUF_CONSTEXPR OrderTimestampDefaultTypeInternal()
      : _instance(::_pbi::ConstantInitialized{}) {}
  ~OrderTimestampDefaultTypeInternal() {}
  union {
    OrderTimestamp _instance;
  };
};
PROTOBUF_ATTRIBUTE_NO_DESTROY PROTOBUF_CONSTINIT PROTOBUF_ATTRIBUTE_INIT_PRIORITY1 OrderTimestampDefaultTypeInternal _OrderTimestamp_default_instance_;
}  // namespace metadata
}  // namespace proto
}  // namespace gen
}  // namespace serialization
}  // namespace hft
static ::_pb::Metadata file_level_metadata_metadata_5fmessages_2eproto[1];
static constexpr ::_pb::EnumDescriptor const** file_level_enum_descriptors_metadata_5fmessages_2eproto = nullptr;
static constexpr ::_pb::ServiceDescriptor const** file_level_service_descriptors_metadata_5fmessages_2eproto = nullptr;

const uint32_t TableStruct_metadata_5fmessages_2eproto::offsets[] PROTOBUF_SECTION_VARIABLE(protodesc_cold) = {
  ~0u,  // no _has_bits_
  PROTOBUF_FIELD_OFFSET(::hft::serialization::gen::proto::metadata::OrderTimestamp, _internal_metadata_),
  ~0u,  // no _extensions_
  ~0u,  // no _oneof_case_
  ~0u,  // no _weak_field_map_
  ~0u,  // no _inlined_string_donated_
  PROTOBUF_FIELD_OFFSET(::hft::serialization::gen::proto::metadata::OrderTimestamp, _impl_.order_id_),
  PROTOBUF_FIELD_OFFSET(::hft::serialization::gen::proto::metadata::OrderTimestamp, _impl_.created_),
  PROTOBUF_FIELD_OFFSET(::hft::serialization::gen::proto::metadata::OrderTimestamp, _impl_.fulfilled_),
  PROTOBUF_FIELD_OFFSET(::hft::serialization::gen::proto::metadata::OrderTimestamp, _impl_.notified_),
};
static const ::_pbi::MigrationSchema schemas[] PROTOBUF_SECTION_VARIABLE(protodesc_cold) = {
  { 0, -1, -1, sizeof(::hft::serialization::gen::proto::metadata::OrderTimestamp)},
};

static const ::_pb::Message* const file_default_instances[] = {
  &::hft::serialization::gen::proto::metadata::_OrderTimestamp_default_instance_._instance,
};

const char descriptor_table_protodef_metadata_5fmessages_2eproto[] PROTOBUF_SECTION_VARIABLE(protodesc_cold) =
  "\n\027metadata_messages.proto\022$hft.serializa"
  "tion.gen.proto.metadata\"X\n\016OrderTimestam"
  "p\022\020\n\010order_id\030\001 \001(\004\022\017\n\007created\030\002 \001(\004\022\021\n\t"
  "fulfilled\030\003 \001(\004\022\020\n\010notified\030\004 \001(\004b\006proto"
  "3"
  ;
static ::_pbi::once_flag descriptor_table_metadata_5fmessages_2eproto_once;
const ::_pbi::DescriptorTable descriptor_table_metadata_5fmessages_2eproto = {
    false, false, 161, descriptor_table_protodef_metadata_5fmessages_2eproto,
    "metadata_messages.proto",
    &descriptor_table_metadata_5fmessages_2eproto_once, nullptr, 0, 1,
    schemas, file_default_instances, TableStruct_metadata_5fmessages_2eproto::offsets,
    file_level_metadata_metadata_5fmessages_2eproto, file_level_enum_descriptors_metadata_5fmessages_2eproto,
    file_level_service_descriptors_metadata_5fmessages_2eproto,
};
PROTOBUF_ATTRIBUTE_WEAK const ::_pbi::DescriptorTable* descriptor_table_metadata_5fmessages_2eproto_getter() {
  return &descriptor_table_metadata_5fmessages_2eproto;
}

// Force running AddDescriptors() at dynamic initialization time.
PROTOBUF_ATTRIBUTE_INIT_PRIORITY2 static ::_pbi::AddDescriptorsRunner dynamic_init_dummy_metadata_5fmessages_2eproto(&descriptor_table_metadata_5fmessages_2eproto);
namespace hft {
namespace serialization {
namespace gen {
namespace proto {
namespace metadata {

// ===================================================================

class OrderTimestamp::_Internal {
 public:
};

OrderTimestamp::OrderTimestamp(::PROTOBUF_NAMESPACE_ID::Arena* arena,
                         bool is_message_owned)
  : ::PROTOBUF_NAMESPACE_ID::Message(arena, is_message_owned) {
  SharedCtor(arena, is_message_owned);
  // @@protoc_insertion_point(arena_constructor:hft.serialization.gen.proto.metadata.OrderTimestamp)
}
OrderTimestamp::OrderTimestamp(const OrderTimestamp& from)
  : ::PROTOBUF_NAMESPACE_ID::Message() {
  OrderTimestamp* const _this = this; (void)_this;
  new (&_impl_) Impl_{
      decltype(_impl_.order_id_){}
    , decltype(_impl_.created_){}
    , decltype(_impl_.fulfilled_){}
    , decltype(_impl_.notified_){}
    , /*decltype(_impl_._cached_size_)*/{}};

  _internal_metadata_.MergeFrom<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(from._internal_metadata_);
  ::memcpy(&_impl_.order_id_, &from._impl_.order_id_,
    static_cast<size_t>(reinterpret_cast<char*>(&_impl_.notified_) -
    reinterpret_cast<char*>(&_impl_.order_id_)) + sizeof(_impl_.notified_));
  // @@protoc_insertion_point(copy_constructor:hft.serialization.gen.proto.metadata.OrderTimestamp)
}

inline void OrderTimestamp::SharedCtor(
    ::_pb::Arena* arena, bool is_message_owned) {
  (void)arena;
  (void)is_message_owned;
  new (&_impl_) Impl_{
      decltype(_impl_.order_id_){uint64_t{0u}}
    , decltype(_impl_.created_){uint64_t{0u}}
    , decltype(_impl_.fulfilled_){uint64_t{0u}}
    , decltype(_impl_.notified_){uint64_t{0u}}
    , /*decltype(_impl_._cached_size_)*/{}
  };
}

OrderTimestamp::~OrderTimestamp() {
  // @@protoc_insertion_point(destructor:hft.serialization.gen.proto.metadata.OrderTimestamp)
  if (auto *arena = _internal_metadata_.DeleteReturnArena<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>()) {
  (void)arena;
    return;
  }
  SharedDtor();
}

inline void OrderTimestamp::SharedDtor() {
  GOOGLE_DCHECK(GetArenaForAllocation() == nullptr);
}

void OrderTimestamp::SetCachedSize(int size) const {
  _impl_._cached_size_.Set(size);
}

void OrderTimestamp::Clear() {
// @@protoc_insertion_point(message_clear_start:hft.serialization.gen.proto.metadata.OrderTimestamp)
  uint32_t cached_has_bits = 0;
  // Prevent compiler warnings about cached_has_bits being unused
  (void) cached_has_bits;

  ::memset(&_impl_.order_id_, 0, static_cast<size_t>(
      reinterpret_cast<char*>(&_impl_.notified_) -
      reinterpret_cast<char*>(&_impl_.order_id_)) + sizeof(_impl_.notified_));
  _internal_metadata_.Clear<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>();
}

const char* OrderTimestamp::_InternalParse(const char* ptr, ::_pbi::ParseContext* ctx) {
#define CHK_(x) if (PROTOBUF_PREDICT_FALSE(!(x))) goto failure
  while (!ctx->Done(&ptr)) {
    uint32_t tag;
    ptr = ::_pbi::ReadTag(ptr, &tag);
    switch (tag >> 3) {
      // uint64 order_id = 1;
      case 1:
        if (PROTOBUF_PREDICT_TRUE(static_cast<uint8_t>(tag) == 8)) {
          _impl_.order_id_ = ::PROTOBUF_NAMESPACE_ID::internal::ReadVarint64(&ptr);
          CHK_(ptr);
        } else
          goto handle_unusual;
        continue;
      // uint64 created = 2;
      case 2:
        if (PROTOBUF_PREDICT_TRUE(static_cast<uint8_t>(tag) == 16)) {
          _impl_.created_ = ::PROTOBUF_NAMESPACE_ID::internal::ReadVarint64(&ptr);
          CHK_(ptr);
        } else
          goto handle_unusual;
        continue;
      // uint64 fulfilled = 3;
      case 3:
        if (PROTOBUF_PREDICT_TRUE(static_cast<uint8_t>(tag) == 24)) {
          _impl_.fulfilled_ = ::PROTOBUF_NAMESPACE_ID::internal::ReadVarint64(&ptr);
          CHK_(ptr);
        } else
          goto handle_unusual;
        continue;
      // uint64 notified = 4;
      case 4:
        if (PROTOBUF_PREDICT_TRUE(static_cast<uint8_t>(tag) == 32)) {
          _impl_.notified_ = ::PROTOBUF_NAMESPACE_ID::internal::ReadVarint64(&ptr);
          CHK_(ptr);
        } else
          goto handle_unusual;
        continue;
      default:
        goto handle_unusual;
    }  // switch
  handle_unusual:
    if ((tag == 0) || ((tag & 7) == 4)) {
      CHK_(ptr);
      ctx->SetLastTag(tag);
      goto message_done;
    }
    ptr = UnknownFieldParse(
        tag,
        _internal_metadata_.mutable_unknown_fields<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(),
        ptr, ctx);
    CHK_(ptr != nullptr);
  }  // while
message_done:
  return ptr;
failure:
  ptr = nullptr;
  goto message_done;
#undef CHK_
}

uint8_t* OrderTimestamp::_InternalSerialize(
    uint8_t* target, ::PROTOBUF_NAMESPACE_ID::io::EpsCopyOutputStream* stream) const {
  // @@protoc_insertion_point(serialize_to_array_start:hft.serialization.gen.proto.metadata.OrderTimestamp)
  uint32_t cached_has_bits = 0;
  (void) cached_has_bits;

  // uint64 order_id = 1;
  if (this->_internal_order_id() != 0) {
    target = stream->EnsureSpace(target);
    target = ::_pbi::WireFormatLite::WriteUInt64ToArray(1, this->_internal_order_id(), target);
  }

  // uint64 created = 2;
  if (this->_internal_created() != 0) {
    target = stream->EnsureSpace(target);
    target = ::_pbi::WireFormatLite::WriteUInt64ToArray(2, this->_internal_created(), target);
  }

  // uint64 fulfilled = 3;
  if (this->_internal_fulfilled() != 0) {
    target = stream->EnsureSpace(target);
    target = ::_pbi::WireFormatLite::WriteUInt64ToArray(3, this->_internal_fulfilled(), target);
  }

  // uint64 notified = 4;
  if (this->_internal_notified() != 0) {
    target = stream->EnsureSpace(target);
    target = ::_pbi::WireFormatLite::WriteUInt64ToArray(4, this->_internal_notified(), target);
  }

  if (PROTOBUF_PREDICT_FALSE(_internal_metadata_.have_unknown_fields())) {
    target = ::_pbi::WireFormat::InternalSerializeUnknownFieldsToArray(
        _internal_metadata_.unknown_fields<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(::PROTOBUF_NAMESPACE_ID::UnknownFieldSet::default_instance), target, stream);
  }
  // @@protoc_insertion_point(serialize_to_array_end:hft.serialization.gen.proto.metadata.OrderTimestamp)
  return target;
}

size_t OrderTimestamp::ByteSizeLong() const {
// @@protoc_insertion_point(message_byte_size_start:hft.serialization.gen.proto.metadata.OrderTimestamp)
  size_t total_size = 0;

  uint32_t cached_has_bits = 0;
  // Prevent compiler warnings about cached_has_bits being unused
  (void) cached_has_bits;

  // uint64 order_id = 1;
  if (this->_internal_order_id() != 0) {
    total_size += ::_pbi::WireFormatLite::UInt64SizePlusOne(this->_internal_order_id());
  }

  // uint64 created = 2;
  if (this->_internal_created() != 0) {
    total_size += ::_pbi::WireFormatLite::UInt64SizePlusOne(this->_internal_created());
  }

  // uint64 fulfilled = 3;
  if (this->_internal_fulfilled() != 0) {
    total_size += ::_pbi::WireFormatLite::UInt64SizePlusOne(this->_internal_fulfilled());
  }

  // uint64 notified = 4;
  if (this->_internal_notified() != 0) {
    total_size += ::_pbi::WireFormatLite::UInt64SizePlusOne(this->_internal_notified());
  }

  return MaybeComputeUnknownFieldsSize(total_size, &_impl_._cached_size_);
}

const ::PROTOBUF_NAMESPACE_ID::Message::ClassData OrderTimestamp::_class_data_ = {
    ::PROTOBUF_NAMESPACE_ID::Message::CopyWithSourceCheck,
    OrderTimestamp::MergeImpl
};
const ::PROTOBUF_NAMESPACE_ID::Message::ClassData*OrderTimestamp::GetClassData() const { return &_class_data_; }


void OrderTimestamp::MergeImpl(::PROTOBUF_NAMESPACE_ID::Message& to_msg, const ::PROTOBUF_NAMESPACE_ID::Message& from_msg) {
  auto* const _this = static_cast<OrderTimestamp*>(&to_msg);
  auto& from = static_cast<const OrderTimestamp&>(from_msg);
  // @@protoc_insertion_point(class_specific_merge_from_start:hft.serialization.gen.proto.metadata.OrderTimestamp)
  GOOGLE_DCHECK_NE(&from, _this);
  uint32_t cached_has_bits = 0;
  (void) cached_has_bits;

  if (from._internal_order_id() != 0) {
    _this->_internal_set_order_id(from._internal_order_id());
  }
  if (from._internal_created() != 0) {
    _this->_internal_set_created(from._internal_created());
  }
  if (from._internal_fulfilled() != 0) {
    _this->_internal_set_fulfilled(from._internal_fulfilled());
  }
  if (from._internal_notified() != 0) {
    _this->_internal_set_notified(from._internal_notified());
  }
  _this->_internal_metadata_.MergeFrom<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(from._internal_metadata_);
}

void OrderTimestamp::CopyFrom(const OrderTimestamp& from) {
// @@protoc_insertion_point(class_specific_copy_from_start:hft.serialization.gen.proto.metadata.OrderTimestamp)
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

bool OrderTimestamp::IsInitialized() const {
  return true;
}

void OrderTimestamp::InternalSwap(OrderTimestamp* other) {
  using std::swap;
  _internal_metadata_.InternalSwap(&other->_internal_metadata_);
  ::PROTOBUF_NAMESPACE_ID::internal::memswap<
      PROTOBUF_FIELD_OFFSET(OrderTimestamp, _impl_.notified_)
      + sizeof(OrderTimestamp::_impl_.notified_)
      - PROTOBUF_FIELD_OFFSET(OrderTimestamp, _impl_.order_id_)>(
          reinterpret_cast<char*>(&_impl_.order_id_),
          reinterpret_cast<char*>(&other->_impl_.order_id_));
}

::PROTOBUF_NAMESPACE_ID::Metadata OrderTimestamp::GetMetadata() const {
  return ::_pbi::AssignDescriptors(
      &descriptor_table_metadata_5fmessages_2eproto_getter, &descriptor_table_metadata_5fmessages_2eproto_once,
      file_level_metadata_metadata_5fmessages_2eproto[0]);
}

// @@protoc_insertion_point(namespace_scope)
}  // namespace metadata
}  // namespace proto
}  // namespace gen
}  // namespace serialization
}  // namespace hft
PROTOBUF_NAMESPACE_OPEN
template<> PROTOBUF_NOINLINE ::hft::serialization::gen::proto::metadata::OrderTimestamp*
Arena::CreateMaybeMessage< ::hft::serialization::gen::proto::metadata::OrderTimestamp >(Arena* arena) {
  return Arena::CreateMessageInternal< ::hft::serialization::gen::proto::metadata::OrderTimestamp >(arena);
}
PROTOBUF_NAMESPACE_CLOSE

// @@protoc_insertion_point(global_scope)
#include <google/protobuf/port_undef.inc>
