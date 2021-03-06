/*********************************************
 * vim:sw=8:ts=8:si:et
 * To use the above modeline in vim you must have "set modeline" in your .vimrc
 * Author: Guido Socher 
 * Copyright: GPL V2
 * http://www.gnu.org/licenses/gpl.html
 *
 * Based on the enc28j60.c file from the AVRlib library by Pascal Stang.
 * For AVRlib See http://www.procyonengineering.com/
 * Used with explicit permission of Pascal Stang.
 *
 * Title: Microchip ENC28J60 Ethernet Interface Driver
 * Chip type           : ATMEGA88 with ENC28J60
 *********************************************/

/*********************************************
 * modified: 2007-07-07
 * Author  : Hotislandn
 * Copyright: GPL V2
 * http://www.mcuzone.com
 * Host chip: AT91SAM7S
**********************************************/


/*********************************************
 * modified: 2007-08-08
 * Author  : awake
 * Copyright: GPL V2
 * http://www.icdev.com.cn/?2213/
 * Host chip: ADUC7026
**********************************************/




#define AT_ARM
#define AT91SAM7S64
#include "s64.h"
//#include <LPC214X.H>
//#include <ADuC7026.h>





#ifdef AT91SAM7S64
#define AT_ARM
#include "AT91SAM7S64.h"
#include "lib_AT91SAM7S64.h"
#endif

#ifdef AT91SAM7S32
#define AT_ARM
#include "AT91SAM7S32.h"
#include "lib_AT91SAM7S32.h"
#endif

#ifdef AT91SAM7S256
#define AT_ARM
#include "AT91SAM7S256.h"
#include "lib_AT91SAM7S256.h"
#endif

#ifdef AT_ARM
#include "enc28j60.h"
//#include "SAM7SDK_BSP.h"
#endif

#define uint8_t  unsigned char
#define uint16_t unsigned short

static uint8_t Enc28j60Bank;
static uint16_t NextPacketPtr;

#ifdef ATMEGA88
#define ENC28J60_CONTROL_PORT   PORTB
#define ENC28J60_CONTROL_DDR    DDRB
#define ENC28J60_CONTROL_CS     2
#define enc28j60CSinit()        ENC28J60_CONTROL_DDR |= (1 << ENC28J60_CONTROL_CS)
// set CS to 0 = active
#define CSACTIVE ENC28J60_CONTROL_PORT&=~(1<<ENC28J60_CONTROL_CS)
// set CS to 1 = passive
#define CSPASSIVE ENC28J60_CONTROL_PORT|=(1<<ENC28J60_CONTROL_CS)
//
#define waitspi() while(!(SPSR&(1<<SPIF)))

#define enc28j60SetSCK() 
#define enc28j60HWreset()
#endif

#ifdef AT_ARM
#define ENC_CS_IO      (unsigned int)(1 << 31)
#define ENC_RST_IO     (unsigned int)(1 << 15)
// use channel 0, for all the peripherals are controled by PIO, not SPI
#define ENC_SPI_CH     (0)

#define waitspi()



/*
  Send data to slave or receive data from slave
*/

/*
  Send data to slave or receive data from slave
*/
unsigned short SAMspiSend(unsigned char ch, unsigned short data)
{		
	//AT91C_BASE_PIOA->PIO_SODR=1<<2;
	AT91C_BASE_SPI->SPI_TDR=data&0x0000FFFF;
	while((*AT91C_SPI_SR&AT91C_SPI_TDRE)==0) {} 
	while((*AT91C_SPI_SR&AT91C_SPI_RDRF)==0) {;} 
	//AT91C_BASE_PIOA->PIO_CODR=1<<2;
	data=(*AT91C_SPI_RDR)&AT91C_SPI_RD;
	return data;
} 

/*
  initialize the contorl logic of /CS pin
*/
static void enc28j60CSinit(void)
{

}

/* 
  enable enc28j60, /CS = Low
*/
static void CSACTIVE(void)
{
	//AT91PS_PIO pPIOA = AT91C_BASE_PIOA;
	AT91C_BASE_PIOA->PIO_CODR=1<<15;

	//pPIOA->PIO_CODR = ENC_CS_IO; // clear
}

