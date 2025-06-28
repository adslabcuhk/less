#!/bin/bash
# Usage: calculate the full node recovery time

source "./load_eval_settings.sh"

LOG_FILE=${proj_dir}/coor_output
# START_PATTERN="StripeStore::scanning repair queue"
START_PATTERN="fullNodeRecovery"
END_PATTERN="Coordinator::repair for "
full_node_num_stripes=$(grep '^full_node_num_stripes = ' "$INI_FILE" | cut -d '=' -f2)
full_node_num_stripes=$(echo "$full_node_num_stripes" | xargs)
TARGET_COUNT=$((full_node_num_stripes))
COUNT=0
START_TIME=""
END_TIME=""

tail -Fn0 "$LOG_FILE" | \
while read line; do
    if [[ -z "$START_TIME" && "$line" == *"$START_PATTERN"* ]]; then
        START_TIME=$(date +%s.%N)
        echo "[$(date)] Start time recorded: $START_TIME"
    elif [[ -n "$START_TIME" && "$line" == "$END_PATTERN"*finishes ]]; then
        COUNT=$((COUNT + 1))
        echo "[$(date)] Match $COUNT/$TARGET_COUNT"
        END_TIME=$(date +%s.%N)
        DURATION=$(echo "scale=6; $END_TIME - $START_TIME" | bc -l)
        echo "[$(date)] Recovery time for $COUNT OEC objects: $DURATION seconds"
        if [[ "$COUNT" -eq "$TARGET_COUNT" ]]; then
            END_TIME=$(date +%s.%N)
            DURATION=$(echo "scale=6; $END_TIME - $START_TIME" | bc -l)
            echo "Full-node recovery time: $DURATION seconds"
            break
        fi
    fi
done
