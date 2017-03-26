#!/bin/sh

sudo apt install linux-tools-generic linux-cloud-tools-generic linux-tools-4.4.0-21-generic linux-cloud-tools-4.4.0-21-generic
sudo sh -c 'echo 0 > /proc/sys/kernel/kptr_restrict'
