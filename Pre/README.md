# Prelab for Lab 6: Character Device Files

This pre-lab develops a simple character device driver that echos the same message back. It prepares for writing driver for the audio transmission as well as the corresponding user app. The pre-lab programs can be executed on QEMU.

# Reading List

- ApriorIT, Linux Driver Tutorial: How to Write a Simple Linux Device Driver, https://www.apriorit.com/dev-blog/195-simple-driver-for-linux-os
- Chapter 3: "Char Drivers" in Linux Device Drivers, Third Edition, O’Reilly, 2005 https://lwn.net/Kernel/LDD3/
- Chapter 5: Concurrency and Race Condition in Linux Device Drivers, Third Edition, O’Reilly, 2005 https://lwn.net/Kernel/LDD3/

## Overview 

The instructions for ths lab are detailed in the following steps:

 1. (Reserved for feedback branch pull request. You will receive top level feedback there). 
 2. [Initial Driver with empty Read and Write Functions](.github/STARTING_ISSUES/2.%20Initial%20Driver%20with%20empty%20Read%20and%20Write%20Functions.md)
 3. [Print User Supplied Message from Driver](.github/STARTING_ISSUES/3.%20Print%20User%20Supplied%20Message%20from%20Driver.md)
 4. [Handling Long Messages](.github/STARTING_ISSUES/4.%20Handling%20Long%20Messages.md)
 5. [User-Level Interface to Character Device](.github/STARTING_ISSUES/5.%20User-Level%20Interface%20to%20Character%20Device.md)
 6. [Echo loopback driver](.github/STARTING_ISSUES/6.%20Echo%20loopback%20driver.md)

After accepting this assignment in github classroom, each step is converted into a [github issue](https://docs.github.com/en/issues). Follow the issues in numerically increasing issue number (the first issue is typically on the bottom of the list). 

## General Rules

Please commit your code frequently or at e very logical break. Each commit should have a meaningful commit message and [cross reference](https://docs.github.com/en/get-started/writing-on-github/working-with-advanced-formatting/autolinked-references-and-urls#issues-and-pull-requests) the issue the commit belongs to. Ideally, there would be no commits without referencing to a github issue. 

Please comment on each issue with the problems faced and your approach to solve them. Close an issue when done. 
