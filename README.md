# Pedestrian-Crossing-Traffic-Controller

An interactive, zero-latency traffic light controller implemented on a **PIC18F4580** microcontroller using external hardware interrupts (`INT0`) and a modular state machine architecture.

---

##  Project Design & Logic

In municipal traffic controllers, registering pedestrian walk requests requires an instant reaction. Traditional "polling" methods can cause noticeable lag or miss button presses entirely if the processor is occupied executing long delay loops for the vehicle signals. 

This project solves that issue by utilizing **External Hardware Interrupts (`INT0`)** to capture walk requests with zero latency:

* **Normal Vehicle Flow:** The traffic light stays Green for cars (`RD0` active) and Red ("Don't Walk") for pedestrians (`RD4` active). The CPU remains in this idle state, executing a clean background routine.
* **Pedestrian Cross Request:** A pedestrian presses the crosswalk button connected to `RB0/INT0`. This instantly triggers a **falling-edge interrupt**, running a minimal Interrupt Service Routine (ISR) that flags the event and immediately exits.
* **Walk Signal Transition:** The main loop detects the request flag and runs a structured state machine:
  1. **Car Green** turns OFF.
  2. **Car Yellow Warning** turns ON (`RD1` active) for a short buffer period, keeping Pedestrian Don't Walk ON.
  3. **Car Red** turns ON (`RD2` active), completely halting vehicle traffic.
  4. **Pedestrian Walk** turns ON (`RD3` active) for a safe crossing interval.
  5. **Walk Warning Strobe:** The Walk light flashes an alternating warning pattern (swapping `RD3` and `RD4`) to notify pedestrians that crossing time is ending, while securely maintaining the Car Red light (`RD2` active).
  6. The system resets cleanly back to normal traffic flow.

> ⚠️ **Clean Embedded Architecture:** Keep Interrupt Service Routines (ISRs) as brief as possible. Do not put delay loops or heavy processing inside the ISR. Instead, set a global variable flag inside the ISR and let the main program loop execute the slow state transitions.

---

##  Connection Guide
![image alt](https://github.com/annabiju26-alt/Pedestrian-Crossing-Traffic-Controller/blob/86cde88ab587430516db63aa90735beeba976511/pedestrian.png)

Follow this layout schema to wire the traffic lights and the pedestrian switch on a breadboard or simulation environment:

| PIC18F4580 Pin | Pin # | Target Hardware | Wiring Instructions |
| :--- | :---: | :--- | :--- |
| **RB0 / INT0** | 33 | Pedestrian Request Button | Connect switch terminal to Ground (GND). Connect a 10kΩ pull-up resistor from RB0 to +5V ($V_{DD}$), or enable the weak internal PORTB pull-ups. Pressing the switch triggers a falling edge. |
| **RD0** | 19 | Green LED (Car Light) | Connect to anode through a 220Ω series resistor to Ground (GND). Indicates car green phase. |
| **RD1** | 20 | Yellow LED (Car Light) | Connect to anode through a 220Ω series resistor to Ground (GND). Indicates car yellow phase. |
| **RD2** | 21 | Red LED (Car Light) | Connect to anode through a 220Ω series resistor to Ground (GND). Indicates car stop phase. |
| **RD3** | 22 | Green LED (Pedestrian Walk) | Connect to anode through a 220Ω series resistor to Ground (GND). Indicates walk phase. |
| **RD4** | 27 | Red LED (Pedestrian Don't Walk) | Connect to anode through a 220Ω series resistor to Ground (GND). Indicates do not walk phase. |
| **VDD / VSS** | 11, 12 | Power Lines (+5V / 0V) | Connect $V_{DD}$ to +5V VCC and $V_{SS}$ to system ground. |

---

##  Project Code (XC8 C Source)

This program implements the structured, modular pedestrian controller separating interrupt flag detection from main state logic. Save this code as `main.c`:

```c
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
