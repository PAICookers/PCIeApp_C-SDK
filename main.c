#include "utils.h"
#include "config.h"
#include "dma2device.h"
#include <unistd.h>
#include <getopt.h>

static struct option const long_opts[] = {
    {"device", required_argument, NULL, 'd'},
    {"user_registers", required_argument, NULL, 'u'},
    {"mode", required_argument, NULL, 'm'},
    {"irq_name", required_argument, NULL, 'i'},
    {"configframe_path", required_argument, NULL, 'c'},
    {"workframe_path", required_argument, NULL, 'w'},
    {"outputframe_path", required_argument, NULL, 'o'},
    {"help", no_argument, NULL, 'h'},
    {"verbose", no_argument, NULL, 'v'},
    {0, 0, 0, 0},
};

extern int verbose;

static void usage(const char *name)
{
    int i = 0;

    fprintf(stdout, "%s\n\n", name);
    fprintf(stdout, "usage: %s [OPTIONS]\n\n", name);

    fprintf(stdout, "  -%c (--%s) device (defaults to %s)\n",
            long_opts[i].val, long_opts[i].name, H2C_DEVICE_NAME_DEFAULT);
    i++;
    fprintf(stdout, "  -%c (--%s) user registers name (defaults to %s)\n",
            long_opts[i].val, long_opts[i].name, USER_REG_NAME_DEFAULT);
    i++;
    fprintf(stdout, "  -%c (--%s) indicates the mode of software\n",
            long_opts[i].val, long_opts[i].name);
    i++;
    fprintf(stdout, "  -%c (--%s) IRQ name\n",
            long_opts[i].val, long_opts[i].name);
    i++;
    fprintf(stdout, "  -%c (--%s) path of config frames\n",
            long_opts[i].val, long_opts[i].name);
    i++;
    fprintf(stdout, "  -%c (--%s) path of work frames\n",
            long_opts[i].val, long_opts[i].name);
    i++;
    fprintf(stdout, "  -%c (--%s) path of output frames to be saved\n",
            long_opts[i].val, long_opts[i].name);
    i++;
    fprintf(stdout, "  -%c (--%s) print usage help and exit\n",
            long_opts[i].val, long_opts[i].name);
    i++;
    fprintf(stdout, "  -%c (--%s) verbose output\n",
            long_opts[i].val, long_opts[i].name);
    i++;
}

int main(int argc, char *argv[])
{
    int cmd_opt;
    char *h2c_dev_name = H2C_DEVICE_NAME_DEFAULT;
    char *c2h_dev_name = C2H_DEVICE_NAME_DEFAULT;
    char *user_reg = USER_REG_NAME_DEFAULT;
    char *irq_ch1_name = IRQ_CH1_NAME_DEFAULT;

    int mode = FPGA_MODE_CONFIG;
    char *configFramePath = CONFIG_FRAMES_PATH_DEFAULT;
    char *workFramePath = WORK_FRAMES_PATH_DEFAULT;
    char *outputFramePath = OUTPUT_FRAMES_PATH_DEFAULT;

    ssize_t rc;

    while ((cmd_opt = getopt_long(argc, argv, "vhd:u:m:i:c:w:o:", long_opts, NULL)) != -1)
    {
        switch (cmd_opt)
        {
        case 0:
            /* long option */
            break;
        case 'd':
            /* device name */
            h2c_dev_name = strdup(optarg);
            break;
        case 'u':
            /* user registers name */
            user_reg = strdup(optarg);
            break;
        case 'm':
            /* work mode */
            mode = getopt_integer(optarg);
            break;
        case 'i':
            /* irq name */
            irq_ch1_name = strdup(optarg);
            break;
        case 'c':
            /* path of config frames */
            configFramePath = strdup(optarg);
            break;
        case 'w':
            /* path of work frames */
            workFramePath = strdup(optarg);
            break;
        case 'o':
            /* path of output frames */
            outputFramePath = strdup(optarg);
            break;

            /* print usage help and exit */
        case 'v':
            verbose = 1;
            break;
        case 'h':
        default:
            usage(argv[0]);
            exit(0);
            break;
        }
    }

    if (verbose)
    {
        fprintf(stdout, "device: %s,\nuser registers: %s,\nmode: %d,\nirq_ch1_name: %s,\nconfigFramePath: %s,\nworkFramePath: %s,\noutputFramePath: %s\n\n",
                h2c_dev_name, user_reg, mode, irq_ch1_name, configFramePath, workFramePath, outputFramePath);
    }

    /*
        Currently, a transaction use an individual program
    */
    if (mode == FPGA_MODE_CONFIG)
    {
        return FramesFile2Device(h2c_dev_name, user_reg, irq_ch1_name, configFramePath, mode);
    }
    else if (mode == FPGA_MODE_WORK)
    {
        rc = FramesFile2Device(h2c_dev_name, user_reg, irq_ch1_name, workFramePath, mode);
        rc = deviceToFramesFile(c2h_dev_name, user_reg, irq_ch1_name, outputFramePath);
    }

    return -1;
}