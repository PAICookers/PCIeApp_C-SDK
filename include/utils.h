#ifndef __UTILS_H__
#define __UTILS_H__

#include <stdint.h>
#include "config.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef uint64_t frame;

typedef struct frameBuffer_struct {
    frame* frames;      // Base address of frames data
    uint64_t size;      // size in bytes
} frameBuffer;

typedef frameBuffer dataFrames;
typedef frameBuffer configFrames;

#ifdef IN_DEV
typedef enum irq_name {
    IRQ_TX_CH1_DONE,
    IRQ_RX_CH1_DONE,
    IRQ_TX_CH2_DONE,
    IRQ_RX_CH2_DONE,
} irq_e;

typedef enum irq_status {
    IRQ_RESET,
    IRQ_TIGGERED
} irq_status;
#endif

uint64_t getopt_integer(char *optarg);

void writeUser(void *baseAddr, off_t offset, uint32_t val);
uint32_t readUser(void *baseAddr, off_t offset);
int checkTXCompleted(void *baseAddr, long timeout);
int checkRXCompleted(void *baseAddr, long timeout);
int eventTriggered(int fd, irq_e irq);

#ifdef IN_PROD
void clearIRQ(void *baseAddr, int irq);
#else
void clearIRQ(void *baseAddr, irq_e irq);
#endif

void reset_xdma(void *userAddr);

int openEvent(char *devName);
uint32_t readEvent(int fd);

int openC2H(char *devName);
void readC2H(int fd, uint64_t baseAddr, void *frameBufferPtr, size_t size);

int long2bin(const frame *dec, char *bin);
int int2bin(const int* dec, char *bin);

#ifdef __cplusplus
}
#endif

#endif /* __UTILS_H__ */