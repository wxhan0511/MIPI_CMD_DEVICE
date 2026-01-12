/* Includes ------------------------------------------------------------------*/
#include "i2c_task.h"
#include <stdlib.h>
#include "main.h"
#include "i2c.h"
#include "oled.h"
#include "bsp_ads1256.h"
#include "bsp_mcp4728.h"
#include "bsp_power.h"
#include "power_control.h"
#include "bsp_calibration.h"
#include "drv_ra_device.h"
/*utils*/
#include "utils.h"
/* User-defined variables -----------------------------------------------------*/
float latest_sample_data[8] = {0};     // Store the latest sampled data for 8 channel after conversion and calibration
float latest_sample_raw_data[8] = {0}; // Store the latest sampled raw data for 8 channel
uint8_t latest_sample_index[8] = {0};  // Store the corresponding index of the latest sampled data for 8 channel

static uint8_t channel_num = 0;
static float offset, gain, IV_data = 0.0f;

static uint8_t cmd_header = 0;
static uint8_t cmd = 0;
static set_power_data_frame set_power_frame;
static BSP_STATUS status = BSP_ERROR;
static uint8_t state = 0;
static uint8_t temp = 0;
static int16_t temp_data;
static double temp_double_data;
static float temp_float_data;
static float_bytes_t float_bytes;

uint8_t i2c1_tx_buf[64] = {0};
uint8_t i2c1_rx_buf[64] = {0};

static uint8_t rx_buf[64] = {0};
static uint8_t tx_buf[64] = {0};

volatile uint8_t i2c1_rx_ready_flag = 0;

osMutexId_t g_i2c1Mutex = NULL;
osMutexId_t g_i2c2Mutex = NULL;

osThreadId_t masterTxTaskHandle = NULL;
osThreadId_t masterRxTaskHandle = NULL;
osThreadId_t slaveTxTaskHandle = NULL;
osThreadId_t slaveRxTaskHandle = NULL;

const osThreadAttr_t masterTxTask_attributes = {
    .name = "MasterTxTask",
    .priority = osPriorityNormal,
    .stack_size = 128 * 4};

const osThreadAttr_t masterRxTask_attributes = {
    .name = "MasterRxTask",
    .priority = osPriorityNormal,
    .stack_size = 128 * 4};
const osThreadAttr_t slaveTxTask_attributes = {
    .name = "SlaveTxTask",
    .priority = osPriorityNormal,
    .stack_size = 128 * 4};
const osThreadAttr_t slaveRxTask_attributes = {
    .name = "SlaveRxTask",
    .priority = osPriorityNormal,
    .stack_size = 128 * 4};
/* User-defined function implementation -------------------------------------- */
void I2C1_Mutex_Init(void)
{
  osMutexAttr_t attr = {
      .name = "I2C1_Mutex",
      // 普通互斥 + 优先级继承；如需递归请改为：osMutexRecursive | osMutexPrioInherit
      .attr_bits = osMutexPrioInherit};
  g_i2c1Mutex = osMutexNew(&attr);
}

osStatus_t I2C1_Lock(uint32_t timeout)
{
  if (g_i2c1Mutex == NULL)
  {
    // 惰性创建（可选），避免忘记初始化
    I2C1_Mutex_Init();
  }
  return osMutexAcquire(g_i2c1Mutex, timeout);
}

osStatus_t I2C1_Unlock(void)
{
  if (g_i2c1Mutex == NULL)
    return osError;
  return osMutexRelease(g_i2c1Mutex);
}
void I2C2_Mutex_Init(void)
{
  osMutexAttr_t attr = {
      .name = "I2C2_Mutex",
      // 普通互斥 + 优先级继承；如需递归请改为：osMutexRecursive | osMutexPrioInherit
      .attr_bits = osMutexPrioInherit};
  g_i2c2Mutex = osMutexNew(&attr);
}
osStatus_t I2C2_Lock(uint32_t timeout)
{
  if (g_i2c2Mutex == NULL)
  {
    // 惰性创建（可选），避免忘记初始化
    I2C2_Mutex_Init();
  }
  return osMutexAcquire(g_i2c2Mutex, timeout);
}

osStatus_t I2C2_Unlock(void)
{
  if (g_i2c2Mutex == NULL)
    return osError;
  return osMutexRelease(g_i2c2Mutex);
}

void slave_rx_task_init()
{
  slaveRxTaskHandle = osThreadNew(SlaveRxTask, NULL, &slaveRxTask_attributes);
  if (slaveRxTaskHandle == NULL)
    RA_POWEREX_ERROR("slaveRxTaskHandle create failed\r\n");
}

void slave_tx_task_init()
{
  slaveTxTaskHandle = osThreadNew(SlaveTxTask, NULL, &slaveTxTask_attributes);
  if (slaveTxTaskHandle == NULL)
    RA_POWEREX_ERROR("slaveTxTaskHandle create failed\r\n");
}

void master_tx_task_init()
{
  masterTxTaskHandle = osThreadNew(MasterTxTask, NULL, &masterTxTask_attributes);
  if (masterTxTaskHandle == NULL)
    RA_POWEREX_ERROR("masterTxTaskHandle create failed\r\n");
}

void master_rx_task_init()
{
  masterRxTaskHandle = osThreadNew(MasterRxTask, NULL, &masterRxTask_attributes);
  if (masterRxTaskHandle == NULL)
    RA_POWEREX_ERROR("masterRxTaskHandle create failed\r\n");
}

