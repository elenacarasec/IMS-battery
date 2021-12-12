
PNAME = ims-battery
PSOURCES = *.cpp
PFLAGS = -lsimlib -lm

CC = g++
CFLAGS = -g -O2 -Wall -Wextra -p

all: $(PNAME)

rebuild: clean all

clean: 
	rm -f $(PNAME) 

$(PNAME): $(PSOURCES)
	$(CC) $(CFLAGS) $(PSOURCES) $(PFLAGS) -o ./$(PNAME)