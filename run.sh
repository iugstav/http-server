#!/bin/sh

set -e
tmpFile=$(mktemp)
gcc -Wall -Wextra -Werror -lz app/*.c -o $tmpFile
exec "$tmpFile" "$@"
