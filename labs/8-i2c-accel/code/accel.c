// engler, cs240lx initial driver code.
//
// everything is put in here so it's easy to find.  when it works,
// seperate it out.
//
// KEY: document why you are doing what you are doing.
//  **** put page numbers for any device-specific things you do ***
//  **** put page numbers for any device-specific things you do ***
//  **** put page numbers for any device-specific things you do ***
//  **** put page numbers for any device-specific things you do ***
//  **** put page numbers for any device-specific things you do ***
//  **** put page numbers for any device-specific things you do ***
//  **** put page numbers for any device-specific things you do ***
// 
// also: a sentence or two will go a long way in a year when you want 
// to re-use the code.
#include "rpi.h"
#include "i2c.h"
#include "lsm6ds33.h"
#include <limits.h>

/**********************************************************************
 * some helpers
 */

/* All page numbers from LSM6DS33: always-on 3D accelerometer and 3D gyroscope */

/* Values from default startup sequence pg. 23 */
enum { ENABLE_ALL_AXES   = 0x38,
       READY_DATA_INTER  = 0x01,
       XLDA_BITMASK      = 1   ,
       READS_PER_POLL    = 6   ,   /* number of axes * 2 */
       ENABLE_BDU        = 1<<6,
       DRDY_MASK         = 0b1000,
      };

enum { VAL_WHO_AM_I      = 0x69, };

// read register <reg> from i2c device <addr>
uint8_t imu_rd(uint8_t addr, uint8_t reg) {
    i2c_write(addr, &reg, 1);
        
    uint8_t v;
    i2c_read(addr,  &v, 1);
    return v;
}

// write register <reg> with value <v> 
void imu_wr(uint8_t addr, uint8_t reg, uint8_t v) {
    uint8_t data[2];
    data[0] = reg;
    data[1] = v;
    i2c_write(addr, data, 2);
}

// <base_reg> = lowest reg in sequence. --- hw will auto-increment 
// if you set IF_INC during initialization.
int imu_rd_n(uint8_t addr, uint8_t base_reg, uint8_t *v, uint32_t n) {
    i2c_write(addr, (void*) &base_reg, 1);
    int read = 0;
    for (size_t i = 0; i < n; i++)
        *(v + i) = imu_rd(addr, base_reg + i);
    return n;
    //return i2c_read(addr, v, n);
}

/**********************************************************************
 * simple accel setup and use
 */

static int divide(int nu, int de) {

    int temp = 1;
    int quotient = 0;

    while (de <= nu) {
        de <<= 1;
        temp <<= 1;
    }

    //printf("%d %d\n",de,temp,nu);
    while (temp > 1) {
        de >>= 1;
        temp >>= 1;

        if (nu >= de) {
            nu -= de;
            //printf("%d %d\n",quotient,temp);
            quotient += temp;
        }
    }

    return quotient;
}

// returns the raw value from the sensor.
static short mg_raw(uint8_t hi, uint8_t lo) {
    uint16_t res = (uint16_t) hi;
    res = res << 8;
    res |= (uint16_t) lo;
    return res;
}
// returns milligauss, integer
static int mg_scaled(int v, int mg_scale) {
    assert(mg_scale <= 16);
    if (v >= 0) {
        int de = divide(0x4000 * 2, mg_scale);
        return divide(v * 1000, de);
    } else {
        v *= -1;
        int de = divide(0x4000 * 2, mg_scale);
        return -divide(v * 1000, de);
    }

}

static void test_mg(int expected, uint8_t h, uint8_t l, unsigned g) {
    int s_i = mg_scaled(mg_raw(h,l),g);
    printk("expect = %d, got %d\n", expected, s_i);
    assert(s_i == expected);
}

static imu_xyz_t xyz_mk(int x, int y, int z) {
    return (imu_xyz_t){.x = x, .y = y, .z = z};
}

// takes in raw data and scales it.
imu_xyz_t accel_scale(accel_t *h, imu_xyz_t xyz) {
    int g = h->g;
    int x = mg_scaled(xyz.x, h->g);
    int y = mg_scaled(xyz.y, h->g);
    int z = mg_scaled(xyz.z, h->g);
    return xyz_mk(x,y,z);
}

