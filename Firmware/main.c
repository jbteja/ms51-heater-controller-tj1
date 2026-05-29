#include "global.h"

const char allStringsDpy[][17] = {
  //"1234567890123456\0"
    "  Sanitary Pad  \0",
    "   Incinerator  \0",
    "    Hygienic    \0",
    "    Disposal    \0",
    "Pres Red to Stop\0",
    " Green to Start \0" 
};

float vTemp = 0.0;
uint8_t tempStr[9] = {'\0'};
uint8_t heaterState = 0, lastState = 0;
uint8_t toggle = 1, lastToggle = 0, dpy = 0;
int8_t sec = 0, min = 0, hrs = 0, lastSec = -1;
uint32_t millies = 0, pmillies = 0, ms = 0;
uint8_t arySize = sizeof(allStringsDpy) / sizeof(allStringsDpy[0]);

/* Liquid crystal display state functions */
void display_on_state(void){
    LiquidCrystal_clear();
    LiquidCrystal_setCursor(0, 0);
    LiquidCrystal_putStr("Switching On");
    delay_ms(250);
    LiquidCrystal_clear();
}

void display_off_state(void){
    LiquidCrystal_clear();
    LiquidCrystal_setCursor(0, 0);
    LiquidCrystal_putStr("Switching Off");
    delay_ms(250);
    LiquidCrystal_clear();
}

void display_exe_state(void) {
    float tr = 0.00;
    uint8_t ittr;

    if (sec != lastSec) {
        // Max6675 in Degrees
        for (ittr = 0; ittr < TEMP_SENSOR_SAMPLES; ittr++) {
            tr += Max6675_readCelsius();
            delay_ms(10);
            if (tr == 0.00) {
                break;
            }
        }
        tr = tr / TEMP_SENSOR_SAMPLES;

        LiquidCrystal_setCursor(0, 0);
        LiquidCrystal_putStr("Time ->");
        display_char(8, 0, hrs);
        display_char(11, 0, min);
        display_char(14, 0, sec);
        
        LiquidCrystal_setCursor(0, 1);
        LiquidCrystal_putStr("Temp ->");
        LiquidCrystal_setCursor(8, 1);
        
        if (tr == 0.00) {
            LiquidCrystal_putStr(" Error! ");
        } else {
            float_to_str(tune_rcvd_val(tr), 2, tempStr);
            LiquidCrystal_putStr("      ");
            LiquidCrystal_setCursor(8, 1);
            LiquidCrystal_putStr(tempStr);
            LiquidCrystal_setCursor(14, 1);
            LiquidCrystal_putChar(0xDF);
            LiquidCrystal_putChar('C');
        }

        lastSec = sec;
    }

    // For clock effect
    if (toggle != lastToggle) {
        if(toggle) {
            LiquidCrystal_setCursor(10, 0);
            LiquidCrystal_putChar(':');
            LiquidCrystal_setCursor(13, 0);
            LiquidCrystal_putChar(':');
        } else {
            LiquidCrystal_setCursor(10, 0);
            LiquidCrystal_putChar(' ');
            LiquidCrystal_setCursor(13, 0);
            LiquidCrystal_putChar(' ');
        }
        lastToggle = toggle;
    }
}

void display_exe_state_valid_temp(void) {
    float tr = 0.00;
    uint8_t ittr, vSamples = 0;

    if (sec != lastSec) {
        // Read temp only if DISPLAY_TEMP is enabled
        if (DISPLAY_TEMP == 1) {
            // Max6675 in Degrees
            for (ittr = 0; ittr < TEMP_SENSOR_SAMPLES; ittr++) {
                float tc = Max6675_readCelsius();
                delay_ms(10);

                // Only accumulate valid readings
                if (tc != 0.00) {
                    tr += tc;
                    tc = 0.00;
                    vSamples++;
                
                } else { 
                    break;
                }
            }

            // Average only if we have valid samples
            if (vSamples > 0) {
                tr /= vSamples;
                vSamples = 0;
            
                // Only update vTemp if the new temp is higher
                if (tr > vTemp) {
                    vTemp = tr;
                    tr = 0.00;
                }
            }
        }

        // Display the time
        LiquidCrystal_setCursor(0, 0);
        LiquidCrystal_putStr("Time ->");
        display_char(8, 0, hrs);
        display_char(11, 0, min);
        display_char(14, 0, sec);

        // Display the temperature only if DISPLAY_TEMP is 1 and the temperature is valid
        if (DISPLAY_TEMP == 1 && vTemp > 0.01) {
            LiquidCrystal_setCursor(0, 1);
            LiquidCrystal_putStr("Temp ->");
            LiquidCrystal_setCursor(8, 1);

            float_to_str(tune_rcvd_val(vTemp), 2, tempStr);
            LiquidCrystal_putStr(tempStr);
            LiquidCrystal_setCursor(14, 1);
            LiquidCrystal_putChar(0xDF);  // Degree symbol
            LiquidCrystal_putChar('C');
        }

        lastSec = sec;
    }

    // Toggle for clock blinking effect
    if (toggle != lastToggle) {
        LiquidCrystal_setCursor(10, 0);
        LiquidCrystal_putChar(toggle ? ':' : ' ');
        LiquidCrystal_setCursor(13, 0);
        LiquidCrystal_putChar(toggle ? ':' : ' ');
        lastToggle = toggle;
    }
}

