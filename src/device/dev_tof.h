#pragma once

#include <stdbool.h>

#include "FreeRTOS.h"
#include "comp_type.h"
#include "queue.h"

typedef struct {
  float dist;
  uint8_t status;
  uint16_t signal_strength;
} tof_feedback_t;

typedef enum {
  TOF_SENSOR_LEFT,
  TOF_SENSOR_RIGHT,
  TOF_SENSOR_NUMBER
} tof_number_t;

typedef struct {
  QueueHandle_t msgq_feedback;
  tof_feedback_t feedback[TOF_SENSOR_NUMBER];
} tof_t;

err_t tof_init(tof_t *tof);
err_t tof_update(tof_t *tof, uint32_t timeout);
err_t tof_handle_offline(tof_t *tof);
