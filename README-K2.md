# K2

## Making

First ensure that we are in the correct branch of `k2`

```
git checkout k2
```

Then ensure to checkout to the correct SHA specified in the PDF documents.

### RPS Game Test

Confirm that k2 is the current branch

Enter root directory and `make`

This will create a binary image called `iotest.img`

Upload the image to any pi and upon reboot, the game tests will run.

There are 3 tests, and after each one, we ask for user input before running the next for visual ease.

### Performance Measurements

`git checkout k2-measurements`

Enter root directory and `make`

This will create a binary image called `iotest.img`

Upload the image to any pi and upon reboot, the tests will run.

Note that without editing the files, the image created will log the performance measurements for all three message sizes in the configuration of receiver-first, -O3 optimization, and both I and D caches turned on. 
