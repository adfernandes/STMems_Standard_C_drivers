/*
 ******************************************************************************
 * @file    fifo.c
 * @author  Sensors Software Solution Team
 * @brief   This file shows how to get data from sensor FIFO.
 *
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
 * All rights reserved.</center></h2>
 *
 * This software component is licensed by ST under BSD 3-Clause license,
 * the "License"; You may not use this file except in compliance with the
 * License. You may obtain a copy of the License at:
 *                        opensource.org/licenses/BSD-3-Clause
 *
 ******************************************************************************
 */

/*
 * This example was developed using the following STMicroelectronics
 * evaluation boards:
 *
 * - STEVAL_MKI109V3 +
 * - NUCLEO_F411RE +
 * - DISCOVERY_SPC584B +
 *
 * Used interfaces:
 *
 * STEVAL_MKI109V3    - Host side:   USB (Virtual COM)
 *                    - Sensor side: SPI(Default) / I2C(supported)
 *
 * NUCLEO_STM32F411RE - Host side: UART(COM) to USB bridge
 *                    - Sensor side: I2C(Default) / SPI(supported)
 *
 * DISCOVERY_SPC584B  - Host side: UART(COM) to USB bridge
 *                    - Sensor side: I2C(Default) / SPI(supported)
 *
 * If you need to run this example on a different hardware platform a
 * modification of the functions: `platform_write`, `platform_read`,
 * `tx_com` and 'platform_init' is required.
 *
 */

/* STMicroelectronics evaluation boards definition
 *
 * Please uncomment ONLY the evaluation boards in use.
 * If a different hardware is used please comment all
 * following target board and redefine yours.
 */

//#define STEVAL_MKI109V3  /* little endian */
//#define NUCLEO_F411RE    /* little endian */
//#define SPC584B_DIS      /* big endian */

/* ATTENTION: By default the driver is little endian. If you need switch
 *            to big endian please see "Endianness definitions" in the
 *            header file of the driver (_reg.h).
 */

#if defined(STEVAL_MKI109V3)
/* MKI109V3: Define communication interface */
#define SENSOR_BUS hspi2
/* MKI109V3: Vdd and Vddio power supply values */
#define PWM_3V3 915

#elif defined(NUCLEO_F411RE)
/* NUCLEO_F411RE: Define communication interface */
#define SENSOR_BUS hi2c1

#elif defined(SPC584B_DIS)
/* DISCOVERY_SPC584B: Define communication interface */
#define SENSOR_BUS I2CD1

#endif

/* Includes ------------------------------------------------------------------*/
#include <string.h>
#include <stdio.h>
#include "lsm6dsv16bx_reg.h"

#if defined(NUCLEO_F411RE)
#include "stm32f4xx_hal.h"
#include "usart.h"
#include "gpio.h"
#include "i2c.h"

#elif defined(STEVAL_MKI109V3)
#include "stm32f4xx_hal.h"
#include "usbd_cdc_if.h"
#include "gpio.h"
#include "spi.h"
#include "tim.h"

#elif defined(SPC584B_DIS)
#include "components.h"
#endif

/* Private macro -------------------------------------------------------------*/
#define    BOOT_TIME            10 //ms

/* Private variables ---------------------------------------------------------*/
static uint8_t whoamI;
static uint8_t tx_buffer[1000];
static lsm6dsv16bx_filt_settling_mask_t filt_settling_mask;

/* Extern variables ----------------------------------------------------------*/

/* Private functions ---------------------------------------------------------*/
static int16_t *datax;
static int16_t *datay;
static int16_t *dataz;

/*
 *   WARNING:
 *   Functions declare in this section are defined at the end of this file
 *   and are strictly related to the hardware platform used.
 *
 */
static int32_t platform_write(void *handle, uint8_t reg, const uint8_t *bufp,
                              uint16_t len);
static int32_t platform_read(void *handle, uint8_t reg, uint8_t *bufp,
                             uint16_t len);
static void tx_com( uint8_t *tx_buffer, uint16_t len );
static void platform_delay(uint32_t ms);
static void platform_init(void);

