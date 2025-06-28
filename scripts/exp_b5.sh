#!/bin/bash
# usage: Exp#B5: Impact of packet size

set -e

source "./load_eval_settings.sh"

echo "Exp#B5: Impact of packet size"

# install dependencies
echo $user_passwd | sudo -S apt-get install python3-pip
pip3 install pathlib numpy scipy

# extract num_runs from settings.ini
numRuns=$(grep "^num_runs = " $INI_FILE | cut -d' ' -f3)

codeList=$(cat <<EOF
RSCONV 14 10 1
Clay 14 10 256
LESS 14 10 2
LESS 14 10 3
LESS 14 10 4
EOF
)

packetSizeByteList=(131072 262144 524288 1048576)

# prepare code list
echo "${codeList[@]}" > $exp_script_dir/code_test_list.txt

# default bandwidth
bandwidth_kbps=1048576
# update in settings.ini
sed -i "s/^bandwidth_kbps = .*/bandwidth_kbps = ${bandwidth_kbps}/" $INI_FILE

# default block size
block_size_byte=67108864
# update in settings.ini
sed -i "s/^block_size_byte = .*/block_size_byte = ${block_size_byte}/" $INI_FILE

# vary packet size
for packet_size_byte in "${packetSizeByteList[@]}"; do
    # update in settings.ini
    sed -i "s/^packet_size_byte = .*/packet_size_byte = ${packet_size_byte}/" $INI_FILE

    # run single-block repair experiment
    echo "Running with packet size: ${packet_size_byte} bytes"
    python3 run_testbed_single_block.py

    echo "Finished with packet size: ${packet_size_byte} bytes"
done

# Extract the logs
echo "Extracting logs for single-block repair experiment"
for packet_size_byte in "${packetSizeByteList[@]}"; do
    echo "Extracting logs for packet size: ${packet_size_byte} bytes"
    
    while IFS= read -r line || [[ -n "$line" ]]; do
        # Skip empty lines and comments
        [[ -z "$line" || "$line" =~ ^[[:space:]]*# ]] && continue
        
        # Read all fields at once using read -a
        read -r -a fields <<< "$line"
        
        code_name="${fields[0]}"
        ecn="${fields[1]}"
        eck="${fields[2]}"
        ecw="${fields[3]}"
        
        code_id="${code_name}_${ecn}_${eck}_${ecw}"
        bandwidth_Gbps=$((bandwidth_kbps / 1024 / 1024))
        block_size_MiB=$((block_size_byte / 1024 / 1024))
        packet_size_KiB=$((packet_size_byte / 1024))

        echo "Code: $code_name ($ecn, $eck, $ecw)"
        echo "Network bandwidth: ${bandwidth_Gbps}Gbps, block size: ${block_size_MiB}MiB, packet size: ${packet_size_KiB}KiB"
        echo ""

        # Run the log extraction script
        python3 extract_testbed_logs_single_block.py \
            -ecn "$ecn" \
            -r "$numRuns" \
            -d "$proj_dir/eval_results/single/${code_id}/bw${bandwidth_Gbps}Gbps_blk${block_size_MiB}MiB_pkt${packet_size_KiB}KiB"

        echo ""
        echo ""
    done < "$exp_script_dir/code_test_list.txt"
done
