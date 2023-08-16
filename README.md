# HFTtool

This is a tool for high-frequency trading analysis. It connects to many exchanges and receives live trade detail data. Developed as a basis for creating technical analysis tools for finding arbitrage and short-term trading opportunities. HFTtool can also be used as the basis for trading bots.

<img src="https://github.com/aakhazov/HFTtool/blob/main/__doc/HFTtool.gif">

### Features

- Maximum power efficiency
- Easy to expand with new exchanges and analysis tools
- Stability proven by 24/7 testing

### Architecture

<img src="https://github.com/aakhazov/HFTtool/blob/main/__doc/HFTtool.png">

### Dependencies

- [Boost](https://github.com/boostorg/boost)
- [ImGui](https://github.com/ocornut/imgui)
- [ImPlot](https://github.com/epezent/implot)
- [spdlog](https://github.com/gabime/spdlog)

### Build

To build HFTtool you should run the script **_./\_\_doc/ubuntu_18.04_setup.sh_**. It will setup Ubuntu and build HFTtool even if the OS is newly installed. Also, you can read this script for details about the build.

For the latest versions of Ubuntu, you need to fix the include paths in **_Makefile_** and in **_./.vscode/c_cpp_properties.json_**.
For example, on Ubuntu 22.04, replace **_/usr/include/x86_64-linux-gnu/c++/7_** with **_/usr/include/x86_64-linux-gnu/c++/11_**

> **Note: If you are using VM, only Oracle VM VirtualBox does not support OpenGL 3. Therefore, you need to disable 3D acceleration in order to emulate it programmatically.**

### Usage

I will write soon...
