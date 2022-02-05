#include "bsp_uart.h"

#include "comp_utils.h"

static bsp_callback_t callback_list[BSP_UART_NUM][BSP_UART_CB_NUM];

static bsp_uart_t uart_get(UART_HandleTypeDef *huart) {
  if (huart->Instance == USART3)
    return BSP_UART_DR16;
  else if (huart->Instance == USART1)
    return BSP_UART_REF;
  else if (huart->Instance == USART6)
    return BSP_UART_AI;
  /*
  else if (huart->Instance == USARTX)
                  return BSP_UART_XXX;
  */
  else
    return BSP_UART_ERR;
}

static void bsp_uart_callback(bsp_uart_callback_t cb_type,
                              UART_HandleTypeDef *huart) {
  bsp_uart_t bsp_uart = uart_get(huart);
  if (bsp_uart != BSP_UART_ERR) {
    bsp_callback_t cb = callback_list[bsp_uart][cb_type];

    if (cb.fn) {
      cb.fn(cb.arg);
    }
  }
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {
  bsp_uart_callback(BSP_UART_TX_CPLT_CB, huart);
}

void HAL_UART_TxHalfCpltCallback(UART_HandleTypeDef *huart) {
  bsp_uart_callback(BSP_UART_TX_HALF_CPLT_CB, huart);
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
  bsp_uart_callback(BSP_UART_RX_CPLT_CB, huart);
}

void HAL_UART_RxHalfCpltCallback(UART_HandleTypeDef *huart) {
  bsp_uart_callback(BSP_UART_RX_HALF_CPLT_CB, huart);
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart) {
  bsp_uart_callback(BSP_UART_ERROR_CB, huart);
}

void HAL_UART_AbortCpltCallback(UART_HandleTypeDef *huart) {
  bsp_uart_callback(BSP_UART_ABORT_CPLT_CB, huart);
}

void HAL_UART_AbortTransmitCpltCallback(UART_HandleTypeDef *huart) {
  bsp_uart_callback(BSP_UART_ABORT_TX_CPLT_CB, huart);
}

void HAL_UART_AbortReceiveCpltCallback(UART_HandleTypeDef *huart) {
  bsp_uart_callback(BSP_UART_ABORT_RX_CPLT_CB, huart);
}

void bsp_uart_irq_handler(UART_HandleTypeDef *huart) {
  if (__HAL_UART_GET_FLAG(huart, UART_FLAG_IDLE)) {
    __HAL_UART_CLEAR_IDLEFLAG(huart);
    bsp_uart_callback(BSP_UART_IDLE_LINE_CB, huart);
  }
}

UART_HandleTypeDef *bsp_uart_get_handle(bsp_uart_t uart) {
  switch (uart) {
    case BSP_UART_DR16:
      return &huart3;
    case BSP_UART_REF:
      return &huart1;
    case BSP_UART_AI:
      return &huart6;
    /*
    case BSP_UART_XXX:
            return &huartX;
    */
    default:
      return NULL;
  }
}

int8_t bsp_uart_register_callback(bsp_uart_t uart, bsp_uart_callback_t type,
                                 void (*callback)(void *), void *callback_arg) {
  ASSERT(callback);
  ASSERT(type != BSP_UART_CB_NUM);

  callback_list[uart][type].fn = callback;
  callback_list[uart][type].arg = callback_arg;
  return BSP_OK;
}