/* Main Example --------------------------------------------------------------*/
void lsm6dsv16bx_fifo(void)
{
  lsm6dsv16bx_fifo_status_t fifo_status;
  lsm6dsv16bx_reset_t rst;
  stmdev_ctx_t dev_ctx;

  /* Uncomment to configure INT 1 */
  //lsm6dsv16bx_pin_int1_route_t int1_route;
  /* Uncomment to configure INT 2 */
  //lsm6dsv16bx_pin_int2_route_t int2_route;

  /* Initialize mems driver interface */
  dev_ctx.write_reg = platform_write;
  dev_ctx.read_reg = platform_read;
  dev_ctx.mdelay = platform_delay;
  dev_ctx.handle = &SENSOR_BUS;

  /* Init test platform */
  platform_init();

  /* Wait sensor boot time */
  platform_delay(BOOT_TIME);

  /* Check device ID */
  lsm6dsv16bx_device_id_get(&dev_ctx, &whoamI);

  if (whoamI != LSM6DSV16BX_ID)
    while (1);

  /* Restore default configuration */
  lsm6dsv16bx_reset_set(&dev_ctx, LSM6DSV16BX_RESTORE_CTRL_REGS);
  do {
    lsm6dsv16bx_reset_get(&dev_ctx, &rst);
  } while (rst != LSM6DSV16BX_READY);

  /* Enable Block Data Update */
  lsm6dsv16bx_block_data_update_set(&dev_ctx, PROPERTY_ENABLE);

  /* Set full scale */
  lsm6dsv16bx_xl_full_scale_set(&dev_ctx, LSM6DSV16BX_2g);
  lsm6dsv16bx_gy_full_scale_set(&dev_ctx, LSM6DSV16BX_2000dps);

  /* Configure filtering chain */
  filt_settling_mask.drdy = PROPERTY_ENABLE;
  filt_settling_mask.irq_xl = PROPERTY_ENABLE;
  filt_settling_mask.irq_g = PROPERTY_ENABLE;
  lsm6dsv16bx_filt_settling_mask_set(&dev_ctx, filt_settling_mask);
  lsm6dsv16bx_filt_gy_lp1_set(&dev_ctx, PROPERTY_ENABLE);
  lsm6dsv16bx_filt_gy_lp1_bandwidth_set(&dev_ctx, LSM6DSV16BX_GY_ULTRA_LIGHT);
  lsm6dsv16bx_filt_xl_lp2_set(&dev_ctx, PROPERTY_ENABLE);
  lsm6dsv16bx_filt_xl_lp2_bandwidth_set(&dev_ctx, LSM6DSV16BX_XL_STRONG);

  /*
   * Set FIFO watermark (number of unread sensor data TAG + 6 bytes
   * stored in FIFO) to 10 samples
   */
  lsm6dsv16bx_fifo_watermark_set(&dev_ctx, 50);
  lsm6dsv16bx_fifo_stop_on_wtm_set(&dev_ctx, PROPERTY_ENABLE);
  lsm6dsv16bx_fifo_xl_batch_set(&dev_ctx, LSM6DSV16BX_XL_BATCHED_AT_7Hz5);
  lsm6dsv16bx_fifo_gy_batch_set(&dev_ctx, LSM6DSV16BX_GY_BATCHED_AT_15Hz);

  /* Set FIFO mode to Stream mode (aka Continuous Mode) */
  lsm6dsv16bx_fifo_mode_set(&dev_ctx, LSM6DSV16BX_STREAM_MODE);

  /* Set Output Data Rate.
   * Selected data rate have to be equal or greater with respect
   * with MLC data rate.
   */
  lsm6dsv16bx_xl_data_rate_set(&dev_ctx, LSM6DSV16BX_XL_ODR_AT_30Hz);
  lsm6dsv16bx_gy_data_rate_set(&dev_ctx, LSM6DSV16BX_GY_ODR_AT_60Hz);

  /* Wait samples. */
  while (1) {
    uint16_t num = 0;

    /* Read watermark flag */
    lsm6dsv16bx_fifo_status_get(&dev_ctx, &fifo_status);

    if (fifo_status.fifo_th == 1) {
      num = fifo_status.fifo_level;
      sprintf((char *)tx_buffer, "-- FIFO num %d \r\n", num);
      tx_com(tx_buffer, strlen((char const *)tx_buffer));

      while (num--) {
        lsm6dsv16bx_fifo_out_raw_t f_data;

        /* Read FIFO tag */
        lsm6dsv16bx_fifo_out_raw_get(&dev_ctx, &f_data);
        dataz = (int16_t *)&f_data.data[0]; /* axis are inverted */
        datay = (int16_t *)&f_data.data[2];
        datax = (int16_t *)&f_data.data[4];

        switch (f_data.tag) {
          case LSM6DSV16BX_XL_NC_TAG:
            sprintf((char *)tx_buffer, "ACC [mg]:\t%4.2f\t%4.2f\t%4.2f\r\n",
                  lsm6dsv16bx_from_fs2_to_mg(*datax),
                  lsm6dsv16bx_from_fs2_to_mg(*datay),
                  lsm6dsv16bx_from_fs2_to_mg(*dataz));
            tx_com(tx_buffer, strlen((char const *)tx_buffer));
            break;

          case LSM6DSV16BX_GY_NC_TAG:
            sprintf((char *)tx_buffer, "GYR [mdps]:\t%4.2f\t%4.2f\t%4.2f\r\n",
                  lsm6dsv16bx_from_fs2000_to_mdps(*datax),
                  lsm6dsv16bx_from_fs2000_to_mdps(*datay),
                  lsm6dsv16bx_from_fs2000_to_mdps(*dataz));
            tx_com(tx_buffer, strlen((char const *)tx_buffer));
            break;

          default:
            /* Flush unused samples */
            break;
        }
      }

      sprintf((char *)tx_buffer, "------ \r\n\r\n");
      tx_com(tx_buffer, strlen((char const *)tx_buffer));
    }
  }
}

