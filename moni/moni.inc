; ******************************************************************************
; *									       *
; *  Monitor fr CBM 610. Ullrich von Bassewitz, 1987, 1988		       *
; *  Modified by Michau, 2010                                                  *
; *									       *
; ******************************************************************************



; -------------------------------------------------------------------------
; Monitor einbinden

InitMon:
	lda	#>Break			; Neuen Break-Vektor eintragen
	ldx	#<Break
	stx	BRKVec
	sta	BRKVec+1
	jsr	InitRS232		; Werte fr Schnittstelle setzen
	jmp	MonitorW


; -------------------------------------------------------------------------
; Monitor-Einsprung nach Reset


MonitorC:
;	jsr	do_ioinit
;	jsr	do_restor
;	jsr	jmp_scrinit

; -------------------------------------------------------------------------
; Monitor-Einsprung bei Warmstart

MonitorW:
;	jsr	clrch			; offene Files schlieáen
;	lda	#$5A			; Warmstartkennung
;	ldx	#<MonitorC
;	ldy	#>MonitorC
;	jsr	do_setwst		; und Adresse setzen
;	cli
	lda	#%11000000		; alle Msgs zugelassen
	sta	MsgFlag
	lda	#MoniText		; '** MONITOR 2.0 **'
	sta	MoniCntr
	lda	#$0f
	sta	IndReg			; immer Systembank bei Start
	bne	LEE31			; unbedingter Sprung
; -------------------------------------------------------------------------
; Monitor-Einsprung bei Break (vom Break-Vektor)

Break:
	jsr	clrch			; offene Files schlieáen
	lda	#BreakText		; 'BREAK'
	sta	MoniCntr
	cld
	ldx	#6-1			; Bytes
LEE2B:
	pla
	sta	PChighSave,x		; Stackinhalt bei Break retten
	dex
	bpl	LEE2B
LEE31:
	lda	IndReg			; Ind. Segment retten
	sta	IndSegSave
	lda	IRQvec
	sta	IRQSaveLo
	lda	IRQvec+1
	sta	IRQSaveHi
	tsx
	stx	SPSave
	cli
	lda	#$08
	sta	MoniDevNr
	ldx	MoniCntr
	jsr	PrtMsg
	jsr	MoniPrtRegs		; Register ausgeben
	jmp	MoniLoop

; -------------------------------------------------------------------------

MoniError:
	pla
	pla
MoniError1:
	jsr	PrtQuestMark

; -------------------------------------------------------------------------
; Eingabeschleife vom Monitor. Wird nach Ausfhrung jeden Befehls wider
; aufgerufen (Rckkehradresse auf Stack).

MoniLoop:
	lda	#$C0
	sta	MsgFlag
	lda	#$00
	sta	FileNameAdrLo
	sta	FileNameLen
	lda	#$02
	sta	FileNameAdrHi
	lda	#$0F
	sta	FileNameAdrSeg

; -------------------------------------------------------------------------
; Kommandozeile holen, Kommando suchen

	jsr	MoniPrompt		; Prompt ausgeben
	jsr	GetLine			; Eingabezeile nach Puffer kopieren

; Kommando aus Zeile holen (erste 4 Zeichen sind gltig)

	lda	#' '
	ldx	#4
GetCmd0:
	sta	MnemoTextBuf-1,x
	dex
	bne	GetCmd0		; xr = 00

LEE69:				; Prompt und Spaces ignorieren
	jsr	IgbnNextCh
	bcs	MoniLoop	; Kein Zeichen: Neuer Prompt
	cmp	#'.'
	beq	LEE69

GetCmdLoop:
	cpx	#4
	bcs	GetCmd1
	sta	MnemoTextBuf,x
GetCmd1:
	inx
	jsr	NextChar
	bcs	MoniCmd		; fertig, suchen
	and	#$7f		; Groábuchstaben zulassen
	cmp	#' '
	bne	GetCmdLoop

; -------------------------------------------------------------------------
; Monitorkommando in Tabelle suchen

MoniCmd:
	ldy	#$00
MoniCmdLoop:
	ldx	#$00		; Kommandol„nge z„hlen
MoniCmdLoop1:
	lda	MoniCmdTab,y
	beq	MoniError1	; Ende dann
	cmp	MnemoTextBuf,x
	bne	NextCmd
	iny
	inx
	cpx	#4
	bne	MoniCmdLoop1

; -------------------------------------------------------------------------
; Kommando gefunden: Adresse eintragen und anspringen

	lda	MoniCmdTab,y
	sta	Adr1
	lda	MoniCmdTab+1,y
	sta	Adr1+1

	lda	#>(MoniLoop-1)		; als Rckkehradresse auf Stack
	pha
	lda	#<(MoniLoop-1)
	pha

	jmp	(Adr1)			; Kommandoverarbeitung anspringen

; -------------------------------------------------------------------------
; Kommando war nicht gleich, Pointer auf n„chstes stellen

NextCmd:
	iny
	inx
	bne	NextCmd
	iny
	iny			; Adresse ignorieren
	jmp	MoniCmdLoop

; -------------------------------------------------------------------------
; Bank umschalten auf Systembank, altes Segment retten

SetSysBank:
	pha
	lda	IndReg
	sta	MoniSegSave
	lda	#$0f
	sta	IndReg
	pla
	rts

; -------------------------------------------------------------------------
; Bank umschalten auf Bank in ac, altes Segment retten

SetacBank:
	pha
	lda	IndReg
	sta	MoniSegSave
	pla
	sta	IndReg
	rts

; -------------------------------------------------------------------------
; Auf alte Bank rckschalten

