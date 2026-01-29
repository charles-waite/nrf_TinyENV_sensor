## Codex Init Notes
This project folder will be home to a version of the TinyENV_Sensor-Thread (located at ~/Documents/Coding\ Projects/MiniSensor/TinyENV_Sensor-Thread/)

### Purpose
The aim is to take the same general project focus but rewrite it to compile and run on an nRF52840 board (in this case, a Seeed XIAO nRF52840). The TinyENV_Sensor-Thread project was built for a XIAO ESP32-C6 board.

### Features
* Matter over Thread
* Low-power implementation
	* Sleepy End Device
	* ICD-LIT enabled
	* ~2 minute update/poll cadence
	* <1mA average consumption target
* SHT41 temp and relative humidity matter endpoints
* Powered by single 18650 Li-Ion Cell
	* wired to built-in battery management circuit in XIAO nRF52840 board
	* Targeting 6mo runtime on ~2200mAh battery
* No OTA support required

### Requirements
* Lowest power consumption is the PRIMARY design requirement, and is the main criteria for evaluating adding or refactoring features and code.
* nRF-native coding and build processes
	* Moving from ESP-IDF to Nordic codebase.
* Efficient coding structure.
* Lean libraries or custom functions for accessing sensor data.
* XIAO nRF board has no boot button, so a custom button over GPIO may need to be added.
* Can use multi-color led to convey unique status codes if possible (not present on ESP32-c6.
* ESP32 source folder is present as a reference but should not necessarily be used to wholesale copy.
	* More efficient Nordic-native code should be prioritized over direct cross compatibility with ESP32 project.
* Likely going to use SEGGER IDE but open to alternate suggestions.
* Generally I want to use non-deprecated functions and libraries and the most recent versions of libraries.

### About Me
* I'm not a developer so some deep-cut coding concepts might be difficult to understand for me. 
* I'm very comfortable in CLI environments, but sometimes I may need explicit instructions to complete a task if a command is new to me. 
* I'm not able to really debug C++ code on my own but I'm able to identify patterns and overall code structures as I become more familiar with what is being done.
* I like to learn about new concepts as it helps me understand the code and what its doing.