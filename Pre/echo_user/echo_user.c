#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/signal.h>




volatile static unsigned char stopLoop = 0;


/* @brief Handle signals
   @param num Signal Number */
static void signal_handler(int num)
{
  stopLoop = 1;
}

int main(void)
{ 
    
    // handle SIGINT (ctrl-c)
  signal(SIGINT, signal_handler);

          int x = system("rmmod echo.ko");
     x = system("insmod echo.ko");

    x = system("echo Hello from C > /dev/echo0");

  return 0;

}