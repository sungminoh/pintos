make clean && make all
rm filesys.dsk && pintos-mkdisk filesys.dsk --filesys-size=2 && pintos -f -q
#../../utils/pintos -v -k -T 60 --bochs  --filesys-size=2 -p tests/userprog/exec-once -a exec-once -p tests/userprog/child-simple -a child-simple -- -q  -f run exec-once
cd build
../../utils/pintos -v -k -T 60 --bochs  --filesys-size=2 -p tests/userprog/write-normal -a write-normal -p ../../tests/userprog/sample.txt -a sample.txt -- -q  -f run write-normal
cd ..
#pintos -p ./build/tests/userprog/exec-once -a exit -- -q
#pintos -q run 'exit'

