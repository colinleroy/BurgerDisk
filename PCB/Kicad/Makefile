SUBDIRS = \
	THT_AngledDB19 \
	THT_DB19_IDC20_Adapter \
	THT_SolderTypeDB19

SRCDIR=$(shell pwd)
OUTDIR=../Gerbers

all clean:
	for dir in $(SUBDIRS); do \
		cd $(SRCDIR); \
		rm -rf $(OUTDIR)/$$dir $(OUTDIR)/$$dir.zip && \
		mkdir -p $(OUTDIR)/$$dir && \
		kicad-cli pcb export gerbers -o $(OUTDIR)/$$dir/ $$dir/*pcb && \
		kicad-cli pcb export drill -o $(OUTDIR)/$$dir/ $$dir/*pcb && \
		cd $(OUTDIR)/$$dir/ && \
		zip ../$$dir.zip *; \
	done
