# Introduction

MSX computers hold a significant place in computing history, and their legacy continues to be celebrated through innovative approaches like FPGA synthesis. Field-Programmable Gate Arrays (FPGAs) provide a unique platform for recreating the MSX architecture, offering a flexible and efficient way to re-create the hardware. 

This synthesis enables enthusiasts to experience the charm of MSX systems without relying on original, often scarce, hardware components. FPGA-based MSX simulations can faithfully replicate the distinctive features of these iconic computers, including the Z80 processor, graphics capabilities, and sound architecture. 

This modern approach not only preserves the nostalgia associated with MSX but also opens up possibilities for enhancements and customizations. FPGA synthesis ensures that the MSX spirit endures, catering to both seasoned aficionados and new generations eager to explore the roots of personal computing.

**WARNING!** If you use this project to implement your own hardware, remember as per the license you must release the modified files under the same license. I've seen multiple projects based on files I publish on Github that were never released to the public, and I think that's a shame (and violation of the license!).

# TRHMSX - A Simple FPGA MSX2+ clone (with additional features)

This FPGA-based MSX2+ hardware represents a straightforward implementation, drawing inspiration from the original 1chipMSX circuit but featuring several enhancements and a new PCB design for improved functionality. Despite its advanced capabilities, the primary focus during development was on maintaining a low-cost solution.

The core of this system is the Cyclone II FPGA, accompanied by a few additional components. The PCB is a 4-layer board measuring 12x15 cm. The FPGA is a 240-pin package, which is challenging to solder manually. Instead, using an SMT oven or a hot air station is advised for optimal results. For those opting for a hot air station, employing a stencil to apply solder paste is highly recommended, significantly streamlining the assembly process.

![Alt text](images/2024-02-25_20-27.png)

Programming the FPGA requires the well-known KDL ESE MSX2+ core, available for download [here](https://gnogni.altervista.org/). This core ensures the proper functioning of the MSX2+ hardware, providing users with a reliable and authentic MSX experience on this FPGA-based system.

## Features

* **MSX2+ Compatibility**: Reproducing the MSX2+ architecture, ensuring compatibility with a wide range of software and games.
* **Expanded Memory Options**: Choose between 2MB or 4MB of mapped RAM, providing users with flexibility for diverse computing needs and applications.
* **9958 Video Display Processor (VDP)**: Incorporating the 9958 VDP for sharp graphics and improved visual performance, delivering an authentic MSX experience.
* **FM and SCC Sound Capabilities**: Featuring FM synthesis and SCC sound to reproduce the iconic audio characteristics of MSX systems, enhancing the overall gaming and multimedia experience.
* **Programmable Sound Generator (PSG)**: Including the original PSG for standard MSX audio output.
* **Keyboard Compatibility**: Supporting PS/2 keyboards with the option for USB keyboards, providing users with a choice for their preferred input device.
* **MicroSD Card Support**: Integrating a microSD card slot for convenient storage and easy access to files, games, and software.
* **MSX Cartridge Slots**: Equipped with two MSX cartridge slots, allowing users to explore a vast library of MSX cartridges for an extended range of applications and games.
* **Dual Joystick Ports**: Featuring two joystick ports for multiplayer gaming and compatibility with classic MSX peripherals.
* **Wireless Network Support**: Incorporating wireless network support through the ESP8266, enabling online connectivity and expanding the possibilities for networked applications.
* **12V Cartridge Lines**: Providing dedicated 12V lines for cartridges, ensuring compatibility with a variety of peripherals and accessories.

## Open Source

This project stands as a testament to the spirit of open source collaboration. Over the years, the original 1chipMSX circuit has inspired numerous clones, some even commercially implemented, each potentially harboring unique modifications. Regrettably, many of these adaptations were never shared with the broader community. In response, this project emerges as a fresh implementation of the original circuit, introducing enhancements and a redesigned PCB.

What sets this initiative apart is its commitment to transparency. From the project's inception, the goal was clear: to share all design files openly. This includes not only the schematics and PCB layout but also the Bill of Materials (BOM) and any other essential documentation for recreating the project.

In stark contrast to its predecessors, this project was conceived with a dedication to community collaboration. If you choose to utilize these resources to construct your own MSX2+ clone or find ways to enhance it, I encourage you to contribute back to the community. Share your experiences, document any improvements you make, and actively participate in the collective evolution of this open source endeavor.

Let this project serve as a reminder of the **reciprocal nature of open source**. By using and benefiting from open source projects, it becomes a responsibility to give back, ensuring the continued growth and enrichment of the shared knowledge pool within the community.

## Bill of materials

[More info to come]

## Building

To build the project you will need the following tools and resources:

* [Quartus Prime Lite Edition](https://fpgasoftware.intel.com/?edition=lite)
* [Quartus Prime Programmer](https://fpgasoftware.intel.com/18.1/?edition=lite&platform=windows&download_manager=dlm3)
* [USB Blaster drivers](https://fpgasoftware.intel.com/18.1/?edition=lite&platform=windows&download_manager=dlm3)
* [OCM-PLD Pack](https://gnogni.altervista.org/ocm/20221127%20OCM-PLD%20Pack%20v3.9.1%20by%20KdL.7z)


## Programming

### FPGA
To program the FPGA you will need a USB Blaster. 
[More info to come]

### ESP8266
To program the ESP8266 you will need a USB to Serial adapter.
[More info to come]

### microSD Card
To use the microSD card you will need... [More info to come]

## License

![Open Hardware](images/ccans.png)

This work is licensed under a [Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International License](http://creativecommons.org/licenses/by-nc-sa/4.0/).

* If you remix, transform, or build upon the material, you must distribute your contributions under the same license as the original.
* You may not use the material for commercial purposes.
* You must give appropriate credit, provide a link to the license, and indicate if changes were made. You may do so in any reasonable manner, but not in any way that suggests the licensor endorses you or your use.

**ATTENTION**

This project was made for the retro community and not for commercial purposes. So only retro hardware forums and individual people can build this project.

THE SALE OF ANY PART OF THIS PROJECT WITHOUT EXPRESS AUTHORIZATION IS PROHIBITED!

