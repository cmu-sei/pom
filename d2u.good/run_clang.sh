#!/bin/bash

ast_out_dir=$1

if [ "$#" -ne 1 ]; then
    echo "Usage $0 ast_out_dir"
    exit 1
fi
if [ ! -d "$ast_out_dir" ]; then
    echo "Error: Output directory ($ast_out_dir) does not exist!"
    exit 1
fi
ast_out_dir=`realpath $ast_out_dir`
if [ "$CLANG" == "" ]; then
    CLANG="clang-$CLANG_VER"
fi;
if [ "$CLANG_PLUSPLUS" == "" ]; then
    CLANG_PLUSPLUS="clang++-$CLANG_VER"
fi;
echo -e '
' Building dos2unix
cd /datasets/dos2unix-7.5.2
$CLANG '-DVER_REVISION="7.5.2"' '-DVER_DATE="2024-01-22"' '-DVER_AUTHOR="Erwin Waterlander"' -DDEBUG=0 -DD2U_UNICODE -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -DENABLE_NLS '-DLOCALEDIR="/usr/share/locale"' '-DPACKAGE="dos2unix"' -O2 -Wall -Wextra -Wconversion -c dos2unix.c -Xclang -ast-dump=json -fsyntax-only 2> $ast_out_dir/dos2unix.955979806ce1344d61d417cc.raw.stderr.txt | gzip > $ast_out_dir/dos2unix.955979806ce1344d61d417cc.raw.ast.json.gz; echo $? > $ast_out_dir/dos2unix.955979806ce1344d61d417cc.raw.retcode.txt
$CLANG '-DVER_REVISION="7.5.2"' '-DVER_DATE="2024-01-22"' '-DVER_AUTHOR="Erwin Waterlander"' -DDEBUG=0 -DD2U_UNICODE -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -DENABLE_NLS '-DLOCALEDIR="/usr/share/locale"' '-DPACKAGE="dos2unix"' -O2 -Wall -Wextra -Wconversion -c dos2unix.c -Xclang -disable-O0-optnone -g -S -O0 -fno-inline -emit-llvm -fno-discard-value-names -o $ast_out_dir/dos2unix.955979806ce1344d61d417cc.raw.ll
echo -e '
' Building querycp
cd /datasets/dos2unix-7.5.2
$CLANG '-DVER_REVISION="7.5.2"' '-DVER_DATE="2024-01-22"' '-DVER_AUTHOR="Erwin Waterlander"' -DDEBUG=0 -DD2U_UNICODE -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -DENABLE_NLS '-DLOCALEDIR="/usr/share/locale"' '-DPACKAGE="dos2unix"' -O2 -Wall -Wextra -Wconversion -c querycp.c -Xclang -ast-dump=json -fsyntax-only 2> $ast_out_dir/querycp.8654988538a257e3708d2bad.raw.stderr.txt | gzip > $ast_out_dir/querycp.8654988538a257e3708d2bad.raw.ast.json.gz; echo $? > $ast_out_dir/querycp.8654988538a257e3708d2bad.raw.retcode.txt
$CLANG '-DVER_REVISION="7.5.2"' '-DVER_DATE="2024-01-22"' '-DVER_AUTHOR="Erwin Waterlander"' -DDEBUG=0 -DD2U_UNICODE -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -DENABLE_NLS '-DLOCALEDIR="/usr/share/locale"' '-DPACKAGE="dos2unix"' -O2 -Wall -Wextra -Wconversion -c querycp.c -Xclang -disable-O0-optnone -g -S -O0 -fno-inline -emit-llvm -fno-discard-value-names -o $ast_out_dir/querycp.8654988538a257e3708d2bad.raw.ll
echo -e '
' Building common
cd /datasets/dos2unix-7.5.2
$CLANG '-DVER_REVISION="7.5.2"' '-DVER_DATE="2024-01-22"' '-DVER_AUTHOR="Erwin Waterlander"' -DDEBUG=0 -DD2U_UNICODE -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -DENABLE_NLS '-DLOCALEDIR="/usr/share/locale"' '-DPACKAGE="dos2unix"' -O2 -Wall -Wextra -Wconversion -c common.c -Xclang -ast-dump=json -fsyntax-only 2> $ast_out_dir/common.5ba35ca31f1cf1a66823b734.raw.stderr.txt | gzip > $ast_out_dir/common.5ba35ca31f1cf1a66823b734.raw.ast.json.gz; echo $? > $ast_out_dir/common.5ba35ca31f1cf1a66823b734.raw.retcode.txt
$CLANG '-DVER_REVISION="7.5.2"' '-DVER_DATE="2024-01-22"' '-DVER_AUTHOR="Erwin Waterlander"' -DDEBUG=0 -DD2U_UNICODE -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -DENABLE_NLS '-DLOCALEDIR="/usr/share/locale"' '-DPACKAGE="dos2unix"' -O2 -Wall -Wextra -Wconversion -c common.c -Xclang -disable-O0-optnone -g -S -O0 -fno-inline -emit-llvm -fno-discard-value-names -o $ast_out_dir/common.5ba35ca31f1cf1a66823b734.raw.ll
