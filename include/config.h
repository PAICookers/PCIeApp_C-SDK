#ifndef __CONFIG_H__
#define __CONFIG_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Debug Switch */
#define IN_DEV

#ifdef IN_DEV
# include "dbg.h"
#endif

#ifdef IN_DEV
# undef IN_PROD
#else
# define IN_PROD
#endif

#define SINGLE_CHANNEL

/* Names of devices, files path */
#define H2C_DEVICE_NAME_DEFAULT     "/dev/xdma0_h2c_0"
#define C2H_DEVICE_NAME_DEFAULT     "/dev/xdma0_c2h_0"
#define USER_REG_NAME_DEFAULT       "/dev/xdma0_user"
#define IRQ_CH1_NAME_DEFAULT        "/dev/xdma0_events_0"
#define CONFIG_FRAMES_PATH_DEFAULT  "./test/config.txt"
#define WORK_FRAMES_PATH_DEFAULT    "./test/input.txt"
#define OUTPUT_FRAMES_PATH_DEFAULT  "./test/output.txt"

/* Memory Mapping Size */ 
#define MAP_SIZE (size_t)(4*1024)   /* 4K bytes */

/* Offset of registers */
#ifdef IN_DEV
/* In user controller register */
# define IRQ_STATUS_RO_ADDR     (0x00)  /* Interrupt status register */
# define IRQ_CONTROL_RW_ADDR    (0x04)  /* Interrupt control register */
# define FPGA_MODE_RO_ADDR      (0x08)  /* Mode status register */
# define TX_STATUS_RW_ADDR      (0x0C)  /* TX status register */
# define RX_STATUS_RW_ADDR      (0x10)  /* RX status register */
# define TRANS_INFO_RW_ADDR     (0x14)  /* Transaction information register */
# define TX_DONE_RW_ADDR        (0x18)  /* TODO Polling. */

/* In H2C/C2H channel status register */
# define CHANNEL_DEBUG_OFFSET   (0x40) /* Address of H2C/C2H channel status register. See P132 */

#else
# define IRQ_REG_OFFSET  0   /* Not used */
# define TX_STATUS_RW_ADDR   4   /* TX status register */ 
# define TICK_OFFSET     8   /* Not used */
# define RX_STATUS_RW_ADDR   12  /* RX status register */ 
# define MODE_REG_OFFSET 16  /* Mode status register */
# define UP_ADDR_OFFSET  20  /* Not used */
# define UP_SIZE_OFFSET  24  /* Not used */
#endif

/* Status of registers */
#ifdef TX_STATUS_RW_ADDR
# define TX_STATUS_READY        0   /* TX ready */
# define TX_STATUS_SENDING      1   /* TX sending */
# define TX_STATUS_DONE         2   /* TX done */
#endif

#ifdef RX_STATUS_RW_ADDR
# define RX_STATUS_READY        0   /* RX ready */
# define RX_STATUS_RECEIVING    1   /* RX receiving */
# define RX_STATUS_DONE         2   /* RX done */
#endif

#ifdef FPGA_MODE_RO_ADDR
# ifdef IN_DEV
#  define FPGA_MODE_RESET      0   /* Reset mode */
#  define FPGA_MODE_CONFIG     1   /* Configuration mode */
#  define FPGA_MODE_WORK       2   /* Work mode */
#  define FPGA_MODE_UNKNOWN    3   /* Reserved */
# else
#  define MODE_CONFIG     0   /* Configuration mode */
#  define MODE_WORK       1   /* Work mode */
#  define MODE_UNKNOWN    2   /* Unknown mode */
# endif
#endif

/* 
    Request or acknowledges events 
    Rrequest TX transfering event or acknowledge RX receiving event.
*/
#ifdef TX_STATUS_RW_ADDR
# define REQ_TX_SENDING  TX_STATUS_SENDING
#endif

/* BRAM Parameters */
#define DOWNSTREAM_BRAM_SIZE        (0x40000)               /* 256K bytes for downstream BRAM */ 
#define UPSTREAM_BRAM_SIZE          (0x40000)               /* 256K bytes for upstream BRAM */ 
#define DOWNSTREAM_BRAM_CH1_ADDR    (0xC0000000)            /* Address of downstream channel 1 BRAM */
#define UPSTREAM_BRAM_CH1_ADDR      (0xC2000000)            /* Address of upstream channel 1 BRAM */
#define DOWNSTREAM_BRAM_CH2_ADDR    (0xC4000000)            /* Address of downstream channel 2 BRAM */
#define UPSTREAM_BRAM_CH2_ADDR      (0xC6000000)            /* Address of upstream channel 2 BRAM */
#define STOP_FRAME                  (0xFFFFFFFFFFFFFFFF)    /* Stop frame for BRAM */

/* Blocking IRQ Definitions */
#ifdef IN_DEV
# ifdef IRQ_CONTROL_RW_ADDR
#  define IRQ_RESET_MASK          ((uint32_t)0)
#  define IRQ_TX_CH1_DONE_MASK    (uint32_t)(1 << 0)  /* Interrupt of end of sending via channel 1 */
#  define IRQ_RX_CH1_DONE_MASK    (uint32_t)(1 << 1)  /* Interrupt of end of receiving via channel 1 */
#  define IRQ_TX_CH2_DONE_MASK    (uint32_t)(1 << 2)  /* Interrupt of end of sending via channel 2 */
#  define IRQ_RX_CH2_DONE_MASK    (uint32_t)(1 << 3)  /* Interrupt of end of receiving via channel 2 */
#  define IRQ_TIGGERED_TIMEOUT    3
# endif
#else
# define IRQ_TX1          1  /* Interrupt of end of sending via channel 0 */
# define IRQ_TX2          2  /* Interrupt of end of sending via channel 1 */
# define IRQ_TICK         3  /* Not used */
# define IRQ_RX1          4  /* Interrupt of end of receiving via channel 0 */
# define IRQ_RX2          5  /* Interrupt of end of receiving via channel 1 */
#endif

#ifdef __cplusplus
}
#endif

#endif /* __CONFIG_H__ */