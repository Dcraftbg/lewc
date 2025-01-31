#!/bin/bash

set -xe

mkdir -p int
mkdir -p bin
mkdir -p bin/iso
../../../bin/lewc entry.lew -o int/entry.s
as int/entry.s -o int/entry.o
nasm -f elf64 io.nasm -o int/io.o
ld -g -T ./linker/link.ld int/entry.o int/io.o -o bin/kernel
cp -v \
    ./bin/kernel \
    ./limine/limine-bios.sys \
    ./limine/limine-bios-cd.bin \
    ./limine/limine-uefi-cd.bin \
    ./limine.cfg \
    bin/iso
xorriso -as mkisofs -b limine-bios-cd.bin -no-emul-boot -boot-load-size 4 -boot-info-table --efi-boot limine-uefi-cd.bin -efi-boot-part --efi-boot-image ./bin/iso -o ./bin/OS.iso
