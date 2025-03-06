all:
	gcc -Wall -o cook cook.c
	gcc -Wall -o waiter waiter.c
	gcc -Wall -o customer customer.c

run:
	./cook &
	sleep 1
	./waiter &
	sleep 1
	./customer

db:
	gcc -Wall -o gencustomers gencustomers.c
	./gencustomers > customers.txt
clean:
	-rm -f cook waiter customer gencustomers
