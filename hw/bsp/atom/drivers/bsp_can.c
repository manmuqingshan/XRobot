#include "bsp_can.h"

#include "FreeRTOS.h"
#include "bsp_def.h"
#include "bsp_sys.h"
#include "bsp_time.h"
#include "main.h"
#include "semphr.h"
#include "stm32g4xx_hal_fdcan.h"
#include "task.h"

typedef struct {
  FDCAN_RxHeaderTypeDef header;
  uint8_t data[64];
} can_raw_rx_t;

typedef struct {
  FDCAN_HandleTypeDef header;
  uint8_t data[8];
} can_raw_tx_t;

typedef struct {
  void (*fn)(bsp_can_t can, uint32_t id, uint8_t *data, void *arg);
  void *arg;
} can_callback_t;

extern FDCAN_HandleTypeDef hfdcan1;

static can_callback_t callback_list[BSP_CAN_NUM][BSP_CAN_CB_NUM];

static bool bsp_can_initd = false;

static can_raw_rx_t rx_buff[BSP_CAN_NUM];

static SemaphoreHandle_t rx_cplt_wait_sem[BSP_CAN_NUM];

FDCAN_HandleTypeDef *bsp_can_get_handle(bsp_can_t can) {
  switch (can) {
    case BSP_CAN_1:
      return &hfdcan1;
    default:
      return NULL;
  }
}

static bsp_can_t can_get(FDCAN_HandleTypeDef *hcan) {
  if (hcan->Instance == FDCAN1) {
    return BSP_CAN_1;
  } else {
    return BSP_CAN_ERR;
  }
}

void HAL_FDCAN_TxBufferCompleteCallback(FDCAN_HandleTypeDef *hcan,
                                        uint32_t BufferIndexes) {
  (void)BufferIndexes;
  BaseType_t px_higher_priority_task_woken = 0;
  xSemaphoreGiveFromISR(rx_cplt_wait_sem[can_get(hcan)],
                        &px_higher_priority_task_woken);
  if (px_higher_priority_task_woken != pdFALSE) {
    portYIELD();
  }
}

void HAL_FDCAN_TxBufferAbortCallback(FDCAN_HandleTypeDef *hcan,
                                     uint32_t BufferIndexes) {
  (void)BufferIndexes;
  BaseType_t px_higher_priority_task_woken = 0;
  xSemaphoreGiveFromISR(rx_cplt_wait_sem[can_get(hcan)],
                        &px_higher_priority_task_woken);
  if (px_higher_priority_task_woken != pdFALSE) {
    portYIELD();
  }
}

void HAL_FDCAN_ErrorCallback(FDCAN_HandleTypeDef *hcan) {
  BaseType_t px_higher_priority_task_woken = 0;
  xSemaphoreGiveFromISR(rx_cplt_wait_sem[can_get(hcan)],
                        &px_higher_priority_task_woken);
  if (px_higher_priority_task_woken != pdFALSE) {
    portYIELD();
  }
}

void bsp_can_init(void) {
  for (int i = 0; i < BSP_CAN_NUM; i++) {
    rx_cplt_wait_sem[i] = xSemaphoreCreateBinary();
  }

  FDCAN_FilterTypeDef can_filter = {0};

  can_filter.IdType = FDCAN_STANDARD_ID;
  can_filter.FilterIndex = 0;
  can_filter.FilterType = FDCAN_FILTER_MASK;
  can_filter.FilterConfig = FDCAN_FILTER_TO_RXFIFO0;
  can_filter.FilterID1 = 0x0000;
  can_filter.FilterID2 = 0x0000;
  if (HAL_FDCAN_ConfigFilter(&hfdcan1, &can_filter) != HAL_OK) {
    XB_ASSERT(false);
  }

  can_filter.IdType = FDCAN_EXTENDED_ID;
  can_filter.FilterIndex = 1;
  can_filter.FilterType = FDCAN_FILTER_MASK;
  can_filter.FilterConfig = FDCAN_FILTER_TO_RXFIFO0;
  can_filter.FilterID1 = 0x0000;
  can_filter.FilterID2 = 0x0000;
  if (HAL_FDCAN_ConfigFilter(&hfdcan1, &can_filter) != HAL_OK) {
    XB_ASSERT(false);
  }

  HAL_FDCAN_Start(&hfdcan1);  // 开启FDCAN
  HAL_FDCAN_ActivateNotification(&hfdcan1, FDCAN_IT_RX_FIFO0_NEW_MESSAGE, 0);
  HAL_FDCAN_ActivateNotification(&hfdcan1, FDCAN_IT_TX_FIFO_EMPTY, 0);

  bsp_can_initd = true;
}

static const uint8_t DLCtoBytes[] = {0, 1,  2,  3,  4,  5,  6,  7,
                                     8, 12, 16, 20, 24, 32, 48, 64};

static uint32_t count = 0;

