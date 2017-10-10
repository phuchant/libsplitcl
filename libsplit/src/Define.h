#ifndef DEFINE_H
#define DEFINE_H

namespace libsplit {

#if CLANGMAJOR >= 5
#define OPENCLFLAGS "-Xclang -finclude-default-header -c -emit-llvm "	\
  "-cl-std=CL1.2"

#define OPTPASSES "-O2"
#else
#define OPENCLFLAGS "-cc1 -cl-std=CL1.2 "              \
  "-emit-llvm-bc "				       \
  "-finclude-default-header"			       \
  " -O0"

#define OPTPASSES                                                      \
  "-targetlibinfo -tbaa -basicaa -globalopt -ipsccp -deadargelim "	\
  "-simplifycfg -basiccg -prune-eh -always-inline -functionattrs -sroa " \
  "-domtree -early-cse -lazy-value-info -jump-threading "		\
  "-correlated-propagation -simplifycfg -tailcallelim -simplifycfg "	\
  "-reassociate -domtree -loops -loop-simplify -lcssa -loop-rotate -licm " \
  "-lcssa -loop-unswitch -scalar-evolution -loop-simplify -lcssa -indvars " \
  "-loop-idiom -loop-deletion -loop-unroll -memdep -memcpyopt -sccp "	\
  "-lazy-value-info -jump-threading -correlated-propagation -domtree -memdep " \
  "-dse -adce -simplifycfg -strip-dead-prototypes -domtree -loop-reduce -verify"
#endif

#define PASSARG "-klanalysis"

#define QUOTE(name) #name
#define STR(macro) QUOTE(macro)

};

#endif /* DEFINE_H */
