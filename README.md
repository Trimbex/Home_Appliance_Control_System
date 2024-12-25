# Home Appliances Control Project

## Project Description

The goal of this project is to develop a small prototype for a mobile-based home appliances control system. The system will allow users to control home appliances and monitor certain environmental conditions remotely via an Android application.

### Features:

1. **Android Application Development**:
   - **Control a 220V Lamp**: The user should be able to turn the lamp on and off through the mobile app. The manual switch should still remain functional.
   - **Control a Plug**: The user should be able to turn a plug on and off through the app. When the plug is off, no one can use it.
   - **Door Status**: The app should display the status of the door, which is indicated by a magnetic switch.
   - **Room Temperature Display**: The app will show the current room temperature.
   - **Temperature Alarm**: If the temperature exceeds a set value, the app will trigger both a software alarm and a physical alarm.
   - **Track Door Usage**: The app should save and display the times when the door is opened and closed.

2. **Hardware Requirements**:
   - **Components Mounting**: All hardware components must be mounted securely on a suitable surface, and the wires should be hidden for safety and aesthetic purposes.
   - **Board**: The project should be implemented using the **TM4C123GH6PM** microcontroller board.

## Requirements

- **Software**:
  - Android Studio or any other Android development environment for building the mobile app.
  - Required libraries and dependencies for controlling the hardware components and displaying the necessary information.
  
- **Hardware**:
  - TM4C123GH6PM board for microcontroller implementation.
  - Magnetic switches for detecting door status.
  - Temperature sensor for monitoring room temperature.
  - Lamp and plug control setup (relay module, etc.).
  - Alarm system (both software and physical alarms).

## How to Run the Project

1. Set up the hardware components as per the project description.
2. Install the required Android development environment and libraries.
3. Connect the TM4C123GH6PM board to the app for control.
4. Run the Android application on a mobile device.
5. Monitor and control the appliances, track door status, and temperature alarms through the app.

## Notes

- Ensure that all wires are hidden and the components are securely mounted for safety.
- The manual switch for the lamp should always remain functional, allowing the lamp to be controlled even without the app.
