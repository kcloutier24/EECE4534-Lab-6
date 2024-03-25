
# AudioTX Kernel Driver and Usermod Player




In post-lab update the README.md (both kaudio and usermode-player) to include: project description, installation/make instructions, usage definition and usage examples. Treat this stage of implementation as version 0.1 of both modules. Add a discussion section to the README to reflect on any challenges and pitfalls.

Update the repository README.md (which currently includes the ALSA audio instructions) into a project overview across the three introduced modules (with links to the respective directories).






## Project description and intial description

The kaudio module is a kernel-level audio driver designed for handling audio output in Linux environments

This code is a Linux kernel module designed to manage audio data using FIFO buffers 


The structure of the audio driver is more complex than the prelab’s echo driver
The echo driver is a kernel driver with a single instance. Conversely, the audio output is a platform driver,  able to handle multiple instances
In result the audio driver has a two-level initialization, esl_audio_init for the class, and esl_audio_probe for each instance


`extern void esl_codec_enable_tx(struct axi_i2s* i2s)`
    Related to enabling transmission (TX) functionality to set up the codec or I2S controller to start transmitting audio data

`extern void esl_codec_disable_tx(struct axi_i2s* i2s)`
    Related to disabling transmission (TX) functionality and responsible for stopping the transmission of audio data

### Structs

`struct esl_audio_instance`
    An individual instance of the audio driver. Contains information like FIFO registers, character device, device number, interrupt number, FIFO depth, wait queue, and a pointer to an I2S controller instance

`struct esl_audio_driver`
    The class of all audio drivers. It includes information like the first device number, class pointer, instance count, and a list of instances


### Utility functions

`static struct esl_audio_instance* inode_to_instance`
    Maps an inode to its corresponding audio instance

`static struct esl_audio_instance* file_to_instance` 
    Maps a file to its corresponding audio instance


### Interupt handler

`static irqreturn_t esl_audio_irq_handler`
    Interrupt handler function for handling interrupts related to audio operations

`static int esl_audio_probe`
    Function called when the module is probed. Sets up the instance, retrieves resources, initializes the character device, and registers interrupts

`static int esl_audio_remove`
    Function called when the module is removed. Cleans up resources and removes the instance



### Initialization and Exit Functions:
`static int esl_audio_init`
    Initializes the module, allocates the character device region, creates a class, and registers the platform driver

`static void esl_audio_exit`
    Cleans up and unregisters the module, including unregistering the character device region and destroying the class










## Make instructions


- Navigate to the kaudio directory
- Run the `make` command to compile the module
- Scp the `.ko` file to a zedbord or qemu
- SSH into a zedboard or qemu and install the module using the `insmod` command
- Use the command `insmod kaudio.ko && ./player zed.wav` to play the wav audio
- Use dmesg to see the messages printed in the main window and to check that everything is working as intended



## Usage

- The kaudio module allows users to interact with audio hardware through the Linux kernel
- Able to configure audio settings, write audio data to the hardware, and manage audio output streams

## Observations/Design Discussion


During the implementation of the kaudio module, observations were made on the challenges we faced. Among those challenges a dealy issue occurred and the usleep range was set from 6000 to 8300. In this lab it was static while in the previous labs, the usleep range was dynamic and depended on other variables 





