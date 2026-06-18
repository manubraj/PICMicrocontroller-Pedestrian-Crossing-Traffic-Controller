#include <xc.h>

// Configuration Bits (20MHz crystal oscillator, watchdog timer off, low voltage programming off)
#pragma config OSC = HS
#pragma config WDT = OFF
#pragma config LVP = OFF

// Function prototype for software delay
void delay();
void run_normal_traffic();
void run_pedestrian_sequence();

// Volatile flag representing pedestrian cross request
volatile unsigned char pedestrian_request = 0;

void main(void) {
    // 1. Configure Port Registers
    TRISD = 0x00;  // Configure PORTD as outputs (traffic and pedestrian LEDs)
    TRISB = 0xFF;  // Configure PORTB as inputs (RB0 is Pedestrian button input)
    ADCON1 = 0x0F; // Configure all pins as digital GPIO

    // 2. Configure Weak Pull-ups on PORTB
    RBPU = 0;      // Enable PORTB weak pull-up resistors (Active Low input safety)

    // 3. Configure External Interrupt (INT0 on RB0)
    INTEDG0 = 0;   // Trigger on Falling Edge (High-to-Low voltage on button press)
    INT0IF = 0;    // Clear interrupt flag initially
    INT0IE = 1;    // Enable INT0 external interrupt
    PEIE = 1;      // Enable peripheral interrupts
    GIE = 1;       // Enable global interrupts

    // Initial Safe State
    run_normal_traffic();

    while (1) {
        if (pedestrian_request == 1) {
            // Execute safe pedestrian walk transition
            run_pedestrian_sequence();
            
            // Clear request flag and resume normal vehicle flow
            pedestrian_request = 0; 
        } else {
            run_normal_traffic();
        }
    }
}

// 4. State Machine Helper Functions
void run_normal_traffic() {
    // Car Green (RD0 = 1), Pedestrian Don't Walk (RD4 = 1) -> 0x11 (0001 0001)
    LATD = 0x11;
}

void run_pedestrian_sequence() {
    // Phase 1: Car Yellow warning (RD1 = 1), Pedestrian Don't Walk (RD4 = 1) -> 0x12 (0001 0010)
    LATD = 0x12;
    delay();

    // Phase 2: Car Red stop (RD2 = 1), Pedestrian Walk (RD3 = 1) -> 0x0C (0000 1100)
    LATD = 0x0C;
    delay();

    // Phase 3: Walk warning strobe (Car Red RD2 remains ON while walk signals alternate)
    // Alternate between Walk (RD3) and Don't Walk (RD4)
    for (int k = 0; k < 3; k++) {
        LATD = 0x14; // Car Red (RD2=1), Pedestrian Don't Walk (RD4=1) -> 0x14 (0001 0100)
        delay();
        LATD = 0x0C; // Car Red (RD2=1), Pedestrian Walk (RD3=1) -> 0x0C (0000 1100)
        delay();
    }

    // Phase 4: All Stop (Car Red RD2=1, Pedestrian Don't Walk RD4=1) -> 0x14 (0001 0100)
    LATD = 0x14;
    delay();
}

// 5. Interrupt Service Routine (ISR)
void __interrupt() isr(void) {
    // Verify that the external interrupt INT0 caused the trigger
    if (INT0IF == 1) {
        // Record walk request
        pedestrian_request = 1; 
        
        // Clear flag in software to enable future interrupts
        INT0IF = 0; 
    }
}

// Software delay definition
void delay() {
    int i, j;
    for (i = 0; i < 600; i++) {
        for (j = 0; j < 600; j++) {
            // Delay loop
        }
    }
}