SetOldBank:
	pha
	lda	MoniSegSave
	sta	IndReg
	pla
	rts

; -------------------------------------------------------------------------
; Byte in ASCII vom Eingabepuffer lesen, muá zweistellig angegeben sein

GetAscByte:
	lda	#$00
	sta	MoniTmp		; Wert = 0
	jsr	IgbnNextCh	; n„chstes Zeichen holen
	bcs	L227C		; keins mehr ? dann Fehler
	jsr	HexCheck	; Zeichen gltig ?
	bcs	L227C		; Nein: Fehler
GetAscByte1:
	jsr	Asc2HexNibble	; NIBBLE WANDELN
	asl	a
	asl	a
	asl	a
	asl	a		; und ins High-Nibble schieben
	sta	MoniTmp		; retten
	jsr	NextChar	; N„chstes Zeichen holen
	bcs	L227C		; keins mehr ? dann Fehler
	jsr	HexCheck	; Zeichen gltig ?
	bcs	L227C		; Nein : Fehler
	jsr	Asc2HexNibble	; wandeln
	ora	MoniTmp		; Zusammensetzen
	clc			; Fehlerfrei !
L227C:
	rts

; -------------------------------------------------------------------------
; ASCII-Nibble nach Hex wandeln

Asc2HexNibble:
	cmp	#'A'-1
	bcc	Asc2HexN1
	sbc	#'A'-10
Asc2HexN1:
	and	#$0f
	rts

; -------------------------------------------------------------------------
; 16 Bit Wert nach Ptr1 holen, muá nicht 4-stellig angegeben sein

GetAdrToPtr1:
	lda	#$00
	sta	Ptr1		; Ptr1 l”schen
	sta	Ptr1+1
	sta	MoniTmp		; Zeichenz„hler
	jsr	IgbnNextCh	; N„chstes Zeichen holen
	bcs	L224A		; Zeilenende dann Fehler
L2225:
	cmp	#' '		; Space = Ende
	beq	L2246
	jsr	HexCheck	; Zeichen gltig ?
	bcs	L224A		; sonst Fehler
	jsr	Asc2HexNibble	; nach Hex-Nibble wandeln
	asl	a
	asl	a
	asl	a
	asl	a		; UND INS High-Nibble schieben
	ldx	#$04		; 4 Bits zu verarbeiten
L2237:
	asl	a
	rol	Ptr1
	rol	Ptr1+1		; Bits von rechts in Ptr1 schieben
	dex
	bne	L2237
	inc	MoniTmp		; Zeichenz„hler erh”hen
	jsr	NextChar
	bcc	L2225
L2246:
	lda	MoniTmp		; Anzahl der Zeichen nach AC
	clc			; Fehlerfrei
	rts

L224A:				; Fehlereinsprung
	lda	#$00		; Anzahl = 0
	sec			; Fehlerfalg setzen
L224D:
	rts
; -------------------------------------------------------------------------
; Langadresse nach X als ZP-Pointer

GetLongAdr:
	jsr	GetAscByte
	bcs	GetLongEnd
	cmp	#$10
	bcs	GetLongEnd
	sta	$02,x
	jsr	GetAscByte
	bcs	GetLongEnd
	sta	$01,x
	jsr	GetAscByte
	bcs	GetLongEnd
	sta    $00,x
	clc				; Fehlerfrei
GetLongEnd:
	rts

; -------------------------------------------------------------------------
; Ptr2 und Ptr3 von Eingabezeile lesen, Carry wenn Fehler

GetPtr2es3:
	jsr	GetAdrToPtr1
	bcs	L224A
	jsr	CopyPtr1to2
	jsr	GetAdrToPtr1
	bcs	L224A
	jmp	CopyPtr1to3

; -------------------------------------------------------------------------
CopyPtr1to2:
	lda	Ptr1
	sta	Ptr2
	lda	Ptr1+1
	sta	Ptr2+1
	rts
; -------------------------------------------------------------------------
CopyPtr1to3:
	lda	Ptr1
	sta	Ptr3
	lda	Ptr1+1
	sta	Ptr3+1
	rts
; -------------------------------------------------------------------------
XchgPtr2es3:
	ldx	#$01
L2165:
	lda	Ptr2,x
	pha
	lda	Ptr3,x
	sta	Ptr2,x
	pla
	sta	Ptr3,x
	dex
	bpl	L2165
	rts
; -------------------------------------------------------------------------
; Ptr2 nach Ptr3 kopieren

CopyPtr2to3:
	lda	Ptr2
	sta	Ptr3
	lda	Ptr2+1
	sta	Ptr3+1
	rts
; -------------------------------------------------------------------------
; Ptr 2 um 1 bzw. Akkuinhalt erh”hen

IncPtr2:
	lda	#$01
IncPtr2AC:
	clc
	adc	Ptr2
	sta	Ptr2
	bcc	IncPtr2End
	inc	Ptr2+1
IncPtr2End:
	rts
; -------------------------------------------------------------------------
; Ptr2 und Ptr3 vergleichen, Carry, wenn Ptr2 kleiner

CmpPtr2es3:
	sec
	lda	Ptr3
	sbc	Ptr2
	lda	Ptr3+1
	sbc	Ptr2+1
	rts
; -------------------------------------------------------------------------
; Meldungen ausgeben
;

CursUp:
	ldx	#CursUpTxt
	.byte	$2c
RVSOn:
	ldx	#RVSOnTxt
PrtMsg:
	lda	Messages,x
	beq	L21CB
	jsr	bsout
	inx
	jmp	PrtMsg
; -------------------------------------------------------------------------
; Cursor eine Zeile hoch, Prompt ('.') ausgeben und Zeichen in AC

