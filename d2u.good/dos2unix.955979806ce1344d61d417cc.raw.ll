; ModuleID = 'dos2unix.c'
source_filename = "dos2unix.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

%struct.CFlag = type { i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32 }

@stdout = external global ptr, align 8
@.str = private unnamed_addr constant [215 x i8] c"Copyright (C) 2009-%d Erwin Waterlander\0ACopyright (C) 1998      Christian Wurll (Version 3.1)\0ACopyright (C) 1998      Bernd Johannes Wuebben (Version 3.0)\0ACopyright (C) 1994-1995 Benjamin Lin\0AAll rights reserved.\0A\0A\00", align 1, !dbg !0
@stderr = external global ptr, align 8
@.str.1 = private unnamed_addr constant [5 x i8] c"%s: \00", align 1, !dbg !7
@.str.2 = private unnamed_addr constant [41 x i8] c"Binary symbol 0x00%02X found at line %u\0A\00", align 1, !dbg !12
@.str.3 = private unnamed_addr constant [32 x i8] c"Added line break to last line.\0A\00", align 1, !dbg !17
@.str.4 = private unnamed_addr constant [37 x i8] c"Converted %u out of %u line breaks.\0A\00", align 1, !dbg !22
@D2UAsciiTable = internal global [256 x i32] [i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15, i32 16, i32 17, i32 18, i32 19, i32 20, i32 21, i32 22, i32 23, i32 24, i32 25, i32 26, i32 27, i32 28, i32 29, i32 30, i32 31, i32 32, i32 33, i32 34, i32 35, i32 36, i32 37, i32 38, i32 39, i32 40, i32 41, i32 42, i32 43, i32 44, i32 45, i32 46, i32 47, i32 48, i32 49, i32 50, i32 51, i32 52, i32 53, i32 54, i32 55, i32 56, i32 57, i32 58, i32 59, i32 60, i32 61, i32 62, i32 63, i32 64, i32 65, i32 66, i32 67, i32 68, i32 69, i32 70, i32 71, i32 72, i32 73, i32 74, i32 75, i32 76, i32 77, i32 78, i32 79, i32 80, i32 81, i32 82, i32 83, i32 84, i32 85, i32 86, i32 87, i32 88, i32 89, i32 90, i32 91, i32 92, i32 93, i32 94, i32 95, i32 96, i32 97, i32 98, i32 99, i32 100, i32 101, i32 102, i32 103, i32 104, i32 105, i32 106, i32 107, i32 108, i32 109, i32 110, i32 111, i32 112, i32 113, i32 114, i32 115, i32 116, i32 117, i32 118, i32 119, i32 120, i32 121, i32 122, i32 123, i32 124, i32 125, i32 126, i32 127, i32 -128, i32 -127, i32 -126, i32 -125, i32 -124, i32 -123, i32 -122, i32 -121, i32 -120, i32 -119, i32 -118, i32 -117, i32 -116, i32 -115, i32 -114, i32 -113, i32 -112, i32 -111, i32 -110, i32 -109, i32 -108, i32 -107, i32 -106, i32 -105, i32 -104, i32 -103, i32 -102, i32 -101, i32 -100, i32 -99, i32 -98, i32 -97, i32 -96, i32 -95, i32 -94, i32 -93, i32 -92, i32 -91, i32 -90, i32 -89, i32 -88, i32 -87, i32 -86, i32 -85, i32 -84, i32 -83, i32 -82, i32 -81, i32 -80, i32 -79, i32 -78, i32 -77, i32 -76, i32 -75, i32 -74, i32 -73, i32 -72, i32 -71, i32 -70, i32 -69, i32 -68, i32 -67, i32 -66, i32 -65, i32 -64, i32 -63, i32 -62, i32 -61, i32 -60, i32 -59, i32 -58, i32 -57, i32 -56, i32 -55, i32 -54, i32 -53, i32 -52, i32 -51, i32 -50, i32 -49, i32 -48, i32 -47, i32 -46, i32 -45, i32 -44, i32 -43, i32 -42, i32 -41, i32 -40, i32 -39, i32 -38, i32 -37, i32 -36, i32 -35, i32 -34, i32 -33, i32 -32, i32 -31, i32 -30, i32 -29, i32 -28, i32 -27, i32 -26, i32 -25, i32 -24, i32 -23, i32 -22, i32 -21, i32 -20, i32 -19, i32 -18, i32 -17, i32 -16, i32 -15, i32 -14, i32 -13, i32 -12, i32 -11, i32 -10, i32 -9, i32 -8, i32 -7, i32 -6, i32 -5, i32 -4, i32 -3, i32 -2, i32 -1], align 16, !dbg !27
@D2U7BitTable = internal global [256 x i32] [i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15, i32 16, i32 17, i32 18, i32 19, i32 20, i32 21, i32 22, i32 23, i32 24, i32 25, i32 26, i32 27, i32 28, i32 29, i32 30, i32 31, i32 32, i32 33, i32 34, i32 35, i32 36, i32 37, i32 38, i32 39, i32 40, i32 41, i32 42, i32 43, i32 44, i32 45, i32 46, i32 47, i32 48, i32 49, i32 50, i32 51, i32 52, i32 53, i32 54, i32 55, i32 56, i32 57, i32 58, i32 59, i32 60, i32 61, i32 62, i32 63, i32 64, i32 65, i32 66, i32 67, i32 68, i32 69, i32 70, i32 71, i32 72, i32 73, i32 74, i32 75, i32 76, i32 77, i32 78, i32 79, i32 80, i32 81, i32 82, i32 83, i32 84, i32 85, i32 86, i32 87, i32 88, i32 89, i32 90, i32 91, i32 92, i32 93, i32 94, i32 95, i32 96, i32 97, i32 98, i32 99, i32 100, i32 101, i32 102, i32 103, i32 104, i32 105, i32 106, i32 107, i32 108, i32 109, i32 110, i32 111, i32 112, i32 113, i32 114, i32 115, i32 116, i32 117, i32 118, i32 119, i32 120, i32 121, i32 122, i32 123, i32 124, i32 125, i32 126, i32 127, i32 32, i32 32, i32 32, i32 32, i32 32, i32 32, i32 32, i32 32, i32 32, i32 32, i32 32, i32 32, i32 32, i32 32, i32 32, i32 32, i32 32, i32 32, i32 32, i32 32, i32 32, i32 32, i32 32, i32 32, i32 32, i32 32, i32 32, i32 32, i32 32, i32 32, i32 32, i32 32, i32 32, i32 32, i32 32, i32 32, i32 32, i32 32, i32 32, i32 32, i32 32, i32 32, i32 32, i32 32, i32 32, i32 32, i32 32, i32 32, i32 32, i32 32, i32 32, i32 32, i32 32, i32 32, i32 32, i32 32, i32 32, i32 32, i32 32, i32 32, i32 32, i32 32, i32 32, i32 32, i32 32, i32 32, i32 32, i32 32, i32 32, i32 32, i32 32, i32 32, i32 32, i32 32, i32 32, i32 32, i32 32, i32 32, i32 32, i32 32, i32 32, i32 32, i32 32, i32 32, i32 32, i32 32, i32 32, i32 32, i32 32, i32 32, i32 32, i32 32, i32 32, i32 32, i32 32, i32 32, i32 32, i32 32, i32 32, i32 32, i32 32, i32 32, i32 32, i32 32, i32 32, i32 32, i32 32, i32 32, i32 32, i32 32, i32 32, i32 32, i32 32, i32 32, i32 32, i32 32, i32 32, i32 32, i32 32, i32 32, i32 32, i32 32, i32 32, i32 32, i32 32, i32 32, i32 32, i32 32], align 16, !dbg !115
@D2UIso437Table = internal global [256 x i32] [i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15, i32 16, i32 17, i32 18, i32 19, i32 20, i32 21, i32 22, i32 23, i32 24, i32 25, i32 26, i32 27, i32 28, i32 29, i32 30, i32 31, i32 32, i32 33, i32 34, i32 35, i32 36, i32 37, i32 38, i32 39, i32 40, i32 41, i32 42, i32 43, i32 44, i32 45, i32 46, i32 47, i32 48, i32 49, i32 50, i32 51, i32 52, i32 53, i32 54, i32 55, i32 56, i32 57, i32 58, i32 59, i32 60, i32 61, i32 62, i32 63, i32 64, i32 65, i32 66, i32 67, i32 68, i32 69, i32 70, i32 71, i32 72, i32 73, i32 74, i32 75, i32 76, i32 77, i32 78, i32 79, i32 80, i32 81, i32 82, i32 83, i32 84, i32 85, i32 86, i32 87, i32 88, i32 89, i32 90, i32 91, i32 92, i32 93, i32 94, i32 95, i32 96, i32 97, i32 98, i32 99, i32 100, i32 101, i32 102, i32 103, i32 104, i32 105, i32 106, i32 107, i32 108, i32 109, i32 110, i32 111, i32 112, i32 113, i32 114, i32 115, i32 116, i32 117, i32 118, i32 119, i32 120, i32 121, i32 122, i32 123, i32 124, i32 125, i32 126, i32 127, i32 -57, i32 -4, i32 -23, i32 -30, i32 -28, i32 -32, i32 -27, i32 -25, i32 -22, i32 -21, i32 -24, i32 -17, i32 -18, i32 -20, i32 -60, i32 -59, i32 -55, i32 -26, i32 -58, i32 -12, i32 -10, i32 -14, i32 -5, i32 -7, i32 -1, i32 -42, i32 -36, i32 -94, i32 -93, i32 -91, i32 46, i32 46, i32 -31, i32 -19, i32 -13, i32 -6, i32 -15, i32 -47, i32 -86, i32 -70, i32 -65, i32 46, i32 -84, i32 -67, i32 -68, i32 -95, i32 -85, i32 -69, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 -33, i32 46, i32 46, i32 46, i32 46, i32 -75, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 -79, i32 46, i32 46, i32 46, i32 46, i32 -9, i32 46, i32 -80, i32 46, i32 -73, i32 46, i32 46, i32 -78, i32 46, i32 -96], align 16, !dbg !121
@D2UIso850Table = internal global [256 x i32] [i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15, i32 16, i32 17, i32 18, i32 19, i32 20, i32 21, i32 22, i32 23, i32 24, i32 25, i32 26, i32 27, i32 28, i32 29, i32 30, i32 31, i32 32, i32 33, i32 34, i32 35, i32 36, i32 37, i32 38, i32 39, i32 40, i32 41, i32 42, i32 43, i32 44, i32 45, i32 46, i32 47, i32 48, i32 49, i32 50, i32 51, i32 52, i32 53, i32 54, i32 55, i32 56, i32 57, i32 58, i32 59, i32 60, i32 61, i32 62, i32 63, i32 64, i32 65, i32 66, i32 67, i32 68, i32 69, i32 70, i32 71, i32 72, i32 73, i32 74, i32 75, i32 76, i32 77, i32 78, i32 79, i32 80, i32 81, i32 82, i32 83, i32 84, i32 85, i32 86, i32 87, i32 88, i32 89, i32 90, i32 91, i32 92, i32 93, i32 94, i32 95, i32 96, i32 97, i32 98, i32 99, i32 100, i32 101, i32 102, i32 103, i32 104, i32 105, i32 106, i32 107, i32 108, i32 109, i32 110, i32 111, i32 112, i32 113, i32 114, i32 115, i32 116, i32 117, i32 118, i32 119, i32 120, i32 121, i32 122, i32 123, i32 124, i32 125, i32 126, i32 127, i32 -57, i32 -4, i32 -23, i32 -30, i32 -28, i32 -32, i32 -27, i32 -25, i32 -22, i32 -21, i32 -24, i32 -17, i32 -18, i32 -20, i32 -60, i32 -59, i32 -55, i32 -26, i32 -58, i32 -12, i32 -10, i32 -14, i32 -5, i32 -7, i32 -1, i32 -42, i32 -36, i32 -8, i32 -93, i32 -40, i32 -41, i32 46, i32 -31, i32 -19, i32 -13, i32 -6, i32 -15, i32 -47, i32 -86, i32 -70, i32 -65, i32 -82, i32 -84, i32 -67, i32 -68, i32 -95, i32 -85, i32 -69, i32 46, i32 46, i32 46, i32 46, i32 46, i32 -63, i32 -62, i32 -64, i32 -87, i32 46, i32 46, i32 46, i32 46, i32 -94, i32 -91, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 -29, i32 -61, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 -92, i32 -16, i32 -48, i32 -54, i32 -53, i32 -56, i32 46, i32 -51, i32 -50, i32 -49, i32 46, i32 46, i32 46, i32 46, i32 -90, i32 -52, i32 46, i32 -45, i32 -33, i32 -44, i32 -46, i32 -11, i32 -43, i32 -75, i32 -2, i32 -34, i32 -38, i32 -37, i32 -39, i32 -3, i32 -35, i32 -81, i32 -76, i32 -83, i32 -79, i32 46, i32 -66, i32 -74, i32 -89, i32 -9, i32 -72, i32 -80, i32 -88, i32 -73, i32 -71, i32 -77, i32 -78, i32 46, i32 -96], align 16, !dbg !123
@D2UIso860Table = internal global [256 x i32] [i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15, i32 16, i32 17, i32 18, i32 19, i32 20, i32 21, i32 22, i32 23, i32 24, i32 25, i32 26, i32 27, i32 28, i32 29, i32 30, i32 31, i32 32, i32 33, i32 34, i32 35, i32 36, i32 37, i32 38, i32 39, i32 40, i32 41, i32 42, i32 43, i32 44, i32 45, i32 46, i32 47, i32 48, i32 49, i32 50, i32 51, i32 52, i32 53, i32 54, i32 55, i32 56, i32 57, i32 58, i32 59, i32 60, i32 61, i32 62, i32 63, i32 64, i32 65, i32 66, i32 67, i32 68, i32 69, i32 70, i32 71, i32 72, i32 73, i32 74, i32 75, i32 76, i32 77, i32 78, i32 79, i32 80, i32 81, i32 82, i32 83, i32 84, i32 85, i32 86, i32 87, i32 88, i32 89, i32 90, i32 91, i32 92, i32 93, i32 94, i32 95, i32 96, i32 97, i32 98, i32 99, i32 100, i32 101, i32 102, i32 103, i32 104, i32 105, i32 106, i32 107, i32 108, i32 109, i32 110, i32 111, i32 112, i32 113, i32 114, i32 115, i32 116, i32 117, i32 118, i32 119, i32 120, i32 121, i32 122, i32 123, i32 124, i32 125, i32 126, i32 127, i32 -57, i32 -4, i32 -23, i32 -30, i32 -29, i32 -32, i32 -63, i32 -25, i32 -22, i32 -54, i32 -24, i32 -51, i32 -44, i32 -20, i32 -61, i32 -62, i32 -55, i32 -64, i32 -56, i32 -12, i32 -11, i32 -14, i32 -38, i32 -7, i32 -52, i32 -43, i32 -36, i32 -94, i32 -93, i32 -39, i32 46, i32 -45, i32 -31, i32 -19, i32 -13, i32 -6, i32 -15, i32 -47, i32 -86, i32 -70, i32 -65, i32 -46, i32 -84, i32 -67, i32 -68, i32 -95, i32 -85, i32 -69, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 -33, i32 46, i32 46, i32 46, i32 46, i32 -75, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 -79, i32 46, i32 46, i32 46, i32 46, i32 -9, i32 46, i32 -80, i32 46, i32 -73, i32 46, i32 46, i32 -78, i32 46, i32 -96], align 16, !dbg !125
@D2UIso863Table = internal global [256 x i32] [i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15, i32 16, i32 17, i32 18, i32 19, i32 20, i32 21, i32 22, i32 23, i32 24, i32 25, i32 26, i32 27, i32 28, i32 29, i32 30, i32 31, i32 32, i32 33, i32 34, i32 35, i32 36, i32 37, i32 38, i32 39, i32 40, i32 41, i32 42, i32 43, i32 44, i32 45, i32 46, i32 47, i32 48, i32 49, i32 50, i32 51, i32 52, i32 53, i32 54, i32 55, i32 56, i32 57, i32 58, i32 59, i32 60, i32 61, i32 62, i32 63, i32 64, i32 65, i32 66, i32 67, i32 68, i32 69, i32 70, i32 71, i32 72, i32 73, i32 74, i32 75, i32 76, i32 77, i32 78, i32 79, i32 80, i32 81, i32 82, i32 83, i32 84, i32 85, i32 86, i32 87, i32 88, i32 89, i32 90, i32 91, i32 92, i32 93, i32 94, i32 95, i32 96, i32 97, i32 98, i32 99, i32 100, i32 101, i32 102, i32 103, i32 104, i32 105, i32 106, i32 107, i32 108, i32 109, i32 110, i32 111, i32 112, i32 113, i32 114, i32 115, i32 116, i32 117, i32 118, i32 119, i32 120, i32 121, i32 122, i32 123, i32 124, i32 125, i32 126, i32 127, i32 -57, i32 -4, i32 -23, i32 -30, i32 -62, i32 -32, i32 -74, i32 -25, i32 -22, i32 -21, i32 -24, i32 -17, i32 -18, i32 46, i32 -64, i32 -89, i32 -55, i32 -56, i32 -54, i32 -12, i32 -53, i32 -49, i32 -5, i32 -7, i32 -92, i32 -44, i32 -36, i32 -94, i32 -93, i32 -39, i32 -37, i32 46, i32 -90, i32 -76, i32 -13, i32 -6, i32 -88, i32 -72, i32 -77, i32 -81, i32 -50, i32 46, i32 -84, i32 -67, i32 -68, i32 -66, i32 -85, i32 -69, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 -33, i32 46, i32 46, i32 46, i32 46, i32 -75, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 -79, i32 46, i32 46, i32 46, i32 46, i32 -9, i32 46, i32 -80, i32 46, i32 -73, i32 46, i32 46, i32 -78, i32 46, i32 -96], align 16, !dbg !127
@D2UIso865Table = internal global [256 x i32] [i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15, i32 16, i32 17, i32 18, i32 19, i32 20, i32 21, i32 22, i32 23, i32 24, i32 25, i32 26, i32 27, i32 28, i32 29, i32 30, i32 31, i32 32, i32 33, i32 34, i32 35, i32 36, i32 37, i32 38, i32 39, i32 40, i32 41, i32 42, i32 43, i32 44, i32 45, i32 46, i32 47, i32 48, i32 49, i32 50, i32 51, i32 52, i32 53, i32 54, i32 55, i32 56, i32 57, i32 58, i32 59, i32 60, i32 61, i32 62, i32 63, i32 64, i32 65, i32 66, i32 67, i32 68, i32 69, i32 70, i32 71, i32 72, i32 73, i32 74, i32 75, i32 76, i32 77, i32 78, i32 79, i32 80, i32 81, i32 82, i32 83, i32 84, i32 85, i32 86, i32 87, i32 88, i32 89, i32 90, i32 91, i32 92, i32 93, i32 94, i32 95, i32 96, i32 97, i32 98, i32 99, i32 100, i32 101, i32 102, i32 103, i32 104, i32 105, i32 106, i32 107, i32 108, i32 109, i32 110, i32 111, i32 112, i32 113, i32 114, i32 115, i32 116, i32 117, i32 118, i32 119, i32 120, i32 121, i32 122, i32 123, i32 124, i32 125, i32 126, i32 127, i32 -57, i32 -4, i32 -23, i32 -30, i32 -28, i32 -32, i32 -27, i32 -25, i32 -22, i32 -21, i32 -24, i32 -17, i32 -18, i32 -20, i32 -60, i32 -59, i32 -55, i32 -26, i32 -58, i32 -12, i32 -10, i32 -14, i32 -5, i32 -7, i32 -1, i32 -42, i32 -36, i32 -8, i32 -93, i32 -40, i32 46, i32 46, i32 -31, i32 -19, i32 -13, i32 -6, i32 -15, i32 -47, i32 -86, i32 -70, i32 -65, i32 46, i32 -84, i32 -67, i32 -68, i32 -95, i32 -85, i32 -92, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 -33, i32 46, i32 46, i32 46, i32 46, i32 -75, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 -79, i32 46, i32 46, i32 46, i32 46, i32 -9, i32 46, i32 -80, i32 46, i32 -73, i32 46, i32 46, i32 -78, i32 46, i32 -96], align 16, !dbg !129
@D2UIso1252Table = internal global [256 x i32] [i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15, i32 16, i32 17, i32 18, i32 19, i32 20, i32 21, i32 22, i32 23, i32 24, i32 25, i32 26, i32 27, i32 28, i32 29, i32 30, i32 31, i32 32, i32 33, i32 34, i32 35, i32 36, i32 37, i32 38, i32 39, i32 40, i32 41, i32 42, i32 43, i32 44, i32 45, i32 46, i32 47, i32 48, i32 49, i32 50, i32 51, i32 52, i32 53, i32 54, i32 55, i32 56, i32 57, i32 58, i32 59, i32 60, i32 61, i32 62, i32 63, i32 64, i32 65, i32 66, i32 67, i32 68, i32 69, i32 70, i32 71, i32 72, i32 73, i32 74, i32 75, i32 76, i32 77, i32 78, i32 79, i32 80, i32 81, i32 82, i32 83, i32 84, i32 85, i32 86, i32 87, i32 88, i32 89, i32 90, i32 91, i32 92, i32 93, i32 94, i32 95, i32 96, i32 97, i32 98, i32 99, i32 100, i32 101, i32 102, i32 103, i32 104, i32 105, i32 106, i32 107, i32 108, i32 109, i32 110, i32 111, i32 112, i32 113, i32 114, i32 115, i32 116, i32 117, i32 118, i32 119, i32 120, i32 121, i32 122, i32 123, i32 124, i32 125, i32 126, i32 127, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 46, i32 -96, i32 -95, i32 -94, i32 -93, i32 -92, i32 -91, i32 -90, i32 -89, i32 -88, i32 -87, i32 -86, i32 -85, i32 -84, i32 -83, i32 -82, i32 -81, i32 -80, i32 -79, i32 -78, i32 -77, i32 -76, i32 -75, i32 -74, i32 -73, i32 -72, i32 -71, i32 -70, i32 -69, i32 -68, i32 -67, i32 -66, i32 -65, i32 -64, i32 -63, i32 -62, i32 -61, i32 -60, i32 -59, i32 -58, i32 -57, i32 -56, i32 -55, i32 -54, i32 -53, i32 -52, i32 -51, i32 -50, i32 -49, i32 -48, i32 -47, i32 -46, i32 -45, i32 -44, i32 -43, i32 -42, i32 -41, i32 -40, i32 -39, i32 -38, i32 -37, i32 -36, i32 -35, i32 -34, i32 -33, i32 -32, i32 -31, i32 -30, i32 -29, i32 -28, i32 -27, i32 -26, i32 -25, i32 -24, i32 -23, i32 -22, i32 -21, i32 -20, i32 -19, i32 -18, i32 -17, i32 -16, i32 -15, i32 -14, i32 -13, i32 -12, i32 -11, i32 -10, i32 -9, i32 -8, i32 -7, i32 -6, i32 -5, i32 -4, i32 -3, i32 -2, i32 -1], align 16, !dbg !131
@.str.5 = private unnamed_addr constant [21 x i8] c"using code page %d.\0A\00", align 1, !dbg !61
@.str.6 = private unnamed_addr constant [39 x i8] c"Binary symbol 0x%02X found at line %u\0A\00", align 1, !dbg !66
@.str.7 = private unnamed_addr constant [9 x i8] c"dos2unix\00", align 1, !dbg !71
@.str.8 = private unnamed_addr constant [19 x i8] c"DOS2UNIX_LOCALEDIR\00", align 1, !dbg !76
@.str.9 = private unnamed_addr constant [18 x i8] c"/usr/share/locale\00", align 1, !dbg !81
@.str.10 = private unnamed_addr constant [3 x i8] c"%s\00", align 1, !dbg !86
@.str.11 = private unnamed_addr constant [70 x i8] c"error: Value of environment variable DOS2UNIX_LOCALEDIR is too long.\0A\00", align 1, !dbg !91
@.str.12 = private unnamed_addr constant [1 x i8] zeroinitializer, align 1, !dbg !96
@.str.13 = private unnamed_addr constant [10 x i8] c"dos2unix:\00", align 1, !dbg !101
@.str.14 = private unnamed_addr constant [5 x i8] c" %s\0A\00", align 1, !dbg !106
@.str.15 = private unnamed_addr constant [9 x i8] c"mac2unix\00", align 1, !dbg !108
@.str.16 = private unnamed_addr constant [13 x i8] c"mac2unix.exe\00", align 1, !dbg !110

; Function Attrs: noinline nounwind uwtable
define dso_local void @PrintLicense() #0 !dbg !141 {
entry:
  %0 = load ptr, ptr @stdout, align 8, !dbg !145
  %call = call ptr @gettext(ptr noundef @.str) #7, !dbg !146
  %call1 = call i32 (ptr, ptr, ...) @fprintf(ptr noundef %0, ptr noundef %call, i32 noundef 2024), !dbg !147
  call void @PrintBSDLicense(), !dbg !148
  ret void, !dbg !149
}

declare i32 @fprintf(ptr noundef, ptr noundef, ...) #1

; Function Attrs: nounwind
declare ptr @gettext(ptr noundef) #2

declare void @PrintBSDLicense() #1

; Function Attrs: noinline nounwind uwtable
define dso_local i32 @StripDelimiterW(ptr noundef %ipInF, ptr noundef %ipOutF, ptr noundef %ipFlag, i32 noundef %CurChar, ptr noundef %converted, ptr noundef %progname) #0 !dbg !150 {
entry:
  %retval = alloca i32, align 4
  %ipInF.addr = alloca ptr, align 8
  %ipOutF.addr = alloca ptr, align 8
  %ipFlag.addr = alloca ptr, align 8
  %CurChar.addr = alloca i32, align 4
  %converted.addr = alloca ptr, align 8
  %progname.addr = alloca ptr, align 8
  %TempNextChar = alloca i32, align 4
  store ptr %ipInF, ptr %ipInF.addr, align 8
  call void @llvm.dbg.declare(metadata ptr %ipInF.addr, metadata !215, metadata !DIExpression()), !dbg !216
  store ptr %ipOutF, ptr %ipOutF.addr, align 8
  call void @llvm.dbg.declare(metadata ptr %ipOutF.addr, metadata !217, metadata !DIExpression()), !dbg !218
  store ptr %ipFlag, ptr %ipFlag.addr, align 8
  call void @llvm.dbg.declare(metadata ptr %ipFlag.addr, metadata !219, metadata !DIExpression()), !dbg !220
  store i32 %CurChar, ptr %CurChar.addr, align 4
  call void @llvm.dbg.declare(metadata ptr %CurChar.addr, metadata !221, metadata !DIExpression()), !dbg !222
  store ptr %converted, ptr %converted.addr, align 8
  call void @llvm.dbg.declare(metadata ptr %converted.addr, metadata !223, metadata !DIExpression()), !dbg !224
  store ptr %progname, ptr %progname.addr, align 8
  call void @llvm.dbg.declare(metadata ptr %progname.addr, metadata !225, metadata !DIExpression()), !dbg !226
  call void @llvm.dbg.declare(metadata ptr %TempNextChar, metadata !227, metadata !DIExpression()), !dbg !228
  %0 = load ptr, ptr %ipInF.addr, align 8, !dbg !229
  %1 = load ptr, ptr %ipFlag.addr, align 8, !dbg !231
  %bomtype = getelementptr inbounds %struct.CFlag, ptr %1, i32 0, i32 13, !dbg !232
  %2 = load i32, ptr %bomtype, align 4, !dbg !232
  %call = call i32 @d2u_getwc(ptr noundef %0, i32 noundef %2), !dbg !233
  store i32 %call, ptr %TempNextChar, align 4, !dbg !234
  %cmp = icmp ne i32 %call, -1, !dbg !235
  br i1 %cmp, label %if.then, label %if.else18, !dbg !236

if.then:                                          ; preds = %entry
  %3 = load i32, ptr %TempNextChar, align 4, !dbg !237
  %4 = load ptr, ptr %ipInF.addr, align 8, !dbg !240
  %5 = load ptr, ptr %ipFlag.addr, align 8, !dbg !241
  %bomtype1 = getelementptr inbounds %struct.CFlag, ptr %5, i32 0, i32 13, !dbg !242
  %6 = load i32, ptr %bomtype1, align 4, !dbg !242
  %call2 = call i32 @d2u_ungetwc(i32 noundef %3, ptr noundef %4, i32 noundef %6), !dbg !243
  %cmp3 = icmp eq i32 %call2, -1, !dbg !244
  br i1 %cmp3, label %if.then4, label %if.end, !dbg !245

if.then4:                                         ; preds = %if.then
  %7 = load ptr, ptr %ipFlag.addr, align 8, !dbg !246
  %8 = load ptr, ptr %progname.addr, align 8, !dbg !248
  call void @d2u_getc_error(ptr noundef %7, ptr noundef %8), !dbg !249
  store i32 -1, ptr %retval, align 4, !dbg !250
  br label %return, !dbg !250

if.end:                                           ; preds = %if.then
  %9 = load i32, ptr %TempNextChar, align 4, !dbg !251
  %cmp5 = icmp ne i32 %9, 10, !dbg !253
  br i1 %cmp5, label %if.then6, label %if.else, !dbg !254

if.then6:                                         ; preds = %if.end
  %10 = load i32, ptr %CurChar.addr, align 4, !dbg !255
  %11 = load ptr, ptr %ipOutF.addr, align 8, !dbg !258
  %12 = load ptr, ptr %ipFlag.addr, align 8, !dbg !259
  %13 = load ptr, ptr %progname.addr, align 8, !dbg !260
  %call7 = call i32 @d2u_putwc(i32 noundef %10, ptr noundef %11, ptr noundef %12, ptr noundef %13), !dbg !261
  %cmp8 = icmp eq i32 %call7, -1, !dbg !262
  br i1 %cmp8, label %if.then9, label %if.end10, !dbg !263

if.then9:                                         ; preds = %if.then6
  %14 = load ptr, ptr %ipFlag.addr, align 8, !dbg !264
  %15 = load ptr, ptr %progname.addr, align 8, !dbg !266
  call void @d2u_putwc_error(ptr noundef %14, ptr noundef %15), !dbg !267
  store i32 -1, ptr %retval, align 4, !dbg !268
  br label %return, !dbg !268

if.end10:                                         ; preds = %if.then6
  br label %if.end17, !dbg !269

if.else:                                          ; preds = %if.end
  %16 = load ptr, ptr %converted.addr, align 8, !dbg !270
  %17 = load i32, ptr %16, align 4, !dbg !272
  %inc = add i32 %17, 1, !dbg !272
  store i32 %inc, ptr %16, align 4, !dbg !272
  %18 = load ptr, ptr %ipFlag.addr, align 8, !dbg !273
  %NewLine = getelementptr inbounds %struct.CFlag, ptr %18, i32 0, i32 5, !dbg !275
  %19 = load i32, ptr %NewLine, align 4, !dbg !275
  %tobool = icmp ne i32 %19, 0, !dbg !273
  br i1 %tobool, label %if.then11, label %if.end16, !dbg !276

if.then11:                                        ; preds = %if.else
  %20 = load ptr, ptr %ipOutF.addr, align 8, !dbg !277
  %21 = load ptr, ptr %ipFlag.addr, align 8, !dbg !280
  %22 = load ptr, ptr %progname.addr, align 8, !dbg !281
  %call12 = call i32 @d2u_putwc(i32 noundef 10, ptr noundef %20, ptr noundef %21, ptr noundef %22), !dbg !282
  %cmp13 = icmp eq i32 %call12, -1, !dbg !283
  br i1 %cmp13, label %if.then14, label %if.end15, !dbg !284

if.then14:                                        ; preds = %if.then11
  %23 = load ptr, ptr %ipFlag.addr, align 8, !dbg !285
  %24 = load ptr, ptr %progname.addr, align 8, !dbg !287
  call void @d2u_putwc_error(ptr noundef %23, ptr noundef %24), !dbg !288
  store i32 -1, ptr %retval, align 4, !dbg !289
  br label %return, !dbg !289

if.end15:                                         ; preds = %if.then11
  br label %if.end16, !dbg !290

if.end16:                                         ; preds = %if.end15, %if.else
  br label %if.end17

if.end17:                                         ; preds = %if.end16, %if.end10
  br label %if.end30, !dbg !291

