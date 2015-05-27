make clean && make all
rm filesys.dsk && pintos-mkdisk filesys.dsk --filesys-size=2 && pintos -f -q

pintos -p ./build/tests/userprog/close-normal -a exit -- -q
pintos -q run 'exit'

