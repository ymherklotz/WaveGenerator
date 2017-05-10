#include "mbed.h"
#include "Adafruit_SSD1306.h"

//Switch input definition
#define PIN21 p21
#define PIN22 p22
#define PIN23 p23
#define PIN24 p24
//#define PIN18 p25
#define PIN18 p18

//Sampling period for the switch oscillator (us)
#define SW_PERIOD 20000
#define LOW_PERIOD 100

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
void select();
void m_abort();
void increment();
void gen_square(uint16_t avg1_threshold);
void gen_wave();
void gen_sine();

//Output for the alive LED
DigitalOut alive(LED1);
DigitalOut ind(LED4);
DigitalOut square(p17);
AnalogOut aout(p18);
PwmOut pwm_pin(p25);

//External interrupt input from the switch oscillator
InterruptIn p_21(PIN21);
InterruptIn p_22(PIN22);
InterruptIn p_23(PIN23);
InterruptIn p_24(PIN24);

//Switch sampling timer
Ticker swtimer;
Ticker tck;

//Registers for the switch counter, switch counter latch register and update flag
volatile uint16_t scounter1=0;
volatile uint16_t scount1=0;
volatile uint16_t scounter2=0;
volatile uint16_t scount2=0;
volatile uint16_t scounter3=0;
volatile uint16_t scount3=0;
volatile uint16_t scounter4=0;
volatile uint16_t scount4=0;
volatile uint16_t update=0;
volatile uint16_t value=0;
volatile uint16_t counter=0;

volatile uint32_t frequency=0;

//Initialise SPI instance for communication with the display
SPIPreInit gSpi(D_MOSI_PIN,NC,D_CLK_PIN); //MOSI,MISO,CLK

//Initialise display driver instance
Adafruit_SSD1306_Spi gOled1(gSpi,D_DC_PIN,D_RST_PIN,D_CS_PIN,64,128); //SPI,DC,RST,CS,Height,Width


const double pi = 3.141592653589793238462;
const double amplitude = 0.5f;
const double offset = 65535/2;
double rads = 0.0;
uint16_t sample = 0;

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
    tck.attach_us(&gen_wave, LOW_PERIOD);
    pwm_pin.period(0.000001);
    
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
            gOled1.printf("T    = %10u us",(unsigned)(1/(float)frequency*1000000));
            
            //Copy the display buffer to the display
            gOled1.display();
            
            //Toggle the alive LED
            alive = !alive;
        }
        gen_sine();
        //wait(0.15);
    }
}


//Interrupt Service Routine for rising edge on the switch oscillator input
void sedge1() {
    //Increment the edge counter
    scounter1++;    
}

void sedge2() {
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
void tout() {
    //Read the edge counter into the output register
    scount1 = scounter1;
    scount2 = scounter2;
    scount3 = scounter3;
    scount4 = scounter4;
    //Reset the edge counter
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
    
    static volatile const uint16_t AVG1=scount1;
    static volatile const uint16_t AVG2=scount2;
    static volatile const uint16_t AVG3=scount3;
    static volatile const uint16_t AVG4=scount4;
            
    float threshold=0.97;
            
    if(scount1<AVG1*threshold)// && scount2>=AVG2*threshold && scount3>=AVG3*threshold && scount4>=AVG4*threshold)
    {
        //m_abort();
        ind=1;
    }
                
    if(scount2<AVG2*threshold)// && scount1>=AVG1*threshold && scount3>=AVG3*threshold && scount4>=AVG4*threshold)
    {
        //gen_square(AVG1*threshold);
        ind=1;
    }
                
    if(scount3<AVG3*threshold)// && scount2>=AVG2*threshold && scount1>=AVG1*threshold && scount4>=AVG4*threshold)
    {
        select();
        ind=1;
    }
                
    if(scount4<AVG4*threshold)// && scount2>=AVG2*threshold && scount3>=AVG3*threshold && scount1>=AVG1*threshold)
    {
        increment();
        ind=1;
    }    
}

void select()
{
    frequency*=10;
}

void m_abort()
{
    frequency=0;
}

void increment()
{
    if(frequency%10==9)
        frequency=frequency-9;
    else
        ++frequency;
}


void gen_square(uint16_t avg1_threshold)
{
    for(;; wait_us((unsigned)(1/(float)frequency*1000000)/2))
    {
        //square=!square;
        
        if(scount1<avg1_threshold)
        {
            m_abort();
            break;
        }
    }
}

void gen_wave()
{
    if(counter*LOW_PERIOD>=(unsigned)1/(float)frequency*500000)
    {
        value=!value;
        pwm_pin.write(value);
        counter=0;
    }
    counter++;
}

void gen_sine()
{
    for (int i = 0; i < 360; i++) {
        rads = (pi * i) / 180.f;
        sample = (uint16_t)(amplitude * (offset * (cos(rads*2 + pi))) + offset);
        aout.write_u16(sample);
    }
}