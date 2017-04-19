# Kernel Splitting

This project aims to automatically split OpenCL kernels in order to execute them
onto several devices (CPUs/GPUs) balancing the load automatically among iterations.


### Description


The project is composed of 5 parts :

#### ClInline and ClTransform

Those tools perform source-to-source transformations over OpenCL C codes.
ClInline allows to have to whole kernel into a single LLVM function in
order to avoid an inter-procedural analysis.
ClTransform makes the appropriate transformations so that the kernel can be
executed with a fraction of the NDRange.

#### LibKernelExpr

This library implements data structures needed to store parametric buffer
regions accessed by a kernel depending on the NDRange.

#### AnalysisPass

This LLVM pass analyses OpenCL Kernels in SPIR intermediate representation in
order to compute the parametric buffer regions accessed by a kernel.

#### Libsplit

This library interpose OpenCL API calls in order to automatically split the
kernels in a manner transparent to the user.

At kernel creation, the kernel is analyzed using AnalysisPass and transformed
using ClInline and ClTransform.

At kernel execution, a scheduler determine a partitioning of the kernel among
the devices, then the parametric analysis is instantiated with this partition in
order to determine how to split the data. Finally the data required by each
sub-kernel is sent to the devices and the sub-kernels are executed.

### Prerequisites

This project clang/LLVM 3.6 (see CLTransform/README) and clang/LLVM 3.2 to
support SPIR (see AnalysisPass/README).

### Installing

First compile LibKernelExpr : see LibKernelExpr/README.

Then compile AnalysisPass : see AnalysisPass/README.

Then compile ClInline : see ClInline/README

Then compile ClTransform : see ClTransform/README.

To finish compile libsplit : see libsplit/README.

### Usage

Edit the file bin/lisplit, then add bin/ to your PATH.

To display all available options :

```
HELP=1 libsplit ./my-opencl-app
```

To split the kernels onto 2 devices

```
DEVICES="<platform0> <device0> <platform1> <device1> ./my-opencl-app
```