/* 
  disable enc28j60, /CS = High
*/
static void CSPASSIVE(void)
{
	//AT91PS_PIO pPIOA = AT91C_BASE_PIOA;
	AT91C_BASE_PIOA->PIO_SODR=1<<15;
	//pPIOA->PIO_SODR = ENC_CS_IO; // set
}

/*
  set ENC28J60 SPI SCK freq, MAX 10MHz
*/
static void enc28j60SetSCK(void)
{
//    SAMspiSetSCBR(ENC_SPI_CH, CPU_Freq / MAX_SCK_FREQ);
}

void delay_us(uint8_t us)
{
	uint8_t s;
	for(;us;us--)
	{
		for(s=3;s;s--)
		{
		}
	}
}
void delay_ms(uint8_t ms)
{
	for(;ms;ms--)
	{
		delay_us(250);
		delay_us(250);
		delay_us(250);
		delay_us(250);
	}
}

/*
  ENC28J60 HW reset
*/
static void enc28j60HWreset(void)
{
//    AT91PS_PIO pPIOA = AT91C_BASE_PIOA;

//	pPIOA->PIO_PER  = ENC_RST_IO; // enable PIO of CS-pin
//	pPIOA->PIO_SODR = ENC_RST_IO; // set
//	pPIOA->PIO_OER  = ENC_RST_IO; // output

//    pPIOA->PIO_CODR = ENC_RST_IO; // clear
//    delay_ms(10);
//    pPIOA->PIO_SODR = ENC_RST_IO; // set
//    delay_ms(200);

}
#endif

uint8_t enc28j60ReadOp(uint8_t op, uint8_t address)
{
        uint8_t dat = 0;
        CSACTIVE();

        dat = op | (address & ADDR_MASK);
        SAMspiSend(ENC_SPI_CH, dat);
        dat = SAMspiSend(ENC_SPI_CH, 0);
        // do dummy read if needed (for mac and mii, see datasheet page 29)
        if(address & 0x80)
        {
                dat = SAMspiSend(ENC_SPI_CH, 0);
        }
        // release CS
        CSPASSIVE();
        return dat;
}

void enc28j60WriteOp(uint8_t op, uint8_t address, uint8_t data)
{
        uint8_t dat = 0;
        CSACTIVE();
        // issue write command
        dat = op | (address & ADDR_MASK);
        SAMspiSend(ENC_SPI_CH, dat);
        // write data
        dat = data;
        SAMspiSend(ENC_SPI_CH, dat);
        CSPASSIVE();
}

void enc28j60ReadBuffer(uint16_t len, uint8_t* data)
{
        CSACTIVE();
        // issue read command
#ifdef ATMEGA88
        SPDR = ENC28J60_READ_BUF_MEM;
        waitspi();
#endif

#ifdef AT_ARM
        SAMspiSend(ENC_SPI_CH, ENC28J60_READ_BUF_MEM);
#endif
        while(len)
        {
                len--;
                // read data
#ifdef ATMEGA88
                SPDR = 0x00;
                waitspi();
                *data = SPDR;
#endif

#ifdef AT_ARM
                *data = (uint8_t)SAMspiSend(ENC_SPI_CH, 0);
#endif
                data++;
        }
        *data='\0';
        CSPASSIVE();
}

void enc28j60WriteBuffer(uint16_t len, uint8_t* data)
{
        CSACTIVE();
        // issue write command
#ifdef ATMEGA88
        SPDR = ENC28J60_WRITE_BUF_MEM;
        waitspi();
#endif

#ifdef AT_ARM
        SAMspiSend(ENC_SPI_CH, ENC28J60_WRITE_BUF_MEM);
#endif
        while(len)
        {
                len--;
                // write data
#ifdef ATMEGA88
                SPDR = *data;
                waitspi();
#endif

#ifdef AT_ARM
                SAMspiSend(ENC_SPI_CH, *data);
#endif
                data++;
        }
        CSPASSIVE();
}

