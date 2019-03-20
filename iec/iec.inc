
IECFlags      = $D7FC
IECTmp	      = $D7FD
IECStatus  	  = $D7FF
IECStatus2    = $D7FE

MyInit:
		jsr SerBusInit
		lda #$40
		sta IECFlags
		lda #0
		sta IECStatus
		sta IECStatus2
		jmp MyVecInit
		
; -------------------------------------------------------------------------
; Check status of the given device
; Input: A - device number
; Output: IECStatus - device status (bit 7 = 0 -> IEEE, = 1 -> IEC)
; -------------------------------------------------------------------------
MyCheckDevice:
		pha
		lda IECFlags
		bpl DoNotForce				; Check whether we are forcing a certain type of device
		asl a
		asl a
		and #$80
		ora #$20
		sta IECTmp
		pla
		pha
		and #$1F
		ora IECTmp
		bne DeviceOverride
DoNotForce:
		pla
		pha
		cmp #6
		bcs NotAPrinter
		bit IECFlags
		bvc NotAPrinter
		ora #$60
		bne DeviceOverride
NotAPrinter:
		pla
		pha
		eor IECStatus				; First look in the saved status buffer
		and #$1F
		beq DeviceFound1			; Device found in the current status byte
		pla
		pha
		eor IECStatus2				; Then look in the saved status buffer
		and #$1F
		clc
		beq DeviceFound2			; Device found in the previous status byte
		sec
DeviceFound2:
		lda IECStatus				; Swap current and previous status
		pha
		lda IECStatus2
		sta IECStatus
		pla
		sta IECStatus2
		bcc DeviceFound1
DeviceNotFound:
		pla
		jmp MyTestDevice			; Device not found in both buffers, recheck
DeviceFound1:
		lda IECStatus				; Do we need to recheck this device?
		and #$C0
		cmp #$C0
		beq DeviceNotFound
		lda IECStatus
		and #$20
		bne DeviceNotFound
		pla
		rts
DeviceOverride:
		pha							; Manually set device status
		eor IECStatus				
		and #$1F
		beq DeviceOverride1			
		lda IECStatus
		sta IECStatus2
DeviceOverride1:
		pla
		sta IECStatus
		pla
		rts
		
; -------------------------------------------------------------------------
; Look for a device on the IEEE and IEC bus
; Input: A - device number
; Output: IECStatus - device status (bit 7 = 0 -> IEEE, = 1 -> IEC)
; -------------------------------------------------------------------------
MyTestDevice:
		sta IECStatus
		pha
		lda #0
		sta Status
		pla
		pha
		jsr SearchForAddr
		jsr do_listen
		lda IECTmp
		ora #$F0
		jsr do_second
		lda #$23
		jsr do_ciout
		lda #$39
		jsr do_ciout
		jsr do_unlisten
		lda Status
		and #$80
		ora IECStatus
		sta IECStatus
		lda #0
		sta Status
		pla
		rts

; -------------------------------------------------------------------------
; Check and mark IEC device as non functioning
; -------------------------------------------------------------------------
MyMarkIEC:
		bit Status
		bpl MyDontMark
		pha
		lda IECStatus
		ora #$40
		sta IECStatus
		pla
MyDontMark:
		rts

; -------------------------------------------------------------------------
; Function wrappers which select appropriate IEEE or IEC function
; -------------------------------------------------------------------------
MyTalk:
		jsr MyCheckDevice
		bit IECStatus
		bmi MyTalk2
		jmp do_talk
MyTalk2:
		jmp SerTalk
		
MyListen:
		jsr MyCheckDevice
		bit IECStatus
		bmi MyListen2
		jmp do_listen
MyListen2:
		jmp SerListen

MyUnTalk:
		bit IECStatus
		bmi MyUnTalk1
		jmp do_untalk
MyUntalk1:
		jsr SerUnTalk
		jmp MyMarkIEC
		
MyUnListen:
		bit IECStatus
		bmi MyUnListen1
		jmp do_unlisten
MyUnListen1:
		jsr SerUnListen
		jmp MyMarkIEC

MySecond:
		bit IECStatus
		bmi MySecond1
		jmp do_second
MySecond1:
		jsr SerSecond
		jmp MyMarkIEC

MyTksa:
		bit IECStatus
		bmi MyTksa1
		jmp do_tksa
MyTksa1:
		jsr SerTksa
		jmp MyMarkIEC

MyAcptr:
		bit IECStatus
		bmi MyAcptr1
		jmp do_acptr
MyAcptr1:
		jsr SerAcptr
		jmp MyMarkIEC

MyCiout:
		bit IECStatus
		bmi MyCiout1
		jmp do_ciout
MyCiout1:
		jsr SerCiout
		jmp MyMarkIEC

; -------------------------------------------------------------------------
; Search for unused second address
; Input: none
; Output: IECTmp - first unused address
; -------------------------------------------------------------------------
SearchForAddr:
		sei
		pha
		txa
		pha
		tya
		pha
		lda #0
		sta Status
		ldy #2
CheckOneAddr:
		ldx #9
CheckOneFile:
		jsr SearchForOneAddr
		bcs AddrIsFound
		dex
		bpl CheckOneFile
AddrIsFree:
		sty IECTmp
		pla
		tay
		pla
		tax
		pla
		cli
		rts
AddrIsFound:
		iny
		cpy #15
		bcc CheckOneAddr
		ldy #0
		beq AddrIsFree

; -------------------------------------------------------------------------
; Check if a file is open with specified second addr
; Input: X - file position, Y - second addr
; Output: C = 1 if second addr found
; -------------------------------------------------------------------------
SearchForOneAddr:
		lda SecondAdrtable,x
		and #$0F
		sta IECTmp
		cpy IECTmp
		bne NotThisDevice
		sec
		rts
NotThisDevice:
		clc
		rts

MyVecInit:
	ldx	#16-1
L3AAD:	lda	MyVecTab,x
	sta	secndVec,x
	dex
	bpl	L3AAD

; Neuen IRQ-Vektor eintragen
	sei
	lda	#<IRQ
	ldy	#>IRQ
	sta	IRQvec
	sty	IRQVec+1
	cli
	rts

MyVecTab:
	.word	MySecond
	.word	MyTksa
	.word	MyAcptr
	.word	MyCiout
	.word	MyUntalk
	.word	MyUnListen
	.word	MyListen
	.word	MyTalk
