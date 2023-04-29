#include "utils.h"
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>

#include <sys/stat.h>

#ifdef IN_DEV
int verbose = 1;
#else
int verbose = 0;
#endif

#if __BYTE_ORDER == __LITTLE_ENDIAN
#define ltohl(x) (x)
#define ltohs(x) (x)
#define htoll(x) (x)
#define htols(x) (x)
#elif __BYTE_ORDER == __BIG_ENDIAN
#define ltohl(x) __bswap_32(x)
#define ltohs(x) __bswap_16(x)
#define htoll(x) __bswap_32(x)
#define htols(x) __bswap_16(x)
#endif

static int timespec_check(struct timespec *t);
static void timespec_sub(struct timespec *t1, struct timespec *t2);

uint64_t getopt_integer(char *optarg)
{
    int rc;
    uint64_t value;

    rc = sscanf(optarg, "0x%lx", &value);
    if (rc <= 0)
        rc = sscanf(optarg, "%lu", &value);

    return value;
}

void writeUser(void *baseAddr, off_t offset, uint32_t val)
{
    *((uint32_t *)(baseAddr + offset)) = htoll(val);
}

uint32_t readUser(void *baseAddr, off_t offset)
{
    return ltohl(*((uint32_t *)(baseAddr + offset)));
}

int checkTXCompleted(void *baseAddr, long timeout)
{
    struct timespec ts_start, ts_cur;
    long total_time = 0;
    time_t cur_time;

#ifdef __USE_ISOC11
#ifndef __USE_TIME_BITS64
    timespec_get(&ts_start, TIME_UTC);
    timespec_get(&ts_cur, TIME_UTC);
#endif
#elif defined __USE_POSIX199309
#ifndef __USE_TIME_BITS64
    rc = clock_gettime(CLOCK_MONOTONIC, &ts_start);
    rc = clock_gettime(CLOCK_MONOTONIC, &ts_cur);
#endif
#endif

    while (total_time < timeout)
    {
        if (readUser(baseAddr, TX_DONE_RW_ADDR) == 0x00000001)
        {
            writeUser(baseAddr, TX_DONE_RW_ADDR, 0x00);
            return 0;
        }

        timespec_sub(&ts_cur, &ts_start);
        total_time = ts_cur.tv_sec;
        sleep(0.5);
    }

    return -1;
}

int checkRXCompleted(void *baseAddr, long timeout)
{
    struct timespec ts_start, ts_cur;
    long total_time = 0;
    time_t cur_time;

#ifdef __USE_ISOC11
#ifndef __USE_TIME_BITS64
    timespec_get(&ts_start, TIME_UTC);
    timespec_get(&ts_cur, TIME_UTC);
#endif
#elif defined __USE_POSIX199309
#ifndef __USE_TIME_BITS64
    rc = clock_gettime(CLOCK_MONOTONIC, &ts_start);
    rc = clock_gettime(CLOCK_MONOTONIC, &ts_cur);
#endif
#endif

    while (total_time < timeout)
    {
        if ((readUser(baseAddr, TX_DONE_RW_ADDR) & 0x00000002) == 0x00000002)
        {
            return 0;
        }

        timespec_get(&ts_cur, TIME_UTC);
        timespec_sub(&ts_cur, &ts_start);
        total_time = ts_cur.tv_sec;

        sleep(0.5);
    }

    return -1;
}

/*
    @brief
        Check the intrrupt whether is triggered.
    @param fs: file description of irq event
    @param irq: interrupt name
*/
int eventTriggered(int fd, irq_e irq)
{
    ssize_t rc;
    uint32_t irq_value;

    irq_value = readEvent(fd);

    if (irq_value & (uint32_t)(1 << irq))
    {
        if (verbose)
        {
            fprintf(stdout, "Interrupt %d triggered successful.\n", irq);
        }
        return 0;
    }

    return -1;
}

/*
    @brief
        Clear the triggered interrupt, irq
*/
void clearIRQ(void *baseAddr, irq_e irq)
{
    uint32_t irq_value = readUser(baseAddr, IRQ_CONTROL_RW_ADDR);

    writeUser(baseAddr, IRQ_CONTROL_RW_ADDR, irq_value & ~(1 << irq));
}

void reset_xdma(void *userAddr)
{
    writeUser(userAddr, FPGA_MODE_RO_ADDR, FPGA_MODE_RESET);
}

int openH2C(char *devName)
{
    int fd;
    fd = open(devName, O_RDWR | O_NONBLOCK);

    if (fd < 0)
    {
        fprintf(stderr, "unable to open H2C %s, %d.\n", devName, fd);
        perror("open H2C");
        return -EINVAL;
    }
    return fd;
}

