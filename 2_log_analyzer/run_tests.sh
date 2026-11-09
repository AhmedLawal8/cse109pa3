#!/usr/bin/env bash

# Usage: ./test_runner.sh <executable> <stdout_ref> <file2_ref> <file2_actual> [program_args...]
if [ "$#" -lt 4 ]; then
    echo "Usage: $0 <executable> <stdout_ref> <file2_ref> <file2_actual> [program_args...]"
    echo "Example: $0 my_program stdout_ref.txt file2_ref.txt output2.txt arg1 arg2"
    exit 1
fi

TARGET="$1"
STDOUT_REF="$2"
FILE2_REF="$3"
FILE2_ACTUAL="$4"
shift 4
PROGRAM_ARGS=("$@")

# Define target stdout redirect file
STDOUT_ACTUAL="stdout_actual.txt"

# 1. Execute the target
echo "=========================================="
echo "Executing target: $TARGET"
echo "=========================================="

if [ ! -f "$TARGET" ] && [ ! -x "./$TARGET" ]; then
    echo "Error: Target executable './$TARGET' not found after build."
    exit 1
fi

if [ ! -f "$STDOUT_REF" ]; then
    echo "Error: Standard output reference file '$STDOUT_REF' does not exist."
    exit 1
fi

if [ ! -f "$FILE2_REF" ]; then
    echo "Error: Second reference file '$FILE2_REF' does not exist."
    exit 1
fi

# 2. Run executable and redirect stdout to file
echo -e "\nRunning ./${TARGET} ${PROGRAM_ARGS[*]} > $STDOUT_ACTUAL...\n"
"./$TARGET" "${PROGRAM_ARGS[@]}" > "$STDOUT_ACTUAL"

if [ ! -f "$FILE2_ACTUAL" ]; then
    echo "Error: Expected output file '$FILE2_ACTUAL' was not created by the program."
    exit 1
fi

