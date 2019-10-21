#!/bin/bash

usage()
{
	printf "Usage: %s OUTPUT.CSV FILE.CSV [FILE.CSV...]\n" "$(basename "$0")"
}

checkargs()
{
	if [ "$#" -lt 2 ]; then
		usage
		exit 2
	fi

	local args=("$@")

	if [ -f "${args[0]}" ]; then
		printf "%s: the output file already exits. It won't be overwritten.\n" \
			   "${args[0]}"
		exit 1
	fi

	for file in "${args[@]:1}"; do
		if [ ! -f "$file" ]; then
			printf "%s: file not found\n" "$file"
			exit 1
		fi
	done
}

to_eureqa()
{
	local _output_file="$1"
	local _input_file="$2"
	local _include_header="$3"

	out="$(cut -d';' -f 1,2,3,4,5,6,7 "$_input_file" | tr ';' ' ')"

	if [ "$_include_header" -eq 1 ]; then
		echo "$out" >> "$_output_file"
	else
		echo "$out" | tail -n +2  >> "$_output_file"
	fi
}

main()
{
	checkargs "$@"

	local _output_file="$1"; shift

	local include_header=1
	for input_file in "$@"; do
		to_eureqa "$_output_file" "$input_file" "$include_header"
		[ "$include_header" -eq 1 ] && include_header=0
	done
}

main "$@"
