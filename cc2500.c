/*
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

   author: fishpepper <AT> gmail.com
*/

#include "cc2500.h"
#include "spi.h"
#include "stm32f0xx_gpio.h"
#include "stm32f0xx_rcc.h"
#include "debug.h"
#include "timeout.h"
#include <string.h>

void cc2500_init(void){
    spi_init();
    cc2500_init_gpio();
}
static void cc2500_init_gpio(void) {
    GPIO_InitTypeDef gpio_init;
    GPIO_StructInit(&gpio_init);

    //PA/LNA:
    // periph clock enable for port
    RCC_AHBPeriphClockCmd(CC2500_LNA_SW_CTX_GPIO_CLK, ENABLE);
    RCC_AHBPeriphClockCmd(CC2500_LNA_SW_CRX_GPIO_CLK, ENABLE);

    //CTX:
    // set all gpio directions to output
    gpio_init.GPIO_Pin = CC2500_LNA_SW_CTX_PIN;
    gpio_init.GPIO_Mode  = GPIO_Mode_OUT;
    gpio_init.GPIO_OType = GPIO_OType_PP;
    gpio_init.GPIO_Speed = GPIO_Speed_50MHz;
    gpio_init.GPIO_PuPd  = GPIO_PuPd_NOPULL;
    GPIO_Init(CC2500_LNA_SW_CTX_GPIO, &gpio_init);
    //CRX:
    gpio_init.GPIO_Pin = CC2500_LNA_SW_CRX_PIN;
    GPIO_Init(CC2500_LNA_SW_CRX_GPIO, &gpio_init);

    cc2500_enter_rxmode();

    //JTAG IS ON LNA pins! -> DISABLE JTAG!
    //RCC_APBxPeriphClockCmd(2, RCC_APB2Periph_AFIO, ENABLE);
    //GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);

    //GDO2
    //periph clock enable for port
    RCC_AHBPeriphClockCmd(CC2500_GDO2_GPIO_CLK, ENABLE);

    // configure GDO2 pin as Input floating
    gpio_init.GPIO_Pin  = CC2500_GDO2_PIN;
    gpio_init.GPIO_Mode = GPIO_Mode_IN;
    GPIO_Init(CC2500_GDO2_GPIO, &gpio_init);
}

inline void cc2500_enter_rxmode(void) {
    //add pa/lna config bit setting here
    CC2500_LNA_SW_CRX_GPIO->BSRR = (CC2500_LNA_SW_CRX_PIN); //1
    delay_us(20);
    CC2500_LNA_SW_CTX_GPIO->BRR  = (CC2500_LNA_SW_CTX_PIN); //0
    delay_us(5);
}

#if 0
inline uint32_t cc2500_set_antenna(uint8_t id){
    //select antenna 0 or 1:
    if (id) {
        CC2500_ANT_SW_CTX_GPIO->BRR  = (CC2500_ANT_SW_CTX_PIN); //0
        CC2500_ANT_SW_CRX_GPIO->BSRR = (CC2500_ANT_SW_CRX_PIN); //1
    } else {
        CC2500_ANT_SW_CTX_GPIO->BSRR = (CC2500_ANT_SW_CTX_PIN); //1
        CC2500_ANT_SW_CRX_GPIO->BRR  = (CC2500_ANT_SW_CRX_PIN); //0
    }
    return id;
}
#endif

#if 0
inline void cc2500_set_gdo_mode(void) {
    CC2500_set_register(IOCFG0, 0x01); //6);
    //CC2500_set_register(IOCFG1, ???);
    CC2500_set_register(IOCFG2, 0x01); //6);
}

inline void cc2500_set_register(uint8_t address, uint8_t data){
    //select device
    hal_spi_csn_lo();

    //wait for ready signal
    while(GPIO_ReadInputDataBit(CC2500_SPI_GPIO, CC2500_SPI_MISO_PIN) == 1){}

    hal_spi_tx(address);
    hal_spi_tx(data);

    //deslect
    hal_spi_csn_hi();
}

inline uint8_t cc2500_get_register(uint8_t address){
    uint8_t result;

    //select device:
    hal_spi_csn_lo();

    //wait for RDY signal:
    while(GPIO_ReadInputDataBit(CC2500_SPI_GPIO, CC2500_SPI_MISO_PIN) == 1){}

    //request address (read request has bit7 set)
    uint8_t status = hal_spi_tx(address | 0x80);
    //debug_put_hex8(status);

    //fetch result:
    result = hal_spi_rx();

    //deselect device
    hal_spi_csn_hi();

    //return result
    return(result);
}

inline void cc2500_strobe(uint8_t address){
    hal_spi_csn_lo();
    uint8_t status = hal_spi_tx(address);
    //debug("s"); debug_put_hex8(status); debug_put_newline();
    hal_spi_csn_hi();
}

inline uint8_t cc2500_get_status(void) {
    hal_spi_csn_lo();
    uint8_t status = hal_spi_tx(0xFF);
    hal_spi_csn_hi();
    return status;
}

