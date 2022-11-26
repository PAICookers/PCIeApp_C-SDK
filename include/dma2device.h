#ifndef __DMA_TO_DEVICE_H__
#define __DMA_TO_DEVICE_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int configframes_to_device(char *devname, char *user_reg, char *irq_ch1, char *infname, uint64_t offset);

#ifdef __cplusplus
}
#endif

#endif /* __DMA_TO_DEVICE_H__ */