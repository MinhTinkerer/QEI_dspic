/*
 Documentation notes
 * author: Freddy Mendoza Ticona UNI FIEE , freddy12120@gmail.com
 * fecha: 18/02/2014
 * Instrumento de medicion de posicion utilizando encoder incremental AMT102-V CUI, plataforma basada enn dspic30f2010,compilador XC16 MPLABX
 * v0.1
 * 
 */

/******************************************************************************/
/* Files to Include                                                           */
/******************************************************************************/

#include <p30Fxxxx.h>      /* Device header file                              */
#include <stdint.h>        /* Includes uint16_t definition                    */
#include <stdbool.h>       /* Includes true/false definition                  */
#include <libpic30.h>      /* Include __delay32() function                    */
#include <stdio.h>
#include <stdlib.h>

#include "system.h"        /* System funct/params, like osc/peripheral config */
#include "user.h"          /* User funct/params, such as InitApp              */
#include "qei.h"
#include "uart.h"
#include "timer.h"
#include "i2c.h"

// funciones prototipo
void Init_measure(void);
void configQEI(void);
void messureTimeforGiro(void);
void TxData(void);

void SendBytetoSlave(unsigned char address, unsigned char Byte);
unsigned char reciveBytetoSlave(unsigned char address);

/******************************************************************************/
/* Global Variable Declaration                                                */

unsigned char comando[3];

unsigned int flagIntUART = 0;
unsigned int flag_escape = 1;
char Rx_data;
unsigned int dataReady = 0;
unsigned char cont_timer = 0;
unsigned int timer1_valuemSeg = 0;
unsigned char timer1_valueSeg  = 0;
unsigned char grados = 0;
unsigned int index_uart = 0;
unsigned int num_vueltas = 0;
unsigned long acumul_pos = 0UL;
unsigned int old_POSCNT = 0;
unsigned int new_POSCNT = 0;

/******************************************************************************/

// Rutinas de interrupcion

// Interrupcion por desbordamiento del Timer1
void __attribute__((__interrupt__,no_auto_psv)) _T1Interrupt(void){

    WriteTimer1(0);
    IFS0bits.T1IF = 0;  // clear timer interrupt flag
  //interrupcion = 1/FCY *(65536-TMR1)*Prescaler;   donde FCY, es la frecuencia de trabajo.
  //overflow  0.83 s
    cont_timer++; //contador de 8 bits cuenta maximo 53.4765 segundos
   // el TIMER1 incrementa cada 12.8 us
   // TMR1 = 313 , para 1ms
   // TMR1 = 625 , para 2ms
}
 
// Interrupcion por recepcion de datos en la UART
void __attribute__((__interrupt__, no_auto_psv)) _U1RXInterrupt(void){

    IFS0bits.U1RXIF = 0;

    // tipo de comando "1/r/n "
    Rx_data = ReadUART1();
       switch (Rx_data){
           case  'A':
                    flag_escape = 1;
                    break;
           case '\r':
                     flagIntUART = 1;
                     dataReady = 0;
                     break;
            default:
                    comando[dataReady] = Rx_data;
                    dataReady++;
                    break;
       }   
}
/* i.e. uint16_t <variable_name>; */
/******************************************************************************/
/* Main Program                                                               */
/******************************************************************************/