void SlaveRxTask(void *argument)
{
  for (;;)
  {
#if 0
    if (i2c1_rx_ready_flag == 1)
    {
      i2c1_rx_ready_flag = 0;
      HAL_NVIC_DisableIRQ(I2C1_EV_IRQn);
      memcpy(rx_buf, i2c1_rx_buf, sizeof(i2c1_rx_buf));
      HAL_NVIC_EnableIRQ(I2C1_EV_IRQn);
    }
    else
    {
      osDelay(10);
      continue;
    }
    
    memset(tx_buf, 0, sizeof(tx_buf));
    // 打印收到的数据

    for (uint32_t i = 0; i < sizeof(rx_buf); i++)
    {
      I2C_DEBUG("%02X ", rx_buf[i]);
      if ((i + 1) % 16 == 0)
      {
        I2C_DEBUG("\r\n");
      }
    }

    // 解析收到的数据
    
    cmd_header = rx_buf[0];
    cmd = rx_buf[1];

    // tx和rx头一样
    tx_buf[0] = cmd_header;
    tx_buf[1] = cmd;

    // 解析header
    if (cmd_header != 0xA0)
    {
      I2C_ERROR("receive data header: %x\r\n", cmd_header);
      break;
    }

    state = BSP_OK;

    switch (cmd)
    {
    case GET_ID:
      tx_buf[2] = id; // 1,2,3,4:bit1~4
      break;
    case GET_SW_VERSION:
      tx_buf[2] = id;
      memcpy(&tx_buf[3], sw_version, 4);
      break;
    case VOL_SET:

      memcpy(&set_power_frame, rx_buf, sizeof(set_power_frame));
      I2C_DEBUG("set_power_frame.power_name:%x\r\n", set_power_frame.power_name);
      switch (set_power_frame.power_name)
      {
      case MCP4728_CHANNEL_A:
        I2C_DEBUG("Setting ELVDD Voltage...\r\n");
        ELVDD_DISABLE();
        I2C_DEBUG("Target Vol:%f\r\n", set_power_frame.value.float_value[0]);
        // 保存最新电压值vi
        g_calibration_manager.data.elvdd_last_voltage = set_power_frame.value.float_value[0];
        calibration_save();
        // 校准数据
        set_power_frame.value.float_value[0] = (set_power_frame.value.float_value[0] - g_calibration_manager.data.da_data.elvdd_set_offset) / (g_calibration_manager.data.da_data.elvdd_set_gain);
        dac_dev.val[0] = float_to_uint16_round(set_power_frame.value.float_value[0]);
        I2C_DEBUG("Vi:%f\r\n", set_power_frame.value.float_value[0]);
        status = bsp_dac_single_voltage_set(&dac_dev, 0, dac_dev.val[0], 0);
        if (status != BSP_OK)
        {
          I2C_DEBUG("ELVDD set voltage failed\r\n");
          state = 1;
        }
        //HAL_GPIO_WritePin(LDAC_Port, LDAC_Pin, GPIO_PIN_RESET);
        ELVDD_ENABLE();
        I2C_DEBUG("ELVDD set vol done\r\n");
        break;
      case MCP4728_CHANNEL_B:
        I2C_DEBUG("Setting ELVSS Voltage...\r\n");
        ELVSS_DISABLE();
        I2C_DEBUG("Target Vol:%f\r\n", set_power_frame.value.float_value[0]);
        // 保存最新电压值
        g_calibration_manager.data.elvss_last_voltage = set_power_frame.value.float_value[0];
        calibration_save();
        // 校准数据    g_calibration_manager.data.elvss_last_voltage = (-g_calibration_manager.data.elvss_last_voltage + da_calibration_data.elvss_set_offset) / (da_calibration_data.elvss_set_gain);

        set_power_frame.value.float_value[0] = (-set_power_frame.value.float_value[0] + g_calibration_manager.data.da_data.elvss_set_offset) / (g_calibration_manager.data.da_data.elvss_set_gain);
        dac_dev.val[1] = float_to_uint16_round(set_power_frame.value.float_value[0]);
        I2C_DEBUG("Vi:%f\r\n", set_power_frame.value.float_value[0]);
        status = bsp_dac_single_voltage_set(&dac_dev, 1, dac_dev.val[1], 0);

        if (status != BSP_OK)
        {
          I2C_DEBUG("ELVSS set voltage failed\r\n");
          state = 1;
        }
        //HAL_GPIO_WritePin(LDAC_Port, LDAC_Pin, GPIO_PIN_RESET);
        I2C_DEBUG("ELVSS set vol done\r\n");
        ELVSS_ENABLE();
        break;
      case MCP4728_CHANNEL_C:
        I2C_DEBUG("Setting VCC Voltage...\r\n");
        VCC_DISABLE();
        I2C_DEBUG("Target Vol:%f\r\n", set_power_frame.value.float_value[0]);
        // 保存最新电压值
        g_calibration_manager.data.vcc_last_voltage = set_power_frame.value.float_value[0];
        calibration_save();
        // 校准数据
        set_power_frame.value.float_value[0] = (set_power_frame.value.float_value[0] - g_calibration_manager.data.da_data.vcc_set_offset) / (g_calibration_manager.data.da_data.vcc_set_gain);
        dac_dev.val[2] = float_to_uint16_round(set_power_frame.value.float_value[0]);
        status = bsp_dac_single_voltage_set(&dac_dev, 2, dac_dev.val[2], 0);
        if (status != BSP_OK)
        {
          I2C_DEBUG("VCC set voltage failed\r\n");
          state = 1;
        }
        I2C_DEBUG("Vi:%f\r\n", set_power_frame.value.float_value[0]);
        //HAL_GPIO_WritePin(LDAC_Port, LDAC_Pin, GPIO_PIN_RESET);
        VCC_ENABLE();
        I2C_DEBUG("VCC set vol done\r\n");
        break;
      case MCP4728_CHANNEL_D:
        I2C_DEBUG("Setting IOVCC Voltage...\r\n");
        IOVCC_DISABLE();
        I2C_DEBUG("Target Vol:%f\r\n", set_power_frame.value.float_value[0]);

        // 保存最新电压值
        g_calibration_manager.data.iovcc_last_voltage = set_power_frame.value.float_value[0];
        calibration_save();
        // 校准数据
        set_power_frame.value.float_value[0] = (set_power_frame.value.float_value[0] - g_calibration_manager.data.da_data.iovcc_set_offset) / (g_calibration_manager.data.da_data.iovcc_set_gain);
        dac_dev.val[3] = float_to_uint16_round(set_power_frame.value.float_value[0]);
        status = bsp_dac_single_voltage_set(&dac_dev, 3, dac_dev.val[3], 0);

        if (status != BSP_OK)
        {
          I2C_DEBUG("IOVCC set voltage failed\r\n");
          state = 1;
        }
        I2C_DEBUG("Vi:%f\r\n", set_power_frame.value.float_value[0]);
        //HAL_GPIO_WritePin(LDAC_Port, LDAC_Pin, GPIO_PIN_RESET);
        IOVCC_ENABLE();
        I2C_DEBUG("IOVCC set vol done\r\n");
        break;
      case 4:
        I2C_DEBUG("Setting IOVCCxb Voltage...\r\n");
        I2C_DEBUG("Target Vol:%f\r\n", set_power_frame.value.float_value[0]);
        temp = float_to_uint8_round(set_power_frame.value.float_value[0] / 100);
        status = ra_dev_main_0.ops->set_power_vol(ra_dev_main_0.dev, RA_POWER_IOVCC, temp);
        // 保存最新电压值
        g_calibration_manager.data.xb_iovcc_last_voltage = set_power_frame.value.float_value[0];
        calibration_save();
        if (status != BSP_OK)
        {
          I2C_DEBUG("ra xb IOVCC set power failed\r\n");
          state = 1;
        }
        ra_dev_main_0.dev->lp3907->read(RA_LP3907_1_ADDRESS, 0x10, &temp);
        I2C_DEBUG("raxb IOVCC read reg done:%x\r\n", temp);
        ra_dev_main_0.dev->lp3907->write(RA_LP3907_1_ADDRESS, 0x10, temp | (1 << 4));
        I2C_DEBUG("ra xb IOVCC write reg done:%x\r\n", temp);
        I2C_DEBUG("ra xb IOVCC set vol done\r\n");
        break;
      case 5:
        I2C_DEBUG("Setting VCIxb Voltage...\r\n");
        I2C_DEBUG("Target Vol:%f\r\n", set_power_frame.value.float_value[0]);
        temp = float_to_uint8_round(set_power_frame.value.float_value[0] / 100);
        // osDelay(10);
        status = ra_dev_main_0.ops->set_power_vol(ra_dev_main_0.dev, RA_POWER_VCI, temp);
        // 保存最新电压值
        g_calibration_manager.data.vci_last_voltage = set_power_frame.value.float_value[0];
        calibration_save();
        if (status != BSP_OK)
        {
          I2C_DEBUG("VCI set power failed\r\n");
          state = 1;
        }
        ra_dev_main_0.dev->lp3907->read(RA_LP3907_1_ADDRESS, 0x10, &temp);
        I2C_DEBUG("VCI read reg done:%x\r\n", temp);
        ra_dev_main_0.dev->lp3907->write(RA_LP3907_1_ADDRESS, 0x10, temp | (1 << 6));
        I2C_DEBUG("VCI write reg done:%x\r\n", temp);
        I2C_DEBUG("VCI set vol done\r\n");
        break;
      case 6:
        I2C_DEBUG("Setting VSPxb Voltage...\r\n");
        I2C_DEBUG("Target Vol:%f\r\n", set_power_frame.value.float_value[0]);
        temp = float_to_uint8_round(set_power_frame.value.float_value[0] / 100);
        // osDelay(10);
        status = ra_dev_main_0.ops->set_power_vol(ra_dev_main_0.dev, RA_POWER_VSP, temp);
        // 保存最新电压值
        g_calibration_manager.data.vsp_last_voltage = set_power_frame.value.float_value[0];
        calibration_save();
        if (status != BSP_OK)
        {
          I2C_DEBUG("VSP set power failed\r\n");
          state = 1;
        }
        ra_dev_main_0.ops->set_power_en(ra_dev_main_0.dev, RA_POWER_VSP, 1);
        I2C_DEBUG("VSP set vol done\r\n");
        break;
      case 7:
        I2C_DEBUG("Setting VSNxb Voltage...\r\n");
        I2C_DEBUG("Target Vol:%f\r\n", set_power_frame.value.float_value[0]);
        temp = float_to_uint8_round(set_power_frame.value.float_value[0] / 100);
        // osDelay(10);
        status = ra_dev_main_0.ops->set_power_vol(ra_dev_main_0.dev, RA_POWER_VSN, temp);
        // 保存最新电压值
        g_calibration_manager.data.vsn_last_voltage = set_power_frame.value.float_value[0];
        calibration_save();
        if (status != BSP_OK)
        {
          I2C_DEBUG("VSN set power failed\r\n");
          state = 1;
        }
        ra_dev_main_0.ops->set_power_en(ra_dev_main_0.dev, RA_POWER_VSN, 1);
        I2C_DEBUG("VSN set vol done\r\n");
        break;
      default:
        I2C_ERROR("unknow power name\r\n");
        break;
      }
      tx_buf[2] = id;
      tx_buf[3] = state;
      break;
    case LIM_SET:
      if (rx_buf[2] == MCP4728_CHANNEL_A)
      {
        memcpy(&float_bytes, &rx_buf[3], sizeof(float_bytes));
        I2C_DEBUG("Setting ELVDD Current Limit...\r\n");
        I2C_DEBUG("Target Lim:%f\r\n", float_bytes.f);
        I2C_DEBUG("Latest ELVDD Current:%f\r\n", latest_sample_data[AD_I_ELVDD]);
        if (latest_sample_data[AD_I_ELVDD] > float_bytes.f)
        {
          ELVDD_DISABLE();
          I2C_DEBUG("ELVDD DISABLE\r\n");
        }
      }
      if (rx_buf[2] == MCP4728_CHANNEL_B)
      {
        memcpy(&float_bytes, &rx_buf[3], sizeof(float_bytes));
        I2C_DEBUG("Setting ELVSS Current Limit...\r\n");
        I2C_DEBUG("Target Lim:%f\r\n", float_bytes.f);
        I2C_DEBUG("Latest ELVSS Current:%f\r\n", latest_sample_data[AD_I_ELVSS]);
        if (latest_sample_data[AD_I_ELVSS] > float_bytes.f)
        {
          ELVSS_DISABLE();
          I2C_DEBUG("ELVSS DISABLE\r\n");
        }
      }
      if (rx_buf[2] == MCP4728_CHANNEL_C)
      {
        memcpy(&float_bytes, &rx_buf[3], sizeof(float_bytes));
        I2C_DEBUG("Setting VCC Current Limit...\r\n");
        I2C_DEBUG("Target Lim:%f\r\n", float_bytes.f);
        I2C_DEBUG("Latest VCC Current:%f\r\n", latest_sample_data[AD_I_VCC]);
        if (latest_sample_data[AD_I_VCC] > float_bytes.f)
        {
          VCC_DISABLE();
          I2C_DEBUG("VCC DISABLE\r\n");
        }
      }
      if (rx_buf[2] == MCP4728_CHANNEL_D)
      {
        memcpy(&float_bytes, &rx_buf[3], sizeof(float_bytes));
        I2C_DEBUG("Setting IOVCC Current Limit...\r\n");
        I2C_DEBUG("Target Lim:%f\r\n", float_bytes.f);
        I2C_DEBUG("Latest IOVCC Current:%f\r\n", latest_sample_data[AD_I_IOVCC]);
        if (latest_sample_data[AD_I_IOVCC] > float_bytes.f)
        {
          IOVCC_DISABLE();
          I2C_DEBUG("IOVCC DISABLE\r\n");
        }
      }
      if (rx_buf[2] == 4) // IOVCC
      {
        memcpy(&float_bytes, &rx_buf[3], sizeof(float_bytes));
        I2C_DEBUG("Setting RA XB IOVCC Current Limit...\r\n");
        I2C_DEBUG("Target Lim:%f\r\n", float_bytes.f);
        status = ra_dev_main_0.dev->ina3221->read(RA_INA3221_ADDRESS, 0x03, &temp_data);
        if (status == BSP_ERROR)
        {
          I2C_DEBUG("ra xb iovcc cur read error\r\n");
          state = BSP_ERROR;
        }
        else
        {
          temp_double_data = (temp_data >> 3) * 0.040 / (RA_INA3221_resistor);
          temp_float_data = abs((float)temp_double_data);
          I2C_DEBUG("Latest RA XB IOVCC Current:%f\r\n", temp_float_data);
          if (temp_float_data > float_bytes.f)
          {
            ra_dev_main_0.dev->lp3907->write(RA_LP3907_1_ADDRESS, 0x10, 0x25);
            I2C_DEBUG("RA XB IOVCC DISABLE\r\n");
          }
        }
      }
      if (rx_buf[2] == 5) // VCi
      {
        memcpy(&float_bytes, &rx_buf[3], sizeof(float_bytes));
        I2C_DEBUG("Setting RA XB VCI Current Limit...\r\n");
        I2C_DEBUG("Target Lim:%f\r\n", float_bytes.f);
        status = ra_dev_main_0.dev->ina3221->read(RA_INA3221_ADDRESS, 0x01, &temp_data);
        if (status == BSP_ERROR)
        {
          I2C_DEBUG("ra xb vci cur read error\r\n");
          state = BSP_ERROR;
        }
        else
        {
          temp_double_data = (temp_data >> 3) * 0.040 / (RA_INA3221_resistor);
          temp_float_data = (float)temp_double_data;
          I2C_DEBUG("Latest RA XB VCI Current:%f\r\n", temp_float_data);
          if (temp_float_data > float_bytes.f)
          {
            ra_dev_main_0.dev->lp3907->write(RA_LP3907_1_ADDRESS, 0x10, 0x25);
            I2C_DEBUG("RA XB VCI DISABLE\r\n");
          }
        }
      }
      if (rx_buf[2] == 6) // VSP
      {
        memcpy(&float_bytes, &rx_buf[3], sizeof(float_bytes));
        I2C_DEBUG("Setting RA XB VSP Current Limit...\r\n");
        I2C_DEBUG("Target Lim:%f\r\n", float_bytes.f);
        status = ra_dev_main_0.dev->ina3221->read(RA_INA3221_ADDRESS, 0x05, &temp_data);
        if (status == BSP_ERROR)
        {
          I2C_DEBUG("ra xb vsp cur read error\r\n");
          state = BSP_ERROR;
        }
        else
        {
          temp_double_data = (temp_data >> 3) * 0.040 / (RA_INA3221_resistor);
          temp_float_data = (float)temp_double_data;
          I2C_DEBUG("Latest RA XB VSP Current:%f\r\n", temp_float_data);
          if (temp_float_data > float_bytes.f)
          {
            ra_dev_main_0.ops->set_power_en(ra_dev_main_0.dev, RA_POWER_VSP, 0);
            I2C_DEBUG("RA XB VSP DISABLE\r\n");
          }
        }
      }
      if (rx_buf[2] == 7) // VSN
      {
        memcpy(&float_bytes, &rx_buf[3], sizeof(float_bytes));
        I2C_DEBUG("Setting RA XB VSN Current Limit...\r\n");
        I2C_DEBUG("Target Lim:%f\r\n", float_bytes.f);
        status = ra_dev_main_0.dev->adc121c027->read(RA_ADC121C027_ADDRESS, 0x00, &temp_data);
        if (status == BSP_ERROR)
        {
          I2C_DEBUG("ra xb vsn cur read error\r\n");
          state = BSP_ERROR;
        }
        else
        {
          temp_double_data = (((uint16_t)temp_data) & 0x0fff) * 8.05664 / (RA_ADC121_resistor) / 100;
          temp_float_data = (float)temp_double_data;
          I2C_DEBUG("Latest RA XB VSN Current:%f\r\n", temp_float_data);
          if (temp_float_data > float_bytes.f)
          {
            ra_dev_main_0.ops->set_power_en(ra_dev_main_0.dev, RA_POWER_VSN, 0);
            I2C_DEBUG("RA XB VSN DISABLE\r\n");
          }
        }
      }
      tx_buf[2] = id;
      tx_buf[3] = state;
      break;
    case ALL_POWER_EN:
      // 01使能，0不使能

      if (rx_buf[2] == 0x01)
        ELVDD_ENABLE();
      else if (rx_buf[2] == 0x0)
        ELVDD_DISABLE();
      if (rx_buf[3] == 0x01)
        ELVSS_ENABLE();
      else if (rx_buf[3] == 0x0)
        ELVSS_DISABLE();
      if (rx_buf[4] == 0x01)
        VCC_ENABLE();
      else if (rx_buf[4] == 0x0)
        VCC_DISABLE();
      if (rx_buf[5] == 0x01)
        IOVCC_ENABLE();
      else if (rx_buf[5] == 0x0)
        IOVCC_DISABLE();
      if (rx_buf[6] == 0x1) // IOVCC
      {
        HAL_I2C_Mem_Read(&hi2c1, RA_LP3907_1_ADDRESS, 0x10, I2C_MEMADD_SIZE_8BIT, &temp, 1, 100);
        I2C_DEBUG("IOVCC temp:%x\r\n", temp);
        ra_dev_main_0.dev->lp3907->write(RA_LP3907_1_ADDRESS, 0x10, temp | (1 << 4));
        I2C_DEBUG("IOVCC temp:%x\r\n", temp);
        // ra_dev_main_0.dev->lp3907->write(RA_LP3907_2_ADDRESS,0x10,25);
      }
      else if (rx_buf[6] == 0x0)
      {
        HAL_I2C_Mem_Read(&hi2c1, RA_LP3907_1_ADDRESS, 0x10, I2C_MEMADD_SIZE_8BIT, &temp, 1, 100);
        I2C_DEBUG("temp:%x\r\n", temp);
        ra_dev_main_0.dev->lp3907->write(RA_LP3907_1_ADDRESS, 0x10, temp & (~(1 << 4)));
        // 打印寄存器值
        I2C_DEBUG("temp:%x\r\n", temp);
      }
      if (rx_buf[7] == 0x1) // VCI
      {
        HAL_I2C_Mem_Read(&hi2c1, RA_LP3907_1_ADDRESS, 0x10, I2C_MEMADD_SIZE_8BIT, &temp, 1, 100);
        I2C_DEBUG("temp:%x\r\n", temp);
        ra_dev_main_0.dev->lp3907->write(RA_LP3907_1_ADDRESS, 0x10, temp | (1 << 6));
        I2C_DEBUG("temp:%x\r\n", temp);
      }
      else if (rx_buf[7] == 0x0)
      {
        HAL_I2C_Mem_Read(&hi2c1, RA_LP3907_1_ADDRESS, 0x10, I2C_MEMADD_SIZE_8BIT, &temp, 1, 100);
        printf("temp:%x\r\n", temp);
        ra_dev_main_0.dev->lp3907->write(RA_LP3907_1_ADDRESS, 0x10, temp & (~(1 << 6)));
        printf("temp:%x\r\n", temp);
      }
      if (rx_buf[8] == 0x1) // VSP
      {
        ra_dev_main_0.ops->set_power_en(ra_dev_main_0.dev, RA_POWER_VSP, 1);
      }
      else if (rx_buf[8] == 0x0)
        ra_dev_main_0.ops->set_power_en(ra_dev_main_0.dev, RA_POWER_VSP, 0);
      // VSN
      if (rx_buf[9] == 0x1)
        ra_dev_main_0.ops->set_power_en(ra_dev_main_0.dev, RA_POWER_VSN, 1);
      else if (rx_buf[9] == 0x0)
        ra_dev_main_0.ops->set_power_en(ra_dev_main_0.dev, RA_POWER_VSN, 0);
      tx_buf[2] = id;
      tx_buf[3] = BSP_OK;
      break;
    case SINGLE_POWER_EN:
      if (rx_buf[2] == MCP4728_CHANNEL_A)
      {
        if (rx_buf[3])
        {
          ELVDD_ENABLE();
          I2C_DEBUG("ELVDD ENABLE\r\n");
        }
        else
        {
          ELVDD_DISABLE();
          I2C_DEBUG("ELVDD DISABLE\r\n");
        }
      }
      if (rx_buf[2] == MCP4728_CHANNEL_B)
      {
        if (rx_buf[3])
        {
          ELVSS_ENABLE();
          I2C_DEBUG("ELVSS ENABLE\r\n");
        }
        else
        {
          ELVSS_DISABLE();
          I2C_DEBUG("ELVSS DISABLE\r\n");
        }
      }
      if (rx_buf[2] == MCP4728_CHANNEL_C)
      {
        if (rx_buf[3])
          VCC_ENABLE();
        else
          VCC_DISABLE();
      }
      if (rx_buf[2] == MCP4728_CHANNEL_D)
      {
        if (rx_buf[3])
          IOVCC_ENABLE();
        else
          IOVCC_DISABLE();
      }
      if (rx_buf[2] == 4) // RA XB IOVCC
      {
        if (rx_buf[3])
        {
          // HAL_I2C_Mem_Read(&hi2c1, RA_LP3907_1_ADDRESS, 0x10, I2C_MEMADD_SIZE_8BIT, &temp, 1, 100);
          // printf("temp:%x\r\n",temp);
          // ra_dev_main_0.dev->lp3907->write(RA_LP3907_1_ADDRESS,0x10,temp|(1<<4));
          // printf("temp:%x\r\n",temp);
          ra_dev_main_0.dev->lp3907->write(RA_LP3907_1_ADDRESS, 0x10, 0x75);
        }
        else
        {
          // HAL_I2C_Mem_Read(&hi2c1, RA_LP3907_1_ADDRESS, 0x10, I2C_MEMADD_SIZE_8BIT, &temp, 1, 100);
          // printf("temp:%x\r\n",temp);
          // ra_dev_main_0.dev->lp3907->write(RA_LP3907_1_ADDRESS,0x10,temp&(~(1<<4)));
          // printf("temp:%x\r\n",temp);
          ra_dev_main_0.dev->lp3907->write(RA_LP3907_1_ADDRESS, 0x10, 0x25);
        }
      }
      if (rx_buf[2] == 5) // VCI
      {
        if (rx_buf[3])
        {
          // ra_dev_main_0.dev->lp3907->read(RA_LP3907_1_ADDRESS,0x10,&temp);
          // printf("temp:%x\r\n",temp);
          // ra_dev_main_0.dev->lp3907->write(RA_LP3907_1_ADDRESS,0x10,temp|(1<<6));
          // printf("temp:%x\r\n",temp);
          ra_dev_main_0.dev->lp3907->write(RA_LP3907_1_ADDRESS, 0x10, 0x75);
        }
        else
        {
          // ra_dev_main_0.dev->lp3907->read(RA_LP3907_1_ADDRESS,0x10,&temp);
          // printf("temp:%x\r\n",temp);
          // ra_dev_main_0.dev->lp3907->write(RA_LP3907_1_ADDRESS,0x10,temp&(~(1<<6)));
          // printf("temp:%x\r\n",temp);
          ra_dev_main_0.dev->lp3907->write(RA_LP3907_1_ADDRESS, 0x10, 0x25);
        }
      }
      if (rx_buf[2] == 6) // VSP
      {
        if (rx_buf[3])
          ra_dev_main_0.ops->set_power_en(ra_dev_main_0.dev, RA_POWER_VSP, 1);
        else
          ra_dev_main_0.ops->set_power_en(ra_dev_main_0.dev, RA_POWER_VSP, 0);
      }
      if (rx_buf[2] == 7) // VSN
      {
        if (rx_buf[3])
          ra_dev_main_0.ops->set_power_en(ra_dev_main_0.dev, RA_POWER_VSN, 1);
        else
          ra_dev_main_0.ops->set_power_en(ra_dev_main_0.dev, RA_POWER_VSN, 0);
      }
      tx_buf[2] = id;
      tx_buf[3] = BSP_OK;
      break;
    case SINGLE_VOL_GET:
      if (rx_buf[2] == MCP4728_CHANNEL_A)
      {
        memcpy(&tx_buf[3], &latest_sample_data[AD_V_ELVDD], 4);
      }
      if (rx_buf[2] == MCP4728_CHANNEL_B)
      {
        memcpy(&tx_buf[3], &latest_sample_data[AD_V_ELVSS], 4);
      }
      if (rx_buf[2] == MCP4728_CHANNEL_C)
      {
        memcpy(&tx_buf[3], &latest_sample_data[AD_V_VCC], 4);
      }
      if (rx_buf[2] == MCP4728_CHANNEL_D)
      {
        memcpy(&tx_buf[3], &latest_sample_data[AD_V_IOVCC], 4);
      }
      if (rx_buf[2] == 4) // IOVCC
      {
        status = ra_dev_main_0.dev->ina3221->read(RA_INA3221_ADDRESS, 0x04, &temp_data);
        if (status == BSP_ERROR)
        {
          I2C_DEBUG("ina3221 read error\r\n");
        }
        else
        {
          temp_float_data = (float)temp_data;
          memcpy(&tx_buf[3], &temp_float_data, 4);
        }
      }
      if (rx_buf[2] == 5) // VCI
      {
        status = ra_dev_main_0.dev->ina3221->read(RA_INA3221_ADDRESS, 0x02, &temp_data);
        if (status == BSP_ERROR)
        {
          state = 1;
        }
        else
        {
          temp_float_data = (float)temp_data;
          memcpy(&tx_buf[3], &temp_float_data, 4);
        }
      }
      if (rx_buf[2] == 6) // VSP
      {
        status = ra_dev_main_0.dev->ina3221->read(RA_INA3221_ADDRESS, 0x06, &temp_data);
        if (status == BSP_ERROR)
        {
          state = 1;
        }
        else
        {
          temp_float_data = (float)temp_data;
          memcpy(&tx_buf[3], &temp_float_data, 4);
        }
      }
      if (rx_buf[2] == 7) // VSN
      {
      }
      tx_buf[2] = id;
      break;
    case SINGLE_CUR_GET:
      if (rx_buf[2] == MCP4728_CHANNEL_A)
      {
        memcpy(&tx_buf[3], &latest_sample_data[AD_I_ELVDD], 4);
        if (latest_sample_data[AD_I_ELVDD] == 0.0f)
        {
          printf("AD_I_ELVDD cur too low,%f\r\n", latest_sample_data[AD_I_ELVDD]);
        }
      }
      if (rx_buf[2] == MCP4728_CHANNEL_B)
      {
        memcpy(&tx_buf[3], &latest_sample_data[AD_I_ELVSS], 4);
        if (latest_sample_data[AD_I_ELVSS] == 0.0f)
        {
          printf("AD_I_ELVSS cur too low,%f\r\n", latest_sample_data[AD_I_ELVSS]);
        }
      }
      if (rx_buf[2] == MCP4728_CHANNEL_C)
      {
        memcpy(&tx_buf[3], &latest_sample_data[AD_I_VCC], 4);
        if (latest_sample_data[AD_I_VCC] == 0.0f)
        {
          printf("AD_I_VCC cur too low,%f\r\n", latest_sample_data[AD_I_VCC]);
        }
      }
      if (rx_buf[2] == MCP4728_CHANNEL_D)
      {
        memcpy(&tx_buf[3], &latest_sample_data[AD_I_IOVCC], 4);
        if (latest_sample_data[AD_I_IOVCC] == 0.0f)
        {
          printf("AD_I_IOVCC cur too low,%f\r\n", latest_sample_data[AD_I_IOVCC]);
        }
      }
      if (rx_buf[2] == 4) // IOVCC
      {
        status = ra_dev_main_0.dev->ina3221->read(RA_INA3221_ADDRESS, 0x03, &temp_data);
        // printf("ra vsp read status %d \r\n",status);
        if (status == BSP_ERROR)
        {
          state = BSP_ERROR;
        }
        else
        {
          temp_double_data = (temp_data >> 3) * 0.040 / (RA_INA3221_resistor);
          temp_float_data = abs((float)temp_double_data);
          memcpy(&tx_buf[3], &temp_float_data, 4);
          if (temp_float_data < 2.0f)
          {
            // printf("RA XB IOVCC cur too low,%f\r\n",temp_float_data);
          }
        }
      }
      if (rx_buf[2] == 5) // VCi
      {
        status = ra_dev_main_0.dev->ina3221->read(RA_INA3221_ADDRESS, 0x01, &temp_data);
        // printf("ra vsp read status %d \r\n",status);
        if (status == BSP_ERROR)
        {
          state = BSP_ERROR;
        }
        else
        {
          temp_double_data = (temp_data >> 3) * 0.040 / (RA_INA3221_resistor);
          temp_float_data = (float)temp_double_data;
          memcpy(&tx_buf[3], &temp_float_data, 4);
          if (temp_float_data < 2.0f)
          {
            // printf("RA XB VCI cur too low,%f\r\n",temp_float_data);
          }
        }
      }
      if (rx_buf[2] == 6) // VSP
      {
        status = ra_dev_main_0.dev->ina3221->read(RA_INA3221_ADDRESS, 0x05, &temp_data);
        // printf("ra vsp read status %d \r\n",status);
        if (status == BSP_ERROR)
        {
          state = BSP_ERROR;
        }
        else
        {
          temp_double_data = (temp_data >> 3) * 0.040 / (RA_INA3221_resistor);
          temp_float_data = (float)temp_double_data;
          memcpy(&tx_buf[3], &temp_float_data, 4);
          if (temp_float_data < 2.0f)
          {
            // printf("RA XB VSP cur too low,%f\r\n",temp_float_data);
          }
        }
      }
      if (rx_buf[2] == 7) // VSN
      {
        status = ra_dev_main_0.dev->adc121c027->read(RA_ADC121C027_ADDRESS, 0x00, &temp_data);
        if (status == BSP_ERROR)
        {
          state = BSP_ERROR;
        }
        else
        {
          temp_double_data = (((uint16_t)temp_data) & 0x0fff) * 8.05664 / (RA_ADC121_resistor) / 100;
          temp_float_data = (float)temp_double_data;
          memcpy(&tx_buf[3], &temp_float_data, 4);
          if (temp_float_data < 2.0f)
          {
            // printf("RA XB VSN cur too low,%f\r\n",temp_float_data);
          }
        }
      }
      tx_buf[2] = id;
      break;
    default:
      I2C_DEBUG("unknow command\r\n");
      break;
    }
    // 打印发送的数据
#ifdef I2C_DEBUG_ENABLE
    for (uint32_t i = 0; i < sizeof(tx_buf); i++)
    {
      printf("%02X ", tx_buf[i]);
      if ((i + 1) % 16 == 0)
      {
        printf("\r\n");
      }
    }
#endif
#endif
    osDelay(100);
  }
}

