###################################################
#
# Makefile
# Simple IRC communication using sockets over TCP/IP
#
# George Syrogiannis <sirogiannisgiw@gmail.com>
# Adonis Tseriotis <adonis.tseriotis@gmail.com>
#
###################################################

CC = gcc

CFLAGS = -Wall
CFLAGS += -g
# CFLAGS += -O2 -fomit-frame-pointer -finline-functions

LIBS = 

BINS = irc_server irc_client

all: $(BINS)

irc_server: irc_server.c irc_server.h
	$(CC) $(CFLAGS) -o $@ $< $(LIBS)

irc_client: irc_client.c irc_server.h
	$(CC) $(CFLAGS) -o $@ $< $(LIBS)

clean:
	rm -f *.o *~ $(BINS)