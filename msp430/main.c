#include <msp430.h> 
#include <stdint.h>
#define LCDADDR 0x27
#define BUFFERSIZE 16

uint8_t i2cSend(uint8_t addr, uint8_t data);
void i2cConfig();
void lcdBlink();
void lcdWriteNibble(uint8_t nibble, uint8_t isChar);
void lcdWriteByte(uint8_t byte, uint8_t isChar);
void lcdInit();
void lcdWrite(char *str);
void uartConfig();


int lcdLine = 0x00;
char rxBuff[BUFFERSIZE];
uint8_t rxIndex;

int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;   // stop watchdog timer
    volatile uint8_t c;

    i2cConfig();
    lcdInit();
    lcdWrite("Aguardando valor\n");
    lcdWrite("do bitcoin...");
    uartConfig();
    __enable_interrupt();

    while(1)
    {

    }

    return 0;
}

void i2cConfig()
{
    UCB0CTL1 = UCSWRST; //RESET
    UCB0CTL0 |= UCMST | UCSYNC | UCMODE_3;
    UCB0CTL1 |= UCSSEL__SMCLK;
    UCB0BRW = 100; //DIVIDE O CLOCK

    //CONFIGURACAO DOS PINOS UCB0
    P3SEL |= BIT0 | BIT1;
    P3REN |= BIT0 | BIT1;
    P3DIR &= ~(BIT0 | BIT1);

    UCB0CTL1 &= ~UCSWRST;
}



uint8_t i2cSend(uint8_t addr, uint8_t data)
{
    UCB0IFG = 0;
    UCB0I2CSA = addr;
    UCB0CTL1 |= UCTXSTT | UCTR; //START E INDICA QUE É UM TRANSMISSOR

    while(!(UCB0IFG & UCTXIFG)); //ESPERA BUFFER INDICAR QUE ESTA VAZIO
    UCB0TXBUF = data;

    while(UCB0CTL1 & UCTXSTT); //ESEPERA CICLO DE ACKNOLODGE

    if(UCB0CTL1 & UCTXNACK)
    {
        UCB0CTL1 |= UCTXSTP;
        while(UCB0CTL1 & UCTXSTP); //espera stop ser transmitido
        return   0;
    }

    while(!(UCB0IFG & UCTXIFG)); //espera carregamento
    UCB0CTL1 |= UCTXSTP;
    while(UCB0CTL1 & UCTXSTP); //espera stop ser transmitido
    return 1;
}

void lcdBlink()
{
    i2cSend(LCDADDR, BIT3);
    __delay_cycles(100000);
    i2cSend(LCDADDR, 0);
}


void lcdWriteNibble(uint8_t nibble, uint8_t isChar)
{
    nibble <<= 4; //SHIFT DO NIBBLE PARA ESCREVERMOS NA PARTE MENOS SIGNIFICATIVA

    i2cSend(LCDADDR, nibble | BIT3 | 0 | 0 | isChar );
    i2cSend(LCDADDR, nibble | BIT3 | BIT2 | 0 | isChar );
    i2cSend(LCDADDR, nibble | BIT3 | 0 | 0 | isChar );

}


void lcdWriteByte(uint8_t byte, uint8_t isChar)
{
    lcdWriteNibble(byte >> 4 ,isChar); //PARTE MAIS SIGNIFICATIVA
    lcdWriteNibble(byte & 0x0F,isChar); //PARTE MENOS SIGNIFICATIVA
}


void lcdInit()
{
    lcdWriteNibble(0x3 ,0); //GARANTIR QUE ANTES ESTÁ NO ESTADO 8 BITS (EVITAR BUG)
    lcdWriteNibble(0x3,0); // GARANTE DE NOVO
    lcdWriteNibble(0x3,0); // GARANTE DE NOVO

    lcdWriteNibble(0x2,0); //ENTRAR NO MODO 4 BITS

    lcdWriteByte(0X06, 0); //CONFIG LCD
    lcdWriteByte(0X0F, 0); //LEVA CURSOR AO COMEÇO, LIGA DISPLAY E LEVA CURSOR AO COMEÇO
    lcdWriteByte(0X14, 0); //CURSOR ANDAR PARA A DIREITA
    lcdWriteByte(0X28, 0); //MODO 4 BITS COM DUAS LINHAS E FONTE PADRAO 5X8
    lcdWriteByte(0X01, 0); //LIMPA O LCD

}

void lcdWrite(char *str)
{
    while(*str)
    {
        if(*str == '\n')
        {
            lcdLine ^= BIT6;
            lcdWriteByte(BIT7 | lcdLine, 0);
            str++;
        }
        else
        {
            lcdWriteByte(*str++, 1);
        }

    }
}


void uartConfig()
{
    rxIndex = 0;

    UCA0CTL1 = UCSWRST;

    UCA0CTL0 &= ~UCPEN;
    UCA0CTL0 |= UCMODE_0 | 0;

    UCA0BRW = 6;
    UCA0MCTL = UCBRF_13 | UCOS16;
    UCA0CTL1 = UCSSEL_2;

    P3SEL |= BIT3 | BIT4; //p3.3 (rx) e p3.4 (tx)

    UCA0CTL1 &= ~UCSWRST;

    UCA0IE = UCRXIE;

}

#pragma vector = USCI_A0_VECTOR
__interrupt void UART_A0()
{
    char received = UCA0RXBUF;
    UCA0TXBUF = 'A';
    if (received != '\0')
    {
       rxBuff[rxIndex++] = received;
    }
    else
    {
        rxBuff[rxIndex++] = received;
        rxIndex &= 0x0F;
        lcdInit();
        lcdWrite(rxBuff);
        rxIndex = 0;
    }
}