UpLineSetCmd:
	pha
	jsr	CursUp
	jsr	PrtPoint
	pla
	jsr	bsout
	jmp	PrtSpace
; -------------------------------------------------------------------------
; Prompt und vorgegebenes Kommando in ac ausgeben

MoniPromptCmd:
	pha				; Kommando retten
	jsr	MoniPrompt
	pla
	jsr	bsout
	jmp	PrtSpace

; -------------------------------------------------------------------------
MoniPrompt:
	jsr	PrtCr

; -------------------------------------------------------------------------
; Ausgabe diverser einzelner Zeichen

PrtPoint:
	lda	#'.'
	.byte	$2c
PrtKomma:
	lda	#','
	.byte	$2C
PrtCR:
	lda	#$0D
	.byte	$2c
PrtSpace:
	lda	#' '
	.byte	$2C
PrtQuestMark:
	lda	#'?'
	jmp	bsout

; -------------------------------------------------------------------------
; Spaces ausgeben

Prt3Spaces:
	jsr	PrtSpace
Prt2Spaces:
	jsr	PrtSpace
	jmp	PrtSpace
; -------------------------------------------------------------------------
Hex2Asc8:
	pha
	jsr	Hex2Asc4
	tax
	pla
	lsr	a
	lsr	a
	lsr	a
	lsr	a
Hex2Asc4:
	and	#$0F
	cmp	#$0A
	bcc	L24A7
	adc	#$06
L24A7:
	adc	#'0'
	rts
; -------------------------------------------------------------------------
HexCheck:
	cmp	#'A'
	bcc	NumCheck
	cmp	#'F'+1
	rts
; -------------------------------------------------------------------------
NumCheck:
	cmp	#'0'
	bcc	NumCheck1
	cmp	#'9'+1
	rts

NumCheck1:
	sec
	rts

; -------------------------------------------------------------------------
; 16Bit Wert in X/Y auf Bildschirm ausgeben

PrtHex2Asc16:
	txa
	jsr	PrtHex2Asc8
	tya

; geht direkt weiter

; -------------------------------------------------------------------------
; Wert in AC nach ASCII wandeln und ausgeben, XR retten

PrtHex2Asc8:
	stx	XSave
	jsr	Hex2Asc8
	jsr	bsout
	txa			; niederwertigeres Nibble
	jsr	bsout
	ldx	XSave
L21CB:
	rts
; -------------------------------------------------------------------------
; Ptr2 in ASCII mit folgenden 3 Spaces ausgeben

PrtPtr2:
	lda	Ptr2+1
	jsr	PrtHex2Asc8
	lda	Ptr2
	jsr	PrtHex2Asc8
	jmp	Prt3Spaces
; -------------------------------------------------------------------------
; Zeichen vom Bildschirm holen und auf CR prfen

BasinChkCr:
	jsr	Basin
	cmp	#$0d
	rts

; -------------------------------------------------------------------------
; Zeile vom Bildschirm nach Puffer kopieren, Pointer setzen

GetLine:
	lda	#$00
	tax			; Eingabepointer = 0
	pha
L21EE:
	jsr	BasinChkCR	; Zeichen holen und auf CR prfen
	beq	L21FB		; CR = Ende
	sta	$0200,x		; sonst ablegen
	inx			; und Pointer erh”hen
	cpx	#80		; 79 Zeichen maximal
	bne	L21EE
L21FB:
	pla			; #$00
	sta	$0200,x		; als Endekennung setzen
	sta	InputPtr	; auf Anfang setzen
	rts
; -------------------------------------------------------------------------
; N„chstes Zeichen aus Eingabepuffer holen

IgbnNextCh:			; mit ignore Spaces
	jsr	IgnoreBlanks
NextChar:
	stx	XSave		; XR retten
	ldx	InputPtr
	lda	$0200,x		; Zeichen holen
	beq	L2211		; Ende ? dann Pointer nicht erh”hen
	inc	InputPtr	; Pointer erh”hen
	clc			; Flag fr Fehlerfrei
L220D:
	ldx	XSave		; altes XR
	rts
L2211:
	lda	#$0d		; Cr fr Ende
	sec			; Flag fr Zeilenende
	bcs	L220D		; unbedingter Sprung

; -------------------------------------------------------------------------
; Spaces ignorieren.

IgnoreBlanks:
	stx	XSave
	ldx	InputPtr
IgBnLoop:
	lda	$200,x
	cmp	#' '
	bne	IgBnEnd
	inx
	jmp	IgBnLoop

IgBnEnd:
	stx	InputPtr
	ldx	XSave
	rts

; -------------------------------------------------------------------------
; Monitor verlassen

MoniExit:
	pla
	pla			; Rcksprungadresse l”schen
	jmp	(wstvec)

; -------------------------------------------------------------------------
; Hilfefunktion

MoniHelp:
	ldy	#$00
MoniHLoop:
	jsr	PrtCr
	jsr	Prt3Spaces
	ldx	#4
MoniHLoop1:
	lda	MoniCmdTab,y
	beq	L2074
	jsr	bsout
	iny
	dex
	bne	MoniHLoop1
	iny
	iny			; Adresse berspringen
	jmp	MoniHLoop

; -------------------------------------------------------------------------
; Disassembler

Moni_DisAss:
	jsr	GetAdrToPtr1		; Anfangsadresse holen
	bcc	L204B
	jsr	CopyPtr2to3
	lda	#20			; Default: disassembliere 20 Bytes
	jsr	IncPtr2AC
	jsr	XchgPtr2es3
	bmi	L205B
