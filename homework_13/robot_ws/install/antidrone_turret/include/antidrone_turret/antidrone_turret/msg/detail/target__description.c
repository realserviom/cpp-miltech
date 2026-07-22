// generated from rosidl_generator_c/resource/idl__description.c.em
// with input from antidrone_turret:msg/Target.idl
// generated code does not contain a copyright notice

#include "antidrone_turret/msg/detail/target__functions.h"

ROSIDL_GENERATOR_C_PUBLIC_antidrone_turret
const rosidl_type_hash_t *
antidrone_turret__msg__Target__get_type_hash(
  const rosidl_message_type_support_t * type_support)
{
  (void)type_support;
  static rosidl_type_hash_t hash = {1, {
      0xc4, 0x92, 0x55, 0x1c, 0xff, 0xd4, 0x4e, 0x59,
      0x69, 0x21, 0xdb, 0xcf, 0x2d, 0xea, 0x69, 0x5a,
      0x2e, 0xde, 0x02, 0x31, 0x75, 0xea, 0xb7, 0x35,
      0x3f, 0x32, 0x8b, 0xfd, 0x6d, 0xe9, 0x2d, 0x5d,
    }};
  return &hash;
}

#include <assert.h>
#include <string.h>

// Include directives for referenced types

// Hashes for external referenced types
#ifndef NDEBUG
#endif

static char antidrone_turret__msg__Target__TYPE_NAME[] = "antidrone_turret/msg/Target";

// Define type names, field names, and default values
static char antidrone_turret__msg__Target__FIELD_NAME__visible[] = "visible";
static char antidrone_turret__msg__Target__FIELD_NAME__x[] = "x";
static char antidrone_turret__msg__Target__FIELD_NAME__y[] = "y";
static char antidrone_turret__msg__Target__FIELD_NAME__distance_m[] = "distance_m";
static char antidrone_turret__msg__Target__FIELD_NAME__confidence[] = "confidence";

static rosidl_runtime_c__type_description__Field antidrone_turret__msg__Target__FIELDS[] = {
  {
    {antidrone_turret__msg__Target__FIELD_NAME__visible, 7, 7},
    {
      rosidl_runtime_c__type_description__FieldType__FIELD_TYPE_BOOLEAN,
      0,
      0,
      {NULL, 0, 0},
    },
    {NULL, 0, 0},
  },
  {
    {antidrone_turret__msg__Target__FIELD_NAME__x, 1, 1},
    {
      rosidl_runtime_c__type_description__FieldType__FIELD_TYPE_FLOAT,
      0,
      0,
      {NULL, 0, 0},
    },
    {NULL, 0, 0},
  },
  {
    {antidrone_turret__msg__Target__FIELD_NAME__y, 1, 1},
    {
      rosidl_runtime_c__type_description__FieldType__FIELD_TYPE_FLOAT,
      0,
      0,
      {NULL, 0, 0},
    },
    {NULL, 0, 0},
  },
  {
    {antidrone_turret__msg__Target__FIELD_NAME__distance_m, 10, 10},
    {
      rosidl_runtime_c__type_description__FieldType__FIELD_TYPE_FLOAT,
      0,
      0,
      {NULL, 0, 0},
    },
    {NULL, 0, 0},
  },
  {
    {antidrone_turret__msg__Target__FIELD_NAME__confidence, 10, 10},
    {
      rosidl_runtime_c__type_description__FieldType__FIELD_TYPE_FLOAT,
      0,
      0,
      {NULL, 0, 0},
    },
    {NULL, 0, 0},
  },
};

const rosidl_runtime_c__type_description__TypeDescription *
antidrone_turret__msg__Target__get_type_description(
  const rosidl_message_type_support_t * type_support)
{
  (void)type_support;
  static bool constructed = false;
  static const rosidl_runtime_c__type_description__TypeDescription description = {
    {
      {antidrone_turret__msg__Target__TYPE_NAME, 27, 27},
      {antidrone_turret__msg__Target__FIELDS, 5, 5},
    },
    {NULL, 0, 0},
  };
  if (!constructed) {
    constructed = true;
  }
  return &description;
}

static char toplevel_type_raw_source[] =
  "bool visible\n"
  "float32 x\n"
  "float32 y\n"
  "float32 distance_m\n"
  "float32 confidence";

static char msg_encoding[] = "msg";

// Define all individual source functions

const rosidl_runtime_c__type_description__TypeSource *
antidrone_turret__msg__Target__get_individual_type_description_source(
  const rosidl_message_type_support_t * type_support)
{
  (void)type_support;
  static const rosidl_runtime_c__type_description__TypeSource source = {
    {antidrone_turret__msg__Target__TYPE_NAME, 27, 27},
    {msg_encoding, 3, 3},
    {toplevel_type_raw_source, 71, 71},
  };
  return &source;
}

const rosidl_runtime_c__type_description__TypeSource__Sequence *
antidrone_turret__msg__Target__get_type_description_sources(
  const rosidl_message_type_support_t * type_support)
{
  (void)type_support;
  static rosidl_runtime_c__type_description__TypeSource sources[1];
  static const rosidl_runtime_c__type_description__TypeSource__Sequence source_sequence = {sources, 1, 1};
  static bool constructed = false;
  if (!constructed) {
    sources[0] = *antidrone_turret__msg__Target__get_individual_type_description_source(NULL),
    constructed = true;
  }
  return &source_sequence;
}
