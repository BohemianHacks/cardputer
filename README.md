# cardputer

# Project Structure and Dependencies

External dependencies to clone:
```bash
# In your project's components directory:
git clone https://github.com/m5stack/M5Cardputer.git
```

# File: CMakeLists.txt
```
cmake_minimum_required(VERSION 3.16)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(m5_audio_test)
```
# File: main/CMakeLists.txt
```
idf_component_register(
    SRCS "main.cpp"
    INCLUDE_DIRS "."
    REQUIRES "M5Cardputer" "esp-dsp" "driver"
)
```

# File: main/main.cpp
```
#include <M5Cardputer.h>
#include "esp_dsp.h"
#include "driver/i2s.h"

// DSP constants
#define SAMPLE_RATE     16000
#define FFT_SIZE        1024
#define SAMPLE_BUFFER   1024

// I2S config for M5Cardputer's microphone
#define I2S_WS_PIN      41
#define I2S_SD_PIN      42
#define I2S_SCK_PIN     40

// Buffer for audio samples
static int16_t samples[SAMPLE_BUFFER];
// Buffer for FFT calculation
static float fft_input[FFT_SIZE*2];
static float fft_output[FFT_SIZE*2];

void setup() {
    M5Cardputer.begin();
    M5Cardputer.Display.setRotation(1);
    M5Cardputer.Display.fillScreen(BLACK);
    M5Cardputer.Display.setTextSize(1);

    // Initialize I2S for the microphone
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 4,
        .dma_buf_len = 1024,
        .use_apll = false,
        .tx_desc_auto_clear = false,
        .fixed_mclk = 0
    };
    
    i2s_pin_config_t pin_config = {
        .mck_io_num = I2S_PIN_NO_CHANGE,
        .bck_io_num = I2S_SCK_PIN,
        .ws_io_num = I2S_WS_PIN,
        .data_out_num = I2S_PIN_NO_CHANGE,
        .data_in_num = I2S_SD_PIN
    };

    i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
    i2s_set_pin(I2S_NUM_0, &pin_config);

    // Initialize DSP
    dsps_fft2r_init_fc32(NULL);
}

void loop() {
    size_t bytes_read = 0;
    
    // Read audio samples
    i2s_read(I2S_NUM_0, samples, sizeof(samples), &bytes_read, portMAX_DELAY);

    // Prepare FFT input
    for (int i = 0; i < FFT_SIZE; i++) {
        fft_input[i*2] = (float)samples[i] / 32768.0f;    // Real part
        fft_input[i*2 + 1] = 0.0f;                        // Imaginary part
    }

    // Perform FFT
    dsps_fft2r_fc32(fft_input, FFT_SIZE);
    dsps_bit_rev_fc32(fft_input, FFT_SIZE);
    dsps_cplx2reC_fc32(fft_input, FFT_SIZE);

    // Find dominant frequencies
    M5Cardputer.Display.fillScreen(BLACK);
    M5Cardputer.Display.setCursor(0, 0);
    M5Cardputer.Display.println("Dominant Frequencies:");

    // Only look at first half of FFT output (Nyquist)
    for (int i = 0; i < FFT_SIZE/2; i++) {
        float magnitude = sqrt(fft_input[i*2] * fft_input[i*2] + 
                             fft_input[i*2 + 1] * fft_input[i*2 + 1]);
        
        // Convert bin to frequency
        float frequency = (float)i * SAMPLE_RATE / FFT_SIZE;
        
        // Print top 5 frequencies with significant magnitude
        if (magnitude > 0.1) {  // Arbitrary threshold
            M5Cardputer.Display.printf("%0.1f Hz: %0.2f\n", frequency, magnitude);
        }
    }

    delay(100);  // Small delay to keep display readable
}
```
# Build Instructions:
1. Create project directory:
   ```bash
   mkdir m5_audio_test
   cd m5_audio_test
   ```

2. Create directory structure:
   ```bash
   mkdir -p main components
   ```

3. Create all files as shown above in their respective locations

4. Clone M5Cardputer library:
   ```bash
   cd components
   git clone https://github.com/m5stack/M5Cardputer.git
   cd ..
   ```

5. Build project:
   ```bash
   idf.py build
   ```

6. Flash to device:
   ```bash
   idf.py -p (PORT) flash monitor
   ```