static void can_rx_cb_fn(bsp_can_t can) {
  while (HAL_FDCAN_GetRxMessage(bsp_can_get_handle(can), FDCAN_RX_FIFO0,
                                &rx_buff[can].header,
                                rx_buff[can].data) == HAL_OK) {
    count++;
    if (rx_buff[can].header.FDFormat == FDCAN_CLASSIC_CAN) {
      if (callback_list[can][CAN_RX_MSG_CALLBACK].fn) {
        callback_list[can][CAN_RX_MSG_CALLBACK].fn(
            can, rx_buff[can].header.Identifier, rx_buff[can].data,
            callback_list[can][CAN_RX_MSG_CALLBACK].arg);
      }
    } else {
      if (callback_list[can][CANFD_RX_MSG_CALLBACK].fn) {
        bsp_canfd_data_t data = {
            .data = rx_buff[can].data,
            .size = DLCtoBytes[rx_buff[can].header.DataLength >> 16U]};

        callback_list[can][CANFD_RX_MSG_CALLBACK].fn(
            can, rx_buff[can].header.Identifier, (uint8_t *)&data,
            callback_list[can][CANFD_RX_MSG_CALLBACK].arg);
      }
    }
  }
}

void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *hcan, uint32_t RxFifo0ITs) {
  (void)RxFifo0ITs;
  can_rx_cb_fn(can_get(hcan));
}

bsp_status_t bsp_can_register_callback(
    bsp_can_t can, bsp_can_callback_t type,
    void (*callback)(bsp_can_t can, uint32_t id, uint8_t *data, void *arg),
    void *callback_arg) {
  assert_param(callback);
  assert_param(type != BSP_CAN_CB_NUM);

  callback_list[can][type].fn = callback;
  callback_list[can][type].arg = callback_arg;
  return BSP_OK;
}

bsp_status_t bsp_can_trans_packet(bsp_can_t can, bsp_can_format_t format,
                                  uint32_t id, uint8_t *data) {
  FDCAN_TxHeaderTypeDef header;

  header.Identifier = id;

  if (format == CAN_FORMAT_STD) {
    header.IdType = FDCAN_STANDARD_ID;
  } else {
    header.IdType = FDCAN_EXTENDED_ID;
  }

  header.TxFrameType = FDCAN_DATA_FRAME;
  header.DataLength = FDCAN_DLC_BYTES_8;
  header.ErrorStateIndicator = FDCAN_ESI_PASSIVE;
  header.BitRateSwitch = FDCAN_BRS_OFF;
  header.FDFormat = FDCAN_CLASSIC_CAN;
  header.TxEventFifoControl = FDCAN_STORE_TX_EVENTS;
  header.MessageMarker = 0x01;
  while ((bsp_can_get_handle(can)->Instance->TXFQS & FDCAN_TXFQS_TFQF) != 0U) {
    if (!xSemaphoreTake(rx_cplt_wait_sem[can], 10)) {
      return BSP_ERR_BUSY;
    }
  }

  return HAL_FDCAN_AddMessageToTxFifoQ(bsp_can_get_handle(can), &header,
                                       data) == HAL_OK
             ? BSP_OK
             : BSP_ERR;
}

static const uint32_t FDCAN_PACK_LEN_MAP[16] = {
    FDCAN_DLC_BYTES_0,  FDCAN_DLC_BYTES_1,  FDCAN_DLC_BYTES_2,
    FDCAN_DLC_BYTES_3,  FDCAN_DLC_BYTES_4,  FDCAN_DLC_BYTES_5,
    FDCAN_DLC_BYTES_6,  FDCAN_DLC_BYTES_7,  FDCAN_DLC_BYTES_8,
    FDCAN_DLC_BYTES_12, FDCAN_DLC_BYTES_16, FDCAN_DLC_BYTES_20,
    FDCAN_DLC_BYTES_24, FDCAN_DLC_BYTES_32, FDCAN_DLC_BYTES_48,
    FDCAN_DLC_BYTES_64,
};

bsp_status_t bsp_canfd_trans_packet(bsp_can_t can, bsp_can_format_t format,
                                    uint32_t id, uint8_t *data, size_t size) {
  FDCAN_TxHeaderTypeDef header;

  XB_ASSERT(size <= 64);

  header.Identifier = id;

  if (format == CAN_FORMAT_STD) {
    header.IdType = FDCAN_STANDARD_ID;
  } else {
    header.IdType = FDCAN_EXTENDED_ID;
  }

  header.TxFrameType = FDCAN_DATA_FRAME;

  if (size <= 8) {
    header.DataLength = FDCAN_PACK_LEN_MAP[size];
  } else if (size <= 24) {
    header.DataLength = FDCAN_PACK_LEN_MAP[(size - 9) / 4 + 1 + 8];
  } else if (size < 32) {
    header.DataLength = FDCAN_DLC_BYTES_32;
  } else if (size < 48) {
    header.DataLength = FDCAN_DLC_BYTES_48;
  } else {
    header.DataLength = FDCAN_DLC_BYTES_64;
  }

  header.ErrorStateIndicator = FDCAN_ESI_PASSIVE;
  header.BitRateSwitch = FDCAN_BRS_ON;
  header.FDFormat = FDCAN_FD_CAN;
  header.TxEventFifoControl = FDCAN_NO_TX_EVENTS;
  header.MessageMarker = 0x00;
  static uint32_t error_count = 0;

  while ((bsp_can_get_handle(can)->Instance->TXFQS & FDCAN_TXFQS_TFQF) != 0U) {
    if (error_count > 10) {
      return BSP_ERR;
    }
    if (!xSemaphoreTake(rx_cplt_wait_sem[can], 1)) {
      error_count++;
      return BSP_ERR_BUSY;
    }
  }

  error_count = 0;

  return HAL_FDCAN_AddMessageToTxFifoQ(bsp_can_get_handle(can), &header,
                                       data) == HAL_OK
             ? BSP_OK
             : BSP_ERR;
}
