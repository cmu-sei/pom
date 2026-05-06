#!/bin/bash
# <legal>
# Pointer Ownership Model (POM) Source Code Release
# 
# Copyright 2025 Carnegie Mellon University.
# 
# NO WARRANTY. THIS CARNEGIE MELLON UNIVERSITY AND SOFTWARE ENGINEERING
# INSTITUTE MATERIAL IS FURNISHED ON AN "AS-IS" BASIS. CARNEGIE MELLON
# UNIVERSITY MAKES NO WARRANTIES OF ANY KIND, EITHER EXPRESSED OR
# IMPLIED, AS TO ANY MATTER INCLUDING, BUT NOT LIMITED TO, WARRANTY OF
# FITNESS FOR PURPOSE OR MERCHANTABILITY, EXCLUSIVITY, OR RESULTS
# OBTAINED FROM USE OF THE MATERIAL. CARNEGIE MELLON UNIVERSITY DOES NOT
# MAKE ANY WARRANTY OF ANY KIND WITH RESPECT TO FREEDOM FROM PATENT,
# TRADEMARK, OR COPYRIGHT INFRINGEMENT.
# 
# Licensed under a MIT (SEI)-style license, please see license.txt or
# contact permission@sei.cmu.edu for full terms.
# 
# [DISTRIBUTION STATEMENT A] This material has been approved for public
# release and unlimited distribution.  Please see Copyright notice for
# non-US Government use and distribution.
# 
# DM25-1262
# </legal>
if [ $# -lt 1 ] || [ ! -r "$1" ]; then
    echo "Usage: $0 <results_file>" >&2
    exit 1
fi

results_file=$1

# Get all the counts
total_tests=$(cat $results_file | egrep "(pass|FAIL):" | wc -l)

supported_tests=$(cat $results_file | egrep -v "UNSUPPORTED|CAST_TO_PTR_PTR" | egrep "(pass|FAIL):" | wc -l)
unsupported_tests=$((total_tests - supported_tests))
bad_total=$(cat $results_file | egrep '(pass|FAIL):' | grep BAD | wc -l)
bad_supported=$(cat $results_file | egrep -v "UNSUPPORTED|CAST_TO_PTR_PTR" | egrep '(pass|FAIL):' | grep BAD | wc -l)
bad_unsupported=$((bad_total - bad_supported))
bad_passing=$(cat $results_file | egrep -v "UNSUPPORTED|CAST_TO_PTR_PTR" | grep 'pass:' | grep BAD | wc -l)
bad_failing=$(cat $results_file | egrep -v "UNSUPPORTED|CAST_TO_PTR_PTR" | grep 'FAIL:' | grep BAD | wc -l)

good_total=$(cat $results_file | egrep '(pass|FAIL):' | grep GOOD | wc -l)
good_supported=$(cat $results_file | egrep -v "UNSUPPORTED|CAST_TO_PTR_PTR" | egrep '(pass|FAIL):' | grep GOOD | wc -l)
good_unsupported=$((good_total - good_supported))
good_passing=$(cat $results_file | egrep -v "UNSUPPORTED|CAST_TO_PTR_PTR" | grep 'pass:' | grep GOOD | wc -l)
good_failing=$(cat $results_file | egrep -v "UNSUPPORTED|CAST_TO_PTR_PTR" | grep 'FAIL:' | grep GOOD | wc -l)

# Calculate percentages
# (Rounds to the nearest integer by adding half the divisor before dividing)
if [ $total_tests -gt 0 ]; then
    supported_pct=$(((supported_tests * 100 + total_tests / 2) / total_tests))
    unsupported_pct=$((100 - supported_pct))
else
    supported_pct="NaN"
fi

if [ $bad_total -gt 0 ]; then
    bad_passing_pct_tot=$(((bad_passing * 100 + bad_total / 2) / bad_total))
    bad_failing_pct_tot=$(((bad_failing * 100 + bad_total / 2) / bad_total))
    bad_unsup_pct_tot=$(((bad_unsupported * 100 + bad_total / 2) / bad_total))
else
    bad_passing_pct_tot="NaN"
    bad_failing_pct_tot="NaN"
    bad_unsup_pct_tot="NaN"
fi

if [ $good_total -gt 0 ]; then
    good_passing_pct_tot=$(((good_passing * 100 + good_total / 2) / good_total))
    good_failing_pct_tot=$(((good_failing * 100 + good_total / 2) / good_total))
    good_unsup_pct_tot=$(((good_unsupported * 100 + good_total / 2) / good_total))
else
    good_passing_pct_tot="NaN"
    good_failing_pct_tot="NaN"
    good_unsup_pct_tot="NaN"
fi


if [ $bad_supported -gt 0 ]; then
    bad_passing_pct_sup=$(((bad_passing * 100 + bad_supported / 2) / bad_supported))
    bad_failing_pct_sup=$(((bad_failing * 100 + bad_supported / 2) / bad_supported))
else
    bad_passing_pct_sup="NaN"
    bad_failing_pct_sup="NaN"
fi

if [ $good_supported -gt 0 ]; then
    good_passing_pct_sup=$(((good_passing * 100 + good_supported / 2) / good_supported))
    good_failing_pct_sup=$(((good_failing * 100 + good_supported / 2) / good_supported))
else
    good_passing_pct_sup="NaN"
    good_failing_pct_sup="NaN"
fi

echo "=== SUMMARY STATISTICS ==="
echo "Total number of tests: $total_tests ($good_total GOOD and $bad_total BAD)"
echo "Errors: "$(cat $results_file | egrep 'ERROR:' | wc -l)
echo
echo "Total number of supported tests: $supported_tests ($supported_pct% of total)"
echo "Total number of unsupported tests: $unsupported_tests ($unsupported_pct% of total)"
echo "Total BAD supported tests:       $bad_supported"
echo "Passing BAD supported tests:     $bad_passing ($bad_passing_pct_sup% of supported BAD tests, $bad_passing_pct_tot% of all BAD tests)"
echo "Failing BAD supported tests:     $bad_failing ($bad_failing_pct_sup% of supported BAD tests, $bad_failing_pct_tot% of all BAD tests)"
echo "Total GOOD supported tests:      $good_supported"
echo "Passing GOOD supported tests:    $good_passing ($good_passing_pct_sup% of supported GOOD tests, $good_passing_pct_tot% of all GOOD tests)"
echo "Failing GOOD supported tests:    $good_failing ($good_failing_pct_sup% of supported GOOD tests, $good_failing_pct_tot% of all GOOD tests)"
echo
echo "=== PIE CHARTS ==="
echo "GOOD test cases: $good_passing ($good_passing_pct_tot%) passing, $good_failing ($good_failing_pct_tot%) failing, $good_unsupported ($good_unsup_pct_tot%) unsupported."
echo "BAD test cases: $bad_passing ($bad_passing_pct_tot%) passing, $bad_failing ($bad_failing_pct_tot%) failing, $bad_unsupported ($bad_unsup_pct_tot%) unsupported."
