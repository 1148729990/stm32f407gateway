/**
 ******************************************************************************
 * File Name          : bsp_uart.c
 * Author             : xf
 * Version            : V1.1
 * Date               : 2021/03/04
 * Description        : 板载接口
 * Copyright (c) 2020 Shenzhen GuoRen
 ******************************************************************************
 **/

#include "bsp_uart.h"
#include "core_usart.h"
#include "cmsis_os.h"

// 使用串口个数
#define BSP_UART_NUM_MAX    6


#define UART_CONFIG_DEF(name,uart_handle,dma_uart_rx,mx_uart_init)\
UartCommonConfig_t name##Config =\
{\
  .hUartHandle = &uart_handle,\
  .rx_queue = NULL,\
  .rx_queue_maxLen = 0,\
  .UartFlag.flag = 0,\
  .hDMAUartRx = &dma_uart_rx,\
  .Uart_Init = mx_uart_init,\
  .Uart_TxCp_Callback = NULL,\
  .Uart_Idle_Callback = NULL,\
  .Uart_Tx_Mode = NULL,\
  .Uart_Rx_Mode = NULL \
}



//串口 1
extern DMA_HandleTypeDef hdma_usart1_rx;
UART_CONFIG_DEF(Uart1,huart1,hdma_usart1_rx,MX_USART1_UART_Init);

//串口 2
extern DMA_HandleTypeDef hdma_usart2_rx;
UART_CONFIG_DEF(Uart2,huart2,hdma_usart2_rx,MX_USART2_UART_Init);

//串口 3
extern DMA_HandleTypeDef hdma_usart3_rx;
UART_CONFIG_DEF(Uart3,huart3,hdma_usart3_rx,MX_USART3_UART_Init);

//串口 4
extern DMA_HandleTypeDef hdma_usart4_rx;
UART_CONFIG_DEF(Uart4,huart4,hdma_usart4_rx,MX_USART4_UART_Init);

//串口 5
extern DMA_HandleTypeDef hdma_usart5_rx;
UART_CONFIG_DEF(Uart5,huart5,hdma_usart5_rx,MX_USART5_UART_Init);

//串口 6
extern DMA_HandleTypeDef hdma_usart6_rx;
UART_CONFIG_DEF(Uart6,huart6,hdma_usart6_rx,MX_USART6_UART_Init);
  
   
// 地址列表
static uint8_t bsp_uart_num = 0;
static UartCommonConfig_t *UartCongfigAdressTable[BSP_UART_NUM_MAX] = {NULL};

#define BSP_UART_ENTER_CRITICAL()       taskENTER_CRITICAL()
#define BSP_UART_EXIT_CRITICAL()        taskEXIT_CRITICAL()

#define BSP_UART_ENTER_CRITICAL_ISR()   uint32_t status_value = taskENTER_CRITICAL_FROM_ISR()
#define BSP_UART_EXIT_CRITICAL_ISR()    taskEXIT_CRITICAL_FROM_ISR(status_value)

/*************************************************
 * Fun Name    : BspUartInit
 * Description : 串口 初始化
 * Input       : None
 * Output      : None
 * Return      : None
 ************************************************/
uint8_t BspUartCommonInit(UartCommonConfig_t *UartXConfig)
{
  
  if(bsp_uart_num >=  BSP_UART_NUM_MAX)
  {
    return 0;
  }
  else
  {
    for(uint8_t i = 0; i < bsp_uart_num;i++)
    {
      if(UartCongfigAdressTable[i] == UartXConfig)
      {
        return 0;
      }
    }
    UartCongfigAdressTable[bsp_uart_num] = UartXConfig;
    bsp_uart_num++;  
  }
  
  UartXConfig->Uart_Init();
  UartXConfig->UartFlag.flag = 0x00;
  
  return 1;
}

/*************************************************
 * Fun Name    : BspUartCommonSendBuffer
 * Description : 串口 发送数据
 * Input       : buffer -  数据地址   length - 数据长度
 * Output      : None
 * Return      : 1 - 成功   0 - 失败
 ************************************************/
