
SetTimB32ms			= $f36f ; TIMER B mit 32ms starten
DevNotPresent			= $F945
do_IOInit			= $f9fb
do_restor			= $FBA2
IRQEnd3				= $fca5
SetFPtoStart			= $FE62
CmpFPtoEnd			= $FE71
IncFP				= $FE7F
tksa				= $ff96 ; sec adr nach talk
acptr				= $ffa5 ; Eingabe von IEC
talk				= $ffb4 ; talk senden
SetLFS				= $ffba
Open				= $ffc0
Close				= $ffc3
ChkIn				= $ffc6
ChkOut				= $ffc9
clrch				= $ffcc
basin				= $ffcf
bsout				= $FFD2
load				= $ffd5
save				= $ffd8
ChkStopKey			= $FFE1
Ramtas				= $FA88
jmp_scrinit			= $E004
do_memtop			= $FB78
do_tape				= $FE5D
jmp_funkey			= $E022

do_ciout = $F297
do_acptr = $F30A
do_tksa = $F280
do_second = $F274
do_unlisten = $F2AF
do_untalk = $F2AB
do_listen = $F234
do_talk = $F230

BitMapTab = $EC67