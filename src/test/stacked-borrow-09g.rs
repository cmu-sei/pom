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


#![allow(unused_mut)]
#![allow(unused_variables)]

use std::fs::File;
use std::io::Read;

fn random_bool() -> bool {
    let mut file = File::open("/dev/urandom").unwrap();
    let mut buf = [0u8; 1];
    file.read_exact(&mut buf).unwrap();
    buf[0] & 1 == 1
}

fn main() {
    let mut x00 : i32 = 0;
    let mut x01 : i32 = 1;
    let mut x02 : i32 = 2;
    let mut x03 : i32 = 3;
    let mut x04 : i32 = 4;
    let mut x05 : i32 = 5;
    let mut x06 : i32 = 6;
    let mut x07 : i32 = 7;
    let mut x08 : i32 = 8;
    let mut x09 : i32 = 9;
    let mut x10 : i32 = 10;
    let mut x11 : i32 = 11;
    let mut x12 : i32 = 12;
    let mut x13 : i32 = 13;
    let mut x14 : i32 = 14;
    let mut x15 : i32 = 15;
    let mut x16 : i32 = 16;
    let mut x17 : i32 = 17;
    let mut x18 : i32 = 18;
    let mut x19 : i32 = 19;
    let mut x20 : i32 = 20;
    let mut x21 : i32 = 21;
    let mut x22 : i32 = 22;
    let mut x23 : i32 = 23;
    let mut x24 : i32 = 24;

    let mut r00 = &mut x00;
    let mut r01 = &mut x01;
    let mut r02 = &mut x02;
    let mut r03 = &mut x03;
    let mut r04 = &mut x04;
    let mut r05 = &mut x05;
    let mut r06 = &mut x06;
    let mut r07 = &mut x07;
    let mut r08 = &mut x08;
    let mut r09 = &mut x09;
    let mut r10 = &mut x10;
    let mut r11 = &mut x11;
    let mut r12 = &mut x12;
    let mut r13 = &mut x13;
    let mut r14 = &mut x14;
    let mut r15 = &mut x15;
    let mut r16 = &mut x16;
    let mut r17 = &mut x17;
    let mut r18 = &mut x18;
    let mut r19 = &mut x19;
    let mut r20 = &mut x20;
    let mut r21 = &mut x21;
    let mut r22 = &mut x22;
    let mut r23 = &mut x23;
    let mut r24 = &mut x24;

    //if random_bool() {std::mem::swap(&mut r00, &mut r01);}
    //if random_bool() {std::mem::swap(&mut r01, &mut r02);}
    if random_bool() {std::mem::swap(&mut r02, &mut r03);}
    if random_bool() {std::mem::swap(&mut r03, &mut r04);}
    if random_bool() {std::mem::swap(&mut r04, &mut r05);}
    if random_bool() {std::mem::swap(&mut r05, &mut r06);}
    if random_bool() {std::mem::swap(&mut r06, &mut r07);}
    if random_bool() {std::mem::swap(&mut r07, &mut r08);}
    if random_bool() {std::mem::swap(&mut r08, &mut r09);}
    if random_bool() {std::mem::swap(&mut r09, &mut r10);}
    if random_bool() {std::mem::swap(&mut r10, &mut r11);}
    if random_bool() {std::mem::swap(&mut r11, &mut r12);}
    if random_bool() {std::mem::swap(&mut r12, &mut r13);}
    if random_bool() {std::mem::swap(&mut r13, &mut r14);}
    if random_bool() {std::mem::swap(&mut r14, &mut r15);}
    if random_bool() {std::mem::swap(&mut r15, &mut r16);}
    if random_bool() {std::mem::swap(&mut r16, &mut r17);}
    if random_bool() {std::mem::swap(&mut r17, &mut r18);}
    if random_bool() {std::mem::swap(&mut r18, &mut r19);}
    if random_bool() {std::mem::swap(&mut r19, &mut r20);}
    if random_bool() {std::mem::swap(&mut r20, &mut r21);}
    if random_bool() {std::mem::swap(&mut r21, &mut r22);}
    //if random_bool() {std::mem::swap(&mut r22, &mut r23);}
    //if random_bool() {std::mem::swap(&mut r23, &mut r24);}
    if random_bool() {r24 = &mut x07;}

    *r00 += 1000;
    *r01 += 2000;
    *r23 += 2000;
    *r24 += 2000;
    print!("*r00={}, *r01={}, *r23={}, *r24={}\n", *r00, *r01, *r23, *r24);
    print!("x00={}, x01={}, x23={}, x24={}\n", x00, x01, x23, x24);
}