void enc28j60SetBank(uint8_t address)
{
        // set the bank (if needed)
        if((address & BANK_MASK) != Enc28j60Bank)
        {
                // set the bank
                enc28j60WriteOp(ENC28J60_BIT_FIELD_CLR, ECON1, (ECON1_BSEL1|ECON1_BSEL0));
                enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, ECON1, (address & BANK_MASK)>>5);
                Enc28j60Bank = (address & BANK_MASK);
        }
}

uint8_t enc28j60Read(uint8_t address)
{
        // set the bank
        enc28j60SetBank(address);
        // do the read
        return enc28j60ReadOp(ENC28J60_READ_CTRL_REG, address);
}

void enc28j60Write(uint8_t address, uint8_t data)
{
        // set the bank
        enc28j60SetBank(address);
        // do the write
        enc28j60WriteOp(ENC28J60_WRITE_CTRL_REG, address, data);
}

void enc28j60PhyWrite(uint8_t address, uint16_t data)
{
        // set the PHY register address
        enc28j60Write(MIREGADR, address);
        // write the PHY data
        enc28j60Write(MIWRL, data);
        enc28j60Write(MIWRH, data>>8);
        // wait until the PHY write completes
        while(enc28j60Read(MISTAT) & MISTAT_BUSY){
                delay_us(15);
        }
}

void enc28j60clkout(uint8_t clk)
{
        //setup clkout: 2 is 12.5MHz:
	enc28j60Write(ECOCON, clk & 0x7);
}

