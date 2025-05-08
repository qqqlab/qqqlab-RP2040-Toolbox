#include <DshotParallel.h>

#define DSHOT_PIN_BASE 16
#define DSHOT_PIN_COUNT 8
#define DSHOT_RATE 300

DshotParallel dshot;

void ds(uint16_t v, uint32_t ms) {
    Serial.printf("Dshot %d for %d ms\n", v, ms);
    //3 seconds of 0
    uint16_t values[DSHOT_PIN_COUNT];
    for(int i = 0; i < DSHOT_PIN_COUNT; i++) {
        values[i] = v; 
    }
    uint32_t ts = millis();
    while(millis() - ts < ms) {
        dshot.write(values);
    }
}

void setup() {
    Serial.begin(115200);

    dshot.begin(DSHOT_PIN_BASE, DSHOT_PIN_COUNT, DSHOT_RATE);
}

void loop() {
    //3 seconds of 0 throttle
    for(int i = 0; i < 3000; i++) {
        set_throttle_all(0);
    }

    //throttle up
    for(int i = 0; i < 2000; i++) {
        set_throttle_all(i);
    }

    //throttle down
    for(int i = 2000; i >= 0; i--) {
        set_throttle_all(i);
    }
    return;
}

void set_throttle_all(uint16_t thr) {
    uint16_t arr[DSHOT_PIN_COUNT];
    for(int i = 0; i < DSHOT_PIN_COUNT; i++) {
        arr[i] = thr;
    }
    dshot.set_throttle(arr);
    delay(1);

    static uint32_t ts = millis();
    if(millis() - ts > 100) {
        ts = millis();
        Serial.printf("throttle:%d dshot%d using pins %d to %d, max refresh rate: %d Hz\n", thr, DSHOT_RATE, DSHOT_PIN_BASE, DSHOT_PIN_BASE + DSHOT_PIN_COUNT - 1, 1000000 / dshot.interval_us); 
    }
}


