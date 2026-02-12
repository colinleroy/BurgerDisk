SUBDIRS = \
	BurgerDisk-THT-AngledDB19 \
	BurgerDisk-THT-SolderTypeDB19 \
	DominoDisk-SMD \
	DominoDisk-THT \
	Small-SMD \
	Small-atmega-SMD \
	DB19-IDC20-Adapter

SRCDIR=$(shell pwd)
TOOLDIR=$(SRCDIR)/tools
OUTDIR=$(SRCDIR)/Gerbers

all clean:
	for dir in $(SUBDIRS); do \
		cd $(SRCDIR); \
		rm -rf $(OUTDIR)/$$dir $(OUTDIR)/$$dir.zip && \
		mkdir -p $(OUTDIR)/$$dir && \
		kicad-cli pcb export gerbers -o $(OUTDIR)/$$dir/ $$dir/*pcb && \
		kicad-cli pcb export drill -o $(OUTDIR)/$$dir/ $$dir/*pcb && \
		kicad-cli pcb export pos -o $(OUTDIR)/$$dir.jlcpcb_pos.csv --format csv --units mm --side both $$dir/*pcb && \
		kicad-cli sch export python-bom -o $(OUTDIR)/$$dir/$$dir.bom.xml $$dir/*sch && \
		cd $(OUTDIR)/$$dir/ && \
		xsltproc -o $(OUTDIR)/$$dir.jlcpcb_bom.csv $(TOOLDIR)/bom2grouped_csv_jlcpcb.xsl $$dir.bom.xml && \
		echo >> $(OUTDIR)/$$dir.jlcpcb_bom.csv && \
		sed -i "s/^Ref,Val.*$$/Designator,Val,Package,Mid X,Mid Y,Rotation,Layer/" $(OUTDIR)/$$dir.jlcpcb_pos.csv && \
		$(TOOLDIR)/add_nano_headers.sh $(OUTDIR)/$$dir.jlcpcb_bom.csv $(OUTDIR)/$$dir.jlcpcb_pos.csv && \
		zip ../$$dir.zip * && \
		cd .. && \
		rm -rf $(OUTDIR)/$$dir; \
	done
