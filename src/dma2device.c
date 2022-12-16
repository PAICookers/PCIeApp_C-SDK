#include "utils.h"
#include "dma_utils.h"
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include <sys/mman.h>
#include <sys/types.h>

extern int verbose;

/*
	@brief
		Read frames file into buffer then send to device
	
	@param devname: Device name of XDMA host2card channel
	@param user_reg: Name of user registers: /dev/xdma0_user
	@param irq_ch1: IRQ name of channel 1
	@param infname: Name of frames file to be read
	@param work_mode: work in which mode
*/
int frames2device(char *devname, char *user_reg, char *irq_ch1, char *infname, int work_mode) {
	ssize_t rc;
	int mode = FPGA_MODE_UNKNOWN;
	size_t bytes_done = 0;
    frameBuffer* FramesBuffer = NULL;
    frame* allocated = NULL;
	
	void* user_addr = NULL;	/* Base address of user registers */
	int user_reg_fd = -1;
	int irq_ch1_fd = -1;
	int infile_fd = -1;
	off_t inf_size = -1;
	int h2c_fd = open(devname, O_RDWR);

	/* 1. Check devices, files, memory mapping */
	if (h2c_fd < 0) {
		fprintf(stderr, "unable to open device %s, %d.\n", devname, h2c_fd);
		perror("open device");
		return -ENXIO;
	}

	user_reg_fd = open(user_reg, O_RDWR | O_SYNC);
	if (user_reg_fd < 0) {
        fprintf(stderr, "unable to open user registers %s, %d.\n", user_reg, user_reg_fd);
		perror("open device");
		rc = -ENXIO;
		goto out;
    }

	user_addr = mmap(NULL, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, user_reg_fd, 0);
	if (user_addr == (void*)-1) {
		fprintf(stderr, "Memory mapped failed.\n");
		perror("mmap error\n");
		rc = -ENOMEM;
		goto out;
	}

	/* Reset device and set mode MODE_CONFIG */
    reset_xdma(user_addr);
    writeUser(user_addr, FPGA_MODE_RO_ADDR, work_mode);

	sleep(1);
    
	mode = readUser(user_addr, FPGA_MODE_RO_ADDR);
	if (mode != work_mode) {
		fprintf(stderr, "mode error, %d.\n", mode);
        perror("mode error\n");
        rc = -EINVAL;
		goto out;
	}
    
    if (verbose) {
		fprintf(stdout, "Hardware now is in mode CONFIG\n");
    }

	irq_ch1_fd = open(irq_ch1, O_RDWR);
	if (irq_ch1_fd < 0) {
		fprintf(stderr, "unable to open event %s, %d.\n", irq_ch1, irq_ch1_fd);
		perror("open event");
		rc = -ENXIO;
		goto out;
	}

	if (infname) {
		infile_fd = open(infname, O_RDONLY);
		if (infile_fd < 0) {
			fprintf(stderr, "unable to open input file %s, %d.\n", infname, infile_fd);
			perror("open input file");
			rc = -ENOENT;
			goto out;
		}
	}

	/* 2. Allocate for frames buffer */
	FramesBuffer = (frameBuffer*)malloc(sizeof(configFrames));
	if (!FramesBuffer) {
		fprintf(stderr, "unable to malloc\n");
		perror("malloc config buffer");
		rc = -EINVAL;
		goto out;
	}

	inf_size = lseek(infile_fd, 0, SEEK_END);
	if (inf_size < 0) {
		fprintf(stderr, "unable to get frames file size: %ld.\n", inf_size);
		perror("get file size");
		rc = -EINVAL;
		goto out;
	}

	/* Move the cursor to the head */
	rc = lseek(infile_fd, 0, SEEK_SET);
	if (rc != 0) {
		fprintf(stderr, "unable to move the cursor to the head.\n");
		perror("move the cursor");
		rc = -EINVAL;
		goto out;
	}

	/*
		For example, a file:
			Line1: 0000000000000000000001000000000000000000000000000000000000000000\n
			Line2: 0000000000000000000001010000000000000000000000000000000000000000\n
		
		- inf_size = 65*2 bytes
		- FramesBuffer->size = 2 frames(64-bits) = 16 bytes
		- size = frame_num * 8
	*/
	FramesBuffer->size = (uint64_t)(inf_size / 65 * 8);
	FramesBuffer->offset = 0;

	posix_memalign((void **)&allocated, 4096 /* alignment */ , FramesBuffer->size + 4096);
	if (!allocated) {
		fprintf(stderr, "OOM %lu.\n", FramesBuffer->size + 4096);
		rc = -ENOMEM;
		goto out;
	}

	FramesBuffer->frames = allocated;

	if (verbose)
		fprintf(stdout, "reading frames into buffer. Size in bytes: %ld\n", FramesBuffer->size);

	/* 3. Read configuration frames from file to buffer */
	if (infile_fd >= 0) {
		rc = read_txt_to_buffer(infname, infile_fd, FramesBuffer->frames, FramesBuffer->size, 0);
		if (rc < 0 || rc < FramesBuffer->size)
			goto out;
	}

	// for (int i = 0; i < FramesBuffer->size / 8; i++) {
	// 	printf("%d: 0x%lx\n", i, FramesBuffer->frames[i]);
	// }

	/* 4. Send to BRAM via single channel */
	rc = single_channel_send(infname, h2c_fd, user_addr, irq_ch1_fd, DOWNSTREAM_BRAM_CH1_ADDR, FramesBuffer);
	if (rc < 0 || rc != FramesBuffer->size) {
		fprintf(stderr, "Sending %s to device %d, address 0x%x via channel %d failed, rc=%ld\n", 
			infname, h2c_fd, DOWNSTREAM_BRAM_CH1_ADDR, irq_ch1_fd, rc);
		perror("send data");
		rc =  -EINVAL;
		goto out;
	}

	if (verbose) {
		fprintf(stdout, "Sending frames OK, total bytes: %ld\n", FramesBuffer->size);
	}

	/* Last, if failed or finished, close and free */
out:
	close(h2c_fd);
	
	if (user_reg_fd >= 0) {
		close(user_reg_fd);
	}

	if (irq_ch1_fd >= 0) {
		close(irq_ch1_fd);
	}
		
	if (infile_fd >= 0) {
		close(infile_fd);
	}
	
	free(FramesBuffer);
	free(allocated);

	if (rc < 0)
		return rc;
	
	return 0;
}