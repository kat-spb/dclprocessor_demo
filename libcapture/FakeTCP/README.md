# libcapture
Capturers for TMK projects

# Build with cmake

```
rm -rf build; mkdir -p build; cd build
cmake ..
make -j$(nproc)
```

# GigE GVCP/GVSP capturer for some devices

Tool fort device discovery:

```
 bin\tmk-finder
```

Utility for test ONE gvcp device

Usage: bin/tmk-test ip [duration]

Examples:

```
bin/tmk-test 10.0.0.2
```

camera test for 4s
```
bin/tmk-test 10.0.0.2 4
```

camera test without limit
```
bin/tmk-test 10.0.0.2 0
```

no camera test, only device callback
```
bin/tmk-test 10.0.0.2 -1
```

# TCP fake opencv camera and capturer

Not supported. Need code update.

Camera:
```
export TEST_SOURCE_PATH="/mnt/testdb/pencil_frames_5" && ./cvcamera 9000
```
Please, wait while source will init

Capturer:
```
./cvcapture localhost 9000
```
