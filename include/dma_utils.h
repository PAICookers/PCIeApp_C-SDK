#ifndef __DMA_UTILS_H__
#define __DMA_UTILS_H__

#include "utils.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

ssize_t read_txt_to_buffer(char *fname, int fd, frameBuffer *buffer, uint64_t base);
ssize_t read_bin_to_buffer(char *fname, int fd, frameBuffer *buffer, ssize_t size, uint64_t base);
ssize_t single_channel_send(char *fname, int fpga_fd, void *user_addr, int irq_fd,  \
    uint64_t addr, frameBuffer *buffer);
ssize_t double_channel_send(char* fname, int fpga_fd, void *user_addr, int irq_fd1, int irq_fd2,    \
    uint64_t addr1, uint64_t addr2, frameBuffer* buffer);
ssize_t single_channel_receive(char *fname, int fpga_fd, void *user_addr, int irq_fd,   \
    uint64_t addr, frameBuffer *buffer);

#ifdef __cplusplus
}
#endif

#endif /* __DMA_UTILS_H__ */