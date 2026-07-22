// generated from rosidl_generator_c/resource/idl__description.c.em
// with input from antidrone_turret:msg/GimbalCommand.idl
// generated code does not contain a copyright notice

#include "antidrone_turret/msg/detail/gimbal_command__functions.h"

ROSIDL_GENERATOR_C_PUBLIC_antidrone_turret
const rosidl_type_hash_t *
antidrone_turret__msg__GimbalCommand__get_type_hash(
  const rosidl_message_type_support_t * type_support)
{
  (void)type_support;
  static rosidl_type_hash_t hash = {1, {
      0xb9, 0x80, 0xe8, 0xce, 0xdd, 0xd2, 0x11, 0x28,
      0x61, 0xf6, 0x62, 0xa5, 0x1b, 0x4f, 0xff, 0xce,
      0xfb, 0x97, 0xde, 0x1f, 0x3c, 0xa5, 0xa8, 0x45,
      0xf3, 0xc0, 0xad, 0xae, 0x5c, 0x4a, 0xd1, 0x27,
    }};
  return &hash;
}

#include <assert.h>
#include <string.h>

// Include directives for referenced types

// Hashes for external referenced types
#ifndef NDEBUG
#endif

static char antidrone_turret__msg__GimbalCommand__TYPE_NAME[] = "antidrone_turret/msg/GimbalCommand";

// Define type names, field names, and default values
static char antidrone_turret__msg__GimbalCommand__FIELD_NAME__direction[] = "direction";
static char antidrone_turret__msg__GimbalCommand__FIELD_NAME__target_y[] = "target_y";
static char antidrone_turret__msg__GimbalCommand__FIELD_NAME__error_y[] = "error_y";

static rosidl_runtime_c__type_description__Field antidrone_turret__msg__GimbalCommand__FIELDS[] = {
  {
    {antidrone_turret__msg__GimbalCommand__FIELD_NAME__direction, 9, 9},
    {
      rosidl_runtime_c__type_description__FieldType__FIELD_TYPE_INT8,
      0,
      0,
      {NULL, 0, 0},
    },
    {NULL, 0, 0},
  },
  {
    {antidrone_turret__msg__GimbalCommand__FIELD_NAME__target_y, 8, 8},
    {
      rosidl_runtime_c__type_description__FieldType__FIELD_TYPE_FLOAT,
      0,
      0,
      {NULL, 0, 0},
    },
    {NULL, 0, 0},
  },
  {
    {antidrone_turret__msg__GimbalCommand__FIELD_NAME__error_y, 7, 7},
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
antidrone_turret__msg__GimbalCommand__get_type_description(
  const rosidl_message_type_support_t * type_support)
{
  (void)type_support;
  static bool constructed = false;
  static const rosidl_runtime_c__type_description__TypeDescription description = {
    {
      {antidrone_turret__msg__GimbalCommand__TYPE_NAME, 34, 34},
      {antidrone_turret__msg__GimbalCommand__FIELDS, 3, 3},
    },
    {NULL, 0, 0},
  };
  if (!constructed) {
    constructed = true;
  }
  return &description;
}

static char toplevel_type_raw_source[] =
  "# Constants\n"
  "int8 DOWN=-1\n"
  "int8 CENTER=0\n"
  "int8 UP=1\n"
  "\n"
  "# Fields\n"
  "int8 direction\n"
  "float32 target_y\n"
  "float32 error_y";

static char msg_encoding[] = "msg";

// Define all individual source functions

const rosidl_runtime_c__type_description__TypeSource *
antidrone_turret__msg__GimbalCommand__get_individual_type_description_source(
  const rosidl_message_type_support_t * type_support)
{
  (void)type_support;
  static const rosidl_runtime_c__type_description__TypeSource source = {
    {antidrone_turret__msg__GimbalCommand__TYPE_NAME, 34, 34},
    {msg_encoding, 3, 3},
    {toplevel_type_raw_source, 107, 107},
  };
  return &source;
}

const rosidl_runtime_c__type_description__TypeSource__Sequence *
antidrone_turret__msg__GimbalCommand__get_type_description_sources(
  const rosidl_message_type_support_t * type_support)
{
  (void)type_support;
  static rosidl_runtime_c__type_description__TypeSource sources[1];
  static const rosidl_runtime_c__type_description__TypeSource__Sequence source_sequence = {sources, 1, 1};
  static bool constructed = false;
  if (!constructed) {
    sources[0] = *antidrone_turret__msg__GimbalCommand__get_individual_type_description_source(NULL),
    constructed = true;
  }
  return &source_sequence;
}
