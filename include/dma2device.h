#ifndef __DMA_TO_DEVICE_H__
#define __DMA_TO_DEVICE_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int framesFileToDevice(char *devname, char *user_reg, char *irq_ch1, char *infname, int work_mode);
int deviceToFramesFile(char *devname, char *user_reg, char *irq_ch1, char *ofname);

#ifdef __cplusplus
}
#endif

#endif /* __DMA_TO_DEVICE_H__ */