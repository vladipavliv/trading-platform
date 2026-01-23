#!/bin/bash

set -e

HUGE_PAGES=4096
HUGE_MOUNT=/mnt/huge
USER_NAME=$(whoami)

echo "[*] Creating mount point $HUGE_MOUNT if missing..."
sudo mkdir -p $HUGE_MOUNT

echo "[*] Mounting hugetlbfs at $HUGE_MOUNT..."
if ! mount | grep -q "$HUGE_MOUNT"; then
    sudo mount -t hugetlbfs none $HUGE_MOUNT
fi

echo "[*] Setting huge page count..."

echo $HUGE_PAGES | sudo tee /proc/sys/vm/nr_hugepages

echo "[*] Fixing ownership and permissions..."
sudo chown $USER_NAME:$USER_NAME $HUGE_MOUNT
sudo chmod 700 $HUGE_MOUNT

echo "[*] Huge page FS ready at $HUGE_MOUNT"
