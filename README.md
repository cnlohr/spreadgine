# spreadgine
An OpenGL ES/WebGL Minimal, multi-output engine.

Was originally developed for OrangePi and Linux desktop.  Trying to port to Raspi.

## editing /etc/rc.local

```
vcgencmd hdmi_timings 2160 1 40 20 46 1200 1 28 2 234 0 0 0 90 0 297000000 5 && tvservice -e "DMT 87"
echo performance > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor

echo 7 > /sys/class/gpio/export
echo out > /sys/class/gpio/gpio7/direction
echo 1 > /sys/class/gpio/gpio7/value
```

## Disable agetty

```systemctl disable getty@tty1.service```


## Current raspi settings

```
dtparam=audio=on

hdmi_pixel_freq_limit=400000000
hvs_priority=0x32ff
gpu_mem=256
   #hdmi_ignore_edid=0xa5000080

max_framebuffer_width=2160
max_framebuffer_height=1200
framebuffer_width=2160
framebuffer_height=1200

hdmi_force_hotplug=1
config_hdmi_boost=2  #0..11

dpi_group=2
dpi_mode=87
hdmi_drive=2  #Must be 2!
hdmi_group=2  #DMT
hdmi_mode=87


hdmi_timings=720 1 16 62 60 480 1 9 6 36 0 0 0 60 0 27027000 5  #Initial, will be changed.
```

```root@rasvive:~# cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq 1400000```

```vcgencmd measure_clock core
frequency(1)=600000000```

```vcgencmd measure_clock isp
frequency(42)=300000000```

Using above configuration and git fd7d7f6 We get 43 FPS.

At 500 MHz core frequency, 42-43 fps.

At 600Core/1.5GHzArm/600MHzRAM, 43-44 FPS.

At that, with a heat sink and a slow fan right above...
```vcgencmd measure_temp
temp=40.8'C```

The following is a little unstable:
```
core_freq=600 # GPU Frequency 
arm_freq=1500 # CPU Frequency 
sdram_freq=700
sdram_schmoo=0x02000020
over_voltage=6 #Electric power sent to CPU / GPU (4 = 1.3V) 
```

RECOMMEND: Do not run ARM at past 1.4GHz.



Just FYI to print all clocks:
```for src in arm core h264 isp v3d uart pwm emmc pixel vec hdmi dpi ; do echo -e "$src:\t$(vcgencmd measure_clock $src)" ; done```


WOAHH!!!

```
core_freq=600 # GPU Frequency 
arm_freq=1400 # CPU Frequency 
sdram_freq=700
sdram_schmoo=0x02000020
over_voltage=6 #Electric power sent to CPU / GPU (4 = 1.3V) 
#disable_splash=1 # Disables the display of the electric alert screen
gpu_freq=350
```
GETS ME 50 FPS! [at 45.1 degrees C]
gpu @ 400 = 55 FPS.
gpu @ 450 = 60 FPS.
gpu @ 500 = 65 FPS.  (46 degrees C)
gpu @ 550 = 70 FPS.  : SEEMS SOLID.
gpu @ 600 = 75 FPS.  : SOMETIMES THIS FAILS.
gpu @ 650 = 60 FPS :( 

Hmm... I seem to be getting internal compiler errors.  Trying to pull down overvolt and sdram.

This seems rock solid:
```
core_freq=600 # GPU Frequency 
arm_freq=1400 # CPU Frequency 
sdram_freq=650
sdram_schmoo=0x02000020
over_voltage=5 #Electric power sent to CPU / GPU (4 = 1.3V) 
gpu_freq=550
```

And I still get 70 fps on this version's test spread.





