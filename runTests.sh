#!/usr/bin/env bash

# If we don't have the arguments, quit
if [ $# -ne 3 ]; then
	echo "Usage: ./runTests <input-dir> <output-dir> <max-threads>"
	exit 1
fi

# Give names to arguments
input_dir="$1"
output_dir="$2"
max_threads="$3"

# If the output doesn't exist or isn't a directory, return Err
if [ ! -d "$output_dir" ]; then
	echo "Output directory doesn't exist!"
	exit 1
fi

# For each test file in the input directory
for cur_file in $(find "$input_dir" -maxdepth 1 -type f); do
	# Set current threads to 1 and go until we hit max threads
	((cur_threads = 1))
	while ((cur_threads <= $max_threads)); do
		# Get the input filename (without extension)
		input_file="$(basename "$cur_file")"
		input_filename="${input_file%.*}"

		# Run the file system and filter stderr to /dev/null
		./tecnicofs "$cur_file" "$output_dir/$input_filename-$cur_threads.txt" "$cur_threads" 2>/dev/null

		# Increase current number of threads
		((cur_threads++))
	done
done
