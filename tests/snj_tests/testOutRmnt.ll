; ModuleID = 'RW.c'
source_filename = "RW.c"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

%union.pthread_attr_t = type { i64, [48 x i8] }

@x = common global i32 0, align 4, !dbg !0

; Function Attrs: noinline nounwind optnone uwtable
define i8* @t0(i8*) #0 !dbg !13 {
  %2 = alloca i8*, align 8
  store i8* %0, i8** %2, align 8
  call void @llvm.dbg.declare(metadata i8** %2, metadata !16, metadata !DIExpression()), !dbg !17
  store i32 1, i32* @x, align 4, !dbg !18
  ret i8* null, !dbg !19
}

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.declare(metadata, metadata, metadata) #1

; Function Attrs: noinline nounwind optnone uwtable
define i8* @t1(i8*) #0 !dbg !20 {
  %2 = alloca i8*, align 8
  %3 = alloca i32, align 4
  store i8* %0, i8** %2, align 8
  call void @llvm.dbg.declare(metadata i8** %2, metadata !21, metadata !DIExpression()), !dbg !22
  call void @llvm.dbg.declare(metadata i32* %3, metadata !23, metadata !DIExpression()), !dbg !24
  %4 = load i32, i32* @x, align 4, !dbg !25
  store i32 %4, i32* %3, align 4, !dbg !24
  ret i8* null, !dbg !26
}

; Function Attrs: noinline nounwind optnone uwtable
define i32 @main(i32, i8**) #0 !dbg !27 {
  %3 = alloca i32, align 4
  %4 = alloca i32, align 4
  %5 = alloca i8**, align 8
  %6 = alloca i64, align 8
  %7 = alloca i64, align 8
  store i32 0, i32* %3, align 4
  store i32 %0, i32* %4, align 4
  call void @llvm.dbg.declare(metadata i32* %4, metadata !33, metadata !DIExpression()), !dbg !34
  store i8** %1, i8*** %5, align 8
  call void @llvm.dbg.declare(metadata i8*** %5, metadata !35, metadata !DIExpression()), !dbg !36
  call void @llvm.dbg.declare(metadata i64* %6, metadata !37, metadata !DIExpression()), !dbg !41
  call void @llvm.dbg.declare(metadata i64* %7, metadata !42, metadata !DIExpression()), !dbg !43
  store i32 0, i32* @x, align 4, !dbg !44
  %8 = call i32 @pthread_create(i64* %6, %union.pthread_attr_t* null, i8* (i8*)* @t0, i8* null) #4, !dbg !45
  %9 = call i32 @pthread_create(i64* %7, %union.pthread_attr_t* null, i8* (i8*)* @t1, i8* null) #4, !dbg !46
  %10 = load i64, i64* %6, align 8, !dbg !47
  %11 = call i32 @pthread_join(i64 %10, i8** null), !dbg !48
  %12 = load i64, i64* %7, align 8, !dbg !49
  %13 = call i32 @pthread_join(i64 %12, i8** null), !dbg !50
  ret i32 0, !dbg !51
}

; Function Attrs: nounwind
declare i32 @pthread_create(i64*, %union.pthread_attr_t*, i8* (i8*)*, i8*) #2

declare i32 @pthread_join(i64, i8**) #3

attributes #0 = { noinline nounwind optnone uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { nounwind readnone speculatable }
attributes #2 = { nounwind "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #3 = { "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #4 = { nounwind }

!llvm.dbg.cu = !{!2}
!llvm.module.flags = !{!9, !10, !11}
!llvm.ident = !{!12}

