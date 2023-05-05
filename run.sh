# !/bin/bash
device=${1:-"/dev/xdma0_h2c_0"}
user_reg=${2:-"/dev/xdma0_user"}
irq_name=${3:-"/dev/xdma0_events_0"}
config=${4:-"./test/config.txt"}
work=${5:-"./test/input.txt"}
output=${6:-"./test/output.txt"}

# Send config frames
./build/bin/PCIeApp -d ${device} -u ${user_reg} -m 1 -i ${irq_name} -c ${config} -w ${work} -o ${output}

if [ $? -ne 0 ]; then
    echo "Error: $?"
    exit 1
fi
echo "Send configuration frames file OK"

# Send work frames
# ./build/pcie_app -d ${device} -u ${user_reg} -m 2 -i ${irq_name} -c ${config} -w ${work} -o ${output}

# if [ $? -ne 0 ]; then
#     echo "Error: $?"
#     exit 1
# fi
# echo "Send work frames file OK"