// generated from rosidl_generator_c/resource/idl__description.c.em
// with input from antidrone_turret:msg/ActuatorStatus.idl
// generated code does not contain a copyright notice

#include "antidrone_turret/msg/detail/actuator_status__functions.h"

ROSIDL_GENERATOR_C_PUBLIC_antidrone_turret
const rosidl_type_hash_t *
antidrone_turret__msg__ActuatorStatus__get_type_hash(
  const rosidl_message_type_support_t * type_support)
{
  (void)type_support;
  static rosidl_type_hash_t hash = {1, {
      0x5a, 0x7d, 0xcc, 0x31, 0x36, 0x11, 0xcf, 0x35,
      0x04, 0x56, 0x11, 0xec, 0xcd, 0x45, 0x7b, 0x83,
      0xc2, 0xe6, 0x5d, 0x3c, 0x3f, 0x94, 0x6e, 0xf9,
      0x6a, 0xc2, 0x8e, 0x1e, 0x43, 0x30, 0xaa, 0x8f,
    }};
  return &hash;
}

#include <assert.h>
#include <string.h>

// Include directives for referenced types

// Hashes for external referenced types
#ifndef NDEBUG
#endif

static char antidrone_turret__msg__ActuatorStatus__TYPE_NAME[] = "antidrone_turret/msg/ActuatorStatus";

// Define type names, field names, and default values
static char antidrone_turret__msg__ActuatorStatus__FIELD_NAME__state[] = "state";
static char antidrone_turret__msg__ActuatorStatus__FIELD_NAME__trigger_count[] = "trigger_count";

static rosidl_runtime_c__type_description__Field antidrone_turret__msg__ActuatorStatus__FIELDS[] = {
  {
    {antidrone_turret__msg__ActuatorStatus__FIELD_NAME__state, 5, 5},
    {
      rosidl_runtime_c__type_description__FieldType__FIELD_TYPE_UINT8,
      0,
      0,
      {NULL, 0, 0},
    },
    {NULL, 0, 0},
  },
  {
    {antidrone_turret__msg__ActuatorStatus__FIELD_NAME__trigger_count, 13, 13},
    {
      rosidl_runtime_c__type_description__FieldType__FIELD_TYPE_UINT32,
      0,
      0,
      {NULL, 0, 0},
    },
    {NULL, 0, 0},
  },
};

const rosidl_runtime_c__type_description__TypeDescription *
antidrone_turret__msg__ActuatorStatus__get_type_description(
  const rosidl_message_type_support_t * type_support)
{
  (void)type_support;
  static bool constructed = false;
  static const rosidl_runtime_c__type_description__TypeDescription description = {
    {
      {antidrone_turret__msg__ActuatorStatus__TYPE_NAME, 35, 35},
      {antidrone_turret__msg__ActuatorStatus__FIELDS, 2, 2},
    },
    {NULL, 0, 0},
  };
  if (!constructed) {
    constructed = true;
  }
  return &description;
}

static char toplevel_type_raw_source[] =
  "uint8 READY=0\n"
  "uint8 RELOADING=1\n"
  "\n"
  "uint8 state\n"
  "uint32 trigger_count";

static char msg_encoding[] = "msg";

// Define all individual source functions

const rosidl_runtime_c__type_description__TypeSource *
antidrone_turret__msg__ActuatorStatus__get_individual_type_description_source(
  const rosidl_message_type_support_t * type_support)
{
  (void)type_support;
  static const rosidl_runtime_c__type_description__TypeSource source = {
    {antidrone_turret__msg__ActuatorStatus__TYPE_NAME, 35, 35},
    {msg_encoding, 3, 3},
    {toplevel_type_raw_source, 66, 66},
  };
  return &source;
}

const rosidl_runtime_c__type_description__TypeSource__Sequence *
antidrone_turret__msg__ActuatorStatus__get_type_description_sources(
  const rosidl_message_type_support_t * type_support)
{
  (void)type_support;
  static rosidl_runtime_c__type_description__TypeSource sources[1];
  static const rosidl_runtime_c__type_description__TypeSource__Sequence source_sequence = {sources, 1, 1};
  static bool constructed = false;
  if (!constructed) {
    sources[0] = *antidrone_turret__msg__ActuatorStatus__get_individual_type_description_source(NULL),
    constructed = true;
  }
  return &source_sequence;
}