!0 = !DIGlobalVariableExpression(var: !1, expr: !DIExpression())
!1 = distinct !DIGlobalVariable(name: "x", scope: !2, file: !3, line: 4, type: !8, isLocal: false, isDefinition: true)
!2 = distinct !DICompileUnit(language: DW_LANG_C99, file: !3, producer: "clang version 6.0.0-1ubuntu2 (tags/RELEASE_600/final)", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, enums: !4, retainedTypes: !5, globals: !7)
!3 = !DIFile(filename: "RW.c", directory: "/home/ramneet/project/nidhugg-snj/viewEqSMC/tests/snj_tests")
!4 = !{}
!5 = !{!6}
!6 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: null, size: 64)
!7 = !{!0}
!8 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!9 = !{i32 2, !"Dwarf Version", i32 4}
!10 = !{i32 2, !"Debug Info Version", i32 3}
!11 = !{i32 1, !"wchar_size", i32 4}
!12 = !{!"clang version 6.0.0-1ubuntu2 (tags/RELEASE_600/final)"}
!13 = distinct !DISubprogram(name: "t0", scope: !3, file: !3, line: 6, type: !14, isLocal: false, isDefinition: true, scopeLine: 6, flags: DIFlagPrototyped, isOptimized: false, unit: !2, variables: !4)
!14 = !DISubroutineType(types: !15)
!15 = !{!6, !6}
!16 = !DILocalVariable(name: "arg", arg: 1, scope: !13, file: !3, line: 6, type: !6)
!17 = !DILocation(line: 6, column: 16, scope: !13)
!18 = !DILocation(line: 7, column: 5, scope: !13)
!19 = !DILocation(line: 9, column: 3, scope: !13)
!20 = distinct !DISubprogram(name: "t1", scope: !3, file: !3, line: 12, type: !14, isLocal: false, isDefinition: true, scopeLine: 12, flags: DIFlagPrototyped, isOptimized: false, unit: !2, variables: !4)
!21 = !DILocalVariable(name: "arg", arg: 1, scope: !20, file: !3, line: 12, type: !6)
!22 = !DILocation(line: 12, column: 16, scope: !20)
!23 = !DILocalVariable(name: "a", scope: !20, file: !3, line: 13, type: !8)
!24 = !DILocation(line: 13, column: 7, scope: !20)
!25 = !DILocation(line: 13, column: 11, scope: !20)
!26 = !DILocation(line: 15, column: 3, scope: !20)
!27 = distinct !DISubprogram(name: "main", scope: !3, file: !3, line: 18, type: !28, isLocal: false, isDefinition: true, scopeLine: 18, flags: DIFlagPrototyped, isOptimized: false, unit: !2, variables: !4)
!28 = !DISubroutineType(types: !29)
!29 = !{!8, !8, !30}
!30 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !31, size: 64)
!31 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !32, size: 64)
!32 = !DIBasicType(name: "char", size: 8, encoding: DW_ATE_signed_char)
!33 = !DILocalVariable(name: "argc", arg: 1, scope: !27, file: !3, line: 18, type: !8)
!34 = !DILocation(line: 18, column: 14, scope: !27)
!35 = !DILocalVariable(name: "argv", arg: 2, scope: !27, file: !3, line: 18, type: !30)
!36 = !DILocation(line: 18, column: 26, scope: !27)
!37 = !DILocalVariable(name: "thr0", scope: !27, file: !3, line: 19, type: !38)
!38 = !DIDerivedType(tag: DW_TAG_typedef, name: "pthread_t", file: !39, line: 27, baseType: !40)
!39 = !DIFile(filename: "/usr/include/x86_64-linux-gnu/bits/pthreadtypes.h", directory: "/home/ramneet/project/nidhugg-snj/viewEqSMC/tests/snj_tests")
!40 = !DIBasicType(name: "long unsigned int", size: 64, encoding: DW_ATE_unsigned)
!41 = !DILocation(line: 19, column: 13, scope: !27)
!42 = !DILocalVariable(name: "thr1", scope: !27, file: !3, line: 20, type: !38)
!43 = !DILocation(line: 20, column: 13, scope: !27)
!44 = !DILocation(line: 22, column: 5, scope: !27)
!45 = !DILocation(line: 24, column: 3, scope: !27)
!46 = !DILocation(line: 25, column: 3, scope: !27)
!47 = !DILocation(line: 27, column: 16, scope: !27)
!48 = !DILocation(line: 27, column: 3, scope: !27)
!49 = !DILocation(line: 28, column: 16, scope: !27)
!50 = !DILocation(line: 28, column: 3, scope: !27)
!51 = !DILocation(line: 30, column: 3, scope: !27)