sfslib : cJSON.o
	#gcc -Wall -lcurl -c -g ../cJSON/cJSON.o sfslib.c -o sfslib.o -I.:../cJSON -lm
	#gcc -lcurl -c sfslib.c -o sfslib.o
	gcc -Wall -lcurl sfslib.c ../cJSON/cJSON.o -o sfslib -I.:../cJSON -lm

cJSON.o : ../cJSON/cJSON.h
	gcc -g -Wall -c ../cJSON/cJSON.c -o ../cJSON/cJSON.o
    
clean:
	#rm -f *.o
	rm -f sfslib *.o
