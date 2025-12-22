# Moonlight Mating

## Stack Overflow
```
2025-12-22 22:00:01 End switch 36 value = 0, delta = 0
2025-12-22 22:00:02 End switch 36 value = 0, delta = 0
Guru Meditation Error: Core  0 panic'ed (Unhandled debug exception). 
Debug exception reason: Stack canary watchpoint triggered (Control Stepper) 
Core  0 register dump:
PC      : 0x40090377  PS      : 0x00060036  A0      : 0x8008ea88  A1      : 0x3ffcf200  
A2      : 0x3ffbf618  A3      : 0xb33fffff  A4      : 0x0000abab  A5      : 0x00060023  
A6      : 0x00060023  A7      : 0x0000cdcd  A8      : 0xb33fffff  A9      : 0xffffffff  
A10     : 0x3ffbf604  A11     : 0x00000001  A12     : 0x00060021  A13     : 0x80000000  
A14     : 0x007bf618  A15     : 0x003fffff  SAR     : 0x00000015  EXCCAUSE: 0x00000001  
EXCVADDR: 0x00000000  LBEG    : 0x4008a871  LEND    : 0x4008a881  LCOUNT  : 0xfffffffa  


Backtrace: 0x40090374:0x3ffcf200 0x4008ea85:0x3ffcf240 0x4008d090:0x3ffcf270 0x4008d040:0xa5a5a5a5 |<-CORRUPTED
```

This stack overflow happened in the "Control Stepper Motor []" task. Watch stack high water mark with
```c
    uxHighWaterMark = uxTaskGetStackHighWaterMark(NULL);
    Serial.printf("%s uxHighWaterMark %d\n", getDateTime().c_str(), uxHighWaterMark);
```
in the main loop of the task.

# Apendix
```bash
find data/ include/ src/ -type f | grep -E 'cpp$|h$|html$|css$|js$|json$' | xargs wc
```
