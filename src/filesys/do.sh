make clean && make all
rm filesys.dsk && pintos-mkdisk filesys.dsk --filesys-size=2 && pintos -f -q
#../../utils/pintos -v -k -T 60 --bochs  --filesys-size=2 -p tests/userprog/exec-once -a exec-once -p tests/userprog/child-simple -a child-simple -- -q  -f run exec-once
cd build
#../../utils/pintos -v -k -T 60 --bochs  --filesys-size=2 -p tests/filesys/extended/grow-file-size -a grow-file-size -- -q  -f run grow-file-size
../../utils/pintos -v -k -T 60 --bochs  --filesys-size=2 -p tests/filesys/extended/dir-vine -a dir-vine -- -q  -f run dir-vine
cd ..
#pintos -p ./build/tests/userprog/exec-once -a exit -- -q
#pintos -q run 'exit'
