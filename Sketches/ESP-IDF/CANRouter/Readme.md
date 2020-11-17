# CAN.h change

We change line 108 of can.h from:

```cpp
#define CAN_IO_UNUSED                   (-1)        /**< Marks GPIO as unused in CAN configuration */
```

to

```cpp
#define CAN_IO_UNUSED                   GPIO_NUM_MAX        /**< Marks GPIO as unused in CAN configuration */
```