void display_normal_state(uint8_t strIdx) {
        LiquidCrystal_clear();
        LiquidCrystal_setCursor(0, 0);
        LiquidCrystal_putStr(allStringsDpy[strIdx]);
        LiquidCrystal_setCursor(0, 1);
        LiquidCrystal_putStr(allStringsDpy[strIdx+1]);
}

/* Heater and Timer1 ON and OFF functions */
void heater_on(void) {
    RLY_PIN = LOW;
    LED_GRN = LOW;
    LED_RED = HIGH;
    heaterState = HIGH;

    // Set clock for heater operation
    ms = 0;
    hrs = ONHRS;
    min = ONMIN;
    sec = ONSEC;

    // Safety clock
    if (IS_SENSOR == 1) {
        float tc = Max6675_readCelsius();

        if (tc > 0.01 && tc > SAFE_TEMP) {
            min = SAFE_ONMIN;
        }
    }

    toggle = 0;
    set_timer1_tl_th(DIV12_1ms);
    ENABLE_TIMER1_INTERRUPT;
    SET_INT_Timer1_LEVEL3;
    set_TCON_TR1; // Timer 1 Run
}

void heater_off(void) {
    RLY_PIN = HIGH;
    LED_GRN = HIGH;
    LED_RED = LOW;

    // Clock
    lastToggle = 1;
    clr_TCON_TR1; // Timer 1 Stop
    DISABLE_TIMER1_INTERRUPT;
}

void heater_auto_cutoff(void) {
    if ((sec == 0) && (min == 0) && (hrs == 0)) {
        heaterState = LOW;
    } else {
        display_exe_state_valid_temp();
    }
}

void count_down_clock(void) {
    ms++;
    if(ms == 499) {
      toggle = ~toggle;
    } else if (ms > 999) {
        ms = 0;
        sec--;
        if (sec == -1) {
            sec = 59;
            min--;
            if (min == -1) {
                min = 59;
                if(hrs != 0) {
                    hrs --;
                }
            }
        }
    }
}

/* Timer 0, 1 & External Int1 ISR functions */
void Timer0_ISR (void) interrupt 1 {
    _push_(SFRS);
    millies++;
    set_timer0_tl_th(DIV12_1ms);
    clr_TCON_TF0;
    _pop_(SFRS);
}

void INT1_ISR (void) interrupt 2 {
	_push_(SFRS);
    heaterState = LOW;
	clr_TCON_IE1;
	_pop_(SFRS);
}

void Timer1_ISR (void) interrupt 3 {
    _push_(SFRS);
    count_down_clock();
    set_timer1_tl_th(DIV12_1ms);
    clr_TCON_TF1;
    _pop_(SFRS);
}

/* Initial SetUp */
void setup(void) {
	GPIO_init();
	MAX6675_GPIO_init();
    Max6675_SPI_init();

    // Timer 0
    start_timer0_mode1(DIV12_1ms);

	// Ext Int 1
	set_P17;
	INT1_LOW_LEVEL_TRIG;
    set_IE_EX1;

    // Timer 1
    ENABLE_TIMER1_MODE1;
    TIMER1_FSYS_DIV12;

	// Heater
    heater_off();

    // LCD
    LiquidCrystal_init(16, 2, 0);
    LiquidCrystal_clear();
    LiquidCrystal_setCursor(4, 0);
    LiquidCrystal_putStr("WELCOME");
    LiquidCrystal_setCursor(2, 1);
    
    LiquidCrystal_putStr("Sandi Community");
    delay_ms(300);
    display_normal_state(dpy);
    
    ENABLE_GLOBAL_INTERRUPT;
}

/* Main */
void main(void) {	
  setup();

  while(1) {
    if ((heaterState == LOW) && (lastState == HIGH)) {
        heater_off();
        display_off_state();
        lastState = LOW;

    } else if ((heaterState == LOW) && (SW_SART == LOW)) {
        display_on_state();
        heater_on();
        lastState = HIGH;

    } else if ((heaterState == HIGH) && (lastState == HIGH)) {
        heater_auto_cutoff();

    } else {
        if ((millies - pmillies) >= LCD_NRL_DPY_INT) {
            (dpy < (arySize - 2)) ? (dpy += 2) : (dpy = 0);
            display_normal_state(dpy);
            pmillies = millies;
        }
    }
  }
}
