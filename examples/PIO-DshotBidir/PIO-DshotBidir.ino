#include <DshotBidir.h>

#define DSHOT_PIN_BASE 16  // pin for first channel
#define DSHOT_PIN_COUNT 1  // number of channels - 1 to 8
#define DSHOT_RATE 300     // dshot rate - 300, 600, or 1200

DshotBidir dshot;

void setup() {
    Serial.begin(115200);

    dshot.begin(DSHOT_PIN_BASE, DSHOT_PIN_COUNT, DSHOT_RATE);
}

void loop() {
    //3 seconds of 0 throttle
    for(int i = 0; i < 3000; i++) {
        set_throttle_all(0);
    }
//return;
    //throttle up
    for(int i = 0; i < 2000; i++) {
        set_throttle_all(i);
    }

    //throttle down
    for(int i = 2000; i >= 0; i--) {
        set_throttle_all(i);
    }
}

void set_throttle_all(uint16_t thr) {
    //read telemetry before sending command
    Serial.printf("TELEM:");
    for(int i = 0; i < DSHOT_PIN_COUNT; i++) {
        uint32_t tlm_val;
        int tlm_type = dshot.read_telem(i, &tlm_val);
        if(tlm_type < 0) {
            Serial.printf(" err%d", -tlm_type);
        }else if(tlm_type==0) {
            Serial.printf(" erpm=%d", tlm_val);
        }else{
            Serial.printf(" tlm%d=%d", tlm_type, tlm_val);
        }
    }

    Serial.printf(" -- SET_THROTTLE:%4d\n", thr);

    uint16_t arr[DSHOT_PIN_COUNT];
    for(int i = 0; i < DSHOT_PIN_COUNT; i++) {
        arr[i] = thr;
    }
    dshot.set_throttle(arr);
    delay(1);

    static uint32_t ts = millis();
    if(millis() - ts > 100) {
        ts = millis();
        Serial.printf("throttle:%d dshot%d_bidir using pins %d to %d, max refresh rate: %d Hz\n", thr, DSHOT_RATE, DSHOT_PIN_BASE, DSHOT_PIN_BASE + DSHOT_PIN_COUNT - 1, 1000000 / dshot.interval_us); 
    }
}