uint8_t BspUartCommonSendBuffer(UartCommonConfig_t *UartXConfig, uint8_t *buffer, uint16_t length)
{
  if (HAL_UART_Transmit(UartXConfig->hUartHandle, buffer, length, 1000) != HAL_OK)
  {
    return 0;
  }
  return 1;
}

/*************************************************
 * Fun Name    : BspUartDmaCommonSendBuffer
 * Description : 串口 DMA发送数据
 * Input       : buffer -  数据地址   length - 数据长度
 * Output      : None
 * Return      : 1 - 成功   0 - 失败
 ************************************************/
uint8_t BspUartCommonDmaSendBuffer(UartCommonConfig_t *UartXConfig, uint8_t *buffer, uint16_t length)
{
  if (HAL_UART_Transmit_DMA(UartXConfig->hUartHandle, (uint8_t *)buffer, length) != HAL_OK)
  {
    return 0;
  }
  return 1;
}

/*************************************************
 * Fun Name    : BspUartCommonDmaReceive
 * Description  : 串口 DMA接收初始化
 * Input       : None
 * Output      : None
 * Return      : 1 - 成功   0 - 失败
 ************************************************/
uint8_t BspUartCommonDmaReceive(UartCommonConfig_t *UartXConfig)
{
  if (HAL_UART_Receive_DMA(UartXConfig->hUartHandle, (uint8_t *)UartXConfig->rx_queue->buffer, UartXConfig->rx_queue_maxLen) == HAL_OK)
  {
    UartXConfig->rx_queue->rear = (UartXConfig->rx_queue_maxLen - (uint16_t)(__HAL_DMA_GET_COUNTER(UartXConfig->hDMAUartRx)) % UartXConfig->rx_queue_maxLen); // modify 2020 07/17 xiaofeng
    UartXConfig->rx_queue->front = UartXConfig->rx_queue->rear;
    __HAL_UART_CLEAR_IDLEFLAG(UartXConfig->hUartHandle);
    __HAL_UART_ENABLE_IT(UartXConfig->hUartHandle, UART_IT_IDLE); // 开空闲中断

    return 1;
  }
  return 0;
}

/*************************************************
 * Fun Name    : HAL_UART_RxCpltCallback
 * Description : UART 接收完成回调函数
 * Input       : None
 * Output      : None
 * Return      : 返回 0 —— 失败 ， 1 —— 成功
 ************************************************/
void Bsp_Uart_Idle_Callback(UartCommonConfig_t *UartXConfig)
{
  uint16_t temp_rear_index = 0;
  uint16_t rear_index = 0;
  uint16_t front_index = 0;
  
  if (__HAL_UART_GET_FLAG(UartXConfig->hUartHandle, UART_FLAG_IDLE) == SET) // 检测空闲中断
  {
    __HAL_UART_CLEAR_IDLEFLAG(UartXConfig->hUartHandle);

    BSP_UART_ENTER_CRITICAL_ISR();

    front_index = UartXConfig->rx_queue->front;
    temp_rear_index = UartXConfig->rx_queue->rear;

    rear_index = (UartXConfig->rx_queue_maxLen - (uint16_t)(__HAL_DMA_GET_COUNTER(UartXConfig->hDMAUartRx))) % UartXConfig->rx_queue_maxLen; // 最多只有 UartXConfig->MaxBuffLen - 1 个有效数据

    if ((temp_rear_index < front_index) && (rear_index >= front_index)) // 数据溢出
    {
      UartXConfig->rx_queue->front = (rear_index + 1) % UartXConfig->rx_queue_maxLen;
    }
    UartXConfig->rx_queue->rear = rear_index;

    BSP_UART_EXIT_CRITICAL_ISR();

    UartXConfig->UartFlag.bit.rx_idle = 1; // 空闲标志位
    
    if (UartXConfig->Uart_Idle_Callback != NULL)
    {
      UartXConfig->Uart_Idle_Callback();
    }
  }
}

static UartCommonConfig_t *bsp_uart_query_uart_config(USART_TypeDef *uart)
{
  UartCommonConfig_t *uart_config = NULL;
  for(uint8_t i = 0;i < bsp_uart_num; i++)
  {
      if((uart == UartCongfigAdressTable[i]->hUartHandle->Instance)&&(uart != NULL))
      {
        uart_config = UartCongfigAdressTable[i];
      }
  }
  return uart_config;
}