L204B:
	jsr	CopyPtr1to2
	jsr	GetAdrToPtr1
	bcs	L2058
	jsr	CopyPtr1to3
	bcc	L205B
L2058:
	jsr	CopyPtr2to3
L205B:
	lda	#','
	jsr	MoniPromptCmd
	jsr	ChkStopKey
	beq	L2074
	jsr	DisAssLine
	inc	OperandLen
	lda	OperandLen
	jsr	IncPtr2AC
	jsr	CmpPtr2es3
	bcs	L205B
L2074:
	rts
; -------------------------------------------------------------------------
; Eine Zeile disassemblieren

DisAssLine:
	jsr	PrtPtr2			; Adresse ausgeben
	ldy	#$00
	lda	(Ptr2),y		; OPCode holen
	jsr	AnalyzeOPCode		; und analysieren
	pha
	ldx	OperandLen		; Anzahl Bytes
	inx
L2083:
	dex
	bpl	L208C			; Bytes ausgeben
	jsr	Prt3Spaces		; 3 Spaces dann
	jmp	L2094
; -------------------------------------------------------------------------
L208C:
	lda	(Ptr2),y		; Byte von Adresse holen
	jsr	PrtHex2Asc8		; ausgeben
	jsr	PrtSpace
L2094:
	iny				; n„chstes
	cpy	#$03			; schon 3 ?
	bcc	L2083			; nein
	jsr	Prt3Spaces
	pla
	ldx	#$03
	jsr	PrtMnemo
	ldx	#$06
L20A4:
	cpx	#$03
	bne	L20BA
	ldy	OperandLen
	beq	L20BA
L20AC:
	lda	AdrFlagBuf
	cmp	#$E8			; Branch Befehl ?
	lda	(Ptr2),y
	bcs	GetBranchAdr
	jsr	PrtHex2Asc8
	dey
	bne	L20AC
L20BA:
	asl	AdrFlagBuf
	bcc	L20CC
	lda	SymbolTab1-1,x
	jsr	bsout
	lda	SymbolTab2-1,x
	beq	L20CC
	jsr	bsout
L20CC:
	dex
	bne	L20A4
	rts
; -------------------------------------------------------------------------
GetBranchAdr:
	jsr	L20DD
	clc
	adc	#$01
	bne	L20D9
	inx
L20D9:
	tay
	jmp	PrtHex2Asc16
; -------------------------------------------------------------------------
L20DD:
	ldx	Ptr2+1
	tay
	bpl	L20E3
	dex
L20E3:
	adc	Ptr2
	bcc	L20E8
	inx
L20E8:
	rts
; -------------------------------------------------------------------------
AnalyzeOPCode:
	tay
	lsr	a
	bcc	L20F8
	lsr	a
	bcs	L2107
	cmp	#$22
	beq	L2107
	and	#$07
	ora	#$80
L20F8:
	lsr	a
	tax
	lda	OffsetTab,x
	bcs	L2103
	lsr	a
	lsr	a
	lsr	a
	lsr	a
L2103:
	and	#$0F
	bne	L210B
L2107:
	ldy	#$80
	lda	#$00
L210B:
	tax
	lda	AdrFlagTab,x
	sta	AdrFlagBuf
	and	#$03
	sta	OperandLen
	tya
	and	#$8F
	tax
	tya
	ldy	#$03
	cpx	#$8A
	beq	L212B
L2120:
	lsr	a
	bcc	L212B
	lsr	a
L2124:
	lsr	a
	ora	#$20
	dey
	bne	L2124
	iny
L212B:
	dey
	bne	L2120
	rts
; -------------------------------------------------------------------------
PrtMnemo:
	tay
	lda	MnemoTab1,y
	sta	MnemoBuf
	lda	MnemoTab2,y
	sta	MnemoBuf+1
L213A:
	lda	#$00
	ldy	#$05
L213E:
	asl	MnemoBuf+1
	rol	MnemoBuf
	rol	a
	dey
	bne	L213E
	adc	#$3F
	jsr	bsout
	dex
	bne	L213A
	jmp	PrtSpace
; -------------------------------------------------------------------------
; Speicherbereich fllen. (Innerhalb vorgegebener Bank).
; Syntax: 'F Start Stop Wert'

Moni_Fill:
	jsr	GetPtr2es3	; Start und Stopadresse holen
	bcc	L2285		; Fehler ?
L2282:
	jmp	MoniError	; dann Ende

L2285:
	jsr	GetAscByte	; zu fllenden Wert holen
	bcs	L2282
	sta	Adr1		; und merken
	ldy	#$00
L228E:
	jsr	CmpPtr2es3	; Fll-Schleife
	bcc	L22DE
	lda	Adr1
	sta	(Ptr2),y
	jsr	IncPtr2
	jmp	L228E
; -------------------------------------------------------------------------
; Speicherbereiche vergleichen (Innerhalb vorgegebener Bank)

Moni_Compare:
	lda	#$00		; Flag fr Compare setzen
	.byte	$2C
; -------------------------------------------------------------------------
; Speicherbereiche verschieben (Innerhalb einer Bank)

Moni_Transfer:
	lda	#$80		; Flag fr Transfer
	sta	Adr1		; Flag merken
	jsr	GetPtr2es3	; Start und Endadresse holen
	bcs	L2282
	jsr	GetAdrToPtr1	; Zieladresse holen
	bcs	L2282
	jsr	PrtCR		; Leerzeile
	ldy	#$00
L22B3:
	lda	(Ptr2),y	; Byte holen
	bit	Adr1		; Compare oder Transfer ?
	bpl	L22BD		; Compare, dann skip
	sta	(Ptr1),y	; transfer, dann Byte speichern
	bmi	L22D0
