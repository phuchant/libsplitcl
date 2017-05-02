#ifndef DEFINE_H
#define DEFINE_H

namespace libsplit {

#define OPENCLFLAGS "-cc1 -cl-std=CL1.2 "		\
  "-emit-llvm-bc "				\
  "-isystem " LLVM_LIB_DIR "/clang/3.9.1/include/ -finclude-default-header" \
  " -O0"

#define OPTPASSES							\
  "-targetlibinfo -tbaa -basicaa -globalopt -ipsccp -deadargelim " \
  "-simplifycfg -basiccg -prune-eh -always-inline -functionattrs -sroa " \
  "-domtree -early-cse -lazy-value-info -jump-threading " \
  "-correlated-propagation -simplifycfg -tailcallelim -simplifycfg "	\
  "-reassociate -domtree -loops -loop-simplify -lcssa -loop-rotate -licm " \
  "-lcssa -loop-unswitch -scalar-evolution -loop-simplify -lcssa -indvars " \
  "-loop-idiom -loop-deletion -loop-unroll -memdep -memcpyopt -sccp "	\
  "-lazy-value-info -jump-threading -correlated-propagation -domtree -memdep " \
  "-dse -adce -simplifycfg -strip-dead-prototypes -domtree -verify"


#define PASSARG "-klanalysis"

#define QUOTE(name) #name
#define STR(macro) QUOTE(macro)

};

#endif /* DEFINE_H */
