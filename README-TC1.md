# TC1

First ensure that we are in the correct SHA specified in the PDF or in the tc1 branch

```
git checkout tc1
```

Enter root directory and `make`

This will create a binary image called `iotest.img`

Upload the image to any pi and upon reboot, train control will be running.


Commands

`tr *train number* *train speed*` Set train to specified speed

`rv *train_number*` Reverse specified train

`sw *switch_number* *switch_direction*` Change switch to specified direction

`nav *train_number* *sensor* `

(e.g. nav 2 E6)
Change switches to navigate train from current position to specified sensor
Does NOT modify train speed so it assumes the train is already running
Uses calibrated velocity to attempt to stop the train at the specified sensor


`go *train_number* *train_speed* *sensor*`
(e.g. go 2 12 E6)
Set train to specified speed regardless of its current speed
Change switches to navigate train from current position to specified sensor
Uses calibrated velocity to attempt to stop the train at the specified sensor

`track *train_letter*`
(e.g. track a)
Recomputes track_node *track to represent specified track

reset
Resets all switches back to original switch directions


`pf *sensor 1* *sensor 2*`
(e.g. pf B16 E6)
*used for pathfinding debugging/observations*
Prints to console using uart_printf the path and required switch changes to get from sensor 1 to sensor 2

Rest commands are same as K4 including `q` for quit.
