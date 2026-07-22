// generated from rosidl_generator_c/resource/idl__description.c.em
// with input from antidrone_turret:msg/ServoCommand.idl
// generated code does not contain a copyright notice

#include "antidrone_turret/msg/detail/servo_command__functions.h"

ROSIDL_GENERATOR_C_PUBLIC_antidrone_turret
const rosidl_type_hash_t *
antidrone_turret__msg__ServoCommand__get_type_hash(
  const rosidl_message_type_support_t * type_support)
{
  (void)type_support;
  static rosidl_type_hash_t hash = {1, {
      0x23, 0x31, 0xeb, 0x8a, 0x34, 0x8c, 0xd1, 0x6e,
      0xc2, 0xc1, 0x21, 0xf2, 0xe4, 0x35, 0xa2, 0xb4,
      0x03, 0x07, 0x70, 0x11, 0x2c, 0xfe, 0x6b, 0xb8,
      0x3b, 0x52, 0x80, 0x80, 0xe4, 0x3e, 0xc3, 0x6a,
    }};
  return &hash;
}

#include <assert.h>
#include <string.h>

// Include directives for referenced types

// Hashes for external referenced types
#ifndef NDEBUG
#endif

static char antidrone_turret__msg__ServoCommand__TYPE_NAME[] = "antidrone_turret/msg/ServoCommand";

// Define type names, field names, and default values
static char antidrone_turret__msg__ServoCommand__FIELD_NAME__direction[] = "direction";
static char antidrone_turret__msg__ServoCommand__FIELD_NAME__target_x[] = "target_x";
static char antidrone_turret__msg__ServoCommand__FIELD_NAME__error_x[] = "error_x";

static rosidl_runtime_c__type_description__Field antidrone_turret__msg__ServoCommand__FIELDS[] = {
  {
    {antidrone_turret__msg__ServoCommand__FIELD_NAME__direction, 9, 9},
    {
      rosidl_runtime_c__type_description__FieldType__FIELD_TYPE_INT8,
      0,
      0,
      {NULL, 0, 0},
    },
    {NULL, 0, 0},
  },
  {
    {antidrone_turret__msg__ServoCommand__FIELD_NAME__target_x, 8, 8},
    {
      rosidl_runtime_c__type_description__FieldType__FIELD_TYPE_FLOAT,
      0,
      0,
      {NULL, 0, 0},
    },
    {NULL, 0, 0},
  },
  {
    {antidrone_turret__msg__ServoCommand__FIELD_NAME__error_x, 7, 7},
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
antidrone_turret__msg__ServoCommand__get_type_description(
  const rosidl_message_type_support_t * type_support)
{
  (void)type_support;
  static bool constructed = false;
  static const rosidl_runtime_c__type_description__TypeDescription description = {
    {
      {antidrone_turret__msg__ServoCommand__TYPE_NAME, 33, 33},
      {antidrone_turret__msg__ServoCommand__FIELDS, 3, 3},
    },
    {NULL, 0, 0},
  };
  if (!constructed) {
    constructed = true;
  }
  return &description;
}

static char toplevel_type_raw_source[] =
  "int8 LEFT=-1\n"
  "int8 CENTER=0\n"
  "int8 RIGHT=1\n"
  "\n"
  "int8 direction\n"
  "float32 target_x\n"
  "float32 error_x";

static char msg_encoding[] = "msg";

// Define all individual source functions

const rosidl_runtime_c__type_description__TypeSource *
antidrone_turret__msg__ServoCommand__get_individual_type_description_source(
  const rosidl_message_type_support_t * type_support)
{
  (void)type_support;
  static const rosidl_runtime_c__type_description__TypeSource source = {
    {antidrone_turret__msg__ServoCommand__TYPE_NAME, 33, 33},
    {msg_encoding, 3, 3},
    {toplevel_type_raw_source, 88, 88},
  };
  return &source;
}

const rosidl_runtime_c__type_description__TypeSource__Sequence *
antidrone_turret__msg__ServoCommand__get_type_description_sources(
  const rosidl_message_type_support_t * type_support)
{
  (void)type_support;
  static rosidl_runtime_c__type_description__TypeSource sources[1];
  static const rosidl_runtime_c__type_description__TypeSource__Sequence source_sequence = {sources, 1, 1};
  static bool constructed = false;
  if (!constructed) {
    sources[0] = *antidrone_turret__msg__ServoCommand__get_individual_type_description_source(NULL),
    constructed = true;
  }
  return &source_sequence;
}
