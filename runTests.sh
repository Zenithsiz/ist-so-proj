#!/bin/env bash

# If we don't have the arguments, quit
if [ $# -ne 3 ]; then
	echo "Usage: ./runTests <input-dir> <output-dir> <max-threads>"
	exit 1
fi

# For each test in the input directory
# TODO: Fix when `$1` is empty.
for cur_file in "$1"/*; do
	if [ -d "$cur_file" ]; then
		continue
	fi

	((cur_threads = 1))
	while ((cur_threads <= $3)); do
		input_filename="$(basename "$cur_file")"

		echo "inputFile=$cur_file numThreads=$cur_threads"

		./tecnicofs "$cur_file" "$2/$input_filename-$cur_threads.txt" "$cur_threads" 2>/dev/null

		((cur_threads++))
	done
done