if.else18:                                        ; preds = %entry
  %25 = load ptr, ptr %ipInF.addr, align 8, !dbg !292
  %call19 = call i32 @ferror(ptr noundef %25) #7, !dbg !295
  %tobool20 = icmp ne i32 %call19, 0, !dbg !295
  br i1 %tobool20, label %if.then21, label %if.end22, !dbg !296

if.then21:                                        ; preds = %if.else18
  %26 = load ptr, ptr %ipFlag.addr, align 8, !dbg !297
  %27 = load ptr, ptr %progname.addr, align 8, !dbg !299
  call void @d2u_getc_error(ptr noundef %26, ptr noundef %27), !dbg !300
  store i32 -1, ptr %retval, align 4, !dbg !301
  br label %return, !dbg !301

if.end22:                                         ; preds = %if.else18
  %28 = load i32, ptr %CurChar.addr, align 4, !dbg !302
  %cmp23 = icmp eq i32 %28, 13, !dbg !304
  br i1 %cmp23, label %if.then24, label %if.end29, !dbg !305

if.then24:                                        ; preds = %if.end22
  %29 = load i32, ptr %CurChar.addr, align 4, !dbg !306
  %30 = load ptr, ptr %ipOutF.addr, align 8, !dbg !309
  %31 = load ptr, ptr %ipFlag.addr, align 8, !dbg !310
  %32 = load ptr, ptr %progname.addr, align 8, !dbg !311
  %call25 = call i32 @d2u_putwc(i32 noundef %29, ptr noundef %30, ptr noundef %31, ptr noundef %32), !dbg !312
  %cmp26 = icmp eq i32 %call25, -1, !dbg !313
  br i1 %cmp26, label %if.then27, label %if.end28, !dbg !314

if.then27:                                        ; preds = %if.then24
  %33 = load ptr, ptr %ipFlag.addr, align 8, !dbg !315
  %34 = load ptr, ptr %progname.addr, align 8, !dbg !317
  call void @d2u_putwc_error(ptr noundef %33, ptr noundef %34), !dbg !318
  store i32 -1, ptr %retval, align 4, !dbg !319
  br label %return, !dbg !319

if.end28:                                         ; preds = %if.then24
  br label %if.end29, !dbg !320

if.end29:                                         ; preds = %if.end28, %if.end22
  br label %if.end30

if.end30:                                         ; preds = %if.end29, %if.end17
  %35 = load i32, ptr %CurChar.addr, align 4, !dbg !321
  store i32 %35, ptr %retval, align 4, !dbg !322
  br label %return, !dbg !322

return:                                           ; preds = %if.end30, %if.then27, %if.then21, %if.then14, %if.then9, %if.then4
  %36 = load i32, ptr %retval, align 4, !dbg !323
  ret i32 %36, !dbg !323
}

; Function Attrs: nocallback nofree nosync nounwind readnone speculatable willreturn
declare void @llvm.dbg.declare(metadata, metadata, metadata) #3

declare i32 @d2u_getwc(ptr noundef, i32 noundef) #1

declare i32 @d2u_ungetwc(i32 noundef, ptr noundef, i32 noundef) #1

declare void @d2u_getc_error(ptr noundef, ptr noundef) #1

declare i32 @d2u_putwc(i32 noundef, ptr noundef, ptr noundef, ptr noundef) #1

declare void @d2u_putwc_error(ptr noundef, ptr noundef) #1

; Function Attrs: nounwind
declare i32 @ferror(ptr noundef) #2

; Function Attrs: noinline nounwind uwtable
define dso_local i32 @StripDelimiter(ptr noundef %ipInF, ptr noundef %ipOutF, ptr noundef %ipFlag, i32 noundef %CurChar, ptr noundef %converted, ptr noundef %progname) #0 !dbg !324 {
entry:
  %retval = alloca i32, align 4
  %ipInF.addr = alloca ptr, align 8
  %ipOutF.addr = alloca ptr, align 8
  %ipFlag.addr = alloca ptr, align 8
  %CurChar.addr = alloca i32, align 4
  %converted.addr = alloca ptr, align 8
  %progname.addr = alloca ptr, align 8
  %TempNextChar = alloca i32, align 4
  store ptr %ipInF, ptr %ipInF.addr, align 8
  call void @llvm.dbg.declare(metadata ptr %ipInF.addr, metadata !327, metadata !DIExpression()), !dbg !328
  store ptr %ipOutF, ptr %ipOutF.addr, align 8
  call void @llvm.dbg.declare(metadata ptr %ipOutF.addr, metadata !329, metadata !DIExpression()), !dbg !330
  store ptr %ipFlag, ptr %ipFlag.addr, align 8
  call void @llvm.dbg.declare(metadata ptr %ipFlag.addr, metadata !331, metadata !DIExpression()), !dbg !332
  store i32 %CurChar, ptr %CurChar.addr, align 4
  call void @llvm.dbg.declare(metadata ptr %CurChar.addr, metadata !333, metadata !DIExpression()), !dbg !334
  store ptr %converted, ptr %converted.addr, align 8
  call void @llvm.dbg.declare(metadata ptr %converted.addr, metadata !335, metadata !DIExpression()), !dbg !336
  store ptr %progname, ptr %progname.addr, align 8
  call void @llvm.dbg.declare(metadata ptr %progname.addr, metadata !337, metadata !DIExpression()), !dbg !338
  call void @llvm.dbg.declare(metadata ptr %TempNextChar, metadata !339, metadata !DIExpression()), !dbg !340
  %0 = load ptr, ptr %ipInF.addr, align 8, !dbg !341
  %call = call i32 @fgetc(ptr noundef %0), !dbg !343
  store i32 %call, ptr %TempNextChar, align 4, !dbg !344
  %cmp = icmp ne i32 %call, -1, !dbg !345
  br i1 %cmp, label %if.then, label %if.else17, !dbg !346

if.then:                                          ; preds = %entry
  %1 = load i32, ptr %TempNextChar, align 4, !dbg !347
  %2 = load ptr, ptr %ipInF.addr, align 8, !dbg !350
  %call1 = call i32 @ungetc(i32 noundef %1, ptr noundef %2), !dbg !351
  %cmp2 = icmp eq i32 %call1, -1, !dbg !352
  br i1 %cmp2, label %if.then3, label %if.end, !dbg !353

if.then3:                                         ; preds = %if.then
  %3 = load ptr, ptr %ipFlag.addr, align 8, !dbg !354
  %4 = load ptr, ptr %progname.addr, align 8, !dbg !356
  call void @d2u_getc_error(ptr noundef %3, ptr noundef %4), !dbg !357
  store i32 -1, ptr %retval, align 4, !dbg !358
  br label %return, !dbg !358

if.end:                                           ; preds = %if.then
  %5 = load i32, ptr %TempNextChar, align 4, !dbg !359
  %cmp4 = icmp ne i32 %5, 10, !dbg !361
  br i1 %cmp4, label %if.then5, label %if.else, !dbg !362

if.then5:                                         ; preds = %if.end
  %6 = load i32, ptr %CurChar.addr, align 4, !dbg !363
  %7 = load ptr, ptr %ipOutF.addr, align 8, !dbg !366
  %call6 = call i32 @fputc(i32 noundef %6, ptr noundef %7), !dbg !367
  %cmp7 = icmp eq i32 %call6, -1, !dbg !368
  br i1 %cmp7, label %if.then8, label %if.end9, !dbg !369

if.then8:                                         ; preds = %if.then5
  %8 = load ptr, ptr %ipFlag.addr, align 8, !dbg !370
  %9 = load ptr, ptr %progname.addr, align 8, !dbg !372
  call void @d2u_putc_error(ptr noundef %8, ptr noundef %9), !dbg !373
  store i32 -1, ptr %retval, align 4, !dbg !374
  br label %return, !dbg !374

if.end9:                                          ; preds = %if.then5
  br label %if.end16, !dbg !375

if.else:                                          ; preds = %if.end
  %10 = load ptr, ptr %converted.addr, align 8, !dbg !376
  %11 = load i32, ptr %10, align 4, !dbg !378
  %inc = add i32 %11, 1, !dbg !378
  store i32 %inc, ptr %10, align 4, !dbg !378
  %12 = load ptr, ptr %ipFlag.addr, align 8, !dbg !379
  %NewLine = getelementptr inbounds %struct.CFlag, ptr %12, i32 0, i32 5, !dbg !381
  %13 = load i32, ptr %NewLine, align 4, !dbg !381
  %tobool = icmp ne i32 %13, 0, !dbg !379
  br i1 %tobool, label %if.then10, label %if.end15, !dbg !382

if.then10:                                        ; preds = %if.else
  %14 = load ptr, ptr %ipOutF.addr, align 8, !dbg !383
  %call11 = call i32 @fputc(i32 noundef 10, ptr noundef %14), !dbg !386
  %cmp12 = icmp eq i32 %call11, -1, !dbg !387
  br i1 %cmp12, label %if.then13, label %if.end14, !dbg !388

if.then13:                                        ; preds = %if.then10
  %15 = load ptr, ptr %ipFlag.addr, align 8, !dbg !389
  %16 = load ptr, ptr %progname.addr, align 8, !dbg !391
  call void @d2u_putc_error(ptr noundef %15, ptr noundef %16), !dbg !392
  store i32 -1, ptr %retval, align 4, !dbg !393
  br label %return, !dbg !393

if.end14:                                         ; preds = %if.then10
  br label %if.end15, !dbg !394

if.end15:                                         ; preds = %if.end14, %if.else
  br label %if.end16

if.end16:                                         ; preds = %if.end15, %if.end9
  br label %if.end29, !dbg !395

if.else17:                                        ; preds = %entry
  %17 = load ptr, ptr %ipInF.addr, align 8, !dbg !396
  %call18 = call i32 @ferror(ptr noundef %17) #7, !dbg !399
  %tobool19 = icmp ne i32 %call18, 0, !dbg !399
  br i1 %tobool19, label %if.then20, label %if.end21, !dbg !400

if.then20:                                        ; preds = %if.else17
  %18 = load ptr, ptr %ipFlag.addr, align 8, !dbg !401
  %19 = load ptr, ptr %progname.addr, align 8, !dbg !403
  call void @d2u_getc_error(ptr noundef %18, ptr noundef %19), !dbg !404
  store i32 -1, ptr %retval, align 4, !dbg !405
  br label %return, !dbg !405

if.end21:                                         ; preds = %if.else17
  %20 = load i32, ptr %CurChar.addr, align 4, !dbg !406
  %cmp22 = icmp eq i32 %20, 13, !dbg !408
  br i1 %cmp22, label %if.then23, label %if.end28, !dbg !409

if.then23:                                        ; preds = %if.end21
  %21 = load i32, ptr %CurChar.addr, align 4, !dbg !410
  %22 = load ptr, ptr %ipOutF.addr, align 8, !dbg !413
  %call24 = call i32 @fputc(i32 noundef %21, ptr noundef %22), !dbg !414
  %cmp25 = icmp eq i32 %call24, -1, !dbg !415
  br i1 %cmp25, label %if.then26, label %if.end27, !dbg !416

if.then26:                                        ; preds = %if.then23
  %23 = load ptr, ptr %ipFlag.addr, align 8, !dbg !417
  %24 = load ptr, ptr %progname.addr, align 8, !dbg !419
  call void @d2u_putc_error(ptr noundef %23, ptr noundef %24), !dbg !420
  store i32 -1, ptr %retval, align 4, !dbg !421
  br label %return, !dbg !421

if.end27:                                         ; preds = %if.then23
  br label %if.end28, !dbg !422

if.end28:                                         ; preds = %if.end27, %if.end21
  br label %if.end29

if.end29:                                         ; preds = %if.end28, %if.end16
  %25 = load i32, ptr %CurChar.addr, align 4, !dbg !423
  store i32 %25, ptr %retval, align 4, !dbg !424
  br label %return, !dbg !424

return:                                           ; preds = %if.end29, %if.then26, %if.then20, %if.then13, %if.then8, %if.then3
  %26 = load i32, ptr %retval, align 4, !dbg !425
  ret i32 %26, !dbg !425
}

declare i32 @fgetc(ptr noundef) #1

declare i32 @ungetc(i32 noundef, ptr noundef) #1

declare i32 @fputc(i32 noundef, ptr noundef) #1

declare void @d2u_putc_error(ptr noundef, ptr noundef) #1

; Function Attrs: noinline nounwind uwtable
define dso_local i32 @ConvertDosToUnixW(ptr noundef %ipInF, ptr noundef %ipOutF, ptr noundef %ipFlag, ptr noundef %progname) #0 !dbg !426 {
entry:
  %ipInF.addr = alloca ptr, align 8
  %ipOutF.addr = alloca ptr, align 8
  %ipFlag.addr = alloca ptr, align 8
  %progname.addr = alloca ptr, align 8
  %RetVal = alloca i32, align 4
  %PrevChar = alloca i32, align 4
  %TempChar = alloca i32, align 4
  %TempNextChar = alloca i32, align 4
  %line_nr = alloca i32, align 4
  %converted = alloca i32, align 4
  store ptr %ipInF, ptr %ipInF.addr, align 8
  call void @llvm.dbg.declare(metadata ptr %ipInF.addr, metadata !429, metadata !DIExpression()), !dbg !430
  store ptr %ipOutF, ptr %ipOutF.addr, align 8
  call void @llvm.dbg.declare(metadata ptr %ipOutF.addr, metadata !431, metadata !DIExpression()), !dbg !432
  store ptr %ipFlag, ptr %ipFlag.addr, align 8
  call void @llvm.dbg.declare(metadata ptr %ipFlag.addr, metadata !433, metadata !DIExpression()), !dbg !434
  store ptr %progname, ptr %progname.addr, align 8
  call void @llvm.dbg.declare(metadata ptr %progname.addr, metadata !435, metadata !DIExpression()), !dbg !436
  call void @llvm.dbg.declare(metadata ptr %RetVal, metadata !437, metadata !DIExpression()), !dbg !438
  store i32 0, ptr %RetVal, align 4, !dbg !438
  call void @llvm.dbg.declare(metadata ptr %PrevChar, metadata !439, metadata !DIExpression()), !dbg !440
  store i32 -1, ptr %PrevChar, align 4, !dbg !440
  call void @llvm.dbg.declare(metadata ptr %TempChar, metadata !441, metadata !DIExpression()), !dbg !442
  call void @llvm.dbg.declare(metadata ptr %TempNextChar, metadata !443, metadata !DIExpression()), !dbg !444
  call void @llvm.dbg.declare(metadata ptr %line_nr, metadata !445, metadata !DIExpression()), !dbg !446
  store i32 1, ptr %line_nr, align 4, !dbg !446
  call void @llvm.dbg.declare(metadata ptr %converted, metadata !447, metadata !DIExpression()), !dbg !448
  store i32 0, ptr %converted, align 4, !dbg !448
  %0 = load ptr, ptr %ipFlag.addr, align 8, !dbg !449
  %status = getelementptr inbounds %struct.CFlag, ptr %0, i32 0, i32 9, !dbg !450
  store i32 0, ptr %status, align 4, !dbg !451
  %1 = load ptr, ptr %ipFlag.addr, align 8, !dbg !452
  %FromToMode = getelementptr inbounds %struct.CFlag, ptr %1, i32 0, i32 4, !dbg !453
  %2 = load i32, ptr %FromToMode, align 4, !dbg !453
  switch i32 %2, label %sw.default [
    i32 0, label %sw.bb
    i32 1, label %sw.bb63
  ], !dbg !454

sw.bb:                                            ; preds = %entry
  br label %while.cond, !dbg !455

while.cond:                                       ; preds = %if.end36, %sw.bb
  %3 = load ptr, ptr %ipInF.addr, align 8, !dbg !457
  %4 = load ptr, ptr %ipFlag.addr, align 8, !dbg !458
  %bomtype = getelementptr inbounds %struct.CFlag, ptr %4, i32 0, i32 13, !dbg !459
  %5 = load i32, ptr %bomtype, align 4, !dbg !459
  %call = call i32 @d2u_getwc(ptr noundef %3, i32 noundef %5), !dbg !460
  store i32 %call, ptr %TempChar, align 4, !dbg !461
  %cmp = icmp ne i32 %call, -1, !dbg !462
  br i1 %cmp, label %while.body, label %while.end, !dbg !455

while.body:                                       ; preds = %while.cond
  %6 = load ptr, ptr %ipFlag.addr, align 8, !dbg !463
  %Force = getelementptr inbounds %struct.CFlag, ptr %6, i32 0, i32 6, !dbg !466
  %7 = load i32, ptr %Force, align 4, !dbg !466
  %cmp1 = icmp eq i32 %7, 0, !dbg !467
  br i1 %cmp1, label %land.lhs.true, label %if.end22, !dbg !468

land.lhs.true:                                    ; preds = %while.body
  %8 = load i32, ptr %TempChar, align 4, !dbg !469
  %cmp2 = icmp ult i32 %8, 32, !dbg !470
  br i1 %cmp2, label %land.lhs.true3, label %if.end22, !dbg !471

land.lhs.true3:                                   ; preds = %land.lhs.true
  %9 = load i32, ptr %TempChar, align 4, !dbg !472
  %cmp4 = icmp ne i32 %9, 10, !dbg !473
  br i1 %cmp4, label %land.lhs.true5, label %if.end22, !dbg !474

land.lhs.true5:                                   ; preds = %land.lhs.true3
  %10 = load i32, ptr %TempChar, align 4, !dbg !475
  %cmp6 = icmp ne i32 %10, 13, !dbg !476
  br i1 %cmp6, label %land.lhs.true7, label %if.end22, !dbg !477

land.lhs.true7:                                   ; preds = %land.lhs.true5
  %11 = load i32, ptr %TempChar, align 4, !dbg !478
  %cmp8 = icmp ne i32 %11, 9, !dbg !479
  br i1 %cmp8, label %land.lhs.true9, label %if.end22, !dbg !480

land.lhs.true9:                                   ; preds = %land.lhs.true7
  %12 = load i32, ptr %TempChar, align 4, !dbg !481
  %cmp10 = icmp ne i32 %12, 12, !dbg !482
  br i1 %cmp10, label %if.then, label %if.end22, !dbg !483

if.then:                                          ; preds = %land.lhs.true9
  store i32 -1, ptr %RetVal, align 4, !dbg !484
  %13 = load ptr, ptr %ipFlag.addr, align 8, !dbg !486
  %status11 = getelementptr inbounds %struct.CFlag, ptr %13, i32 0, i32 9, !dbg !487
  %14 = load i32, ptr %status11, align 4, !dbg !488
  %or = or i32 %14, 1, !dbg !488
  store i32 %or, ptr %status11, align 4, !dbg !488
  %15 = load ptr, ptr %ipFlag.addr, align 8, !dbg !489
  %verbose = getelementptr inbounds %struct.CFlag, ptr %15, i32 0, i32 1, !dbg !491
  %16 = load i32, ptr %verbose, align 4, !dbg !491
  %tobool = icmp ne i32 %16, 0, !dbg !489
  br i1 %tobool, label %if.then12, label %if.end21, !dbg !492

if.then12:                                        ; preds = %if.then
  %17 = load ptr, ptr %ipFlag.addr, align 8, !dbg !493
  %stdio_mode = getelementptr inbounds %struct.CFlag, ptr %17, i32 0, i32 10, !dbg !496
  %18 = load i32, ptr %stdio_mode, align 4, !dbg !496
  %tobool13 = icmp ne i32 %18, 0, !dbg !497
  br i1 %tobool13, label %land.lhs.true14, label %if.end, !dbg !498

land.lhs.true14:                                  ; preds = %if.then12
  %19 = load ptr, ptr %ipFlag.addr, align 8, !dbg !499
  %error = getelementptr inbounds %struct.CFlag, ptr %19, i32 0, i32 12, !dbg !500
  %20 = load i32, ptr %error, align 4, !dbg !500
  %tobool15 = icmp ne i32 %20, 0, !dbg !499
  br i1 %tobool15, label %if.end, label %if.then16, !dbg !501

if.then16:                                        ; preds = %land.lhs.true14
  %21 = load ptr, ptr %ipFlag.addr, align 8, !dbg !502
  %error17 = getelementptr inbounds %struct.CFlag, ptr %21, i32 0, i32 12, !dbg !503
  store i32 1, ptr %error17, align 4, !dbg !504
  br label %if.end, !dbg !502

if.end:                                           ; preds = %if.then16, %land.lhs.true14, %if.then12
  %22 = load ptr, ptr @stderr, align 8, !dbg !505
  %23 = load ptr, ptr %progname.addr, align 8, !dbg !506
  %call18 = call i32 (ptr, ptr, ...) @fprintf(ptr noundef %22, ptr noundef @.str.1, ptr noundef %23), !dbg !507
  %24 = load ptr, ptr @stderr, align 8, !dbg !508
  %call19 = call ptr @gettext(ptr noundef @.str.2) #7, !dbg !509
  %25 = load i32, ptr %TempChar, align 4, !dbg !510
  %26 = load i32, ptr %line_nr, align 4, !dbg !511
  %call20 = call i32 (ptr, ptr, ...) @fprintf(ptr noundef %24, ptr noundef %call19, i32 noundef %25, i32 noundef %26), !dbg !512
  br label %if.end21, !dbg !513

if.end21:                                         ; preds = %if.end, %if.then
  br label %while.end, !dbg !514

if.end22:                                         ; preds = %land.lhs.true9, %land.lhs.true7, %land.lhs.true5, %land.lhs.true3, %land.lhs.true, %while.body
  %27 = load i32, ptr %TempChar, align 4, !dbg !515
  %cmp23 = icmp ne i32 %27, 13, !dbg !517
  br i1 %cmp23, label %if.then24, label %if.else, !dbg !518

if.then24:                                        ; preds = %if.end22
  %28 = load i32, ptr %TempChar, align 4, !dbg !519
  %cmp25 = icmp eq i32 %28, 10, !dbg !522
  br i1 %cmp25, label %if.then26, label %if.end27, !dbg !523

if.then26:                                        ; preds = %if.then24
  %29 = load i32, ptr %line_nr, align 4, !dbg !524
  %inc = add i32 %29, 1, !dbg !524
  store i32 %inc, ptr %line_nr, align 4, !dbg !524
  br label %if.end27, !dbg !524

if.end27:                                         ; preds = %if.then26, %if.then24
  %30 = load i32, ptr %TempChar, align 4, !dbg !525
  %31 = load ptr, ptr %ipOutF.addr, align 8, !dbg !527
  %32 = load ptr, ptr %ipFlag.addr, align 8, !dbg !528
  %33 = load ptr, ptr %progname.addr, align 8, !dbg !529
  %call28 = call i32 @d2u_putwc(i32 noundef %30, ptr noundef %31, ptr noundef %32, ptr noundef %33), !dbg !530
  %cmp29 = icmp eq i32 %call28, -1, !dbg !531
  br i1 %cmp29, label %if.then30, label %if.end31, !dbg !532

if.then30:                                        ; preds = %if.end27
  store i32 -1, ptr %RetVal, align 4, !dbg !533
  %34 = load ptr, ptr %ipFlag.addr, align 8, !dbg !535
  %35 = load ptr, ptr %progname.addr, align 8, !dbg !536
  call void @d2u_putwc_error(ptr noundef %34, ptr noundef %35), !dbg !537
  br label %while.end, !dbg !538

if.end31:                                         ; preds = %if.end27
  br label %if.end36, !dbg !539

if.else:                                          ; preds = %if.end22
  %36 = load ptr, ptr %ipInF.addr, align 8, !dbg !540
  %37 = load ptr, ptr %ipOutF.addr, align 8, !dbg !543
  %38 = load ptr, ptr %ipFlag.addr, align 8, !dbg !544
  %39 = load i32, ptr %TempChar, align 4, !dbg !545
  %40 = load ptr, ptr %progname.addr, align 8, !dbg !546
  %call32 = call i32 @StripDelimiterW(ptr noundef %36, ptr noundef %37, ptr noundef %38, i32 noundef %39, ptr noundef %converted, ptr noundef %40), !dbg !547
  %cmp33 = icmp eq i32 %call32, -1, !dbg !548
  br i1 %cmp33, label %if.then34, label %if.end35, !dbg !549

if.then34:                                        ; preds = %if.else
  store i32 -1, ptr %RetVal, align 4, !dbg !550
  br label %while.end, !dbg !552

if.end35:                                         ; preds = %if.else
  br label %if.end36

if.end36:                                         ; preds = %if.end35, %if.end31
  %41 = load i32, ptr %TempChar, align 4, !dbg !553
  store i32 %41, ptr %PrevChar, align 4, !dbg !554
  br label %while.cond, !dbg !455, !llvm.loop !555

while.end:                                        ; preds = %if.then34, %if.then30, %if.end21, %while.cond
  %42 = load i32, ptr %TempChar, align 4, !dbg !558
  %cmp37 = icmp eq i32 %42, -1, !dbg !560
  br i1 %cmp37, label %land.lhs.true38, label %if.end56, !dbg !561

land.lhs.true38:                                  ; preds = %while.end
  %43 = load ptr, ptr %ipFlag.addr, align 8, !dbg !562
  %add_eol = getelementptr inbounds %struct.CFlag, ptr %43, i32 0, i32 20, !dbg !563
  %44 = load i32, ptr %add_eol, align 4, !dbg !563
  %tobool39 = icmp ne i32 %44, 0, !dbg !562
  br i1 %tobool39, label %land.lhs.true40, label %if.end56, !dbg !564

land.lhs.true40:                                  ; preds = %land.lhs.true38
  %45 = load i32, ptr %PrevChar, align 4, !dbg !565
  %cmp41 = icmp ne i32 %45, -1, !dbg !566
  br i1 %cmp41, label %land.lhs.true42, label %if.end56, !dbg !567

land.lhs.true42:                                  ; preds = %land.lhs.true40
  %46 = load i32, ptr %PrevChar, align 4, !dbg !568
  %cmp43 = icmp ne i32 %46, 10, !dbg !569
  br i1 %cmp43, label %if.then44, label %if.end56, !dbg !570

if.then44:                                        ; preds = %land.lhs.true42
  %47 = load ptr, ptr %ipFlag.addr, align 8, !dbg !571
  %verbose45 = getelementptr inbounds %struct.CFlag, ptr %47, i32 0, i32 1, !dbg !574
  %48 = load i32, ptr %verbose45, align 4, !dbg !574
  %cmp46 = icmp sgt i32 %48, 1, !dbg !575
  br i1 %cmp46, label %if.then47, label %if.end51, !dbg !576

if.then47:                                        ; preds = %if.then44
  %49 = load ptr, ptr @stderr, align 8, !dbg !577
  %50 = load ptr, ptr %progname.addr, align 8, !dbg !579
  %call48 = call i32 (ptr, ptr, ...) @fprintf(ptr noundef %49, ptr noundef @.str.1, ptr noundef %50), !dbg !580
  %51 = load ptr, ptr @stderr, align 8, !dbg !581
  %call49 = call ptr @gettext(ptr noundef @.str.3) #7, !dbg !582
  %call50 = call i32 (ptr, ptr, ...) @fprintf(ptr noundef %51, ptr noundef %call49), !dbg !583
  br label %if.end51, !dbg !584

if.end51:                                         ; preds = %if.then47, %if.then44
  %52 = load ptr, ptr %ipOutF.addr, align 8, !dbg !585
  %53 = load ptr, ptr %ipFlag.addr, align 8, !dbg !587
  %54 = load ptr, ptr %progname.addr, align 8, !dbg !588
  %call52 = call i32 @d2u_putwc(i32 noundef 10, ptr noundef %52, ptr noundef %53, ptr noundef %54), !dbg !589
  %cmp53 = icmp eq i32 %call52, -1, !dbg !590
  br i1 %cmp53, label %if.then54, label %if.end55, !dbg !591

if.then54:                                        ; preds = %if.end51
  store i32 -1, ptr %RetVal, align 4, !dbg !592
  %55 = load ptr, ptr %ipFlag.addr, align 8, !dbg !594
  %56 = load ptr, ptr %progname.addr, align 8, !dbg !595
  call void @d2u_putwc_error(ptr noundef %55, ptr noundef %56), !dbg !596
  br label %if.end55, !dbg !597

if.end55:                                         ; preds = %if.then54, %if.end51
  br label %if.end56, !dbg !598

if.end56:                                         ; preds = %if.end55, %land.lhs.true42, %land.lhs.true40, %land.lhs.true38, %while.end
  %57 = load i32, ptr %TempChar, align 4, !dbg !599
  %cmp57 = icmp eq i32 %57, -1, !dbg !601
  br i1 %cmp57, label %land.lhs.true58, label %if.end62, !dbg !602

land.lhs.true58:                                  ; preds = %if.end56
  %58 = load ptr, ptr %ipInF.addr, align 8, !dbg !603
  %call59 = call i32 @ferror(ptr noundef %58) #7, !dbg !604
  %tobool60 = icmp ne i32 %call59, 0, !dbg !604
  br i1 %tobool60, label %if.then61, label %if.end62, !dbg !605

if.then61:                                        ; preds = %land.lhs.true58
  store i32 -1, ptr %RetVal, align 4, !dbg !606
  %59 = load ptr, ptr %ipFlag.addr, align 8, !dbg !608
  %60 = load ptr, ptr %progname.addr, align 8, !dbg !609
  call void @d2u_getc_error(ptr noundef %59, ptr noundef %60), !dbg !610
  br label %if.end62, !dbg !611

if.end62:                                         ; preds = %if.then61, %land.lhs.true58, %if.end56
  br label %sw.epilog, !dbg !612

sw.bb63:                                          ; preds = %entry
  br label %while.cond64, !dbg !613

while.cond64:                                     ; preds = %if.end141, %if.end125, %sw.bb63
  %61 = load ptr, ptr %ipInF.addr, align 8, !dbg !614
  %62 = load ptr, ptr %ipFlag.addr, align 8, !dbg !615
  %bomtype65 = getelementptr inbounds %struct.CFlag, ptr %62, i32 0, i32 13, !dbg !616
  %63 = load i32, ptr %bomtype65, align 4, !dbg !616
  %call66 = call i32 @d2u_getwc(ptr noundef %61, i32 noundef %63), !dbg !617
  store i32 %call66, ptr %TempChar, align 4, !dbg !618
  %cmp67 = icmp ne i32 %call66, -1, !dbg !619
  br i1 %cmp67, label %while.body68, label %while.end142, !dbg !613

while.body68:                                     ; preds = %while.cond64
  %64 = load ptr, ptr %ipFlag.addr, align 8, !dbg !620
  %Force69 = getelementptr inbounds %struct.CFlag, ptr %64, i32 0, i32 6, !dbg !623
  %65 = load i32, ptr %Force69, align 4, !dbg !623
  %cmp70 = icmp eq i32 %65, 0, !dbg !624
  br i1 %cmp70, label %land.lhs.true71, label %if.end99, !dbg !625

land.lhs.true71:                                  ; preds = %while.body68
  %66 = load i32, ptr %TempChar, align 4, !dbg !626
  %cmp72 = icmp ult i32 %66, 32, !dbg !627
  br i1 %cmp72, label %land.lhs.true73, label %if.end99, !dbg !628

land.lhs.true73:                                  ; preds = %land.lhs.true71
  %67 = load i32, ptr %TempChar, align 4, !dbg !629
  %cmp74 = icmp ne i32 %67, 10, !dbg !630
  br i1 %cmp74, label %land.lhs.true75, label %if.end99, !dbg !631

land.lhs.true75:                                  ; preds = %land.lhs.true73
  %68 = load i32, ptr %TempChar, align 4, !dbg !632
  %cmp76 = icmp ne i32 %68, 13, !dbg !633
  br i1 %cmp76, label %land.lhs.true77, label %if.end99, !dbg !634

land.lhs.true77:                                  ; preds = %land.lhs.true75
  %69 = load i32, ptr %TempChar, align 4, !dbg !635
  %cmp78 = icmp ne i32 %69, 9, !dbg !636
  br i1 %cmp78, label %land.lhs.true79, label %if.end99, !dbg !637

land.lhs.true79:                                  ; preds = %land.lhs.true77
  %70 = load i32, ptr %TempChar, align 4, !dbg !638
  %cmp80 = icmp ne i32 %70, 12, !dbg !639
  br i1 %cmp80, label %if.then81, label %if.end99, !dbg !640

