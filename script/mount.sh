#!/bin/sh

sudo modprobe virtiofs
mkdir -p work
sudo mount -t 9p -o trans=virtio,version=9p2000.L hostshare ./work
sudo chown wanghan:wanghan work
