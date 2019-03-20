@echo off
ca65 cart.src -l
ld65 -o cart.bin -C cart.cfg cart.o
