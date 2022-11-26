#include "dma_utils.h"
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>

#define RW_MAX_SIZE			(0x7ffff000)
#define TXT_MODE

#ifdef TXT_MODE
# define READ_ONE_LINE 		65
# define BYTES_IN_ONE_LINE	8
#endif

ssize_t write_from_buffer(char *fname, int fd, frame *buffer, uint64_t size, uint64_t base);
ssize_t write_h2c_with_limit(char *fname, int fd, void *user_addr, int irq_fd, frameBuffer *buffer,	\
	uint64_t base, uint64_t max_limit);

extern int verbose;

/*
	@brief 
		frames file(.txt) to frames(uint64_t)

	@param fname: Input filename
	@param fd: File description of input file
	@param buffer: Frame buffer
	@param size: The size of what to read in bytes
	@param base: Usually is 0
*/
ssize_t read_to_buffer(char *fname, int fd, frame *buffer, uint64_t size, uint64_t base)
{
	ssize_t rc;
	uint64_t count = 0;
	frame* buf = buffer;
	off_t offset = base;
	int loop = 0;
#ifdef TXT_MODE
	char txt_buf[65];
#endif

	while (count < size) {
		uint64_t bytes = size - count;

#ifdef TXT_MODE
		if (bytes > READ_ONE_LINE)
			bytes = READ_ONE_LINE;
#else
		if (bytes > RW_MAX_SIZE)
			bytes = RW_MAX_SIZE;
#endif

		if (offset) {
			rc = lseek(fd, offset, SEEK_SET);
			if (rc != offset) {
				fprintf(stderr, "%s, seek off 0x%lx != 0x%lx.\n",
					fname, rc, offset);
				perror("seek file");
				return -EIO;
			}
		}

		/* read data from file into memory buffer */
#ifdef TXT_MODE
		rc = read(fd, (void*)txt_buf, bytes);
#else
		rc = read(fd, (void*)buf, bytes);
#endif
		if (rc < 0) {
			fprintf(stderr, "%s, read 0x%lx @ 0x%lx failed %ld.\n",
				fname, bytes, offset, rc);
			perror("read file");
			return -EIO;
		}

		if (rc != bytes) {
			fprintf(stderr, "%s, read underflow 0x%lx/0x%lx @ 0x%lx.\n",
				fname, rc, bytes, offset);
			break;
		}

#ifdef TXT_MODE
		txt_buf[READ_ONE_LINE-1] = '\0';
		buf = (frame*)strtoul(txt_buf, NULL, 2);
		count += 8;
		buf += 8;
#else
		count += rc;
		buf += bytes;
#endif
		
		offset += bytes;
		loop++;
	}

	if (count != size && loop)
		fprintf(stderr, "%s, read underflow 0x%lx/0x%lx.\n", fname, count, size);
	
	return count;
}

/*
	@brief 
		Write data into fd with buffer and size
	@param fname: Input filename
	@param fd: File description of input file
	@param buffer: Frame buffer
	@param size: The size of what to write in bytes
	@param base: Usually is 0
*/
ssize_t write_from_buffer(char *fname, int fd, frame *buffer, uint64_t size, uint64_t base)
{
	ssize_t rc;
	uint64_t count = 0;
	frame *buf = buffer;
	off_t offset = base;
	int loop = 0;

	while (count < size) {
		uint64_t bytes = size - count;

		if (bytes > RW_MAX_SIZE)
			bytes = RW_MAX_SIZE;

		if (offset) {
			rc = lseek(fd, offset, SEEK_SET);
			if (rc != offset) {
				fprintf(stderr, "%s, seek off 0x%lx != 0x%lx.\n",
					fname, rc, offset);
				perror("seek file");
				return -EIO;
			}
		}

		/* write data to file from memory buffer */
		rc = write(fd, buf, bytes);

		if (rc < 0) {
			fprintf(stderr, "%s, write 0x%lx @ 0x%lx failed %ld.\n",
				fname, bytes, offset, rc);
			perror("write file");
			return -EIO;
		}

		count += rc;
		if (rc != bytes) {
			fprintf(stderr, "%s, write underflow 0x%lx/0x%lx @ 0x%lx.\n",
				fname, rc, bytes, offset);
			break;
		}

		buf += bytes;
		
		offset += bytes;
		loop++;
	}	

	if (count != size && loop)
		fprintf(stderr, "%s, write underflow 0x%lx/0x%lx.\n",
			fname, count, size);

	return count;
}