L22BD:
	sta	Adr1+1		; Compare: Byte merken
	lda	(Ptr1),y	; Zielbyte holen
	cmp	Adr1+1		; Vergleich
	beq	L22D0		; gleich ? dann weiter
	jsr	PrtPtr2		; sonst Adresse ausgeben
	jsr	PrtSpace	; und ein Space
	jsr	ChkStopKey	; Stop Taste gedrckt ?
	beq	L22DE		; ja: dann Ende
L22D0:
	inc	Ptr1		; Pointer 1 erh”hen
	bne	L22D6
	inc	Ptr1+1
L22D6:
	jsr	IncPtr2		; Pointer 2 erh”hen
	jsr	CmpPtr2es3	; fertig ?
	bcs	L22B3		; Nein: dann weiter
L22DE:
	rts
; -------------------------------------------------------------------------
; Bytefolge innerhalb vorgegebenem Segment suchen

Moni_Find:
	jsr	GetPtr2es3
	bcc	L22E7
L22E4:
	jmp	MoniError
; -------------------------------------------------------------------------
L22E7:
	ldx	#$00
	jsr	IgbnNextCh		; N„chstes Zeichen
	bcs	L22E4			; Keins mehr : Fehler

	cmp	#$27			; Hochkomma ?
	bne	L2302
L22F4:
	jsr	NextChar
	bcs	L2313
	sta	FindBuf,x
	inx
	cpx	#10
	bne	L22F4
	beq	L2313
L2302:
	jsr	GetAscByte1
	bcs	L22E4
L2307:
	sta	FindBuf,x
	inx
	cpx	#10
	beq	L2313
	jsr	GetAscByte
	bcc	L2307
L2313:
	stx	Adr1
	jsr	PrtCR
L2318:
	ldy	#$00
L231A:
	lda	(Ptr2),y
	cmp	FindBuf,y
	bne	L2331
	iny
	cpy	Adr1
	bne	L231A
	jsr	PrtPtr2
	jsr	PrtSpace
	jsr	ChkStopKey
	beq	L2339
L2331:
	jsr	IncPtr2
	jsr	CmpPtr2es3
	bcs	L2318
L2339:
	rts
; -------------------------------------------------------------------------
Moni_Assemble:
	jsr	GetAdrToPtr1
	bcc	L2342
	jmp	MoniError
; -------------------------------------------------------------------------
L2342:
	jsr	CopyPtr1to2
	ldx	#$00
	stx	MnemoBuf2+1
L2349:
	jsr	NextChar	; Opcode nach MnemoTextBuf kopieren
	bcc	L234F
	rts
; -------------------------------------------------------------------------
L234F:
	cmp	#' '
	beq	L2349
	sta	MnemoTextBuf,x
	inx
	cpx	#$03
	bne	L2349
L235A:
	dex				; fertig ?
	bmi	L236E
	lda	MnemoTextBuf,x
	sec
	sbc	#$3F
	ldy	#$05
L2364:
	lsr	a
	ror	MnemoBuf2+1
	ror	MnemoBuf2
	dey
	bne	L2364
	beq	L235A
L236E:
	ldx	#$02
L2370:
	jsr	NextChar
	bcs	L2393
	cmp	#' '
	beq	L2370
	jsr	HexCheck
	bcs	L238C
	jsr	GetAscByte1
	ldy	Ptr3
	sty	Ptr3+1
	sta	Ptr3
	lda	#$30
	sta	MnemoBuf2,x
	inx
L238C:
	sta	MnemoBuf2,x
	inx
	cpx	#10
	bcc	L2370
L2393:
	stx	Adr2
	ldx	#$00
	stx	Adr1
	beq	L23A2
L239B:
	inc	Adr1
	bne	L23A2
	jmp	MoniError
; -------------------------------------------------------------------------
L23A2:
	ldx	#$00
	stx	Adr1+1
	lda	Adr1
	jsr	AnalyzeOPCode
	ldx	AdrFlagBuf
	stx	Adr2+1
	tax
	lda	MnemoTab2,x
	jsr	CmpTestCode
	bcs	L239B
	lda	MnemoTab1,x
	jsr	CmpTestCode
	bcs	L239B
	ldx	#$06
L23C2:
	cpx	#$03
	bne	L23DA
	ldy	OperandLen
	beq	L23DA
L23CA:
	lda	Adr2+1
	cmp	#$E8		; Branch ?
	lda	#$30
	bcs	L23F5
	jsr	CmpTestCodes
	bcs	L239B
	dey
	bne	L23CA
L23DA:
	asl	Adr2+1
	bcc	L23F0
	lda	SymbolTab1-1,x
	jsr	CmpTestCode
	bcs	L239B
	lda	SymbolTab2-1,x
	beq	L23F0
	jsr	CmpTestCode
	bcs	L239B
L23F0:
	dex
	bne	L23C2
	beq	L23FF
L23F5:
	jsr	CmpTestCodes
	bcs	L239B
	jsr	CmpTestCodes
	bcs	L239B
L23FF:
	lda	Adr2
	cmp	Adr1+1
	bne	L239B
	ldy	OperandLen
	beq	L243D
	lda	AdrFlagBuf
	cmp	#$9D
	bne	L2435
	lda	Ptr3
	sbc	Ptr2
	sta	Adr2+1
	lda	Ptr3+1
	sbc	Ptr2+1
	bcc	L2426
	beq	L2420
L241D:
	jmp	MoniError
; -------------------------------------------------------------------------
L2420:
	ldx	Adr2+1
	bmi	L241D
	bpl	L242E
