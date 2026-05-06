# Below are the steps for running POM on Juliet (using only the whole-program
# SAT approach, not the LLM) and reproducing the numbers in the last sentence
# of the abstract ("81% of the 2,302 memory-safe examples were correctly
# recognized as memory-safe") and in Figure 6 of the paper.

# Build constraint generator
cd /host/src/constraint_gen/
make clean
OPT_FLAG="-O3" make

# Make sure simple tests pass
cd /host/src/test
./run_test.py *.expect.json
# It should report "Number of failing tests: 0"

# Get the Juliet-subset zip
# The file `juliet-subset.zip` can be created from the official SARD distribution (https://samate.nist.gov/SARD/test-suites/112) as follows:
# zip --copy 2017-10-01-juliet-test-suite-for-c-cplusplus-v1-3.zip --out juliet-subset.zip "C/testcasesupport/*" "C/testcases/CWE401_Memory_Leak/*" "C/testcases/CWE415_Double_Free/*" "C/testcases/CWE416_Use_After_Free/*" "C/testcases/CWE590_Free_Memory_Not_on_Heap/*" "C/testcases/CWE761_Free_Pointer_Not_at_Start_of_Buffer/*"

# Run Juliet tests
cd /host/src/test/juliet
unzip $DATA_DIR/juliet-subset.zip # where $DATA_DIR is where juliet-subset.zip can be found.
mkdir out
export OUT_DIR=$PWD/out
export COMPILER_OPTS="-I/host/src/test/juliet/C/testcasesupport/"
export POM_FILE=/host/src/test/test.famous.llm.pom.yml
/host/src/constraint_gen/compile_to_ll.sh C/testcasesupport/io.c -o $PWD/io.raw.ll -c $COMPILER_OPTS
time ./run_with_conv_dimacs_coproc.sh ./process_juliet_testsets.py --testcases-dir ./C/testcases --random-order -o results.txt --error-log errors.txt
# Expected: warnings about missing debug info, compiler warnings from Clang.
# On my machine, each of the 2,302 test cases required about 0.5 seconds, for a total time of about 20 minutes.

# Show summary statistics
./summarize_results.sh results.txt
# The last two lines of the output are expected to be:
#   GOOD test cases: 1866 (81%) passing, 206 (9%) failing, 230 (10%) unsupported.
#   BAD test cases: 2072 (90%) passing, 0 (0%) failing, 230 (10%) unsupported.
