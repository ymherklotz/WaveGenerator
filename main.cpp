#include "mbed.h"
#include "Adafruit_SSD1306.h"
#include "function_tables.h"

//Switch input definition
#define PIN21 p21
#define PIN22 p22
#define PIN23 p23
#define PIN24 p24
#define PIN18 p18

//Sampling period for the switch oscillator (us)
#define SW_PERIOD 20000

//Display interface pin definitions
#define D_MOSI_PIN p5
#define D_CLK_PIN p7
#define D_DC_PIN p8
#define D_RST_PIN p9
#define D_CS_PIN p10

//an SPI sub-class that sets up format and clock speed
class SPIPreInit : public SPI
{
public:
    SPIPreInit(PinName mosi, PinName miso, PinName clk) : SPI(mosi,miso,clk)
    {
        format(8,3);
        frequency(2000000);
    };
};

//Interrupt Service Routine prototypes (functions defined below)
void sedge1();
void sedge2();
void sedge3();
void sedge4();
void tout();
void checkButtonInput();
void setTimer();
void increment();
void decrement();
void cycleAmount();
void genWave();
void genSine();
void genTriangle();

//Output for the alive LED
DigitalOut alive(LED1);
DigitalOut ind(LED4);
AnalogOut analog_pin(PIN18);

//External interrupt input from the switch oscillator
InterruptIn p_21(PIN21);
InterruptIn p_22(PIN22);
InterruptIn p_23(PIN23);
InterruptIn p_24(PIN24);

//Switch sampling timer
Ticker swtimer;
// generates the wave
Ticker tck;

// different switche counters as globals 
uint16_t scounter1=0;
uint16_t scount1=0;
uint16_t scounter2=0;
uint16_t scount2=0;
uint16_t scounter3=0;
uint16_t scount3=0;
uint16_t scounter4=0;
uint16_t scount4=0;
uint16_t update=0;

uint32_t frequency=0;
uint32_t generated_frequency=0;
uint32_t period=0;

uint16_t amount=1;
uint16_t wave_count=0;
uint8_t alternating=0;

extern int sine_table[16];
extern int triangle_table[16];

//Initialise SPI instance for communication with the display
SPIPreInit gSpi(D_MOSI_PIN,NC,D_CLK_PIN); //MOSI,MISO,CLK

//Initialise display driver instance
Adafruit_SSD1306_Spi gOled1(gSpi,D_DC_PIN,D_RST_PIN,D_CS_PIN,64,128); //SPI,DC,RST,CS,Height,Width

int main() { 
    //Initialisation
    gOled1.setRotation(2); //Set display rotation
    
    //Attach switch oscillator counter ISR to the switch input instance for a rising edge
    p_21.rise(&sedge1);
    p_22.rise(&sedge2);
    p_23.rise(&sedge3);
    p_24.rise(&sedge4);
    
    //Attach switch sampling timer ISR to the timer instance with the required period
    swtimer.attach_us(&tout, SW_PERIOD);
    
    gOled1.clearDisplay();
    
    //Main loop
    while(1)
    {
        //Has the update flag been set?       
        if (update) {
            //Clear the update flag
            update = 0;
            
            gOled1.setTextCursor(0, 0);
            
            checkButtonInput();
            gOled1.printf("freq = %10u Hz\n",frequency);
            switch(amount)
            {
                case 1:
                    gOled1.printf("                ^\n");
                    break;
                case 10:
                    gOled1.printf("               ^ \n");
                    break;
                case 100:
                    gOled1.printf("              ^  \n");
                    break;
                case 1000:
                    gOled1.printf("             ^   \n");
                    break;
                default:;
            }
            
            if(frequency!=0)
            {
                period=(uint32_t)1/(float)frequency*1000000;
            }

            switch(alternating)
            {
                case 0:
                    gOled1.printf("wave: square  \n");
                    break;
                case 1:
                    gOled1.printf("wave: sine    \n");
                    break;
                case 2:
                    gOled1.printf("wave: triangle\n");
                    break;
                default:;
            }
            gOled1.printf("gen  = %10u Hz\n", generated_frequency);
            
            gOled1.drawLine(0, 50, 128, 50, WHITE);
            gOled1.drawLine(0, 63, 128, 63, WHITE);
            
            gOled1.drawLine(0, 50, 0, 64, WHITE);
            gOled1.drawLine(32, 50, 32, 64, WHITE);
            gOled1.drawLine(64, 50, 64, 64, WHITE);
            gOled1.drawLine(96, 50, 96, 64, WHITE);
            gOled1.drawLine(127, 50, 127, 64, WHITE);
            
            gOled1.setTextCursor(10, 40);
            gOled1.printf("Button Operations");
            
            gOled1.setTextCursor(8, 54);
            gOled1.printf("set");
            
            gOled1.setTextCursor(40, 54);
            gOled1.printf("sel");
            
            gOled1.setTextCursor(78, 54);
            gOled1.printf("-");
            
            gOled1.setTextCursor(110, 54);
            gOled1.printf("+");
            
            //Copy the display buffer to the display
            gOled1.display();
            
            //Toggle the alive LED
            alive = !alive;
        }
    }
}


