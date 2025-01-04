ESP-IDF Setup:

1. First, ensure ESP-IDF is installed (assuming you have this since you mentioned preferring it)

2. ESP-DSP Component:
```bash
cd $IDF_PATH/components
git clone https://github.com/espressif/esp-dsp.git
```
The DSP libraries are actually now included in newer ESP-IDF versions (post 4.4), so you might not need this step if you're on a recent version.

3. M5Cardputer Library:
Create a components directory in your project if it doesn't exist:
```bash
mkdir -p components
cd components
git clone https://github.com/m5stack/M5Cardputer.git
```

4. In your project's main CMakeLists.txt:
```cmake
cmake_minimum_required(VERSION 3.16)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(m5_audio_test)

# Add DSP component (if not using built-in)
set(EXTRA_COMPONENT_DIRS $ENV{IDF_PATH}/components/esp-dsp)
```

5. In your component's CMakeLists.txt (create in main directory):
```cmake
idf_component_register(
    SRCS "main.cpp"
    INCLUDE_DIRS "."
    REQUIRES "M5Cardputer" "esp-dsp" "driver"
)
```

Then you can build with:
```bash
idf.py build
```
