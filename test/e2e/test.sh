#!/bin/bash

run_tests() 
{
    local test_file="$1"

    echo "starting test: $test_file"
    ./build/test/$test_file 2>&1  | tee test_lo
    cat test_output.log
}

check_errors() 
{
    if grep -q -E "(Segmentation fault|Abort)" test_output.log; then
        echo "Segmentation fault occured"
        exit 1
    else
        echo "test passed"
    fi
}

test_files=($(find ./build/test/e2e -type f))
for test_file in $test_files; do
    echo $test_file
    run_tests "$(basename "$test_file")"
    check_errors
done
