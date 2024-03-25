# Lab 6 Kernel Mode Audio TX


See the lab instructions [here](LabInstructions.md).


## Release 0.1 Documentation: AudioTX Kernel Driver and Usermod Player


Update the repository README.md (which currently includes the ALSA audio instructions) into a project overview across the three introduced modules (with links to the respective directories).





### Project description
This code is a Linux kernel module designed to manage audio data using FIFO buffers ([Lab Instructions](LabInstructions.md))

See the kaudio Module [here](/kaudio/README.md).

See the usermode-player [here](/kaudio/usermode-player/README.md).




## Release 0.2 Documentation: Timed Polling


### Performance Impacts
What is the difference between the write and fwrite C functions? 

- The write function is a call to the operating system made by your application, it is slower than fwrite()
- It lacks buffering, which makes it slow
- Write is atomic whereas fwrite is not
- The write function is a system call used for writing data to a file descriptor
- fwrite is a standard library function used for writing to file streams
- For character device writing, write is more common and may have better performance due to fewer abstraction layers


How does the polling interval affect the system performance? How is it different from the case when the polling was done in the userspace application?

- Shorter polling intervals increase CPU utilization but reduce latency
- Longer intervals may increase wait times. 
- Polling in the kernel may be more efficient than userspace polling due to reduced context switch overhead


How can you use this interrupt to avoid polling in the blocking write call?

- You can use an interrupt to avoid polling in the blocking write call by only polling if the interrupt is called
- When enabling the interrupt and replacing the usleep and polling statements in the write function with the kernel macro `wait_event_interruptible` and using the `wake_up` macro we observe the empty threshold triggered. We observed the solution is helpful because it is independent of the sample rate. This approach is different from previous ones because you do not have to poll. 




