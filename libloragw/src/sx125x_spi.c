/*
 / _____)             _              | |
( (____  _____ ____ _| |_ _____  ____| |__
 \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 _____) ) ____| | | || |_| ____( (___| | | |
(______/|_____)_|_|_| \__)_____)\____)_| |_|
  (C)2019 Semtech

Description:
    Functions used to handle LoRa concentrator SX1255/SX1257 radios.

License: Revised BSD License, see LICENSE.TXT file include in the project
*/

/* -------------------------------------------------------------------------- */
/* --- DEPENDANCIES --------------------------------------------------------- */

#include <stdint.h>     /* C99 types */
#include <stdbool.h>    /* bool type */
#include <stdio.h>      /* printf fprintf */
#include <string.h>     /* memset */

#include <sys/ioctl.h>
#include <linux/spi/spidev.h>

#include "sx125x_spi.h"
#include "loragw_spi.h"

/* -------------------------------------------------------------------------- */
/* --- PRIVATE MACROS ------------------------------------------------------- */

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#if DEBUG_RAD == 1
    #define DEBUG_MSG(str)              fprintf(stdout, str)
    #define DEBUG_PRINTF(fmt, args...)  fprintf(stdout,"%s:%d: "fmt, __FUNCTION__, __LINE__, args)
    #define CHECK_NULL(a)               if(a==NULL){fprintf(stderr,"%s:%d: ERROR: NULL POINTER AS ARGUMENT\n", __FUNCTION__, __LINE__);return LGW_SPI_ERROR;}
#else
    #define DEBUG_MSG(str)
    #define DEBUG_PRINTF(fmt, args...)
    #define CHECK_NULL(a)               if(a==NULL){return LGW_SPI_ERROR;}
#endif

/* -------------------------------------------------------------------------- */
/* --- PRIVATE TYPES -------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/* --- PRIVATE CONSTANTS ---------------------------------------------------- */

#define READ_ACCESS     0x00
#define WRITE_ACCESS    0x80

/* -------------------------------------------------------------------------- */
/* --- PRIVATE VARIABLES ---------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/* --- PRIVATE FUNCTIONS ---------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/* --- PUBLIC FUNCTIONS DEFINITION ------------------------------------------ */

/* Simple read */
int sx125x_spi_r(void *com_target, uint8_t spi_mux_target, uint8_t address, uint8_t *data) {
    int com_device;
    uint8_t out_buf[3];
    uint8_t in_buf[1];
    struct spi_ioc_transfer k[2];
    int a;

    /* check input variables */
    CHECK_NULL(com_target);
    CHECK_NULL(data);

    com_device = *(int *)com_target;

    /* prepare frame to be sent */
    out_buf[0] = spi_mux_target;
    out_buf[1] = READ_ACCESS | (address & 0x7F);
    out_buf[2] = 0x00;

    /* I/O transaction: split into a tx-only command phase and a rx-only data
     * phase instead of a single full-duplex transfer (tx_buf + rx_buf set on
     * the same spi_ioc_transfer). Some SPI masters (e.g. the MT7621/MT7628
     * controller) do not support combined tx+rx in one transfer and return
     * EINVAL. */
    memset(&k, 0, sizeof(k)); /* clear k */
    k[0].tx_buf = (unsigned long) out_buf;
    k[0].len = sizeof(out_buf);
    k[0].cs_change = 0;
    k[1].rx_buf = (unsigned long) in_buf;
    k[1].len = sizeof(in_buf);
    k[1].cs_change = 0;
    a = ioctl(com_device, SPI_IOC_MESSAGE(2), &k);

    /* determine return code */
    if (a != (int)(k[0].len + k[1].len)) {
        DEBUG_MSG("ERROR: SPI READ FAILURE\n");
        return LGW_SPI_ERROR;
    } else {
        //DEBUG_MSG("Note: SPI read success\n");
        *data = in_buf[0];
        return LGW_SPI_SUCCESS;
    }
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

int sx125x_spi_w(void *spi_target, uint8_t spi_mux_target, uint8_t address, uint8_t data) {
    int spi_device;
    uint8_t out_buf[3];
    uint8_t command_size;
    struct spi_ioc_transfer k;
    int a;

    /* check input variables */
    CHECK_NULL(spi_target);

    spi_device = *(int *)spi_target; /* must check that spi_target is not null beforehand */

    /* prepare frame to be sent */
    out_buf[0] = spi_mux_target;
    out_buf[1] = WRITE_ACCESS | (address & 0x7F);
    out_buf[2] = data;
    command_size = 3;

    /* I/O transaction */
    memset(&k, 0, sizeof(k)); /* clear k */
    k.tx_buf = (unsigned long) out_buf;
    k.len = command_size;
    k.speed_hz = SPI_SPEED;
    k.cs_change = 0;
    k.bits_per_word = 8;
    a = ioctl(spi_device, SPI_IOC_MESSAGE(1), &k);

    /* determine return code */
    if (a != (int)k.len) {
        DEBUG_MSG("ERROR: SPI WRITE FAILURE\n");
        return LGW_SPI_ERROR;
    } else {
        //DEBUG_MSG("Note: SPI write success\n");
        return LGW_SPI_SUCCESS;
    }
}

/* --- EOF ------------------------------------------------------------------ */