if.then81:                                        ; preds = %land.lhs.true79
  store i32 -1, ptr %RetVal, align 4, !dbg !641
  %71 = load ptr, ptr %ipFlag.addr, align 8, !dbg !643
  %status82 = getelementptr inbounds %struct.CFlag, ptr %71, i32 0, i32 9, !dbg !644
  %72 = load i32, ptr %status82, align 4, !dbg !645
  %or83 = or i32 %72, 1, !dbg !645
  store i32 %or83, ptr %status82, align 4, !dbg !645
  %73 = load ptr, ptr %ipFlag.addr, align 8, !dbg !646
  %verbose84 = getelementptr inbounds %struct.CFlag, ptr %73, i32 0, i32 1, !dbg !648
  %74 = load i32, ptr %verbose84, align 4, !dbg !648
  %tobool85 = icmp ne i32 %74, 0, !dbg !646
  br i1 %tobool85, label %if.then86, label %if.end98, !dbg !649

if.then86:                                        ; preds = %if.then81
  %75 = load ptr, ptr %ipFlag.addr, align 8, !dbg !650
  %stdio_mode87 = getelementptr inbounds %struct.CFlag, ptr %75, i32 0, i32 10, !dbg !653
  %76 = load i32, ptr %stdio_mode87, align 4, !dbg !653
  %tobool88 = icmp ne i32 %76, 0, !dbg !654
  br i1 %tobool88, label %land.lhs.true89, label %if.end94, !dbg !655

land.lhs.true89:                                  ; preds = %if.then86
  %77 = load ptr, ptr %ipFlag.addr, align 8, !dbg !656
  %error90 = getelementptr inbounds %struct.CFlag, ptr %77, i32 0, i32 12, !dbg !657
  %78 = load i32, ptr %error90, align 4, !dbg !657
  %tobool91 = icmp ne i32 %78, 0, !dbg !656
  br i1 %tobool91, label %if.end94, label %if.then92, !dbg !658

if.then92:                                        ; preds = %land.lhs.true89
  %79 = load ptr, ptr %ipFlag.addr, align 8, !dbg !659
  %error93 = getelementptr inbounds %struct.CFlag, ptr %79, i32 0, i32 12, !dbg !660
  store i32 1, ptr %error93, align 4, !dbg !661
  br label %if.end94, !dbg !659

if.end94:                                         ; preds = %if.then92, %land.lhs.true89, %if.then86
  %80 = load ptr, ptr @stderr, align 8, !dbg !662
  %81 = load ptr, ptr %progname.addr, align 8, !dbg !663
  %call95 = call i32 (ptr, ptr, ...) @fprintf(ptr noundef %80, ptr noundef @.str.1, ptr noundef %81), !dbg !664
  %82 = load ptr, ptr @stderr, align 8, !dbg !665
  %call96 = call ptr @gettext(ptr noundef @.str.2) #7, !dbg !666
  %83 = load i32, ptr %TempChar, align 4, !dbg !667
  %84 = load i32, ptr %line_nr, align 4, !dbg !668
  %call97 = call i32 (ptr, ptr, ...) @fprintf(ptr noundef %82, ptr noundef %call96, i32 noundef %83, i32 noundef %84), !dbg !669
  br label %if.end98, !dbg !670

if.end98:                                         ; preds = %if.end94, %if.then81
  br label %while.end142, !dbg !671

if.end99:                                         ; preds = %land.lhs.true79, %land.lhs.true77, %land.lhs.true75, %land.lhs.true73, %land.lhs.true71, %while.body68
  %85 = load i32, ptr %TempChar, align 4, !dbg !672
  %cmp100 = icmp ne i32 %85, 13, !dbg !674
  br i1 %cmp100, label %if.then101, label %if.else110, !dbg !675

if.then101:                                       ; preds = %if.end99
  %86 = load i32, ptr %TempChar, align 4, !dbg !676
  %cmp102 = icmp eq i32 %86, 10, !dbg !679
  br i1 %cmp102, label %if.then103, label %if.end105, !dbg !680

if.then103:                                       ; preds = %if.then101
  %87 = load i32, ptr %line_nr, align 4, !dbg !681
  %inc104 = add i32 %87, 1, !dbg !681
  store i32 %inc104, ptr %line_nr, align 4, !dbg !681
  br label %if.end105, !dbg !681

if.end105:                                        ; preds = %if.then103, %if.then101
  %88 = load i32, ptr %TempChar, align 4, !dbg !682
  %89 = load ptr, ptr %ipOutF.addr, align 8, !dbg !684
  %90 = load ptr, ptr %ipFlag.addr, align 8, !dbg !685
  %91 = load ptr, ptr %progname.addr, align 8, !dbg !686
  %call106 = call i32 @d2u_putwc(i32 noundef %88, ptr noundef %89, ptr noundef %90, ptr noundef %91), !dbg !687
  %cmp107 = icmp eq i32 %call106, -1, !dbg !688
  br i1 %cmp107, label %if.then108, label %if.end109, !dbg !689

if.then108:                                       ; preds = %if.end105
  store i32 -1, ptr %RetVal, align 4, !dbg !690
  %92 = load ptr, ptr %ipFlag.addr, align 8, !dbg !692
  %93 = load ptr, ptr %progname.addr, align 8, !dbg !693
  call void @d2u_putwc_error(ptr noundef %92, ptr noundef %93), !dbg !694
  br label %while.end142, !dbg !695

if.end109:                                        ; preds = %if.end105
  br label %if.end141, !dbg !696

if.else110:                                       ; preds = %if.end99
  %94 = load ptr, ptr %ipInF.addr, align 8, !dbg !697
  %95 = load ptr, ptr %ipFlag.addr, align 8, !dbg !700
  %bomtype111 = getelementptr inbounds %struct.CFlag, ptr %95, i32 0, i32 13, !dbg !701
  %96 = load i32, ptr %bomtype111, align 4, !dbg !701
  %call112 = call i32 @d2u_getwc(ptr noundef %94, i32 noundef %96), !dbg !702
  store i32 %call112, ptr %TempNextChar, align 4, !dbg !703
  %cmp113 = icmp ne i32 %call112, -1, !dbg !704
  br i1 %cmp113, label %if.then114, label %if.end127, !dbg !705

if.then114:                                       ; preds = %if.else110
  %97 = load i32, ptr %TempNextChar, align 4, !dbg !706
  %98 = load ptr, ptr %ipInF.addr, align 8, !dbg !709
  %99 = load ptr, ptr %ipFlag.addr, align 8, !dbg !710
  %bomtype115 = getelementptr inbounds %struct.CFlag, ptr %99, i32 0, i32 13, !dbg !711
  %100 = load i32, ptr %bomtype115, align 4, !dbg !711
  %call116 = call i32 @d2u_ungetwc(i32 noundef %97, ptr noundef %98, i32 noundef %100), !dbg !712
  %cmp117 = icmp eq i32 %call116, -1, !dbg !713
  br i1 %cmp117, label %if.then118, label %if.end119, !dbg !714

if.then118:                                       ; preds = %if.then114
  %101 = load ptr, ptr %ipFlag.addr, align 8, !dbg !715
  %102 = load ptr, ptr %progname.addr, align 8, !dbg !717
  call void @d2u_getc_error(ptr noundef %101, ptr noundef %102), !dbg !718
  store i32 -1, ptr %RetVal, align 4, !dbg !719
  br label %while.end142, !dbg !720

if.end119:                                        ; preds = %if.then114
  %103 = load i32, ptr %TempNextChar, align 4, !dbg !721
  %cmp120 = icmp eq i32 %103, 10, !dbg !723
  br i1 %cmp120, label %if.then121, label %if.end126, !dbg !724

if.then121:                                       ; preds = %if.end119
  %104 = load ptr, ptr %ipOutF.addr, align 8, !dbg !725
  %105 = load ptr, ptr %ipFlag.addr, align 8, !dbg !728
  %106 = load ptr, ptr %progname.addr, align 8, !dbg !729
  %call122 = call i32 @d2u_putwc(i32 noundef 13, ptr noundef %104, ptr noundef %105, ptr noundef %106), !dbg !730
  %cmp123 = icmp eq i32 %call122, -1, !dbg !731
  br i1 %cmp123, label %if.then124, label %if.end125, !dbg !732

if.then124:                                       ; preds = %if.then121
  %107 = load ptr, ptr %ipFlag.addr, align 8, !dbg !733
  %108 = load ptr, ptr %progname.addr, align 8, !dbg !735
  call void @d2u_putwc_error(ptr noundef %107, ptr noundef %108), !dbg !736
  store i32 -1, ptr %RetVal, align 4, !dbg !737
  br label %while.end142, !dbg !738

if.end125:                                        ; preds = %if.then121
  %109 = load i32, ptr %TempChar, align 4, !dbg !739
  store i32 %109, ptr %PrevChar, align 4, !dbg !740
  br label %while.cond64, !dbg !741, !llvm.loop !742

if.end126:                                        ; preds = %if.end119
  br label %if.end127, !dbg !744

if.end127:                                        ; preds = %if.end126, %if.else110
  %110 = load ptr, ptr %ipOutF.addr, align 8, !dbg !745
  %111 = load ptr, ptr %ipFlag.addr, align 8, !dbg !747
  %112 = load ptr, ptr %progname.addr, align 8, !dbg !748
  %call128 = call i32 @d2u_putwc(i32 noundef 10, ptr noundef %110, ptr noundef %111, ptr noundef %112), !dbg !749
  %cmp129 = icmp eq i32 %call128, -1, !dbg !750
  br i1 %cmp129, label %if.then130, label %if.end131, !dbg !751

if.then130:                                       ; preds = %if.end127
  store i32 -1, ptr %RetVal, align 4, !dbg !752
  %113 = load ptr, ptr %ipFlag.addr, align 8, !dbg !754
  %114 = load ptr, ptr %progname.addr, align 8, !dbg !755
  call void @d2u_putwc_error(ptr noundef %113, ptr noundef %114), !dbg !756
  br label %while.end142, !dbg !757

if.end131:                                        ; preds = %if.end127
  %115 = load i32, ptr %converted, align 4, !dbg !758
  %inc132 = add i32 %115, 1, !dbg !758
  store i32 %inc132, ptr %converted, align 4, !dbg !758
  %116 = load i32, ptr %line_nr, align 4, !dbg !759
  %inc133 = add i32 %116, 1, !dbg !759
  store i32 %inc133, ptr %line_nr, align 4, !dbg !759
  %117 = load ptr, ptr %ipFlag.addr, align 8, !dbg !760
  %NewLine = getelementptr inbounds %struct.CFlag, ptr %117, i32 0, i32 5, !dbg !762
  %118 = load i32, ptr %NewLine, align 4, !dbg !762
  %tobool134 = icmp ne i32 %118, 0, !dbg !760
  br i1 %tobool134, label %if.then135, label %if.end140, !dbg !763

if.then135:                                       ; preds = %if.end131
  %119 = load ptr, ptr %ipOutF.addr, align 8, !dbg !764
  %120 = load ptr, ptr %ipFlag.addr, align 8, !dbg !767
  %121 = load ptr, ptr %progname.addr, align 8, !dbg !768
  %call136 = call i32 @d2u_putwc(i32 noundef 10, ptr noundef %119, ptr noundef %120, ptr noundef %121), !dbg !769
  %cmp137 = icmp eq i32 %call136, -1, !dbg !770
  br i1 %cmp137, label %if.then138, label %if.end139, !dbg !771

if.then138:                                       ; preds = %if.then135
  store i32 -1, ptr %RetVal, align 4, !dbg !772
  %122 = load ptr, ptr %ipFlag.addr, align 8, !dbg !774
  %123 = load ptr, ptr %progname.addr, align 8, !dbg !775
  call void @d2u_putwc_error(ptr noundef %122, ptr noundef %123), !dbg !776
  br label %while.end142, !dbg !777

if.end139:                                        ; preds = %if.then135
  br label %if.end140, !dbg !778

if.end140:                                        ; preds = %if.end139, %if.end131
  br label %if.end141

if.end141:                                        ; preds = %if.end140, %if.end109
  %124 = load i32, ptr %TempChar, align 4, !dbg !779
  store i32 %124, ptr %PrevChar, align 4, !dbg !780
  br label %while.cond64, !dbg !613, !llvm.loop !742

while.end142:                                     ; preds = %if.then138, %if.then130, %if.then124, %if.then118, %if.then108, %if.end98, %while.cond64
  %125 = load i32, ptr %TempChar, align 4, !dbg !781
  %cmp143 = icmp eq i32 %125, -1, !dbg !783
  br i1 %cmp143, label %land.lhs.true144, label %if.end164, !dbg !784

land.lhs.true144:                                 ; preds = %while.end142
  %126 = load ptr, ptr %ipFlag.addr, align 8, !dbg !785
  %add_eol145 = getelementptr inbounds %struct.CFlag, ptr %126, i32 0, i32 20, !dbg !786
  %127 = load i32, ptr %add_eol145, align 4, !dbg !786
  %tobool146 = icmp ne i32 %127, 0, !dbg !785
  br i1 %tobool146, label %land.lhs.true147, label %if.end164, !dbg !787

land.lhs.true147:                                 ; preds = %land.lhs.true144
  %128 = load i32, ptr %PrevChar, align 4, !dbg !788
  %cmp148 = icmp ne i32 %128, -1, !dbg !789
  br i1 %cmp148, label %land.lhs.true149, label %if.end164, !dbg !790

land.lhs.true149:                                 ; preds = %land.lhs.true147
  %129 = load i32, ptr %PrevChar, align 4, !dbg !791
  %cmp150 = icmp eq i32 %129, 10, !dbg !792
  br i1 %cmp150, label %if.end164, label %lor.lhs.false, !dbg !793

lor.lhs.false:                                    ; preds = %land.lhs.true149
  %130 = load i32, ptr %PrevChar, align 4, !dbg !794
  %cmp151 = icmp eq i32 %130, 13, !dbg !795
  br i1 %cmp151, label %if.end164, label %if.then152, !dbg !796

if.then152:                                       ; preds = %lor.lhs.false
  %131 = load ptr, ptr %ipFlag.addr, align 8, !dbg !797
  %verbose153 = getelementptr inbounds %struct.CFlag, ptr %131, i32 0, i32 1, !dbg !800
  %132 = load i32, ptr %verbose153, align 4, !dbg !800
  %cmp154 = icmp sgt i32 %132, 1, !dbg !801
  br i1 %cmp154, label %if.then155, label %if.end159, !dbg !802

if.then155:                                       ; preds = %if.then152
  %133 = load ptr, ptr @stderr, align 8, !dbg !803
  %134 = load ptr, ptr %progname.addr, align 8, !dbg !805
  %call156 = call i32 (ptr, ptr, ...) @fprintf(ptr noundef %133, ptr noundef @.str.1, ptr noundef %134), !dbg !806
  %135 = load ptr, ptr @stderr, align 8, !dbg !807
  %call157 = call ptr @gettext(ptr noundef @.str.3) #7, !dbg !808
  %call158 = call i32 (ptr, ptr, ...) @fprintf(ptr noundef %135, ptr noundef %call157), !dbg !809
  br label %if.end159, !dbg !810

if.end159:                                        ; preds = %if.then155, %if.then152
  %136 = load ptr, ptr %ipOutF.addr, align 8, !dbg !811
  %137 = load ptr, ptr %ipFlag.addr, align 8, !dbg !813
  %138 = load ptr, ptr %progname.addr, align 8, !dbg !814
  %call160 = call i32 @d2u_putwc(i32 noundef 10, ptr noundef %136, ptr noundef %137, ptr noundef %138), !dbg !815
  %cmp161 = icmp eq i32 %call160, -1, !dbg !816
  br i1 %cmp161, label %if.then162, label %if.end163, !dbg !817

if.then162:                                       ; preds = %if.end159
  store i32 -1, ptr %RetVal, align 4, !dbg !818
  %139 = load ptr, ptr %ipFlag.addr, align 8, !dbg !820
  %140 = load ptr, ptr %progname.addr, align 8, !dbg !821
  call void @d2u_putwc_error(ptr noundef %139, ptr noundef %140), !dbg !822
  br label %if.end163, !dbg !823

if.end163:                                        ; preds = %if.then162, %if.end159
  br label %if.end164, !dbg !824

if.end164:                                        ; preds = %if.end163, %lor.lhs.false, %land.lhs.true149, %land.lhs.true147, %land.lhs.true144, %while.end142
  %141 = load i32, ptr %TempChar, align 4, !dbg !825
  %cmp165 = icmp eq i32 %141, -1, !dbg !827
  br i1 %cmp165, label %land.lhs.true166, label %if.end170, !dbg !828

land.lhs.true166:                                 ; preds = %if.end164
  %142 = load ptr, ptr %ipInF.addr, align 8, !dbg !829
  %call167 = call i32 @ferror(ptr noundef %142) #7, !dbg !830
  %tobool168 = icmp ne i32 %call167, 0, !dbg !830
  br i1 %tobool168, label %if.then169, label %if.end170, !dbg !831

if.then169:                                       ; preds = %land.lhs.true166
  store i32 -1, ptr %RetVal, align 4, !dbg !832
  %143 = load ptr, ptr %ipFlag.addr, align 8, !dbg !834
  %144 = load ptr, ptr %progname.addr, align 8, !dbg !835
  call void @d2u_getc_error(ptr noundef %143, ptr noundef %144), !dbg !836
  br label %if.end170, !dbg !837

if.end170:                                        ; preds = %if.then169, %land.lhs.true166, %if.end164
  br label %sw.epilog, !dbg !838

sw.default:                                       ; preds = %entry
  br label %sw.epilog, !dbg !839

sw.epilog:                                        ; preds = %sw.default, %if.end170, %if.end62
  %145 = load ptr, ptr %ipFlag.addr, align 8, !dbg !840
  %status171 = getelementptr inbounds %struct.CFlag, ptr %145, i32 0, i32 9, !dbg !842
  %146 = load i32, ptr %status171, align 4, !dbg !842
  %and = and i32 %146, 256, !dbg !843
  %tobool172 = icmp ne i32 %and, 0, !dbg !843
  br i1 %tobool172, label %if.then173, label %if.end175, !dbg !844

if.then173:                                       ; preds = %sw.epilog
  %147 = load i32, ptr %line_nr, align 4, !dbg !845
  %148 = load ptr, ptr %ipFlag.addr, align 8, !dbg !846
  %line_nr174 = getelementptr inbounds %struct.CFlag, ptr %148, i32 0, i32 19, !dbg !847
  store i32 %147, ptr %line_nr174, align 4, !dbg !848
  br label %if.end175, !dbg !846

if.end175:                                        ; preds = %if.then173, %sw.epilog
  %149 = load i32, ptr %RetVal, align 4, !dbg !849
  %cmp176 = icmp eq i32 %149, 0, !dbg !851
  br i1 %cmp176, label %land.lhs.true177, label %if.end184, !dbg !852

land.lhs.true177:                                 ; preds = %if.end175
  %150 = load ptr, ptr %ipFlag.addr, align 8, !dbg !853
  %verbose178 = getelementptr inbounds %struct.CFlag, ptr %150, i32 0, i32 1, !dbg !854
  %151 = load i32, ptr %verbose178, align 4, !dbg !854
  %cmp179 = icmp sgt i32 %151, 1, !dbg !855
  br i1 %cmp179, label %if.then180, label %if.end184, !dbg !856

if.then180:                                       ; preds = %land.lhs.true177
  %152 = load ptr, ptr @stderr, align 8, !dbg !857
  %153 = load ptr, ptr %progname.addr, align 8, !dbg !859
  %call181 = call i32 (ptr, ptr, ...) @fprintf(ptr noundef %152, ptr noundef @.str.1, ptr noundef %153), !dbg !860
  %154 = load ptr, ptr @stderr, align 8, !dbg !861
  %call182 = call ptr @gettext(ptr noundef @.str.4) #7, !dbg !862
  %155 = load i32, ptr %converted, align 4, !dbg !863
  %156 = load i32, ptr %line_nr, align 4, !dbg !864
  %sub = sub i32 %156, 1, !dbg !865
  %call183 = call i32 (ptr, ptr, ...) @fprintf(ptr noundef %154, ptr noundef %call182, i32 noundef %155, i32 noundef %sub), !dbg !866
  br label %if.end184, !dbg !867

if.end184:                                        ; preds = %if.then180, %land.lhs.true177, %if.end175
  %157 = load i32, ptr %RetVal, align 4, !dbg !868
  ret i32 %157, !dbg !869
}

; Function Attrs: noinline nounwind uwtable
define dso_local i32 @ConvertDosToUnix(ptr noundef %ipInF, ptr noundef %ipOutF, ptr noundef %ipFlag, ptr noundef %progname) #0 !dbg !870 {
entry:
  %retval = alloca i32, align 4
  %ipInF.addr = alloca ptr, align 8
  %ipOutF.addr = alloca ptr, align 8
  %ipFlag.addr = alloca ptr, align 8
  %progname.addr = alloca ptr, align 8
  %RetVal = alloca i32, align 4
  %PrevChar = alloca i32, align 4
  %TempChar = alloca i32, align 4
  %TempNextChar = alloca i32, align 4
  %ConvTable = alloca ptr, align 8
  %line_nr = alloca i32, align 4
  %converted = alloca i32, align 4
  store ptr %ipInF, ptr %ipInF.addr, align 8
  call void @llvm.dbg.declare(metadata ptr %ipInF.addr, metadata !871, metadata !DIExpression()), !dbg !872
  store ptr %ipOutF, ptr %ipOutF.addr, align 8
  call void @llvm.dbg.declare(metadata ptr %ipOutF.addr, metadata !873, metadata !DIExpression()), !dbg !874
  store ptr %ipFlag, ptr %ipFlag.addr, align 8
  call void @llvm.dbg.declare(metadata ptr %ipFlag.addr, metadata !875, metadata !DIExpression()), !dbg !876
  store ptr %progname, ptr %progname.addr, align 8
  call void @llvm.dbg.declare(metadata ptr %progname.addr, metadata !877, metadata !DIExpression()), !dbg !878
  call void @llvm.dbg.declare(metadata ptr %RetVal, metadata !879, metadata !DIExpression()), !dbg !880
  store i32 0, ptr %RetVal, align 4, !dbg !880
  call void @llvm.dbg.declare(metadata ptr %PrevChar, metadata !881, metadata !DIExpression()), !dbg !882
  store i32 -1, ptr %PrevChar, align 4, !dbg !882
  call void @llvm.dbg.declare(metadata ptr %TempChar, metadata !883, metadata !DIExpression()), !dbg !884
  call void @llvm.dbg.declare(metadata ptr %TempNextChar, metadata !885, metadata !DIExpression()), !dbg !886
  call void @llvm.dbg.declare(metadata ptr %ConvTable, metadata !887, metadata !DIExpression()), !dbg !889
  call void @llvm.dbg.declare(metadata ptr %line_nr, metadata !890, metadata !DIExpression()), !dbg !891
  store i32 1, ptr %line_nr, align 4, !dbg !891
  call void @llvm.dbg.declare(metadata ptr %converted, metadata !892, metadata !DIExpression()), !dbg !893
  store i32 0, ptr %converted, align 4, !dbg !893
  %0 = load ptr, ptr %ipFlag.addr, align 8, !dbg !894
  %status = getelementptr inbounds %struct.CFlag, ptr %0, i32 0, i32 9, !dbg !895
  store i32 0, ptr %status, align 4, !dbg !896
  %1 = load ptr, ptr %ipFlag.addr, align 8, !dbg !897
  %ConvMode = getelementptr inbounds %struct.CFlag, ptr %1, i32 0, i32 3, !dbg !898
  %2 = load i32, ptr %ConvMode, align 4, !dbg !898
  switch i32 %2, label %sw.default [
    i32 0, label %sw.bb
    i32 1, label %sw.bb
    i32 2, label %sw.bb
    i32 3, label %sw.bb1
    i32 437, label %sw.bb2
    i32 850, label %sw.bb3
    i32 860, label %sw.bb4
    i32 863, label %sw.bb5
    i32 865, label %sw.bb6
    i32 1252, label %sw.bb7
  ], !dbg !899

sw.bb:                                            ; preds = %entry, %entry, %entry
  store ptr @D2UAsciiTable, ptr %ConvTable, align 8, !dbg !900
  br label %sw.epilog, !dbg !902

sw.bb1:                                           ; preds = %entry
  store ptr @D2U7BitTable, ptr %ConvTable, align 8, !dbg !903
  br label %sw.epilog, !dbg !904

sw.bb2:                                           ; preds = %entry
  store ptr @D2UIso437Table, ptr %ConvTable, align 8, !dbg !905
  br label %sw.epilog, !dbg !906

sw.bb3:                                           ; preds = %entry
  store ptr @D2UIso850Table, ptr %ConvTable, align 8, !dbg !907
  br label %sw.epilog, !dbg !908

sw.bb4:                                           ; preds = %entry
  store ptr @D2UIso860Table, ptr %ConvTable, align 8, !dbg !909
  br label %sw.epilog, !dbg !910

sw.bb5:                                           ; preds = %entry
  store ptr @D2UIso863Table, ptr %ConvTable, align 8, !dbg !911
  br label %sw.epilog, !dbg !912

sw.bb6:                                           ; preds = %entry
  store ptr @D2UIso865Table, ptr %ConvTable, align 8, !dbg !913
  br label %sw.epilog, !dbg !914

sw.bb7:                                           ; preds = %entry
  store ptr @D2UIso1252Table, ptr %ConvTable, align 8, !dbg !915
  br label %sw.epilog, !dbg !916

sw.default:                                       ; preds = %entry
  %3 = load ptr, ptr %ipFlag.addr, align 8, !dbg !917
  %status8 = getelementptr inbounds %struct.CFlag, ptr %3, i32 0, i32 9, !dbg !918
  %4 = load i32, ptr %status8, align 4, !dbg !919
  %or = or i32 %4, 4, !dbg !919
  store i32 %or, ptr %status8, align 4, !dbg !919
  store i32 -1, ptr %retval, align 4, !dbg !920
  br label %return, !dbg !920

sw.epilog:                                        ; preds = %sw.bb7, %sw.bb6, %sw.bb5, %sw.bb4, %sw.bb3, %sw.bb2, %sw.bb1, %sw.bb
  %5 = load ptr, ptr %ipFlag.addr, align 8, !dbg !921
  %bomtype = getelementptr inbounds %struct.CFlag, ptr %5, i32 0, i32 13, !dbg !923
  %6 = load i32, ptr %bomtype, align 4, !dbg !923
  %cmp = icmp sgt i32 %6, 0, !dbg !924
  br i1 %cmp, label %if.then, label %if.end, !dbg !925

if.then:                                          ; preds = %sw.epilog
  store ptr @D2UAsciiTable, ptr %ConvTable, align 8, !dbg !926
  br label %if.end, !dbg !927

if.end:                                           ; preds = %if.then, %sw.epilog
  %7 = load ptr, ptr %ipFlag.addr, align 8, !dbg !928
  %ConvMode9 = getelementptr inbounds %struct.CFlag, ptr %7, i32 0, i32 3, !dbg !930
  %8 = load i32, ptr %ConvMode9, align 4, !dbg !930
  %cmp10 = icmp sgt i32 %8, 3, !dbg !931
  br i1 %cmp10, label %land.lhs.true, label %if.end15, !dbg !932

land.lhs.true:                                    ; preds = %if.end
  %9 = load ptr, ptr %ipFlag.addr, align 8, !dbg !933
  %verbose = getelementptr inbounds %struct.CFlag, ptr %9, i32 0, i32 1, !dbg !934
  %10 = load i32, ptr %verbose, align 4, !dbg !934
  %tobool = icmp ne i32 %10, 0, !dbg !935
  br i1 %tobool, label %if.then11, label %if.end15, !dbg !936

if.then11:                                        ; preds = %land.lhs.true
  %11 = load ptr, ptr @stderr, align 8, !dbg !937
  %12 = load ptr, ptr %progname.addr, align 8, !dbg !939
  %call = call i32 (ptr, ptr, ...) @fprintf(ptr noundef %11, ptr noundef @.str.1, ptr noundef %12), !dbg !940
  %13 = load ptr, ptr @stderr, align 8, !dbg !941
  %call12 = call ptr @gettext(ptr noundef @.str.5) #7, !dbg !942
  %14 = load ptr, ptr %ipFlag.addr, align 8, !dbg !943
  %ConvMode13 = getelementptr inbounds %struct.CFlag, ptr %14, i32 0, i32 3, !dbg !944
  %15 = load i32, ptr %ConvMode13, align 4, !dbg !944
  %call14 = call i32 (ptr, ptr, ...) @fprintf(ptr noundef %13, ptr noundef %call12, i32 noundef %15), !dbg !945
  br label %if.end15, !dbg !946

if.end15:                                         ; preds = %if.then11, %land.lhs.true, %if.end
  %16 = load ptr, ptr %ipFlag.addr, align 8, !dbg !947
  %FromToMode = getelementptr inbounds %struct.CFlag, ptr %16, i32 0, i32 4, !dbg !948
  %17 = load i32, ptr %FromToMode, align 4, !dbg !948
  switch i32 %17, label %sw.default194 [
    i32 0, label %sw.bb16
    i32 1, label %sw.bb87
  ], !dbg !949

sw.bb16:                                          ; preds = %if.end15
  br label %while.cond, !dbg !950

while.cond:                                       ; preds = %if.end60, %sw.bb16
  %18 = load ptr, ptr %ipInF.addr, align 8, !dbg !952
  %call17 = call i32 @fgetc(ptr noundef %18), !dbg !953
  store i32 %call17, ptr %TempChar, align 4, !dbg !954
  %cmp18 = icmp ne i32 %call17, -1, !dbg !955
  br i1 %cmp18, label %while.body, label %while.end, !dbg !950

while.body:                                       ; preds = %while.cond
  %19 = load ptr, ptr %ipFlag.addr, align 8, !dbg !956
  %Force = getelementptr inbounds %struct.CFlag, ptr %19, i32 0, i32 6, !dbg !959
  %20 = load i32, ptr %Force, align 4, !dbg !959
  %cmp19 = icmp eq i32 %20, 0, !dbg !960
  br i1 %cmp19, label %land.lhs.true20, label %if.end46, !dbg !961

land.lhs.true20:                                  ; preds = %while.body
  %21 = load i32, ptr %TempChar, align 4, !dbg !962
  %cmp21 = icmp slt i32 %21, 32, !dbg !963
  br i1 %cmp21, label %land.lhs.true22, label %if.end46, !dbg !964

land.lhs.true22:                                  ; preds = %land.lhs.true20
  %22 = load i32, ptr %TempChar, align 4, !dbg !965
  %cmp23 = icmp ne i32 %22, 10, !dbg !966
  br i1 %cmp23, label %land.lhs.true24, label %if.end46, !dbg !967

land.lhs.true24:                                  ; preds = %land.lhs.true22
  %23 = load i32, ptr %TempChar, align 4, !dbg !968
  %cmp25 = icmp ne i32 %23, 13, !dbg !969
  br i1 %cmp25, label %land.lhs.true26, label %if.end46, !dbg !970

land.lhs.true26:                                  ; preds = %land.lhs.true24
  %24 = load i32, ptr %TempChar, align 4, !dbg !971
  %cmp27 = icmp ne i32 %24, 9, !dbg !972
  br i1 %cmp27, label %land.lhs.true28, label %if.end46, !dbg !973

land.lhs.true28:                                  ; preds = %land.lhs.true26
  %25 = load i32, ptr %TempChar, align 4, !dbg !974
  %cmp29 = icmp ne i32 %25, 12, !dbg !975
  br i1 %cmp29, label %if.then30, label %if.end46, !dbg !976

if.then30:                                        ; preds = %land.lhs.true28
  store i32 -1, ptr %RetVal, align 4, !dbg !977
  %26 = load ptr, ptr %ipFlag.addr, align 8, !dbg !979
  %status31 = getelementptr inbounds %struct.CFlag, ptr %26, i32 0, i32 9, !dbg !980
  %27 = load i32, ptr %status31, align 4, !dbg !981
  %or32 = or i32 %27, 1, !dbg !981
  store i32 %or32, ptr %status31, align 4, !dbg !981
  %28 = load ptr, ptr %ipFlag.addr, align 8, !dbg !982
  %verbose33 = getelementptr inbounds %struct.CFlag, ptr %28, i32 0, i32 1, !dbg !984
  %29 = load i32, ptr %verbose33, align 4, !dbg !984
  %tobool34 = icmp ne i32 %29, 0, !dbg !982
  br i1 %tobool34, label %if.then35, label %if.end45, !dbg !985

