#ifndef __NRF_DEFAULT_VALUES_H__
#define __NRF_DEFAULT_VALUES_H__

// seperated out so you can mess with to control values.
// if you are getting conflicts, you have two options:
//   1. change the client/server addresses.   easiest!
//   2. change the frequency.   this seems very sensitive.

// default client and server.
// addresses can increase error.  these have worked:
//  0xe5e5e5,
//  0xe3e3e3,
//  0xe7e7e7,
//  0xe1e1e1,
//  0xd3d3d3,

#define RF_DR_HI(x) ((x) << 3)
#define RF_DR_LO(x) ((x) << 5)
// clear both bits.
#define RF_DR_CLR(x) ((x) & ~(RF_DR_HI(1) | RF_DR_LO(1)))
typedef enum {
    nrf_1Mbps   = 0,            // both RF_DR_HI=0 and RF_DR_LO=0.
    nrf_2Mbps   = RF_DR_HI(1),  // RF_DR_LO=0, RF_DR_HI=1.
    nrf_250kbps = RF_DR_LO(1),  // RF_DR_LO=1, RF_DR_HI=0.
} nrf_datarate_t;


typedef enum {
    dBm_minus18 = 0b00 << 1, // 7mA
    dBm_minus12 = 0b01 << 1, // 7.5mA
    dBm_minus6  = 0b10 << 1, // 9mA
    dBm_0       = 0b11 << 1, // 11mA
} nrf_db_t;

enum {
    server_addr = 0xd5d5d5,
    client_addr = 0xe5e5e5,
};



enum {
    nrf_default_nbytes              = 4,            // 4 byte packets.

    // allegedly semi-safe from interference
    // RF is really sensitive.   maybe worth writing the code to find.
    // bounce around, send/recv and check.
    nrf_default_channel             = 113,          

    // lower data rate ==> longer distance.
    nrf_default_data_rate           = nrf_2Mbps,    

    // this is full power.
    nrf_default_db                  = dBm_0,

    // 6 retran attempts
    nrf_default_retran_attempts     = 6,            

    // 2000 usec retran delay
    nrf_default_retran_delay        = 2000,         

    // if we increase?  doesn't seem to matter; 4,5 also legal.
    nrf_default_addr_nbytes         = 3,
};

#endif
