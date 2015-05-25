make clean && make all
rm filesys.dsk && pintos-mkdisk filesys.dsk --filesys-size=2 && pintos -f -q
#pintos -p ./build/tests/userprog/args-many -a t1 -- -q
#pintos -q run 't1 1 2 3 4 5'

