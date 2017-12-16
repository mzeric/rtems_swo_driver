# SWO driver of RTEMS
the driver and rtems have been tested on  stm32f4-discovery board

## Build

```
export RTEMS_MAKEFILE_PATH=/path/to/your/rtems/
cd rtems_swo_driver
make
arm-rtems5-objcopy -Obinary ./o-optimize/timer.exe stm32_rtems_swo_timer.bin
```
## Run

1. use  your flash program tools to program the flash, if you choose openocd

```
> flash write_image erase ./rtems_swo_dirver/stm32_rtems_swo_timer.bin 0x8000000
> reset
```

## Get Output
in the openocd configure:

```
source [find interface/stlink-v2.cfg]
source [find target/stm32f4x.cfg]

transport select hla_swd
reset_config srst_only
itm port 0 on
tpiu config internal swo.log uart off 16000000 2000000
```
the `16000000` is the default clock freq(16MHz) set by rtems,200kHz is the freq of st-link on board

after launch `openocd -f stm32_swo.cfg`

`tail -f swo.log ` and you will watch the output scroll