L2426:
	tay
	iny
	bne	L241D
	ldx	Adr2+1
	bpl	L241D
L242E:
	dex
	dex
	txa
	ldy	OperandLen
	bne	L2438
L2435:
	lda	Ptr2+1,y
L2438:
	sta	(Ptr2),y
	dey
	bne	L2435
L243D:
	lda	Adr1
	sta	(Ptr2),y
	lda	#'A'
	jsr	UpLineSetCmd
	jsr	DisAssLine
	inc	OperandLen
	lda	OperandLen
	jsr	IncPtr2AC
	lda	#'A'
	sta	Keyd
	lda	#' '
	sta	Keyd+1
	sta	Keyd+6
	sta	Keyd+7
	sta	Keyd+8
	lda	Ptr2+1
	jsr	Hex2Asc8
	sta	Keyd+2
	stx	Keyd+3
	lda	Ptr2
	jsr	Hex2Asc8
	sta	Keyd+4
	stx	Keyd+5
	lda	#$09
	sta	KeyBufIndex
	rts
; -------------------------------------------------------------------------
CmpTestCodes:
	jsr	CmpTestCode
	bcs	L248C
CmpTestCode:
	stx	XSave
	ldx	Adr1+1
	cmp	FindBuf,x
	beq	L248E
L248C:
	sec
	rts
; -------------------------------------------------------------------------
L248E:
	inc	Adr1+1
	ldx	XSave
	clc
	rts
; -------------------------------------------------------------------------
Moni_DumpMem:
	jsr	GetAdrToPtr1
	bcc	L24CA
	jsr	CopyPtr2to3
	lda	#$FF
	jsr	IncPtr2AC
	jsr	XchgPtr2es3
	bmi	L24DA
L24CA:
	jsr	CopyPtr1to2
	jsr	GetAdrToPtr1
	bcs	L24D7
	jsr	CopyPtr1to3
	bcc	L24DA
L24D7:
	jsr	CopyPtr2to3
L24DA:
	lda	#':'
	jsr	MoniPromptCmd
	jsr	PrtHexDumpRow
	jsr	ChkStopKey
	beq	L24F1
	lda	#$10
	jsr	IncPtr2AC
	jsr	CmpPtr2es3
	bcs	L24DA
L24F1:
	rts
; -------------------------------------------------------------------------
; Eine Reihe Hexdump ausgeben

PrtHexDumpRow:
	lda	Bellmode		; Klingel ausschalten
	pha
	lda	#$80
	sta	Bellmode

; Anfangsadresse ausgeben

	jsr	PrtPtr2
	ldy	#$00
L24F7:
	lda	(Ptr2),y
	jsr	PrtHex2Asc8
	jsr	PrtSpace
	iny
	cpy	#$10
	bne	L24F7
	jsr	RVSOn
	ldy	#$00
L250B:
	lda	(Ptr2),y
	and	#$7F
	cmp	#' '
	bcs	L2515
	lda	#'.'
L2515:
	jsr	bsout
	iny
	cpy	#$10
	bne	L250B

; alten Klingelmode wieder zurck

	pla
	sta	BellMode

	rts
; -------------------------------------------------------------------------
; Eine Zeile Hexdump mit max 16 Bytes nach Speicher schreiben (Com. ':' )

WrtHexDumpRow:
	jsr	GetAdrToPtr1		; Adresse angegeben ?
	bcc	WHexDumpRow1		; ja, weiter
	jmp	MoniError		; nein, Fehler
; -------------------------------------------------------------------------
WHexDumpRow1:
	jsr	CopyPtr1to2
	ldx	#$10
	jsr	WriteHexBytes
	jsr	CursUp
	lda	#':'
	jsr	UpLineSetCmd		; Kommando ':' vor Zeile
	jmp	PrtHexDumpRow		; Zeile komplett ausgeben mit ASCII

; -------------------------------------------------------------------------
; Bytes nach Speicher schreiben

WriteHexBytes:
	ldy	#$00
WHexBytesLoop:
	jsr	GetAscByte
	bcs	L2538
	sta	(Ptr2),y
	iny
	dex
	bne	WHexBytesLoop
L2538:
	rts

; -------------------------------------------------------------------------
; Registerwerte setzen

MoniSetRegs:
	jsr	SetSysBank
	lda	#$00
	sta	Ptr2+1
	lda	#PCHighSave
	sta	Ptr2
	ldx	#9		; Maximal 9 Bytes
	jsr	WriteHexBytes	; nach Ptr2 schreiben
	jsr	SetOldBank
	rts

; -------------------------------------------------------------------------
; Register ausgeben

MoniPrtRegs:
	ldx	#RegText
	jsr	PrtMsg		; Text fr Registerausgabe
	ldx	PCHighSave
	ldy	PCLowSave
	jsr	PrtHex2Asc16	; PC ausgeben
	jsr	PrtSpace
	ldx	#$00		; 5 Bytes jetzt
PrtRegLoop:
	lda	SRSave,x
	jsr	PrtHex2Asc8	; Byte ausgeben
	jsr	Prt2Spaces	; Raum lassen
	inx
	cpx	#5
	bne	PrtRegLoop

	ldx	IRQSaveHi	; IRQ Vektor ausgeben
	ldy	IRQSaveLo
	jsr	PrtHex2Asc16

	rts

; -------------------------------------------------------------------------
; Segment setzen / anzeigen

SetBank:
	jsr	GetAscByte		; Byte holen
	bcc	SetBank1
	jsr	PrtCr
	lda	IndReg
	jmp	PrtHex2Asc8		; ausgeben

SetBank1:
	sta	IndReg
	rts

