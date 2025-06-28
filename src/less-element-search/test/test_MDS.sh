echo > test_MDS.log

./LESS_verify 127 125 2  8    2 >> test_MDS.log
./LESS_verify  44  41 2  8   50 >> test_MDS.log
./LESS_verify 127 124 2 16    2 >> test_MDS.log
./LESS_verify  40  37 3  8   14 >> test_MDS.log
./LESS_verify 127 124 3 16    2 >> test_MDS.log
./LESS_verify  23  19 2  8    6 >> test_MDS.log
./LESS_verify 127 123 2 16   46 >> test_MDS.log
./LESS_verify  17  13 3  8    2 >> test_MDS.log
./LESS_verify 127 123 3 16 1362 >> test_MDS.log
./LESS_verify  16  12 4  8   14 >> test_MDS.log
./LESS_verify 127 123 4 16  635 >> test_MDS.log


echo "We have run 11 test cases (see test_MDS.log for details)"