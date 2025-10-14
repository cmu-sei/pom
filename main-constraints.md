# Main constraints

This describes generation of constraints other than those for borrowing/lifetimes.
See `borrow_design.txt` for borrowing/lifetimes constraints.

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


## Type of atomic propositions used in constraints
Type-qualifier properties (doesn't depend on program point):
- "responsible(p)": indicates that p is a responsible pointer.
- "irresponsible(p): indicates that p is an irresponsible pointer.
- "mut(p): indicates that p can be used to write to memory (as opposed to only read from memory).
- "outscope(p)": indicates p is an out-of-scope pointer.

State properties (depends on program point) for pointers:
- "good(p, t)": indicates that p MAY be a good pointer at program point t.
- "null(p, t)": indicates that p MAY be a null pointer at program point t.
- "zombie(p, t)": indicates that p MAY be a zombie at program point t.  A pointer variable is a zombie if it is uninitialized or it is freed or its value has been assigned to another responsible pointer variable.

A *program point* is a point immediately before or immediately after a given statement is executed.  Given a statement *S*, we write *S-pre* to denote that program point before *S* and we write *S-post* to denote the program point after *S*.

In the absence of structs, unions, and arrays, a *C-path* is a local or global variable with zero or more dereferences.  E.g., if `v` is a variable, then `v`, `*v`, and `**v` are all C-paths.

Note: Initially, we started with C-paths like `**p` (and this is still used in this file) but we switched to `p[0][0]` (which is used in `borrow_design.txt`).  Some parts of the implementation still use the old notation, but there are helper functions that interconvert where necessary.

## Invariants

Our constraints are designed to establish the following invariants:
 - At every program point, for every memory block M allocated via malloc/calloc/realloc, there is exactly one location that holds a good responsible pointer that points to M.  (Here, the term "location" encompasses both allocated memory locations and LLVM temporaries.)
 - At every program point, for every memory location M, there is at most one C-path that can be used to write to M.  Such a C-path must be marked `mut` and must have no outstanding borrows.
 - No irresponsible pointer is a dangling reference to a memory block that has already been freed.
 - ...

## Constraint Generation

We generate constraints for (1) specifying what happens during execution of a statement (an LLVM instruction) and (2) connecting the end of one statement to the beginning of the next statement.

### Control Flow

Consider two LLVM IR instructions P and S, where P is a predecessor of S.  If P and S are in the same basic block, then P is the instruction immediately preceding S in the basic block.  If P and S are in different basic blocks, then P must be the last instruction of a basic block and S must be the first instruction of a basic block.

On any given path, the time point `P-post` is essentially the same time point as `S-pre`.  However, if S has multiple predecessors, then it is useful to have separate names, so that we can generate constraints for P and S separately and then tie them together with control-flow constraints.

We generate the following constraints for every C-path `ptr` that already appears in existing constraints:
- `good(ptr, P-post) -> good(ptr, S-pre)` except if P is a conditional jump to S that is conditioned on ptr being null.
- `null(ptr, P-post) -> null(ptr, S-pre)` except if P is a conditional jump to S that is conditioned on ptr being non-null.
- `zombie(ptr, P-post) -> zombie(ptr, S-pre)` except if P is a conditional jump to S that is conditioned on ptr being null.

### Preservation Constraints

State properties of most C-paths are preserved during execution of an instruction.  We identify which C-paths need to be excluded from being preserved:
1. A C-path of a memory location whose contents change during the instruction.
2. A C-path that points to a different memory location after the instruction than before the instruction.
3. A C-path of a pointer whose value doesn't change but becomes a zombie because it is free or copied to another responsible pointer.

Let us define `IsExcluded` as in the following pseudocode:
```
function IsExcluded(string cpath, set<string> changedCPaths, set<string> maybeZombifiedCPaths) {
    // changedCPaths is the set of cpaths whose contents change during the instruction.
    if ((cpath in changedCPaths) or (cpath in maybeZombifiedCPaths)) {
        return true;
    }
    else if (cpath has the form "*parentPath") {
        // If an ancestor path points to a mem loc whose contents changed, then
        // this cpath points to a different mem loc after the instruction than
        // before the instruction.
        return IsExcluded(parentPath, changedCPaths, {});
    }
    else {
        return false;
    }
}
```

For example:
- `IsExcluded("x", {"x"}, {}) == true`   // excluded because it appears in changedPaths.
- `IsExcluded("x", {}, {"x"}) == true`   // excluded because it appears in maybeZombifiedCPaths.
- `IsExcluded("*x", {"x"}, {}) == true`  // excluded because parent path appears in changedCPaths.
- `IsExcluded("*x", {}, {"x"}) == false` // parent path appearing in maybeZombifiedCPaths doesn't cause exclusion.
- `IsExcluded("x", {"*x"}, {}) == false` // child path appearing in changedCPaths doesn't cause exclusion.


For a variable (or more accurately, an LLVM `Value*`), we use the term *live*
in the sense of `https://en.wikipedia.org/wiki/Live-variable_analysis`.
We say that a C-path is *live* if it is rooted at a live variable.

Let `PreservationConstraints(changed, maybeZomb)` consist of the following constraints
for all live C-paths `x` where `!IsExcluded(x, changed, maybeZomb)`:
- `good(x, S-post)  <->  good(x, S-pre)`
- `null(x, S-post)  <->  null(x, S-pre)`
- `zombie(x, S-post) <-> zombie(x, S-pre)`


### Pointer-Copying Constraints

We say that two pointers are *pointer aliases* of each other iff they point to the same memory location.
We say that two l-values are *location aliases* of each other if their addresses are pointer aliases of each other.
E.g., given pointers p1 and p2, if p1 == p2, then p1 is a pointer alias of p2 and `*p1` is a location alias of `*p2`.

We define `location_aliases_of(e)` as follows:
 - If `e` is an LLVM temporary, then `location_aliases_of(e) = {e}`.
 - If `e` is a pointer dereference `*p`, then `location_aliases_of(e)` is the set of location aliases of `*p`, including `*p` itself.  (This is a MAY analysis, not a MUST analysis.)


Let `PtrCopyConstraints(dest, src, S-pre, S-post, S-post-dest)` consist of the following constraints:

- `responsible(dest) -> responsible(src)` # Cannot assign a non-resp ptr value to a resp ptr dest.
- `mut(dest) -> mut(src)` # Cannot assign a non-mut ptr value to a mut ptr dest.
- `responsible(dest) | !zombie(src, S-pre)` # POM constraint: cannot assign zombie value to irresp ptr.

- For each `src_ali` in `location_aliases_of(src)`:
  - `responsible(dest) -> (good(src_ali, S-pre) -> zombie(src_ali, S-post))` # Move semantics: copying a resp ptr from src to dest makes src a zombie.
  - if src_ali is a must alias:
    - `!responsible(dest) -> (good(src_ali, S-pre) -> good(src_ali, S-post))`
  - if src_ali isn't a must alias:
    - `good(src_ali, S-pre) -> good(src_ali, S-post)`
  - `null(src_ali, S-pre) -> null(src_ali, S-post)`
  - `zombie(src_ali, S-pre) -> zombie(src_ali, S-post)`

- For each `dest_ali` in `location_aliases_of(dest)`:
  - `(responsible(dest_ali) && good(src, S-pre)) -> good(dest_ali, S-post-dest)`
  - `null(src, S-pre) -> null(dest_ali, S-post-dest)`
  - `(responsible(dest_ali) && zombie(src, S-pre)) -> zombie(dest_ali, S-post-dest)`

- DeepPtrCopyConstraints(dest, src, S-pre, S-post-dest)
If S-post-dest is omitted, it defaults to S-post.

Let `DeepPtrCopyConstraints(dest, src, S-pre, S-post-dest)` consist of the following constraints:
- `responsible(*src) <-> responsible(*dest)` and ditto for deeper levels of indirection
- `mut(*dest) -> mut(*src)` and ditto for deeper levels of indirection
- For each `dest_ali` in `location_aliases_of(dest)`:
  - `good(*src, S-pre) -> good(*dest_ali, S-post-dest)` and ditto for `null` and `zombie`, and ditto for deeper levels of indirection

Note: Since LLVM has switched to opaque pointers, it is nontrivial to determine the pointer depth of pointers.  We can get this info fairly easily from the debug info for function arguments and return values.  For other LLVM Value objects, we piggyback off seenCPaths to infer the pointer depth.  (TODO: It seems we can also get type info for locals from the debug info.  This will speed up convergence.)


### Statement: `p = alloca ptr ...`

Constraints: 
- `!responsible(p)`
- `responsible(*p) -> zombie(*p, S-post)` if `*p` is a pointer type, and ditto for deeper derefs
- PreservationConstraints({p}, {})

### Statement: `p = call ... @malloc(...)` or `p = call ... @calloc(...)`

Constraints: 
- `responsible(p)`
- `good(p, S-post)`
- `null(p, S-post)`
- `!zombie(p, S-post)`
- `responsible(*p) -> zombie(*p, S-post)` if `*p` is a pointer type and the callee isn't calloc, and ditto for deeper derefs
- PreservationConstraints({p}, {})

### Statement: `call ... @free(p)`

Constraints: 
- `responsible(p)`
- `!zombie(p, S-pre)`
- `good(p, S-pre) -> zombie(p, S-post)`
- `null(p, S-pre) -> null(p, S-post)`
- `!responsible(*p) | !good(*p, S-pre)` (if the type of p is a pointer to a pointer) # prevent memory leak
- PreservationConstraints({p}, {})

### Statement: `t = call ... @userfunc(act_arg_1, ..., act_arg_n)`

Constraints:
- `PtrCopyConstraints(
    dest = userfunc::formal_param_i,
    src = act_arg_i,
    S-pre = S-pre,
    S-post = S-post,
    S-post-dest = "start")` for i in [1, ..., n] if arg[i] is of pointer type
- `!zombie(act_arg, stmtPre)` for args of pointer type  # Don't pass zombies to functions.
- `good(*userfunc::formal_param_i, end) -> good(*act_arg_i, S-post)` for i in [1, ..., n] if arg[i] is of pointer type, and ditto for `null` and `zombie` # Function summary indicates how pointed-to object change during the function.
- If the return value is of pointer type:
    - `responsible(userfunc::return) <-> responsible(t)`
    - `mut(t) -> mut(userfunc::return)` # Cannot assign a non-mut ptr value to a mut ptr dest.
    - `good(userfunc::return, end) -> good(t, S-post)` and ditto for `null` and `zombie`
    - `DeepPtrCopyConstraints(dest=t, src=userfunc::return, S-pre="end", S-post=S-post)`
- PreservationConstraints({`t`, `act_arg_1`, `act_arg_2`, ..., `act_arg_n`}, {})


### Statement: `%t = load typ, ptr %p1`, where `typ` is a non-pointer type

Constraints: 
- `!null(p1, S-pre)`  # Cannot read via a NULL ptr.
- `!zombie(p1, S-pre)`  # Cannot read via a zombie ptr.
- `PreservationConstraints({}, {})`

### Statement: `%t = load ptr, ptr %p1`

Constraints: 
- `!null(p1, S-pre)`  # Cannot read via a NULL ptr.
- `!zombie(p1, S-pre)`  # Cannot read via a zombie ptr.
- `!mut(p1) -> !mut(t)`  # Cannot get mutable access via an immutable ptr.
- `PtrCopyConstraints(dest=t, src=*p1, S-pre, S-post)`
- Let `maybeZombified` be `location_aliases_of(*p1)`
- `PreservationConstraints({t}, maybeZombified)`


### Statement: `store typ t, ptr %p1`, where `typ` is a non-pointer type

Constraints: 
- `mut(p1)`
- `!null(p1, S-pre)`
- `!zombie(p1, S-pre)`
- PreservationConstraints({}, {})

### Statement: `store ptr t, ptr %p1`

Constraints: 
- `mut(p1)`
- `!null(p1, S-pre)`
- `!zombie(p1, S-pre)`
- `responsible(*p1) -> !good(*p1, S-pre)` # to prevent memory leaks
- `PtrCopyConstraints(dest=*p1, src=t, S-pre, S-post)`
- Let `changedCPaths` be `location_must_aliases_of(*p1)`
- `PreservationConstraints(changedCPaths, {t})`


### Statement: `%var = __pom_var_store(%src)`

Constraints:
- `PtrCopyConstraints(dest=var, src=src, S-pre, S-post)` if var is of pointer type
- `PreservationConstraints({var}, {src})`


### Statement: `%t = phi ptr [ %x_1, %L_1 ], ..., [ %x_n, %L_n ]`

Constraints:
- `PtrCopyConstraints(dest=t, src=x_i, S-pre=S-pre, S-post=S-post)` if t is of pointer type
- PreservationConstraints({`t`}, {`x_1`, ..., `x_n`})

### Statement: `return %t`

Constraints (if t is of pointer type):
- `responsible(t) <-> responsible(containing_func::return)` and ditto for deeper indirection
- `good(t, S-pre) -> good(containing_func::return, end)` and ditto for `null` and `zombie` and deeper indirection.
- `mut(containing_func::return) -> mut(t)` and ditto for deeper indirection

### Statement: `%t = getelementptr ...` where it calculates address of element of array

Constraints:
- `!responsible(t)`
- `mut(t) -> mut(base_ptr)`
- `PtrCopyConstraints(dest=t, src=base_ptr, S-pre=S-pre, S-post=S-post)`
- PreservationConstraints({t}, {})

### Statement: `%t = getelementptr ...` where it calculates address of field of struct

Constraints:
- `!responsible(t)`
- `mut(t) -> mut(base_ptr)`
- `responsible(*t) <-> responsible(pomFieldName)` where `pomFieldName` is `struct::$structName::$fieldName`
- `mut(*t) <-> mut(pomFieldName)` where `pomFieldName` is `struct::$structName::$fieldName`
- `PtrCopyConstraints(dest=t, src=base_ptr.fieldname, S-pre=S-pre, S-post=S-post)` // See `borrow_design.txt` for weirdness regarding the ".fieldname" notation that we are using.
- PreservationConstraints({t}, {})


### All other statements (e.g., br, cmp, etc.):

Constraints:
- PreservationConstraints({}, {})


### Dead variables

When a variable x is live immediately before an instruction S but dead immediately after the instruction S (e.g., the instruction S has the last use of x), then generate the following constraint to prohibit memory leaks:
- `responsible(x) -> !good(x, S-pre)`


### All responsible pointers are mutable

For every seen cpath `x`, we generate a constraint `responsible(x) -> mut(x)`.
