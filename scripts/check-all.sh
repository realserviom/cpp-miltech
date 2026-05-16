#!/bin/bash

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/includes/check.sh"

validate_homework_arg "$1"

TARGET_DIR=$1

bash "$SCRIPT_DIR/check-format.sh" "$TARGET_DIR" 
bash "$SCRIPT_DIR/check-quality.sh" "$TARGET_DIR" 
bash "$SCRIPT_DIR/check-test.sh" "$TARGET_DIR"