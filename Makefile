COMPILER=gcc
FLAGS=-Wall -g

all: app child view

app: ./app.c
	$(COMPILER) $^ $(FLAGS) -o $@

child: ./child.c
	$(COMPILER) $^ $(FLAGS) -o $@

view: ./view.c
	$(COMPILER) $^ $(FLAGS) -o $@

clean:
	rm -f app child view md5_output.txt

.PHONY: all clean app child view