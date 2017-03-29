#-----------------------------
SOURCE=web.cpp
MYPROGRAM=web
CC=g++
MYLIBRARIES=curl
FLAGS=Wall
#----------------------------
all: $(MYPROGRAM)
	
$(MYPROGRAM): $(SOURCE)
	
	$(CC) $(SOURCE) -o $(MYPROGRAM) -l$(MYLIBRARIES) -$(FLAGS) 

clean:
	
	rm $(MYPROGRAM) *.csv
