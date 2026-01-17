#!/bin/bash

set -e

HUGE_MOUNT=/mnt/huge
USER_ID=$(id -u)
GROUP_ID=$(id -g)

echo "[I] Setting up hugetlbfs at $HUGE_MOUNT"

# Unmount if already mounted
if mountpoint -q "$HUGE_MOUNT"; then
    echo "[I] Unmounting existing $HUGE_MOUNT"
    sudo umount "$HUGE_MOUNT"
fi

# Make sure directory exists
sudo mkdir -p "$HUGE_MOUNT"

# Mount hugetlbfs with user ownership
sudo mount -t hugetlbfs -o uid=$USER_ID,gid=$GROUP_ID nodev "$HUGE_MOUNT"

echo "[I] Mounted hugetlbfs with uid=$USER_ID, gid=$GROUP_ID"

# Remove old files
for f in hft_upstream hft_downstream hft_telemetry; do
    FILE="$HUGE_MOUNT/$f"
    if [ -e "$FILE" ]; then
        echo "[I] Removing old huge page file $FILE"
        rm -f "$FILE"
    fi
done

echo "[I] Huge pages setup complete"
