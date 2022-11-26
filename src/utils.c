#include "utils.h"
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>

#include <sys/stat.h>

static int timespec_check(struct timespec *t);
static void timespec_sub(struct timespec *t1, struct timespec *t2);

#ifdef IN_DEV
int verbose = 1;
#else
int verbose = 0;
#endif

uint64_t getopt_integer(char *optarg) {
	int rc;
	uint64_t value;

	rc = sscanf(optarg, "0x%lx", &value);
	if (rc <= 0)
		rc = sscanf(optarg, "%lu", &value);

	return value;
}

void writeUser(void *baseAddr, off_t offset, uint32_t val) {
	*((uint32_t*)(baseAddr + offset)) = val;
}

uint32_t readUser(void *baseAddr, off_t offset) {
	return *((uint32_t *)(baseAddr + offset));
}

/*
    @brief
        Check the intrrupt whether is triggered within timeout.
    @param fs: file description of irq event
    @param irq: interrupt name
    @param timeout: return failed if timeout, unit: second
*/
int eventTriggered(int fd, irq_e irq, long timeout) {
    struct timespec ts_start, ts_cur;
    long total_time = 0;
    ssize_t rc;
    uint32_t irq_value;

#ifdef __USE_ISOC11
# ifndef __USE_TIME_BITS64
        rc = timespec_get(&ts_start, TIME_UTC);
        rc = timespec_get(&ts_cur, TIME_UTC);
# endif
#elif defined __USE_POSIX199309
# ifndef __USE_TIME_BITS64
    rc = clock_gettime(CLOCK_MONOTONIC, &ts_start);
    rc = clock_gettime(CLOCK_MONOTONIC, &ts_cur);
# endif
#endif

    while(total_time < timeout) {
        irq_value = readEvent(fd);
        
        if (irq_value & (uint32_t)(1 << irq)) {
            if (verbose) {
                fprintf(stdout, "Interrupt %d triggered successful.\n", irq);
            }
            break;
        }

        timespec_sub(&ts_cur, &ts_start);
        total_time += ts_cur.tv_sec;
    }

    if (total_time < timeout) {
        return 0;
    }

    return -1;
}

#ifdef IN_PROD
void clearIRQ(void *baseAddr, int irq) {
    uint32_t irq_value = readUser(baseAddr, IRQ_REG_OFFSET);
    
    switch (irq)
    {
        case IRQ_TX1:{
            writeUser(baseAddr, IRQ_REG_OFFSET, irq_value & 0xfffffffd);
            break;
        }
        case IRQ_TX2:{
            writeUser(baseAddr, IRQ_REG_OFFSET, irq_value & 0xfffffffb);
            break;
        }
        case IRQ_TICK:{
            writeUser(baseAddr, IRQ_REG_OFFSET, irq_value & 0xfffffff7);
            break;
        }
        case IRQ_RX1:{
            writeUser(baseAddr, IRQ_REG_OFFSET, irq_value & 0xffffffef);
            break;
        }
        case IRQ_RX2:{
            writeUser(baseAddr, IRQ_REG_OFFSET, irq_value & 0xffffffdf);
            break;
        }
        default:
            break;
    }
}
#else
/*
	@brief
        Clear the triggered interrupt, irq
*/
void clearIRQ(void *baseAddr, irq_e irq) {
    uint32_t irq_value = readUser(baseAddr, IRQ_CONTROL_REG_OFFSET);

    writeUser(baseAddr, IRQ_CONTROL_REG_OFFSET, irq_value & ~(1 << irq));
}
#endif

void reset_xdma(void *userAddr) {
    writeUser(userAddr, FPGA_MODE_REG_OFFSET, FPGA_MODE_RESET);
}

///////////////////////////////////////////////// xdma0_h2c

int openH2C(char *devName) {
    int fd;
    fd = open(devName, O_RDWR | O_NONBLOCK);
    
    if(fd < 0)
    {
        fprintf(stderr, "unable to open H2C %s, %d.\n", devName, fd);
		perror("open H2C");
		return -EINVAL;
    }
    return fd;
}

void writeH2C(int fd, uint64_t baseAddr, void *frameBufferPtr, size_t size) {
    ssize_t rc;
    off_t offset = baseAddr;
  
    rc = lseek(fd, offset, SEEK_SET);
    if (rc != offset) {
        fprintf(stderr, "%s, seek off 0x%lx != 0x%lx.\n", fd, rc, offset);
        perror("seek file");
        return;
    }
    
    rc = write(fd, frameBufferPtr, size);
    
    if (rc < 0) {
        fprintf(stderr, "%s, write 0x%lx @ 0x%lx failed %ld.\n", fd, size, offset, rc);
		perror("write file");
    }
    else {
        printf("writing %d bytes into a %d bytes buffer\n", size, rc);
    }
}

