# GigE: udpcapture
Capturers for DCL projects

# Build with cmake

```
rm -rf build; mkdir -p build; cd build
cmake ..
make -j$(nproc)
```

# GigE GVCP/GVSP capturer for some devices

Tool fort device discovery:

```
 bin\dcl-finder
```

Utility for test ONE gvcp device

Usage: bin/dcl-udpcapture-test ip [duration]

Examples:

```
bin/dcl-udpcapture-test 10.0.0.2
```

camera test for 4s
```
bin/dcl-udpcapture-test 10.0.0.2 4
```

camera test without limit
```
bin/dcl-udpcapture-test 10.0.0.2 0
```

no camera test, only device callback
```
bin/dcl-udpcapture-test 10.0.0.2 -1
```