; -------------------------------------------------------------------------
; GO-Befehl ausfhren

MoniGo:
	jsr	GetAdrtoPtr1		; Adresse holen
	bcs	MoniGo1			; keine angegeben: Skip
	lda	Ptr1+1
	sta	PCHighSave		; Register auf alte Werte
	lda	Ptr1
	sta	PCLowSave
MoniGo1:
	ldx	SPSave
	txs
	lda	IRQSaveHi
	ldx	IRQSaveLo
	sei
	sta	IRQVec+1
	stx	IRQVec
	lda	IndSegSave
	sta	IndReg
	ldx	#$00
PushRegLoop:
	lda	PCHighSave,x
	pha
	inx
	cpx	#6
	bne	PushRegLoop
	jmp	IRQEnd3			; pull regs + rti

; -------------------------------------------------------------------------
; Registerwerte fr RS232 vorbesetzen

InitRS232:
	lda	#$1e			; 9600 8N1
	sta	m6551ctrl
InitRs232Cmd:
	lda	#$0b			; ohne Interrupts
	sta	m6551cmd
	rts

; -------------------------------------------------------------------------
; Registerwerte fr RS232 setzen (setBaudrate)

SetRS232:
	jsr	GetAscByte	; Controlregisterwert vorhanden ?
	bcs	InitRS232	; nein, dann Default setzen
	sta	m6551ctrl
	jsr	GetAscByte	; Comandregisterwert vorhanden ?
	bcs	InitRS232Cmd	; Default, wenn nicht
	sta	m6551cmd
	rts

; -------------------------------------------------------------------------
; Status der DSR-Leitung prfen, evtl Abbruch

CheckRS232:
	bit	acia+AStatusReg
	bvc	ChkRS232A	; ok
	pla
	pla			; Rcksprungadresse l”schen
	jmp	DevNotPresent

ChkRS232A:
	rts

; -------------------------------------------------------------------------
; Speicherbereich nach serieller Schnittstelle ausgeben
; ohne Fehlercheck, Startadresse im aktuellen Segment

SerOut:
	lda	#$80		; Flag fr Schreiben
	.byte	$2c

; -------------------------------------------------------------------------
; Daten von serieller Schnittstelle nach Speicher lesen, Start im aktuellen
; Segment, Adresse im Segment muá angegeben werden.


SerIn:
	lda	#$00			; Flag fr Lesen
	sta	Adr1+1			; Flag merken

	jsr	GetAdrToPtr1		; Startadresse holen
	bcc	Ser1
SerErr:
	jmp	MoniError
Ser1:
	lda	IndReg			; Staradresse nach FP-Start kopieren
	sta	StartAdrSeg
	lda	Ptr1+1
	sta	StartAdrHi
	lda	Ptr1
	sta	StartAdrLow

; Bei Schreiben : Endadresse holen

	bit	Adr1+1
	bpl	Ser2
	jsr	GetAdrtoPtr1
	bcs	SerErr

	lda	IndReg			; Endadresse nach FP-Ende kopieren
	sta	EndAdrSeg
	lda	Ptr1+1
	sta	EndAdrHi
	lda	Ptr1
	sta	EndAdrLow

; Status der Schnittstelle testen

Ser2:
	jsr	CheckRS232		; DSR Status testen

	jsr	PrtCR			; Neue Zeile fr Meldungen

	jsr	SetFPtoStart		; FilePointer auf Start setzen

	lda	m6551ctrl		; Register schon belegen
	sta	acia+ACtrlReg
	lda	m6551cmd
	sta	acia+ACmdReg

	ldy	#$00

; Flag testen und je nachdem Schleife anspringen

	bit	Adr1+1
	bpl	SerInLoop

SerOutLoop:
	lda	(SaveAdrLow),y
	jsr	PutSerByte
	bcs	Ser3			; StopTaste gedrckt
	jsr	IncFP
	jsr	CmpFPtoEnd		; Ende ?
	bcc	SerOutLoop
	bcs	Ser3

SerInLoop:
	jsr	GetSerByte		; Byte holen
	bcs	Ser3			; Stop Taste
	sta	(SaveAdrLow),y		; Byte ablegen
	jsr	IncFP			; Pointer erh”hen
	jmp	SerInLoop		; und weiter

; Ende, Start- und Endadresse ausgeben

Ser3:
	ldx	#StartAdrSeg
	jsr	Ser4
	jsr	Prt3Spaces
	ldx	#SaveAdrSeg
Ser4:
	ldy	#3			; 3 Bytes
Ser5:
	lda	$00,x			; Byte holen
	jsr	PrtHex2Asc8		; ausgeben
	dex
	dey
	bne	Ser5


	rts

; -------------------------------------------------------------------------
; Byte von serieller Schnittstelle holen mit Check auf Stoptaste
; Carry wenn Stoptaste

GetSerByte:
	jsr	ChkStopKey	; Stop - Taste gedrckt ?
	beq	GetSer1
	lda	#%1000
	bit	acia+AStatusReg ; Testen ob Zeichen da
	bvs	GetSerByte	; Kein DSR
	beq	GetSerByte	; Kein Zeichen da
	lda	acia+ADataReg	; Zeichen holen
	clc
	rts

GetSer1:
	sec			; Flag setzen
	rts


; -------------------------------------------------------------------------
; Byte ber serielle ausgeben, mit Check auf Stoptaste
; Carry wenn Stoptaste

PutSerByte:

; warten bis DSR und Transmit Register leer

	pha

PutSerByteLoop:
	jsr	ChkStopKey
	bne	PutSer1			; keine Stoptaste
	pla				; Ausgabezeichen
	sec
	rts

