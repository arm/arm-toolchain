!===----------------------------------------------------------------------===!
! This directory can be used to add Integration tests involving multiple
! stages of the compiler (for eg. from Fortran to LLVM IR). It should not
! contain executable tests. We should only add tests here sparingly and only
! if there is no other way to test. Repeat this message in each test that is
! added to this directory and sub-directories.
!===----------------------------------------------------------------------===!

!RUN: %flang_fc1 -emit-llvm -fopenmp %s -o - | FileCheck %s

program tp1
  real, save :: eq_a, eq_b
  equivalence(eq_a, eq_b)
  !$omp threadprivate(eq_a)
end program tp1

! check we don't crash
! CHECK: call ptr @__kmpc_threadprivate_cached(