/*************************************************
 * Fun Name    : HAL_UART_TxCpltCallback
 * Description : UART 发送完成回调函数
 * Input       : None
 * Output      : None
 * Return      : 返回 0 —— 失败 ， 1 —— 成功
 ************************************************/
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *UartHandle)
{
  
  UartCommonConfig_t *uart_config = NULL;

  uart_config = bsp_uart_query_uart_config(UartHandle->Instance);
  
  if(NULL != uart_config)
  {
    if (uart_config->Uart_TxCp_Callback != NULL)
    {
      uart_config->Uart_TxCp_Callback();
    }
    uart_config->UartFlag.bit.dma_tx_end = 1;
  }
}

/*************************************************
 * Fun Name    : HAL_UART_RxCpltCallback
 * Description : UART DMA接收满完成回调函数
 * Input       : None
 * Output      : None
 * Return      : 返回 0 —— 失败 ， 1 —— 成功
 ************************************************/
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *UartHandle)
{
  uint16_t temp_rear_index = 0;
  uint16_t rear_index = 0;
  uint16_t front_index = 0;
  
  UartCommonConfig_t *uart_config = NULL;
  
  uart_config = bsp_uart_query_uart_config(UartHandle->Instance);
  
  if(NULL != uart_config)
  {
    BSP_UART_ENTER_CRITICAL_ISR();
    
    temp_rear_index = uart_config->rx_queue->rear; 
    front_index     = uart_config->rx_queue->front;
    
    rear_index      = (uart_config->rx_queue_maxLen - (uint16_t)(__HAL_DMA_GET_COUNTER(uart_config->hDMAUartRx))) % uart_config->rx_queue_maxLen; // 最多只有 UartXConfig->MaxBuffLen - 1 个有效数据
    
    if((temp_rear_index < front_index) || (rear_index == front_index)) // 数据溢出
    {
      uart_config->rx_queue->front = (rear_index + 1) % uart_config->rx_queue_maxLen;
    }
    uart_config->rx_queue->rear = rear_index;

    BSP_UART_EXIT_CRITICAL_ISR();
  }
}


//串口环形队列软件更新接收数据
void bsp_uart_rx_queue_dequeue_software_update(UartCommonConfig_t *UartXConfig)
{
  uint16_t temp_rear_index = 0;
  uint16_t rear_index = 0;
  uint16_t front_index = 0;
  
  uart_queue_t *queue;
  queue = UartXConfig->rx_queue;
  
  BSP_UART_ENTER_CRITICAL();
  
  front_index = queue->front;
  temp_rear_index = queue->rear;
  
  rear_index = (UartXConfig->rx_queue_maxLen - (uint16_t)(__HAL_DMA_GET_COUNTER(UartXConfig->hDMAUartRx))) % UartXConfig->rx_queue_maxLen; // 最多只有 UartXConfig->MaxBuffLen - 1 个有效数据
  
  if ((temp_rear_index < front_index) && (rear_index >= front_index)) // 数据溢出
  {
    queue->front = (rear_index + 1) % UartXConfig->rx_queue_maxLen;
  }
  queue->rear = rear_index;
  
  BSP_UART_EXIT_CRITICAL();
}

// 出队列   返回  0 队列空无数据  1 有数据
uint8_t bsp_uart_queue_dequeue(UartCommonConfig_t *uart, uint8_t *data)
{
  uart_queue_t *queue;
  uint32_t front_index = 0;
  
  queue = uart->rx_queue;
  if (queue->front != queue->rear) // 非空
  {
    BSP_UART_ENTER_CRITICAL();
    
    front_index = queue->front;
    queue->front = (front_index + 1) % uart->rx_queue_maxLen;
    *data = queue->buffer[front_index];
    
    BSP_UART_EXIT_CRITICAL();
    return 1;    
  }
  else
  {
    return 0;
  }
}

/*************************************************
 * Fun Name    : HAL_UART_ErrorCallback
 * Description : UART 错误回调函数
 * Input       : None
 * Output      : None
 * Return      : 返回 0 —— 失败 ， 1 —— 成功
 ************************************************/