/* Read STATUS_REG, XLDA bit determines if data ready, pg 8, 23 */
int accel_has_data(accel_t *h) {
    uint8_t status = imu_rd (h->addr, STATUS_REG);
    return status & XLDA_BITMASK;
}

// block until there is data and then return it (raw)
//
// p26 interprets the data.
// if high bit is set, then accel is negative.
//
// read them all at once for consistent
// readings using autoincrement.
// these are blocking.  perhaps make non-block?
// returns raw, unscaled values.
imu_xyz_t accel_rd(accel_t *h) {
    // not sure if we have to drain the queue if there are more readings?

    // unsigned mg_scale = h->g;
    // uint8_t addr = h->addr;

    while (!accel_has_data (h))
        ;

    uint8_t buf[READS_PER_POLL];
    imu_rd_n (h->addr, OUTX_L_XL, buf, READS_PER_POLL);
    return (imu_xyz_t) {
        .x = mg_raw (buf[1], buf[0]),
        .y = mg_raw (buf[3], buf[2]),
        .z = mg_raw (buf[5], buf[4])
    };

}

// first do the cookbook from the data sheet.
// make sure:
//  - you use BDU and auto-increment.
//  - you get reasonable results!
//
// note: the initial readings are garbage!  skip these (see the data sheet)
accel_t accel_init(uint8_t addr, lsm6ds33_g_t g, lsm6ds33_hz_t hz) {
    dev_barrier();

    // some sanity checking
    switch(hz) {
    // these only work for "low-power" i think (p10)
    case lsm6ds33_1660hz:
    case lsm6ds33_3330hz:
    case lsm6ds33_6660hz:
        panic("accel: hz setting of %x does not appear to work\n", hz);
    default:
        break;
    }
    if(!legal_G(g))
        panic("invalid G value: %x\n", g);
    if(hz > lsm6ds33_6660hz)
        panic("invalid hz: %x\n", hz);

    // see header: pull out the scale and the bit pattern.
    unsigned g_scale = g >> 16;
    unsigned g_bits = g&0xff;
    assert(legal_g_bits(g_bits));

    accel_t h = (accel_t) {.addr = addr, .hz = hz, .g = g_scale };

    /* Startup sequence accelerator pg. 23 */
    imu_wr (h.addr, CTRL9_XL, ENABLE_ALL_AXES);
    imu_wr (h.addr, CTRL1_XL, h.hz << 4 | g_bits << 2);
    imu_wr (h.addr, CTRL3_C, ENABLE_BDU); // BDU pg 24
    imu_wr (h.addr, CTRL4_C, DRDY_MASK);  // Status register low until garbage values done pg. 19
    delay_ms(20);


    return h;
}


void do_accel_test(void) {
    // initialize accel.
    accel_t h = accel_init(lsm6ds33_default_addr, lsm6ds33_2g, lsm6ds33_416hz);

    int x,y,z;

    // p 26 of application note.
    test_mg(0, 0x00, 0x00, 2);
    test_mg(350, 0x16, 0x69, 2);
    test_mg(1000, 0x40, 0x09, 2);
    test_mg(-350, 0xe9, 0x97, 2);
    test_mg(-1000, 0xbf, 0xf7, 2);

    for(int i = 0; i < 100; i++) {
        imu_xyz_t v = accel_rd(&h);
        printk("accel raw values: x=%d,y=%d,z=%d\n", v.x,v.y,v.z);

        v = accel_scale(&h, v);
        printk("accel scaled values in mg: x=%d,y=%d,z=%d\n", v.x,v.y,v.z);

        delay_ms(500);
    }
}

/**********************************************************************
 * trivial driver.
 */
void notmain(void) {
    uart_init();

    delay_ms(100);   // allow time for device to boot up.
    i2c_init();
    delay_ms(100);   // allow time to settle after init.


    uint8_t dev_addr = lsm6ds33_default_addr;
    uint8_t v = imu_rd(dev_addr, WHO_AM_I);
    if(v != VAL_WHO_AM_I)
        panic("Initial probe failed: expected %x, got %x\n", VAL_WHO_AM_I, v);
    else
        printk("SUCCESS: lsm acknowledged our ping!!\n");

    do_accel_test();
    clean_reboot();
}
