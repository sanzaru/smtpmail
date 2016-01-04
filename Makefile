CC=cc
OPTS=-Wall --pedantic -O3
INCLUDE=
LIBS=-lssl
UNAME_S := $(shell uname -s)

# MacOS X requirements
ifeq ($(UNAME_S),Darwin)
	INCLUDE += -I /opt/local/include/
    LIBS += -lcrypto
endif

all:
	$(CC) $(OPTS) -o testMail $(INCLUDE) src/testMail.c src/smtpMail.c src/base64.c $(LIBS)
	rm -rf *.o


shared:
	$(CC) $(OPTS) $(INCLUDE) -shared -o smtpMail.so -fPIC src/smtpMail.c src/base64.c $(LIBS)
	rm -rf *.o


base64:
	$(CC) $(OPTS) $(INCLUDE) -c src/base64.c


mail:
	$(CC) $(OPTS) $(INCLUDE) -c src/smtpMail.c


clean:
	rm -rf *.o
