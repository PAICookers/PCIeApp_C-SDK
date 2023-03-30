#include "dma_utils.h"
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>

#define RW_MAX_SIZE (0x7ffff000)
// #define TXT_MODE			// Deprecated

#ifdef TXT_MODE
#define READ_ONE_LINE 65
#define BYTES_IN_ONE_LINE 8
#endif

ssize_t write_from_buffer(char *fname, int fd, frame *buffer, uint64_t size, uint64_t base);
ssize_t write_h2c_with_limit(char *fname, int fd, void *user_addr, int irq_fd, frameBuffer *buffer,
							 uint64_t base, uint64_t max_limit);

extern int verbose;

ssize_t receive_to_buffer(char *fname, int fd, frameBuffer *buffer, uint64_t base)
{
	ssize_t rc;
	uint64_t count = 0;
	frame *buf = buffer->frames;
	off_t offset = base;
	int loop = 0;

	while (count < UPSTREAM_BRAM_SIZE)
	{
		uint64_t bytes = UPSTREAM_BRAM_SIZE - count;

		if (bytes > UPSTREAM_BRAM_SIZE)
			bytes = UPSTREAM_BRAM_SIZE;

		if (offset)
		{
			rc = lseek(fd, offset, SEEK_SET);
			if (rc != offset)
			{
				fprintf(stderr, "%s, seek off 0x%lx != 0x%lx.\n",
						fname, rc, offset);
				perror("seek file");
				return -EIO;
			}
		}

		/* read data from file into memory buffer */
		rc = read(fd, buf, bytes);
		if (rc < 0)
		{
			fprintf(stderr, "%s, read 0x%lx @ 0x%lx failed %ld.\n",
					fname, bytes, offset, rc);
			perror("read file");
			return -EIO;
		}

		count += rc;
		if (rc != bytes)
		{
			fprintf(stderr, "%s, read underflow 0x%lx/0x%lx @ 0x%lx.\n",
					fname, rc, bytes, offset);
			break;
		}

		buf += bytes;
		offset += bytes;
		loop++;
	}

	if (count != UPSTREAM_BRAM_SIZE && loop)
		fprintf(stderr, "%s, read underflow 0x%lx/0x%x.\n", fname, count, UPSTREAM_BRAM_SIZE);

	return count;
}

/*
	@brief
		frames file(.txt) to frames(uint64_t)

	@param fname: Input filename
	@param fd: File description of input file
	@param buffer: Frame buffer
	@param size: The size of what to read in bytes
	@param base: Usually is 0
*/
#ifdef TXT_MODE
ssize_t read_txt_to_buffer(char *fname, int fd, frameBuffer *buffer, uint64_t base)
{
	ssize_t rc;
	uint64_t count = 0; // size in bytes
	frame *buf = buffer->frames;
	uint64_t size = buffer->size;
	off_t offset = base;
	int loop = 0;
	char txt_buf[READ_ONE_LINE];

	while (count < size)
	{
		if (offset)
		{
			rc = lseek(fd, offset, SEEK_SET);
			if (rc != offset)
			{
				fprintf(stderr, "%s, seek off 0x%lx != 0x%lx.\n",
						fname, rc, offset);
				perror("seek file");
				return -EIO;
			}
		}

		/* read data from file into txt buffer */
		rc = read(fd, (void *)txt_buf, READ_ONE_LINE);

		if (rc < 0)
		{
			fprintf(stderr, "%s, read 65 @ 0x%lx failed %ld.\n",
					fname, offset, rc);
			perror("read file");
			return -EIO;
		}

		if (rc != READ_ONE_LINE)
		{
			fprintf(stderr, "%s, read underflow 65/0x%lx @ 0x%lx.\n",
					fname, rc, offset);
			break;
		}

		txt_buf[READ_ONE_LINE - 1] = '\0';
		*buf = (frame)strtoul(txt_buf, NULL, 2);

		count += BYTES_IN_ONE_LINE;
		buf += 1;

		offset += READ_ONE_LINE;
		loop++;
	}

	if (count != size && loop)
		fprintf(stderr, "%s, read underflow 0x%lx/0x%lx.\n", fname, count, size);

	return count;
}
#endif

