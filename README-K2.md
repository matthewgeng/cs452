# K2

## RPS Game Test
First ensure that we are in the correct RPS SHA specified in the PDF or in the k2 branch

```
git checkout k2
```

Enter root directory and `make`

This will create a binary image called `iotest.img`

Upload the image to any pi and upon reboot, the game tests will run.

There are 3 tests, and after each one, we prompt for user input before running the next for visual ease.

## Performance Measurements

First ensure that we are in the correct measurements SHA specified in the PDF or in the k2-measurements branch

`git checkout k2-measurements`

Enter root directory and `make`

This will create a binary image called `iotest.img`

Upload the image to any pi and upon reboot, the tests will run.

Note that without editing the files, the image created will log the performance measurements for all three message sizes in the configuration of receiver-first, -O3 optimization, and both I and D caches turned on. 
