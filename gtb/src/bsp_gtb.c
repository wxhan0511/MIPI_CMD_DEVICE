#include "bsp_gtb.h"
#include "bsp_i2c_bus.h"
#include "main.h"
#include "tp_define.h"
uint8_t SPI_Buffer_Rx_tmp[MAXI2CSPIBUFFERSIZE] = {0};
volatile int spi_rx_tx_flag = 0;
volatile int spi_rx_flag = 0;
volatile int spi_tx_flag = 0;
SPI_HandleTypeDef hspi_tp;
extern I2C_HandleTypeDef hi2c2;
void  tp_spi_set_mode(uint8_t mode){
    switch (mode) {
        case 0:
            hspi_tp.Init.CLKPhase          = SPI_PHASE_1EDGE;
            hspi_tp.Init.CLKPolarity       = SPI_POLARITY_LOW;
            break;
        case 1:
            hspi_tp.Init.CLKPhase          = SPI_PHASE_2EDGE;
            hspi_tp.Init.CLKPolarity       = SPI_POLARITY_LOW;
            break;
        case 2:
            hspi_tp.Init.CLKPhase          = SPI_PHASE_1EDGE;
            hspi_tp.Init.CLKPolarity       = SPI_POLARITY_HIGH;
            break;
        default:
            hspi_tp.Init.CLKPhase          = SPI_PHASE_2EDGE;
            hspi_tp.Init.CLKPolarity       = SPI_POLARITY_HIGH;
            break;
    }
    if (HAL_SPI_Init(&hspi_tp) != HAL_OK)
    {
        /* Initialization Error */
        GTB_DEBUG("HAL_SPI_Init failed!\r\n");
    }
}

void tp_spi_cs_enable(bool state)
{
    if(state == true){
        HAL_GPIO_WritePin(TSPI_CS_GPIO_Port, TSPI_CS_Pin, 0);
        GTB_DEBUG("CS Low\r\n");
    }
    else{
        HAL_GPIO_WritePin(TSPI_CS_GPIO_Port, TSPI_CS_Pin, 1);
        GTB_DEBUG("CS High\r\n");
        
    }
}
void bsp_gtb_init(uint8_t mode){

    /* Set the SPI parameters */
    hspi_tp.Instance               = SPI2;
    hspi_tp.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_256;
    hspi_tp.Init.Direction         = SPI_DIRECTION_2LINES;
    hspi_tp.Init.CLKPhase          = SPI_PHASE_1EDGE;
    hspi_tp.Init.CLKPolarity       = SPI_POLARITY_LOW;
    hspi_tp.Init.DataSize          = SPI_DATASIZE_8BIT;
    hspi_tp.Init.FirstBit          = SPI_FIRSTBIT_MSB;
    hspi_tp.Init.TIMode            = SPI_TIMODE_DISABLE;
    hspi_tp.Init.CRCCalculation    = SPI_CRCCALCULATION_DISABLE;
    hspi_tp.Init.CRCPolynomial     = 7;
    hspi_tp.Init.NSS               = SPI_NSS_SOFT;
    hspi_tp.Init.Mode 			 = SPI_MODE_MASTER;
    tp_spi_set_mode(mode);

    tp_spi_cs_enable(false);
   
}


void ex_ti_initial(tp_config_t *tp_config,FunctionalState state){
    //prevent error int when re-enable exti
    if(state == ENABLE)
    {
        if(tp_config->ex_ti_flag == true)
            return;
        tp_config->ex_ti_flag = true;
        HAL_NVIC_SetPriority(EXTI9_5_IRQn, 0, 0);
        HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);
    }
    else if(state == DISABLE){
//        if (tp_config->ex_ti_flag == false)
//            return;

        HAL_NVIC_DisableIRQ(EXTI9_5_IRQn);
        tp_config->ex_ti_flag = false;
    }
}


HAL_StatusTypeDef spi_read_write_data1( uint8_t *write_data, uint8_t *read_data,
                                        uint16_t read_size,uint16_t write_size)
{
    HAL_StatusTypeDef status = HAL_OK;
    status = HAL_SPI_TransmitReceive(&hspi_tp,write_data,read_data,write_size+read_size,1000);
    if (status != HAL_OK)
        printf("[error] gtb read write %d \r\n",status);
    return status;
}

HAL_StatusTypeDef spi_read_write_data2( uint8_t *write_data, uint8_t *read_data,
                                        uint16_t read_size,uint16_t write_size)
{
    HAL_StatusTypeDef status = HAL_OK;
    //status = HAL_SPI_TransmitReceive_IT(&hspi_tp,write_data,read_data,write_size+read_size);
    status = HAL_SPI_TransmitReceive(&hspi_tp,write_data,read_data,write_size+read_size,10);
    
    GTB_DEBUG("spi_read_write_data2 status(HAL_OK:0): %d\r\n",status);
    if (status != HAL_OK) GTB_INFO("[error] gtb read write irq %d \r\n",status);
    else spi_rx_tx_flag=1;
    while(spi_rx_tx_flag == 0) 
    {
        GTB_INFO("[wait] spi_rx_tx_flag == 0\r\n");
    }
    spi_rx_tx_flag = 0;
    return status;
}


HAL_StatusTypeDef i2c_write_data(uint8_t slave_address,uint8_t *write_data,uint16_t write_size)
{
    HAL_StatusTypeDef status = HAL_OK;
    status = HAL_I2C_Master_Transmit(&hi2c2,slave_address<<1,write_data,write_size,1000);
    if (status != HAL_OK)
    {
        printf("i2c_write_data error %d\r\n",status);
    }
    return status;
}