uint32_t readEvent(int fd)
{
	uint32_t val;
    ssize_t rc;

	rc = read(fd, (void*)&val, 4);
    if (rc != 4) {
        fprintf(stderr, "%s, read 0x04 @ 0 failed %ld.\n", fd, rc);
        perror("read file");
        return -EIO;
    }

	return val;
}

///////////////////////////////////////////////// xdma0_c2h

int openC2H(char *devName) {
    int fd;
    fd = open(devName, O_RDWR | O_NONBLOCK);
    
    if(fd < 0)
    {
        fprintf(stderr, "unable to open C2H %s, %d.\n", devName, fd);
		perror("open C2H");
		return -EINVAL;
    }
    return fd;
}

void readC2H(int fd, uint64_t baseAddr, void *frameBufferPtr, size_t size) {
    ssize_t rc;
    off_t offset = baseAddr;

    rc = lseek(fd, offset, SEEK_SET);
    if (rc != offset) {
        fprintf(stderr, "%s, seek off 0x%lx != 0x%lx.\n", fd, rc, offset);
        perror("seek file");
        return;
    }

    rc = read(fd, frameBufferPtr, size);

    if (rc < 0) {
        fprintf(stderr, "%s, read 0x%lx @ 0x%lx failed %ld.\n", fd, size, offset, rc);
		perror("read file");
    }
    else {
        printf("reading %d bytes of a %d bytes buffer\n", size, rc);
    }
}

frame char2frame(char *frame){
    return strtoul(frame, NULL, 2);
}

/*
    Return size of ALL file in bytes
*/
off_t getFileSize(const char *filename) {
    struct stat statbuff;
    
    if(stat(filename, &statbuff) < 0) {  
        return 0;
    }
    else {
        return statbuff.st_size;
    }
}

int long2bin(const frame *dec, char *bin){
    if(bin == NULL)
        return -1;
    
    char *start = bin;
    frame dec_tmp = *dec;
    int i = 64;
    while(i > 0){
        if(dec_tmp & 0x1)
            *bin++ = 0x31;
        else
            *bin++ = 0x30;
 
        dec_tmp >>= 1;
        i--;
    }
 
    *bin = 0;
    //reverse the order
    char *low, *high, temp;
    low = start, high = bin - 1;
    
    while(low < high){
        temp = *low;
        *low = *high;
        *high = temp;
 
        ++low; 
        --high;
    }
    return 0;
}

///////////////////////////////////////////////// configFrame

int read_txt_to_frame(int fd, configFrames *frameBufferPtr, size_t frameNumber) {
    char lineBuffer[65];
    int i;
    ssize_t rc;

    for(i = 0; i < frameNumber; i++){
        rc = read(fd, lineBuffer, 65);
        
        if (rc < 0 || rc < 65) {
            goto out;
        }

        lineBuffer[64] = '\0';
        frameBufferPtr->frames[i] = char2frame(lineBuffer);
    }

    frameBufferPtr->size = frameNumber*8;// 1 frame = 8 buffersize
    printf("reading config frame, frameNumber: %d, bufferSize: %d\n", frameNumber, frameNumber*8);

out:
    close(fd);
    if (rc < 0)
        return rc;
    
    return 1;
}

///////////////////////////////////////////////// workFrame

int openFrame(char *filePath) {
    int fd = open(filePath, O_RDONLY);
    if(fd == -1)
    {
        printf("open txt %s error\n", filePath);
        return -1;
    }
    return fd;
}

static int timespec_check(struct timespec *t)
{
	if ((t->tv_nsec < 0) || (t->tv_nsec >= 1000000000))
		return -1;
	return 0;
}

static void timespec_sub(struct timespec *t1, struct timespec *t2)
{
	if (timespec_check(t1) < 0) {
		fprintf(stderr, "invalid time #1: %lld.%.9ld.\n",
			(long long)t1->tv_sec, t1->tv_nsec);
		return;
	}
	if (timespec_check(t2) < 0) {
		fprintf(stderr, "invalid time #2: %lld.%.9ld.\n",
			(long long)t2->tv_sec, t2->tv_nsec);
		return;
	}
	t1->tv_sec -= t2->tv_sec;
	t1->tv_nsec -= t2->tv_nsec;
	if (t1->tv_nsec >= 1000000000) {
		t1->tv_sec++;
		t1->tv_nsec -= 1000000000;
	} else if (t1->tv_nsec < 0) {
		t1->tv_sec--;
		t1->tv_nsec += 1000000000;
	}
}