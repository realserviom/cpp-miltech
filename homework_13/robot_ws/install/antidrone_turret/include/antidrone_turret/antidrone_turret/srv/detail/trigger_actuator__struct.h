// generated from rosidl_generator_c/resource/idl__struct.h.em
// with input from antidrone_turret:srv/TriggerActuator.idl
// generated code does not contain a copyright notice

// IWYU pragma: private, include "antidrone_turret/srv/trigger_actuator.h"


#ifndef ANTIDRONE_TURRET__SRV__DETAIL__TRIGGER_ACTUATOR__STRUCT_H_
#define ANTIDRONE_TURRET__SRV__DETAIL__TRIGGER_ACTUATOR__STRUCT_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>


// Constants defined in the message

/// Struct defined in srv/TriggerActuator in the package antidrone_turret.
typedef struct antidrone_turret__srv__TriggerActuator_Request
{
  float confidence;
  float distance_m;
} antidrone_turret__srv__TriggerActuator_Request;

// Struct for a sequence of antidrone_turret__srv__TriggerActuator_Request.
typedef struct antidrone_turret__srv__TriggerActuator_Request__Sequence
{
  antidrone_turret__srv__TriggerActuator_Request * data;
  /// The number of valid items in data
  size_t size;
  /// The number of allocated items in data
  size_t capacity;
} antidrone_turret__srv__TriggerActuator_Request__Sequence;

// Constants defined in the message

/// Struct defined in srv/TriggerActuator in the package antidrone_turret.
typedef struct antidrone_turret__srv__TriggerActuator_Response
{
  bool accepted;
  uint32_t trigger_count;
} antidrone_turret__srv__TriggerActuator_Response;

// Struct for a sequence of antidrone_turret__srv__TriggerActuator_Response.
typedef struct antidrone_turret__srv__TriggerActuator_Response__Sequence
{
  antidrone_turret__srv__TriggerActuator_Response * data;
  /// The number of valid items in data
  size_t size;
  /// The number of allocated items in data
  size_t capacity;
} antidrone_turret__srv__TriggerActuator_Response__Sequence;

// Constants defined in the message

// Include directives for member types
// Member 'info'
#include "service_msgs/msg/detail/service_event_info__struct.h"

// constants for array fields with an upper bound
// request
enum
{
  antidrone_turret__srv__TriggerActuator_Event__request__MAX_SIZE = 1
};
// response
enum
{
  antidrone_turret__srv__TriggerActuator_Event__response__MAX_SIZE = 1
};

/// Struct defined in srv/TriggerActuator in the package antidrone_turret.
typedef struct antidrone_turret__srv__TriggerActuator_Event
{
  service_msgs__msg__ServiceEventInfo info;
  antidrone_turret__srv__TriggerActuator_Request__Sequence request;
  antidrone_turret__srv__TriggerActuator_Response__Sequence response;
} antidrone_turret__srv__TriggerActuator_Event;

// Struct for a sequence of antidrone_turret__srv__TriggerActuator_Event.
typedef struct antidrone_turret__srv__TriggerActuator_Event__Sequence
{
  antidrone_turret__srv__TriggerActuator_Event * data;
  /// The number of valid items in data
  size_t size;
  /// The number of allocated items in data
  size_t capacity;
} antidrone_turret__srv__TriggerActuator_Event__Sequence;

#ifdef __cplusplus
}
#endif

#endif  // ANTIDRONE_TURRET__SRV__DETAIL__TRIGGER_ACTUATOR__STRUCT_H_