/*
 * @brief  Write generic device register (platform dependent)
 *
 * @param  handle    customizable argument. In this examples is used in
 *                   order to select the correct sensor bus handler.
 * @param  reg       register to write
 * @param  bufp      pointer to data to write in register reg
 * @param  len       number of consecutive register to write
 *
 */
static int32_t platform_write(void *handle, uint8_t reg, const uint8_t *bufp,
                              uint16_t len)
{
#if defined(NUCLEO_F411RE)
  HAL_I2C_Mem_Write(handle, LSM6DSV16BX_I2C_ADD_L, reg,
                    I2C_MEMADD_SIZE_8BIT, (uint8_t*) bufp, len, 1000);
#elif defined(STEVAL_MKI109V3)
  HAL_GPIO_WritePin(CS_up_GPIO_Port, CS_up_Pin, GPIO_PIN_RESET);
  HAL_SPI_Transmit(handle, &reg, 1, 1000);
  HAL_SPI_Transmit(handle, (uint8_t*) bufp, len, 1000);
  HAL_GPIO_WritePin(CS_up_GPIO_Port, CS_up_Pin, GPIO_PIN_SET);
#elif defined(SPC584B_DIS)
  i2c_lld_write(handle,  LSM6DSV16BX_I2C_ADD_L & 0xFE, reg, (uint8_t*) bufp, len);
#endif
  return 0;
}

/*
 * @brief  Read generic device register (platform dependent)
 *
 * @param  handle    customizable argument. In this examples is used in
 *                   order to select the correct sensor bus handler.
 * @param  reg       register to read
 * @param  bufp      pointer to buffer that store the data read
 * @param  len       number of consecutive register to read
 *
 */
static int32_t platform_read(void *handle, uint8_t reg, uint8_t *bufp,
                             uint16_t len)
{
#if defined(NUCLEO_F411RE)
  HAL_I2C_Mem_Read(handle, LSM6DSV16BX_I2C_ADD_L, reg,
                   I2C_MEMADD_SIZE_8BIT, bufp, len, 1000);
#elif defined(STEVAL_MKI109V3)
  reg |= 0x80;
  HAL_GPIO_WritePin(CS_up_GPIO_Port, CS_up_Pin, GPIO_PIN_RESET);
  HAL_SPI_Transmit(handle, &reg, 1, 1000);
  HAL_SPI_Receive(handle, bufp, len, 1000);
  HAL_GPIO_WritePin(CS_up_GPIO_Port, CS_up_Pin, GPIO_PIN_SET);
#elif defined(SPC584B_DIS)
  i2c_lld_read(handle, LSM6DSV16BX_I2C_ADD_L & 0xFE, reg, bufp, len);
#endif
  return 0;
}

/*
 * @brief  platform specific outputs on terminal (platform dependent)
 *
 * @param  tx_buffer     buffer to transmit
 * @param  len           number of byte to send
 *
 */
static void tx_com(uint8_t *tx_buffer, uint16_t len)
{
#if defined(NUCLEO_F411RE)
  HAL_UART_Transmit(&huart2, tx_buffer, len, 1000);
#elif defined(STEVAL_MKI109V3)
  CDC_Transmit_FS(tx_buffer, len);
#elif defined(SPC584B_DIS)
  sd_lld_write(&SD2, tx_buffer, len);
#endif
}

/*
 * @brief  platform specific delay (platform dependent)
 *
 * @param  ms        delay in ms
 *
 */
static void platform_delay(uint32_t ms)
{
#if defined(NUCLEO_F411RE) | defined(STEVAL_MKI109V3)
  HAL_Delay(ms);
#elif defined(SPC584B_DIS)
  osalThreadDelayMilliseconds(ms);
#endif
}

/*
 * @brief  platform specific initialization (platform dependent)
 */
static void platform_init(void)
{
#if defined(STEVAL_MKI109V3)
  TIM3->CCR1 = PWM_3V3;
  TIM3->CCR2 = PWM_3V3;
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2);
  HAL_Delay(1000);
#endif
}