//Interrupt Service Routine for rising edge on the switch oscillator input
void sedge1() 
{
    //Increment the edge counter
    scounter1++;    
}

void sedge2() 
{
    //Increment the edge counter
    scounter2++;    
}

void sedge3() {
    //Increment the edge counter
    scounter3++;    
}

void sedge4() {
    //Increment the edge counter
    scounter4++;    
}

//Interrupt Service Routine for the switch sampling timer
void tout() 
{
    //Read the edge counters into the output register
    scount1 = scounter1;
    scount2 = scounter2;
    scount3 = scounter3;
    scount4 = scounter4;
    //Reset the edge counters
    scounter1 = 0;
    scounter2 = 0;
    scounter3 = 0;
    scounter4 = 0;
    //Trigger a display update in the main loop
    update = 1;
}

void checkButtonInput()
{
    ind=0;

    // defines the rest state of each switch
    static const uint16_t AVG1=scount1;
    static const uint16_t AVG2=scount2;
    static const uint16_t AVG3=scount3;
    static const uint16_t AVG4=scount4;
            
    float threshold=0.97;
            
    if(scount1<AVG1*threshold && scount2>=AVG2*threshold && scount3>=AVG3*threshold && scount4>=AVG4*threshold)
    {
        if(alternating==0)
            tck.attach_us(&genWave, period/2);
        else if(alternating==1)
            tck.attach_us(&genSine, period/16);
        else if(alternating==2)
            tck.attach_us(&genTriangle, period/16);
            
        if(generated_frequency!=frequency)
            generated_frequency=frequency;
        else
        {
            if(scount1<AVG1*(threshold-0.1))
            {
                alternating++;
                if(alternating>2)
                    alternating=0;
                wait_ms(250);
            }
        }
        ind=1;
    }
                
    if(scount2<AVG2*threshold && scount1>=AVG1*threshold && scount3>=AVG3*threshold && scount4>=AVG4*threshold)
    {
        cycleAmount();
        ind=1;
        wait_ms(150);
    }
                
    if(scount3<AVG3*threshold && scount2>=AVG2*threshold && scount1>=AVG1*threshold && scount4>=AVG4*threshold)
    {
        decrement();
        ind=1;
    }
                
    if(scount4<AVG4*threshold && scount2>=AVG2*threshold && scount3>=AVG3*threshold && scount1>=AVG1*threshold)
    {
        increment();
        ind=1;
    }    
}

void increment()
{
    if(frequency<(unsigned)-1)
    {
        frequency+=amount;
    }
}

void decrement()
{
    if(frequency>0)
    {
        frequency-=amount;
    }
}

void cycleAmount()
{
    if(amount%1000==0)
    {
        amount=1;
    }
    else
    {
        amount*=10;
    }
}

void genWave()
{
    analog_pin=!analog_pin;
}

void genSine()
{
    analog_pin.write_u16(sine_table[wave_count++]);
    if(wave_count>15)
        wave_count=0;
}

void genTriangle()
{
    analog_pin.write_u16(triangle_table[wave_count++]);
    if(wave_count>15)
        wave_count=0;
}