#ifdef BIN_MODE
/*
	@brief
		frames bin(.bin) to frames(uint64_t)

	@param fname: Input filename
	@param fd: File description of input file
	@param buffer: Frame buffer
	@param size: The size of what to read in bytes
	@param base: Usually is 0
*/
ssize_t read_bin_to_buffer(char *fname, int fd, frameBuffer *buffer, uint64_t base)
{
	ssize_t rc;
	uint64_t count = 0; // size in bytes
	frame *buf = buffer->frames;
	ssize_t size = buffer->size;
	off_t offset = base;
	int loop = 0;

	while (count < size)
	{
		if (offset)
		{
			rc = lseek(fd, offset, SEEK_SET);
			if (rc != offset)
			{
				fprintf(stderr, "%s, seek off 0x%lx != 0x%lx.\n", fname, rc, offset);
				perror("seek file");
				return -EIO;
			}
		}

		/* read data from file into bin buffer */
		rc = read(fd, (void *)buf, 8);

		if (rc < 0)
		{
			fprintf(stderr, "%s, read 8 @ 0x%lx failed %ld.\n", fname, offset, rc);
			perror("read file");
			return -EIO;
		}

		if (rc != 8)
		{
			fprintf(stderr, "%s, read underflow 65/0x%lx @ 0x%lx.\n", fname, rc, offset);
			break;
		}

		buf += 8;
		offset += 8;

		count += 8;
		loop++;
	}

	if (count != size && loop)
		fprintf(stderr, "%s, read underflow 0x%lx/0x%lx.\n", fname, count, size);

	return count;
}
#endif

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

	while (count < size)
	{
		uint64_t bytes = size - count;

		if (bytes > RW_MAX_SIZE)
			bytes = RW_MAX_SIZE;

		if (offset)
		{
			rc = lseek(fd, offset, SEEK_SET);
			if (rc != offset)
			{
				fprintf(stderr, "%s, seek off 0x%lx != 0x%lx.\n",
						fname, rc, offset);
				perror("seek file");
				return -EIO;
			}
		}

		/* write data to file from memory buffer */
		rc = write(fd, buf, bytes);

		if (rc < 0)
		{
			fprintf(stderr, "%s, write 0x%lx @ 0x%lx failed %ld.\n",
					fname, bytes, offset, rc);
			perror("write file");
			return -EIO;
		}

		count += rc;
		if (rc != bytes)
		{
			fprintf(stderr, "%s, write underflow 0x%lx/0x%lx @ 0x%lx.\n",
					fname, rc, bytes, offset);
			break;
		}

		buf += bytes;
		offset += bytes;
		loop++;
	}

	if (count != size && loop)
		fprintf(stderr, "%s, write underflow 0x%lx/0x%lx.\n", fname, count, size);

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
ssize_t write_h2c_with_limit(char *fname, int fd, void *user_addr, int irq_fd,
							 frameBuffer *buffer, uint64_t base, uint64_t max_limit)
{
	ssize_t rc;
	uint64_t size = buffer->size;
	uint64_t count = 0;
	frame *buf = buffer->frames;
	uint64_t stop_frame = STOP_FRAME;
	off_t offset = base;
	int loop = 0;

	while (count < size)
	{
		uint64_t bytes = size - count;

		if (bytes > max_limit)
			bytes = max_limit;

		if (offset)
		{
			rc = lseek(fd, offset, SEEK_SET);
			if (rc != offset)
			{
				fprintf(stderr, "%s, seek off 0x%lx != 0x%lx.\n",
						fname, rc, offset);
				perror("seek file");
				return -EIO;
			}
		}

		/* write data to h2c from memory buffer */
		rc = write(fd, buf, bytes);
		if (rc < 0)
		{
			fprintf(stderr, "%s, write 0x%lx @ 0x%lx failed %ld.\n",
					fname, bytes, offset, rc);
			perror("write file");
			return -EIO;
		}

		count += rc;
		if (rc != bytes)
		{
			fprintf(stderr, "%s, write underflow 0x%lx/0x%lx @ 0x%lx.\n",
					fname, rc, bytes, offset);
			break;
		}

		buf += bytes / BYTES_IN_ONE_LINE;
		offset += bytes;
		loop++;

		/* Send stop frame when ALL frames sending to card is completed. */
		if (count == size)
		{
			/* Set the cursor at the end */
			rc = lseek(fd, offset, SEEK_SET);
			if (rc != offset)
			{
				fprintf(stderr, "%s, seek off 0x%lx != 0x%lx.\n",
						fname, rc, offset);
				perror("seek file");
				return -EIO;
			}

			rc = write(fd, &stop_frame, 8);
			if (rc < 0)
			{
				fprintf(stderr, "Sending stop frame failed.\n");
				return -EIO;
			}

			if (verbose)
				fprintf(stdout, "Sending stop frame successful.\n");
		}

		/* 1. When sending max_limit, tell FPGA to steart sending */
		writeUser(user_addr, TX_STATUS_RW_ADDR, REQ_TX_SENDING);

		/* 2. Read the interrupt and do service */
		// fprintf(stdout, "Reading interrupt IRQ_TX_CH1_DONE.\n");
		// rc = eventTriggered(irq_fd, IRQ_TX_CH1_DONE);
		// if (rc < 0) {
		// 	fprintf(stderr, "Interrupt %d triggered failed.\n", IRQ_TX_CH1_DONE);
		// 	return -EIO;
		// }

		/* Poll check TX DONE */
		rc = checkTXCompleted(user_addr, IRQ_TIGGERED_TIMEOUT);
		if (rc < 0)
		{
			fprintf(stderr, "Got TX done failed.\n");
			return -EIO;
		}

		uint32_t tx_write_bytes = readUser(user_addr, TX_BYTES_NUM_ADDR);
		printf("Last TX wrote bytes: %d\n", tx_write_bytes & 0xFFFFFFFF);

		/* 3. Clear the interrupt */
		// clearIRQ(user_addr, IRQ_TX_CH1_DONE);

		if (verbose)
		{
			fprintf(stdout, "Loop #%d: Send %ld frames(%ld bytes) successful.\n", loop, count / BYTES_IN_ONE_LINE, count);
		}

		if (offset - base >= DOWNSTREAM_BRAM_SIZE)
		{
			offset = base;
		}
	}

	if (count != size && loop)
		fprintf(stderr, "%s, write underflow 0x%lx/0x%lx.\n", fname, count, size);
	else
		fprintf(stdout, "TX transaction completed!\n");

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
	@param buffer: Pointer of frames buffer
*/
ssize_t single_channel_send(char *fname, int fpga_fd, void *user_addr, int irq_fd, uint64_t addr, frameBuffer *buffer)
{
	ssize_t rc;
	uint32_t _read, tx_frames_num;

	/* ceil() */
	tx_frames_num = (buffer->size + DOWNSTREAM_BRAM_SIZE / 2) / DOWNSTREAM_BRAM_SIZE;

	if (verbose)
	{
		fprintf(stdout, "%d loop(s) will be sent.\n", tx_frames_num);
	}

	_read = readUser(user_addr, TRANS_INFO_RW_ADDR);

	/* Set the # of frames that will be sent. */
	writeUser(user_addr, TRANS_INFO_RW_ADDR, (_read & 0xFFFFFF00) | tx_frames_num);

	rc = write_h2c_with_limit(fname, fpga_fd, user_addr, irq_fd, buffer, addr, DOWNSTREAM_BRAM_SIZE);
	_read = readUser(user_addr, TRANS_INFO_RW_ADDR);

	/*
		A TX transaction fails when:
		1. # of sent frames is NOT correct.
		2. The rest loops is NOT 0.
	*/
	if ((rc != buffer->size) || (_read & 0x000000FF != 0))
	{
		fprintf(stderr, "write failed. Actual wrote: %ld.\nLoop(s) left: %d\n", rc, _read & 0x000000FF);
	}

	return rc;
}

/*
	@brief
		Send data in frame buffer via TWO channels

	@param fname: Input file name
	@param fpga_fd: File description of XDMA0_H2C channel
	@param user_addr: Address of user registers
	@param irq_fd: File description of IRQ channel 1
	@param addr: Address of where to write
	@param buffer: Pointer of frames buffer
*/
ssize_t double_channel_send(char *fname, int fpga_fd, void *user_addr, int irq_fd1, int irq_fd2,
							uint64_t addr1, uint64_t addr2, frameBuffer *buffer)
{
}

/*
	@brief
		Receive data into a file via single channel

	@param fname: output file name
	@param fpga_fd: File description of XDMA0_C2H channel
	@param user_addr: Address of user registers
	@param irq_fd: File description of IRQ channel 1
	@param addr: Address of where to write, C2H device, UPSTREAM_BRAM_ADDR
	@param buffer: Pointer of frames buffer
*/
ssize_t single_channel_receive(char *fname, int fpga_fd, void *user_addr, int irq_fd, uint64_t addr, frameBuffer *buffer)
{
	ssize_t rc;
	uint32_t _read;

	/* Poll check RX DONE */
	rc = checkRXCompleted(user_addr, IRQ_TIGGERED_TIMEOUT);
	if (rc < 0)
	{
		fprintf(stderr, "Got RX done failed.\n");
		return -EIO;
	}

	/* Read fpga_fd+addr to buffer via fpga_fd */
	rc = receive_to_buffer(fname, fpga_fd, buffer, addr);

	_read = readUser(user_addr, TX_DONE_RW_ADDR);
	writeUser(user_addr, TX_DONE_RW_ADDR, _read & 0xFFFFFFFD);

	if (rc != UPSTREAM_BRAM_SIZE)
	{
		fprintf(stderr, "read failed. Actual read: %ld.\n", rc);
	}

	return rc;
}