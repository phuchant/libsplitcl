all: ClInline ClTransform LibKernelExpr AnalysisPass libsplit

.PHONY: clean ClInline ClTransform LibKernelExpr AnalysisPass libsplit bin

ClInline:
	make -C ClInline

ClTransform:
	make -C ClTransform

LibKernelExpr:
	make -C LibKernelExpr

AnalysisPass:
	make -C AnalysisPass

libsplit:
	make -C libsplit

bin:
	echo 'LD_PRELOAD=$(PWD)/libsplit/lib/libhookocl.so $$*' > bin/libsplit
	chmod +x bin/libsplit

clean:
	make clean -C ClInline
	make clean -C ClTransform
	make clean -C LibKernelExpr
	make clean -C AnalysisPass
	make clean -C libsplit
