# K2

## Making

### RPS Game Test

Confirm that k2 is the current branch

Enter root directory and `make`

This will create a binary image called `iotest.img`

### Performance Measurements

`git checkout k2-measurements`

Enter root directory and `make`

This will create a binary image called `iotest.img`

Note that without editing the files, the image created will log the performance measurements for all three message sizes in the configuration of receiver-first, -O3 optimization, and both I and D caches turned on. 

## Operating

Upload iotest.img to a specific rpi. Program will run after rpi reboot.
