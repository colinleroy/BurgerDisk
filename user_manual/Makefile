all: dist

clean:
		rm -f *.pdf

dist: clean
	libreoffice --headless --convert-to pdf *.odt

upload: dist
	scp *.pdf colino.net:www/wordpress/wp-content/
	ssh colino.net "curl -X PURGE https://www.colino.net/wordpress/wp-content/burgerdisk-user-manual.pdf"
