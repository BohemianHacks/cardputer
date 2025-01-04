#include <M5Cardputer.h>
#include <driver/i2s.h>
#include "esp_dsp.h"

typedef void (*WhistleCallback)(float frequency, uint32_t duration);

class WhistleDetector {
private:
    static const size_t SAMPLE_RATE = 16000;
    static const size_t BUFFER_SIZE = 1024;  // Must be power of 2
    static const size_t FFT_SIZE = BUFFER_SIZE / 2;
    
    // FFT buffers in DMA-capable memory
    float* fft_window = nullptr;
    float* fft_input = nullptr;
    float* fft_output = nullptr;
    esp_dsp_fft2r_fc32_t fft_config;
    
    // Sample buffer
    int16_t samples[BUFFER_SIZE];
    WhistleCallback callback;
    
    // Whistle detection parameters
    static const float MIN_WHISTLE_FREQ = 500.0;   // Hz
    static const float MAX_WHISTLE_FREQ = 5000.0;  // Hz
    static const float MIN_MAGNITUDE = 50000.0;    // Adjusted for ESP32 DSP
    static const uint32_t MIN_DURATION = 100;      // ms
    
    // Whistle state tracking
    bool whistleActive = false;
    float currentFreq = 0.0;
    uint32_t whistleStartTime = 0;
    uint32_t lastWhistleTime = 0;

    void initFFT() {
        // Allocate DMA capable memory
        fft_window = (float*)heap_caps_malloc(BUFFER_SIZE * sizeof(float), MALLOC_CAP_8BIT);
        fft_input = (float*)heap_caps_malloc(BUFFER_SIZE * sizeof(float), MALLOC_CAP_8BIT);
        fft_output = (float*)heap_caps_malloc(BUFFER_SIZE * sizeof(float), MALLOC_CAP_8BIT);
        
        if (!fft_window || !fft_input || !fft_output) {
            log_e("Failed to allocate FFT buffers");
            return;
        }
        
        // Initialize Hanning window
        dsps_wind_hann_f32(fft_window, BUFFER_SIZE);
        
        // Initialize FFT
        dsps_fft2r_init_fc32(&fft_config, BUFFER_SIZE);
    }

    float findDominantFrequency() {
        // Convert samples to float and apply window
        for (int i = 0; i < BUFFER_SIZE; i++) {
            fft_input[i] = (float)samples[i] * fft_window[i];
        }
        
        // Perform FFT
        dsps_fft2r_fc32(&fft_config, fft_input, fft_output);
        
        // Convert to power spectrum
        dsps_cplx2reC_fc32(fft_output, BUFFER_SIZE);
        
        // Find peak frequency in whistle range
        float maxMagnitude = MIN_MAGNITUDE;
        int maxIndex = -1;
        
        for (int i = 2; i < FFT_SIZE; i++) {
            float frequency = (float)i * SAMPLE_RATE / BUFFER_SIZE;
            if (frequency >= MIN_WHISTLE_FREQ && frequency <= MAX_WHISTLE_FREQ) {
                float magnitude = fft_output[i];
                if (magnitude > maxMagnitude) {
                    maxMagnitude = magnitude;
                    maxIndex = i;
                }
            }
        }

        if (maxIndex != -1) {
            return (float)maxIndex * SAMPLE_RATE / BUFFER_SIZE;
        }
        return 0.0;
    }

public:
    WhistleDetector() : callback(nullptr) {}
    
    ~WhistleDetector() {
        if (fft_window) heap_caps_free(fft_window);
        if (fft_input) heap_caps_free(fft_input);
        if (fft_output) heap_caps_free(fft_output);
    }

    void begin() {
        M5Cardputer.begin();
        
        // Initialize FFT
        initFFT();
        
        // Configure I2S
        i2s_config_t i2s_config = {
            .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
            .sample_rate = SAMPLE_RATE,
            .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
            .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
            .communication_format = I2S_COMM_FORMAT_STAND_I2S,
            .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
            .dma_buf_count = 8,
            .dma_buf_len = BUFFER_SIZE,
            .use_apll = false,
            .tx_desc_auto_clear = false,
            .fixed_mclk = 0
        };
        
        // CardPuter I2S pins
        i2s_pin_config_t pin_config = {
            .bck_io_num = 41,
            .ws_io_num = 42,
            .data_out_num = -1,
            .data_in_num = 40
        };
        
        ESP_ERROR_CHECK(i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL));
        ESP_ERROR_CHECK(i2s_set_pin(I2S_NUM_0, &pin_config));
    }

    void setCallback(WhistleCallback cb) {
        callback = cb;
    }

    void update() {
        size_t bytes_read = 0;
        i2s_read(I2S_NUM_0, samples, sizeof(samples), &bytes_read, portMAX_DELAY);

        float freq = findDominantFrequency();
        uint32_t currentTime = millis();

        if (freq > 0.0) {
            if (!whistleActive) {
                whistleActive = true;
                whistleStartTime = currentTime;
                currentFreq = freq;
            } else {
                // Update frequency with hysteresis
                if (abs(freq - currentFreq) < 200.0) {
                    currentFreq = (currentFreq * 0.7 + freq * 0.3);
                }
            }
            lastWhistleTime = currentTime;
        } else if (whistleActive && (currentTime - lastWhistleTime > 50)) {
            uint32_t duration = currentTime - whistleStartTime;
            if (duration >= MIN_DURATION && callback) {
                callback(currentFreq, duration);
            }
            whistleActive = false;
        }
    }
};
