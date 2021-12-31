#!/bin/bash

mkdir leak_reports
curl https://svn.python.org/projects/python/trunk/Misc/valgrind-python.supp > valgrind-python.supp



function process(){
	wait $1
	awk '/HEAP SUMMARY:/{found=1} found' /tmp/reporting.txt > "leak_reports/${2}"
	rm /tmp/reporting.txt
}



valgrind --leak-check=full --log-file=/tmp/reporting.txt --tool=memcheck --suppressions=valgrind-python.supp python3 -E -tt ./kmeans_pp.py 3 333 0 test_data/test_data/input_1_db_1.txt test_data/test_data/input_1_db_2.txt &
process_id=$!
process $process_id "memory_leak_report1.txt"

valgrind --leak-check=full --log-file=/tmp/reporting.txt --tool=memcheck --suppressions=valgrind-python.supp python3 -E -tt ./kmeans_pp.py 7 0 test_data/test_data/input_2_db_1.txt test_data/test_data/input_2_db_2.txt &
process_id=$!
process $process_id "memory_leak_report2.txt"

valgrind --leak-check=full --log-file=/tmp/reporting.txt --tool=memcheck --suppressions=valgrind-python.supp python3 -E -tt ./kmeans_pp.py 15 750 0 test_data/test_data/input_3_db_1.txt test_data/test_data/input_3_db_2.txt &
process_id=$!
process $process_id "memory_leak_report3.txt"
