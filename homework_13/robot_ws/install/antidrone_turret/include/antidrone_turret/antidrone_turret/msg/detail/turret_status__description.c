// generated from rosidl_generator_c/resource/idl__description.c.em
// with input from antidrone_turret:msg/TurretStatus.idl
// generated code does not contain a copyright notice

#include "antidrone_turret/msg/detail/turret_status__functions.h"

ROSIDL_GENERATOR_C_PUBLIC_antidrone_turret
const rosidl_type_hash_t *
antidrone_turret__msg__TurretStatus__get_type_hash(
  const rosidl_message_type_support_t * type_support)
{
  (void)type_support;
  static rosidl_type_hash_t hash = {1, {
      0xfe, 0xa5, 0x30, 0x73, 0xa2, 0x05, 0xd5, 0xed,
      0x95, 0xf3, 0xe5, 0xd4, 0xe9, 0xad, 0x41, 0x87,
      0x6b, 0xb5, 0x4c, 0x8e, 0xe3, 0x5a, 0x82, 0xb0,
      0x30, 0xaa, 0x70, 0x1b, 0x52, 0x32, 0x8b, 0xbd,
    }};
  return &hash;
}

#include <assert.h>
#include <string.h>

// Include directives for referenced types

// Hashes for external referenced types
#ifndef NDEBUG
#endif

static char antidrone_turret__msg__TurretStatus__TYPE_NAME[] = "antidrone_turret/msg/TurretStatus";

// Define type names, field names, and default values
static char antidrone_turret__msg__TurretStatus__FIELD_NAME__target_state[] = "target_state";
static char antidrone_turret__msg__TurretStatus__FIELD_NAME__action[] = "action";
static char antidrone_turret__msg__TurretStatus__FIELD_NAME__trigger_state[] = "trigger_state";
static char antidrone_turret__msg__TurretStatus__FIELD_NAME__confidence[] = "confidence";
static char antidrone_turret__msg__TurretStatus__FIELD_NAME__distance_m[] = "distance_m";

static rosidl_runtime_c__type_description__Field antidrone_turret__msg__TurretStatus__FIELDS[] = {
  {
    {antidrone_turret__msg__TurretStatus__FIELD_NAME__target_state, 12, 12},
    {
      rosidl_runtime_c__type_description__FieldType__FIELD_TYPE_UINT8,
      0,
      0,
      {NULL, 0, 0},
    },
    {NULL, 0, 0},
  },
  {
    {antidrone_turret__msg__TurretStatus__FIELD_NAME__action, 6, 6},
    {
      rosidl_runtime_c__type_description__FieldType__FIELD_TYPE_UINT8,
      0,
      0,
      {NULL, 0, 0},
    },
    {NULL, 0, 0},
  },
  {
    {antidrone_turret__msg__TurretStatus__FIELD_NAME__trigger_state, 13, 13},
    {
      rosidl_runtime_c__type_description__FieldType__FIELD_TYPE_UINT8,
      0,
      0,
      {NULL, 0, 0},
    },
    {NULL, 0, 0},
  },
  {
    {antidrone_turret__msg__TurretStatus__FIELD_NAME__confidence, 10, 10},
    {
      rosidl_runtime_c__type_description__FieldType__FIELD_TYPE_FLOAT,
      0,
      0,
      {NULL, 0, 0},
    },
    {NULL, 0, 0},
  },
  {
    {antidrone_turret__msg__TurretStatus__FIELD_NAME__distance_m, 10, 10},
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
antidrone_turret__msg__TurretStatus__get_type_description(
  const rosidl_message_type_support_t * type_support)
{
  (void)type_support;
  static bool constructed = false;
  static const rosidl_runtime_c__type_description__TypeDescription description = {
    {
      {antidrone_turret__msg__TurretStatus__TYPE_NAME, 33, 33},
      {antidrone_turret__msg__TurretStatus__FIELDS, 5, 5},
    },
    {NULL, 0, 0},
  };
  if (!constructed) {
    constructed = true;
  }
  return &description;
}

static char toplevel_type_raw_source[] =
  "# Target states\n"
  "uint8 TARGET_NONE=0\n"
  "uint8 TARGET_LOW_CONFIDENCE=1\n"
  "uint8 TARGET_LOCKED=2\n"
  "\n"
  "# Action states\n"
  "uint8 ACTION_IDLE=0\n"
  "uint8 ACTION_TRACK=1\n"
  "\n"
  "# Trigger states\n"
  "uint8 TRIGGER_SKIP=0\n"
  "uint8 TRIGGER_REQUESTED=1\n"
  "uint8 TRIGGER_RELOADING=2\n"
  "\n"
  "# Fields\n"
  "uint8 target_state\n"
  "uint8 action\n"
  "uint8 trigger_state\n"
  "float32 confidence\n"
  "float32 distance_m";

static char msg_encoding[] = "msg";

// Define all individual source functions

const rosidl_runtime_c__type_description__TypeSource *
antidrone_turret__msg__TurretStatus__get_individual_type_description_source(
  const rosidl_message_type_support_t * type_support)
{
  (void)type_support;
  static const rosidl_runtime_c__type_description__TypeSource source = {
    {antidrone_turret__msg__TurretStatus__TYPE_NAME, 33, 33},
    {msg_encoding, 3, 3},
    {toplevel_type_raw_source, 336, 336},
  };
  return &source;
}

const rosidl_runtime_c__type_description__TypeSource__Sequence *
antidrone_turret__msg__TurretStatus__get_type_description_sources(
  const rosidl_message_type_support_t * type_support)
{
  (void)type_support;
  static rosidl_runtime_c__type_description__TypeSource sources[1];
  static const rosidl_runtime_c__type_description__TypeSource__Sequence source_sequence = {sources, 1, 1};
  static bool constructed = false;
  if (!constructed) {
    sources[0] = *antidrone_turret__msg__TurretStatus__get_individual_type_description_source(NULL),
    constructed = true;
  }
  return &source_sequence;
}
