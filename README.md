# Vlink <img width="100" height="100" alt="Vlink_logo" src="https://github.com/user-attachments/assets/e858d264-b5a8-43e8-bf3c-bdb1bcdbf632" /> 

Smart Glove with Gesture-Based User Interaction
The Smart Glove with Gesture-Based Interface is a wearable electronic device designed to interpret human generated 
gestures into digital control commands, enabling natural, seamless human–computer interaction. It combines multiple 
sensors, a microcontroller, wireless communication, and an intuitive user interface into a single compact and ergonomic 
wearable glove system as shown in Fig. 1.

![WhatsApp Image 2026-01-25 at 9 32 30 AM](https://github.com/user-attachments/assets/707bd6a1-cb79-4528-8321-8c3fe8004199)
Fig. 1

The Smart Glove with Gesture-Based Interface operates on the principle of motion and flex sensing, translating hand and 
finger gestures into digital control commands for human–computer interaction. 
The glove is embedded with flex sensors that are placed along each finger to detect and record the degree of finger bend 
and an Inertial Measurement Unit (IMU), consisting of an accelerometer and gyroscope – to capture orientation, tilt, and 
motion of the hand. These sensors generate analog and motion data corresponding to each gesture performed by the user. 

The microcontroller processes these sensor data in real time and maps to specific gesture patterns by thresholds. Once the 
mapping is recognized, the control command is triggered and the corresponding command with its function is transmitted 
wirelessly via Bluetooth Low Energy (BLE) to the connected computer system. 
On selection of an operation mode, the mode – library with all gesture mappings and functionalities are loaded into the 
microcontroller, when the gesture generated is mapped to the stored gesture patterns the assigned function is triggered such 
as hand motion for cursor navigation, index finger and middle finger for mouse clicks, tilt for gaming control and other 
gestures for presentation control etc., . Visual feedback of triggered commands is provided on the OLED to confirm the 
detected gesture and operation mode.

<img width="1159" height="1358" alt="Vlink Operational Flow" src="https://github.com/user-attachments/assets/daba0e11-2cd6-41f2-8afc-a1c30d902fe0" />

This intuitive system framework enables natural, creative, and intuitive control over computer systems for humans bridging 
the gap between the virtual world and real world. 
