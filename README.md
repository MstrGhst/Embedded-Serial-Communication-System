# Embedded-Serial-Communication-System
C implementation (Keil uVision) of a Master-Slave serial communication protocol for a 3-node network using the BIG8051 system (Silicon Labs C8051F040). Features UART1 with an external TTL - RS-485 adapter, multiprocessor mode, binary message encoding, and LRC error detection.
--------------------------------------------------------------
Developed a custom multi-node serial communication protocol utilizing a Master-Slave architecture for a 3-node network. The project was implemented on the BIG8051 development microsystem featuring the Silicon Labs C8051F040 microcontroller.

-> Designed and implemented the message preparation and serial transmission mechanics.
-> Configured the UART1 peripheral to interface with an external TTL to RS-485 adapter for robust differential signaling.
-> Developed 9-bit multiprocessor mode addressing, where the 9th bit differentiates address bytes from data bytes to optimize network listening.
-> Integrated binary message encoding along with automated Longitudinal Redundancy Check (LRC) modulo-2 checksum calculations for error detection.
