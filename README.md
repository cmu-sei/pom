# Pointer Ownership Model

This is the Pointer Ownership Model (POM) project. It consists of a temporal memory-safety model for C code, a p-model generator that specifies (in a YAML file) the application of POM to a particular program, and an automated verifier that checks whether the program (and if input, the p-model(s)) satisfies the POM constraints.

## Copyright

<legal>  
Pointer Ownership Model (POM) Source Code Release  
  
Copyright 2025 Carnegie Mellon University.  
  
NO WARRANTY. THIS CARNEGIE MELLON UNIVERSITY AND SOFTWARE ENGINEERING  
INSTITUTE MATERIAL IS FURNISHED ON AN "AS-IS" BASIS. CARNEGIE MELLON  
UNIVERSITY MAKES NO WARRANTIES OF ANY KIND, EITHER EXPRESSED OR  
IMPLIED, AS TO ANY MATTER INCLUDING, BUT NOT LIMITED TO, WARRANTY OF  
FITNESS FOR PURPOSE OR MERCHANTABILITY, EXCLUSIVITY, OR RESULTS  
OBTAINED FROM USE OF THE MATERIAL. CARNEGIE MELLON UNIVERSITY DOES NOT  
MAKE ANY WARRANTY OF ANY KIND WITH RESPECT TO FREEDOM FROM PATENT,  
TRADEMARK, OR COPYRIGHT INFRINGEMENT.  
  
Licensed under a MIT (SEI)-style license, please see license.txt or  
contact permission@sei.cmu.edu for full terms.  
  
[DISTRIBUTION STATEMENT A] This material has been approved for public  
release and unlimited distribution.  Please see Copyright notice for  
non-US Government use and distribution.  
  
DM25-1262  
</legal>  

## Build instructions

The code is designed to run inside a Docker container, which means you will need [Docker](https://www.docker.com/). In particular, once you can run containers, you will need to share filesystems between your host and the various containers. For that you want the `-v` switch in your `docker run` command; its documentation is available here: <https://docs.docker.com/reference/cli/docker/container/run/#volume>

To build the Docker container:

``` bash
docker build -f Dockerfile -t docker.cc.cert.org/pom/pointer-ownership-model:latest .
```

For SEI users, we provide a useful container image for working with POM. It is available via the SEI IT Artifactory: `docker.cc.cert.org/pom/pointer-ownership-model:latest`.

The image contains all the dependencies needed to run the repair tool, but it does not actually contain the repair tool itself. This container is useful if you wish to debug or extend the tool itself, as you can edit files on your host and immediately access them in the container.

To pull the image:

``` bash
docker pull docker.cc.cert.org/pom/pointer-ownership-model:latest
```

In the examples below, we assume you have created a directory, in which you will put your codebases to analyze for temporal memory safety by building p-models for the codebase and then validating the p-model against the codebase.
Your filepath to that directory should be substituted for `<FILEPATH_TO_CODEBASES_DIR>`.

To start a Bash shell in the container:

``` bash
docker run -it --rm -v ${PWD}:/host -v <FILEPATH_TO_CODEBASES_DIR>:/datasets -w /host docker.cc.cert.org/pom/pointer-ownership-model:latest bash
```

Our p-model builder script interacts with OpenAI's API. Ubuntu and other Linux users may choose to use the `secret-tool` to store their OpenAI API Key. (e.g., `secret-tool store --label="OPENAI_API_KEY" service openai  key api_key` and then provide the key at the `secret-tool`'s prompt). To pass that environment variable at Docker run time, those users can start the container as follows:

``` bash
docker run -it --rm  -e OPENAI_API_KEY="$(secret-tool lookup service openai key api_key)" -v ${PWD}:/host -v <FILEPATH_TO_CODEBASES_DIR>:/datasets -w /host docker.cc.cert.org/pom/pointer-ownership-model:latest bash
```

## Source Code

