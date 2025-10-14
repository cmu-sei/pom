//
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
//


fn foo<'a>(ppp: &&Box<i32>, qqq: &&Box<i32>, mystery1: &mut &'a Box<i32>, mystery2: &&'a Box<i32> ) -> i32 {
    *mystery1 = *mystery2;
    return ***ppp;
}

fn main_good() -> () {
    let p = Box::new(42);
    let q = Box::new(24);
    let mut pp = &p;
    let qq = &q;
    let ppp: &mut &Box<i32> = &mut pp;
    let qqq = &qq;
    #[cfg(feature = "has_bug")]
    {
        foo(ppp, qqq, ppp, qqq);
    }
    #[cfg(not(feature = "has_bug"))]
    {
        foo(ppp, qqq, ppp, ppp); // error[E0502]: cannot borrow `*ppp` as mutable because it is also borrowed as immutable
    }
    drop(p);
}


fn main() {
    main_good();
}
