; ***********************************************************
;
; Datei mit Diskroutinen f�r Moni 610
;


; -------------------------------------------------------------------------
; IEC Status abfragen und ausgeben

MoniIECStatus:
	jsr	OpenCmdCh
	bcs	CloseCmdCh	; Fehler

; Falls vorhanden Kommandozeilenstring auf IEC ausgeben

	jsr	IgbnNextCh	; String holen falls vorhanden
	bcs	PrintIECStatus1 ; keiner vorhanden: Status lesen
	pha			; Zeichen retten
	ldx	#$00
	jsr	chkout		; #00 ist Ausgabeger�t
	pla			; auszugebendes Zeichen
	bcs	CloseCmdCh	; Fehler bei Chkout
	bcc	LF18B
LF188:
	jsr	NextChar
	php			; Carry von NextChar retten
LF18B:
	jsr	bsout
	plp
	lda	Status
	bne	CloseCmdPrtErr
	bcc	LF188
	bcs	CloseCmdCh	; Ende

; -------------------------------------------------------------------------
; Kommandokanal mit KanalNr 00 �ffnen

OpenCmdCh:
	lda	#$00
	sta	Status		; Status l�schen
	sta	FileNameLen	; vorsichtshalber
	ldx	MoniDevNr	; Default Prim�radresse
	ldy	#$0F		; Sekund�radresse
	jsr	SetLFS		; f�r logisches File #00 setzen
	clc
	jsr	open		; #00 �ffnen
OpenCmdEnd:
	rts

; -------------------------------------------------------------------------
; Kommandokanal wieder schlie�en

CloseCmdPrtErr:			; mit Fehlerausgabe
	jsr	DevNotPresent
CloseCmdCh:
	jsr	clrch
	lda	#$00
	clc
	jmp	close		; #00 schlie�en

; -------------------------------------------------------------------------
; Floppystatus abfragen und ausgeben

PrintIECStatus:
	jsr	OpenCmdCh
	bcs	CloseCmdCh	; Fehler

PrintIECStatus1:		;
	jsr	PrtCr		; Neue Zeile
	ldx	#$00
	jsr	chkin		; #00 wird Eingabeger�t
	bcs	CloseCmdCh	; Fehler bei chkin

	jsr	GetLine		; Nach Puffer holen
	lda	Status		; Fehler ?
	and	#$BF
	bne	CloseCmdPrtErr

LF1A4:
	jsr	NextChar
	bcs	CloseCmdCh
	jsr	bsout
	jmp	LF1A4

; -------------------------------------------------------------------------
; Default IEC Prim�radresse �ndern / ausgeben

MoniUnit:
	jsr	GetAscByte
	bcs	MoniUnit2	; kein Parameter: ausgeben
	cmp	#$0f
	bcc	MoniUnit1
	jmp	MoniError
MoniUnit1:
	sta	MoniDevNr
	rts

MoniUnit2:
	jsr	PrtCr
	lda	MoniDevNr
	jmp	PrtHex2Asc8

; -------------------------------------------------------------------------
; Load

MoniLoad:
	lda	#$80		; Flag f�r Load
	.byte	$2c

; -------------------------------------------------------------------------
; Save

MoniSave:
	lda	#$00
	sta	OperandLen	; Flag merken

	ldy	MoniDevNr	; Default Prim�radresse nehmen
	sty	FirstAdr
	ldy	#$00
	lda	#$FF
	sta	StartAdrLow
	sta	StartAdrHi
	lda	IndReg
	sta	StartAdrSeg
	jsr	IgbnNextCh	; n�chstes Zeichen
	bcs	LF07E
	cmp	#'"'
	bne	MoniDiskErr
	lda	InputPtr
	sta	FileNameAdrLo

LF06A:
	jsr	NextChar
	bcs	MoniDiskErr
	cmp	#'"'
	beq	LF090
	inc	FileNameLen
	iny
	cpy	#$10
	beq	MoniDiskErr
	bne	LF06A

; Einsprung f�r Load

LF07E:
	bit	OperandLen	; Load ?
	bpl	MoniDiskErr	      ; Fehler sonst
	lda	StartAdrSeg
	ldx	StartAdrLow
	ldy	StartAdrHi
	jsr	load
	jmp	PrintIECStatus ; gleich Status ausgeben

; -------------------------------------------------------------------------

LF090:
	jsr	NextChar
	bcs	LF07E
	cmp	#','
LF097:
	bne	MoniDiskErr
	jsr	GetAscByte	; Prim�radresse holen
	bcs	MoniDiskErr	; nicht angegeben ?
	sta	FirstAdr
	jsr	NextChar
	bcs	LF07E		; Zeilenende ? dann Load
	cmp	#','
LF0A5:
	bne	MoniDiskErr	; sonst mu� Komma kommen
	ldx	#StartAdrLow	; Pointer
	jsr	GetLongAdr	; Langadresse holen
	bcs	MoniDiskErr

	jsr	NextChar	; ZeilenEnde ?
	bcs	LF07E
	cmp	#','
	bne	MoniDiskErr
	ldx	#EndAdrLow
	jsr	GetLongAdr
	jsr	IgbnNextCh
	bcc	MoniDiskErr	; darf nichts mehr kommen sonst Fehler
LF0E3:
	bit	OperandLen
	bmi	MoniDiskErr	; darf nur SAVE sein (2 Adressen)
	ldx	#StartAdrLow
	ldy	#EndAdrLow
	jsr	save
	jmp	PrintIECStatus ; danach Status ausgeben

; -------------------------------------------------------------------------
; Fehlereinsprung

MoniDiskErr:
	jmp	MoniError

; -------------------------------------------------------------------------
; Directory ausgeben

MoniDir:
	lda	#'$'		; Filename f�r Directory
	sta	$200		; Pointer darauf ist gesetzt
	lda	#1
	sta	FileNameLen	; L�nge setzen
	lda	#$00
	ldx	MoniDevNr	; Default Prim�radresse
	ldy	#$00		; Sekund�radresse
	jsr	SetLFS		; f�r logisches File #00 setzen
	clc
	jsr	open		; #00 �ffnen
	bcs	MoniDiskErr	; Fehler bei Open

	lda	FirstAdr
	jsr	talk
	lda	SecondAdr
	jsr	tksa

	jsr	PrtCr		; Neue Zeile
	ldy	#3		; erste 3 Bytes �berlesen

MoniDirLoop:
	sty	MoniCntr	; Z�hler
	jsr	acptr		; Byte lesen
	sta	MoniTmp
	ldy	Status
	bne	MoniDirErr
	jsr	acptr
	ldy	Status
	bne	MoniDirErr
	ldy	MoniCntr
	dey
	bne	MoniDirLoop
	jsr	PrtHex2Asc8
	lda	MoniTmp
	jsr	PrtHex2Asc8	; Anzahl der belegten Bl�cke
	jsr	PrtSpace
MoniDirLoop1:
	jsr	acptr		; neues Byte
	ldx	Status
	bne	MoniDirErr
	tax			; Zeilenende ?
	beq	MoniDir1
	jsr	bsout
	jmp	MoniDirLoop1
MoniDir1:
	jsr	PrtCr		; Neue Zeile
	ldy	#2		; Linkadresse �bergehen
	jmp	MoniDirLoop

MoniDirErr:
	jsr	clrch
	lda	#$00
	sec
	jsr	close
	jmp	PrintIECStatus

; -------------------------------------------------------------------------

