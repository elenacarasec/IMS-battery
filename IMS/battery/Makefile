# CPP=g++

# all: ims-battery.o

# $(CPP) -o ims-battery ims-battery.o -lsimlib -lm
# 	rm -f *.o

# clean:
# 	rm -f ims-battery

# run: 
# 	./ims-battery
	

# test:
# 	./ims-battery
# 	cat stats.out

################# PROJECT #########################

PNAME = ims-battery
PSOURCES = *.cpp
PFLAGS = -lsimlib -lm

################# FLAGS ###########################

CC = g++
CFLAGS = -g -O2

################# SCRIPTS #########################

all: $(PNAME)

rebuild: clean all

clean: 
	rm -f $(PNAME) 

$(PNAME): $(PSOURCES)
	$(CC) $(CFLAGS) $(PSOURCES) $(PFLAGS) -o ./$(PNAME)