We are using the `dos2unix` codebase, version 7.5.2. It is freely available from [dos2unix.sourceforge.io/](https://sourceforge.net/projects/dos2unix/files/dos2unix/7.5.2/dos2unix-7.5.2.tar.gz/download).  For this demo, you should download `dos2unix-7.5.2`, unpack it, and place its contents in a `dos2unix-7.5.2` sub-directory that is available to the POM container (e.g., `mv dos2unix-7.5.2 <FILEPATH_TO_CODEBASES_DIR>`). You could use this command to unpack, which creates the required `dos2unix-7.5.2` sub-directory:

```sh
tar xzf dos2unix-7.5.2.tar.gz
```

## Running the p-model Builder

Input to the p-model builder includes the source code and output from the Clang compiler tool. The Clang output needed is an abstract syntax tree (AST) and an intermediate representation (IR) files (LLVM IR files are `.ll` files).

There are two methods to create a p-model file: LLM-based or SAT-solver-based. The SAT-solver method creates a p-model after verifying that the program satisfies POM constraints, and cannot create a p-model for the program if it doesn't. The LLM-based method can create a p-model for the program, regardless of a SAT-solver's determination. If you only use an LLM to generate a p-model, you don’t know if the code is compliant or if the LLM made a mistake.

### Creating a compilation database

The following command, when run in the POM container, creates compilation database file (named `compile_commands.json`) for dos2unix:

``` sh
cd /datasets/dos2unix-7.5.2
bear -- make CC=clang dos2unix
make clean
mv compile_commands.json  ..
cd ..
```

(For this example, we are ignoring the `unix2dos` program which is also part of Dos2Unix.)

### Creation of ASTs and LLVM intermediate representation (IR) files

The next step is to run the `make_run_clang.py` script. This produces a shell script for running Clang on the source code. You must run this shellscript explicitly, as the next step.

This step requires the `compile_commands.json` file produced in the previous step.

```sh
cd dos2unix-7.5.2
mkdir /host/d2u # already exists, if you ran the Docker container command above, mounting the volume
python3 /host/src/make_run_clang.py  -c /datasets/compile_commands.json  --output-clang-script /host/d2u/run_clang.sh
```

### Running the Clang shell script

Next, the Clang shellscript should be run. This script takes one argument: a temporary directory that contains the AST(s) and .ll files generated by Clang.

This step requires the `run_clang.sh` file from the previous step.

```sh
/bin/sh -f /host/d2u/run_clang.sh /host/d2u
```

### Running the LLM-based p-model Builder

You now have the AST and source code. With those as input, you can run the LLM-based Builder. To do this, you must have the `OPENAI_API_KEY` environment variable set up as per OpenAI's instructions for accessing GPT from the command line, and your machine must be able to access GPT. (For an example of how provide your `OPENAI_API_KEY` to the container, see `docker run` instruction including `secret-tool lookup` in the [Build instructions](#build-instructions)) section.

If you have more than 3 files that match `/host/d2u/*.ast.json.gz`, delete all but the newest 3.

```sh
cd /host/src
python3 build.py --verbose /host/src/dos2unix.llm.pom.yml /datasets/dos2unix-7.5.2  /host/d2u/*.ast.json.gz
```

This will produce a `/host/src/dos2unix.llm.pom.yml`, the p-model constructed by GPT.

### Running the SAT-solver based p-model Builder (SAT-builder method)

The script `props_to_pom_yaml.py` (in directory `src/constraint_gen/`) uses the SAT-solver output solution JSON-formatted file to generate a p-model.

The SAT-builder method can be used if the program has only one function or if the SAT-solver is used in whole-program mode.

In whole-program mode, the SAT solver still needs p-models for the standard library functions.

You can create and save p-models for standard library functions. You can do this using the SAT-builder method or using the LLM-based method provided in Section [Undefined Function p-models](#undefined-function-p-models). We provide p-models for some library functions in `src/test/test.famous.llm.pom.yml` (e.g., `free`, `strlen`, `strcmp`).  What the SAT solver needs for library functions is their index numbers and their names. If you have a program that calls library functions, then you should provide a p-model for the library and also provide an argument index ID for each function argument.  (See [./d2u/dos2unix.famous.sorted.pom.yml](./d2u/dos2unix.famous.sorted.pom.yml) for examples with field `arg_idx` providing the argument index ID. Some examples in that file include the `chmod` and `chown` GNU library functions.)

An example using the SAT-builder method to create a p-model file `myprogram.pom.yml`, from directory `src/constraint_gen/`, after getting SAT output from the [SAT-solver verifier](sat-solver-based-verifier):

`./props_to_pom_yaml.py out/solution.json --lib ../test/test.famous.llm.pom.yml > myprogram.pom.yml`


### Verifying Builder Output

The following files can be used to verify that the builder's p-model is correct.

 * `/host/d2u/dos2unix.manual.pom.yml`

A model created by hand while studying the dos2unix source code

 * `/host/d2u/dos2unix.sorted.pom.yml`

This is the manual p-model file, but missing comments, and all the keys are sorted.

You can see any differences between the file produced by the LLM or SAT-solver and the sorted file using `diff`:

```sh
diff -u /host/src/dos2unix.llm.pom.yml /host/d2u/dos2unix.sorted.pom.yml
```

If there are no differences from the LLM-produced p-model, then the LLM was 100% accurate in generating the model.


## Running the Verifier

You should run the verifier to confirm that the code complies with POM and (if you are using an already-produced p-model for the program) that the p-model is complete.

We provide a [pre-existing p-model](./d2u/dos2unix.manual.pom.yml) for dos2unix.

The current verifier uses a SAT solver. There is also an older, incomplete verifier that checks if the AST complies with the POM. The following sections describe each.

### SAT-solver based verifier

This verifier examines the LLVM IR generated from the source code.  It runs our constraint generator and a SAT solver.

Optionally, input can include p-model files, as discussed in Section [Running the p-model Builder](#running-the-p-model-builder). The schema used by the p-model files is formally specified in [pom.yamale.yml](./src/pom.yamale.yml).

The constraint generator takes exactly one LLVM IR file as input. You can use [llvm-link](https://llvm.org/docs/CommandGuide/llvm-link.html) to produce a combined file (use the clang version as `$CLANG_VER` in the following command, e.g., `llvm-link-15`):

`llvm-link-$CLANG_VER input_1.raw.ll input_2.raw.ll ... input_N.raw.ll -S -o combined.raw.ll`

[run_solver.sh](./src/run_solver.sh) automatically does the `llvm-link` step if you have multiple `.ll` files.

Run the verifier step using [./src/run_solver.sh](./src/run_solver.sh) or [./src/constraint_gen/run.sh](./src/constraint_gen/run.sh). [run_solver.sh](./src/run_solver.sh) iterates through all of the functions and analyzes each function separately and reports SAT or UNSAT for each function. In contrast, [run.sh](./src/constraint_gen/run.sh) inputs one `.ll` file and analyzes either just a single function or the whole program in whole-program mode. To run the constraint generator in whole-program mode, set the `LLVM_OPTS` environment variable to that, e.g.: `export LLVM_OPTS="-whole-prog"`.

As input, `run_solver.sh` takes one or more `.ll` files, plus optional arguments including the p-model for the codebase, library function p-models, an output directory, a clang version (e.g., `18`), and whether to output results in JSON format.

Command-line arguments to `run.sh` consist of a single `.ll` file and optionally the output directory (defaults to "out" in the current directory). One or more ".pom.yml" files (which specify p-models of functions) can be provided via the `POM_FILE` environment variable. (Multiple ".pom.yml" files can specified like this: `POM_FILE="file_1.pom.yml file_2.pom.yml ... file_N.pom.yml"`.) The environment variable `FUNC` specifies which function to analyze; the default is to analyze all functions.

The verifier output is designed to help developers to quickly understand if the code is POM-compliant or not.

* If the finding is `SAT` (it is POM-compliant), developers are provided with the validation detail.
* If the finding is `UNSAT` (it is not POM-compliant), developers are pointed to an UNSAT core (a subset of the original clauses that is sufficient to prove unsatisfiability) to help them understand the problem so they can fix it. Related output files provide traces to both the `.c` code (with line numbers and variable names) and `.ll` code (with line numbers and variable names).

If the constraints are satisfiable, these files are generated:

- `solution.json`: Assignment of `true` or `false` to each of the named variables appearing in `constraints.txt`.
- `solution.txt`: Raw output from SAT Solver, using numeric variable IDs.

If the constraints are unsatisfiable, these files are generated:

- `proof.drat`: A proof of unsatisfiability.
- `core.unsat`: The subset of clauses from `constraints.dimacs` that are used in the above proof.
- `core.unsat.named`: Same as `core.unsat`, except using descriptive variable names instead of numeric variable IDs.

#### SAT-solver verifier detail, four demos, and more

More detail about our SAT-solver verifier, along with demonstration examples with detailed steps (one `SAT`, one `UNSAT`, one using an input p-model file, and a demo of summarizing a function), with .c source files, commands, and explanation of outputs (plus more) is provided [here](src/constraint_gen/README.sat.solver.md).
Some differences between these examples from other use cases and the dos2unix example in the Builder section above:

- These examples start by calling `compile_to_ll.sh`: This compiles the single `.c` file example into `.ll` (LLVM IR), accomplishing that aspect of the build step.
- These examples do not use `make_run_clang.py` as in the dos2unix example above. However, you can use the build instructions in the [Running the p-model Builder](running-the-p-model-builder) (for dos2unix or another program) to prepare for and then run clang, to produce `.ll` files from your source code, and then use those `.ll` files with the SAT-solver verifier.
- Although the `compile_to_ll.sh` script uses some extra arguments to clang (`ggdb` and `-fexperimental-assignment-tracking=enabled`), those do not change the output compared to running clang with arguments provided by `make_run_clang.py`.


#### Example SAT-solver verification building on the previous dos2unix example steps

The dos2unix LLVM IR `.ll` files are in your container's `/host/d2u` directory.

Combine the LLVM IR files (use the filenames you get, which may have different strings before the `raw.ll` part of the filenames:
```
cd /host/d2u
llvm-link-15 common.5ba35ca31f1cf1a66823b734.raw.ll dos2unix.955979806ce1344d61d417cc.raw.ll querycp.8654988538a257e3708d2bad.raw.ll -S -o combined.raw.ll
```

Run `run.sh` in whole-program mode on that single combined `.ll` file:

```
cd /host/src/constraint_gen
export LLVM_OPTS="-whole-prog"
./run.sh /host/d2u/combined.raw.ll outd2u_combined
```
Output says `SATISFIABLE` but with many warnings `Warning: an argument of function <FUNCTION_NAME> is missing a name.`

In the general case, you would next create a p-model for each of the warned-about library functions. We did that already, to create [./d2u/dos2unix.famous.manual.pom.yml](./d2u/dos2unix.famous.manual.pom.yml).
Note: dos2unix uses the POSIX library function `dirname` on POSIX builds but defines its own `dirname` function for Windows builds. We use the p-model for POSIX library function `dirname` in `dos2unix.famous.manual.pom.yml`. Separately, we use the p-model for the dos2unix custom `dirname` function (that is only active on the Windows configuration of dos2unix) in `dos2unix.manual.pom.yml`.

```
cd /host/src/constraint_gen
POM_FILE="/host/d2u/dos2unix.sorted.pom.yml /host/d2u/dos2unix.famous.manual.pom.yml"  ./run.sh /host/d2u/combined.raw.ll outd2u_comb_fam
```

The output says `SATISFIABLE` with no warnings about arguments missing a name.

There are some warnings that say `Warning: indirect function calls (i.e., calls via function pointers) are not supported`. That's because dos2unix has function pointers. Currently, we treat calls via function pointers as no-ops.


The borrow design (work in progress) is described [here](src/constraint_gen/borrow_design.txt).


### AST based p-model verifier

Our AST verifier examines an AST file generated from the source code. It also takes the p-model as input.

Execution of the following AST verifier example requires all of the `.ast.json.gz` and `.ll` files from the `temp` directory that were produced in the example step above [Running the p-model Builder](#running-the-p-model-builder).

This command examines the AST of the dos2unix file `common.c`:

``` bash
cd /host
python3 src/verify.py src/dos2unix.llm.pom.yml d2u/common.*.raw.ast.json.gz > verifier.output
```

The output indicates the details the verifier analyzed. The verifier also warns about any differences between the p-model file's functions and the AST's functions. Since the p-model contains all functions in the dos2unix program, the verifier issues warnings about functions not in common.c, such as `main` and standard C functions like `stoi`.

The AST verifier is incomplete. It only confirms that the pointers listed in the functions' arguments, local variables, and return values are listed both in the code and the p-model.


## Making Code Extracts from Design Document

Our design document includes a lot of code examples that the POM design considered. Some are common code constructs and others are corner cases that we determined should be within or outside of the scope of POM constraints. You can check how POM (and your selected LLM) handles these code examples, starting by automatically extracting code from the document as follows:

Note: this does not need to be run in the `docker.cc.cert.org/pom/pointer-ownership-model` if Python 3 is available on the host system

1. Clone [Markdown Code Extractor](https://github.com/sei-svishnubhatla/markdown-code-extractor) into repository root

``` bash
git clone https://github.com/sei-svishnubhatla/markdown-code-extractor.git
```

2. Run MCE on design document and output to [`design-extracts`](./design-extracts/)

``` bash
python3 markdown-code-extractor/markdown-code-extractor.py design.md design-extracts
```

## Troubleshooting
### Git error

If you get this error inside your container:

``` text
!fatal: detected dubious ownership in repository at 'dos2unix'
To add an exception for this directory, call:
```

Then, run the `git config` command as recommended. (This error can occur because shared files inside one Docker container may have a different owner than the same files outside the container.)

``` bash
git config --global --add safe.directory dos2unix
```

### I cannot install `bear`

The `bear` command is useful for building compilation databases, aka the `compile_commands.json` file, which is used by POM to understand how software is built. For more info on `bear`, see: <https://github.com/rizsotto/Bear>.

If you cannot install or run Bear on the build platform, you must create the compilation database via other means. The [format of the JSON compilation database](https://clang.llvm.org/docs/JSONCompilationDatabase.html) contains a specification of the compilation database.  It also contains instructions for producing this database using Clang.  The format is fairly straightforward, and so you might be able to produce the file manually.

### Undefined Function p-models

The Verifier may give you errors saying that several common functions, like `strcpy` have no associated p-models. You need to provide p-models for these functions.

For every function name input, our script `build_famous_fn_pom.py` interacts with GPT and builds a POM signature for that function, dealing with its arguments and return value.  This will only work if the function is "famous" so that GPT can find out what its signature and documentation is.

``` bash
cd /host/src
python3 build_famous_fn_pom.py --verbose  <function1> <function2> ...
```

You may call this with as many arguments as you want, or call it once for each argument.

Once you have valid results for each function, you can compile them all together into a p-model using the command:

``` bash
python3 build_famous.yml.py --verbose  dos2unix.famous.llm.pom.yml
```

## Software Bill of Materials

This project uses the following open-source software.  All of this software is installed by the Dockerfile, except for ghostq, which lives in [src/constraint_gen/ghostq](src/constraint_gen/ghostq):

| Software           | Version     | Host   | License                |
| [Clang](https://clang.llvm.org)         | 15.0.7      | Github | Apache 2               |
| [yamale](https://github.com/23andMe/Yamale)        | 6.0.0       | Github | MIT                    |
| [cryptominisat](https://www.msoos.org/cryptominisat5) | 5.11.15     | Github | MIT + GPL for Bliss    |
| [minisat](http://minisat.se)       | 2.2.1       | self   | OSS (Niklas Sorensson) |
| [drat-trim](https://github.com/marijnheule/drat-trim)     | v05.22.2023 | Github | MIT                    |
| [ghostq](https://www.wklieber.com/ghostq)        |             | self   | GPL 3                  |

## Contributions to the Pointer Ownership Model are Welcome

Contributions to the Pointer Ownership Model codebase are welcome!  If you have code repairs or other contributions, we welcome that - you could submit a pull request via GitHub, or contact us if you'd prefer a different way.
