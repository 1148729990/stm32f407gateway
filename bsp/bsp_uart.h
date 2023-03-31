/**
  ******************************************************************************
  * File Name          : bsp_uart.h
  * Author             : xf
  * Version            : V1.1
  * Date               : 2021/03/04
  * Description        : 锟斤拷锟截接匡拷
  * Copyright (c) 2020 Shenzhen hande
  ******************************************************************************
**/

#ifndef _BSP_UART_H_
#define _BSP_UART_H_

#include "stm32g0xx_hal.h"

typedef union
{
	struct{
		uint8_t rx_idle       :1;  //锟斤拷锟斤拷锟斤拷锟捷猴拷锟斤拷锟竭匡拷锟斤拷
		uint8_t dma_tx_end    :1;  //DMA锟斤拷锟斤拷锟斤拷锟斤拷尾锟斤拷志位
		uint8_t dma_rx_end    :1;  //DMA锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷志位
		uint8_t error         :1;  //锟斤拷锟节达拷锟斤拷
                uint8_t rx_query_loop :1;  // 查询接收数据完成回环一次
		uint8_t               :3;  //锟斤拷锟斤拷
	}bit;
	uint8_t flag;
}uart_flag_u;                  //锟斤拷锟节憋拷志位

typedef struct{                         
    volatile uint16_t front;                       
    volatile uint16_t rear;                        
    uint8_t *buffer;    //锟斤拷锟轿伙拷锟斤拷锟斤拷
}uart_queue_t;          //锟斤拷锟节讹拷锟斤拷

typedef struct{ 
  UART_HandleTypeDef*  hUartHandle;
  uart_queue_t*        rx_queue;
  uint16_t             rx_queue_maxLen;
  volatile uart_flag_u UartFlag;
  DMA_HandleTypeDef*   hDMAUartRx; 
  void                 (* Uart_Init)(void);
  void                 (* Uart_TxCp_Callback)(void);
  void                 (* Uart_Idle_Callback)(void);
  void                 (* Uart_Tx_Mode)(void);            //锟斤拷锟斤拷485锟秸凤拷
  void                 (* Uart_Rx_Mode)(void);            //锟斤拷锟斤拷485锟秸凤拷
}UartCommonConfig_t;    //锟斤拷锟斤拷锟斤拷锟斤拷


extern UartCommonConfig_t  Uart1Config;
extern UartCommonConfig_t  Uart2Config;
extern UartCommonConfig_t  Uart3Config;
extern UartCommonConfig_t  Uart6Config;
extern UartCommonConfig_t  Uart4Config;
extern UartCommonConfig_t  Uart5Config;

#define BSP_UART_CONFIG(name)   &name##Config

extern uint8_t BspUartCommonInit(UartCommonConfig_t* UartXConfig);
extern uint8_t BspUartCommonSendBuffer(UartCommonConfig_t* UartXConfig,uint8_t *buffer,uint16_t length);
extern uint8_t BspUartCommonDmaSendBuffer(UartCommonConfig_t* UartXConfig,uint8_t *buffer,uint16_t length);
extern uint8_t BspUartCommonDmaReceive(UartCommonConfig_t* UartXConfig);
extern void Bsp_Uart_Idle_Callback(UartCommonConfig_t* UartXConfig);
extern uint8_t bsp_uart_queue_dequeue(UartCommonConfig_t *uart,uint8_t *data);
extern void bsp_uart_rx_queue_dequeue_software_update(UartCommonConfig_t *UartXConfig);
extern void Bsp_Two_Uarts_Direct_Connection_Test(UartCommonConfig_t* Uart1,UartCommonConfig_t* Uart2);

#endif