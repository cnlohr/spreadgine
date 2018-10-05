# spreadgine
An OpenGL ES/WebGL Minimal, multi-output engine.

Was originally developed for OrangePi and Linux desktop.  Trying to port to Raspi.


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


hdmi_timings=720 1 16 62 60 480 1 9 6 36 0 0 0 60 0 27027000 5

core_freq=500 # GPU Frequency 
#arm_freq=1000 # CPU Frequency 
sdram_freq=600
sdram_schmoo=0x02000020
```

Using above configuration and git ______ We get ####


