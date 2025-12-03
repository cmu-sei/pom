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

results_file=$1

# Get all the counts
total_tests=$(cat $results_file | egrep "(pass|FAIL)" | wc -l)

supported_tests=$(cat $results_file | egrep -v "UNSUPPORTED|CAST_TO_PTR_PTR" | egrep "(pass|FAIL):" | wc -l)
bad_total=$(cat $results_file | egrep -v "UNSUPPORTED|CAST_TO_PTR_PTR" | egrep '(pass|FAIL):' | grep BAD | wc -l)
bad_passing=$(cat $results_file | egrep -v "UNSUPPORTED|CAST_TO_PTR_PTR" | grep 'pass:' | grep BAD | wc -l)
bad_failing=$(cat $results_file | egrep -v "UNSUPPORTED|CAST_TO_PTR_PTR" | grep 'FAIL:' | grep BAD | wc -l)

good_total=$(cat $results_file | egrep -v "UNSUPPORTED|CAST_TO_PTR_PTR" | egrep '(pass|FAIL):' | grep GOOD | wc -l)
good_passing=$(cat $results_file | egrep -v "UNSUPPORTED|CAST_TO_PTR_PTR" | grep 'pass:' | grep GOOD | wc -l)
good_failing=$(cat $results_file | egrep -v "UNSUPPORTED|CAST_TO_PTR_PTR" | grep 'FAIL:' | grep GOOD | wc -l)

# Calculate percentages
# (Rounds to the nearest integer by adding half the divisor before dividing)
if [ $total_tests -gt 0 ]; then
    supported_pct=$(((supported_tests * 100 + total_tests / 2) / total_tests))
else
    supported_pct="NaN"
fi

if [ $bad_total -gt 0 ]; then
    bad_passing_pct=$(((bad_passing * 100 + bad_total / 2) / bad_total))
    bad_failing_pct=$(((bad_failing * 100 + bad_total / 2) / bad_total))
else
    bad_passing_pct="NaN"
    bad_failing_pct="NaN"
fi

if [ $good_total -gt 0 ]; then
    good_passing_pct=$(((good_passing * 100 + good_total / 2) / good_total))
    good_failing_pct=$(((good_failing * 100 + good_total / 2) / good_total))
else
    good_passing_pct="NaN"
    good_failing_pct="NaN"
fi

echo "=== ALL TEST CASES ==="
echo "Total number of tests: $total_tests"
echo "Errors: "$(cat $results_file | egrep 'ERROR:' | wc -l)
echo
echo "=== TEST CASES WITHOUT UNSUPPORTED FEATURES ==="
echo "Total number of supported tests: $supported_tests ($supported_pct% of total)"
echo "Total BAD supported tests:       $bad_total"
echo "Passing BAD supported tests:     $bad_passing ($bad_passing_pct%)"
echo "Failing BAD supported tests:     $bad_failing ($bad_failing_pct%)"
echo "Total GOOD supported tests:      $good_total"
echo "Passing GOOD supported tests:    $good_passing ($good_passing_pct%)"
echo "Failing GOOD supported tests:    $good_failing ($good_failing_pct%)"
