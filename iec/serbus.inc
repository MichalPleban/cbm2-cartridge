; Routinen fÅr seriellen IEC-Bus am CBM 610
;
; Folgende Bedeutung haben die Leitungen des Cassettenports:
;
; Switch		=	Clock	--> tpi1 pb7
; Motor			=	ATN	--> tpi1 pb6	Vorsicht: Invertiert !
; Write			=	Data	--> tpi1 pb5
; Read			=	Data	--> cia  Flag
;
; Die Read-Leitung wird nur benutzt, um zu ÅberprÅfen, ob der Adapter
; angeschlossen ist, in diesem Falle fÅhrt ein Puls auf der Write-Leitung
; zu einem gesetzten Zustands-Flag fÅr die Flag-Leitung.
;

; ***********************************************************
;
; Externe Aufrufe im Kernal
;

jmp_ScnKey	=	$e013		; Tastatur abfragen
do_udtim	=	$f979
OrStatus	=	$fb5f		; ora status; sta status; rts
do_IRQ1		=	$fbf5		; IRQ nach Check ob von 6525
do_ciout_end	=	$f2a3		; Zeichen setzen
IRQChkCass	=	$fc87
IRQEnd1		=	$fc9f		; mit Schreiben von IRQCtrl
IRQEnd2		=	$fca2		; mit pla; sta indreg



; -------------------------------------------------------------------------

SerBusInit:
	sei
	lda	#%00100000		; Write-Leitung setzen
	ora	tpi1+tpiPortB
	sta	tpi1+tpiPortB
	bit	cia+IntCtrReg		; Read-Flag lîschen
	and	#$DF
	sta	tpi1+tpiPortB		; Write-Leitung rÅcksetzen
	ora	#$20
	sta	tpi1+tpiPortB		; und wieder setzen
	lda	#%10000			; FLAG (=Read) testen
	bit	cia+IntCtrReg
	cli
	jmp	L3D2B			; Bus Reset

; -------------------------------------------------------------------------
IRQ:
	lda	IndReg
	pha
	cld
	lda	tpi1+tpiActIntReg
	bne	L3AF0
L3AED:
	jmp	IRQEnd2
; -------------------------------------------------------------------------
L3AF0:
	pha
	lsr	a
	pla
	bcs	L3AF8
L3AF5:
	jmp	do_IRQ1
; -------------------------------------------------------------------------
L3AF8:
	jsr	jmp_scnkey
	lda	RepeatCount
	cmp	#$03		; wurde RepeatCount rÅckgesetzt ?
	bne	L3B34		; Nein
	lsr	RepeatCount	; sonst 1 nach RepeatCount
L3B34:
	jsr	do_udtim
L3B41:
	jmp	IRQEnd1
; -------------------------------------------------------------------------
SerTalk:
	ora	#$40
	.byte	$2C
SerListen:
	ora	#$20
	pha
	jsr	L3D2B
	pla
	pha
	lda	SecondAdr
	bpl	L3B8E
	pla
	jsr	L3B9F
	jmp	SetATNHigh
; -------------------------------------------------------------------------
L3B8E:
	pla
	jmp	L3B9F
; -------------------------------------------------------------------------
L3B92:
	pha
	lda	CTemp
	bpl	L3B9E
	sta	PrtData
	jsr	L3BBD
	lsr	CTemp
L3B9E:
	pla
L3B9F:
	lsr	PrtData
L3BA1:
	sta	snsw1
	sei
	jsr	SetDataHigh
	jsr	SetATNLow
L3BAA:
	sei
	jsr	SetClkLow
	jsr	SetDataHigh
	tya
	pha
	ldy	#$20
L3BB5:
	jsr	WaitLoop
	dey
	bne	L3BB5
	pla
	tay
L3BBD:
	sei
	jsr	L3D3C
	bne	L3C1B
	jsr	SetClkHigh
	bit	PrtData
	bpl	L3BD4
L3BCA:
	jsr	L3D44
	beq	L3BCA
L3BCF:
	jsr	L3D44
	bne	L3BCF
L3BD4:
	jsr	L3D44
	beq	L3BD4
	jsr	SetClkLow
	txa
	ldx	#$03
L3BDF:
	dex
	bne	L3BDF
	tax
	lda	#$08
	sta	PrtData
L3BE7:
	jsr	L3D3C
	beq	L3C1E
	ror	snsw1
	bcs	L3BF5
	jsr	SetDataLow
	bcc	L3BF8
L3BF5:
	jsr	SetDataHigh
L3BF8:
	jsr	SetClkHigh
	txa
	ldx	#$04
L3BFE:
	dex
	bne	L3BFE
	tax
	jsr	SetClkLow
	dec	PrtData
	bne	L3BE7
	jsr	SetTimB32ms
