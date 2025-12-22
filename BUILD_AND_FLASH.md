# STM32G473CET6 Blinker - Build and Flash Instructions

## Project Overview
This is an STM32G473CET6 blinker project that toggles GPIO PA3 every 500ms. The project uses CMake and can be built, flashed, and debugged using VSCode with ST-LINK via SWDIO interface.

## Prerequisites

### Required Tools
1. **ARM GCC Toolchain** - Install ARM GNU Toolchain for embedded development
   - Download from: https://developer.arm.com/downloads/-/gnu-rm
   - Add to PATH

2. **CMake** (3.22 or higher)
   - Download from: https://cmake.org/download/
   - Add to PATH

3. **Ninja Build System**
   - Download from: https://github.com/ninja-build/ninja/releases
   - Add to PATH

4. **STM32CubeProgrammer** - For flashing via ST-LINK
   - Download from: https://www.st.com/en/development-tools/stm32cubeprog.html
   - Install and add `STM32_Programmer_CLI` to PATH

5. **ST-LINK Drivers** - For ST-LINK hardware recognition
   - Included with STM32CubeProgrammer or download separately from ST website

### VSCode Extensions (Recommended)
1. **C/C++** (ms-vscode.cpptools)
2. **CMake Tools** (ms-vscode.cmake-tools)
3. **Cortex-Debug** (marus25.cortex-debug) - For debugging

## Hardware Setup
1. Connect ST-LINK to your STM32G473CET6 board via SWDIO interface:
   - SWDIO → SWDIO pin
   - SWCLK → SWCLK pin
   - GND → GND
   - VCC → 3.3V (if powering from ST-LINK)

2. Connect an LED to PA3 (GPIO pin 3) with appropriate current-limiting resistor

3. Connect ST-LINK to your PC via USB

## Building the Project

### Method 1: Using VSCode Tasks (Recommended)
1. Open the project folder in VSCode
2. Press `Ctrl+Shift+P` and select **Tasks: Run Build Task**
3. Select **CMake: Build (Debug)** or press `Ctrl+Shift+B`

### Method 2: Using Command Line
```powershell
# Configure CMake
cmake --preset=Debug

# Build the project
cmake --build build/Debug
```

## Flashing to STM32

### Method 1: Using VSCode Task
1. Press `Ctrl+Shift+P` and select **Tasks: Run Task**
2. Select **Build and Flash** (this will build and flash in sequence)

OR

1. Run **Flash to STM32 (ST-LINK)** task separately after building

### Method 2: Using Command Line
```powershell
# Flash using STM32CubeProgrammer CLI
STM32_Programmer_CLI -c port=SWD mode=UR reset=HWrst -w build/Debug/sensores_banco_g4_v0.elf -rst
```

## Debugging

### Using VSCode with Cortex-Debug
1. Make sure **Cortex-Debug** extension is installed
2. Press `F5` or go to **Run and Debug** panel
3. Select **Debug STM32 (Cortex-Debug)** configuration
4. The project will build automatically and start debugging

### GDB Configuration
- The launch configuration uses `arm-none-eabi-gdb`
- Make sure it's in your PATH
- Debug session will stop at `main()` entry point

## Available VSCode Tasks

- **CMake: Configure (Debug)** - Configure the CMake project
- **CMake: Build (Debug)** - Build the project (Default: `Ctrl+Shift+B`)
- **Flash to STM32 (ST-LINK)** - Flash firmware to device
- **Build and Flash** - Build and flash in one step (Default test task)
- **Clean Build** - Remove build directory

## Troubleshooting

### ST-LINK Not Detected
```powershell
# Verify ST-LINK connection
STM32_Programmer_CLI -l
```
- If not listed, check USB connection and drivers
- Try different USB port or cable

### Build Errors
- Ensure ARM GCC toolchain is properly installed and in PATH
- Check that `cmake/gcc-arm-none-eabi.cmake` toolchain file exists
- Verify CMake and Ninja are installed

### Flash Errors
- Ensure SWDIO and SWCLK are properly connected
- Check that STM32 has power
- Try resetting the board
- Verify STM32CubeProgrammer CLI is in PATH

### LED Not Blinking
- Verify LED is connected to PA3 with proper polarity
- Check LED current-limiting resistor value
- Use debugger to verify code is running
- Check that GPIO clock is enabled (should be automatic)

## Project Structure
```
sensores_banco_g4_v0/
├── .vscode/              # VSCode configuration
│   ├── tasks.json        # Build and flash tasks
│   ├── launch.json       # Debug configuration
│   └── settings.json     # Editor settings
├── Core/
│   ├── Src/
│   │   └── main.c        # Main application (blinker logic)
│   └── Inc/              # Header files
├── Drivers/              # STM32 HAL drivers
├── cmake/                # CMake configuration files
├── CMakeLists.txt        # CMake project file
├── CMakePresets.json     # CMake presets
└── STM32G473XX_FLASH.ld  # Linker script

```

## Blinker Code
The LED on PA3 toggles every 500ms in the main loop:
```c
while (1) {
    HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_3);
    HAL_Delay(500);
}
```

## Additional Resources
- [STM32G4 Reference Manual](https://www.st.com/resource/en/reference_manual/rm0440-stm32g4-series-advanced-armbased-32bit-mcus-stmicroelectronics.pdf)
- [STM32G473 Datasheet](https://www.st.com/resource/en/datasheet/stm32g473ce.pdf)
- [STM32CubeMX Documentation](https://www.st.com/en/development-tools/stm32cubemx.html)
