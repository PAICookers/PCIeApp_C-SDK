#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <byteswap.h>

typedef uint64_t frame;
typedef struct FrameBuffer_TypeDef
{
	frame *frames; // Base address of frames data
	ssize_t size;  // size in bytes
} FrameBuffer;

ssize_t read_bin_to_buffer(char *fname, int fd, FrameBuffer *buffer, ssize_t size, uint64_t base)
{
	ssize_t rc;
	uint64_t count = 0; // size in bytes
	frame *buf = buffer->frames;
	off_t offset = base;
	int loop = 0;

	while (count < size)
	{
		rc = lseek(fd, offset, SEEK_SET);
		if (rc != offset)
		{
			fprintf(stderr, "%s, seek offset 0x%lx != 0x%lx.\n", fname, rc, offset);
			perror("seek file");
			return -1;
		}

		/* read data from file into bin buffer */
		rc = read(fd, (void *)buf, 8);
		*buf = bswap_64(*buf);

		if (rc < 0)
		{
			fprintf(stderr, "%s, read 8 @ 0x%lx failed %ld.\n", fname, offset, rc);
			perror("read file");
			return -1;
		}

		if (rc != 8)
		{
			fprintf(stderr, "%s, read underflow 65/0x%lx @ 0x%lx.\n", fname, rc, offset);
			break;
		}

		buf++;
		offset += 8;
		count++;
		loop++;
	}

	buffer->size = size;

	if (count != size && loop)
		fprintf(stderr, "%s, read underflow 0x%lx/0x%lx.\n", fname, count, size);

	// Check the data in cache
	printf("count = %ld, loop = %d\n", count, loop);

	for (int i = 0; i < size; i++)
	{
		printf("#%d: 0x%lx\n", i, *(buffer->frames + i));
	}

	return count;
}

int main(int argc, char **argv)
{
	char *infname = "./test/config.bin";
	// char *txtname = "./test/config.txt";
	int rc;

	int infile_fd = open(infname, O_RDONLY);
	if (infile_fd < 0)
	{
		fprintf(stderr, "unable to open input file %s, %d.\n", infname, infile_fd);
		perror("open input file");
		rc = -1;
		goto out;
	}

	int inf_size = lseek(infile_fd, 0, SEEK_END);
	if (inf_size < 0)
	{
		fprintf(stderr, "unable to get frames file size: %d.\n", inf_size);
		perror("get file size");
		rc = -1;
		goto out;
	}

	/* Move the cursor to the head */
	// rc = lseek(infile_fd, 0, SEEK_SET);
	// if (rc != 0) {
	// 	fprintf(stderr, "unable to move the cursor to the head.\n");
	// 	perror("move the cursor");
	// 	rc = -1;
	// 	goto out;
	// }

	printf("file_size = %d\n", inf_size);

	FrameBuffer *FramesBuffer = (FrameBuffer *)malloc(sizeof(FrameBuffer));
	if (!FramesBuffer)
	{
		fprintf(stderr, "unable to malloc\n");
		perror("malloc frames buffer");
		rc = -1;
		goto out;
	}

	frame *allocated = NULL;

	posix_memalign((void **)&allocated, 4096 /* alignment */, inf_size / 8 + 4096);
	if (!allocated)
	{
		fprintf(stderr, "OOM %u.\n", (inf_size/8 + 4096));
		rc = -1;
		goto out;
	}

	FramesBuffer->frames = allocated;

	rc = read_bin_to_buffer(infname, infile_fd, FramesBuffer, inf_size / 8, 0);

	printf("rc = %d\n", rc);

out:
	close(infile_fd);

	return 0;
}