L3C0C:
	lda	cia+IntCtrReg
	and	#$02
	bne	L3C1E
	jsr	L3D3C
	bne	L3C0C
	clc
	cli
	rts
; -------------------------------------------------------------------------
L3C1B:
	lda	#$80
	.byte	$2C
L3C1E:
	lda	#$01
	.byte	$2C
L3C21:
	lda	#$02
	jsr	OrStatus
	cli
	sec
	jmp	L3CDF
; -------------------------------------------------------------------------
SerTksa:
	jsr	L3BA1
	bcs	L3C47
	sei
	jsr	SetDataLow
	jsr	SetATNHigh
	jsr	SwClkInput
L3C42:
	jsr	L3D20
	bne	L3C42
L3C47:
	cli
	rts
; -------------------------------------------------------------------------
SerCiout:
	pha
	clc
	bit	CTemp
	bpl	L3C5A
	jsr	L3BBD
L3C5A:
	jmp	do_ciout_end
; -------------------------------------------------------------------------
SerAcptr:
	sei
	jsr	SwClkInput
	lda	#$08
	sta	PrtData
	sta	snsw1
L3C6F:
	jsr	L3D20
	beq	L3C6F
L3C74:
	jsr	SetTimB32ms
	jsr	SetDataHigh
L3C7A:
	lda	cia+IntCtrReg
	and	#$02
	bne	L3C88
	jsr	L3D20
	bne	L3C7A
	beq	L3C9F
L3C88:
	bit	snsw1
	bvs	L3C21
	jsr	SetDataLow
	txa
	ldx	#$0E
L3C92:
	dex
	bne	L3C92
	tax
	lda	#$40
	jsr	OrStatus
	sta	snsw1
	bne	L3C74
L3C9F:
	jsr	L3D20
	beq	L3C9F
	lda	tpi1+tpiPortB
	and	#$20
	cmp	#$20
	ror	snsw1
L3CAD:
	jsr	L3D20
	bne	L3CAD
	dec	PrtData
	bne	L3C9F
	jsr	SetDataLow
	bit	Status
	bvc	L3CC0
	jsr	L3CE2
L3CC0:
	lda	snsw1
	cli
	clc
	rts
; -------------------------------------------------------------------------
SerUntalk:
	lda	#$5F
	jsr	L3B9F
	jmp	L3CDF
; -------------------------------------------------------------------------
SerUnListen:
	lda	#$3F
	jsr	L3B92
L3CDF:
	jsr	SetATNHigh
L3CE2:
	jsr	WaitLoop
	jsr	SetClkHigh
SetDataHigh:
	lda	tpi1+tpiPortB
	ora	#$20
	bne	L3CF4
SetDataLow:
	lda	tpi1+tpiPortB
	and	#$DF
L3CF4:
	sta	tpi1+tpiPortB
	lda	tpi1+tpiDDRB
	ora	#$20
	sta	tpi1+tpiDDRB
	rts
; -------------------------------------------------------------------------
SerSecond:
	sta	snsw1
	jsr	L3BAA
	jmp	SetATNHigh
; -------------------------------------------------------------------------
SetATNLow:
	lda	tpi1+tpiPortB
	ora	#$40
	bne	L3D1C
SetATNHigh:
	lda	tpi1+tpiPortB
	and	#$BF
L3D1C:
	sta	tpi1+tpiPortB
	rts
; -------------------------------------------------------------------------
L3D20:
	lda	tpi1+tpiPortB
	cmp	tpi1+tpiPortB
	bne	L3D20
	and	#$80
	rts
; -------------------------------------------------------------------------
L3D2B:
	lda	tpi1+tpiDDRB
	ora	#$40
	sta	tpi1+tpiDDRB
	jsr	SetClkHigh
	jsr	SetATNHigh
	jmp	SetDataHigh
; -------------------------------------------------------------------------
L3D3C:
	lda	tpi1+tpiDDRB
	and	#$DF
	sta	tpi1+tpiDDRB
L3D44:
	lda	tpi1+tpiPortB
	cmp	tpi1+tpiPortB
	bne	L3D44
	and	#$20
	rts
; -------------------------------------------------------------------------
SetClkHigh:
	lda	tpi1+tpiPortB
	ora	#$80
	bne	L3D69
SetClkLow:
	lda	tpi1+tpiPortB
	and	#$7F
L3D69:
	sta	tpi1+tpiPortB
	lda	tpi1+tpiDDRB
	ora	#$80
	bne	L3D78
SwClkInput:
	lda	tpi1+tpiDDRB
	and	#$7F
L3D78:
	sta	tpi1+tpiDDRB
	rts
; -------------------------------------------------------------------------
WaitLoop:
	txa
	ldx	#$10
L3D7F:
	dex
	bne	L3D7F
	tax
	rts

