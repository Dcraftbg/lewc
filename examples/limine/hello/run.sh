#!/bin/bash

set -xe

qemu-system-x86_64 -serial stdio -smp 2 -m 128 -cdrom ./bin/OS.iso
