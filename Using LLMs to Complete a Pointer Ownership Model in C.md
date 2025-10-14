# ETP: Using LLMs to Complete a Pointer Ownership Model in C
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

## The Problem

C/C++ programs are insecure, not memory safe. 
We focus on temporal memory safety, rather than spatial memory safety.

Temporal memory safety CWEs: 
 * [CWE-401: Missing Release of Memory after Effective Lifetime](https://cwe.mitre.org/data/definitions/401.html)
 * [CWE-415: Double Free](https://cwe.mitre.org/data/definitions/415.html)
 * [CWE-416: Use After Free](https://cwe.mitre.org/data/definitions/416.html?)

## Exploits

 * [CVE-2007-0071](https://nvd.nist.gov/vuln/detail/CVE-2007-0071) describes the vulnerability for the first Flash exploit. Made possible by code that failed to check the return value of `calloc()`, and therefore performed pointer arithmetic with a null pointer.
 * [CVE-2013-1347](https://nvd.nist.gov/vuln/detail/CVE-2013-1347) describes a use-after-free vulnerability in MS Internet Explorer 8.
 * [CVE-2003-1048](https://nvd.nist.gov/vuln/detail/CVE-2003-1048) describes a double-free vulnerability in MS Internet Explorer 6.

## Background
### C++ supports [RAII](https://en.cppreference.com/w/cpp/language/raii)
### [Rust](https://www.rust-lang.org/) and its [Borrow Checker](https://doc.rust-lang.org/1.8.0/book/references-and-borrowing.html)
Safe vs unsafe Rust

Rust has "safe references" enforced by the borrow checker and "unsafe pointers" that aren't...In Rust "unsafe pointers" are just as powerful and dangerous as pointers in C/C++.

Enforces a simple model of ownership on 'references'. Enforced by borrow checker. 
(Also prevents data-races, via read-many-write-once semantics.)

Provides encapsulation of unsafe data types, eg. doubly-linked lists

Rust does not prevent memory leaks (CWE-401), b/c less harmful than other temporal memory safety issues

Proves that a compile-time model can work and scale (on many C pointers)
### Translating All C To Rust [TRACTOR](https://creative.gryphontechnologies.com/darpa/i2o/tractor/pd/?p=agenda)
Translating All C To Rust (TRACTOR)
https://creative.gryphontechnologies.com/darpa/i2o/tractor/pd/?p=agenda)

DARPA initiative to fund industry to translate C code to safe / idiomatic Rust

Tom says: B/c of TRACTOR the SEI should not research C->Rust translation.

We avoid this by only using Rust's ownership model to inspire our work. Our work will be restricted to C/C++ code.  It is unlikely (but not impossible) that a TRACTOR proposal would try to solve this problem as part of translation.

### [Pointer Ownership Model](https://insights.sei.cmu.edu/library/pointer-ownership-model/) (POM)
SEI FY13 LENS project. Has its own [SEI Internal Wiki page](https://wiki-int.sei.cmu.edu/confluence/pages/viewpage.action?spaceKey=~svoboda&title=Pointer+Ownership+Model)

Svoboda, David and Wrage, Lutz,
Pointer Ownership Model
2014 47th Hawaii International Conference on System Sciences (HICSS) (2014)
ISBN: 978-1-4799-2504-9,  pp. 5090-5099
http://resources.sei.cmu.edu/library/asset-view.cfm?assetid=55000


Provides properties for pointer ownership in C or C++
Used for temporal memory safety

Responsible/irresponsible pointer == Rust class (Box/Cell) / Rust reference
but did not enforce borrow checker on irresponsible pointers
#### POM Lessons
##### Ambiguity of assignment also applies to memcpy() and other fns that can copy/assign/move pointers
##### Effective zombie cleanup: struct w zombie pointers can be deleted
##### Aliasing problem: element deletion from doubly-linked list
## Our Solution

Develop a model framework for C/C++ code, based on POM and Rust's ownership model.

Develop an "analyzer" that is used to create an ownership model for a C program. (In POM, this analyzer was driven by a human who would decree the ownership of pointers the analyzer could not determine).
The analyzer outputs annotations that make up the model. (prob using ACSL)

Employ an LLM to recognize owning vs. non-owning pointers.
Focus on pointers that could not easily be determined by the research (see related work)
But it is academically interesting to try all pointers, including those that research succeeded on.

This should raise the ratio of unidentified pointers from 83% (via "safer Rust" paper) to ~100%.

Develop a "verifier" that confirms that the model is consistent and complete, based on analyzer output and the program source.

### Rust translation implications

Verifying that a pointer in a C/C++ program obeys our ownership model would be helpful when translating the program to Rust...it would mean the pointer could be translated to a safe Rust pointer class, such as `Box<>`, rather than using an unsafe raw pointer.

There are several other things that would need to be done to use a Rust `mut Box<>` object in place of a C pointer that complies with our model:
 * Verify that the pointed-to object is only accessible from one thread. (If accessible from multiple threads, use Arc instead of Box)
 * Verify that the pointed-to object is not really an array. (If array, use a Rust `Vec` object or something like `str`)
 * Verify that the pointer is never NULL. (if it can be NULL, wrap Optional<> around the object)
 * Verify that the pointer undergoes no type changing (eg void* casts, or C++ inheritance classes)

## Approach

The analyzer and verifier can both utilize ASTs (and perhaps LLVM-IRs) generated by Clang, the same way Redemption did.
But this assumes we have to build them from scratch...we can always try to use pre-existing solutions.

## Related Work
### Safe/Secure C/C++

Plum, Thomas, and David M. Keaton.
Eliminating Buffer Overflows, Using the Compiler or a Standalone Tool
Proceedings of the Workshop on Software Security Assurance Tools, Techniques, and Metrics. 2005.
https://d1wqtxts1xzle7.cloudfront.net/39789032/Eliminating_Buffer_Overflows_Using_the_C20151107-7607-qwodcq-libre.pdf?1446958538=&response-content-disposition=inline%3B+filename%3DEliminating_Buffer_Overflows_Using_the_C.pdf&Expires=1726755510&Signature=Zm-5mpzpPQJae0LD6bza727fzUVl4iwNftz~KoxlXfKW5qLQw-4GfYzKL97vP7ISO0rchgyvkmABJ7VqNlzFNDRHJKMFakTW94YLNRFrBGLB~hgWCtmLtWc80U3fPYn7K1qEQJj~nqCcBmfPcYB9YkFEjYtIABxUNb7A5Bekuse-lrTtpt3V6vnegZ5P0FqiOQU--TdLP6S3~bmWQLyMw-T6x-NO0GTMho~ZBzLzndMFFPh8LbQtlZ3sHvNwJnYBxw01ZT02-jIiqBKINW96D~yddd-3fs6Q65UO4DrPu2f3T6VQT71FaRC4jT2jlhH1OqHe4fyZ4nYGyec5-zShtA__&Key-Pair-Id=APKAJLOHF5GGSLRBV4ZA

### [Frama-C](https://frama-c.com/)
Frama-C, https://frama-c.com/

Platform for static analysis & formal methods for C source code.
Invariants and preconditions modeled with [ACSL](https://en.wikipedia.org/wiki/ANSI/ISO_C_Specification_Language)
Plugin framework, much like [Eclipse](https://www.eclipse.org/)
### [Checked C](https://www.microsoft.com/en-us/research/project/checked-c/)
Microsoft Research,
Checked C
Established: May 15, 2015
https://www.microsoft.com/en-us/research/project/checked-c/

Dialect of C that adds spatial memory safety. Adds type annotations, so Checked-C program is no longer ISO C.
### The [SeL4](https://sel4.systems/) microkernel
Klein, Gerwin,  Andronick, June, Elphinstone, Kevin, Murray, Toby, Sewell, Thomas, Kolanski, Rafal, Heiser, Gernot, 
Comprehensive formal verification of an OS microkernel
ACM Transactions on Computer Systems, Vol. 32, Feb, 2014
https://trustworthy.systems/publications/nictaabstracts/Klein_AEMSKH_14.abstract

### [Fat Pointers For Temporal Memory Safety of C](https://dl.acm.org/doi/pdf/10.1145/3586038) (2023)
Zhou, Jie, Criswell, John, Hicks, Michael
Fat Pointers for Temporal Memory Safety of C
Proceedings of the ACM on Programming Languages, Volume 7, Issue OOPSLA1
Published: 06 April 2023
Article No.: 86, Pages 316 - 347
https://doi.org/10.1145/3586038

Describes an extension to Checked C that adds new pointers that provide temporal memory safety.
### [CBMC for C modelling](https://www.cprover.org/cbmc/) for spatial memory safety
### Translating C to Rust
#### Deterministic translation from C to 'unsafe' rust, using [c2rust](https://c2rust.com/) by Immunant
Immunant, Deterministic translation from C to “unsafe” rust, using c2rust (2018-2024), https://c2rust.com/

Safety not guaranteed, but program can be made more safe by refactoring.
#### [Translating C to Safer Rust](https://dl.acm.org/doi/pdf/10.1145/3485498)
Emre, Mehmet, Schroeder, Ryan, Dewey, Kyle, Hardekopf, Ben,  
Translating C to Safer Rust, 
Proceedings of the ACM on Programming Languages, 
Volume 5, Issue OOPSLA, Article No.: 121, Pages 1 - 29,
https://doi.org/10.1145/3485498

Initial effort to translate unsafe Rust pointers (produced by c2rust) into safe Rust pointers.

83% ((127+69+40)/236) of unsafe functions (made unsafe exclusively via `Lifetime` pointers) made safe using their SA techniques. (Presumably the rest can be made safe manually)

Assesses current SOTA: c2rust is SOTA for now. unsafe->safe Rust problem is hard with lots of (prob solvable) problems
#### [Ownership Guided C to Rust Translation](https://link.springer.com/content/pdf/10.1007/978-3-031-37709-9_22.pdf)
Zhang, H., David, C., Yu, Y., Wang, M. (2023). Ownership Guided C to Rust Translation. In: Enea, C., Lal, A. (eds) Computer Aided Verification. CAV 2023. Lecture Notes in Computer Science, vol 13966. Springer, Cham. https://doi.org/10.1007/978-3-031-37709-9_22

Rust is a relatively new programming language that targets efficient and

Intro contains useful related work.
Mainly about adding some ownership to C2rust output. (converts some C ptrs to Rust refs). using static-analysis, not relying on borrow checker.
Prototype tool: Crown

Doesn't handle: void*, arrays, function pointers, malloc/free wrappers (a common problem)

37% of raw (C-style) pointers translated to Rust `Box<>` (safe & owning) pointers.

### Applying Rust-y Solutions for C/C++
#### [Borrowing Trouble: The Difficulties of a C++ Borrow-Checker](https://docs.google.com/document/d/e/2PACX-1vSt2VB1zQAJ6JDMaIA9PlmEgBxz2K5Tx6w2JqJNeYCy0gU4aoubdTxlENSKNSrQ2TXqPWcuwtXe6PlO/pub)
danakj@chromium.org, lukasza@chromium.org, palmer@chromium.org
Borrowing Trouble: The Difficulties of a C++ Borrow-Checker, 
Google Docs, 10th September 2021
https://docs.google.com/document/d/e/2PACX-1vSt2VB1zQAJ6JDMaIA9PlmEgBxz2K5Tx6w2JqJNeYCy0gU4aoubdTxlENSKNSrQ2TXqPWcuwtXe6PlO/pub

By several Chrome engineers at Google
Tried to implement a borrow checker using C++ classes and compile-time checking. Failed.
#### [C++ Core Guidelines Lifetime Safety Profile](https://github.com/isocpp/CppCoreGuidelines/blob/1b37b50162692d0a50d08e02821b1ed694c12e77/docs/Lifetime.pdf)
Sutter, Herb, 
Lifetime safety: Preventing common dangling
C++ Core Guidelines
Document Number: P1179 R1 – version 1.1
2019-11-22
https://github.com/isocpp/CppCoreGuidelines/blob/1b37b50162692d0a50d08e02821b1ed694c12e77/docs/Lifetime.pdf

spec for Rust-style borrow checking in C++, published in 2019.  Still not supported by MSVC or Clang.
### Other C research papers
Kang, Jeehoon, et al. "A formal C memory model supporting integer-pointer casts." ACM SIGPLAN Notices 50.6 (2015): 326-335.
https://dl.acm.org/doi/abs/10.1145/2813885.2738005
A memory model for C that supports treating pointers as integers

Sammler, Michael, et al. "RefinedC: automating the foundational verification of C code with refined ownership types." Proceedings of the 42nd ACM SIGPLAN International Conference on Programming Language Design and Implementation. 2021.
https://dl.acm.org/doi/10.1145/3453483.3454036
A type model for C, inspired by RustBelt

Priya, Siddharth, and Arie Gurfinkel. "Ownership in low-level intermediate representation." arXiv preprint arXiv:2408.04043 (2024).
An ownership model for LLVM IR

Memarian, Kayvan, et al. "Exploring C semantics and pointer provenance." Proceedings of the ACM on Programming Languages 3.POPL (2019): 1-32.
https://dl.acm.org/doi/abs/10.1145/3290380
Tries to build a provenance model for pointers. (will soon be adopted into C2y)
### Pointer ownership has been successfully added to [SPARK](https://blog.adacore.com/using-pointers-in-spark).
Dross, Claire, 
Using Pointers in SPARK
AdaCore blog, Jun 06, 2019
https://blog.adacore.com/using-pointers-in-spark

### LLM-Related Work
#### [Evaluating the code quality of ai-assisted code generation tools](https://arxiv.org/abs/2304.10778)
Yetiştiren, Burak, et al. 
Evaluating the code quality of ai-assisted code generation tools: An empirical study on github copilot, amazon codewhisperer, and chatgpt.
arXiv preprint arXiv:2304.10778 (2023).
https://arxiv.org/abs/2304.10778

#### [Evaluating AI-Generated Code for C++, Fortran, Go, Java, Julia, Matlab, Python, R, and Rust](https://arxiv.org/pdf/2405.13101)
Diehl, Patrick, Nader, Noujoud, Brandt, Steve, Kaiser, Hartmut,
Evaluating AI-Generated Code for C++, Fortran, Go, Java, Julia, Matlab, Python, R, and Rust
Submitted on 21 May 2024 (v1), last revised 5 Jul 2024, https://arxiv.org/abs/2405.13101

Uses ChatGPT to generate code to solve some math problems in many languages, including Rust, and surveys the results.

#### [Can ChatGPT support software verification?](https://link.springer.com/chapter/10.1007/978-3-031-57259-3_13#:~:text=Our%20evaluation%20shows%20that%20ChatGPT%20can%20support%20software%20verifiers,valid%20and%20useful%20loop%20invariants.)
Janßen, C., Richter, C., Wehrheim, H. (2024). Can ChatGPT support software verification?. In: Beyer, D., Cavalcanti, A. (eds) Fundamental Approaches to Software Engineering. FASE 2024. Lecture Notes in Computer Science, vol 14573. Springer, Cham. https://doi.org/10.1007/978-3-031-57259-3_13

suggests that ChatGPT can be used to create loop invariants with Frama-C.

#### [Function Argument Nullability Using an LLM](https://galois.com/blog/2024/11/function-argument-nullability-using-an-llm/)
Tullsen, Mark, Pernsteiner, Stuart, and Dodds, Mike
Wednesday, November 20, 2024

suggests that GPT 4.o is a good initial SA (but imperfect) for detecting null-ability.
### Later work (during POM work)
#### Sean Baxter, [C++ borrow checker proposal](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2024/p3390r0.html)
Uses 'Circle' compiler (project on Github, supported by Godbolt).
Github project un-documented.
In theory, has to reproduce all of the standard C++ library in 'safe C++'. Yuge effort.
Current impl may only need to be big enough to support doc examples.
Brings into C++ the hard part of Rust (borrow checker) while advertising "you don't have to learn Rust".
## Additional links
### [Learn Rust With Entirely Too Many Linked Lists](https://rust-unofficial.github.io/too-many-lists/index.html)

Strong analysis of Rust's types, esp references, Box, RC.

### [RustiC, a Rust-Like Type System for Temporal Memory Safety in C](https://compilers.cs.uni-saarland.de/publications/theses/schillo_bsc.pdf)
Schillo, Yannick
RustiC, a Rust-Like Type System for Temporal Memory Safety in C
Saarland University, July 14, 2020

Tries to apply Rust's ownership & lifetime semantics to LLVM IR (w/o any external model)

In pg 75, the following tests revealed no unsafe pointers: 8 olden tests except olden-mst and olden-voronoi. Other tests, including aigre, brotli-dec, bstring, bzip2 demonstrated 'unsafe pointers'. (which I bet translate to POM violations.  They modified these codebases to eliminate unsafe pointers.

### [A Lightweight Formalism for Reference Lifetimes and Borrowing in Rust](https://doi.org/10.1145/3443420)
Pearce, David J.
A Lightweight Formalism for Reference Lifetimes and Borrowing in Rust
ACM Transactions on Programming Languages and Systems (TOPLAS), Volume 43, Issue 1
Article No.: 3, Pages 1 - 73
 17 April 2021

Models & proves the safety of Rust borrow checker and lifetimes, using a formalism language.  Used to detect a rustc compiler bug.

# end