# Function to compare two files line-by-line ignoring only horizontal spaces/tabs
compare_files() {
    local label="$1"
    local ref_file="$2"
    local actual_file="$3"

    # Temp files for horizontally normalized content (preserving newlines)
    local norm_ref norm_act
    norm_ref=$(mktemp)
    norm_act=$(mktemp)

    # Strip leading and trailing spaces and tabs per line, remove Windows carriage returns (\r), and drop empty lines
    tr -d '\r' < "$actual_file" | sed 's/^[ \t]*//; s/[ \t]*$//' | grep -v '^$' > "$norm_act"
    tr -d '\r' < "$ref_file"    | sed 's/^[ \t]*//; s/[ \t]*$//' | grep -v '^$' > "$norm_ref"

    # Read cleaned normalized lines into arrays (1 array entry per line)
    mapfile -t clean_act < "$norm_act"
    mapfile -t clean_ref < "$norm_ref"

    # Read original lines for display output (skipping blank lines)
    mapfile -t orig_act < <(tr -d '\r' < "$actual_file" | grep -v '^[[:space:]]*$')
    mapfile -t orig_ref < <(tr -d '\r' < "$ref_file"    | grep -v '^[[:space:]]*$')

    rm -f "$norm_act" "$norm_ref"

    local count_act=${#clean_act[@]}
    local count_ref=${#clean_ref[@]}
    local total_lines=$count_ref
    [ "$count_act" -gt "$total_lines" ] && total_lines=$count_act

    local match_count=0
    local mismatch_count=0

    echo "=========================================="
    echo "COMPARISON: $label"
    echo "Expected Ref: $ref_file | Actual Output: $actual_file"
    echo "=========================================="

    if [ "$total_lines" -eq 0 ]; then
        echo "Both files are empty (100% Match)."
    fi
    echo "Total lines to compare: $total_lines"

    for (( i=0; i<total_lines; i++ )); do
        local line_num=$((i + 1))
        local act_str="${clean_act[$i]}"
        local ref_str="${clean_ref[$i]}"

        local act_disp="${orig_act[$i]:-<END OF FILE>}"
        local ref_disp="${orig_ref[$i]:-<END OF FILE>}"

        if [ "$act_str" == "$ref_str" ]; then
            echo -e "Line $line_num: \033[32m[MATCH]\033[0m"
            echo "  Actual output:   $act_disp"
            ((match_count++))
        else
            echo -e "Line $line_num: \033[31m[MISMATCH]\033[0m"
            echo "  Expected output: $ref_disp"
            echo "  Actual output:   $act_disp"
            ((mismatch_count++))
        fi
        echo "------------------------------------------"
    done

    # Return stats separated by space: match mismatch total
    echo "$match_count $mismatch_count $total_lines"
}

# 3. Perform Pair Comparisons
COMP_OUTPUT1=$(compare_files "Standard Output vs Reference" "$STDOUT_REF" "$STDOUT_ACTUAL" | head -n -1)
STATS_PAIR1=$(compare_files "Standard Output vs Reference" "$STDOUT_REF" "$STDOUT_ACTUAL" | tail -n 1)
MATCH1=$(echo "$STATS_PAIR1" | awk '{print $1}')
MISMATCH1=$(echo "$STATS_PAIR1" | awk '{print $2}')
TOTAL1=$(echo "$STATS_PAIR1" | awk '{print $3}')

COMP_OUTPUT2=$(compare_files "Generated File vs Reference File" "$FILE2_REF" "$FILE2_ACTUAL" | head -n -1)
STATS_PAIR2=$(compare_files "Generated File vs Reference File" "$FILE2_REF" "$FILE2_ACTUAL" | tail -n 1)
MATCH2=$(echo "$STATS_PAIR2" | awk '{print $1}')
MISMATCH2=$(echo "$STATS_PAIR2" | awk '{print $2}')
TOTAL2=$(echo "$STATS_PAIR2" | awk '{print $3}')

# Print comparison outputs for both pairs

echo -e "\n\nComparison Results for Pair 1 (Standard Output vs $STDOUT_REF):"
echo "$COMP_OUTPUT1"
echo -e "\n\nComparison Results for Pair 2 ($FILE2_ACTUAL vs $FILE2_REF):"
echo "$COMP_OUTPUT2"

# Helper for score calculation
calc_score() {
    local match=$1
    local total=$2
    if [ "$total" -gt 0 ]; then
        awk "BEGIN {printf \"%.2f\", ($match / $total) * 100}"
    else
        echo "100.00"
    fi
}

SCORE1=$(calc_score "$MATCH1" "$TOTAL1")
SCORE2=$(calc_score "$MATCH2" "$TOTAL2")

TOTAL_MATCH=$((MATCH1 + MATCH2))
TOTAL_MISMATCH=$((MISMATCH1 + MISMATCH2))
TOTAL_COMBINED=$((TOTAL1 + TOTAL2))
SCORE_COMBINED=$(calc_score "$TOTAL_MATCH" "$TOTAL_COMBINED")

# 4. Final Summary
echo -e "\n=========================================="
echo "FINAL SCORE SUMMARY"
echo "=========================================="
echo "Pair 1 (Stdout vs $STDOUT_REF):"
echo "  Matching:     $MATCH1 / $TOTAL1"
echo "  Non-Matching: $MISMATCH1"
echo "  Score:        ${SCORE1}%"
echo "------------------------------------------"
echo "Pair 2 ($FILE2_ACTUAL vs $FILE2_REF):"
echo "  Matching:     $MATCH2 / $TOTAL2"
echo "  Non-Matching: $MISMATCH2"
echo "  Score:        ${SCORE2}%"
echo "------------------------------------------"
echo "Overall Combined Total:"
echo "  Matching:     $TOTAL_MATCH / $TOTAL_COMBINED"
echo "  Non-Matching: $TOTAL_MISMATCH"
echo "  Score:        ${SCORE_COMBINED}%"
echo "=========================================="