void HAL_UART_ErrorCallback(UART_HandleTypeDef *UartHandle)
{
  
  UartCommonConfig_t *uart_config = NULL;  
  
  uart_config = bsp_uart_query_uart_config(UartHandle->Instance);
  
  __HAL_UART_CLEAR_FLAG(UartHandle, UART_CLEAR_OREF);
  
  if(NULL != uart_config)
  {
    BSP_UART_ENTER_CRITICAL_ISR();
    
    uart_config->UartFlag.bit.error = 1;
    BspUartCommonDmaReceive(uart_config);    
    
    BSP_UART_EXIT_CRITICAL_ISR();
  }
  
  __HAL_UART_ENABLE_IT(UartHandle, UART_IT_ERR);  
}

/*************************************************
 * Fun Name    : Bsp_UARTS_Direct_Connection
 * Description  : 两个串口直连
 * Input       : None
 * Output      : None
 * Return      : 1 - 成功   0 - 失败
 ************************************************/
void Bsp_Two_Uarts_Direct_Connection_Test(UartCommonConfig_t *Uart1, UartCommonConfig_t *Uart2)
{

  uint8_t test_buffer[10] = "1234567890";
  uint16_t length = 0;
  uint16_t temp_rear = 0;
  BspUartCommonInit(Uart1);
  BspUartCommonInit(Uart2);
  //	Uart1->Uart_Tx_Mode();
  //	Uart2->Uart_Tx_Mode();
  BspUartCommonDmaReceive(Uart1);
  BspUartCommonDmaReceive(Uart2);

  Uart2->UartFlag.bit.dma_tx_end = 0;
  BspUartCommonDmaSendBuffer(Uart2, test_buffer, 10);
  while (Uart2->UartFlag.bit.dma_tx_end == 0)
  {
    ;
  }
  while (1)
  {
    if (1 == Uart1->UartFlag.bit.rx_idle)
    {
      Uart1->UartFlag.bit.rx_idle = 0;
      while (Uart1->rx_queue->rear != Uart1->rx_queue->front)
      {
        //				Uart2->Uart_Tx_Mode();
        temp_rear = Uart1->rx_queue->rear;
        if (Uart1->rx_queue->front < temp_rear)
        {
          length = temp_rear - Uart1->rx_queue->front;
          Uart2->UartFlag.bit.dma_tx_end = 0;
          BspUartCommonDmaSendBuffer(Uart2, &Uart1->rx_queue->buffer[Uart1->rx_queue->front], length);
          while (Uart2->UartFlag.bit.dma_tx_end == 0)
          {
            ;
          }
          Uart1->rx_queue->front = temp_rear;
        }
        else
        {
          length = Uart1->rx_queue_maxLen - Uart1->rx_queue->front;
          BspUartCommonDmaSendBuffer(Uart1, &Uart1->rx_queue->buffer[Uart1->rx_queue->front], length);
          Uart1->rx_queue->front = 0;
        }
        //				Uart2->Uart_Rx_Mode();
      }
    }

    if (1 == Uart2->UartFlag.bit.rx_idle)
    {

      Uart2->UartFlag.bit.rx_idle = 0;
      while (Uart2->rx_queue->rear != Uart2->rx_queue->front)
      {
        //				Uart1->Uart_Tx_Mode();
        temp_rear = Uart2->rx_queue->rear;
        if (Uart2->rx_queue->front < temp_rear)
        {
          length = temp_rear - Uart2->rx_queue->front;
          Uart1->UartFlag.bit.dma_tx_end = 0;
          BspUartCommonDmaSendBuffer(Uart1, &Uart2->rx_queue->buffer[Uart2->rx_queue->front], length);
          while (Uart1->UartFlag.bit.dma_tx_end == 0)
          {
            ;
          }
          Uart2->rx_queue->front = temp_rear;
        }
        else
        {
          length = Uart2->rx_queue_maxLen - Uart2->rx_queue->front;
          BspUartCommonDmaSendBuffer(Uart2, &Uart2->rx_queue->buffer[Uart2->rx_queue->front], length);
          Uart2->rx_queue->front = 0;
        }
        //				Uart1->Uart_Rx_Mode();
      }
    }
  }
}