void enc28j60Init(uint8_t* macaddr)
{
	AT91C_BASE_PIOA->PIO_PER=1<<15;
	AT91C_BASE_PIOA->PIO_OER=1<<15;
	//PMC_PCER=(AT91C_PA12_MISO|AT91C_PA13_MOSI|AT91C_PA14_SPCK|AT91C_PA11_NPCS0);
	* AT91C_PMC_PCER =0x20;
	//PIO外设功能时钟始能
	
	*AT91C_PIOA_PDR =(AT91C_PA12_MISO|AT91C_PA13_MOSI|AT91C_PA14_SPCK|AT91C_PA11_NPCS0);
	//PIO使能引脚的外设功能
	
	*AT91C_PIOA_ASR=(AT91C_PA12_MISO|AT91C_PA13_MOSI|AT91C_PA14_SPCK|AT91C_PA11_NPCS0);
	//外设A分配给SPI外设A功能
	
	
	
	*AT91C_SPI_CR=AT91C_SPI_SPIEN ;
	//允许SPI口
	*AT91C_SPI_MR=(AT91C_SPI_MSTR|AT91C_SPI_PS_FIXED|AT91C_SPI_MODFDIS|AT91C_SPI_DLYBCS);
	//主机模式,不分频,固定片选0,禁止错误检测
	*AT91C_SPI_CSR=(AT91C_SPI_NCPHA|AT91C_SPI_CSAAT|AT91C_SPI_BITS_8|(12<<8)|AT91C_SPI_DLYBS|AT91C_SPI_DLYBCT);
	//8位数据，传输完成后片选保持，48M/12分频，传输前延时255，连续传输延时255 AT91C_SPI_SCBR
	// initialize I/O
	enc28j60CSinit();
	CSPASSIVE();
	delay_ms(200);
	enc28j60clkout(2); // change clkout from 6.25MHz to 12.5MHz
    delay_ms(20);
    // 0x476 is PHLCON LEDA=links status, LEDB=receive/transmit
    enc28j60PhyWrite(PHLCON,0xd76);	//0x476	  
	delay_ms(20);

#ifdef ATMEGA88
        //	
	DDRB  |= 1<<PB2 |1<<PB3 | 1<<PB5; // mosi, sck, ss output
	cbi(DDRB,PINB4); // MISO is input
        //
	CSPASSIVE();
        cbi(PORTB,PB3); // MOSI low
        cbi(PORTB,PB5); // SCK low
	//
	// initialize SPI interface
	// master mode and Fosc/2 clock:
        SPCR = (1<<SPE)|(1<<MSTR);
        SPSR |= (1<<SPI2X);
#endif

    enc28j60SetSCK();
    enc28j60HWreset();
	// perform system reset
	enc28j60WriteOp(ENC28J60_SOFT_RESET, 0, ENC28J60_SOFT_RESET);
	delay_ms(250);
	// check CLKRDY bit to see if reset is complete
        // The CLKRDY does not work. See Rev. B4 Silicon Errata point. Just wait.
	//while(!(enc28j60Read(ESTAT) & ESTAT_CLKRDY));
	// do bank 0 stuff
	// initialize receive buffer
	// 16-bit transfers, must write low byte first
	// set receive buffer start address
	NextPacketPtr = RXSTART_INIT;
        // Rx start
	enc28j60Write(ERXSTL, RXSTART_INIT&0xFF);
	enc28j60Write(ERXSTH, RXSTART_INIT>>8);
	// set receive pointer address
	enc28j60Write(ERXRDPTL, RXSTART_INIT&0xFF);
	enc28j60Write(ERXRDPTH, RXSTART_INIT>>8);
	// RX end
	enc28j60Write(ERXNDL, RXSTOP_INIT&0xFF);
	enc28j60Write(ERXNDH, RXSTOP_INIT>>8);
	// TX start
	enc28j60Write(ETXSTL, TXSTART_INIT&0xFF);
	enc28j60Write(ETXSTH, TXSTART_INIT>>8);
	// TX end
	enc28j60Write(ETXNDL, TXSTOP_INIT&0xFF);
	enc28j60Write(ETXNDH, TXSTOP_INIT>>8);
	// do bank 1 stuff, packet filter:
        // For broadcast packets we allow only ARP packtets
        // All other packets should be unicast only for our mac (MAADR)
        //
        // The pattern to match on is therefore
        // Type     ETH.DST
        // ARP      BROADCAST
        // 06 08 -- ff ff ff ff ff ff -> ip checksum for theses bytes=f7f9
        // in binary these poitions are:11 0000 0011 1111
        // This is hex 303F->EPMM0=0x3f,EPMM1=0x30
	enc28j60Write(ERXFCON, ERXFCON_UCEN|ERXFCON_CRCEN|ERXFCON_PMEN);
	enc28j60Write(EPMM0, 0x3f);
	enc28j60Write(EPMM1, 0x30);
	enc28j60Write(EPMCSL, 0xf9);
	enc28j60Write(EPMCSH, 0xf7);
        //
        //
	// do bank 2 stuff
	// enable MAC receive
	enc28j60Write(MACON1, MACON1_MARXEN|MACON1_TXPAUS|MACON1_RXPAUS);
	// bring MAC out of reset
	enc28j60Write(MACON2, 0x00);
	// enable automatic padding to 60bytes and CRC operations
	enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, MACON3, MACON3_PADCFG0|MACON3_TXCRCEN|MACON3_FRMLNEN|MACON3_FULDPX);
	// set inter-frame gap (non-back-to-back)
	enc28j60Write(MAIPGL, 0x12);
	enc28j60Write(MAIPGH, 0x0C);
	// set inter-frame gap (back-to-back)
	enc28j60Write(MABBIPG, 0x12);
	// Set the maximum packet size which the controller will accept
        // Do not send packets longer than MAX_FRAMELEN:
	enc28j60Write(MAMXFLL, MAX_FRAMELEN&0xFF);	
	enc28j60Write(MAMXFLH, MAX_FRAMELEN>>8);
	// do bank 3 stuff
        // write MAC address
        // NOTE: MAC address in ENC28J60 is byte-backward
        enc28j60Write(MAADR5, macaddr[0]);
        enc28j60Write(MAADR4, macaddr[1]);
        enc28j60Write(MAADR3, macaddr[2]);
        enc28j60Write(MAADR2, macaddr[3]);
        enc28j60Write(MAADR1, macaddr[4]);
        enc28j60Write(MAADR0, macaddr[5]);

	enc28j60PhyWrite(PHCON1, PHCON1_PDPXMD);


	// no loopback of transmitted frames
	enc28j60PhyWrite(PHCON2, PHCON2_HDLDIS);
	// switch to bank 0
	enc28j60SetBank(ECON1);
	// enable interrutps
	enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, EIE, EIE_INTIE|EIE_PKTIE);
	// enable packet reception
	enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, ECON1, ECON1_RXEN);
}

