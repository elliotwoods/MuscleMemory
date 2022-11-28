# Introduction

An open source 'industrial servo motor' which employs Reinforcement Learning to train itself how best to drive a load. The user can decide in this case what is meant by 'best', e.g. speed, accuracy or energy efficiency. The model is run locally on a low cost microcontroller (i.e. ESP32), but is trained remotely where more computational resources are available (e.g. in the cloud).

![image](.github/introduction.png)

# Project status

If you would like to get involved with development and prototyping then please get in touch via elliot@kimchiandchips.com.

## Update 2021-05
* Operational firmware with CAN control (check Firmware folder)
* Early testing with the reinforcement learning engine (MVP)
* Our first installation is out in the wild with 'Muscle Memory V2'. You can see some images/video here:
    * https://www.instagram.com/p/CIBZggRlJ9w/
    * https://www.instagram.com/p/CHZwTcIlO6C/
    * https://www.instagram.com/p/CH5BXcqFXb2/
    * https://www.instagram.com/p/CF10joaFSJC/
* We are testing 'Muscle Memory v3' which is a high power (4A / 48V) design with brake control, end stops
* We have a design for a low cost version which aims for a fully working set (controller + motor) for 30 USD called 'Muscle Memory Minimal'


# Reinforcement learning

## Network architecture

* Server
  * High performance hardware (e.g. desktrop CPU + GPU)
  * FastAPI REST service
  * TensorFlow implemented RL algorithms (e.g. DDPG / NAF)
* Client
  * Low cost hardware (e.g. ESP32 microcontroller)
  * MicroPython
  * [TensorFlow Lite module](https://github.com/elliotwoods/micropython/tree/elliot-modules/modules/tensorflow)
  * (download model from server, run actor, gather samples, send to server) : repeat

# Prior work

Muscle Memory builds on the work of previous projects, most notably [Mechaduino](https://github.com/jcchurch13/Mechaduino-Firmware) by Tropical Labs. A list of other open source motor driver projects can be seen at https://github.com/cajt/list_of_robot_electronics

# Credits

Muscle Memory is a project of Kimchi and Chips art studio, and is partly funded by the Arts Council of Korea via the State, Action, Reward project.
