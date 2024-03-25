# Lab 5: Audio TX User Mode



## Release 0.1 Documentation: User-mode Player


### Project description

- In this section we will play sounds using the audio codec on the board. The pre-lab simulates this functionality by pushing data samples to the hardware FIFO, we have chosen Jareds prelab.

- The codec configuration is handled mostly automatically by standard kernel drivers that are already in place. Initialization is required in order to set the correct playback parameters such as sample rate. 
- Two functions, `int i2s_enable_tx(void)` and `int i2s_disable_tx(void)`, were created to enable and disable the TX portion of the I2S controller.
- The `codec_setup function` was implemented using the `snd_pcm_hw_params_set_format` function to set the sample format.
- The channel count was set using the `snd_pcm_hw_params_set_channels` function, using 2 channels (stereo) as this is fixed by the I2S controller.
- The sample rate was set with the `snd_pcm_hw_params_set_rate_near` function. It was noted that he approximate sampling rate might be different from the requested rate.
- The configuration was written to the hardware with the `snd_pcm_hw_params` function.


### Make instructions

- Cd into the `usermode-hw-player` folder
- Type `$CC player_user.c -o player` into a Linux command line
- Transfer the file `player` over to the Zedboard via SCP

### Usage

- Use `./player` to run the file


### Observations/Design Discussion

- Writing to the audio output FIFO bears challenges due to real-time constraints. Audio samples have to be written within a regular time, otherwise distortion will be audible.
- The codec must be properly configured before it is ready for playback.

- It was noted that he approximate sampling rate might be different from the requested rate.
    - The approximate sampling rate was observed to be 11025 while the requested sampling rate was observed to be 11025. It was observed that it was giving the same rate as we were requesting

- Once initialized the sound was played. 
- Upon first observations the sound played with a substatntial amount of noise 
- If the FIFO vacancy check is disabled before writing to the AXI FIFO then the observed sound is a breif second of audio and an a bus error was reported, killing the script
- The player impacts the CPU due to the constant writing of samples, thus taking up processing resources. If processor was busy the samples would need to be a higher priority to be written on time 





## Release 0.2 Documentation: Timed Polling


### Project description

- User mode player fills the FIFO as soon as there is any vacancy. However, this is not necessary since it takes some time until the CODEC has played the FIFO’s contents. To increase efficiency this section illustrates timed polling to only fill the FIFO when needed.

- The Pre-lab calculated maximum sleep period, 20952 ms,  before needing to fill the FIFO again with a 44.1 kHz sampling rate. 
- It was calculated that the new sleep period would 0.0838 ms for a 11025Hz frequency by using (924/sampling rate)*(frequency)
- A WAVE file with a higher sampling rate of 44100 Hz or 48000 Hz was downloaded and played using the delay value calculated.


### Observations/Design Discussion
- We were able to use a sound recording that was at 44100Hz an signed 8 bit by adding 127 to the buffer
- Upon implementing the new sleep period with the 11025Hz frequency the audio was observed to play clearly  
- When using a wave file with a higher sampling rate of 44100 Hz or 48000 Hz the sound is choppy becasue they delay is too long for the faster sampling rate 