if.then35:                                        ; preds = %if.then30
  %30 = load ptr, ptr %ipFlag.addr, align 8, !dbg !986
  %stdio_mode = getelementptr inbounds %struct.CFlag, ptr %30, i32 0, i32 10, !dbg !989
  %31 = load i32, ptr %stdio_mode, align 4, !dbg !989
  %tobool36 = icmp ne i32 %31, 0, !dbg !990
  br i1 %tobool36, label %land.lhs.true37, label %if.end41, !dbg !991

land.lhs.true37:                                  ; preds = %if.then35
  %32 = load ptr, ptr %ipFlag.addr, align 8, !dbg !992
  %error = getelementptr inbounds %struct.CFlag, ptr %32, i32 0, i32 12, !dbg !993
  %33 = load i32, ptr %error, align 4, !dbg !993
  %tobool38 = icmp ne i32 %33, 0, !dbg !992
  br i1 %tobool38, label %if.end41, label %if.then39, !dbg !994

if.then39:                                        ; preds = %land.lhs.true37
  %34 = load ptr, ptr %ipFlag.addr, align 8, !dbg !995
  %error40 = getelementptr inbounds %struct.CFlag, ptr %34, i32 0, i32 12, !dbg !996
  store i32 1, ptr %error40, align 4, !dbg !997
  br label %if.end41, !dbg !995

if.end41:                                         ; preds = %if.then39, %land.lhs.true37, %if.then35
  %35 = load ptr, ptr @stderr, align 8, !dbg !998
  %36 = load ptr, ptr %progname.addr, align 8, !dbg !999
  %call42 = call i32 (ptr, ptr, ...) @fprintf(ptr noundef %35, ptr noundef @.str.1, ptr noundef %36), !dbg !1000
  %37 = load ptr, ptr @stderr, align 8, !dbg !1001
  %call43 = call ptr @gettext(ptr noundef @.str.6) #7, !dbg !1002
  %38 = load i32, ptr %TempChar, align 4, !dbg !1003
  %39 = load i32, ptr %line_nr, align 4, !dbg !1004
  %call44 = call i32 (ptr, ptr, ...) @fprintf(ptr noundef %37, ptr noundef %call43, i32 noundef %38, i32 noundef %39), !dbg !1005
  br label %if.end45, !dbg !1006

if.end45:                                         ; preds = %if.end41, %if.then30
  br label %while.end, !dbg !1007

if.end46:                                         ; preds = %land.lhs.true28, %land.lhs.true26, %land.lhs.true24, %land.lhs.true22, %land.lhs.true20, %while.body
  %40 = load i32, ptr %TempChar, align 4, !dbg !1008
  %cmp47 = icmp ne i32 %40, 13, !dbg !1010
  br i1 %cmp47, label %if.then48, label %if.else, !dbg !1011

if.then48:                                        ; preds = %if.end46
  %41 = load i32, ptr %TempChar, align 4, !dbg !1012
  %cmp49 = icmp eq i32 %41, 10, !dbg !1015
  br i1 %cmp49, label %if.then50, label %if.end51, !dbg !1016

if.then50:                                        ; preds = %if.then48
  %42 = load i32, ptr %line_nr, align 4, !dbg !1017
  %inc = add i32 %42, 1, !dbg !1017
  store i32 %inc, ptr %line_nr, align 4, !dbg !1017
  br label %if.end51, !dbg !1017

if.end51:                                         ; preds = %if.then50, %if.then48
  %43 = load ptr, ptr %ConvTable, align 8, !dbg !1018
  %44 = load i32, ptr %TempChar, align 4, !dbg !1020
  %idxprom = sext i32 %44 to i64, !dbg !1018
  %arrayidx = getelementptr inbounds i32, ptr %43, i64 %idxprom, !dbg !1018
  %45 = load i32, ptr %arrayidx, align 4, !dbg !1018
  %46 = load ptr, ptr %ipOutF.addr, align 8, !dbg !1021
  %call52 = call i32 @fputc(i32 noundef %45, ptr noundef %46), !dbg !1022
  %cmp53 = icmp eq i32 %call52, -1, !dbg !1023
  br i1 %cmp53, label %if.then54, label %if.end55, !dbg !1024

if.then54:                                        ; preds = %if.end51
  store i32 -1, ptr %RetVal, align 4, !dbg !1025
  %47 = load ptr, ptr %ipFlag.addr, align 8, !dbg !1027
  %48 = load ptr, ptr %progname.addr, align 8, !dbg !1028
  call void @d2u_putc_error(ptr noundef %47, ptr noundef %48), !dbg !1029
  br label %while.end, !dbg !1030

if.end55:                                         ; preds = %if.end51
  br label %if.end60, !dbg !1031

if.else:                                          ; preds = %if.end46
  %49 = load ptr, ptr %ipInF.addr, align 8, !dbg !1032
  %50 = load ptr, ptr %ipOutF.addr, align 8, !dbg !1035
  %51 = load ptr, ptr %ipFlag.addr, align 8, !dbg !1036
  %52 = load i32, ptr %TempChar, align 4, !dbg !1037
  %53 = load ptr, ptr %progname.addr, align 8, !dbg !1038
  %call56 = call i32 @StripDelimiter(ptr noundef %49, ptr noundef %50, ptr noundef %51, i32 noundef %52, ptr noundef %converted, ptr noundef %53), !dbg !1039
  %cmp57 = icmp eq i32 %call56, -1, !dbg !1040
  br i1 %cmp57, label %if.then58, label %if.end59, !dbg !1041

if.then58:                                        ; preds = %if.else
  store i32 -1, ptr %RetVal, align 4, !dbg !1042
  br label %while.end, !dbg !1044

if.end59:                                         ; preds = %if.else
  br label %if.end60

if.end60:                                         ; preds = %if.end59, %if.end55
  %54 = load i32, ptr %TempChar, align 4, !dbg !1045
  store i32 %54, ptr %PrevChar, align 4, !dbg !1046
  br label %while.cond, !dbg !950, !llvm.loop !1047

while.end:                                        ; preds = %if.then58, %if.then54, %if.end45, %while.cond
  %55 = load i32, ptr %TempChar, align 4, !dbg !1049
  %cmp61 = icmp eq i32 %55, -1, !dbg !1051
  br i1 %cmp61, label %land.lhs.true62, label %if.end80, !dbg !1052

land.lhs.true62:                                  ; preds = %while.end
  %56 = load ptr, ptr %ipFlag.addr, align 8, !dbg !1053
  %add_eol = getelementptr inbounds %struct.CFlag, ptr %56, i32 0, i32 20, !dbg !1054
  %57 = load i32, ptr %add_eol, align 4, !dbg !1054
  %tobool63 = icmp ne i32 %57, 0, !dbg !1053
  br i1 %tobool63, label %land.lhs.true64, label %if.end80, !dbg !1055

land.lhs.true64:                                  ; preds = %land.lhs.true62
  %58 = load i32, ptr %PrevChar, align 4, !dbg !1056
  %cmp65 = icmp ne i32 %58, -1, !dbg !1057
  br i1 %cmp65, label %land.lhs.true66, label %if.end80, !dbg !1058

land.lhs.true66:                                  ; preds = %land.lhs.true64
  %59 = load i32, ptr %PrevChar, align 4, !dbg !1059
  %cmp67 = icmp ne i32 %59, 10, !dbg !1060
  br i1 %cmp67, label %if.then68, label %if.end80, !dbg !1061

if.then68:                                        ; preds = %land.lhs.true66
  %60 = load ptr, ptr %ipFlag.addr, align 8, !dbg !1062
  %verbose69 = getelementptr inbounds %struct.CFlag, ptr %60, i32 0, i32 1, !dbg !1065
  %61 = load i32, ptr %verbose69, align 4, !dbg !1065
  %cmp70 = icmp sgt i32 %61, 1, !dbg !1066
  br i1 %cmp70, label %if.then71, label %if.end75, !dbg !1067

if.then71:                                        ; preds = %if.then68
  %62 = load ptr, ptr @stderr, align 8, !dbg !1068
  %63 = load ptr, ptr %progname.addr, align 8, !dbg !1070
  %call72 = call i32 (ptr, ptr, ...) @fprintf(ptr noundef %62, ptr noundef @.str.1, ptr noundef %63), !dbg !1071
  %64 = load ptr, ptr @stderr, align 8, !dbg !1072
  %call73 = call ptr @gettext(ptr noundef @.str.3) #7, !dbg !1073
  %call74 = call i32 (ptr, ptr, ...) @fprintf(ptr noundef %64, ptr noundef %call73), !dbg !1074
  br label %if.end75, !dbg !1075

if.end75:                                         ; preds = %if.then71, %if.then68
  %65 = load ptr, ptr %ipOutF.addr, align 8, !dbg !1076
  %call76 = call i32 @fputc(i32 noundef 10, ptr noundef %65), !dbg !1078
  %cmp77 = icmp eq i32 %call76, -1, !dbg !1079
  br i1 %cmp77, label %if.then78, label %if.end79, !dbg !1080

if.then78:                                        ; preds = %if.end75
  store i32 -1, ptr %RetVal, align 4, !dbg !1081
  %66 = load ptr, ptr %ipFlag.addr, align 8, !dbg !1083
  %67 = load ptr, ptr %progname.addr, align 8, !dbg !1084
  call void @d2u_putc_error(ptr noundef %66, ptr noundef %67), !dbg !1085
  br label %if.end79, !dbg !1086

if.end79:                                         ; preds = %if.then78, %if.end75
  br label %if.end80, !dbg !1087

if.end80:                                         ; preds = %if.end79, %land.lhs.true66, %land.lhs.true64, %land.lhs.true62, %while.end
  %68 = load i32, ptr %TempChar, align 4, !dbg !1088
  %cmp81 = icmp eq i32 %68, -1, !dbg !1090
  br i1 %cmp81, label %land.lhs.true82, label %if.end86, !dbg !1091

land.lhs.true82:                                  ; preds = %if.end80
  %69 = load ptr, ptr %ipInF.addr, align 8, !dbg !1092
  %call83 = call i32 @ferror(ptr noundef %69) #7, !dbg !1093
  %tobool84 = icmp ne i32 %call83, 0, !dbg !1093
  br i1 %tobool84, label %if.then85, label %if.end86, !dbg !1094

if.then85:                                        ; preds = %land.lhs.true82
  store i32 -1, ptr %RetVal, align 4, !dbg !1095
  %70 = load ptr, ptr %ipFlag.addr, align 8, !dbg !1097
  %71 = load ptr, ptr %progname.addr, align 8, !dbg !1098
  call void @d2u_getc_error(ptr noundef %70, ptr noundef %71), !dbg !1099
  br label %if.end86, !dbg !1100

if.end86:                                         ; preds = %if.then85, %land.lhs.true82, %if.end80
  br label %sw.epilog195, !dbg !1101

sw.bb87:                                          ; preds = %if.end15
  br label %while.cond88, !dbg !1102

while.cond88:                                     ; preds = %if.end164, %if.end148, %sw.bb87
  %72 = load ptr, ptr %ipInF.addr, align 8, !dbg !1103
  %call89 = call i32 @fgetc(ptr noundef %72), !dbg !1104
  store i32 %call89, ptr %TempChar, align 4, !dbg !1105
  %cmp90 = icmp ne i32 %call89, -1, !dbg !1106
  br i1 %cmp90, label %while.body91, label %while.end165, !dbg !1102

while.body91:                                     ; preds = %while.cond88
  %73 = load ptr, ptr %ipFlag.addr, align 8, !dbg !1107
  %Force92 = getelementptr inbounds %struct.CFlag, ptr %73, i32 0, i32 6, !dbg !1110
  %74 = load i32, ptr %Force92, align 4, !dbg !1110
  %cmp93 = icmp eq i32 %74, 0, !dbg !1111
  br i1 %cmp93, label %land.lhs.true94, label %if.end122, !dbg !1112

land.lhs.true94:                                  ; preds = %while.body91
  %75 = load i32, ptr %TempChar, align 4, !dbg !1113
  %cmp95 = icmp slt i32 %75, 32, !dbg !1114
  br i1 %cmp95, label %land.lhs.true96, label %if.end122, !dbg !1115

land.lhs.true96:                                  ; preds = %land.lhs.true94
  %76 = load i32, ptr %TempChar, align 4, !dbg !1116
  %cmp97 = icmp ne i32 %76, 10, !dbg !1117
  br i1 %cmp97, label %land.lhs.true98, label %if.end122, !dbg !1118

land.lhs.true98:                                  ; preds = %land.lhs.true96
  %77 = load i32, ptr %TempChar, align 4, !dbg !1119
  %cmp99 = icmp ne i32 %77, 13, !dbg !1120
  br i1 %cmp99, label %land.lhs.true100, label %if.end122, !dbg !1121

land.lhs.true100:                                 ; preds = %land.lhs.true98
  %78 = load i32, ptr %TempChar, align 4, !dbg !1122
  %cmp101 = icmp ne i32 %78, 9, !dbg !1123
  br i1 %cmp101, label %land.lhs.true102, label %if.end122, !dbg !1124

land.lhs.true102:                                 ; preds = %land.lhs.true100
  %79 = load i32, ptr %TempChar, align 4, !dbg !1125
  %cmp103 = icmp ne i32 %79, 12, !dbg !1126
  br i1 %cmp103, label %if.then104, label %if.end122, !dbg !1127

if.then104:                                       ; preds = %land.lhs.true102
  store i32 -1, ptr %RetVal, align 4, !dbg !1128
  %80 = load ptr, ptr %ipFlag.addr, align 8, !dbg !1130
  %status105 = getelementptr inbounds %struct.CFlag, ptr %80, i32 0, i32 9, !dbg !1131
  %81 = load i32, ptr %status105, align 4, !dbg !1132
  %or106 = or i32 %81, 1, !dbg !1132
  store i32 %or106, ptr %status105, align 4, !dbg !1132
  %82 = load ptr, ptr %ipFlag.addr, align 8, !dbg !1133
  %verbose107 = getelementptr inbounds %struct.CFlag, ptr %82, i32 0, i32 1, !dbg !1135
  %83 = load i32, ptr %verbose107, align 4, !dbg !1135
  %tobool108 = icmp ne i32 %83, 0, !dbg !1133
  br i1 %tobool108, label %if.then109, label %if.end121, !dbg !1136

if.then109:                                       ; preds = %if.then104
  %84 = load ptr, ptr %ipFlag.addr, align 8, !dbg !1137
  %stdio_mode110 = getelementptr inbounds %struct.CFlag, ptr %84, i32 0, i32 10, !dbg !1140
  %85 = load i32, ptr %stdio_mode110, align 4, !dbg !1140
  %tobool111 = icmp ne i32 %85, 0, !dbg !1141
  br i1 %tobool111, label %land.lhs.true112, label %if.end117, !dbg !1142

land.lhs.true112:                                 ; preds = %if.then109
  %86 = load ptr, ptr %ipFlag.addr, align 8, !dbg !1143
  %error113 = getelementptr inbounds %struct.CFlag, ptr %86, i32 0, i32 12, !dbg !1144
  %87 = load i32, ptr %error113, align 4, !dbg !1144
  %tobool114 = icmp ne i32 %87, 0, !dbg !1143
  br i1 %tobool114, label %if.end117, label %if.then115, !dbg !1145

if.then115:                                       ; preds = %land.lhs.true112
  %88 = load ptr, ptr %ipFlag.addr, align 8, !dbg !1146
  %error116 = getelementptr inbounds %struct.CFlag, ptr %88, i32 0, i32 12, !dbg !1147
  store i32 1, ptr %error116, align 4, !dbg !1148
  br label %if.end117, !dbg !1146

if.end117:                                        ; preds = %if.then115, %land.lhs.true112, %if.then109
  %89 = load ptr, ptr @stderr, align 8, !dbg !1149
  %90 = load ptr, ptr %progname.addr, align 8, !dbg !1150
  %call118 = call i32 (ptr, ptr, ...) @fprintf(ptr noundef %89, ptr noundef @.str.1, ptr noundef %90), !dbg !1151
  %91 = load ptr, ptr @stderr, align 8, !dbg !1152
  %call119 = call ptr @gettext(ptr noundef @.str.6) #7, !dbg !1153
  %92 = load i32, ptr %TempChar, align 4, !dbg !1154
  %93 = load i32, ptr %line_nr, align 4, !dbg !1155
  %call120 = call i32 (ptr, ptr, ...) @fprintf(ptr noundef %91, ptr noundef %call119, i32 noundef %92, i32 noundef %93), !dbg !1156
  br label %if.end121, !dbg !1157

if.end121:                                        ; preds = %if.end117, %if.then104
  br label %while.end165, !dbg !1158

if.end122:                                        ; preds = %land.lhs.true102, %land.lhs.true100, %land.lhs.true98, %land.lhs.true96, %land.lhs.true94, %while.body91
  %94 = load i32, ptr %TempChar, align 4, !dbg !1159
  %cmp123 = icmp ne i32 %94, 13, !dbg !1161
  br i1 %cmp123, label %if.then124, label %if.else135, !dbg !1162

if.then124:                                       ; preds = %if.end122
  %95 = load i32, ptr %TempChar, align 4, !dbg !1163
  %cmp125 = icmp eq i32 %95, 10, !dbg !1166
  br i1 %cmp125, label %if.then126, label %if.end128, !dbg !1167

if.then126:                                       ; preds = %if.then124
  %96 = load i32, ptr %line_nr, align 4, !dbg !1168
  %inc127 = add i32 %96, 1, !dbg !1168
  store i32 %inc127, ptr %line_nr, align 4, !dbg !1168
  br label %if.end128, !dbg !1168

if.end128:                                        ; preds = %if.then126, %if.then124
  %97 = load ptr, ptr %ConvTable, align 8, !dbg !1169
  %98 = load i32, ptr %TempChar, align 4, !dbg !1171
  %idxprom129 = sext i32 %98 to i64, !dbg !1169
  %arrayidx130 = getelementptr inbounds i32, ptr %97, i64 %idxprom129, !dbg !1169
  %99 = load i32, ptr %arrayidx130, align 4, !dbg !1169
  %100 = load ptr, ptr %ipOutF.addr, align 8, !dbg !1172
  %call131 = call i32 @fputc(i32 noundef %99, ptr noundef %100), !dbg !1173
  %cmp132 = icmp eq i32 %call131, -1, !dbg !1174
  br i1 %cmp132, label %if.then133, label %if.end134, !dbg !1175

if.then133:                                       ; preds = %if.end128
  store i32 -1, ptr %RetVal, align 4, !dbg !1176
  %101 = load ptr, ptr %ipFlag.addr, align 8, !dbg !1178
  %102 = load ptr, ptr %progname.addr, align 8, !dbg !1179
  call void @d2u_putc_error(ptr noundef %101, ptr noundef %102), !dbg !1180
  br label %while.end165, !dbg !1181

if.end134:                                        ; preds = %if.end128
  br label %if.end164, !dbg !1182

if.else135:                                       ; preds = %if.end122
  %103 = load ptr, ptr %ipInF.addr, align 8, !dbg !1183
  %call136 = call i32 @fgetc(ptr noundef %103), !dbg !1186
  store i32 %call136, ptr %TempNextChar, align 4, !dbg !1187
  %cmp137 = icmp ne i32 %call136, -1, !dbg !1188
  br i1 %cmp137, label %if.then138, label %if.end150, !dbg !1189

if.then138:                                       ; preds = %if.else135
  %104 = load i32, ptr %TempNextChar, align 4, !dbg !1190
  %105 = load ptr, ptr %ipInF.addr, align 8, !dbg !1193
  %call139 = call i32 @ungetc(i32 noundef %104, ptr noundef %105), !dbg !1194
  %cmp140 = icmp eq i32 %call139, -1, !dbg !1195
  br i1 %cmp140, label %if.then141, label %if.end142, !dbg !1196

if.then141:                                       ; preds = %if.then138
  %106 = load ptr, ptr %ipFlag.addr, align 8, !dbg !1197
  %107 = load ptr, ptr %progname.addr, align 8, !dbg !1199
  call void @d2u_getc_error(ptr noundef %106, ptr noundef %107), !dbg !1200
  store i32 -1, ptr %RetVal, align 4, !dbg !1201
  br label %while.end165, !dbg !1202

if.end142:                                        ; preds = %if.then138
  %108 = load i32, ptr %TempNextChar, align 4, !dbg !1203
  %cmp143 = icmp eq i32 %108, 10, !dbg !1205
  br i1 %cmp143, label %if.then144, label %if.end149, !dbg !1206

if.then144:                                       ; preds = %if.end142
  %109 = load ptr, ptr %ipOutF.addr, align 8, !dbg !1207
  %call145 = call i32 @fputc(i32 noundef 13, ptr noundef %109), !dbg !1210
  %cmp146 = icmp eq i32 %call145, -1, !dbg !1211
  br i1 %cmp146, label %if.then147, label %if.end148, !dbg !1212

if.then147:                                       ; preds = %if.then144
  store i32 -1, ptr %RetVal, align 4, !dbg !1213
  %110 = load ptr, ptr %ipFlag.addr, align 8, !dbg !1215
  %111 = load ptr, ptr %progname.addr, align 8, !dbg !1216
  call void @d2u_putc_error(ptr noundef %110, ptr noundef %111), !dbg !1217
  br label %while.end165, !dbg !1218

if.end148:                                        ; preds = %if.then144
  %112 = load i32, ptr %TempChar, align 4, !dbg !1219
  store i32 %112, ptr %PrevChar, align 4, !dbg !1220
  br label %while.cond88, !dbg !1221, !llvm.loop !1222

if.end149:                                        ; preds = %if.end142
  br label %if.end150, !dbg !1224

if.end150:                                        ; preds = %if.end149, %if.else135
  %113 = load ptr, ptr %ipOutF.addr, align 8, !dbg !1225
  %call151 = call i32 @fputc(i32 noundef 10, ptr noundef %113), !dbg !1227
  %cmp152 = icmp eq i32 %call151, -1, !dbg !1228
  br i1 %cmp152, label %if.then153, label %if.end154, !dbg !1229

if.then153:                                       ; preds = %if.end150
  store i32 -1, ptr %RetVal, align 4, !dbg !1230
  %114 = load ptr, ptr %ipFlag.addr, align 8, !dbg !1232
  %115 = load ptr, ptr %progname.addr, align 8, !dbg !1233
  call void @d2u_putc_error(ptr noundef %114, ptr noundef %115), !dbg !1234
  br label %while.end165, !dbg !1235

if.end154:                                        ; preds = %if.end150
  %116 = load i32, ptr %converted, align 4, !dbg !1236
  %inc155 = add i32 %116, 1, !dbg !1236
  store i32 %inc155, ptr %converted, align 4, !dbg !1236
  %117 = load i32, ptr %line_nr, align 4, !dbg !1237
  %inc156 = add i32 %117, 1, !dbg !1237
  store i32 %inc156, ptr %line_nr, align 4, !dbg !1237
  %118 = load ptr, ptr %ipFlag.addr, align 8, !dbg !1238
  %NewLine = getelementptr inbounds %struct.CFlag, ptr %118, i32 0, i32 5, !dbg !1240
  %119 = load i32, ptr %NewLine, align 4, !dbg !1240
  %tobool157 = icmp ne i32 %119, 0, !dbg !1238
  br i1 %tobool157, label %if.then158, label %if.end163, !dbg !1241

if.then158:                                       ; preds = %if.end154
  %120 = load ptr, ptr %ipOutF.addr, align 8, !dbg !1242
  %call159 = call i32 @fputc(i32 noundef 10, ptr noundef %120), !dbg !1245
  %cmp160 = icmp eq i32 %call159, -1, !dbg !1246
  br i1 %cmp160, label %if.then161, label %if.end162, !dbg !1247

if.then161:                                       ; preds = %if.then158
  store i32 -1, ptr %RetVal, align 4, !dbg !1248
  %121 = load ptr, ptr %ipFlag.addr, align 8, !dbg !1250
  %122 = load ptr, ptr %progname.addr, align 8, !dbg !1251
  call void @d2u_putc_error(ptr noundef %121, ptr noundef %122), !dbg !1252
  br label %while.end165, !dbg !1253

if.end162:                                        ; preds = %if.then158
  br label %if.end163, !dbg !1254

if.end163:                                        ; preds = %if.end162, %if.end154
  br label %if.end164

if.end164:                                        ; preds = %if.end163, %if.end134
  %123 = load i32, ptr %TempChar, align 4, !dbg !1255
  store i32 %123, ptr %PrevChar, align 4, !dbg !1256
  br label %while.cond88, !dbg !1102, !llvm.loop !1222

while.end165:                                     ; preds = %if.then161, %if.then153, %if.then147, %if.then141, %if.then133, %if.end121, %while.cond88
  %124 = load i32, ptr %TempChar, align 4, !dbg !1257
  %cmp166 = icmp eq i32 %124, -1, !dbg !1259
  br i1 %cmp166, label %land.lhs.true167, label %if.end187, !dbg !1260

land.lhs.true167:                                 ; preds = %while.end165
  %125 = load ptr, ptr %ipFlag.addr, align 8, !dbg !1261
  %add_eol168 = getelementptr inbounds %struct.CFlag, ptr %125, i32 0, i32 20, !dbg !1262
  %126 = load i32, ptr %add_eol168, align 4, !dbg !1262
  %tobool169 = icmp ne i32 %126, 0, !dbg !1261
  br i1 %tobool169, label %land.lhs.true170, label %if.end187, !dbg !1263

land.lhs.true170:                                 ; preds = %land.lhs.true167
  %127 = load i32, ptr %PrevChar, align 4, !dbg !1264
  %cmp171 = icmp ne i32 %127, -1, !dbg !1265
  br i1 %cmp171, label %land.lhs.true172, label %if.end187, !dbg !1266

land.lhs.true172:                                 ; preds = %land.lhs.true170
  %128 = load i32, ptr %PrevChar, align 4, !dbg !1267
  %cmp173 = icmp eq i32 %128, 10, !dbg !1268
  br i1 %cmp173, label %if.end187, label %lor.lhs.false, !dbg !1269

lor.lhs.false:                                    ; preds = %land.lhs.true172
  %129 = load i32, ptr %PrevChar, align 4, !dbg !1270
  %cmp174 = icmp eq i32 %129, 13, !dbg !1271
  br i1 %cmp174, label %if.end187, label %if.then175, !dbg !1272

if.then175:                                       ; preds = %lor.lhs.false
  %130 = load ptr, ptr %ipFlag.addr, align 8, !dbg !1273
  %verbose176 = getelementptr inbounds %struct.CFlag, ptr %130, i32 0, i32 1, !dbg !1276
  %131 = load i32, ptr %verbose176, align 4, !dbg !1276
  %cmp177 = icmp sgt i32 %131, 1, !dbg !1277
  br i1 %cmp177, label %if.then178, label %if.end182, !dbg !1278

if.then178:                                       ; preds = %if.then175
  %132 = load ptr, ptr @stderr, align 8, !dbg !1279
  %133 = load ptr, ptr %progname.addr, align 8, !dbg !1281
  %call179 = call i32 (ptr, ptr, ...) @fprintf(ptr noundef %132, ptr noundef @.str.1, ptr noundef %133), !dbg !1282
  %134 = load ptr, ptr @stderr, align 8, !dbg !1283
  %call180 = call ptr @gettext(ptr noundef @.str.3) #7, !dbg !1284
  %call181 = call i32 (ptr, ptr, ...) @fprintf(ptr noundef %134, ptr noundef %call180), !dbg !1285
  br label %if.end182, !dbg !1286

if.end182:                                        ; preds = %if.then178, %if.then175
  %135 = load ptr, ptr %ipOutF.addr, align 8, !dbg !1287
  %call183 = call i32 @fputc(i32 noundef 10, ptr noundef %135), !dbg !1289
  %cmp184 = icmp eq i32 %call183, -1, !dbg !1290
  br i1 %cmp184, label %if.then185, label %if.end186, !dbg !1291

if.then185:                                       ; preds = %if.end182
  store i32 -1, ptr %RetVal, align 4, !dbg !1292
  %136 = load ptr, ptr %ipFlag.addr, align 8, !dbg !1294
  %137 = load ptr, ptr %progname.addr, align 8, !dbg !1295
  call void @d2u_putc_error(ptr noundef %136, ptr noundef %137), !dbg !1296
  br label %if.end186, !dbg !1297

if.end186:                                        ; preds = %if.then185, %if.end182
  br label %if.end187, !dbg !1298

if.end187:                                        ; preds = %if.end186, %lor.lhs.false, %land.lhs.true172, %land.lhs.true170, %land.lhs.true167, %while.end165
  %138 = load i32, ptr %TempChar, align 4, !dbg !1299
  %cmp188 = icmp eq i32 %138, -1, !dbg !1301
  br i1 %cmp188, label %land.lhs.true189, label %if.end193, !dbg !1302

land.lhs.true189:                                 ; preds = %if.end187
  %139 = load ptr, ptr %ipInF.addr, align 8, !dbg !1303
  %call190 = call i32 @ferror(ptr noundef %139) #7, !dbg !1304
  %tobool191 = icmp ne i32 %call190, 0, !dbg !1304
  br i1 %tobool191, label %if.then192, label %if.end193, !dbg !1305

if.then192:                                       ; preds = %land.lhs.true189
  store i32 -1, ptr %RetVal, align 4, !dbg !1306
  %140 = load ptr, ptr %ipFlag.addr, align 8, !dbg !1308
  %141 = load ptr, ptr %progname.addr, align 8, !dbg !1309
  call void @d2u_getc_error(ptr noundef %140, ptr noundef %141), !dbg !1310
  br label %if.end193, !dbg !1311

if.end193:                                        ; preds = %if.then192, %land.lhs.true189, %if.end187
  br label %sw.epilog195, !dbg !1312

sw.default194:                                    ; preds = %if.end15
  br label %sw.epilog195, !dbg !1313

sw.epilog195:                                     ; preds = %sw.default194, %if.end193, %if.end86
  %142 = load i32, ptr %RetVal, align 4, !dbg !1314
  %cmp196 = icmp eq i32 %142, 0, !dbg !1316
  br i1 %cmp196, label %land.lhs.true197, label %if.end204, !dbg !1317

land.lhs.true197:                                 ; preds = %sw.epilog195
  %143 = load ptr, ptr %ipFlag.addr, align 8, !dbg !1318
  %verbose198 = getelementptr inbounds %struct.CFlag, ptr %143, i32 0, i32 1, !dbg !1319
  %144 = load i32, ptr %verbose198, align 4, !dbg !1319
  %cmp199 = icmp sgt i32 %144, 1, !dbg !1320
  br i1 %cmp199, label %if.then200, label %if.end204, !dbg !1321

if.then200:                                       ; preds = %land.lhs.true197
  %145 = load ptr, ptr @stderr, align 8, !dbg !1322
  %146 = load ptr, ptr %progname.addr, align 8, !dbg !1324
  %call201 = call i32 (ptr, ptr, ...) @fprintf(ptr noundef %145, ptr noundef @.str.1, ptr noundef %146), !dbg !1325
  %147 = load ptr, ptr @stderr, align 8, !dbg !1326
  %call202 = call ptr @gettext(ptr noundef @.str.4) #7, !dbg !1327
  %148 = load i32, ptr %converted, align 4, !dbg !1328
  %149 = load i32, ptr %line_nr, align 4, !dbg !1329
  %sub = sub i32 %149, 1, !dbg !1330
  %call203 = call i32 (ptr, ptr, ...) @fprintf(ptr noundef %147, ptr noundef %call202, i32 noundef %148, i32 noundef %sub), !dbg !1331
  br label %if.end204, !dbg !1332

if.end204:                                        ; preds = %if.then200, %land.lhs.true197, %sw.epilog195
  %150 = load i32, ptr %RetVal, align 4, !dbg !1333
  store i32 %150, ptr %retval, align 4, !dbg !1334
  br label %return, !dbg !1334

return:                                           ; preds = %if.end204, %sw.default
  %151 = load i32, ptr %retval, align 4, !dbg !1335
  ret i32 %151, !dbg !1335
}

