# Usermode-player



In post-lab update the README.md (both kaudio and usermode-player) to include: project description, installation/make instructions, usage definition and usage examples. Treat this stage of implementation as version 0.1 of both modules. Add a discussion section to the README to reflect on any challenges and pitfalls.

Update the repository README.md (which currently includes the ALSA audio instructions) into a project overview across the three introduced modules (with links to the respective directories).



## Project description and intial description

The usermode-player module is a user-space audio player designed for playing audio files in Linux environments


- The codec configuration is handled mostly automatically by standard kernel drivers that are already in place. Initialization is required in order to set the correct playback parameters such as sample rate
- Two functions, `int i2s_enable_tx(void)` and `int i2s_disable_tx(void)`, were created to enable and disable the TX portion of the I2S controller
- The `codec_setup function` was implemented using the `snd_pcm_hw_params_set_format` function to set the sample format
- The channel count was set using the `snd_pcm_hw_params_set_channels` function, using 2 channels (stereo) as this is fixed by the I2S controller
- The sample rate was set with the `snd_pcm_hw_params_set_rate_near` function. It was noted that he approximate sampling rate might be different from the requested rate
- The configuration was written to the hardware with the `snd_pcm_hw_params` function



## Make instructions


- Cd into the `usermode-hw-player` folder
- Type `$CC player_user.c -o player` into a Linux command line
- Transfer the file `player` over to the Zedboard via SCP
- Use the command `insmod kaudio.ko && ./player zed.wav` to play the wav audio



## Usage

- The usage allows users to play audio files from the command line or through a graphical user interface
- Able to pecify the audio file to play and adjust playback settings as needed



## Observations/Design Discussion

During the implementation of the usermode-player module, several observations were made from the challenges we faced. Among those challenges, we observed a new file directory to be opened being `/dev/zedaudio0`. The purpose of opening this was to write the word into the fifo and play the audio. 
