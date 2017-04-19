#ifndef DEFINE_H
#define DEFINE_H

#define SPIRFLAGS "-cc1 -cl-std=CL1.2 "		\
  "-emit-llvm-bc "				\
  "-triple spir64-unknown-unknown "		\
  "-include " SPIRHEADER			\
  " -O0"

#define OPTPASSES \
  "-targetlibinfo -no-aa -tbaa -basicaa -globalopt -ipsccp -deadargelim " \
  "-simplifycfg -basiccg -prune-eh -always-inline -functionattrs -sroa " \
  "-domtree -early-cse -simplify-libcalls -lazy-value-info -jump-threading " \
  "-correlated-propagation -simplifycfg -tailcallelim -simplifycfg " \
  "-reassociate -domtree -loops -loop-simplify -lcssa -loop-rotate -licm " \
  "-lcssa -loop-unswitch -scalar-evolution -loop-simplify -lcssa -indvars " \
  "-loop-idiom -loop-deletion -loop-unroll -memdep -memcpyopt -sccp " \
  "-lazy-value-info -jump-threading -correlated-propagation -domtree -memdep " \
  "-dse -adce -simplifycfg -strip-dead-prototypes -preverify -domtree -verify"


#define PASSARG "-klanalysis"

#define SPIRPASS "-spirtransform"

#define MAXMERGEARGS 7

#define QUOTE(name) #name
#define STR(macro) QUOTE(macro)

#endif /* DEFINE_H */
