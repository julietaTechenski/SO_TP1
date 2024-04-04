COMPILER=gcc
FLAGS=-Wall

all: app child view

# run: ./app files/* | ./view.c

app: ./app.c
	$(COMPILER) $^ $(FLAGS) -o $@

child: ./child.c
	$(COMPILER) $^ $(FLAGS) -o $@

view: ./view.c
	$(COMPILER) $^ $(FLAGS) -o $@

clean:
	rm -f app child view

.PHONY: all clean app child view