HAL_StatusTypeDef i2c_read_data(uint8_t slave_address,uint8_t *read_data,uint16_t read_size)
{
    HAL_StatusTypeDef status = HAL_OK;
    status = HAL_I2C_Master_Receive(&hi2c2,slave_address<<1,read_data,read_size,1000);
    if (status != HAL_OK)
    {
        printf("i2c_read_data error %d\r\n",status);
    }
    return status;
}

HAL_StatusTypeDef spi_read_data2( uint8_t *read_data,
                                        uint16_t read_size)
{

    HAL_StatusTypeDef status = HAL_OK;
    status = HAL_SPI_Receive(&hspi_tp,read_data,read_size,1000);
    return status;
}

HAL_StatusTypeDef spi_read_write_data3( uint8_t *write_data, uint8_t *read_data,
                                        uint16_t read_size,uint16_t write_size)
{

    HAL_StatusTypeDef status = HAL_OK;
    status = HAL_SPI_Receive(&hspi_tp,read_data,write_size+read_size,100);
    if (status == HAL_OK)
        spi_rx_flag = 1;
    while(spi_rx_flag == 0) {}
    spi_rx_flag = 0;
    return status;
}

HAL_StatusTypeDef tp_spi_read_write_byte(uint8_t byte){
    HAL_StatusTypeDef status = HAL_OK;

    //status = HAL_SPI_TransmitReceive(&hspi1,write_data,read_data,write_size+read_size,1000);
    status = HAL_SPI_TransmitReceive_DMA(&hspi_tp,&byte,NULL,1);
    while(spi_rx_tx_flag == 0) {}
    spi_rx_tx_flag = 0;
    return status;

}

HAL_StatusTypeDef gtb_read_data(tp_config_t *tp_config,bool mode,uint8_t *read_cmd,uint8_t *read_data,uint32_t data_len)
{
    HAL_StatusTypeDef status = HAL_OK;
    if(mode == I2C_MODE)
    {
        //printf("gtb i2c read\r\n");
        tp_config->transfer_flag = true;
        //status = I2C1_ReadData(pData,DataLength);
        i2c_read_data(tp_config->i2c_slave_address,read_data,data_len);
        tp_config->transfer_flag = false;
    }
    else
    {
        // printf("    [gtb read] ");
        // for (uint8_t i=0;i<data_len;i++)
        //     printf("0x%x ",read_cmd[i]);
        // printf("\r\n");
        if (tp_config->cs_high_en == true)
        {
            tp_config->transfer_flag = true;
            tp_spi_cs_enable(true);
        }
        //delay_us(10);
        status = spi_read_write_data2(read_cmd,read_data,data_len,0);
        if (tp_config->cs_low_en == true)
        {
            tp_spi_cs_enable(false);
            tp_config->transfer_flag = false;
        }
        // printf("    [gtb read data] ");
        // for (uint8_t i=0;i<data_len;i++)
        //     printf("0x%x ",read_data[i]);
        // printf("\r\n");
    }

    return status;
}

void tp_i2c_set_speed(uint8_t speed){

    //hi2c.Instance = I2C1;
    if(I2C_SPEED_100K == speed){
        hi2c2.Init.ClockSpeed = 100000;
    }
    else{
        hi2c2.Init.ClockSpeed = 400000;
    }

    hi2c2.Init.OwnAddress1 = 0;
    hi2c2.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c2.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c2.Init.OwnAddress2 = 0;
    hi2c2.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c2.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
    if (HAL_I2C_Init(&hi2c2) != HAL_OK)
    {
        //Error_Handler(__FILE__,__LINE__);
    }
}

void tp_spi_set_speed(uint8_t speed){

//    if (HAL_SPI_DeInit(&hspi_tp) != HAL_OK)
//    {
//        /* Initialization Error */
//        Error_Handler(__FILE__,__LINE__);
//    }

    if((speed == 0) || (speed == 1)){
        hspi_tp.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;
    }
    else if((speed == 2) || (speed == 3)){
        hspi_tp.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_32;
    }
    else if((speed == 4) || (speed == 5)){
        hspi_tp.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_64;
    }
    else if((speed == 6) || (speed == 7)){
        hspi_tp.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_128;
    }

    if (HAL_SPI_Init(&hspi_tp) != HAL_OK)
    {
        /* Initialization Error */
        //Error_Handler(__FILE__,__LINE__);
    }
}

HAL_StatusTypeDef gtb_write_data(tp_config_t *tp_config,bool mode,uint8_t *pData,uint32_t data_len)
{
    HAL_StatusTypeDef status = HAL_OK;
    if(mode == I2C_MODE)
    {
        tp_config->transfer_flag = true;
        //status = I2C1_WriteData(pData,DataLength);
        i2c_write_data(tp_config->i2c_slave_address,pData,data_len);
        tp_config->transfer_flag = false;
    }
    else
    {

        GTB_DEBUG("    [gtb write] (%d)",data_len);
        // if (data_len != 56)
        // {
        //     for (uint8_t i=0;i<data_len;i++)
        //         GTB_DEBUG("0x%x ",pData[i]);
        // }
        //for (uint8_t i=0;i<data_len;i++)
        //{
        //    GTB_INFO("0x%x, ",pData[i]);
        //}
        //GTB_INFO("\r\n");
        //GTB_INFO("-----------------------------------------------------------------\r\n");
        if (tp_config->cs_high_en == true)
        {
            tp_config->transfer_flag = true;
            tp_spi_cs_enable(true);
            GTB_DEBUG("CS Enable\r\n");
        }
        if(data_len > MAXI2CSPIBUFFERSIZE)
            return HAL_ERROR;
        status = spi_read_write_data2(pData,SPI_Buffer_Rx_tmp,data_len,0);
        
        if (tp_config->cs_low_en == true)
        {
            tp_spi_cs_enable(false);
            tp_config->transfer_flag = false;
        }
    }

    return status;
}