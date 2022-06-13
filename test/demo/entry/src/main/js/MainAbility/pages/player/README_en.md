## Functions

This demo shows the functions of playing, pausing, playing from scratch, switching audio source, progress display,
rendering rate setting, volume setting, (un)mute, displaying AudioRenderer and AudioManager information.

## Instructions

After the program runs, we need to send the pcm files to the device. The commands are as follows. (The first parameter
after "send" is the path of the local file)

```
hdc_std file send \test.pcm /data/app/el2/100/base/com.example.audioplayerdemo/haps/entry/files/

hdc_std file send \test2.pcm /data/app/el2/100/base/com.example.audioplayerdemo/haps/entry/files/
```