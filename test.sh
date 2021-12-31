#!/bin/bash
./recompile
python3 kmeans_pp.py 7 0 test_data/test_data/input_2_db_1.txt test_data/test_data/input_2_db_2.txt &
sudo pmap -x $!