PutSer1:
	lda	#%10000			; Transmit Reg Status
	bit	acia+AStatusReg
	bvs	PutSerByteLoop		; DSR war high
	beq	PutSerByteLoop		; Transmit Reg voll

	pla				; Ausgabezeichen
	sta	acia+ADataReg		; und weg damit
	clc				; Kein Timeout
	rts


; -------------------------------------------------------------------------
Messages:

BreakText = (*-Messages)
	.byte	$0d, "BREAK", $0d, $00

MoniText  = (*-Messages)

	.byte	$0d, "** MONITOR 2.0 **", $0d, $00

RegText  = (*-Messages)
	.byte	$0d, "   PC  SR  AC  XR  YR  SP  IRQ", $0d,';',' ',$00

CursUpTxt = (*-Messages)
	.byte	$91,$0D,$00

RVSOnTxt  = (*-Messages)
	.byte	$20,$20,$3E,$12,$00


; -------------------------------------------------------------------------
; Tabellen fr Assembler

OffsetTab:
	.byte	$40,$02,$45,$03,$D0,$08,$40,$09
	.byte	$30,$22,$45,$33,$D0,$08,$40,$09
	.byte	$40,$02,$45,$33,$D0,$08,$40,$09
	.byte	$40,$02,$45,$B3,$D0,$08,$40,$09
	.byte	$00,$22,$44,$33,$D0,$8C,$44,$00
	.byte	$11,$22,$44,$33,$D0,$8C,$44,$9A
	.byte	$10,$22,$44,$33,$D0,$08,$40,$09
	.byte	$10,$22,$44,$33,$D0,$08,$40,$09
	.byte	$62,$13,$78,$A9

AdrFlagTab:
	.byte	$00,$21,$81,$82,$00,$00,$59,$4D
	.byte	$91,$92,$86,$4A,$85,$9D

SymbolTab1:
	.byte	$2C,$29,$2C,$23,$28,$24

SymbolTab2:
	.byte	$59,$00,$58,$24,$24,$00

MnemoTab1:
	.byte	$1C,$8A,$1C,$23,$5D,$8B,$1B,$A1
	.byte	$9D,$8A,$1D,$23,$9D,$8B,$1D,$A1
	.byte	$00,$29,$19,$AE,$69,$A8,$19,$23
	.byte	$24,$53,$1B,$23,$24,$53,$19,$A1
	.byte	$00,$1A,$5B,$5B,$A5,$69,$24,$24
	.byte	$AE,$AE,$A8,$AD,$29,$00,$7C,$00
	.byte	$15,$9C,$6D,$9C,$A5,$69,$29,$53
	.byte	$84,$13,$34,$11,$A5,$69,$23,$A0

MnemoTab2:
	.byte	$D8,$62,$5A,$48,$26,$62,$94,$88
	.byte	$54,$44,$C8,$54,$68,$44,$E8,$94
	.byte	$00,$B4,$08,$84,$74,$B4,$28,$6E
	.byte	$74,$F4,$CC,$4A,$72,$F2,$A4,$8A
	.byte	$00,$AA,$A2,$A2,$74,$74,$74,$72
	.byte	$44,$68,$B2,$32,$B2,$00,$22,$00
	.byte	$1A,$1A,$26,$26,$72,$72,$88,$C8
	.byte	$C4,$CA,$26,$48,$44,$44,$A2,$C8


; Tabelle mit Monitorkommandos
; Ein Eintrag besteht aus:
;
;	.text	'xxx'	; Kommando mit max. 4 Zeichen
;	.word	Sprungadresse
;


MoniCmdTab:
	.byte	"A   "		; Zeile assemblieren
	.word	Moni_Assemble

	.byte	"BANK"		; Set Indirct Segment
	.word	SetBank

	.byte	"BAUD"		; RS232 Baudrate etc. setzen
	.word	SetRS232

	.byte	"COMP"		; Compare Memory
	.word	Moni_Compare

	.byte	"D   "		; Disassemble
	.word	Moni_DisAss

	.byte	"DIR "		; Directory ausgeben
	.word	MoniDir

	.byte	"EXIT"		; Exit Monitor
	.word	MoniExit

	.byte	"FILL"		; Fill Memory
	.word	Moni_Fill

	.byte	"FIND"		; Bytefolge finden
	.word	Moni_Find

	.byte	"G   "		; Programm starten
	.word	MoniGo

	.byte	"HELP"		; Hilfefunktion
	.word	MoniHelp

	.byte	"LOAD"		; Load File
	.word	MoniLoad

	.byte	"M   "		; Dump Memory
	.word	Moni_DumpMem

	.byte	"REGS"		; Print Registers
	.word	MoniPrtRegs

	.byte	"SAVE"		; Save File
	.word	MoniSave

	.byte	"SIN "		; Input RS232
	.word	SerIn

	.byte	"SOUT"		; Output RS232
	.word	SerOut

	.byte	"XFER"		; Transfer Memory
	.word	Moni_Transfer

	.byte	"UNIT"		; Set Unit Number
	.word	MoniUnit

	.byte	"@   "		; Print IEC Status
	.word	MoniIECStatus

	.byte	":   "		; Write Bytes to Memory
	.word	WrtHexDumpRow

	.byte	";   "		; Registerwerte lesen
	.word	MoniSetRegs

	.byte	",   "		; Zeile assemblieren
	.word	Moni_Assemble

	.byte	$00		; Endekennung der Tabelle

; -------------------------------------------------------------------------


; ***********************************************************
;
; Einbinden der Routinen fr den seriellen IEC-Bus
;


