#!/bin/bash
# QEMU ARM 虚拟机启动脚本 (-M virt)
# 用法: ./start_qemu.sh

QEMU_DIR="$(cd "$(dirname "$0")" && pwd)"

qemu-system-arm \
    -M virt \
    -m 512M \
    -kernel "$QEMU_DIR/boot/zImage" \
    -nographic \
    -append "console=ttyAMA0,115200 root=/dev/ram rdinit=/sbin/init" \
    -initrd "$QEMU_DIR/boot/rootfs.cpio.gz"
