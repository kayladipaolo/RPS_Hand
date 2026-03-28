# Rock-Paper-Scissors Minus-One Mechatronic System

## Overview
This project implements a fully physical version of **Rock-Paper-Scissors Minus-One** using two ESP32-based embedded systems (Master and Slave). Each board controls a robotic hand capable of forming gestures and using a servo-driven cover mechanism to remove one option after both players reveal their hands.

The system combines:
- Embedded control (ESP32)
- DC motor actuation for finger control
- Servo mechanisms for cover and uncover actions
- Wireless communication using ESP-NOW
- Game theory-based decision logic on the master board

## System Architecture

### Master Board
The master board controls the **left robotic hand locally** and handles all high-level game logic. It selects the stage one pair, receives the opponent input through the Serial Monitor, decides which final hand to keep, and sends commands wirelessly to the slave board. The master implementation includes local motor control, local servo cover control, ESP-NOW transmission, and round strategy logic. 

### Slave Board
The slave board controls the **right robotic hand only**. It does not make any strategic decisions. Instead, it receives structured packets from the master, executes the requested gesture or cover state, and resets the hand when commanded. The slave code processes only the newest packet using sequence numbers to reduce the chance of repeated or stale actions. 

## Hardware Configuration

Each robotic hand uses:
- 2 DC motors
- 1 servo motor
- ESP32 microcontroller
- Motor driver

### Motor Functions
- **Motor A** controls the thumb, ring, and pinky fingers
- **Motor B** controls the index and middle fingers

### Pin Assignments
The current code uses the following pin mapping on both boards:

| Component | Pin |
|---|---:|
| Motor A IN1 | 26 |
| Motor A IN2 | 16 |
| Motor B IN1 | 27 |
| Motor B IN2 | 17 |
| Cover Servo | 4 |

## Communication Protocol

The master and slave communicate using **ESP-NOW**. The master sends a structured packet to the slave containing:
- a sequence number
- a command type
- a gesture value
- a cover state

The packet format used in the implementation is:

```cpp
struct RightHandPacket {
  uint32_t seq;
  uint8_t command;
  uint8_t gesture;
  uint8_t cover;
};
```

### Supported Commands
- `CMD_SET_GESTURE` – command the slave to form Rock, Paper, or Scissors  
- `CMD_SET_COVER` – open or close the slave cover  
- `CMD_RESET_HAND` – reset the slave hand to the default position  

The slave only executes packets newer than the last processed sequence number, which helps prevent duplicate execution and stale command issues.

---

## Game Logic

The game logic is implemented on the master board.

### Stage 1
The master randomly selects one of three non-identical pairs:
- Paper + Rock  
- Paper + Scissors  
- Rock + Scissors  

The same pair is not allowed to repeat in consecutive rounds.

### Stage 2
After the opponent pair is entered through the Serial Monitor, the master decides which of its two gestures to keep using the following logic:

- If the opponent plays an identical pair such as RR, PP, or SS, the system keeps the hand that performs better against that repeated symbol.  
- If both players have the same pair, the system keeps the dominant hand.  
- If the players have different non-identical pairs, the system keeps the shared hand with probability **2/3** and the other hand with probability **1/3**.  

---

## Startup and Round Flow

### At Startup
- Both boards initialize their motor pins  
- Both covers are opened  
- The right hand is reset by the slave  
- The left hand is reset and controlled locally by the master  

### During a Round
1. A new round is started from the master Serial Monitor  
2. The master selects its own pair  
3. The left hand is actuated locally  
4. The right hand is commanded wirelessly  
5. The opponent pair is entered manually  
6. The master decides which hand to keep  
7. One cover closes to hide the discarded hand  
8. Both hands are reset for the next round  

---

## Serial Commands

The master code currently uses the following Serial Monitor inputs:

- `n` → start a new round  
- `PR`, `RS`, `SP`, etc. → enter opponent left and right gestures  

### Example
```text
n
PR
This starts a round and tells the master that the opponent displayed Paper on the left and Rock on the right.

---

## Files in This Repository

This repository includes both final system code and supporting test code used during development:

- Final master board code  
- Final slave board code  
- ESP-NOW communication test code  
- Motor driver and direction test code  
- Servo test code  
- MAC address check utility  

These test files were used to isolate hardware behavior, verify motor directions, confirm servo motion, and validate communication before full system integration.

---

## Setup Instructions

1. Upload the **slave code** to the ESP32 controlling the right hand  
2. Open the Serial Monitor and record the slave MAC address  
3. Update the `SLAVE_MAC` value in the master code  
4. Upload the **master code** to the ESP32 controlling the left hand  
5. Power both systems  
6. Open the Serial Monitor on the master board  
7. Start a round using `n`  
8. Enter the opponent pair when prompted  

---

## Notes and Limitations

- Gesture timing is based on fixed motor delays (no feedback control)  
- Timing values are experimentally tuned and may vary depending on build quality  
- The system assumes **Paper** as the default reset position  
- Opponent input is manual (Serial Monitor), not automated  
- Performance depends on mechanical consistency and alignment  

---

## Future Improvements

- Sensor feedback for gesture confirmation  
- Computer vision for opponent detection  
- Closed-loop motor control  
- Improved mechanical linkage design  
- Wireless acknowledgement / retry system  
- Fully autonomous interface without Serial input  
