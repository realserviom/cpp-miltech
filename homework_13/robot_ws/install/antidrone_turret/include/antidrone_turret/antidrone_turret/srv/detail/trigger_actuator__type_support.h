// generated from rosidl_generator_c/resource/idl__type_support.h.em
// with input from antidrone_turret:srv/TriggerActuator.idl
// generated code does not contain a copyright notice

// IWYU pragma: private, include "antidrone_turret/srv/trigger_actuator.h"


#ifndef ANTIDRONE_TURRET__SRV__DETAIL__TRIGGER_ACTUATOR__TYPE_SUPPORT_H_
#define ANTIDRONE_TURRET__SRV__DETAIL__TRIGGER_ACTUATOR__TYPE_SUPPORT_H_

#include "rosidl_typesupport_interface/macros.h"

#include "antidrone_turret/msg/rosidl_generator_c__visibility_control.h"

#ifdef __cplusplus
extern "C"
{
#endif

#include "rosidl_runtime_c/message_type_support_struct.h"

// Forward declare the get type support functions for this type.
ROSIDL_GENERATOR_C_PUBLIC_antidrone_turret
const rosidl_message_type_support_t *
ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(
  rosidl_typesupport_c,
  antidrone_turret,
  srv,
  TriggerActuator_Request
)(void);

// already included above
// #include "rosidl_runtime_c/message_type_support_struct.h"

// Forward declare the get type support functions for this type.
ROSIDL_GENERATOR_C_PUBLIC_antidrone_turret
const rosidl_message_type_support_t *
ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(
  rosidl_typesupport_c,
  antidrone_turret,
  srv,
  TriggerActuator_Response
)(void);

// already included above
// #include "rosidl_runtime_c/message_type_support_struct.h"

// Forward declare the get type support functions for this type.
ROSIDL_GENERATOR_C_PUBLIC_antidrone_turret
const rosidl_message_type_support_t *
ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(
  rosidl_typesupport_c,
  antidrone_turret,
  srv,
  TriggerActuator_Event
)(void);

#include "rosidl_runtime_c/service_type_support_struct.h"

// Forward declare the get type support functions for this type.
ROSIDL_GENERATOR_C_PUBLIC_antidrone_turret
const rosidl_service_type_support_t *
ROSIDL_TYPESUPPORT_INTERFACE__SERVICE_SYMBOL_NAME(
  rosidl_typesupport_c,
  antidrone_turret,
  srv,
  TriggerActuator
)(void);

// Forward declare the function to create a service event message for this type.
ROSIDL_GENERATOR_C_PUBLIC_antidrone_turret
void *
ROSIDL_TYPESUPPORT_INTERFACE__SERVICE_CREATE_EVENT_MESSAGE_SYMBOL_NAME(
  rosidl_typesupport_c,
  antidrone_turret,
  srv,
  TriggerActuator
)(
  const rosidl_service_introspection_info_t * info,
  rcutils_allocator_t * allocator,
  const void * request_message,
  const void * response_message);

// Forward declare the function to destroy a service event message for this type.
ROSIDL_GENERATOR_C_PUBLIC_antidrone_turret
bool
ROSIDL_TYPESUPPORT_INTERFACE__SERVICE_DESTROY_EVENT_MESSAGE_SYMBOL_NAME(
  rosidl_typesupport_c,
  antidrone_turret,
  srv,
  TriggerActuator
)(
  void * event_msg,
  rcutils_allocator_t * allocator);

#ifdef __cplusplus
}
#endif

#endif  // ANTIDRONE_TURRET__SRV__DETAIL__TRIGGER_ACTUATOR__TYPE_SUPPORT_H_
