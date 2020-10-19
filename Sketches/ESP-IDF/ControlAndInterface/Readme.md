# Building TensorFlow

In the tensorflow repo:

```
git clone https://github.com/tensorflow/tensorflow --recursive
cd tensorflow
git checkout r2.3
sudo apt-get install unzip
source /mnt/c/dev/esp-idf/export.sh
make -f tensorflow/lite/micro/tools/make/Makefile TARGET=esp32 generate_hello_world_make_project
```

You should now have the correct files at C:\dev\tensorflow\tensorflow\lite\micro\tools\make\gen\esp32_x86_64\prj\hello_world\make


If you have issues with `min.h` and `max.h` change `std::fmin` to `fmin`, etc

# CAN.h change

We change line 108 of can.h from:

```cpp
#define CAN_IO_UNUSED                   (-1)        /**< Marks GPIO as unused in CAN configuration */
```

to

```cpp
#define CAN_IO_UNUSED                   GPIO_NUM_MAX        /**< Marks GPIO as unused in CAN configuration */
```
