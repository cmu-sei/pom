# Quiz about POM on dos2unix
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

## Introduction

This quiz assumes that you have access to dos2unix (for specific details, see the [codebases.yml](codebases.yml) file.

The [design.md](design.md) document specifies the design of POM; you should be familiar with it before trying to answer these questions.

The answers to these questions are all implicit in the [dos2unix.pom.txt](dos2unix.pom.txt) file.

## Questions
### 1 In common.c: d2u_fclose(), should fp be a responsible pointer, and if so what is its input & output states?
Remember, FILE pointers can be responsible even though they get passed to fclose() rather than free().

### 2 In common.c MakeTempFileFrom(), are dir or cpy responsible?
HINT: If you compile dos2unix in the Redemption container, the build process specifies certain values for the preprocessor conditionals. You may assume these conditions for the purpose of this project. That is, you can ignore code that is preprocessed out by these conditions (by #ifdef).

In the Redemption container (eg Ubuntu 24.04) while dirname() and basename() are defined in common.c, they are ifdef'd out by the Redemption container. And POSIX says: The dirname() function may modify the string pointed to by its 'path' argument, and may return a pointer to static storage that may then be overwritten by a subsequent call to dirname().
And POSIX says the same wrt basename().

### 3 In common.c MakeTempFileFrom(), why does the use of fname_str violate POM?  How could you easily fix this?

### 4 In common.c ResolveSymbolicLink(), why does the use of rFN violate POM?  How could you easily fix this?

### 5 In common.c ConvertNewFile(), why does use of TargetFN violate POM?  How could you easily fix this?
HINT: See line 1515 of common.c

### 6 In dos2unix.c main(), is argv_new responsible?  (not *argv_new)  Are there any POM violations with argv_new?
HINT: parse_options has only irresponsible pointers
HINT: glob_warg's argv ptr is responsible, but pFlag is irresponsible