int16_t main(void)
{

    /* Configure the oscillator for the device */
    ConfigureOscillator();
 
    /* TODO <INSERT USER APPLICATION CODE HERE> */
    configQEI();

    //*******************************************************************************
     /******************************************************************************/
    // configuracion del UART

    /* Holds the value of baud register */
    // baudvalu=FCY/(16*baudrate) - 1
    unsigned int baudvalue = 4;    // Baud Rate = 115200 para FCY = 20MIPS
    /* Holds the value of uart config reg */
    unsigned int U1MODEvalue =  UART_EN & UART_IDLE_CON & UART_ALTRX_ALTTX &
                                UART_DIS_WAKE & UART_DIS_LOOPBACK &
                                UART_DIS_ABAUD & UART_NO_PAR_8BIT &
                                UART_1STOPBIT;;
    /* Holds the information regarding uart TX & RX interrupt modes */
    unsigned int U1STAvalue =   UART_INT_TX_BUF_EMPTY &
                                UART_TX_PIN_NORMAL &
                                UART_TX_ENABLE & UART_INT_RX_CHAR &
                                UART_ADR_DETECT_DIS &
                                UART_RX_OVERRUN_CLEAR;;

    /* Turn off UART1module */
    CloseUART1();

    /* Configure uart1 receive and transmit interrupt */
    ConfigIntUART1( UART_RX_INT_EN & UART_RX_INT_PR2 &
                    UART_TX_INT_DIS & UART_TX_INT_PR6);

    OpenUART1(U1MODEvalue, U1STAvalue, baudvalue);
 /********************************************************************************/
/********************************************************************************/
 // config Timer1
    unsigned int match_value = 0xFFFF; // 16 bits

    ConfigIntTimer1(T1_INT_PRIOR_1 & T1_INT_ON);
    WriteTimer1(0);
    OpenTimer1(T1_ON & T1_GATE_OFF & T1_IDLE_STOP & T1_PS_1_256 & T1_SYNC_EXT_OFF
               & T1_SOURCE_INT, match_value);

    /***********************************************************************************/
    // configuracion del I2C
     unsigned int config1, config2;
  
   config2 = 40; // de la formula para 400Khz
   config1 = (I2C_ON
            & I2C_IDLE_CON & I2C_CLK_HLD & I2C_IPMI_DIS & I2C_7BIT_ADD & I2C_SLW_DIS
            & I2C_SM_DIS & I2C_GCALL_DIS & I2C_STR_DIS & I2C_NACK & I2C_ACK_DIS & I2C_RCV_DIS
            & I2C_STOP_DIS & I2C_RESTART_DIS & I2C_START_DIS);

  OpenI2C(config1, config2);

    /******************************************************************************/

    char sbuf[20];
    int y;
    comando[0]=0;
    while(1)
    {
        SendBytetoSlave(0x0A,'A');

        __delay32(20000000);


           
        /*if (flagIntUART){

            Init_measure();

            while(flag_escape == 0){
                POSCNT = 0;// resetea posicion!
                //index_uart = index_uart + 1;
                messureTimeforGiro();
                TxData();
                new_POSCNT = POSCNT;

                if ((int)POSCNT >0){ // limitado a 2097152 revoluciones
                     acumul_pos = acumul_pos + (unsigned long)POSCNT;

                }else if((int)POSCNT < 0){
                     acumul_pos = acumul_pos + (unsigned long)(-POSCNT);
                }else {
                ;
                }
                 
           

            }
             y = sprintf(sbuf,"N%lu",acumul_pos);
             printf("%s\n",sbuf);
             acumul_pos = 0UL;

            flagIntUART = 0;
        }*/
    }
    CloseQEI();
    CloseTimer1();
}
//Funcion que inicia la medicion
//input: none
//output: 1 init , 0 no init
void Init_measure(void){

    switch (comando[0]){
        case 'Q':
            grados = 1;
            flag_escape = 0;
            break;
        case 'W':
            grados = 2;
            flag_escape = 0;
            break;
        case 'E':
            grados = 3;
            flag_escape = 0;
            break;
        case 'R':
            grados = 4;
            flag_escape = 0;
            break;
        case 'T':
            grados = 5;
            flag_escape = 0;
            break;
        case 'Y':
            grados = 6;
            flag_escape = 0;
            break;
        case 'B':
            putcUART1('F'); // set serial interface
            while(BusyUART1());
            break;
    }
}

void configQEI(void){

    // Encoder a  512 PPR max 15000 RPM
    // dspic 20 MIPS
    // resolution = 0.7 grados sexagesimales.
    // mode de operacion X4
    //sin reset por index pulse 
    // filtro a la entrada de channel A y B
    ADPCFG = 0xFFFF;            // pines configurados como entradas digitales.
    QEICONbits.QEIM = 0;        // disable QEI module
    QEICONbits.CNTERR = 0;     //borra todos los errores  de contador
    QEICONbits.QEISIDL = 0;     //continua contando en sleep mode
    QEICONbits.SWPAB = 0;       // no intercambia
    QEICONbits.PCDOUT =  0;     // normal IO pin operacion
    QEICONbits.POSRES = 0;      // pulso de index resetea la POSCTN (position counter)
    DFLTCONbits.CEID = 1;       // count error interrupts disable
    DFLTCONbits.QEOUT = 1;      // digital filters outputs enable for QE pins
    DFLTCONbits.QECK = 4;       //1:32 clock divide for digital filter
    POSCNT = 0;
    QEICONbits.QEIM = 6;        // X4 mode con position counter reset by index
}


