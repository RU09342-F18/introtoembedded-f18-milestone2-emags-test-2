//Milestone 2 - Embedded Systems
//Michael Maloney and Michael Sorce
//Last Updated - 12/2/18

//Includes MSP430F5529 Header File
#include <msp430.h> 
//includes c standard library (for math function abs())
#include "stdlib.h"

//defines variables used for calculations throughout the program
float actualTemp = 0;//actual temperature reading of the PTAT
float deltaTemp = 0;//difference between actual temperature and desired temperature
float magDeltaTemp = 0;//absolute value of deltaTemp
float temp = 27;//Desired temperature for the system to maintain (user inputs this value, initially at 27
float volt = 0;//voltage reading by the ADC

//Main function
int main(void)
{
    //stop watchdog timer
    WDTCTL = WDTPW | WDTHOLD;

    //Pin set up for outputs
        P1DIR |= BIT3;                             //P1.3 set to output
        P1SEL |= BIT3;                             //P1.3 select sent to 1, enabling hardware PWM

        WDTCTL = WDTPW + WDTHOLD;                 // Stop WDT
        ADC12CTL0 = ADC12SHT02 + ADC12ON;         // Sampling time, ADC12 on
        ADC12CTL1 = ADC12SHP;                     // Use sampling timer
        ADC12IE = BIT0;                           // Enable interrupt
        ADC12CTL0 |= ADC12ENC;
        P6SEL |= BIT0;                            // P6.0 ADC option select


    //UART Initialization
        P4SEL |= (BIT4 + BIT5);                   //Sets P4.4 to UART TX mode, P4.5 to UART RX mode
        UCA1CTL1 |= UCSWRST;                      //puts state machine in reset
        UCA1CTL1 = UCSSEL_2;                      //UART uses SMCLOCK

        //The following three lines of code sets the baud-rate to 9600bps at 1 MHz (SMCLK)
        //Found using TI Sample Code
        UCA1BR0 = 6;
        UCA1BR1 = 0;
        UCA1MCTL |= UCBRS_0 + UCBRF_13 + UCOS16;

        UCA1CTL1 &= ~UCSWRST;                     //starts UART state machine
        UCA1IE |= UCRXIE;                         //Enable USCI_A0 RX interrupt

    //PWM Timer Initialization
        TA0CCR0 = 1000;                           // Max number the timer will count to (makes it run at 1kHz)
        TA0CCTL2 = OUTMOD_7;                      //PWM mode 7 reset/set
        TA0CCR2 = 0;                              //Initial duty cycle set to 50% on/off (1000/500)
        TA0CTL = TASSEL_2 + MC_1 + TACLR;         //timer set up: SMCLK, up mode, clear TAR




        //***Had issue using ADC interrupt so control system code is done using polling***

        //Infinite loop
        while (1)
        {
            ADC12CTL0 |= ADC12SC;                   // Start sampling/conversion for ADC, reads PTAT voltage
            temp = UCA1RXBUF;                       // sets temperature to user input through UART register
            __delzay_cycles(10000);                 // delays 10000 clock cycles
            int adchold = ADC12MEM0;                // creates hold variable for ADC reading
            volt = (3.3/4095) * adchold;            // calculates voltage at PTAT from the ADC result (DAC conversion)


            //actualTemp = (1/10)*(((3.3*adchold)/4.095)-500);
            //The following code solves for the equation seen above
            float step1 = 3.3 * adchold;
            float step2 = step1/4.095;
            float step3 = step2 - 500;
            actualTemp = step3/10;


            deltaTemp = actualTemp - temp;          //Solves for the difference between actual temperature and desired temperature
            magDeltaTemp = abs(deltaTemp);          //solves for magnitude of delta temp
            if(deltaTemp > 0)                       //If PTAT is too hot
            {
                if(TA0CCR2 + 1 < 1000)              //If fan pwm is at least 1 below max
                TA0CCR2 = TA0CCR2 + 1;              //Increase fan speed PWM
            }
            else if (deltaTemp <= 0)                //If PTAT is too cold
            {
                if(TA0CCR2 - 1 > 10)                //if fan pwm is at least 1 above 0
                TA0CCR2 = TA0CCR2 - 1;              //Decrease fan speed PWM
            }
        }
}

//UART Interrupt Vector and ISR
#pragma vector = USCI_A1_VECTOR
__interrupt void USCI_A1_ISR(void)
{
    //Case statement to decipher UART interrupt (0: No Interrupt, 2: RX Interrupt, 4: TX Interrupt)
    switch(__even_in_range(UCA1IV, 4))
    {
    case 0://Vector 0: No Interrupt
        break;
    case 2://Vector 2: RXIFG (interrupt on receiving data)
        temp = UCA1RXBUF; //Set temp to user input value
        break;
    case 4://Vector 4: TXIFG (interrupt on transmitting data)
        break;
    default://Do nothing
        break;
    }
}


//ADC Interrupt vector
#pragma vector = ADC12_VECTOR
__interrupt void ADC12_ISR(void)
{
    __bic_SR_register_on_exit(LPM0_bits);   // Exit active CPU
}
