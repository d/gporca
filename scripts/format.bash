#!/bin/bash

set -e -u

: "${CLANG_FORMAT:=clang-format}"

format() {
	git ls-files --full-name :!scripts '*.cpp' '*.h' '*.inl' | parallel --quote -X --max-chars 16384 "${CLANG_FORMAT}" -i {} {}
}

stage() {
	git add -u :!.clang-format :!scripts
}

diff() {
	git -c core.pager='less -x1,5' diff "$@" -- :!.clang-format :!scripts
}

restore_code() {
	git checkout HEAD -- :!.clang-format :!scripts
}

turn_off_reflow_comments() {
	ruby -ryaml <<-RUBY
	config = YAML.load_file(".clang-format").merge("ReflowComments" => false)
	File.write(".clang-format",
		YAML.dump(config))
	RUBY
}

turn_on_reflow_comments() {
	ruby -ryaml <<-RUBY
	config = YAML.load_file(".clang-format").merge("ReflowComments" => true)
	File.write(".clang-format",
		YAML.dump(config))
	RUBY
}

generate_diff() {
	git checkout HEAD -- .clang-format
	# defensively ensure comments are not reflowed
	turn_off_reflow_comments
	format
	stage

	# now turn on reflow
	turn_on_reflow_comments

	format
	diff > reflow.diff

	turn_off_reflow_comments
	restore_code
}

_main() {
	if [ $# -lt 1 ]; then
		help
		return 0;
	fi

	local -r command=$1
	shift
	case $command in
		generate_diff)
			generate_diff
			;;
		format)
			format
			;;
		stage)
			stage
			;;
		baseline)
			format
			stage
			;;
		diff)
			diff "$@"
			;;
		restore_code)
			restore_code
			;;
		*)
			printf 'unexpected command: %s\n' "${command}"
			help
			return 1;
	esac
}

help() {
	local -a -r commands=(
	generate_diff
	format
	stage
	baseline
	diff
	restore_code
	)
	printf 'usage: %s <command>\n' "$0"
	(
	IFS=,
	printf 'supported commands: %s\n' "${commands[*]}"
	)
}

_main "$@"
