#!/bin/bash

# this file is invoked by the makefile, after the tests have been built

{
    err_string=$(find build -type f -executable -print0 | while IFS= read -r -d '' file; do
        $("$file" &> /dev/null)
        if [ $? -eq 0 ]; then
            echo -en "\u2705" # checkmark
        else
            echo -en "\u274C" # X
            # any character to stderr (captured in variable for count)
            echo -n 'z' 1>&2
        fi
        echo -n " "
        echo -- "$file" | cut -c 15-
    done 3>&1 1>&2 2>&3)
} 3>&1 1>&2 2>&3

echo "==================="
err_count=$(echo -n "$err_string" | wc -c)
echo "$err_count errors"
exit "$err_count"
