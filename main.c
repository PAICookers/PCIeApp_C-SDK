#include "utils.h"
#include "config.h"
#include "dma2device.h"
#include <string.h>
#include <getopt.h>

static struct option const long_opts[] = {
	{"device", required_argument, NULL, 'd'},
    {"user_registers", required_argument, NULL, 'u'},
    {"mode", required_argument, NULL, 'm'},
    {"irq_name", required_argument, NULL, 'i'},
    {"initNum", required_argument, NULL, 'n'},
    {"configframe_path", required_argument, NULL, 'c'},
    {"workframe_path", required_argument, NULL, 'w'},
    {"outputframe_path", required_argument, NULL, 'o'},
    {"help", no_argument, NULL, 'h'},
    {"verbose", no_argument, NULL, 'v'},
    {0, 0, 0, 0},
};

extern int verbose;

static void usage(const char* name) {
    int i = 0;

	fprintf(stdout, "%s\n\n", name);
	fprintf(stdout, "usage: %s [OPTIONS]\n\n", name);

    fprintf(stdout, "  -%c (--%s) device (defaults to %s)\n",
		long_opts[i].val, long_opts[i].name, DEVICE_NAME_DEFAULT);
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
    fprintf(stdout, "  -%c (--%s) number of initial frames in work frames\n",
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

int main(int argc, char *argv[]) {
    int cmd_opt;
    char* dev_name = DEVICE_NAME_DEFAULT;
    char* user_reg = USER_REG_NAME_DEFAULT;
    char* irq_ch1_name = IRQ_CH1_NAME_DEFAULT;
    
    int mode = FPGA_MODE_CONFIG;
    uint32_t initFrmNum = 0;
    char* configFramePath = CONFIG_FRAMES_PATH_DEFAULT;
    char* workFramePath = WORK_FRAMES_PATH_DEFAULT;
    char* outputFramePath = OUTPUT_FRAMES_PATH_DEFAULT;

    while ((cmd_opt = getopt_long(argc, argv, "vhd:u:m:i:n:c:w:o:", long_opts, NULL)) != -1) {
		switch (cmd_opt) {
            case 0:
                /* long option */
                break;
            case 'd':
                /* device name */
                dev_name = strdup(optarg);
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
            case 'n':
                /* number of initial frames */
                initFrmNum = (uint32_t)getopt_integer(optarg);
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

    if (verbose) {
        fprintf(stdout, "device: %s,\nuser registers: %s,\nmode: %d,\nirq_ch1_name: %s,\ninitFrmNum: %d,\nconfigFramePath: %s,\nworkFramePath: %s,\noutputFramePath: %s\n\n", 
            dev_name, user_reg, mode, initFrmNum, irq_ch1_name, configFramePath, workFramePath, outputFramePath);
    }

    if (mode == FPGA_MODE_CONFIG) {
        return configframes_to_device(dev_name, user_reg, irq_ch1_name, configFramePath, 0);
    }
    else if (mode == FPGA_MODE_WORK){
        return 0;
    }
    
    return -1;
}