void writeH2C(int fd, uint64_t baseAddr, void *frameBufferPtr, size_t size)
{
    ssize_t rc;
    off_t offset = baseAddr;

    rc = lseek(fd, offset, SEEK_SET);
    if (rc != offset)
    {
        fprintf(stderr, "%d, seek off 0x%lx != 0x%lx.\n", fd, rc, offset);
        perror("seek file");
        return;
    }

    rc = write(fd, frameBufferPtr, size);

    if (rc < 0)
    {
        fprintf(stderr, "%d, write 0x%lx @ 0x%lx failed %ld.\n", fd, size, offset, rc);
        perror("write file");
    }
    else
    {
        printf("writing %ld bytes into a %ld bytes buffer\n", size, rc);
    }
}

uint32_t readEvent(int fd)
{
    uint32_t val;
    ssize_t rc;

    rc = read(fd, &val, sizeof(val));
    if (rc != 4)
    {
        fprintf(stderr, "%d, read 0x04 @ 0 failed %ld.\n", fd, rc);
        perror("read file");
        return -EIO;
    }

    return val;
}

int openC2H(char *devName)
{
    int fd;
    fd = open(devName, O_RDWR | O_NONBLOCK);

    if (fd < 0)
    {
        fprintf(stderr, "unable to open C2H %s, %d.\n", devName, fd);
        perror("open C2H");
        return -EINVAL;
    }
    return fd;
}

void readC2H(int fd, uint64_t baseAddr, void *frameBufferPtr, size_t size)
{
    ssize_t rc;
    off_t offset = baseAddr;

    rc = lseek(fd, offset, SEEK_SET);
    if (rc != offset)
    {
        fprintf(stderr, "%d, seek off 0x%lx != 0x%lx.\n", fd, rc, offset);
        perror("seek file");
        return;
    }

    rc = read(fd, frameBufferPtr, size);

    if (rc < 0)
    {
        fprintf(stderr, "%d, read 0x%lx @ 0x%lx failed %ld.\n", fd, size, offset, rc);
        perror("read file");
    }
    else
    {
        printf("reading %ld bytes of a %ld bytes buffer\n", size, rc);
    }
}

int long2bin(const frame *dec, char *bin)
{
    if (bin == NULL)
        return -1;

    char *start = bin;
    frame dec_tmp = *dec;
    int i = 64;

    while (i > 0)
    {
        if (dec_tmp & 0x1)
            *bin++ = 0x31;
        else
            *bin++ = 0x30;

        dec_tmp >>= 1;
        i--;
    }

    *bin = 0;
    // reverse the order
    char *low, *high, temp;
    low = start;
    high = bin - 1;

    while (low < high)
    {
        temp = *low;
        *low = *high;
        *high = temp;

        ++low;
        --high;
    }

    return 0;
}

int int2bin(const int *dec, char *bin)
{
    if (bin == NULL)
        return -1;

    char *start = bin;
    frame dec_tmp = *dec;
    int i = 32;

    while (i > 0)
    {
        if (dec_tmp & 0x1)
            *bin++ = 0x31;
        else
            *bin++ = 0x30;

        dec_tmp >>= 1;
        i--;
    }

    *bin = 0;
    // reverse the order
    char *low, *high, temp;
    low = start;
    high = bin - 1;

    while (low < high)
    {
        temp = *low;
        *low = *high;
        *high = temp;

        ++low;
        --high;
    }
    return 0;
}

static int timespec_check(struct timespec *t)
{
    if ((t->tv_nsec < 0) || (t->tv_nsec >= 1000000000))
        return -1;

    return 0;
}

static void timespec_sub(struct timespec *t1, struct timespec *t2)
{
    if (timespec_check(t1) < 0)
    {
        fprintf(stderr, "invalid time #1: %lld.%.9ld.\n",
                (long long)t1->tv_sec, t1->tv_nsec);
        return;
    }
    if (timespec_check(t2) < 0)
    {
        fprintf(stderr, "invalid time #2: %lld.%.9ld.\n",
                (long long)t2->tv_sec, t2->tv_nsec);
        return;
    }

    t1->tv_sec -= t2->tv_sec;
    t1->tv_nsec -= t2->tv_nsec;

    if (t1->tv_nsec >= 1000000000)
    {
        t1->tv_sec++;
        t1->tv_nsec -= 1000000000;
    }
    else if (t1->tv_nsec < 0)
    {
        t1->tv_sec--;
        t1->tv_nsec += 1000000000;
    }
}