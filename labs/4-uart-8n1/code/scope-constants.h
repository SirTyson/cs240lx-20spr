// expected number of cycles between transitions.
#define CYCLE_PER_FLIP 6000
#define BAUD_RATE 115200
#define MONITOR_PIN 21
#define IN_0  26
#define OUT_0 20
#define IN_1  19
#define OUT_1 16
#define cycles_per_bit() 700000000 / BAUD_RATE

#define START_BIT 0
#define STOP_BIT 1
#define TIMEOUT 5 * 1000 * 1000
#define DELAY cycles_per_bit() / 2
#define BALL_TEST_N 8
#define BALL 0xab