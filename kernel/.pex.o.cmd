savedcmd_/mnt/c/Users/Deepthi/Desktop/Softee_cursor/kernel/pex.o := ld -m elf_x86_64 -z noexecstack --no-warn-rwx-segments   -r -o /mnt/c/Users/Deepthi/Desktop/Softee_cursor/kernel/pex.o @/mnt/c/Users/Deepthi/Desktop/Softee_cursor/kernel/pex.mod  ; ./tools/objtool/objtool --hacks=jump_label --hacks=noinstr --hacks=skylake --ibt --retpoline --rethunk --stackval --static-call --uaccess --prefix=16  --link  --module /mnt/c/Users/Deepthi/Desktop/Softee_cursor/kernel/pex.o

/mnt/c/Users/Deepthi/Desktop/Softee_cursor/kernel/pex.o: $(wildcard ./tools/objtool/objtool)