// read the revision of the chip:
uint8_t enc28j60getrev(void)
{
	return(enc28j60Read(EREVID));
}

void enc28j60PacketSend(uint16_t len, uint8_t* packet)
{
	// Set the write pointer to start of transmit buffer area
	enc28j60Write(EWRPTL, TXSTART_INIT&0xFF);
	enc28j60Write(EWRPTH, TXSTART_INIT>>8);
	// Set the TXND pointer to correspond to the packet size given
	enc28j60Write(ETXNDL, (TXSTART_INIT+len)&0xFF);
	enc28j60Write(ETXNDH, (TXSTART_INIT+len)>>8);
	// write per-packet control byte (0x00 means use macon3 settings)
	enc28j60WriteOp(ENC28J60_WRITE_BUF_MEM, 0, 0x00);
	// copy the packet into the transmit buffer
	enc28j60WriteBuffer(len, packet);
	// send the contents of the transmit buffer onto the network
	enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, ECON1, ECON1_TXRTS);
        // Reset the transmit logic problem. See Rev. B4 Silicon Errata point 12.
	if( (enc28j60Read(EIR) & EIR_TXERIF) ){
                enc28j60WriteOp(ENC28J60_BIT_FIELD_CLR, ECON1, ECON1_TXRTS);
        }
}

// Gets a packet from the network receive buffer, if one is available.
// The packet will by headed by an ethernet header.
//      maxlen  The maximum acceptable length of a retrieved packet.
//      packet  Pointer where packet data should be stored.
// Returns: Packet length in bytes if a packet was retrieved, zero otherwise.
uint16_t enc28j60PacketReceive(uint16_t maxlen, uint8_t* packet)
{
	uint16_t rxstat;
	uint16_t len;
	// check if a packet has been received and buffered
	//if( !(enc28j60Read(EIR) & EIR_PKTIF) ){
        // The above does not work. See Rev. B4 Silicon Errata point 6.
	if( enc28j60Read(EPKTCNT) ==0 ){
		return(0);
        }

	// Set the read pointer to the start of the received packet
	enc28j60Write(ERDPTL, (NextPacketPtr));
	enc28j60Write(ERDPTH, (NextPacketPtr)>>8);
	// read the next packet pointer
	NextPacketPtr  = enc28j60ReadOp(ENC28J60_READ_BUF_MEM, 0);
	NextPacketPtr |= enc28j60ReadOp(ENC28J60_READ_BUF_MEM, 0)<<8;
	// read the packet length (see datasheet page 43)
	len  = enc28j60ReadOp(ENC28J60_READ_BUF_MEM, 0);
	len |= enc28j60ReadOp(ENC28J60_READ_BUF_MEM, 0)<<8;
        len-=4; //remove the CRC count
	// read the receive status (see datasheet page 43)
	rxstat  = enc28j60ReadOp(ENC28J60_READ_BUF_MEM, 0);
	rxstat |= enc28j60ReadOp(ENC28J60_READ_BUF_MEM, 0)<<8;
	// limit retrieve length
        if (len>maxlen-1){
                len=maxlen-1;
        }
        // check CRC and symbol errors (see datasheet page 44, table 7-3):
        // The ERXFCON.CRCEN is set by default. Normally we should not
        // need to check this.
        if ((rxstat & 0x80)==0){
                // invalid
                len=0;
        }else{
                // copy the packet from the receive buffer
                enc28j60ReadBuffer(len, packet);
        }
	// Move the RX read pointer to the start of the next received packet
	// This frees the memory we just read out
	enc28j60Write(ERXRDPTL, (NextPacketPtr));
	enc28j60Write(ERXRDPTH, (NextPacketPtr)>>8);
	// decrement the packet counter indicate we are done with this packet
	enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, ECON2, ECON2_PKTDEC);
	return(len);
}

