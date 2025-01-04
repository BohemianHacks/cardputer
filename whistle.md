Here's a basic whistle detection program for the M5Stack CardPuter:

```cpp
#include <M5Cardputer.h>
#include <driver/i2s.h>

// Callback type definition
typedef void (*WhistleCallback)(float frequency, uint32_t duration);

class WhistleDetector {
private:
    static const size_t SAMPLE_RATE = 16000;
    static const size_t BUFFER_SIZE = 1024;
    int16_t samples[BUFFER_SIZE];
    WhistleCallback callback;

public:
    WhistleDetector() : callback(nullptr) {}

    void begin() {
        M5Cardputer.begin();
        // Configure I2S for microphone
        i2s_config_t i2s_config = {
            .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
            .sample_rate = SAMPLE_RATE,
            .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
            .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
            .communication_format = I2S_COMM_FORMAT_STAND_I2S,
            .intr_alloc_flags = 0,
            .dma_buf_count = 8,
            .dma_buf_len = BUFFER_SIZE,
            .use_apll = false
        };
        i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
    }

    void setCallback(WhistleCallback cb) {
        callback = cb;
    }

    void update() {
        size_t bytes_read = 0;
        i2s_read(I2S_NUM_0, samples, sizeof(samples), &bytes_read, portMAX_DELAY);
        
        // Very basic threshold detection - replace with FFT for better accuracy
        int16_t peak = 0;
        for(int i = 0; i < BUFFER_SIZE; i++) {
            if(abs(samples[i]) > peak) peak = abs(samples[i]);
        }

        if(peak > 10000 && callback) {  // Arbitrary threshold
            // For now just pass a dummy frequency and duration
            callback(1000.0f, 100);
        }
    }
};
```

Basic usage:
```cpp
WhistleDetector detector;

void onWhistle(float freq, uint32_t dur) {
    Serial.printf("Whistle detected! Freq: %.1f Hz, Duration: %d ms\n", freq, dur);
}

void setup() {
    detector.begin();
    detector.setCallback(onWhistle);
}

void loop() {
    detector.update();
}
```
