<div align="center">

<img src="k.png" width="120" alt="Kodanima">

# Kodanima 2D

<img src="b1.png" width="800" alt="Kodanima - captura 1">

<img src="b2.png" width="800" alt="Kodanima - captura 2">

</div>

# Kodanima 2D

**Kodanima 2D** is a 2D drawing and animation editor project developed in C++ for Microsoft Windows. Its goal is to provide a native interface for drawing, organizing frames, and working with an animation timeline.

## Features

- Interface built using native Windows APIs.
- Drawing and frame organization for 2D animation.
- Timeline for navigating and organizing frames.
- Support for images such as PNG and BMP, depending on the features available in each version.
- Audio features and media importing, depending on the current implementation.
- Support for compatible styluses and graphics tablets, depending on drivers and configuration.
- Settings and keyboard shortcuts.

## Technologies 



<p align="center">
  <a href="https://www.microsoft.com/" title="Microsoft">
    <img src="https://learn.microsoft.com/el-gr/entra/identity-platform/media/howto-add-branding-in-apps/ms-symbollockup_mssymbol_19.svg"
         height="40" alt="Microsoft">
  </a>
  &nbsp;&nbsp;&nbsp;
  <a href="https://openai.com/" title="OpenAI">
    <img src="https://cdn.jsdelivr.net/gh/glincker/thesvg@main/public/icons/openai/default.svg"
         height="40" alt="OpenAI">
  </a>
  &nbsp;&nbsp;&nbsp;
  <a href="https://www.amd.com/" title="AMD">
    <img src="https://cdn.simpleicons.org/amd/ED1C24"
         height="40" alt="AMD">
  </a>
  &nbsp;&nbsp;&nbsp;
  <a href="https://www.intel.com/" title="Intel">
    <img src="https://cdn.simpleicons.org/intel/0071C5"
         height="40" alt="Intel">
  </a>
  &nbsp;&nbsp;&nbsp;
  <a href="https://www.wacom.com/" title="Wacom">
    <img src="https://cdn.simpleicons.org/wacom/737373"
         height="40" alt="Wacom">
  </a>
</p>

<h1 align="center">Kodanima 2D</h1>


<h1 align="center">Kodanima 2D</h1>


- **C++** — primary programming language.
- **Win32 API** — window creation, controls, events, and Windows integration.
- **GDI** — interface drawing and compatible graphics operations.
- **DirectX** — graphics APIs used in parts of the program that implement graphics acceleration. Availability depends on the operating system, hardware, and drivers.
- **Wintab** — integration with compatible graphics tablets, when available.
- **Intel and AMD CPU instructions** — the program can detect and take advantage of compatible instruction sets, such as MMX, SSE, and AVX, when implemented and supported by the processor and system. Not every instruction is used for every operation.
- **MinGW-w64 / GCC (G++)** — compilation.
- **Geany** — development environment used for the project.

## Operating Systems

The project targets **Microsoft Windows**. Compatibility with Windows XP and later versions depends on the APIs used, linked libraries, and build configuration. Newer DirectX, media, or CPU instruction features may require specific Windows versions, compatible hardware, or drivers.

## Computer Resource Usage

Kodanima uses the **CPU** for processing and calculations, **RAM** for temporary data, and available graphics resources to draw the interface and process visual content. In implemented components, **DirectX may enable GPU acceleration**. Performance depends on the hardware, drivers, Windows version, and features actually enabled.

Detecting processor capabilities does not mean that every Intel or AMD instruction is used for every task. The program should execute only instructions supported by the processor and runtime environment.

## Building the Project

The project is designed to be compiled using **MinGW-w64/G++**, for example, through Geany. Libraries and linker options may vary depending on the code version.

1. Install a compatible MinGW-w64 distribution.
2. Open the `.cpp` file in Geany.
3. Check the compilation and linking options required by the current source code.
4. Compile and test the program on the target Windows system.

The build command may vary between project versions.

## Getting Started

1. Download or clone this repository.
2. Open the source code in Geany or another C++-compatible editor.
3. Compile it with the dependencies required by the corresponding version.
4. Run the program and test the available features.

## Limitations and Compatibility

The project is under development. Not every planned media format or feature is necessarily complete or available in every version. Graphics acceleration, graphics tablet support, and media export depend on the implementation and the runtime environment.

## Reporting Issues

When reporting a bug, please include:

- Windows version.
- Processor (Intel or AMD and model, if known).
- Compiler used.
- Build command.
- Full error message.
- Steps required to reproduce the issue.

## License

This project is distributed under the terms of the **GNU General Public License v3.0 (GPL-3.0)**. See the repository's `LICENSE` file for the full license text. If the file does not yet exist, add an official copy of the GPL-3.0 before publishing the repository as licensed under this agreement.
