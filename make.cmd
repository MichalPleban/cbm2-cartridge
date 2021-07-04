@echo off
ca65 -D ORG2000 cart.src
ld65 -o cart_2000.bin -C cart.cfg cart.o
ca65 -D ORG4000 cart.src
ld65 -o cart_4000.bin -C cart.cfg cart.o
ca65 -D ORG6000 cart.src
ld65 -o cart_6000.bin -C cart.cfg cart.o
