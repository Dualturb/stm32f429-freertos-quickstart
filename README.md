# STM32F429 FreeRTOS Quickstart

A robust, production-ready starter framework for the **STM32F429I-DISC1** Discovery board. This repository provides a pre-configured environment with FreeRTOS, HAL drivers, and basic peripheral initialization.

## 🛠 Hardware Requirements
* **Board:** STM32F429I-DISC1 (Discovery Kit)
* **Power Note:** Ensure the **JP3 (IDD)** jumper is populated. If the jumper is missing, the MCU will not power up, and the ST-LINK will report "Target no device found."
* **Connectivity:** Use the Mini-USB port (ST-LINK) for programming and debugging.

## 🚀 Key Features
* **RTOS:** FreeRTOS CMSIS-V1 integration.
* **Clock:** Configured for 180MHz (HSE).
* **Peripherals:** Pre-initialized GPIO (PG13/PG14 LEDs), LTDC (LCD), and USART1 (115200 bps).
* **Memory:** Includes SDRAM (FMC) initialization.

## 💻 Software Setup
1. Clone this repository.
2. Open **STM32CubeIDE**.
3. Go to `File > Import > Existing Projects into Workspace`.
4. Select this folder and click Finish.
5. (Optional) Open the `.ioc` file to modify peripheral configurations.

## 📂 Project Structure
* `Core/`: Main application logic and RTOS task definitions.
* `Drivers/`: STM32F4xx HAL and CMSIS drivers.
* `USB_HOST/`: Pre-configured USB stack for Host mode.

## ⚖️ License
This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