/*
	@brief 
		Write data into fd with buffer and size
	@param fname: Input filename
	@param fd: File description of input file
	@param user_addr: Address of user register to control IRQ
	@param irq_fd: File description of interrupt event
	@param buffer: Frame buffer
	@param base: Base offset of H2C device, DOWNSTREAM_BRAM_CH1_ADDR
	@param size: The size of what to write in bytes
*/
ssize_t write_h2c_with_limit(char *fname, int fd, void *user_addr, int irq_fd, frameBuffer *buffer,	\
	uint64_t base, uint64_t max_limit)
{
	ssize_t rc;
	uint64_t size = buffer->size;
	uint64_t count = 0;
	frame *buf = buffer->frames;
	uint64_t stop_frame = STOP_FRAME;
	off_t offset = base;
	int loop = 0;

	while (count < size) {
		uint64_t bytes = size - count;

		if (bytes > max_limit)
			bytes = max_limit;

		if (offset) {
			rc = lseek(fd, offset, SEEK_SET);
			if (rc != offset) {
				fprintf(stderr, "%s, seek off 0x%lx != 0x%lx.\n",
					fname, rc, offset);
				perror("seek file");
				return -EIO;
			}
		}

		/* write data to file from memory buffer */
		rc = write(fd, buf, bytes);
		if (rc < 0) {
			fprintf(stderr, "%s, write 0x%lx @ 0x%lx failed %ld.\n",
				fname, bytes, offset, rc);
			perror("write file");
			return -EIO;
		}

		count += rc;
		if (rc != bytes) {
			fprintf(stderr, "%s, write underflow 0x%lx/0x%lx @ 0x%lx.\n",
				fname, rc, bytes, offset);
			break;
		}

		buf += bytes;
		offset += bytes;
		loop++;

		/* Send stop frame when count == size */
		if (count == size) {
			rc = write_from_buffer(fname, fd, (frame*)&stop_frame, 8, 0);
			if (rc < 0) {
				fprintf(stderr, "Sending stop frame failed.\n");
				return -EIO;
			}
		}

		/* 1. When sending max_limit, tell FPGA to steart sending */
		writeUser(user_addr, TX_REG_OFFSET, TX_STATUS_SENDING);

		/* 2. Read the interrupt and do service */
		rc = eventTriggered(irq_fd, IRQ_TX_CH1_DONE, IRQ_TIGGERED_TIMEOUT);
		if (rc < 0) {
			fprintf(stderr, "Interrupt %d triggered failed.\n", IRQ_TX_CH1_DONE);
			return -EIO;
		}

		/* 3. Clear the interrupt */
		clearIRQ(user_addr, IRQ_TX_CH1_DONE);

		if (verbose) {
			fprintf(stdout, "Send %d bytes successful.\n", count);
		}
	}

	if (count != size && loop)
		fprintf(stderr, "%s, write underflow 0x%lx/0x%lx.\n", fname, count, size);

	return count;
}

/*
	@brief
		Send data in frame buffer via single channel
	
	@param fname: Input file name
	@param fpga_fd: File description of XDMA0_H2C channel
	@param user_addr: Address of user registers
	@param irq_fd: File description of IRQ channel 1
	@param addr: Address of where to write, H2C device
	@param buffer: Frames buffer
	@param size: Size of writing data, no more than DOWNSTREAM_BRAM_SIZE
*/
ssize_t single_channel_send(char *fname, int fpga_fd, void *user_addr, int irq_fd, uint64_t addr, frameBuffer *buffer) {
	ssize_t rc;

	rc = write_h2c_with_limit(fname, fpga_fd, user_addr, irq_fd, buffer, addr, DOWNSTREAM_BRAM_SIZE);
	return rc; // Just return rc.
}

/*
	@brief
		Send data in frame buffer via TWO channels
	
	@param fname: Input file name
	@param fpga_fd: File description of XDMA0_H2C channel
	@param user_addr: Address of user registers
	@param irq_fd: File description of IRQ channel 1
	@param addr: Address of where to write
	@param buffer: Frames buffer
	@param size: Size of writing data
*/
ssize_t double_channel_send(char* fname, int fpga_fd, void *user_addr, int irq_fd1, int irq_fd2,	\
	uint64_t addr1, uint64_t addr2, frameBuffer* buffer) {

}