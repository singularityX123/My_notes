#!/bin/bash
# 重新打包 rootfs 并启动 QEMU (-M virt)
# 用法: 编译好 .ko 放 /srv/nfs/rootfs/ 下，跑这个脚本

QEMU_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOTFS_SRC="/srv/nfs/rootfs"

echo "==> 重新打包 rootfs..."
cd "$ROOTFS_SRC" && find . -print0 | cpio --null -ov --format=newc 2>/dev/null | gzip -9 > "$QEMU_DIR/boot/rootfs.cpio.gz"
echo "==> 打包完成 ($(du -h "$QEMU_DIR/boot/rootfs.cpio.gz" | cut -f1))"

echo "==> 启动 QEMU (-M virt)..."
qemu-system-arm \
    -M virt \
    -m 512M \
    -kernel "$QEMU_DIR/boot/zImage" \
    -nographic \
    -append "console=ttyAMA0,115200 root=/dev/ram rdinit=/sbin/init" \
    -initrd "$QEMU_DIR/boot/rootfs.cpio.gz"