void SlaveTxTask(void *argument)
{
  for (;;)
  {
    osDelay(100);
  }
}

void MasterRxTask(void *argument)
{
  for (;;)
  {
    osDelay(100);
  }
}

void MasterTxTask(void *argument)
{
  OLED_Init();
  OLED_Clear();
  for (;;)
  {
#if 0
    HAL_NVIC_DisableIRQ(EXTI3_IRQn); // Prevent data contention with sampling interruptions
    for (uint8_t i = 0; i < 8; i++)
    {

      latest_sample_raw_data[i] = raw_data_queue_get_data(raw_data_queue_head - 1 - i);
      latest_sample_index[i] = raw_data_queue_get_index(raw_data_queue_head - 1 - i);
    }
    HAL_NVIC_EnableIRQ(EXTI3_IRQn);

    // 从latest_sample_index[i]获取通道号
    for (uint8_t i = 0; i < 8; i++)
    {
      channel_num = latest_sample_index[i];
      if (channel_num < 4)
      {
        sel_cali_param((channel_num & 0x07), 1, 1, &offset, &gain);
        IV_data = latest_sample_raw_data[i] * 500;
        if (channel_num == AD_I_ELVSS)
        {
          IV_data = -IV_data; // ELVSS电流为负值
        }
        IV_data = gain * IV_data + offset;
      }
      else if (3 < channel_num && channel_num < 8)
      {
        // sel_cali_param((channel_num & 0x07), 1, 0, &offset, &gain);
        IV_data = latest_sample_raw_data[i] * 1e3;
        if (channel_num == AD_V_ELVSS)
        {
          IV_data = -IV_data; // ELVSS电压为负值
        }
        if (channel_num == AD_V_ELVDD)
        {
          IV_data = IV_data * 2.5; // ELVDD电压为2.5倍
        }
        // IV_data = gain*IV_data + offset;
      }
      latest_sample_data[channel_num] = IV_data;
    }
    AD_DATA_INFO("Latest sampled data (after calibration):\r\n");
    for (uint8_t i = 0; i < 8; i++)
    {
      AD_DATA_DEBUG("-----Channel %d: %f\r\n", latest_sample_index[i], latest_sample_data[i]);
    }

    // Moster mode and listening are mutually exclusive
    HAL_I2C_DisableListen_IT(&hi2c1);

    OLED_ShowString(0, 0, "ELVSS", 12, 0);
    OLED_Showdecimal(32, 0, latest_sample_data[AD_I_ELVSS], 3, 3, 12, 0);
    OLED_Showdecimal(78, 0, latest_sample_data[AD_V_ELVSS], 4, 2, 12, 0);
    OLED_ShowString(0, 10, "IOVCC", 12, 0);
    OLED_Showdecimal(32, 10, latest_sample_data[AD_I_IOVCC], 3, 3, 12, 0);
    OLED_Showdecimal(78, 10, latest_sample_data[AD_V_IOVCC], 4, 2, 12, 0);

    // if(latest_sample_data[AD_I_IOVCC]>190)
    // {
    //   IOVCC_DISABLE();
    //   ELVSS_DISABLE();
    //   ELVDD_DISABLE();
    //   VCC_DISABLE();
    //   while(1);
    // }

    OLED_ShowString(0, 12, "ELVDD", 12, 0);
    if (HAL_GPIO_ReadPin(ELVDD_EN_GPIO_Port, ELVDD_EN_Pin) == GPIO_PIN_RESET)
    {
      OLED_Showdecimal(32, 12, 0, 3, 3, 12, 0);
      OLED_Showdecimal(78, 12, 0, 4, 2, 12, 0);
    }
    else
    {
      OLED_Showdecimal(32, 12, latest_sample_data[AD_I_ELVDD], 3, 3, 12, 0);
      OLED_Showdecimal(78, 12, latest_sample_data[AD_V_ELVDD], 4, 2, 12, 0);
    }

    OLED_ShowString(0, 14, "VCC", 12, 0);
    OLED_Showdecimal(32, 14, latest_sample_data[AD_I_VCC], 3, 3, 12, 0);
    OLED_Showdecimal(78, 14, latest_sample_data[AD_V_VCC], 4, 2, 12, 0);
    // OLED_Showdecimal(0,6,num2,2,3,12, 0);
    // OLED_HorizontalShift(0x26);//全屏水平向右滚动播放
    HAL_I2C_EnableListen_IT(&hi2c1);
#endif
    osDelay(100);
  }
}