uint8_t cc2500_transmission_completed(void) {
    //after tx cc25xx goes back to RX (configured by mcsm1 register)
    return ((cc2500_get_status() & (0x70)) == CC2500_STATUS_STATE_RX);
}



inline void cc2500_enter_txmode(void) {
    //add pa/lna config bit setting here
    CC2500_LNA_SW_CRX_GPIO->BRR  = (CC2500_LNA_SW_CRX_PIN); //0
    delay_us(20);
    CC2500_LNA_SW_CTX_GPIO->BSRR = (CC2500_ANT_SW_CTX_PIN); //1
    delay_us(5);
}


inline void cc2500_enable_receive(void){
    //switch on rx again
    cc2500_enter_rxmode();
}


inline uint8_t cc2500_get_gdo_status(void) {
    if (GPIO_ReadInputDataBit(CC2500_GDO2_GPIO, GPIO_Pin_3)){
        return 1;
    }else{
        return 0;
    }
}

inline void cc2500_read_fifo(uint8_t *buf, uint8_t len){
    cc2500_register_read_multi(CC2500_FIFO | READ_FLAG | BURST_FLAG, buf, len);
}

inline void cc2500_register_read_multi(uint8_t address, uint8_t *buffer, uint8_t len){
    // select device:
    hal_spi_csn_lo();

    // wait for ready signal
    while(GPIO_ReadInputDataBit(CC2500_SPI_GPIO, CC2500_SPI_MISO_PIN) == 1){}

    //debug("read "); debug_put_uint8(len); debug_flush();
    // request address (read request)
    uint8_t status = hal_spi_tx(address);

    //fill buffer with read commands:
    memset(buffer, 0xFF, len);

    hal_spi_dma_xfer(buffer, len);
    /*
    while(len--){
        *buf = hal_spi_rx();
        buf++;
    }*/

    // deselect device
    hal_spi_csn_hi();
}

inline void cc2500_register_write_multi(uint8_t address, uint8_t *buffer, uint8_t len){
    //s elect device:
    hal_spi_csn_lo();

    // wait for RDY signal:
    while(GPIO_ReadInputDataBit(CC2500_SPI_GPIO, CC2500_SPI_MISO_PIN) == 1){}

    //request address (write request)
    hal_spi_tx(address | BURST_FLAG);

    // send array
    hal_spi_dma_xfer(buffer, len);

    //deselect device
    hal_spi_csn_hi();
}

inline void cc2500_process_packet(volatile uint8_t *packet_received, volatile uint8_t *buffer, uint8_t maxlen){
    if(cc2500_get_gdo_status() == 1){
        //data received, fetch data
        //timeout_set_100us(5);

        *packet_received = 0;

        //there is a bug in the cc2500
        //see p3 http://www.ti.com/lit/er/swrz002e/swrz002e.pdf
        //workaround: read len register very quickly twice:
        uint8_t len1, len2, len, i;

        //try this 10 times befor giving up:
        for (i=0; i<10; i++){
            len1 = cc2500_get_register_burst(RXBYTES) & 0x7F;
            len2 = cc2500_get_register_burst(RXBYTES) & 0x7F;
            if (len1==len2) break;
        }

        //valid len found?
        if (len1==len2){
            len = len1;

            //packet received, grab data
            uint8_t tmp_buffer[len];
            cc2500_read_fifo(tmp_buffer, len);

            //only accept valid packet lenbghts:
            if (len == maxlen){
                uint8_t i;
                for(i=0; i<maxlen; i++){
                    buffer[i] = tmp_buffer[i];
                }
                *packet_received = 1;
            }
        }else{
            //no, ignore this
            len = 0;
        }
    }
}

void cc2500_transmit_packet(volatile uint8_t *buffer, uint8_t len) {
    //flush tx fifo
    cc2500_strobe(RFST_SFTX);
    //copy to fifo
    cc2500_register_write_multi(CC2500_FIFO, (uint8_t *)buffer, buffer[0]+1);
    //and send!
    cc2500_strobe(RFST_STX);
}
/*
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

   author: fishpepper <AT> gmail.com
*/

#include "cc25xx.h"
#include "debug.h"
#include "timeout.h"

uint8_t CC2500_current_antenna;

void CC2500_init(void) {
    debug("cc25xx: init\n"); debug_flush();
    CC2500_current_antenna = 0;
    cc2500_init();
    cc2500_set_antenna(CC2500_current_antenna);
}

void CC2500_switch_antenna(void) {
    // switch to next antenna
    if (CC2500_current_antenna) {
        CC2500_current_antenna = cc2500_set_antenna(0);
    }else{
        CC2500_current_antenna = cc2500_set_antenna(1);
    }
}

void CC2500_wait_for_transmission_complete(void) {
    //after STX we go back to RX state (see MCSM1 register)
    //so wait a maximum of 9ms for completion
    timeout2_set_100us(90);

    while(!timeout2_timed_out()){
        if (cc2500_transmission_completed()){
            //done with tx, return
            return;
        }
    }

    //if we reach this point, tx timed out:
    debug("!TX");
}
#endif
