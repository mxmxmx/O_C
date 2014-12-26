#include <spi4teensy3.h>
#include <Arduino.h>

/*make sure this is a power of two. */
#define mbxs 1024
static uint8_t My_Buff_x[mbxs]; /* buffer */

extern "C" {

        int _write(int fd, const char *ptr, int len) {
                int j;
                for (j = 0; j < len; j++) {
                        if (fd == 1)
                                Serial.write(*ptr++);
                        else if (fd == 2)
                                Serial3.write(*ptr++);
                }
                return len;
        }

        int _read(int fd, char *ptr, int len) {
                if (len > 0 && fd == 0) {
                        while (!Serial.available());
                        *ptr = Serial.read();
                        return 1;
                }
                return 0;
        }

#include <sys/stat.h>

        int _fstat(int fd, struct stat *st) {
                memset(st, 0, sizeof (*st));
                st->st_mode = S_IFCHR;
                st->st_blksize = 1024;
                return 0;
        }

        int _isatty(int fd) {
                return (fd < 3) ? 1 : 0;
        }
}

void setup() {
        uint32_t wt;
        uint32_t start;
        uint32_t end;
        uint32_t ii = 10485760LU / mbxs;
        spi4teensy3::init(); // full speed, cpol 0, cpha 0
        while (!Serial);
        printf_P(PSTR("\r\n\n\n\n\n\n\n\n\n\n\n\n\n\n\nStart\r\n"));
        Serial.flush();
        delay(2);
        start = millis();
        while (start == millis());
        for (; ii > 0LU; ii--) {
                spi4teensy3::send(My_Buff_x, mbxs);
        }

        end = millis();

        wt = (end - start) - 1;
        printf_P(PSTR("Time to write 10485760 bytes: %lu ms (%lu sec) \r\n"), wt, (500 + wt) / 1000UL);
        float bpms = 10485760.0 / wt;
        printf_P(PSTR("%f Bytes/msec\r\n%f Bytes/sec\r\n"), bpms, bpms * 1000);
}

void loop() {
}