void messureTimeforGiro(void){
    // seteamos valore iniciales!
    
    WriteTimer1(0);
    cont_timer = 0;
   old_POSCNT = POSCNT;
    // High precision instrumental!
    switch (grados){
        case 1:
            while(((int)POSCNT < 6) && ((int)POSCNT > -6)) ;
            timer1_valuemSeg = ReadTimer1();
          //  timer1_valueSeg = cont_timer;
            break;
        case 2:
            while(((int)POSCNT < 12) && ((int)POSCNT > -12)) ;
           
            timer1_valuemSeg = ReadTimer1();
          //  timer1_valueSeg = cont_timer;
            break;
        case 3:
            while(((int)POSCNT < 17) && ((int)POSCNT > -17)) ;
            timer1_valuemSeg = ReadTimer1();
         //   timer1_valueSeg = cont_timer;
            break;
        case 4:
            while(((int)POSCNT < 23) && ((int)POSCNT > -23)) ;
            
            timer1_valuemSeg = ReadTimer1();
         //   timer1_valueSeg = cont_timer;
            break;
        case 5:
            while(((int)POSCNT < 29) && ((int)POSCNT > -29)) ;
            timer1_valuemSeg = ReadTimer1();
          //  timer1_valueSeg = cont_timer;
            break;

       case 6:
            while(((int)POSCNT < 2048) && ((int)POSCNT > -2048)) ;
            timer1_valuemSeg = ReadTimer1();
          //  timer1_valueSeg = cont_timer;
            break;
    }
}
void TxData(void){
        char sbuf[7];
        int y;
        // la aplicacion se encarga de decodificar la trama de datos.
        
        
        //y = sprintf(sbuf,"A%uD%u",timer1_valuemSeg,timer1_valueSeg);
        y = sprintf(sbuf,"A%u",timer1_valuemSeg);
       // putcUART1(timer1_valuemSeg>>8); //todo lo envia en 0.4ms
       // while(BusyUART1());   // espera que se desocupe la transmision
       // putcUART1(timer1_valuemSeg);
       // while(BusyUART1());
       // putcUART1(timer1_valueSeg);
       // while(BusyUART1());
       // printf("\n");// manda /r y /n
       // while(BusyUART1());
         printf("%s\n",sbuf);
         while(BusyUART1());
        
 
}

void SendBytetoSlave(unsigned char address, unsigned char Byte){

    unsigned char send1byte = (address<<1);
  // escritura de datos
    IdleI2C(); // genera wait condition hasta que I2C bus este en idle
    StartI2C(); // genera I2C start condition
    while(I2CCONbits.SEN); // espera hasta que la start sequencia es completada.
    MasterWriteI2C(send1byte); // direccion + escritura r/w = 0

    while(I2CSTATbits.TBF); // espera que complete la transmision
    while(I2CSTATbits.ACKSTAT); // espera el ACK

    MasterputcI2C(Byte); // manda char


    StopI2C(); // genera I2C stop condition
    while(I2CCONbits.PEN); // espera hasta  STOP sequencia es completada

}

unsigned char reciveBytetoSlave(unsigned char address){

    // lectura de datos
    unsigned char value;
    unsigned char send1byte = (address<<1) + 1;
    IdleI2C();
    StartI2C();
    while(I2CCONbits.SEN);
    MasterWriteI2C(send1byte); // direccion + lectura r/w = 0
    while(I2CSTATbits.TBF); // espera que complete la transmision
    while(I2CSTATbits.ACKSTAT); // espera el ACK

    value =  MasterReadI2C();

    StopI2C(); // genera I2C stop condition
    while(I2CCONbits.PEN); // espera hasta  STOP sequencia es completada

    return value;
}
 