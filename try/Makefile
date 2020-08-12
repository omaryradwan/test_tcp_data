# Name: Omar Radwan
# UID: 205105562
# Class: CS118 Spring 2020
# Project: Project 1

default:
	gcc -g  -Werror server.c -o server
	gcc -g  -Werror client.c -o client
build: default
	tar -cf 205105562.tar.gz server.c client.c Makefile
dist: default
	tar -cf 205105562.tar.gz server.c client.c Makefile README packet.h
clean: build default
	-rm server client *tar.gz