; Function Attrs: noinline nounwind uwtable
define dso_local i32 @main(i32 noundef %argc, ptr noundef %argv) #0 !dbg !1336 {
entry:
  %retval = alloca i32, align 4
  %argc.addr = alloca i32, align 4
  %argv.addr = alloca ptr, align 8
  %progname = alloca [9 x i8], align 1
  %pFlag = alloca ptr, align 8
  %ptr = alloca ptr, align 8
  %localedir = alloca [1024 x i8], align 16
  %ret = alloca i32, align 4
  %argc_new = alloca i32, align 4
  %argv_new = alloca ptr, align 8
  store i32 0, ptr %retval, align 4
  store i32 %argc, ptr %argc.addr, align 4
  call void @llvm.dbg.declare(metadata ptr %argc.addr, metadata !1340, metadata !DIExpression()), !dbg !1341
  store ptr %argv, ptr %argv.addr, align 8
  call void @llvm.dbg.declare(metadata ptr %argv.addr, metadata !1342, metadata !DIExpression()), !dbg !1343
  call void @llvm.dbg.declare(metadata ptr %progname, metadata !1344, metadata !DIExpression()), !dbg !1345
  call void @llvm.dbg.declare(metadata ptr %pFlag, metadata !1346, metadata !DIExpression()), !dbg !1347
  call void @llvm.dbg.declare(metadata ptr %ptr, metadata !1348, metadata !DIExpression()), !dbg !1349
  call void @llvm.dbg.declare(metadata ptr %localedir, metadata !1350, metadata !DIExpression()), !dbg !1354
  call void @llvm.dbg.declare(metadata ptr %ret, metadata !1355, metadata !DIExpression()), !dbg !1356
  call void @llvm.dbg.declare(metadata ptr %argc_new, metadata !1357, metadata !DIExpression()), !dbg !1358
  call void @llvm.dbg.declare(metadata ptr %argv_new, metadata !1359, metadata !DIExpression()), !dbg !1360
  %arrayidx = getelementptr inbounds [9 x i8], ptr %progname, i64 0, i64 8, !dbg !1361
  store i8 0, ptr %arrayidx, align 1, !dbg !1362
  %arraydecay = getelementptr inbounds [9 x i8], ptr %progname, i64 0, i64 0, !dbg !1363
  %call = call ptr @strcpy(ptr noundef %arraydecay, ptr noundef @.str.7) #7, !dbg !1364
  %call1 = call ptr @getenv(ptr noundef @.str.8) #7, !dbg !1365
  store ptr %call1, ptr %ptr, align 8, !dbg !1366
  %0 = load ptr, ptr %ptr, align 8, !dbg !1367
  %cmp = icmp eq ptr %0, null, !dbg !1369
  br i1 %cmp, label %if.then, label %if.else, !dbg !1370

if.then:                                          ; preds = %entry
  %arraydecay2 = getelementptr inbounds [1024 x i8], ptr %localedir, i64 0, i64 0, !dbg !1371
  %call3 = call ptr @d2u_strncpy(ptr noundef %arraydecay2, ptr noundef @.str.9, i64 noundef 1024), !dbg !1372
  br label %if.end16, !dbg !1372

if.else:                                          ; preds = %entry
  %1 = load ptr, ptr %ptr, align 8, !dbg !1373
  %call4 = call i64 @strlen(ptr noundef %1) #8, !dbg !1376
  %cmp5 = icmp ult i64 %call4, 1024, !dbg !1377
  br i1 %cmp5, label %if.then6, label %if.else9, !dbg !1378

if.then6:                                         ; preds = %if.else
  %arraydecay7 = getelementptr inbounds [1024 x i8], ptr %localedir, i64 0, i64 0, !dbg !1379
  %2 = load ptr, ptr %ptr, align 8, !dbg !1380
  %call8 = call ptr @d2u_strncpy(ptr noundef %arraydecay7, ptr noundef %2, i64 noundef 1024), !dbg !1381
  br label %if.end, !dbg !1381

if.else9:                                         ; preds = %if.else
  %3 = load ptr, ptr @stderr, align 8, !dbg !1382
  %arraydecay10 = getelementptr inbounds [9 x i8], ptr %progname, i64 0, i64 0, !dbg !1384
  %call11 = call i32 (ptr, ptr, ...) @fprintf(ptr noundef %3, ptr noundef @.str.1, ptr noundef %arraydecay10), !dbg !1385
  %4 = load ptr, ptr @stderr, align 8, !dbg !1386
  %call12 = call ptr @gettext(ptr noundef @.str.11) #7, !dbg !1387
  %call13 = call i32 (ptr, ptr, ...) @fprintf(ptr noundef %4, ptr noundef @.str.10, ptr noundef %call12), !dbg !1388
  %arraydecay14 = getelementptr inbounds [1024 x i8], ptr %localedir, i64 0, i64 0, !dbg !1389
  %call15 = call ptr @d2u_strncpy(ptr noundef %arraydecay14, ptr noundef @.str.9, i64 noundef 1024), !dbg !1390
  br label %if.end

if.end:                                           ; preds = %if.else9, %if.then6
  br label %if.end16

if.end16:                                         ; preds = %if.end, %if.then
  %call17 = call ptr @setlocale(i32 noundef 6, ptr noundef @.str.12) #7, !dbg !1391
  %arraydecay18 = getelementptr inbounds [1024 x i8], ptr %localedir, i64 0, i64 0, !dbg !1392
  %call19 = call ptr @bindtextdomain(ptr noundef @.str.7, ptr noundef %arraydecay18) #7, !dbg !1393
  %call20 = call ptr @textdomain(ptr noundef @.str.7) #7, !dbg !1394
  %call21 = call noalias ptr @malloc(i64 noundef 84) #9, !dbg !1395
  store ptr %call21, ptr %pFlag, align 8, !dbg !1396
  %5 = load ptr, ptr %pFlag, align 8, !dbg !1397
  %cmp22 = icmp eq ptr %5, null, !dbg !1399
  br i1 %cmp22, label %if.then23, label %if.end29, !dbg !1400

if.then23:                                        ; preds = %if.end16
  %6 = load ptr, ptr @stderr, align 8, !dbg !1401
  %call24 = call i32 (ptr, ptr, ...) @fprintf(ptr noundef %6, ptr noundef @.str.13), !dbg !1403
  %7 = load ptr, ptr @stderr, align 8, !dbg !1404
  %call25 = call ptr @__errno_location() #10, !dbg !1405
  %8 = load i32, ptr %call25, align 4, !dbg !1405
  %call26 = call ptr @strerror(i32 noundef %8) #7, !dbg !1406
  %call27 = call i32 (ptr, ptr, ...) @fprintf(ptr noundef %7, ptr noundef @.str.14, ptr noundef %call26), !dbg !1407
  %call28 = call ptr @__errno_location() #10, !dbg !1408
  %9 = load i32, ptr %call28, align 4, !dbg !1408
  store i32 %9, ptr %retval, align 4, !dbg !1409
  br label %return, !dbg !1409

if.end29:                                         ; preds = %if.end16
  %10 = load ptr, ptr %pFlag, align 8, !dbg !1410
  %FromToMode = getelementptr inbounds %struct.CFlag, ptr %10, i32 0, i32 4, !dbg !1411
  store i32 0, ptr %FromToMode, align 4, !dbg !1412
  %11 = load ptr, ptr %pFlag, align 8, !dbg !1413
  %keep_bom = getelementptr inbounds %struct.CFlag, ptr %11, i32 0, i32 15, !dbg !1414
  store i32 0, ptr %keep_bom, align 4, !dbg !1415
  %12 = load ptr, ptr %argv.addr, align 8, !dbg !1416
  %arrayidx30 = getelementptr inbounds ptr, ptr %12, i64 0, !dbg !1416
  %13 = load ptr, ptr %arrayidx30, align 8, !dbg !1416
  %call31 = call ptr @strrchr(ptr noundef %13, i32 noundef 47) #8, !dbg !1418
  store ptr %call31, ptr %ptr, align 8, !dbg !1419
  %cmp32 = icmp eq ptr %call31, null, !dbg !1420
  br i1 %cmp32, label %land.lhs.true, label %if.else38, !dbg !1421

land.lhs.true:                                    ; preds = %if.end29
  %14 = load ptr, ptr %argv.addr, align 8, !dbg !1422
  %arrayidx33 = getelementptr inbounds ptr, ptr %14, i64 0, !dbg !1422
  %15 = load ptr, ptr %arrayidx33, align 8, !dbg !1422
  %call34 = call ptr @strrchr(ptr noundef %15, i32 noundef 92) #8, !dbg !1423
  store ptr %call34, ptr %ptr, align 8, !dbg !1424
  %cmp35 = icmp eq ptr %call34, null, !dbg !1425
  br i1 %cmp35, label %if.then36, label %if.else38, !dbg !1426

if.then36:                                        ; preds = %land.lhs.true
  %16 = load ptr, ptr %argv.addr, align 8, !dbg !1427
  %arrayidx37 = getelementptr inbounds ptr, ptr %16, i64 0, !dbg !1427
  %17 = load ptr, ptr %arrayidx37, align 8, !dbg !1427
  store ptr %17, ptr %ptr, align 8, !dbg !1428
  br label %if.end39, !dbg !1429

if.else38:                                        ; preds = %land.lhs.true, %if.end29
  %18 = load ptr, ptr %ptr, align 8, !dbg !1430
  %incdec.ptr = getelementptr inbounds i8, ptr %18, i32 1, !dbg !1430
  store ptr %incdec.ptr, ptr %ptr, align 8, !dbg !1430
  br label %if.end39

if.end39:                                         ; preds = %if.else38, %if.then36
  %19 = load ptr, ptr %ptr, align 8, !dbg !1431
  %call40 = call i32 @strcasecmp(ptr noundef @.str.15, ptr noundef %19) #8, !dbg !1431
  %cmp41 = icmp eq i32 %call40, 0, !dbg !1433
  br i1 %cmp41, label %if.then44, label %lor.lhs.false, !dbg !1434

lor.lhs.false:                                    ; preds = %if.end39
  %20 = load ptr, ptr %ptr, align 8, !dbg !1435
  %call42 = call i32 @strcasecmp(ptr noundef @.str.16, ptr noundef %20) #8, !dbg !1435
  %cmp43 = icmp eq i32 %call42, 0, !dbg !1436
  br i1 %cmp43, label %if.then44, label %if.end48, !dbg !1437

if.then44:                                        ; preds = %lor.lhs.false, %if.end39
  %21 = load ptr, ptr %pFlag, align 8, !dbg !1438
  %FromToMode45 = getelementptr inbounds %struct.CFlag, ptr %21, i32 0, i32 4, !dbg !1440
  store i32 1, ptr %FromToMode45, align 4, !dbg !1441
  %arraydecay46 = getelementptr inbounds [9 x i8], ptr %progname, i64 0, i64 0, !dbg !1442
  %call47 = call ptr @strcpy(ptr noundef %arraydecay46, ptr noundef @.str.15) #7, !dbg !1443
  br label %if.end48, !dbg !1444

if.end48:                                         ; preds = %if.then44, %lor.lhs.false
  %22 = load i32, ptr %argc.addr, align 4, !dbg !1445
  store i32 %22, ptr %argc_new, align 4, !dbg !1446
  %23 = load ptr, ptr %argv.addr, align 8, !dbg !1447
  store ptr %23, ptr %argv_new, align 8, !dbg !1448
  %24 = load i32, ptr %argc_new, align 4, !dbg !1449
  %25 = load ptr, ptr %argv_new, align 8, !dbg !1450
  %26 = load ptr, ptr %pFlag, align 8, !dbg !1451
  %arraydecay49 = getelementptr inbounds [1024 x i8], ptr %localedir, i64 0, i64 0, !dbg !1452
  %arraydecay50 = getelementptr inbounds [9 x i8], ptr %progname, i64 0, i64 0, !dbg !1453
  %call51 = call i32 @parse_options(i32 noundef %24, ptr noundef %25, ptr noundef %26, ptr noundef %arraydecay49, ptr noundef %arraydecay50, ptr noundef @PrintLicense, ptr noundef @ConvertDosToUnix, ptr noundef @ConvertDosToUnixW), !dbg !1454
  store i32 %call51, ptr %ret, align 4, !dbg !1455
  %27 = load ptr, ptr %pFlag, align 8, !dbg !1456
  call void @free(ptr noundef %27) #7, !dbg !1457
  %28 = load i32, ptr %ret, align 4, !dbg !1458
  store i32 %28, ptr %retval, align 4, !dbg !1459
  br label %return, !dbg !1459

return:                                           ; preds = %if.end48, %if.then23
  %29 = load i32, ptr %retval, align 4, !dbg !1460
  ret i32 %29, !dbg !1460
}

; Function Attrs: nounwind
declare ptr @strcpy(ptr noundef, ptr noundef) #2

; Function Attrs: nounwind
declare ptr @getenv(ptr noundef) #2

declare ptr @d2u_strncpy(ptr noundef, ptr noundef, i64 noundef) #1

; Function Attrs: nounwind readonly willreturn
declare i64 @strlen(ptr noundef) #4

; Function Attrs: nounwind
declare ptr @setlocale(i32 noundef, ptr noundef) #2

; Function Attrs: nounwind
declare ptr @bindtextdomain(ptr noundef, ptr noundef) #2

; Function Attrs: nounwind
declare ptr @textdomain(ptr noundef) #2

; Function Attrs: nounwind allocsize(0)
declare noalias ptr @malloc(i64 noundef) #5

; Function Attrs: nounwind
declare ptr @strerror(i32 noundef) #2

; Function Attrs: nounwind readnone willreturn
declare ptr @__errno_location() #6

; Function Attrs: nounwind readonly willreturn
declare ptr @strrchr(ptr noundef, i32 noundef) #4

; Function Attrs: nounwind readonly willreturn
declare i32 @strcasecmp(ptr noundef, ptr noundef) #4

declare i32 @parse_options(i32 noundef, ptr noundef, ptr noundef, ptr noundef, ptr noundef, ptr noundef, ptr noundef, ptr noundef) #1

; Function Attrs: nounwind
declare void @free(ptr noundef) #2

attributes #0 = { noinline nounwind uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #1 = { "frame-pointer"="all" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #2 = { nounwind "frame-pointer"="all" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #3 = { nocallback nofree nosync nounwind readnone speculatable willreturn }
attributes #4 = { nounwind readonly willreturn "frame-pointer"="all" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #5 = { nounwind allocsize(0) "frame-pointer"="all" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #6 = { nounwind readnone willreturn "frame-pointer"="all" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #7 = { nounwind }
attributes #8 = { nounwind readonly willreturn }
attributes #9 = { nounwind allocsize(0) }
attributes #10 = { nounwind readnone willreturn }

!llvm.dbg.cu = !{!29}
!llvm.module.flags = !{!133, !134, !135, !136, !137, !138, !139}
!llvm.ident = !{!140}

