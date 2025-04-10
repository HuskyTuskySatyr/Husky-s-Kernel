qemu-system-x86_64 -cdrom build/huskyos.iso -vga cirrus -drive id=disk,file=qemu/sata.img,format=raw,if=none -device ahci,id=ahci -device ide-hd,drive=disk,bus=ahci.0
