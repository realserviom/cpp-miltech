#pragma once

enum class DroneStateId { 
    STOPPED = 0, 
    ACCELERATING = 1, 
    DECELERATING = 2, 
    TURNING = 3, 
    MOVING = 4 
};