
WedgeDevice = $0220
WedgeTmp    = $D7FB

; --------------------------------------------------------
; Install wedge routine hook into BASIC
; --------------------------------------------------------
WedgeInstall:
		lda #<WedgeHook
		sta $0282
		lda #>WedgeHook
		sta $0283
		lda #8
		sta WedgeDevice
		rts
		
; --------------------------------------------------------
; Hook into BASIC "process next line" routine
; --------------------------------------------------------
WedgeHook:
		ldx #$ff
		stx $43
		stx $0298
		stx $0299
		stx $8f
		jsr BasicInputLine
		sta $85
		sty $86
		stx $87
		jsr Chrget
		tax
		beq WedgeHook
		bcc WedgeLineNumber
		cmp #'@'
		beq WedgeExecute
		bit $8001
		bmi WedgeFollow256
		jmp $85EA
WedgeFollow256:
		jmp $85E2
WedgeLineNumber:
		bit $8001
		bmi WedgeLineNumber256
		jmp $85F3
WedgeLineNumber256:
		jmp $85EB
		
; --------------------------------------------------------
; Execure wedge command
; --------------------------------------------------------
WedgeExecute:
		lda #0
		sta Status
		lda 1
		pha
		lda $87
		sta 1
		lda #0
		sta WedgeTmp
		ldy #1
		lda ($85),y
		beq WedgeStatus
		cmp #$5F
		beq WedgeChangeNumber
		cmp #'0'
		bcc WedgeExecute1
		cmp #'9'+1
		bcc WedgeSetNumber
WedgeExecute1:
		jsr WedgeCommand
WedgeEnd:
		pla
		sta 1
		bit Status
		bpl WedgeEnd1
		ldx #10
		jmp BasicPrintError		
WedgeEnd1:
		jmp BasicPrintReady

; --------------------------------------------------------
; Set drive number
; --------------------------------------------------------
WedgeChangeNumber:
		iny
		lda ($85),y
		jsr WedgeGetNumber
		cmp #8
		bcc WedgeError
		cmp #32
		bcs WedgeError
		sta WedgeTmp
		lda #<CommandSetNumber
		sta $85
		lda #>CommandSetNumber
		sta $86
		lda #$0F
		sta 1
		ldy #0
		jsr WedgeCommand
		bit Status
		bmi WedgeChangeNumber1
		lda WedgeTmp
		sta WedgeDevice
		lda #0
		sta IECStatus
		sta IECStatus2
WedgeChangeNumber1:
		jmp WedgeEnd
		
; --------------------------------------------------------
; Show drive status
; --------------------------------------------------------
WedgeStatus:
		jmp WedgeEnd	
			
; --------------------------------------------------------
; "ILLEGAL QUANTITY ERROR"
; --------------------------------------------------------
WedgeError:
		pla
		sta 1
		ldx #18
		jmp BasicPrintError
				
; --------------------------------------------------------
; Change current drive number
; --------------------------------------------------------
WedgeSetNumber:
		jsr WedgeGetNumber
		cmp #$72
		beq WedgeSet8050
		cmp #$3A
		beq WedgeSet8250
		cmp #8
		bcc WedgeError
		cmp #32
		bcs WedgeError
		sta WedgeDevice
		jmp WedgeEnd

; --------------------------------------------------------
; Send special commands to the drive
; --------------------------------------------------------
WedgeSet8050:
		lda #<CommandSet8050
		sta $85
		lda #>CommandSet8050
		sta $86
		bne WedgeSet
WedgeSet8250:
		lda #<CommandSet8250
		sta $85
		lda #>CommandSet8250
		sta $86
WedgeSet:
		lda #$0F
		sta 1
		ldy #0
		jsr WedgeCommand
		lda #<CommandSoftReset
		sta $85
		lda #>CommandSoftReset
		sta $86
		ldy #0
		jsr WedgeCommand
		jmp WedgeEnd
		
; --------------------------------------------------------
; Send command to the drive
; --------------------------------------------------------
WedgeCommand:
		lda WedgeDevice
		jsr MyListen
		lda #$FF
		jsr MySecond
WedgeCommand2:
		lda ($85),y
		beq WedgeCommand1
		jsr MyCiout
		iny
		bne WedgeCommand2
WedgeCommand1:
		lda WedgeTmp
		beq WedgeCommand5
		lda #0
		jsr MyCiout
		lda #2
		jsr MyCiout
		lda WedgeTmp
		ora #32
		jsr MyCiout
		lda WedgeTmp
		ora #64
		jsr MyCiout
WedgeCommand5:
		jsr MyUnlisten
		lda WedgeDevice
		jsr MyTalk
		lda #$6F
		jsr MyTksa
WedgeCommand4:
		jsr MyAcptr
		tax
		lda Status
		bne WedgeCommand3
		txa
		jsr $FFD2
		jmp WedgeCommand4
WedgeCommand3:
		lda #13
		jsr $FFD2		
		lda WedgeDevice
		jsr MyListen
		lda #$EF
		jsr MySecond
		jsr MyUnlisten
		rts

; --------------------------------------------------------
; Retrieve a number from command line
; --------------------------------------------------------
WedgeGetNumber:
		sec
		sbc #'0'
		sta $87
WedgeGetNumber1:
		iny
		lda ($85),y
		beq WedgeGetNumber2
		cmp #'0'
		bcc WedgeGetNumber2
		cmp #'9'+1
		bcs WedgeGetNumber2
		sec
		sbc #'0'
		tax
		asl $87
		lda $87
		asl a
		asl a
		clc
		adc $87
		sta $87
		clc
		txa
		adc $87
		sta $87
		jmp WedgeGetNumber1
WedgeGetNumber2:
		lda $87
		rts

; --------------------------------------------------------
; Special commands
; --------------------------------------------------------
CommandSoftReset:
		.byte "U9", 0
CommandSetNumber:
		.byte "M-W", 12, 0
CommandSet8050:
		.byte "M-W", 172, 16, 1, 1, 0
CommandSet8250:
		.byte "M-W", 172, 16, 1, 2, 0

Chrget:
		bit $8001
		bmi Chrget256
		jmp $BA26
Chrget256:
		jmp $B988 

Chrgot:
		bit $8001
		bmi Chrgot256
		jmp $BA29
Chrgot256:
		jmp $B98B 

BasicInputLine:
		bit $8001
		bmi BasicInputLine256
		jmp $86E3
BasicInputLine256:
		jmp $86DB			

BasicPrintReady:
		bit $8001
		bmi BasicPrintReady256
		jmp $85C0
BasicPrintReady256:
		jmp $85B8			
		
BasicPrintError:
		bit $8001
		bmi BasicPrintError256
		jmp $8555
BasicPrintError256:
		jmp $854D			
		