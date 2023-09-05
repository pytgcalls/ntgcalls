# NativeTgCalls: <br/>  An experimental successor to PyTgCalls
Welcome to NativeTgCalls, an innovative open-source project. NativeTgCalls represents the next evolution in Telegram calling, building on the foundation laid by PyTgCalls.

|                                                           Powerful                                                            |                                                   Simple                                                   |                                                                     Light                                                                      |
|:-----------------------------------------------------------------------------------------------------------------------------:|:----------------------------------------------------------------------------------------------------------:|:----------------------------------------------------------------------------------------------------------------------------------------------:|
| <img src=".github/images/fast.gif" width=150 alt="Fast Logo"/><br>Built from scratch in C++ using Boost, libwebrtc and FFmpeg | <img src=".github/images/simple.gif" width=150 alt="Simple Logo"/><br>Simple Python, GO and C Bindings<br> | <img src=".github/images/light.gif" width=150 alt="Light logo"/><br>We removed anything that could burden the library, including <b>NodeJS</b> |

...and more, **without even rewriting your code that uses PyTgCalls!**

## Overview
NativeTgCalls, often referred to as NTgCalls, is an exciting open-source venture. This project redefines the 
Telegram calling experience and introduces innovative features while preserving the legacy of PyTgCalls.

## Build Status
| Architecture |                                Windows                                 |                              Linux                              |                                 MacOS                                  |
|:------------:|:----------------------------------------------------------------------:|:---------------------------------------------------------------:|:----------------------------------------------------------------------:|
|    x86_64    |    ![BUILD](https://img.shields.io/badge/build-passing-dark_green)     | ![BUILD](https://img.shields.io/badge/build-passing-dark_green) | ![BUILD](https://img.shields.io/badge/build-not%20supported-lightgrey) |
|    ARM64     | ![BUILD](https://img.shields.io/badge/build-not%20supported-lightgrey) |    ![BUILD](https://img.shields.io/badge/build-failing-red)     |    ![BUILD](https://img.shields.io/badge/build-passing-dark_green)     |

## Key Features

### Experimentation and Customization
NativeTgCalls is a playground for experimentation. We understand the importance of customization for developers, 
empowering you to tailor your Telegram calling solutions to your precise requirements.

### Advancing with PyTgCalls
Building upon the success of PyTgCalls, NativeTgCalls provides a natural progression for developers familiar with its predecessor. 
This evolution streamlines the development process and extends the capabilities of Telegram calling.

<i>Importantly, PyTgCalls will seamlessly integrate the new core (NTgCalls) to ensure backward compatibility.</i>

## Compiling

### Python Bindings
NativeTgCalls offers Py Bindings, enabling seamless integration with Python. Follow these steps to compile NativeTgCalls with Python Bindings:
1. Ensure you are in the root directory of the NativeTgCalls project.
2. Run the following command to install the Py Bindings:

   ```shell
   python3 setup.py install
   ```
### C Bindings
For developers looking to use NativeTgCalls with C and C++, we provide C Bindings. Follow these steps to compile NativeTgCalls with C Bindings:
1. Ensure you are in the root directory of the NativeTgCalls project.
2. Run the following command to generate the shared libs:
   ```shell
   python3 setup.py build_shared
   ```
3. Upon successful execution, a shared library will be generated in the "shared-output" directory. 
   You can now use this library to develop applications with NativeTgCalls.
4. To include the necessary headers in your C/C++ projects, you will find the "include" folder in the root of the project. 
   Utilize this folder for including the required header files.
