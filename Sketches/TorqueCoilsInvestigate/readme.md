A run looks something like this:

```
C:\dev\MuscleMemory\Sketches\TorqueCoilsInvestigate>mode com3: BAUD=115200
Default to 7 data bits.
Default to even parity.

Status for device COM3:
-----------------------
    Baud:            115200
    Parity:          Even
    Data Bits:       7
    Stop Bits:       1
    Timeout:         OFF
    XON/XOFF:        OFF
    CTS handshaking: OFF
    DSR handshaking: OFF
    DSR sensitivity: OFF
    DTR circuit:     OFF
    RTS circuit:     OFF


C:\dev\MuscleMemory\Sketches\TorqueCoilsInvestigate>type com3: > run_4_pos.csv
The I/O operation has been aborted because of either a thread exit or an application request.

C:\dev\MuscleMemory\Sketches\TorqueCoilsInvestigate>type com3: > run_4_neg.csv
The I/O operation has been aborted because of either a thread exit or an application request.
```

Then remove the first and last lines of each file (they tend to be incomplete and will fail when parsing).

Then edit the filenames in plot.py and run