!0 = !DIGlobalVariableExpression(var: !1, expr: !DIExpression())
!1 = distinct !DIGlobalVariable(scope: null, file: !2, line: 79, type: !3, isLocal: true, isDefinition: true)
!2 = !DIFile(filename: "dos2unix.c", directory: "/datasets/dos2unix-7.5.2", checksumkind: CSK_MD5, checksum: "417b0dd75ef40c9a0f817583b776dfa5")
!3 = !DICompositeType(tag: DW_TAG_array_type, baseType: !4, size: 1720, elements: !5)
!4 = !DIBasicType(name: "char", size: 8, encoding: DW_ATE_signed_char)
!5 = !{!6}
!6 = !DISubrange(count: 215)
!7 = !DIGlobalVariableExpression(var: !8, expr: !DIExpression())
!8 = distinct !DIGlobalVariable(scope: null, file: !2, line: 212, type: !9, isLocal: true, isDefinition: true)
!9 = !DICompositeType(tag: DW_TAG_array_type, baseType: !4, size: 40, elements: !10)
!10 = !{!11}
!11 = !DISubrange(count: 5)
!12 = !DIGlobalVariableExpression(var: !13, expr: !DIExpression())
!13 = distinct !DIGlobalVariable(scope: null, file: !2, line: 213, type: !14, isLocal: true, isDefinition: true)
!14 = !DICompositeType(tag: DW_TAG_array_type, baseType: !4, size: 328, elements: !15)
!15 = !{!16}
!16 = !DISubrange(count: 41)
!17 = !DIGlobalVariableExpression(var: !18, expr: !DIExpression())
!18 = distinct !DIGlobalVariable(scope: null, file: !2, line: 237, type: !19, isLocal: true, isDefinition: true)
!19 = !DICompositeType(tag: DW_TAG_array_type, baseType: !4, size: 256, elements: !20)
!20 = !{!21}
!21 = !DISubrange(count: 32)
!22 = !DIGlobalVariableExpression(var: !23, expr: !DIExpression())
!23 = distinct !DIGlobalVariable(scope: null, file: !2, line: 338, type: !24, isLocal: true, isDefinition: true)
!24 = !DICompositeType(tag: DW_TAG_array_type, baseType: !4, size: 296, elements: !25)
!25 = !{!26}
!26 = !DISubrange(count: 37)
!27 = !DIGlobalVariableExpression(var: !28, expr: !DIExpression())
!28 = distinct !DIGlobalVariable(name: "D2UAsciiTable", scope: !29, file: !117, line: 34, type: !118, isLocal: true, isDefinition: true)
!29 = distinct !DICompileUnit(language: DW_LANG_C99, file: !2, producer: "Ubuntu clang version 15.0.7", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, retainedTypes: !30, globals: !60, splitDebugInlining: false, nameTableKind: None)
!30 = !{!31, !32}
!31 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: null, size: 64)
!32 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !33, size: 64)
!33 = !DIDerivedType(tag: DW_TAG_typedef, name: "CFlag", file: !34, line: 253, baseType: !35)
!34 = !DIFile(filename: "./common.h", directory: "/datasets/dos2unix-7.5.2", checksumkind: CSK_MD5, checksum: "26bc2c6b37c09e44e591f80b29ca9daf")
!35 = distinct !DICompositeType(tag: DW_TAG_structure_type, file: !34, line: 230, size: 672, elements: !36)
!36 = !{!37, !39, !40, !41, !42, !43, !44, !45, !46, !47, !48, !49, !50, !51, !52, !53, !54, !55, !56, !57, !59}
!37 = !DIDerivedType(tag: DW_TAG_member, name: "NewFile", scope: !35, file: !34, line: 232, baseType: !38, size: 32)
!38 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!39 = !DIDerivedType(tag: DW_TAG_member, name: "verbose", scope: !35, file: !34, line: 233, baseType: !38, size: 32, offset: 32)
!40 = !DIDerivedType(tag: DW_TAG_member, name: "KeepDate", scope: !35, file: !34, line: 234, baseType: !38, size: 32, offset: 64)
!41 = !DIDerivedType(tag: DW_TAG_member, name: "ConvMode", scope: !35, file: !34, line: 235, baseType: !38, size: 32, offset: 96)
!42 = !DIDerivedType(tag: DW_TAG_member, name: "FromToMode", scope: !35, file: !34, line: 236, baseType: !38, size: 32, offset: 128)
!43 = !DIDerivedType(tag: DW_TAG_member, name: "NewLine", scope: !35, file: !34, line: 237, baseType: !38, size: 32, offset: 160)
!44 = !DIDerivedType(tag: DW_TAG_member, name: "Force", scope: !35, file: !34, line: 238, baseType: !38, size: 32, offset: 192)
!45 = !DIDerivedType(tag: DW_TAG_member, name: "AllowChown", scope: !35, file: !34, line: 239, baseType: !38, size: 32, offset: 224)
!46 = !DIDerivedType(tag: DW_TAG_member, name: "Follow", scope: !35, file: !34, line: 240, baseType: !38, size: 32, offset: 256)
!47 = !DIDerivedType(tag: DW_TAG_member, name: "status", scope: !35, file: !34, line: 241, baseType: !38, size: 32, offset: 288)
!48 = !DIDerivedType(tag: DW_TAG_member, name: "stdio_mode", scope: !35, file: !34, line: 242, baseType: !38, size: 32, offset: 320)
!49 = !DIDerivedType(tag: DW_TAG_member, name: "to_stdout", scope: !35, file: !34, line: 243, baseType: !38, size: 32, offset: 352)
!50 = !DIDerivedType(tag: DW_TAG_member, name: "error", scope: !35, file: !34, line: 244, baseType: !38, size: 32, offset: 384)
!51 = !DIDerivedType(tag: DW_TAG_member, name: "bomtype", scope: !35, file: !34, line: 245, baseType: !38, size: 32, offset: 416)
!52 = !DIDerivedType(tag: DW_TAG_member, name: "add_bom", scope: !35, file: !34, line: 246, baseType: !38, size: 32, offset: 448)
!53 = !DIDerivedType(tag: DW_TAG_member, name: "keep_bom", scope: !35, file: !34, line: 247, baseType: !38, size: 32, offset: 480)
!54 = !DIDerivedType(tag: DW_TAG_member, name: "keep_utf16", scope: !35, file: !34, line: 248, baseType: !38, size: 32, offset: 512)
!55 = !DIDerivedType(tag: DW_TAG_member, name: "file_info", scope: !35, file: !34, line: 249, baseType: !38, size: 32, offset: 544)
!56 = !DIDerivedType(tag: DW_TAG_member, name: "locale_target", scope: !35, file: !34, line: 250, baseType: !38, size: 32, offset: 576)
!57 = !DIDerivedType(tag: DW_TAG_member, name: "line_nr", scope: !35, file: !34, line: 251, baseType: !58, size: 32, offset: 608)
!58 = !DIBasicType(name: "unsigned int", size: 32, encoding: DW_ATE_unsigned)
!59 = !DIDerivedType(tag: DW_TAG_member, name: "add_eol", scope: !35, file: !34, line: 252, baseType: !38, size: 32, offset: 640)
!60 = !{!0, !7, !12, !17, !22, !61, !66, !71, !76, !81, !86, !91, !96, !101, !106, !108, !110, !27, !115, !121, !123, !125, !127, !129, !131}
!61 = !DIGlobalVariableExpression(var: !62, expr: !DIExpression())
!62 = distinct !DIGlobalVariable(scope: null, file: !2, line: 397, type: !63, isLocal: true, isDefinition: true)
!63 = !DICompositeType(tag: DW_TAG_array_type, baseType: !4, size: 168, elements: !64)
!64 = !{!65}
!65 = !DISubrange(count: 21)
!66 = !DIGlobalVariableExpression(var: !67, expr: !DIExpression())
!67 = distinct !DIGlobalVariable(scope: null, file: !2, line: 421, type: !68, isLocal: true, isDefinition: true)
!68 = !DICompositeType(tag: DW_TAG_array_type, baseType: !4, size: 312, elements: !69)
!69 = !{!70}
!70 = !DISubrange(count: 39)
!71 = !DIGlobalVariableExpression(var: !72, expr: !DIExpression())
!72 = distinct !DIGlobalVariable(scope: null, file: !2, line: 569, type: !73, isLocal: true, isDefinition: true)
!73 = !DICompositeType(tag: DW_TAG_array_type, baseType: !4, size: 72, elements: !74)
!74 = !{!75}
!75 = !DISubrange(count: 9)
!76 = !DIGlobalVariableExpression(var: !77, expr: !DIExpression())
!77 = distinct !DIGlobalVariable(scope: null, file: !2, line: 572, type: !78, isLocal: true, isDefinition: true)
!78 = !DICompositeType(tag: DW_TAG_array_type, baseType: !4, size: 152, elements: !79)
!79 = !{!80}
!80 = !DISubrange(count: 19)
!81 = !DIGlobalVariableExpression(var: !82, expr: !DIExpression())
!82 = distinct !DIGlobalVariable(scope: null, file: !2, line: 574, type: !83, isLocal: true, isDefinition: true)
!83 = !DICompositeType(tag: DW_TAG_array_type, baseType: !4, size: 144, elements: !84)
!84 = !{!85}
!85 = !DISubrange(count: 18)
!86 = !DIGlobalVariableExpression(var: !87, expr: !DIExpression())
!87 = distinct !DIGlobalVariable(scope: null, file: !2, line: 580, type: !88, isLocal: true, isDefinition: true)
!88 = !DICompositeType(tag: DW_TAG_array_type, baseType: !4, size: 24, elements: !89)
!89 = !{!90}
!90 = !DISubrange(count: 3)
!91 = !DIGlobalVariableExpression(var: !92, expr: !DIExpression())
!92 = distinct !DIGlobalVariable(scope: null, file: !2, line: 580, type: !93, isLocal: true, isDefinition: true)
!93 = !DICompositeType(tag: DW_TAG_array_type, baseType: !4, size: 560, elements: !94)
!94 = !{!95}
!95 = !DISubrange(count: 70)
!96 = !DIGlobalVariableExpression(var: !97, expr: !DIExpression())
!97 = distinct !DIGlobalVariable(scope: null, file: !2, line: 595, type: !98, isLocal: true, isDefinition: true)
!98 = !DICompositeType(tag: DW_TAG_array_type, baseType: !4, size: 8, elements: !99)
!99 = !{!100}
!100 = !DISubrange(count: 1)
!101 = !DIGlobalVariableExpression(var: !102, expr: !DIExpression())
!102 = distinct !DIGlobalVariable(scope: null, file: !2, line: 608, type: !103, isLocal: true, isDefinition: true)
!103 = !DICompositeType(tag: DW_TAG_array_type, baseType: !4, size: 80, elements: !104)
!104 = !{!105}
!105 = !DISubrange(count: 10)
!106 = !DIGlobalVariableExpression(var: !107, expr: !DIExpression())
!107 = distinct !DIGlobalVariable(scope: null, file: !2, line: 609, type: !9, isLocal: true, isDefinition: true)
!108 = !DIGlobalVariableExpression(var: !109, expr: !DIExpression())
!109 = distinct !DIGlobalVariable(scope: null, file: !2, line: 620, type: !73, isLocal: true, isDefinition: true)
!110 = !DIGlobalVariableExpression(var: !111, expr: !DIExpression())
!111 = distinct !DIGlobalVariable(scope: null, file: !2, line: 620, type: !112, isLocal: true, isDefinition: true)
!112 = !DICompositeType(tag: DW_TAG_array_type, baseType: !4, size: 104, elements: !113)
!113 = !{!114}
!114 = !DISubrange(count: 13)
!115 = !DIGlobalVariableExpression(var: !116, expr: !DIExpression())
!116 = distinct !DIGlobalVariable(name: "D2U7BitTable", scope: !29, file: !117, line: 58, type: !118, isLocal: true, isDefinition: true)
!117 = !DIFile(filename: "./dos2unix.h", directory: "/datasets/dos2unix-7.5.2", checksumkind: CSK_MD5, checksum: "e246759484e3e4f5e348ba4412f7c13c")
!118 = !DICompositeType(tag: DW_TAG_array_type, baseType: !38, size: 8192, elements: !119)
!119 = !{!120}
!120 = !DISubrange(count: 256)
!121 = !DIGlobalVariableExpression(var: !122, expr: !DIExpression())
!122 = distinct !DIGlobalVariable(name: "D2UIso437Table", scope: !29, file: !117, line: 82, type: !118, isLocal: true, isDefinition: true)
!123 = !DIGlobalVariableExpression(var: !124, expr: !DIExpression())
!124 = distinct !DIGlobalVariable(name: "D2UIso850Table", scope: !29, file: !117, line: 105, type: !118, isLocal: true, isDefinition: true)
!125 = !DIGlobalVariableExpression(var: !126, expr: !DIExpression())
!126 = distinct !DIGlobalVariable(name: "D2UIso860Table", scope: !29, file: !117, line: 128, type: !118, isLocal: true, isDefinition: true)
!127 = !DIGlobalVariableExpression(var: !128, expr: !DIExpression())
!128 = distinct !DIGlobalVariable(name: "D2UIso863Table", scope: !29, file: !117, line: 151, type: !118, isLocal: true, isDefinition: true)
!129 = !DIGlobalVariableExpression(var: !130, expr: !DIExpression())
!130 = distinct !DIGlobalVariable(name: "D2UIso865Table", scope: !29, file: !117, line: 174, type: !118, isLocal: true, isDefinition: true)
!131 = !DIGlobalVariableExpression(var: !132, expr: !DIExpression())
!132 = distinct !DIGlobalVariable(name: "D2UIso1252Table", scope: !29, file: !117, line: 197, type: !118, isLocal: true, isDefinition: true)
!133 = !{i32 7, !"Dwarf Version", i32 5}
!134 = !{i32 2, !"Debug Info Version", i32 3}
!135 = !{i32 1, !"wchar_size", i32 4}
!136 = !{i32 7, !"PIC Level", i32 2}
!137 = !{i32 7, !"PIE Level", i32 2}
!138 = !{i32 7, !"uwtable", i32 2}
!139 = !{i32 7, !"frame-pointer", i32 2}
!140 = !{!"Ubuntu clang version 15.0.7"}
!141 = distinct !DISubprogram(name: "PrintLicense", scope: !2, file: !2, line: 77, type: !142, scopeLine: 78, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !29, retainedNodes: !144)
!142 = !DISubroutineType(types: !143)
!143 = !{null}
!144 = !{}
!145 = !DILocation(line: 79, column: 20, scope: !141)
!146 = !DILocation(line: 79, column: 27, scope: !141)
!147 = !DILocation(line: 79, column: 3, scope: !141)
!148 = !DILocation(line: 85, column: 3, scope: !141)
!149 = !DILocation(line: 86, column: 1, scope: !141)
!150 = distinct !DISubprogram(name: "StripDelimiterW", scope: !2, file: !2, line: 89, type: !151, scopeLine: 90, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !29, retainedNodes: !144)
!151 = !DISubroutineType(types: !152)
!152 = !{!153, !155, !155, !32, !153, !212, !213}
!153 = !DIDerivedType(tag: DW_TAG_typedef, name: "wint_t", file: !154, line: 20, baseType: !58)
!154 = !DIFile(filename: "/usr/include/x86_64-linux-gnu/bits/types/wint_t.h", directory: "", checksumkind: CSK_MD5, checksum: "aa31b53ef28dc23152ceb41e2763ded3")
!155 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !156, size: 64)
!156 = !DIDerivedType(tag: DW_TAG_typedef, name: "FILE", file: !157, line: 7, baseType: !158)
!157 = !DIFile(filename: "/usr/include/x86_64-linux-gnu/bits/types/FILE.h", directory: "", checksumkind: CSK_MD5, checksum: "571f9fb6223c42439075fdde11a0de5d")
!158 = distinct !DICompositeType(tag: DW_TAG_structure_type, name: "_IO_FILE", file: !159, line: 49, size: 1728, elements: !160)
!159 = !DIFile(filename: "/usr/include/x86_64-linux-gnu/bits/types/struct_FILE.h", directory: "", checksumkind: CSK_MD5, checksum: "7a6d4a00a37ee6b9a40cd04bd01f5d00")
!160 = !{!161, !162, !164, !165, !166, !167, !168, !169, !170, !171, !172, !173, !174, !177, !179, !180, !181, !185, !187, !189, !190, !193, !195, !198, !201, !202, !203, !207, !208}
!161 = !DIDerivedType(tag: DW_TAG_member, name: "_flags", scope: !158, file: !159, line: 51, baseType: !38, size: 32)
!162 = !DIDerivedType(tag: DW_TAG_member, name: "_IO_read_ptr", scope: !158, file: !159, line: 54, baseType: !163, size: 64, offset: 64)
!163 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !4, size: 64)
!164 = !DIDerivedType(tag: DW_TAG_member, name: "_IO_read_end", scope: !158, file: !159, line: 55, baseType: !163, size: 64, offset: 128)
!165 = !DIDerivedType(tag: DW_TAG_member, name: "_IO_read_base", scope: !158, file: !159, line: 56, baseType: !163, size: 64, offset: 192)
!166 = !DIDerivedType(tag: DW_TAG_member, name: "_IO_write_base", scope: !158, file: !159, line: 57, baseType: !163, size: 64, offset: 256)
!167 = !DIDerivedType(tag: DW_TAG_member, name: "_IO_write_ptr", scope: !158, file: !159, line: 58, baseType: !163, size: 64, offset: 320)
!168 = !DIDerivedType(tag: DW_TAG_member, name: "_IO_write_end", scope: !158, file: !159, line: 59, baseType: !163, size: 64, offset: 384)
!169 = !DIDerivedType(tag: DW_TAG_member, name: "_IO_buf_base", scope: !158, file: !159, line: 60, baseType: !163, size: 64, offset: 448)
!170 = !DIDerivedType(tag: DW_TAG_member, name: "_IO_buf_end", scope: !158, file: !159, line: 61, baseType: !163, size: 64, offset: 512)
!171 = !DIDerivedType(tag: DW_TAG_member, name: "_IO_save_base", scope: !158, file: !159, line: 64, baseType: !163, size: 64, offset: 576)
!172 = !DIDerivedType(tag: DW_TAG_member, name: "_IO_backup_base", scope: !158, file: !159, line: 65, baseType: !163, size: 64, offset: 640)
!173 = !DIDerivedType(tag: DW_TAG_member, name: "_IO_save_end", scope: !158, file: !159, line: 66, baseType: !163, size: 64, offset: 704)
!174 = !DIDerivedType(tag: DW_TAG_member, name: "_markers", scope: !158, file: !159, line: 68, baseType: !175, size: 64, offset: 768)
!175 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !176, size: 64)
!176 = !DICompositeType(tag: DW_TAG_structure_type, name: "_IO_marker", file: !159, line: 36, flags: DIFlagFwdDecl)
!177 = !DIDerivedType(tag: DW_TAG_member, name: "_chain", scope: !158, file: !159, line: 70, baseType: !178, size: 64, offset: 832)
!178 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !158, size: 64)
!179 = !DIDerivedType(tag: DW_TAG_member, name: "_fileno", scope: !158, file: !159, line: 72, baseType: !38, size: 32, offset: 896)
!180 = !DIDerivedType(tag: DW_TAG_member, name: "_flags2", scope: !158, file: !159, line: 73, baseType: !38, size: 32, offset: 928)
!181 = !DIDerivedType(tag: DW_TAG_member, name: "_old_offset", scope: !158, file: !159, line: 74, baseType: !182, size: 64, offset: 960)
!182 = !DIDerivedType(tag: DW_TAG_typedef, name: "__off_t", file: !183, line: 152, baseType: !184)
!183 = !DIFile(filename: "/usr/include/x86_64-linux-gnu/bits/types.h", directory: "", checksumkind: CSK_MD5, checksum: "e1865d9fe29fe1b5ced550b7ba458f9e")
!184 = !DIBasicType(name: "long", size: 64, encoding: DW_ATE_signed)
!185 = !DIDerivedType(tag: DW_TAG_member, name: "_cur_column", scope: !158, file: !159, line: 77, baseType: !186, size: 16, offset: 1024)
!186 = !DIBasicType(name: "unsigned short", size: 16, encoding: DW_ATE_unsigned)
!187 = !DIDerivedType(tag: DW_TAG_member, name: "_vtable_offset", scope: !158, file: !159, line: 78, baseType: !188, size: 8, offset: 1040)
!188 = !DIBasicType(name: "signed char", size: 8, encoding: DW_ATE_signed_char)
!189 = !DIDerivedType(tag: DW_TAG_member, name: "_shortbuf", scope: !158, file: !159, line: 79, baseType: !98, size: 8, offset: 1048)
!190 = !DIDerivedType(tag: DW_TAG_member, name: "_lock", scope: !158, file: !159, line: 81, baseType: !191, size: 64, offset: 1088)
!191 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !192, size: 64)
!192 = !DIDerivedType(tag: DW_TAG_typedef, name: "_IO_lock_t", file: !159, line: 43, baseType: null)
!193 = !DIDerivedType(tag: DW_TAG_member, name: "_offset", scope: !158, file: !159, line: 89, baseType: !194, size: 64, offset: 1152)
!194 = !DIDerivedType(tag: DW_TAG_typedef, name: "__off64_t", file: !183, line: 153, baseType: !184)
!195 = !DIDerivedType(tag: DW_TAG_member, name: "_codecvt", scope: !158, file: !159, line: 91, baseType: !196, size: 64, offset: 1216)
!196 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !197, size: 64)
!197 = !DICompositeType(tag: DW_TAG_structure_type, name: "_IO_codecvt", file: !159, line: 37, flags: DIFlagFwdDecl)
!198 = !DIDerivedType(tag: DW_TAG_member, name: "_wide_data", scope: !158, file: !159, line: 92, baseType: !199, size: 64, offset: 1280)
!199 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !200, size: 64)
!200 = !DICompositeType(tag: DW_TAG_structure_type, name: "_IO_wide_data", file: !159, line: 38, flags: DIFlagFwdDecl)
!201 = !DIDerivedType(tag: DW_TAG_member, name: "_freeres_list", scope: !158, file: !159, line: 93, baseType: !178, size: 64, offset: 1344)
!202 = !DIDerivedType(tag: DW_TAG_member, name: "_freeres_buf", scope: !158, file: !159, line: 94, baseType: !31, size: 64, offset: 1408)
!203 = !DIDerivedType(tag: DW_TAG_member, name: "__pad5", scope: !158, file: !159, line: 95, baseType: !204, size: 64, offset: 1472)
!204 = !DIDerivedType(tag: DW_TAG_typedef, name: "size_t", file: !205, line: 46, baseType: !206)
!205 = !DIFile(filename: "/usr/lib/llvm-15/lib/clang/15.0.7/include/stddef.h", directory: "", checksumkind: CSK_MD5, checksum: "b76978376d35d5cd171876ac58ac1256")
!206 = !DIBasicType(name: "unsigned long", size: 64, encoding: DW_ATE_unsigned)
!207 = !DIDerivedType(tag: DW_TAG_member, name: "_mode", scope: !158, file: !159, line: 96, baseType: !38, size: 32, offset: 1536)
!208 = !DIDerivedType(tag: DW_TAG_member, name: "_unused2", scope: !158, file: !159, line: 98, baseType: !209, size: 160, offset: 1568)
!209 = !DICompositeType(tag: DW_TAG_array_type, baseType: !4, size: 160, elements: !210)
!210 = !{!211}
!211 = !DISubrange(count: 20)
!212 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !58, size: 64)
!213 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !214, size: 64)
!214 = !DIDerivedType(tag: DW_TAG_const_type, baseType: !4)
!215 = !DILocalVariable(name: "ipInF", arg: 1, scope: !150, file: !2, line: 89, type: !155)
!216 = !DILocation(line: 89, column: 30, scope: !150)
!217 = !DILocalVariable(name: "ipOutF", arg: 2, scope: !150, file: !2, line: 89, type: !155)
!218 = !DILocation(line: 89, column: 43, scope: !150)
!219 = !DILocalVariable(name: "ipFlag", arg: 3, scope: !150, file: !2, line: 89, type: !32)
!220 = !DILocation(line: 89, column: 58, scope: !150)
!221 = !DILocalVariable(name: "CurChar", arg: 4, scope: !150, file: !2, line: 89, type: !153)
!222 = !DILocation(line: 89, column: 73, scope: !150)
!223 = !DILocalVariable(name: "converted", arg: 5, scope: !150, file: !2, line: 89, type: !212)
!224 = !DILocation(line: 89, column: 96, scope: !150)
!225 = !DILocalVariable(name: "progname", arg: 6, scope: !150, file: !2, line: 89, type: !213)
!226 = !DILocation(line: 89, column: 119, scope: !150)
!227 = !DILocalVariable(name: "TempNextChar", scope: !150, file: !2, line: 91, type: !153)
!228 = !DILocation(line: 91, column: 10, scope: !150)
!229 = !DILocation(line: 95, column: 34, scope: !230)
!230 = distinct !DILexicalBlock(scope: !150, file: !2, line: 95, column: 8)
!231 = !DILocation(line: 95, column: 41, scope: !230)
!232 = !DILocation(line: 95, column: 49, scope: !230)
!233 = !DILocation(line: 95, column: 24, scope: !230)
!234 = !DILocation(line: 95, column: 22, scope: !230)
!235 = !DILocation(line: 95, column: 59, scope: !230)
!236 = !DILocation(line: 95, column: 8, scope: !150)
!237 = !DILocation(line: 96, column: 22, scope: !238)
!238 = distinct !DILexicalBlock(scope: !239, file: !2, line: 96, column: 9)
!239 = distinct !DILexicalBlock(scope: !230, file: !2, line: 95, column: 68)
!240 = !DILocation(line: 96, column: 36, scope: !238)
!241 = !DILocation(line: 96, column: 43, scope: !238)
!242 = !DILocation(line: 96, column: 51, scope: !238)
!243 = !DILocation(line: 96, column: 9, scope: !238)
!244 = !DILocation(line: 96, column: 60, scope: !238)
!245 = !DILocation(line: 96, column: 9, scope: !239)
!246 = !DILocation(line: 97, column: 24, scope: !247)
!247 = distinct !DILexicalBlock(scope: !238, file: !2, line: 96, column: 69)
!248 = !DILocation(line: 97, column: 31, scope: !247)
!249 = !DILocation(line: 97, column: 9, scope: !247)
!250 = !DILocation(line: 98, column: 9, scope: !247)
!251 = !DILocation(line: 100, column: 10, scope: !252)
!252 = distinct !DILexicalBlock(scope: !239, file: !2, line: 100, column: 10)
!253 = !DILocation(line: 100, column: 23, scope: !252)
!254 = !DILocation(line: 100, column: 10, scope: !239)
!255 = !DILocation(line: 101, column: 21, scope: !256)
!256 = distinct !DILexicalBlock(scope: !257, file: !2, line: 101, column: 11)
!257 = distinct !DILexicalBlock(scope: !252, file: !2, line: 100, column: 33)
!258 = !DILocation(line: 101, column: 30, scope: !256)
!259 = !DILocation(line: 101, column: 38, scope: !256)
!260 = !DILocation(line: 101, column: 46, scope: !256)
!261 = !DILocation(line: 101, column: 11, scope: !256)
!262 = !DILocation(line: 101, column: 56, scope: !256)
!263 = !DILocation(line: 101, column: 11, scope: !257)
!264 = !DILocation(line: 102, column: 27, scope: !265)
!265 = distinct !DILexicalBlock(scope: !256, file: !2, line: 101, column: 65)
!266 = !DILocation(line: 102, column: 34, scope: !265)
!267 = !DILocation(line: 102, column: 11, scope: !265)
!268 = !DILocation(line: 103, column: 11, scope: !265)
!269 = !DILocation(line: 105, column: 5, scope: !257)
!270 = !DILocation(line: 106, column: 9, scope: !271)
!271 = distinct !DILexicalBlock(scope: !252, file: !2, line: 105, column: 12)
!272 = !DILocation(line: 106, column: 19, scope: !271)
!273 = !DILocation(line: 107, column: 11, scope: !274)
!274 = distinct !DILexicalBlock(scope: !271, file: !2, line: 107, column: 11)
!275 = !DILocation(line: 107, column: 19, scope: !274)
!276 = !DILocation(line: 107, column: 11, scope: !271)
!277 = !DILocation(line: 108, column: 29, scope: !278)
!278 = distinct !DILexicalBlock(scope: !279, file: !2, line: 108, column: 13)
!279 = distinct !DILexicalBlock(scope: !274, file: !2, line: 107, column: 28)
!280 = !DILocation(line: 108, column: 37, scope: !278)
!281 = !DILocation(line: 108, column: 45, scope: !278)
!282 = !DILocation(line: 108, column: 13, scope: !278)
!283 = !DILocation(line: 108, column: 55, scope: !278)
!284 = !DILocation(line: 108, column: 13, scope: !279)
!285 = !DILocation(line: 109, column: 29, scope: !286)
!286 = distinct !DILexicalBlock(scope: !278, file: !2, line: 108, column: 64)
!287 = !DILocation(line: 109, column: 36, scope: !286)
!288 = !DILocation(line: 109, column: 13, scope: !286)
!289 = !DILocation(line: 110, column: 13, scope: !286)
!290 = !DILocation(line: 112, column: 7, scope: !279)
!291 = !DILocation(line: 114, column: 3, scope: !239)
!292 = !DILocation(line: 115, column: 16, scope: !293)
!293 = distinct !DILexicalBlock(scope: !294, file: !2, line: 115, column: 9)
!294 = distinct !DILexicalBlock(scope: !230, file: !2, line: 114, column: 10)
!295 = !DILocation(line: 115, column: 9, scope: !293)
!296 = !DILocation(line: 115, column: 9, scope: !294)
!297 = !DILocation(line: 116, column: 24, scope: !298)
!298 = distinct !DILexicalBlock(scope: !293, file: !2, line: 115, column: 24)
!299 = !DILocation(line: 116, column: 31, scope: !298)
!300 = !DILocation(line: 116, column: 9, scope: !298)
!301 = !DILocation(line: 117, column: 9, scope: !298)
!302 = !DILocation(line: 119, column: 10, scope: !303)
!303 = distinct !DILexicalBlock(scope: !294, file: !2, line: 119, column: 10)
!304 = !DILocation(line: 119, column: 18, scope: !303)
!305 = !DILocation(line: 119, column: 10, scope: !294)
!306 = !DILocation(line: 120, column: 23, scope: !307)
!307 = distinct !DILexicalBlock(scope: !308, file: !2, line: 120, column: 13)
!308 = distinct !DILexicalBlock(scope: !303, file: !2, line: 119, column: 28)
!309 = !DILocation(line: 120, column: 32, scope: !307)
!310 = !DILocation(line: 120, column: 40, scope: !307)
!311 = !DILocation(line: 120, column: 48, scope: !307)
!312 = !DILocation(line: 120, column: 13, scope: !307)
!313 = !DILocation(line: 120, column: 58, scope: !307)
!314 = !DILocation(line: 120, column: 13, scope: !308)
!315 = !DILocation(line: 121, column: 29, scope: !316)
!316 = distinct !DILexicalBlock(scope: !307, file: !2, line: 120, column: 67)
!317 = !DILocation(line: 121, column: 36, scope: !316)
!318 = !DILocation(line: 121, column: 13, scope: !316)
!319 = !DILocation(line: 122, column: 13, scope: !316)
!320 = !DILocation(line: 124, column: 5, scope: !308)
!321 = !DILocation(line: 126, column: 10, scope: !150)
!322 = !DILocation(line: 126, column: 3, scope: !150)
!323 = !DILocation(line: 127, column: 1, scope: !150)
!324 = distinct !DISubprogram(name: "StripDelimiter", scope: !2, file: !2, line: 135, type: !325, scopeLine: 136, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !29, retainedNodes: !144)
!325 = !DISubroutineType(types: !326)
!326 = !{!38, !155, !155, !32, !38, !212, !213}
!327 = !DILocalVariable(name: "ipInF", arg: 1, scope: !324, file: !2, line: 135, type: !155)
!328 = !DILocation(line: 135, column: 26, scope: !324)
!329 = !DILocalVariable(name: "ipOutF", arg: 2, scope: !324, file: !2, line: 135, type: !155)
!330 = !DILocation(line: 135, column: 39, scope: !324)
!331 = !DILocalVariable(name: "ipFlag", arg: 3, scope: !324, file: !2, line: 135, type: !32)
!332 = !DILocation(line: 135, column: 54, scope: !324)
!333 = !DILocalVariable(name: "CurChar", arg: 4, scope: !324, file: !2, line: 135, type: !38)
!334 = !DILocation(line: 135, column: 66, scope: !324)
!335 = !DILocalVariable(name: "converted", arg: 5, scope: !324, file: !2, line: 135, type: !212)
!336 = !DILocation(line: 135, column: 89, scope: !324)
!337 = !DILocalVariable(name: "progname", arg: 6, scope: !324, file: !2, line: 135, type: !213)
!338 = !DILocation(line: 135, column: 112, scope: !324)
!339 = !DILocalVariable(name: "TempNextChar", scope: !324, file: !2, line: 137, type: !38)
!340 = !DILocation(line: 137, column: 7, scope: !324)
!341 = !DILocation(line: 141, column: 30, scope: !342)
!342 = distinct !DILexicalBlock(scope: !324, file: !2, line: 141, column: 8)
!343 = !DILocation(line: 141, column: 24, scope: !342)
!344 = !DILocation(line: 141, column: 22, scope: !342)
!345 = !DILocation(line: 141, column: 38, scope: !342)
!346 = !DILocation(line: 141, column: 8, scope: !324)
!347 = !DILocation(line: 142, column: 17, scope: !348)
!348 = distinct !DILexicalBlock(scope: !349, file: !2, line: 142, column: 9)
!349 = distinct !DILexicalBlock(scope: !342, file: !2, line: 141, column: 46)
!350 = !DILocation(line: 142, column: 31, scope: !348)
!351 = !DILocation(line: 142, column: 9, scope: !348)
!352 = !DILocation(line: 142, column: 39, scope: !348)
!353 = !DILocation(line: 142, column: 9, scope: !349)
!354 = !DILocation(line: 143, column: 24, scope: !355)
!355 = distinct !DILexicalBlock(scope: !348, file: !2, line: 142, column: 47)
!356 = !DILocation(line: 143, column: 31, scope: !355)
!357 = !DILocation(line: 143, column: 9, scope: !355)
!358 = !DILocation(line: 144, column: 9, scope: !355)
!359 = !DILocation(line: 146, column: 10, scope: !360)
!360 = distinct !DILexicalBlock(scope: !349, file: !2, line: 146, column: 10)
!361 = !DILocation(line: 146, column: 23, scope: !360)
!362 = !DILocation(line: 146, column: 10, scope: !349)
!363 = !DILocation(line: 147, column: 18, scope: !364)
!364 = distinct !DILexicalBlock(scope: !365, file: !2, line: 147, column: 11)
!365 = distinct !DILexicalBlock(scope: !360, file: !2, line: 146, column: 35)
!366 = !DILocation(line: 147, column: 27, scope: !364)
!367 = !DILocation(line: 147, column: 11, scope: !364)
!368 = !DILocation(line: 147, column: 36, scope: !364)
!369 = !DILocation(line: 147, column: 11, scope: !365)
!370 = !DILocation(line: 148, column: 26, scope: !371)
!371 = distinct !DILexicalBlock(scope: !364, file: !2, line: 147, column: 44)
!372 = !DILocation(line: 148, column: 33, scope: !371)
!373 = !DILocation(line: 148, column: 11, scope: !371)
!374 = !DILocation(line: 149, column: 11, scope: !371)
!375 = !DILocation(line: 151, column: 5, scope: !365)
!376 = !DILocation(line: 152, column: 9, scope: !377)
!377 = distinct !DILexicalBlock(scope: !360, file: !2, line: 151, column: 12)
!378 = !DILocation(line: 152, column: 19, scope: !377)
!379 = !DILocation(line: 153, column: 11, scope: !380)
!380 = distinct !DILexicalBlock(scope: !377, file: !2, line: 153, column: 11)
!381 = !DILocation(line: 153, column: 19, scope: !380)
!382 = !DILocation(line: 153, column: 11, scope: !377)
!383 = !DILocation(line: 154, column: 27, scope: !384)
!384 = distinct !DILexicalBlock(scope: !385, file: !2, line: 154, column: 13)
!385 = distinct !DILexicalBlock(scope: !380, file: !2, line: 153, column: 28)
!386 = !DILocation(line: 154, column: 13, scope: !384)
!387 = !DILocation(line: 154, column: 35, scope: !384)
!388 = !DILocation(line: 154, column: 13, scope: !385)
!389 = !DILocation(line: 155, column: 28, scope: !390)
!390 = distinct !DILexicalBlock(scope: !384, file: !2, line: 154, column: 43)
!391 = !DILocation(line: 155, column: 35, scope: !390)
!392 = !DILocation(line: 155, column: 13, scope: !390)
!393 = !DILocation(line: 156, column: 13, scope: !390)
!394 = !DILocation(line: 158, column: 7, scope: !385)
!395 = !DILocation(line: 160, column: 3, scope: !349)
!396 = !DILocation(line: 161, column: 16, scope: !397)
!397 = distinct !DILexicalBlock(scope: !398, file: !2, line: 161, column: 9)
!398 = distinct !DILexicalBlock(scope: !342, file: !2, line: 160, column: 10)
!399 = !DILocation(line: 161, column: 9, scope: !397)
!400 = !DILocation(line: 161, column: 9, scope: !398)
!401 = !DILocation(line: 162, column: 24, scope: !402)
!402 = distinct !DILexicalBlock(scope: !397, file: !2, line: 161, column: 24)
!403 = !DILocation(line: 162, column: 31, scope: !402)
!404 = !DILocation(line: 162, column: 9, scope: !402)
!405 = !DILocation(line: 163, column: 9, scope: !402)
!406 = !DILocation(line: 165, column: 10, scope: !407)
!407 = distinct !DILexicalBlock(scope: !398, file: !2, line: 165, column: 10)
!408 = !DILocation(line: 165, column: 18, scope: !407)
!409 = !DILocation(line: 165, column: 10, scope: !398)
!410 = !DILocation(line: 166, column: 20, scope: !411)
!411 = distinct !DILexicalBlock(scope: !412, file: !2, line: 166, column: 13)
!412 = distinct !DILexicalBlock(scope: !407, file: !2, line: 165, column: 30)
!413 = !DILocation(line: 166, column: 29, scope: !411)
!414 = !DILocation(line: 166, column: 13, scope: !411)
!415 = !DILocation(line: 166, column: 38, scope: !411)
!416 = !DILocation(line: 166, column: 13, scope: !412)
!417 = !DILocation(line: 167, column: 28, scope: !418)
!418 = distinct !DILexicalBlock(scope: !411, file: !2, line: 166, column: 46)
!419 = !DILocation(line: 167, column: 35, scope: !418)
!420 = !DILocation(line: 167, column: 13, scope: !418)
!421 = !DILocation(line: 168, column: 13, scope: !418)
!422 = !DILocation(line: 170, column: 5, scope: !412)
!423 = !DILocation(line: 172, column: 10, scope: !324)
!424 = !DILocation(line: 172, column: 3, scope: !324)
!425 = !DILocation(line: 173, column: 1, scope: !324)
!426 = distinct !DISubprogram(name: "ConvertDosToUnixW", scope: !2, file: !2, line: 180, type: !427, scopeLine: 181, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !29, retainedNodes: !144)
!427 = !DISubroutineType(types: !428)
!428 = !{!38, !155, !155, !32, !213}
!429 = !DILocalVariable(name: "ipInF", arg: 1, scope: !426, file: !2, line: 180, type: !155)
!430 = !DILocation(line: 180, column: 29, scope: !426)
!431 = !DILocalVariable(name: "ipOutF", arg: 2, scope: !426, file: !2, line: 180, type: !155)
!432 = !DILocation(line: 180, column: 42, scope: !426)
!433 = !DILocalVariable(name: "ipFlag", arg: 3, scope: !426, file: !2, line: 180, type: !32)
!434 = !DILocation(line: 180, column: 57, scope: !426)
!435 = !DILocalVariable(name: "progname", arg: 4, scope: !426, file: !2, line: 180, type: !213)
!436 = !DILocation(line: 180, column: 77, scope: !426)
!437 = !DILocalVariable(name: "RetVal", scope: !426, file: !2, line: 182, type: !38)
!438 = !DILocation(line: 182, column: 9, scope: !426)
!439 = !DILocalVariable(name: "PrevChar", scope: !426, file: !2, line: 183, type: !153)
!440 = !DILocation(line: 183, column: 12, scope: !426)
!441 = !DILocalVariable(name: "TempChar", scope: !426, file: !2, line: 184, type: !153)
!442 = !DILocation(line: 184, column: 12, scope: !426)
!443 = !DILocalVariable(name: "TempNextChar", scope: !426, file: !2, line: 185, type: !153)
!444 = !DILocation(line: 185, column: 12, scope: !426)
!445 = !DILocalVariable(name: "line_nr", scope: !426, file: !2, line: 186, type: !58)
!446 = !DILocation(line: 186, column: 18, scope: !426)
!447 = !DILocalVariable(name: "converted", scope: !426, file: !2, line: 187, type: !58)
!448 = !DILocation(line: 187, column: 18, scope: !426)
!449 = !DILocation(line: 189, column: 5, scope: !426)
!450 = !DILocation(line: 189, column: 13, scope: !426)
!451 = !DILocation(line: 189, column: 20, scope: !426)
!452 = !DILocation(line: 198, column: 13, scope: !426)
!453 = !DILocation(line: 198, column: 21, scope: !426)
!454 = !DILocation(line: 198, column: 5, scope: !426)
!455 = !DILocation(line: 201, column: 9, scope: !456)
!456 = distinct !DILexicalBlock(scope: !426, file: !2, line: 199, column: 5)
!457 = !DILocation(line: 201, column: 38, scope: !456)
!458 = !DILocation(line: 201, column: 45, scope: !456)
!459 = !DILocation(line: 201, column: 53, scope: !456)
!460 = !DILocation(line: 201, column: 28, scope: !456)
!461 = !DILocation(line: 201, column: 26, scope: !456)
!462 = !DILocation(line: 201, column: 63, scope: !456)
!463 = !DILocation(line: 202, column: 16, scope: !464)
!464 = distinct !DILexicalBlock(scope: !465, file: !2, line: 202, column: 15)
!465 = distinct !DILexicalBlock(scope: !456, file: !2, line: 201, column: 72)
!466 = !DILocation(line: 202, column: 24, scope: !464)
!467 = !DILocation(line: 202, column: 30, scope: !464)
!468 = !DILocation(line: 202, column: 36, scope: !464)
!469 = !DILocation(line: 203, column: 16, scope: !464)
!470 = !DILocation(line: 203, column: 25, scope: !464)
!471 = !DILocation(line: 203, column: 31, scope: !464)
!472 = !DILocation(line: 204, column: 16, scope: !464)
!473 = !DILocation(line: 204, column: 25, scope: !464)
!474 = !DILocation(line: 204, column: 34, scope: !464)
!475 = !DILocation(line: 205, column: 16, scope: !464)
!476 = !DILocation(line: 205, column: 25, scope: !464)
!477 = !DILocation(line: 205, column: 34, scope: !464)
!478 = !DILocation(line: 206, column: 16, scope: !464)
!479 = !DILocation(line: 206, column: 25, scope: !464)
!480 = !DILocation(line: 206, column: 34, scope: !464)
!481 = !DILocation(line: 207, column: 16, scope: !464)
!482 = !DILocation(line: 207, column: 25, scope: !464)
!483 = !DILocation(line: 202, column: 15, scope: !465)
!484 = !DILocation(line: 208, column: 20, scope: !485)
!485 = distinct !DILexicalBlock(scope: !464, file: !2, line: 207, column: 35)
!486 = !DILocation(line: 209, column: 13, scope: !485)
!487 = !DILocation(line: 209, column: 21, scope: !485)
!488 = !DILocation(line: 209, column: 28, scope: !485)
!489 = !DILocation(line: 210, column: 17, scope: !490)
!490 = distinct !DILexicalBlock(scope: !485, file: !2, line: 210, column: 17)
!491 = !DILocation(line: 210, column: 25, scope: !490)
!492 = !DILocation(line: 210, column: 17, scope: !485)
!493 = !DILocation(line: 211, column: 20, scope: !494)
!494 = distinct !DILexicalBlock(scope: !495, file: !2, line: 211, column: 19)
!495 = distinct !DILexicalBlock(scope: !490, file: !2, line: 210, column: 34)
!496 = !DILocation(line: 211, column: 28, scope: !494)
!497 = !DILocation(line: 211, column: 19, scope: !494)
!498 = !DILocation(line: 211, column: 40, scope: !494)
!499 = !DILocation(line: 211, column: 45, scope: !494)
!500 = !DILocation(line: 211, column: 53, scope: !494)
!501 = !DILocation(line: 211, column: 19, scope: !495)
!502 = !DILocation(line: 211, column: 61, scope: !494)
!503 = !DILocation(line: 211, column: 69, scope: !494)
!504 = !DILocation(line: 211, column: 75, scope: !494)
!505 = !DILocation(line: 212, column: 32, scope: !495)
!506 = !DILocation(line: 212, column: 48, scope: !495)
!507 = !DILocation(line: 212, column: 15, scope: !495)
!508 = !DILocation(line: 213, column: 32, scope: !495)
!509 = !DILocation(line: 213, column: 40, scope: !495)
!510 = !DILocation(line: 213, column: 87, scope: !495)
!511 = !DILocation(line: 213, column: 97, scope: !495)
!512 = !DILocation(line: 213, column: 15, scope: !495)
!513 = !DILocation(line: 214, column: 13, scope: !495)
!514 = !DILocation(line: 215, column: 13, scope: !485)
!515 = !DILocation(line: 217, column: 15, scope: !516)
!516 = distinct !DILexicalBlock(scope: !465, file: !2, line: 217, column: 15)
!517 = !DILocation(line: 217, column: 24, scope: !516)
!518 = !DILocation(line: 217, column: 15, scope: !465)
!519 = !DILocation(line: 218, column: 17, scope: !520)
!520 = distinct !DILexicalBlock(scope: !521, file: !2, line: 218, column: 17)
!521 = distinct !DILexicalBlock(scope: !516, file: !2, line: 217, column: 33)
!522 = !DILocation(line: 218, column: 26, scope: !520)
!523 = !DILocation(line: 218, column: 17, scope: !521)
!524 = !DILocation(line: 219, column: 15, scope: !520)
!525 = !DILocation(line: 220, column: 27, scope: !526)
!526 = distinct !DILexicalBlock(scope: !521, file: !2, line: 220, column: 17)
!527 = !DILocation(line: 220, column: 37, scope: !526)
!528 = !DILocation(line: 220, column: 45, scope: !526)
!529 = !DILocation(line: 220, column: 53, scope: !526)
!530 = !DILocation(line: 220, column: 17, scope: !526)
!531 = !DILocation(line: 220, column: 63, scope: !526)
!532 = !DILocation(line: 220, column: 17, scope: !521)
!533 = !DILocation(line: 221, column: 22, scope: !534)
!534 = distinct !DILexicalBlock(scope: !526, file: !2, line: 220, column: 72)
!535 = !DILocation(line: 222, column: 31, scope: !534)
!536 = !DILocation(line: 222, column: 38, scope: !534)
!537 = !DILocation(line: 222, column: 15, scope: !534)
!538 = !DILocation(line: 223, column: 15, scope: !534)
!539 = !DILocation(line: 225, column: 11, scope: !521)
!540 = !DILocation(line: 226, column: 34, scope: !541)
!541 = distinct !DILexicalBlock(scope: !542, file: !2, line: 226, column: 17)
!542 = distinct !DILexicalBlock(scope: !516, file: !2, line: 225, column: 18)
!543 = !DILocation(line: 226, column: 41, scope: !541)
!544 = !DILocation(line: 226, column: 49, scope: !541)
!545 = !DILocation(line: 226, column: 57, scope: !541)
!546 = !DILocation(line: 226, column: 79, scope: !541)
!547 = !DILocation(line: 226, column: 17, scope: !541)
!548 = !DILocation(line: 226, column: 89, scope: !541)
!549 = !DILocation(line: 226, column: 17, scope: !542)
!550 = !DILocation(line: 227, column: 22, scope: !551)
!551 = distinct !DILexicalBlock(scope: !541, file: !2, line: 226, column: 98)
!552 = !DILocation(line: 228, column: 15, scope: !551)
!553 = !DILocation(line: 231, column: 22, scope: !465)
!554 = !DILocation(line: 231, column: 20, scope: !465)
!555 = distinct !{!555, !455, !556, !557}
!556 = !DILocation(line: 232, column: 9, scope: !456)
!557 = !{!"llvm.loop.mustprogress"}
!558 = !DILocation(line: 233, column: 13, scope: !559)
!559 = distinct !DILexicalBlock(scope: !456, file: !2, line: 233, column: 13)
!560 = !DILocation(line: 233, column: 22, scope: !559)
!561 = !DILocation(line: 233, column: 30, scope: !559)
!562 = !DILocation(line: 233, column: 33, scope: !559)
!563 = !DILocation(line: 233, column: 41, scope: !559)
!564 = !DILocation(line: 233, column: 49, scope: !559)
!565 = !DILocation(line: 233, column: 52, scope: !559)
!566 = !DILocation(line: 233, column: 61, scope: !559)
!567 = !DILocation(line: 233, column: 69, scope: !559)
!568 = !DILocation(line: 233, column: 72, scope: !559)
!569 = !DILocation(line: 233, column: 81, scope: !559)
!570 = !DILocation(line: 233, column: 13, scope: !456)
!571 = !DILocation(line: 235, column: 17, scope: !572)
!572 = distinct !DILexicalBlock(scope: !573, file: !2, line: 235, column: 17)
!573 = distinct !DILexicalBlock(scope: !559, file: !2, line: 233, column: 90)
!574 = !DILocation(line: 235, column: 25, scope: !572)
!575 = !DILocation(line: 235, column: 33, scope: !572)
!576 = !DILocation(line: 235, column: 17, scope: !573)
!577 = !DILocation(line: 236, column: 32, scope: !578)
!578 = distinct !DILexicalBlock(scope: !572, file: !2, line: 235, column: 38)
!579 = !DILocation(line: 236, column: 48, scope: !578)
!580 = !DILocation(line: 236, column: 15, scope: !578)
!581 = !DILocation(line: 237, column: 32, scope: !578)
!582 = !DILocation(line: 237, column: 40, scope: !578)
!583 = !DILocation(line: 237, column: 15, scope: !578)
!584 = !DILocation(line: 238, column: 13, scope: !578)
!585 = !DILocation(line: 239, column: 33, scope: !586)
!586 = distinct !DILexicalBlock(scope: !573, file: !2, line: 239, column: 17)
!587 = !DILocation(line: 239, column: 41, scope: !586)
!588 = !DILocation(line: 239, column: 49, scope: !586)
!589 = !DILocation(line: 239, column: 17, scope: !586)
!590 = !DILocation(line: 239, column: 59, scope: !586)
!591 = !DILocation(line: 239, column: 17, scope: !573)
!592 = !DILocation(line: 240, column: 22, scope: !593)
!593 = distinct !DILexicalBlock(scope: !586, file: !2, line: 239, column: 68)
!594 = !DILocation(line: 241, column: 31, scope: !593)
!595 = !DILocation(line: 241, column: 38, scope: !593)
!596 = !DILocation(line: 241, column: 15, scope: !593)
!597 = !DILocation(line: 242, column: 13, scope: !593)
!598 = !DILocation(line: 243, column: 9, scope: !573)
!599 = !DILocation(line: 244, column: 14, scope: !600)
!600 = distinct !DILexicalBlock(scope: !456, file: !2, line: 244, column: 13)
!601 = !DILocation(line: 244, column: 23, scope: !600)
!602 = !DILocation(line: 244, column: 32, scope: !600)
!603 = !DILocation(line: 244, column: 42, scope: !600)
!604 = !DILocation(line: 244, column: 35, scope: !600)
!605 = !DILocation(line: 244, column: 13, scope: !456)
!606 = !DILocation(line: 245, column: 18, scope: !607)
!607 = distinct !DILexicalBlock(scope: !600, file: !2, line: 244, column: 50)
!608 = !DILocation(line: 246, column: 26, scope: !607)
!609 = !DILocation(line: 246, column: 33, scope: !607)
!610 = !DILocation(line: 246, column: 11, scope: !607)
!611 = !DILocation(line: 247, column: 9, scope: !607)
!612 = !DILocation(line: 248, column: 9, scope: !456)
!613 = !DILocation(line: 250, column: 9, scope: !456)
!614 = !DILocation(line: 250, column: 38, scope: !456)
!615 = !DILocation(line: 250, column: 45, scope: !456)
!616 = !DILocation(line: 250, column: 53, scope: !456)
!617 = !DILocation(line: 250, column: 28, scope: !456)
!618 = !DILocation(line: 250, column: 26, scope: !456)
!619 = !DILocation(line: 250, column: 63, scope: !456)
!620 = !DILocation(line: 251, column: 16, scope: !621)
!621 = distinct !DILexicalBlock(scope: !622, file: !2, line: 251, column: 15)
!622 = distinct !DILexicalBlock(scope: !456, file: !2, line: 250, column: 72)
!623 = !DILocation(line: 251, column: 24, scope: !621)
!624 = !DILocation(line: 251, column: 30, scope: !621)
!625 = !DILocation(line: 251, column: 36, scope: !621)
!626 = !DILocation(line: 252, column: 16, scope: !621)
!627 = !DILocation(line: 252, column: 25, scope: !621)
!628 = !DILocation(line: 252, column: 31, scope: !621)
!629 = !DILocation(line: 253, column: 16, scope: !621)
!630 = !DILocation(line: 253, column: 25, scope: !621)
!631 = !DILocation(line: 253, column: 34, scope: !621)
!632 = !DILocation(line: 254, column: 16, scope: !621)
!633 = !DILocation(line: 254, column: 25, scope: !621)
!634 = !DILocation(line: 254, column: 34, scope: !621)
!635 = !DILocation(line: 255, column: 16, scope: !621)
!636 = !DILocation(line: 255, column: 25, scope: !621)
!637 = !DILocation(line: 255, column: 34, scope: !621)
!638 = !DILocation(line: 256, column: 16, scope: !621)
!639 = !DILocation(line: 256, column: 25, scope: !621)
!640 = !DILocation(line: 251, column: 15, scope: !622)
!641 = !DILocation(line: 257, column: 20, scope: !642)
!642 = distinct !DILexicalBlock(scope: !621, file: !2, line: 256, column: 35)
!643 = !DILocation(line: 258, column: 13, scope: !642)
!644 = !DILocation(line: 258, column: 21, scope: !642)
!645 = !DILocation(line: 258, column: 28, scope: !642)
!646 = !DILocation(line: 259, column: 17, scope: !647)
!647 = distinct !DILexicalBlock(scope: !642, file: !2, line: 259, column: 17)
!648 = !DILocation(line: 259, column: 25, scope: !647)
!649 = !DILocation(line: 259, column: 17, scope: !642)
!650 = !DILocation(line: 260, column: 20, scope: !651)
!651 = distinct !DILexicalBlock(scope: !652, file: !2, line: 260, column: 19)
!652 = distinct !DILexicalBlock(scope: !647, file: !2, line: 259, column: 34)
!653 = !DILocation(line: 260, column: 28, scope: !651)
!654 = !DILocation(line: 260, column: 19, scope: !651)
!655 = !DILocation(line: 260, column: 40, scope: !651)
!656 = !DILocation(line: 260, column: 45, scope: !651)
!657 = !DILocation(line: 260, column: 53, scope: !651)
!658 = !DILocation(line: 260, column: 19, scope: !652)
!659 = !DILocation(line: 260, column: 61, scope: !651)
!660 = !DILocation(line: 260, column: 69, scope: !651)
!661 = !DILocation(line: 260, column: 75, scope: !651)
!662 = !DILocation(line: 261, column: 32, scope: !652)
!663 = !DILocation(line: 261, column: 48, scope: !652)
!664 = !DILocation(line: 261, column: 15, scope: !652)
!665 = !DILocation(line: 262, column: 32, scope: !652)
!666 = !DILocation(line: 262, column: 40, scope: !652)
!667 = !DILocation(line: 262, column: 88, scope: !652)
!668 = !DILocation(line: 262, column: 98, scope: !652)
!669 = !DILocation(line: 262, column: 15, scope: !652)
!670 = !DILocation(line: 263, column: 13, scope: !652)
!671 = !DILocation(line: 264, column: 13, scope: !642)
!672 = !DILocation(line: 266, column: 16, scope: !673)
!673 = distinct !DILexicalBlock(scope: !622, file: !2, line: 266, column: 15)
!674 = !DILocation(line: 266, column: 25, scope: !673)
!675 = !DILocation(line: 266, column: 15, scope: !622)
!676 = !DILocation(line: 267, column: 17, scope: !677)
!677 = distinct !DILexicalBlock(scope: !678, file: !2, line: 267, column: 17)
!678 = distinct !DILexicalBlock(scope: !673, file: !2, line: 266, column: 35)
!679 = !DILocation(line: 267, column: 26, scope: !677)
!680 = !DILocation(line: 267, column: 17, scope: !678)
!681 = !DILocation(line: 268, column: 15, scope: !677)
!682 = !DILocation(line: 269, column: 26, scope: !683)
!683 = distinct !DILexicalBlock(scope: !678, file: !2, line: 269, column: 16)
!684 = !DILocation(line: 269, column: 36, scope: !683)
!685 = !DILocation(line: 269, column: 44, scope: !683)
!686 = !DILocation(line: 269, column: 52, scope: !683)
!687 = !DILocation(line: 269, column: 16, scope: !683)
!688 = !DILocation(line: 269, column: 62, scope: !683)
!689 = !DILocation(line: 269, column: 16, scope: !678)
!690 = !DILocation(line: 270, column: 22, scope: !691)
!691 = distinct !DILexicalBlock(scope: !683, file: !2, line: 269, column: 71)
!692 = !DILocation(line: 271, column: 31, scope: !691)
!693 = !DILocation(line: 271, column: 38, scope: !691)
!694 = !DILocation(line: 271, column: 15, scope: !691)
!695 = !DILocation(line: 272, column: 15, scope: !691)
!696 = !DILocation(line: 274, column: 11, scope: !678)
!697 = !DILocation(line: 276, column: 44, scope: !698)
!698 = distinct !DILexicalBlock(scope: !699, file: !2, line: 276, column: 18)
!699 = distinct !DILexicalBlock(scope: !673, file: !2, line: 274, column: 17)
!700 = !DILocation(line: 276, column: 51, scope: !698)
!701 = !DILocation(line: 276, column: 59, scope: !698)
!702 = !DILocation(line: 276, column: 34, scope: !698)
!703 = !DILocation(line: 276, column: 32, scope: !698)
!704 = !DILocation(line: 276, column: 69, scope: !698)
!705 = !DILocation(line: 276, column: 18, scope: !699)
!706 = !DILocation(line: 277, column: 32, scope: !707)
!707 = distinct !DILexicalBlock(scope: !708, file: !2, line: 277, column: 19)
!708 = distinct !DILexicalBlock(scope: !698, file: !2, line: 276, column: 78)
!709 = !DILocation(line: 277, column: 46, scope: !707)
!710 = !DILocation(line: 277, column: 53, scope: !707)
!711 = !DILocation(line: 277, column: 61, scope: !707)
!712 = !DILocation(line: 277, column: 19, scope: !707)
!713 = !DILocation(line: 277, column: 70, scope: !707)
!714 = !DILocation(line: 277, column: 19, scope: !708)
!715 = !DILocation(line: 278, column: 32, scope: !716)
!716 = distinct !DILexicalBlock(scope: !707, file: !2, line: 277, column: 79)
!717 = !DILocation(line: 278, column: 39, scope: !716)
!718 = !DILocation(line: 278, column: 17, scope: !716)
!719 = !DILocation(line: 279, column: 24, scope: !716)
!720 = !DILocation(line: 280, column: 17, scope: !716)
!721 = !DILocation(line: 283, column: 20, scope: !722)
!722 = distinct !DILexicalBlock(scope: !708, file: !2, line: 283, column: 20)
!723 = !DILocation(line: 283, column: 33, scope: !722)
!724 = !DILocation(line: 283, column: 20, scope: !708)
!725 = !DILocation(line: 284, column: 37, scope: !726)
!726 = distinct !DILexicalBlock(scope: !727, file: !2, line: 284, column: 21)
!727 = distinct !DILexicalBlock(scope: !722, file: !2, line: 283, column: 43)
!728 = !DILocation(line: 284, column: 45, scope: !726)
!729 = !DILocation(line: 284, column: 53, scope: !726)
!730 = !DILocation(line: 284, column: 21, scope: !726)
!731 = !DILocation(line: 284, column: 63, scope: !726)
!732 = !DILocation(line: 284, column: 21, scope: !727)
!733 = !DILocation(line: 285, column: 35, scope: !734)
!734 = distinct !DILexicalBlock(scope: !726, file: !2, line: 284, column: 72)
!735 = !DILocation(line: 285, column: 42, scope: !734)
!736 = !DILocation(line: 285, column: 19, scope: !734)
!737 = !DILocation(line: 286, column: 26, scope: !734)
!738 = !DILocation(line: 287, column: 19, scope: !734)
!739 = !DILocation(line: 289, column: 28, scope: !727)
!740 = !DILocation(line: 289, column: 26, scope: !727)
!741 = !DILocation(line: 290, column: 17, scope: !727)
!742 = distinct !{!742, !613, !743, !557}
!743 = !DILocation(line: 309, column: 9, scope: !456)
!744 = !DILocation(line: 292, column: 13, scope: !708)
!745 = !DILocation(line: 293, column: 33, scope: !746)
!746 = distinct !DILexicalBlock(scope: !699, file: !2, line: 293, column: 17)
!747 = !DILocation(line: 293, column: 41, scope: !746)
!748 = !DILocation(line: 293, column: 49, scope: !746)
!749 = !DILocation(line: 293, column: 17, scope: !746)
!750 = !DILocation(line: 293, column: 59, scope: !746)
!751 = !DILocation(line: 293, column: 17, scope: !699)
!752 = !DILocation(line: 294, column: 22, scope: !753)
!753 = distinct !DILexicalBlock(scope: !746, file: !2, line: 293, column: 68)
!754 = !DILocation(line: 295, column: 31, scope: !753)
!755 = !DILocation(line: 295, column: 38, scope: !753)
!756 = !DILocation(line: 295, column: 15, scope: !753)
!757 = !DILocation(line: 296, column: 15, scope: !753)
!758 = !DILocation(line: 298, column: 22, scope: !699)
!759 = !DILocation(line: 299, column: 20, scope: !699)
!760 = !DILocation(line: 300, column: 17, scope: !761)
!761 = distinct !DILexicalBlock(scope: !699, file: !2, line: 300, column: 17)
!762 = !DILocation(line: 300, column: 25, scope: !761)
!763 = !DILocation(line: 300, column: 17, scope: !699)
!764 = !DILocation(line: 301, column: 35, scope: !765)
!765 = distinct !DILexicalBlock(scope: !766, file: !2, line: 301, column: 19)
!766 = distinct !DILexicalBlock(scope: !761, file: !2, line: 300, column: 34)
!767 = !DILocation(line: 301, column: 43, scope: !765)
!768 = !DILocation(line: 301, column: 51, scope: !765)
!769 = !DILocation(line: 301, column: 19, scope: !765)
!770 = !DILocation(line: 301, column: 61, scope: !765)
!771 = !DILocation(line: 301, column: 19, scope: !766)
!772 = !DILocation(line: 302, column: 24, scope: !773)
!773 = distinct !DILexicalBlock(scope: !765, file: !2, line: 301, column: 70)
!774 = !DILocation(line: 303, column: 33, scope: !773)
!775 = !DILocation(line: 303, column: 40, scope: !773)
!776 = !DILocation(line: 303, column: 17, scope: !773)
!777 = !DILocation(line: 304, column: 17, scope: !773)
!778 = !DILocation(line: 306, column: 13, scope: !766)
!779 = !DILocation(line: 308, column: 22, scope: !622)
!780 = !DILocation(line: 308, column: 20, scope: !622)
!781 = !DILocation(line: 310, column: 13, scope: !782)
!782 = distinct !DILexicalBlock(scope: !456, file: !2, line: 310, column: 13)
!783 = !DILocation(line: 310, column: 22, scope: !782)
!784 = !DILocation(line: 310, column: 30, scope: !782)
!785 = !DILocation(line: 310, column: 33, scope: !782)
!786 = !DILocation(line: 310, column: 41, scope: !782)
!787 = !DILocation(line: 310, column: 49, scope: !782)
!788 = !DILocation(line: 310, column: 52, scope: !782)
!789 = !DILocation(line: 310, column: 61, scope: !782)
!790 = !DILocation(line: 310, column: 69, scope: !782)
!791 = !DILocation(line: 310, column: 74, scope: !782)
!792 = !DILocation(line: 310, column: 83, scope: !782)
!793 = !DILocation(line: 310, column: 91, scope: !782)
!794 = !DILocation(line: 310, column: 94, scope: !782)
!795 = !DILocation(line: 310, column: 103, scope: !782)
!796 = !DILocation(line: 310, column: 13, scope: !456)
!797 = !DILocation(line: 312, column: 17, scope: !798)
!798 = distinct !DILexicalBlock(scope: !799, file: !2, line: 312, column: 17)
!799 = distinct !DILexicalBlock(scope: !782, file: !2, line: 310, column: 113)
!800 = !DILocation(line: 312, column: 25, scope: !798)
!801 = !DILocation(line: 312, column: 33, scope: !798)
!802 = !DILocation(line: 312, column: 17, scope: !799)
!803 = !DILocation(line: 313, column: 32, scope: !804)
!804 = distinct !DILexicalBlock(scope: !798, file: !2, line: 312, column: 38)
!805 = !DILocation(line: 313, column: 48, scope: !804)
!806 = !DILocation(line: 313, column: 15, scope: !804)
!807 = !DILocation(line: 314, column: 32, scope: !804)
!808 = !DILocation(line: 314, column: 40, scope: !804)
!809 = !DILocation(line: 314, column: 15, scope: !804)
!810 = !DILocation(line: 315, column: 13, scope: !804)
!811 = !DILocation(line: 316, column: 33, scope: !812)
!812 = distinct !DILexicalBlock(scope: !799, file: !2, line: 316, column: 17)
!813 = !DILocation(line: 316, column: 41, scope: !812)
!814 = !DILocation(line: 316, column: 49, scope: !812)
!815 = !DILocation(line: 316, column: 17, scope: !812)
!816 = !DILocation(line: 316, column: 59, scope: !812)
!817 = !DILocation(line: 316, column: 17, scope: !799)
!818 = !DILocation(line: 317, column: 22, scope: !819)
!819 = distinct !DILexicalBlock(scope: !812, file: !2, line: 316, column: 68)
!820 = !DILocation(line: 318, column: 31, scope: !819)
!821 = !DILocation(line: 318, column: 38, scope: !819)
!822 = !DILocation(line: 318, column: 15, scope: !819)
!823 = !DILocation(line: 319, column: 13, scope: !819)
!824 = !DILocation(line: 320, column: 9, scope: !799)
!825 = !DILocation(line: 321, column: 14, scope: !826)
!826 = distinct !DILexicalBlock(scope: !456, file: !2, line: 321, column: 13)
!827 = !DILocation(line: 321, column: 23, scope: !826)
!828 = !DILocation(line: 321, column: 32, scope: !826)
!829 = !DILocation(line: 321, column: 42, scope: !826)
!830 = !DILocation(line: 321, column: 35, scope: !826)
!831 = !DILocation(line: 321, column: 13, scope: !456)
!832 = !DILocation(line: 322, column: 18, scope: !833)
!833 = distinct !DILexicalBlock(scope: !826, file: !2, line: 321, column: 50)
!834 = !DILocation(line: 323, column: 26, scope: !833)
!835 = !DILocation(line: 323, column: 33, scope: !833)
!836 = !DILocation(line: 323, column: 11, scope: !833)
!837 = !DILocation(line: 324, column: 9, scope: !833)
!838 = !DILocation(line: 325, column: 9, scope: !456)
!839 = !DILocation(line: 333, column: 5, scope: !456)
!840 = !DILocation(line: 334, column: 9, scope: !841)
!841 = distinct !DILexicalBlock(scope: !426, file: !2, line: 334, column: 9)
!842 = !DILocation(line: 334, column: 17, scope: !841)
!843 = !DILocation(line: 334, column: 24, scope: !841)
!844 = !DILocation(line: 334, column: 9, scope: !426)
!845 = !DILocation(line: 335, column: 27, scope: !841)
!846 = !DILocation(line: 335, column: 9, scope: !841)
!847 = !DILocation(line: 335, column: 17, scope: !841)
!848 = !DILocation(line: 335, column: 25, scope: !841)
!849 = !DILocation(line: 336, column: 10, scope: !850)
!850 = distinct !DILexicalBlock(scope: !426, file: !2, line: 336, column: 9)
!851 = !DILocation(line: 336, column: 17, scope: !850)
!852 = !DILocation(line: 336, column: 23, scope: !850)
!853 = !DILocation(line: 336, column: 27, scope: !850)
!854 = !DILocation(line: 336, column: 35, scope: !850)
!855 = !DILocation(line: 336, column: 43, scope: !850)
!856 = !DILocation(line: 336, column: 9, scope: !426)
!857 = !DILocation(line: 337, column: 24, scope: !858)
!858 = distinct !DILexicalBlock(scope: !850, file: !2, line: 336, column: 49)
!859 = !DILocation(line: 337, column: 40, scope: !858)
!860 = !DILocation(line: 337, column: 7, scope: !858)
!861 = !DILocation(line: 338, column: 24, scope: !858)
!862 = !DILocation(line: 338, column: 32, scope: !858)
!863 = !DILocation(line: 338, column: 76, scope: !858)
!864 = !DILocation(line: 338, column: 87, scope: !858)
!865 = !DILocation(line: 338, column: 95, scope: !858)
!866 = !DILocation(line: 338, column: 7, scope: !858)
!867 = !DILocation(line: 339, column: 5, scope: !858)
!868 = !DILocation(line: 340, column: 12, scope: !426)
!869 = !DILocation(line: 340, column: 5, scope: !426)
!870 = distinct !DISubprogram(name: "ConvertDosToUnix", scope: !2, file: !2, line: 348, type: !427, scopeLine: 349, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !29, retainedNodes: !144)
!871 = !DILocalVariable(name: "ipInF", arg: 1, scope: !870, file: !2, line: 348, type: !155)
!872 = !DILocation(line: 348, column: 28, scope: !870)
!873 = !DILocalVariable(name: "ipOutF", arg: 2, scope: !870, file: !2, line: 348, type: !155)
!874 = !DILocation(line: 348, column: 41, scope: !870)
!875 = !DILocalVariable(name: "ipFlag", arg: 3, scope: !870, file: !2, line: 348, type: !32)
!876 = !DILocation(line: 348, column: 56, scope: !870)
!877 = !DILocalVariable(name: "progname", arg: 4, scope: !870, file: !2, line: 348, type: !213)
!878 = !DILocation(line: 348, column: 76, scope: !870)
!879 = !DILocalVariable(name: "RetVal", scope: !870, file: !2, line: 350, type: !38)
!880 = !DILocation(line: 350, column: 9, scope: !870)
!881 = !DILocalVariable(name: "PrevChar", scope: !870, file: !2, line: 351, type: !38)
!882 = !DILocation(line: 351, column: 9, scope: !870)
!883 = !DILocalVariable(name: "TempChar", scope: !870, file: !2, line: 352, type: !38)
!884 = !DILocation(line: 352, column: 9, scope: !870)
!885 = !DILocalVariable(name: "TempNextChar", scope: !870, file: !2, line: 353, type: !38)
!886 = !DILocation(line: 353, column: 9, scope: !870)
!887 = !DILocalVariable(name: "ConvTable", scope: !870, file: !2, line: 354, type: !888)
!888 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !38, size: 64)
!889 = !DILocation(line: 354, column: 10, scope: !870)
!890 = !DILocalVariable(name: "line_nr", scope: !870, file: !2, line: 355, type: !58)
!891 = !DILocation(line: 355, column: 18, scope: !870)
!892 = !DILocalVariable(name: "converted", scope: !870, file: !2, line: 356, type: !58)
!893 = !DILocation(line: 356, column: 18, scope: !870)
!894 = !DILocation(line: 358, column: 5, scope: !870)
!895 = !DILocation(line: 358, column: 13, scope: !870)
!896 = !DILocation(line: 358, column: 20, scope: !870)
!897 = !DILocation(line: 360, column: 13, scope: !870)
!898 = !DILocation(line: 360, column: 21, scope: !870)
!899 = !DILocation(line: 360, column: 5, scope: !870)
!900 = !DILocation(line: 364, column: 19, scope: !901)
!901 = distinct !DILexicalBlock(scope: !870, file: !2, line: 360, column: 31)
!902 = !DILocation(line: 365, column: 9, scope: !901)
!903 = !DILocation(line: 367, column: 19, scope: !901)
!904 = !DILocation(line: 368, column: 9, scope: !901)
!905 = !DILocation(line: 370, column: 19, scope: !901)
!906 = !DILocation(line: 371, column: 9, scope: !901)
!907 = !DILocation(line: 373, column: 19, scope: !901)
!908 = !DILocation(line: 374, column: 9, scope: !901)
!909 = !DILocation(line: 376, column: 19, scope: !901)
!910 = !DILocation(line: 377, column: 9, scope: !901)
!911 = !DILocation(line: 379, column: 19, scope: !901)
!912 = !DILocation(line: 380, column: 9, scope: !901)
!913 = !DILocation(line: 382, column: 19, scope: !901)
!914 = !DILocation(line: 383, column: 9, scope: !901)
!915 = !DILocation(line: 385, column: 19, scope: !901)
!916 = !DILocation(line: 386, column: 9, scope: !901)
!917 = !DILocation(line: 388, column: 9, scope: !901)
!918 = !DILocation(line: 388, column: 17, scope: !901)
!919 = !DILocation(line: 388, column: 24, scope: !901)
!920 = !DILocation(line: 389, column: 9, scope: !901)
!921 = !DILocation(line: 392, column: 9, scope: !922)
!922 = distinct !DILexicalBlock(scope: !870, file: !2, line: 392, column: 9)
!923 = !DILocation(line: 392, column: 17, scope: !922)
!924 = !DILocation(line: 392, column: 25, scope: !922)
!925 = !DILocation(line: 392, column: 9, scope: !870)
!926 = !DILocation(line: 393, column: 17, scope: !922)
!927 = !DILocation(line: 393, column: 7, scope: !922)
!928 = !DILocation(line: 395, column: 10, scope: !929)
!929 = distinct !DILexicalBlock(scope: !870, file: !2, line: 395, column: 9)
!930 = !DILocation(line: 395, column: 18, scope: !929)
!931 = !DILocation(line: 395, column: 27, scope: !929)
!932 = !DILocation(line: 395, column: 44, scope: !929)
!933 = !DILocation(line: 395, column: 48, scope: !929)
!934 = !DILocation(line: 395, column: 56, scope: !929)
!935 = !DILocation(line: 395, column: 47, scope: !929)
!936 = !DILocation(line: 395, column: 9, scope: !870)
!937 = !DILocation(line: 396, column: 25, scope: !938)
!938 = distinct !DILexicalBlock(scope: !929, file: !2, line: 395, column: 66)
!939 = !DILocation(line: 396, column: 41, scope: !938)
!940 = !DILocation(line: 396, column: 8, scope: !938)
!941 = !DILocation(line: 397, column: 25, scope: !938)
!942 = !DILocation(line: 397, column: 33, scope: !938)
!943 = !DILocation(line: 397, column: 61, scope: !938)
!944 = !DILocation(line: 397, column: 69, scope: !938)
!945 = !DILocation(line: 397, column: 8, scope: !938)
!946 = !DILocation(line: 398, column: 5, scope: !938)
!947 = !DILocation(line: 407, column: 13, scope: !870)
!948 = !DILocation(line: 407, column: 21, scope: !870)
!949 = !DILocation(line: 407, column: 5, scope: !870)
!950 = !DILocation(line: 409, column: 9, scope: !951)
!951 = distinct !DILexicalBlock(scope: !870, file: !2, line: 407, column: 33)
!952 = !DILocation(line: 409, column: 34, scope: !951)
!953 = !DILocation(line: 409, column: 28, scope: !951)
!954 = !DILocation(line: 409, column: 26, scope: !951)
!955 = !DILocation(line: 409, column: 42, scope: !951)
!956 = !DILocation(line: 410, column: 16, scope: !957)
!957 = distinct !DILexicalBlock(scope: !958, file: !2, line: 410, column: 15)
!958 = distinct !DILexicalBlock(scope: !951, file: !2, line: 409, column: 50)
!959 = !DILocation(line: 410, column: 24, scope: !957)
!960 = !DILocation(line: 410, column: 30, scope: !957)
!961 = !DILocation(line: 410, column: 36, scope: !957)
!962 = !DILocation(line: 411, column: 16, scope: !957)
!963 = !DILocation(line: 411, column: 25, scope: !957)
!964 = !DILocation(line: 411, column: 31, scope: !957)
!965 = !DILocation(line: 412, column: 16, scope: !957)
!966 = !DILocation(line: 412, column: 25, scope: !957)
!967 = !DILocation(line: 412, column: 36, scope: !957)
!968 = !DILocation(line: 413, column: 16, scope: !957)
!969 = !DILocation(line: 413, column: 25, scope: !957)
!970 = !DILocation(line: 413, column: 36, scope: !957)
!971 = !DILocation(line: 414, column: 16, scope: !957)
!972 = !DILocation(line: 414, column: 25, scope: !957)
!973 = !DILocation(line: 414, column: 36, scope: !957)
!974 = !DILocation(line: 415, column: 16, scope: !957)
!975 = !DILocation(line: 415, column: 25, scope: !957)
!976 = !DILocation(line: 410, column: 15, scope: !958)
!977 = !DILocation(line: 416, column: 20, scope: !978)
!978 = distinct !DILexicalBlock(scope: !957, file: !2, line: 415, column: 37)
!979 = !DILocation(line: 417, column: 13, scope: !978)
!980 = !DILocation(line: 417, column: 21, scope: !978)
!981 = !DILocation(line: 417, column: 28, scope: !978)
!982 = !DILocation(line: 418, column: 17, scope: !983)
!983 = distinct !DILexicalBlock(scope: !978, file: !2, line: 418, column: 17)
!984 = !DILocation(line: 418, column: 25, scope: !983)
!985 = !DILocation(line: 418, column: 17, scope: !978)
!986 = !DILocation(line: 419, column: 20, scope: !987)
!987 = distinct !DILexicalBlock(scope: !988, file: !2, line: 419, column: 19)
!988 = distinct !DILexicalBlock(scope: !983, file: !2, line: 418, column: 34)
!989 = !DILocation(line: 419, column: 28, scope: !987)
!990 = !DILocation(line: 419, column: 19, scope: !987)
!991 = !DILocation(line: 419, column: 40, scope: !987)
!992 = !DILocation(line: 419, column: 45, scope: !987)
!993 = !DILocation(line: 419, column: 53, scope: !987)
!994 = !DILocation(line: 419, column: 19, scope: !988)
!995 = !DILocation(line: 419, column: 61, scope: !987)
!996 = !DILocation(line: 419, column: 69, scope: !987)
!997 = !DILocation(line: 419, column: 75, scope: !987)
!998 = !DILocation(line: 420, column: 32, scope: !988)
!999 = !DILocation(line: 420, column: 48, scope: !988)
!1000 = !DILocation(line: 420, column: 15, scope: !988)
!1001 = !DILocation(line: 421, column: 32, scope: !988)
!1002 = !DILocation(line: 421, column: 40, scope: !988)
!1003 = !DILocation(line: 421, column: 85, scope: !988)
!1004 = !DILocation(line: 421, column: 95, scope: !988)
!1005 = !DILocation(line: 421, column: 15, scope: !988)
!1006 = !DILocation(line: 422, column: 13, scope: !988)
!1007 = !DILocation(line: 423, column: 13, scope: !978)
!1008 = !DILocation(line: 425, column: 15, scope: !1009)
!1009 = distinct !DILexicalBlock(scope: !958, file: !2, line: 425, column: 15)
!1010 = !DILocation(line: 425, column: 24, scope: !1009)
!1011 = !DILocation(line: 425, column: 15, scope: !958)
!1012 = !DILocation(line: 426, column: 17, scope: !1013)
!1013 = distinct !DILexicalBlock(scope: !1014, file: !2, line: 426, column: 17)
!1014 = distinct !DILexicalBlock(scope: !1009, file: !2, line: 425, column: 35)
!1015 = !DILocation(line: 426, column: 26, scope: !1013)
!1016 = !DILocation(line: 426, column: 17, scope: !1014)
!1017 = !DILocation(line: 427, column: 15, scope: !1013)
!1018 = !DILocation(line: 428, column: 23, scope: !1019)
!1019 = distinct !DILexicalBlock(scope: !1014, file: !2, line: 428, column: 17)
!1020 = !DILocation(line: 428, column: 33, scope: !1019)
!1021 = !DILocation(line: 428, column: 44, scope: !1019)
!1022 = !DILocation(line: 428, column: 17, scope: !1019)
!1023 = !DILocation(line: 428, column: 52, scope: !1019)
!1024 = !DILocation(line: 428, column: 17, scope: !1014)
!1025 = !DILocation(line: 429, column: 22, scope: !1026)
!1026 = distinct !DILexicalBlock(scope: !1019, file: !2, line: 428, column: 60)
!1027 = !DILocation(line: 430, column: 30, scope: !1026)
!1028 = !DILocation(line: 430, column: 37, scope: !1026)
!1029 = !DILocation(line: 430, column: 15, scope: !1026)
!1030 = !DILocation(line: 431, column: 15, scope: !1026)
!1031 = !DILocation(line: 433, column: 11, scope: !1014)
!1032 = !DILocation(line: 434, column: 33, scope: !1033)
!1033 = distinct !DILexicalBlock(scope: !1034, file: !2, line: 434, column: 17)
!1034 = distinct !DILexicalBlock(scope: !1009, file: !2, line: 433, column: 18)
!1035 = !DILocation(line: 434, column: 40, scope: !1033)
!1036 = !DILocation(line: 434, column: 48, scope: !1033)
!1037 = !DILocation(line: 434, column: 56, scope: !1033)
!1038 = !DILocation(line: 434, column: 78, scope: !1033)
!1039 = !DILocation(line: 434, column: 17, scope: !1033)
!1040 = !DILocation(line: 434, column: 88, scope: !1033)
!1041 = !DILocation(line: 434, column: 17, scope: !1034)
!1042 = !DILocation(line: 435, column: 22, scope: !1043)
!1043 = distinct !DILexicalBlock(scope: !1033, file: !2, line: 434, column: 96)
!1044 = !DILocation(line: 436, column: 15, scope: !1043)
!1045 = !DILocation(line: 439, column: 22, scope: !958)
!1046 = !DILocation(line: 439, column: 20, scope: !958)
!1047 = distinct !{!1047, !950, !1048, !557}
!1048 = !DILocation(line: 440, column: 9, scope: !951)
!1049 = !DILocation(line: 441, column: 13, scope: !1050)
!1050 = distinct !DILexicalBlock(scope: !951, file: !2, line: 441, column: 13)
!1051 = !DILocation(line: 441, column: 22, scope: !1050)
!1052 = !DILocation(line: 441, column: 29, scope: !1050)
!1053 = !DILocation(line: 441, column: 32, scope: !1050)
!1054 = !DILocation(line: 441, column: 40, scope: !1050)
!1055 = !DILocation(line: 441, column: 48, scope: !1050)
!1056 = !DILocation(line: 441, column: 51, scope: !1050)
!1057 = !DILocation(line: 441, column: 60, scope: !1050)
!1058 = !DILocation(line: 441, column: 67, scope: !1050)
!1059 = !DILocation(line: 441, column: 70, scope: !1050)
!1060 = !DILocation(line: 441, column: 79, scope: !1050)
!1061 = !DILocation(line: 441, column: 13, scope: !951)
!1062 = !DILocation(line: 443, column: 17, scope: !1063)
!1063 = distinct !DILexicalBlock(scope: !1064, file: !2, line: 443, column: 17)
!1064 = distinct !DILexicalBlock(scope: !1050, file: !2, line: 441, column: 90)
!1065 = !DILocation(line: 443, column: 25, scope: !1063)
!1066 = !DILocation(line: 443, column: 33, scope: !1063)
!1067 = !DILocation(line: 443, column: 17, scope: !1064)
!1068 = !DILocation(line: 444, column: 32, scope: !1069)
!1069 = distinct !DILexicalBlock(scope: !1063, file: !2, line: 443, column: 38)
!1070 = !DILocation(line: 444, column: 48, scope: !1069)
!1071 = !DILocation(line: 444, column: 15, scope: !1069)
!1072 = !DILocation(line: 445, column: 32, scope: !1069)
!1073 = !DILocation(line: 445, column: 40, scope: !1069)
!1074 = !DILocation(line: 445, column: 15, scope: !1069)
!1075 = !DILocation(line: 446, column: 13, scope: !1069)
!1076 = !DILocation(line: 447, column: 30, scope: !1077)
!1077 = distinct !DILexicalBlock(scope: !1064, file: !2, line: 447, column: 16)
!1078 = !DILocation(line: 447, column: 16, scope: !1077)
!1079 = !DILocation(line: 447, column: 38, scope: !1077)
!1080 = !DILocation(line: 447, column: 16, scope: !1064)
!1081 = !DILocation(line: 448, column: 22, scope: !1082)
!1082 = distinct !DILexicalBlock(scope: !1077, file: !2, line: 447, column: 46)
!1083 = !DILocation(line: 449, column: 30, scope: !1082)
!1084 = !DILocation(line: 449, column: 37, scope: !1082)
!1085 = !DILocation(line: 449, column: 15, scope: !1082)
!1086 = !DILocation(line: 450, column: 13, scope: !1082)
!1087 = !DILocation(line: 451, column: 9, scope: !1064)
!1088 = !DILocation(line: 452, column: 14, scope: !1089)
!1089 = distinct !DILexicalBlock(scope: !951, file: !2, line: 452, column: 13)
!1090 = !DILocation(line: 452, column: 23, scope: !1089)
!1091 = !DILocation(line: 452, column: 31, scope: !1089)
!1092 = !DILocation(line: 452, column: 41, scope: !1089)
!1093 = !DILocation(line: 452, column: 34, scope: !1089)
!1094 = !DILocation(line: 452, column: 13, scope: !951)
!1095 = !DILocation(line: 453, column: 18, scope: !1096)
!1096 = distinct !DILexicalBlock(scope: !1089, file: !2, line: 452, column: 49)
!1097 = !DILocation(line: 454, column: 26, scope: !1096)
!1098 = !DILocation(line: 454, column: 33, scope: !1096)
!1099 = !DILocation(line: 454, column: 11, scope: !1096)
!1100 = !DILocation(line: 455, column: 9, scope: !1096)
!1101 = !DILocation(line: 456, column: 9, scope: !951)
!1102 = !DILocation(line: 458, column: 9, scope: !951)
!1103 = !DILocation(line: 458, column: 34, scope: !951)
!1104 = !DILocation(line: 458, column: 28, scope: !951)
!1105 = !DILocation(line: 458, column: 26, scope: !951)
!1106 = !DILocation(line: 458, column: 42, scope: !951)
!1107 = !DILocation(line: 459, column: 16, scope: !1108)
!1108 = distinct !DILexicalBlock(scope: !1109, file: !2, line: 459, column: 15)
!1109 = distinct !DILexicalBlock(scope: !951, file: !2, line: 458, column: 50)
!1110 = !DILocation(line: 459, column: 24, scope: !1108)
!1111 = !DILocation(line: 459, column: 30, scope: !1108)
!1112 = !DILocation(line: 459, column: 36, scope: !1108)
!1113 = !DILocation(line: 460, column: 16, scope: !1108)
!1114 = !DILocation(line: 460, column: 25, scope: !1108)
!1115 = !DILocation(line: 460, column: 31, scope: !1108)
!1116 = !DILocation(line: 461, column: 16, scope: !1108)
!1117 = !DILocation(line: 461, column: 25, scope: !1108)
!1118 = !DILocation(line: 461, column: 36, scope: !1108)
!1119 = !DILocation(line: 462, column: 16, scope: !1108)
!1120 = !DILocation(line: 462, column: 25, scope: !1108)
!1121 = !DILocation(line: 462, column: 36, scope: !1108)
!1122 = !DILocation(line: 463, column: 16, scope: !1108)
!1123 = !DILocation(line: 463, column: 25, scope: !1108)
!1124 = !DILocation(line: 463, column: 36, scope: !1108)
!1125 = !DILocation(line: 464, column: 16, scope: !1108)
!1126 = !DILocation(line: 464, column: 25, scope: !1108)
!1127 = !DILocation(line: 459, column: 15, scope: !1109)
!1128 = !DILocation(line: 465, column: 20, scope: !1129)
!1129 = distinct !DILexicalBlock(scope: !1108, file: !2, line: 464, column: 37)
!1130 = !DILocation(line: 466, column: 13, scope: !1129)
!1131 = !DILocation(line: 466, column: 21, scope: !1129)
!1132 = !DILocation(line: 466, column: 28, scope: !1129)
!1133 = !DILocation(line: 467, column: 17, scope: !1134)
!1134 = distinct !DILexicalBlock(scope: !1129, file: !2, line: 467, column: 17)
!1135 = !DILocation(line: 467, column: 25, scope: !1134)
!1136 = !DILocation(line: 467, column: 17, scope: !1129)
!1137 = !DILocation(line: 468, column: 20, scope: !1138)
!1138 = distinct !DILexicalBlock(scope: !1139, file: !2, line: 468, column: 19)
!1139 = distinct !DILexicalBlock(scope: !1134, file: !2, line: 467, column: 34)
!1140 = !DILocation(line: 468, column: 28, scope: !1138)
!1141 = !DILocation(line: 468, column: 19, scope: !1138)
!1142 = !DILocation(line: 468, column: 40, scope: !1138)
!1143 = !DILocation(line: 468, column: 45, scope: !1138)
!1144 = !DILocation(line: 468, column: 53, scope: !1138)
!1145 = !DILocation(line: 468, column: 19, scope: !1139)
!1146 = !DILocation(line: 468, column: 61, scope: !1138)
!1147 = !DILocation(line: 468, column: 69, scope: !1138)
!1148 = !DILocation(line: 468, column: 75, scope: !1138)
!1149 = !DILocation(line: 469, column: 32, scope: !1139)
!1150 = !DILocation(line: 469, column: 48, scope: !1139)
!1151 = !DILocation(line: 469, column: 15, scope: !1139)
!1152 = !DILocation(line: 470, column: 32, scope: !1139)
!1153 = !DILocation(line: 470, column: 40, scope: !1139)
!1154 = !DILocation(line: 470, column: 85, scope: !1139)
!1155 = !DILocation(line: 470, column: 95, scope: !1139)
!1156 = !DILocation(line: 470, column: 15, scope: !1139)
!1157 = !DILocation(line: 471, column: 13, scope: !1139)
!1158 = !DILocation(line: 472, column: 13, scope: !1129)
!1159 = !DILocation(line: 474, column: 16, scope: !1160)
!1160 = distinct !DILexicalBlock(scope: !1109, file: !2, line: 474, column: 15)
!1161 = !DILocation(line: 474, column: 25, scope: !1160)
!1162 = !DILocation(line: 474, column: 15, scope: !1109)
!1163 = !DILocation(line: 475, column: 17, scope: !1164)
!1164 = distinct !DILexicalBlock(scope: !1165, file: !2, line: 475, column: 17)
!1165 = distinct !DILexicalBlock(scope: !1160, file: !2, line: 474, column: 37)
!1166 = !DILocation(line: 475, column: 26, scope: !1164)
!1167 = !DILocation(line: 475, column: 17, scope: !1165)
!1168 = !DILocation(line: 476, column: 15, scope: !1164)
!1169 = !DILocation(line: 477, column: 22, scope: !1170)
!1170 = distinct !DILexicalBlock(scope: !1165, file: !2, line: 477, column: 16)
!1171 = !DILocation(line: 477, column: 32, scope: !1170)
!1172 = !DILocation(line: 477, column: 43, scope: !1170)
!1173 = !DILocation(line: 477, column: 16, scope: !1170)
!1174 = !DILocation(line: 477, column: 51, scope: !1170)
!1175 = !DILocation(line: 477, column: 16, scope: !1165)
!1176 = !DILocation(line: 478, column: 22, scope: !1177)
!1177 = distinct !DILexicalBlock(scope: !1170, file: !2, line: 477, column: 59)
!1178 = !DILocation(line: 479, column: 30, scope: !1177)
!1179 = !DILocation(line: 479, column: 37, scope: !1177)
!1180 = !DILocation(line: 479, column: 15, scope: !1177)
!1181 = !DILocation(line: 480, column: 15, scope: !1177)
!1182 = !DILocation(line: 482, column: 11, scope: !1165)
!1183 = !DILocation(line: 484, column: 40, scope: !1184)
!1184 = distinct !DILexicalBlock(scope: !1185, file: !2, line: 484, column: 18)
!1185 = distinct !DILexicalBlock(scope: !1160, file: !2, line: 482, column: 17)
!1186 = !DILocation(line: 484, column: 34, scope: !1184)
!1187 = !DILocation(line: 484, column: 32, scope: !1184)
!1188 = !DILocation(line: 484, column: 48, scope: !1184)
!1189 = !DILocation(line: 484, column: 18, scope: !1185)
!1190 = !DILocation(line: 485, column: 27, scope: !1191)
!1191 = distinct !DILexicalBlock(scope: !1192, file: !2, line: 485, column: 19)
!1192 = distinct !DILexicalBlock(scope: !1184, file: !2, line: 484, column: 56)
!1193 = !DILocation(line: 485, column: 41, scope: !1191)
!1194 = !DILocation(line: 485, column: 19, scope: !1191)
!1195 = !DILocation(line: 485, column: 49, scope: !1191)
!1196 = !DILocation(line: 485, column: 19, scope: !1192)
!1197 = !DILocation(line: 486, column: 32, scope: !1198)
!1198 = distinct !DILexicalBlock(scope: !1191, file: !2, line: 485, column: 57)
!1199 = !DILocation(line: 486, column: 39, scope: !1198)
!1200 = !DILocation(line: 486, column: 17, scope: !1198)
!1201 = !DILocation(line: 487, column: 24, scope: !1198)
!1202 = !DILocation(line: 488, column: 17, scope: !1198)
!1203 = !DILocation(line: 491, column: 20, scope: !1204)
!1204 = distinct !DILexicalBlock(scope: !1192, file: !2, line: 491, column: 20)
!1205 = !DILocation(line: 491, column: 33, scope: !1204)
!1206 = !DILocation(line: 491, column: 20, scope: !1192)
!1207 = !DILocation(line: 492, column: 35, scope: !1208)
!1208 = distinct !DILexicalBlock(scope: !1209, file: !2, line: 492, column: 21)
!1209 = distinct !DILexicalBlock(scope: !1204, file: !2, line: 491, column: 45)
!1210 = !DILocation(line: 492, column: 21, scope: !1208)
!1211 = !DILocation(line: 492, column: 43, scope: !1208)
!1212 = !DILocation(line: 492, column: 21, scope: !1209)
!1213 = !DILocation(line: 493, column: 26, scope: !1214)
!1214 = distinct !DILexicalBlock(scope: !1208, file: !2, line: 492, column: 51)
!1215 = !DILocation(line: 494, column: 34, scope: !1214)
!1216 = !DILocation(line: 494, column: 41, scope: !1214)
!1217 = !DILocation(line: 494, column: 19, scope: !1214)
!1218 = !DILocation(line: 495, column: 19, scope: !1214)
!1219 = !DILocation(line: 497, column: 28, scope: !1209)
!1220 = !DILocation(line: 497, column: 26, scope: !1209)
!1221 = !DILocation(line: 498, column: 17, scope: !1209)
!1222 = distinct !{!1222, !1102, !1223, !557}
!1223 = !DILocation(line: 517, column: 9, scope: !951)
!1224 = !DILocation(line: 500, column: 13, scope: !1192)
!1225 = !DILocation(line: 501, column: 31, scope: !1226)
!1226 = distinct !DILexicalBlock(scope: !1185, file: !2, line: 501, column: 17)
!1227 = !DILocation(line: 501, column: 17, scope: !1226)
!1228 = !DILocation(line: 501, column: 39, scope: !1226)
!1229 = !DILocation(line: 501, column: 17, scope: !1185)
!1230 = !DILocation(line: 502, column: 22, scope: !1231)
!1231 = distinct !DILexicalBlock(scope: !1226, file: !2, line: 501, column: 47)
!1232 = !DILocation(line: 503, column: 30, scope: !1231)
!1233 = !DILocation(line: 503, column: 37, scope: !1231)
!1234 = !DILocation(line: 503, column: 15, scope: !1231)
!1235 = !DILocation(line: 504, column: 15, scope: !1231)
!1236 = !DILocation(line: 506, column: 22, scope: !1185)
!1237 = !DILocation(line: 507, column: 20, scope: !1185)
!1238 = !DILocation(line: 508, column: 17, scope: !1239)
!1239 = distinct !DILexicalBlock(scope: !1185, file: !2, line: 508, column: 17)
!1240 = !DILocation(line: 508, column: 25, scope: !1239)
!1241 = !DILocation(line: 508, column: 17, scope: !1185)
!1242 = !DILocation(line: 509, column: 33, scope: !1243)
!1243 = distinct !DILexicalBlock(scope: !1244, file: !2, line: 509, column: 19)
!1244 = distinct !DILexicalBlock(scope: !1239, file: !2, line: 508, column: 34)
!1245 = !DILocation(line: 509, column: 19, scope: !1243)
!1246 = !DILocation(line: 509, column: 41, scope: !1243)
!1247 = !DILocation(line: 509, column: 19, scope: !1244)
!1248 = !DILocation(line: 510, column: 24, scope: !1249)
!1249 = distinct !DILexicalBlock(scope: !1243, file: !2, line: 509, column: 49)
!1250 = !DILocation(line: 511, column: 32, scope: !1249)
!1251 = !DILocation(line: 511, column: 39, scope: !1249)
!1252 = !DILocation(line: 511, column: 17, scope: !1249)
!1253 = !DILocation(line: 512, column: 17, scope: !1249)
!1254 = !DILocation(line: 514, column: 13, scope: !1244)
!1255 = !DILocation(line: 516, column: 22, scope: !1109)
!1256 = !DILocation(line: 516, column: 20, scope: !1109)
!1257 = !DILocation(line: 518, column: 13, scope: !1258)
!1258 = distinct !DILexicalBlock(scope: !951, file: !2, line: 518, column: 13)
!1259 = !DILocation(line: 518, column: 22, scope: !1258)
!1260 = !DILocation(line: 518, column: 29, scope: !1258)
!1261 = !DILocation(line: 518, column: 32, scope: !1258)
!1262 = !DILocation(line: 518, column: 40, scope: !1258)
!1263 = !DILocation(line: 518, column: 48, scope: !1258)
!1264 = !DILocation(line: 518, column: 51, scope: !1258)
!1265 = !DILocation(line: 518, column: 60, scope: !1258)
!1266 = !DILocation(line: 518, column: 67, scope: !1258)
!1267 = !DILocation(line: 518, column: 72, scope: !1258)
!1268 = !DILocation(line: 518, column: 81, scope: !1258)
!1269 = !DILocation(line: 518, column: 91, scope: !1258)
!1270 = !DILocation(line: 518, column: 94, scope: !1258)
!1271 = !DILocation(line: 518, column: 103, scope: !1258)
!1272 = !DILocation(line: 518, column: 13, scope: !951)
!1273 = !DILocation(line: 520, column: 17, scope: !1274)
!1274 = distinct !DILexicalBlock(scope: !1275, file: !2, line: 520, column: 17)
!1275 = distinct !DILexicalBlock(scope: !1258, file: !2, line: 518, column: 115)
!1276 = !DILocation(line: 520, column: 25, scope: !1274)
!1277 = !DILocation(line: 520, column: 33, scope: !1274)
!1278 = !DILocation(line: 520, column: 17, scope: !1275)
!1279 = !DILocation(line: 521, column: 32, scope: !1280)
!1280 = distinct !DILexicalBlock(scope: !1274, file: !2, line: 520, column: 38)
!1281 = !DILocation(line: 521, column: 48, scope: !1280)
!1282 = !DILocation(line: 521, column: 15, scope: !1280)
!1283 = !DILocation(line: 522, column: 32, scope: !1280)
!1284 = !DILocation(line: 522, column: 40, scope: !1280)
!1285 = !DILocation(line: 522, column: 15, scope: !1280)
!1286 = !DILocation(line: 523, column: 13, scope: !1280)
!1287 = !DILocation(line: 524, column: 31, scope: !1288)
!1288 = distinct !DILexicalBlock(scope: !1275, file: !2, line: 524, column: 17)
!1289 = !DILocation(line: 524, column: 17, scope: !1288)
!1290 = !DILocation(line: 524, column: 39, scope: !1288)
!1291 = !DILocation(line: 524, column: 17, scope: !1275)
!1292 = !DILocation(line: 525, column: 22, scope: !1293)
!1293 = distinct !DILexicalBlock(scope: !1288, file: !2, line: 524, column: 47)
!1294 = !DILocation(line: 526, column: 30, scope: !1293)
!1295 = !DILocation(line: 526, column: 37, scope: !1293)
!1296 = !DILocation(line: 526, column: 15, scope: !1293)
!1297 = !DILocation(line: 527, column: 13, scope: !1293)
!1298 = !DILocation(line: 528, column: 9, scope: !1275)
!1299 = !DILocation(line: 529, column: 14, scope: !1300)
!1300 = distinct !DILexicalBlock(scope: !951, file: !2, line: 529, column: 13)
!1301 = !DILocation(line: 529, column: 23, scope: !1300)
!1302 = !DILocation(line: 529, column: 31, scope: !1300)
!1303 = !DILocation(line: 529, column: 41, scope: !1300)
!1304 = !DILocation(line: 529, column: 34, scope: !1300)
!1305 = !DILocation(line: 529, column: 13, scope: !951)
!1306 = !DILocation(line: 530, column: 18, scope: !1307)
!1307 = distinct !DILexicalBlock(scope: !1300, file: !2, line: 529, column: 49)
!1308 = !DILocation(line: 531, column: 26, scope: !1307)
!1309 = !DILocation(line: 531, column: 33, scope: !1307)
!1310 = !DILocation(line: 531, column: 11, scope: !1307)
!1311 = !DILocation(line: 532, column: 9, scope: !1307)
!1312 = !DILocation(line: 533, column: 9, scope: !951)
!1313 = !DILocation(line: 541, column: 5, scope: !951)
!1314 = !DILocation(line: 542, column: 10, scope: !1315)
!1315 = distinct !DILexicalBlock(scope: !870, file: !2, line: 542, column: 9)
!1316 = !DILocation(line: 542, column: 17, scope: !1315)
!1317 = !DILocation(line: 542, column: 23, scope: !1315)
!1318 = !DILocation(line: 542, column: 27, scope: !1315)
!1319 = !DILocation(line: 542, column: 35, scope: !1315)
!1320 = !DILocation(line: 542, column: 43, scope: !1315)
!1321 = !DILocation(line: 542, column: 9, scope: !870)
!1322 = !DILocation(line: 543, column: 24, scope: !1323)
!1323 = distinct !DILexicalBlock(scope: !1315, file: !2, line: 542, column: 49)
!1324 = !DILocation(line: 543, column: 40, scope: !1323)
!1325 = !DILocation(line: 543, column: 7, scope: !1323)
!1326 = !DILocation(line: 544, column: 24, scope: !1323)
!1327 = !DILocation(line: 544, column: 32, scope: !1323)
!1328 = !DILocation(line: 544, column: 75, scope: !1323)
!1329 = !DILocation(line: 544, column: 86, scope: !1323)
!1330 = !DILocation(line: 544, column: 94, scope: !1323)
!1331 = !DILocation(line: 544, column: 7, scope: !1323)
!1332 = !DILocation(line: 545, column: 5, scope: !1323)
!1333 = !DILocation(line: 546, column: 12, scope: !870)
!1334 = !DILocation(line: 546, column: 5, scope: !870)
!1335 = !DILocation(line: 547, column: 1, scope: !870)
!1336 = distinct !DISubprogram(name: "main", scope: !2, file: !2, line: 550, type: !1337, scopeLine: 551, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !29, retainedNodes: !144)
!1337 = !DISubroutineType(types: !1338)
!1338 = !{!38, !38, !1339}
!1339 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !163, size: 64)
!1340 = !DILocalVariable(name: "argc", arg: 1, scope: !1336, file: !2, line: 550, type: !38)
!1341 = !DILocation(line: 550, column: 15, scope: !1336)
!1342 = !DILocalVariable(name: "argv", arg: 2, scope: !1336, file: !2, line: 550, type: !1339)
!1343 = !DILocation(line: 550, column: 27, scope: !1336)
!1344 = !DILocalVariable(name: "progname", scope: !1336, file: !2, line: 553, type: !73)
!1345 = !DILocation(line: 553, column: 8, scope: !1336)
!1346 = !DILocalVariable(name: "pFlag", scope: !1336, file: !2, line: 554, type: !32)
!1347 = !DILocation(line: 554, column: 10, scope: !1336)
!1348 = !DILocalVariable(name: "ptr", scope: !1336, file: !2, line: 555, type: !163)
!1349 = !DILocation(line: 555, column: 9, scope: !1336)
!1350 = !DILocalVariable(name: "localedir", scope: !1336, file: !2, line: 556, type: !1351)
!1351 = !DICompositeType(tag: DW_TAG_array_type, baseType: !4, size: 8192, elements: !1352)
!1352 = !{!1353}
!1353 = !DISubrange(count: 1024)
!1354 = !DILocation(line: 556, column: 8, scope: !1336)
!1355 = !DILocalVariable(name: "ret", scope: !1336, file: !2, line: 557, type: !38)
!1356 = !DILocation(line: 557, column: 7, scope: !1336)
!1357 = !DILocalVariable(name: "argc_new", scope: !1336, file: !2, line: 561, type: !38)
!1358 = !DILocation(line: 561, column: 8, scope: !1336)
!1359 = !DILocalVariable(name: "argv_new", scope: !1336, file: !2, line: 562, type: !1339)
!1360 = !DILocation(line: 562, column: 10, scope: !1336)
!1361 = !DILocation(line: 568, column: 3, scope: !1336)
!1362 = !DILocation(line: 568, column: 15, scope: !1336)
!1363 = !DILocation(line: 569, column: 10, scope: !1336)
!1364 = !DILocation(line: 569, column: 3, scope: !1336)
!1365 = !DILocation(line: 572, column: 10, scope: !1336)
!1366 = !DILocation(line: 572, column: 8, scope: !1336)
!1367 = !DILocation(line: 573, column: 8, scope: !1368)
!1368 = distinct !DILexicalBlock(scope: !1336, file: !2, line: 573, column: 8)
!1369 = !DILocation(line: 573, column: 12, scope: !1368)
!1370 = !DILocation(line: 573, column: 8, scope: !1336)
!1371 = !DILocation(line: 574, column: 19, scope: !1368)
!1372 = !DILocation(line: 574, column: 7, scope: !1368)
!1373 = !DILocation(line: 576, column: 18, scope: !1374)
!1374 = distinct !DILexicalBlock(scope: !1375, file: !2, line: 576, column: 11)
!1375 = distinct !DILexicalBlock(scope: !1368, file: !2, line: 575, column: 9)
!1376 = !DILocation(line: 576, column: 11, scope: !1374)
!1377 = !DILocation(line: 576, column: 23, scope: !1374)
!1378 = !DILocation(line: 576, column: 11, scope: !1375)
!1379 = !DILocation(line: 577, column: 22, scope: !1374)
!1380 = !DILocation(line: 577, column: 32, scope: !1374)
!1381 = !DILocation(line: 577, column: 10, scope: !1374)
!1382 = !DILocation(line: 579, column: 27, scope: !1383)
!1383 = distinct !DILexicalBlock(scope: !1374, file: !2, line: 578, column: 12)
!1384 = !DILocation(line: 579, column: 41, scope: !1383)
!1385 = !DILocation(line: 579, column: 10, scope: !1383)
!1386 = !DILocation(line: 580, column: 27, scope: !1383)
!1387 = !DILocation(line: 580, column: 41, scope: !1383)
!1388 = !DILocation(line: 580, column: 10, scope: !1383)
!1389 = !DILocation(line: 581, column: 22, scope: !1383)
!1390 = !DILocation(line: 581, column: 10, scope: !1383)
!1391 = !DILocation(line: 595, column: 4, scope: !1336)
!1392 = !DILocation(line: 600, column: 29, scope: !1336)
!1393 = !DILocation(line: 600, column: 4, scope: !1336)
!1394 = !DILocation(line: 601, column: 4, scope: !1336)
!1395 = !DILocation(line: 606, column: 19, scope: !1336)
!1396 = !DILocation(line: 606, column: 9, scope: !1336)
!1397 = !DILocation(line: 607, column: 7, scope: !1398)
!1398 = distinct !DILexicalBlock(scope: !1336, file: !2, line: 607, column: 7)
!1399 = !DILocation(line: 607, column: 13, scope: !1398)
!1400 = !DILocation(line: 607, column: 7, scope: !1336)
!1401 = !DILocation(line: 608, column: 22, scope: !1402)
!1402 = distinct !DILexicalBlock(scope: !1398, file: !2, line: 607, column: 22)
!1403 = !DILocation(line: 608, column: 5, scope: !1402)
!1404 = !DILocation(line: 609, column: 22, scope: !1402)
!1405 = !DILocation(line: 609, column: 48, scope: !1402)
!1406 = !DILocation(line: 609, column: 39, scope: !1402)
!1407 = !DILocation(line: 609, column: 5, scope: !1402)
!1408 = !DILocation(line: 610, column: 12, scope: !1402)
!1409 = !DILocation(line: 610, column: 5, scope: !1402)
!1410 = !DILocation(line: 612, column: 3, scope: !1336)
!1411 = !DILocation(line: 612, column: 10, scope: !1336)
!1412 = !DILocation(line: 612, column: 21, scope: !1336)
!1413 = !DILocation(line: 613, column: 3, scope: !1336)
!1414 = !DILocation(line: 613, column: 10, scope: !1336)
!1415 = !DILocation(line: 613, column: 19, scope: !1336)
!1416 = !DILocation(line: 615, column: 22, scope: !1417)
!1417 = distinct !DILexicalBlock(scope: !1336, file: !2, line: 615, column: 8)
!1418 = !DILocation(line: 615, column: 14, scope: !1417)
!1419 = !DILocation(line: 615, column: 13, scope: !1417)
!1420 = !DILocation(line: 615, column: 36, scope: !1417)
!1421 = !DILocation(line: 615, column: 45, scope: !1417)
!1422 = !DILocation(line: 615, column: 62, scope: !1417)
!1423 = !DILocation(line: 615, column: 54, scope: !1417)
!1424 = !DILocation(line: 615, column: 53, scope: !1417)
!1425 = !DILocation(line: 615, column: 77, scope: !1417)
!1426 = !DILocation(line: 615, column: 8, scope: !1336)
!1427 = !DILocation(line: 616, column: 11, scope: !1417)
!1428 = !DILocation(line: 616, column: 9, scope: !1417)
!1429 = !DILocation(line: 616, column: 5, scope: !1417)
!1430 = !DILocation(line: 618, column: 8, scope: !1417)
!1431 = !DILocation(line: 620, column: 8, scope: !1432)
!1432 = distinct !DILexicalBlock(scope: !1336, file: !2, line: 620, column: 7)
!1433 = !DILocation(line: 620, column: 33, scope: !1432)
!1434 = !DILocation(line: 620, column: 39, scope: !1432)
!1435 = !DILocation(line: 620, column: 43, scope: !1432)
!1436 = !DILocation(line: 620, column: 72, scope: !1432)
!1437 = !DILocation(line: 620, column: 7, scope: !1336)
!1438 = !DILocation(line: 621, column: 5, scope: !1439)
!1439 = distinct !DILexicalBlock(scope: !1432, file: !2, line: 620, column: 79)
!1440 = !DILocation(line: 621, column: 12, scope: !1439)
!1441 = !DILocation(line: 621, column: 23, scope: !1439)
!1442 = !DILocation(line: 622, column: 12, scope: !1439)
!1443 = !DILocation(line: 622, column: 5, scope: !1439)
!1444 = !DILocation(line: 623, column: 3, scope: !1439)
!1445 = !DILocation(line: 642, column: 14, scope: !1336)
!1446 = !DILocation(line: 642, column: 12, scope: !1336)
!1447 = !DILocation(line: 643, column: 14, scope: !1336)
!1448 = !DILocation(line: 643, column: 12, scope: !1336)
!1449 = !DILocation(line: 647, column: 23, scope: !1336)
!1450 = !DILocation(line: 647, column: 33, scope: !1336)
!1451 = !DILocation(line: 647, column: 43, scope: !1336)
!1452 = !DILocation(line: 647, column: 50, scope: !1336)
!1453 = !DILocation(line: 647, column: 61, scope: !1336)
!1454 = !DILocation(line: 647, column: 9, scope: !1336)
!1455 = !DILocation(line: 647, column: 7, scope: !1336)
!1456 = !DILocation(line: 651, column: 8, scope: !1336)
!1457 = !DILocation(line: 651, column: 3, scope: !1336)
!1458 = !DILocation(line: 652, column: 10, scope: !1336)
!1459 = !DILocation(line: 652, column: 3, scope: !1336)
!1460 = !DILocation(line: 653, column: 1, scope: !1336)
