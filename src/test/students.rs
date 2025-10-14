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


use std::env;

// We could use &str everywhere since all strings here are static string literals.

struct Student {
    name: String,
    major: String,
}

fn main() {
    let hello : String = String::from("hello");
    let mut students = vec![
        Student { name: String::from("Alice"),   major: String::from("Anthropology")},
        Student { name: String::from("Bob"),     major: String::from("Business")},
        Student { name: String::from("Charlie"), major: String::from("Computer Science")},
    ];
    let num_students : usize = students.len();
    let mut chosen: &mut Student = &mut students[env::args().len() % num_students];
    // // Question: is `chosen.name` responsible or irresponsible?
    // #ifdef COPY
    chosen.name = hello; // OK in Rust, as hello is a String, not &str
    // #else
    // drop(chosen.name); // cannot move out of `chosen.name` which is behind a mutable reference
    // #endif

    for i in 0..num_students {
        let name = &students[i].name;
        let major = &students[i].major;
        println!("{name}: {major}");
    }

    // No cleanup necessary, done automatically
}
