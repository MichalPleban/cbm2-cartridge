
BannerSrc = $F0
BannerDst = $F2

CRTC_RegNo = $d800
CRTC_RegVal = $d801


BannerShow:
        ldx #$03
        jsr CHKOUT
        lda #147
        jsr BSOUT
        lda #$0F
        sta $01
        lda $DE06
        ora #$10
        sta $DE06
        lda #128
       	sta BellMode
    	lda #$60
    	ldx #$0a
    	stx CRTC_RegNo
    	sta CRTC_RegVal
        lda #$40
        sta BannerDst
        lda #$D1
        sta BannerDst+1
        lda #<BannerData
        sta BannerSrc
        lda #>BannerData
        sta BannerSrc+1
        jsr BannerDraw
        lda #$20            ; +480
        sta BannerDst
        lda #$D3
        sta BannerDst+1
        lda #<BannerData2
        sta BannerSrc
        lda #>BannerData2
        sta BannerSrc+1
        jsr BannerDraw
        clc
        ldx #18
        ldy #0
        jsr $E019
		lda #<BannerText
		ldx #>BannerText
		jmp BASICBanner		
    
BannerDraw:
        ldy #0
BannerDraw1:
        lda (BannerSrc),y
        pha
        and #$0F
        jsr BannerChar
        inc BannerDst
        bne BannerDraw2
        inc BannerDst+1
BannerDraw2:
        pla
        lsr a
        lsr a
        lsr a
        lsr a
        jsr BannerChar
        iny
        cpy #240
        bne BannerDraw1
        rts

BannerChar:
        tax
        lda BannerCodes,x
        sta (BannerDst),y
        rts

BannerText:        
		.byte   "8088 BOOTSTRAP V1.2 (C) 2020 MICHAL PLEBAN", 13, 0

BannerData:
        .byte   $00, $00, $00, $00, $00, $00, $F0, $0F, $00, $00, $00, $00, $00, $00, $FF, $00, $00, $00, $00, $00
        .byte   $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $F0, $0F, $00, $00, $00, $00
        .byte   $FF, $00, $00, $00, $00, $00, $F0, $0F, $00, $00, $00, $00, $00, $00, $FF, $00, $00, $00, $00, $FF
        .byte   $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $FF, $00, $00, $00, $F0, $0F, $00, $00, $00, $00
        .byte   $33, $00, $00, $00, $00, $00, $F0, $0F, $00, $00, $00, $00, $00, $00, $FF, $00, $00, $00, $00, $33
        .byte   $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $33, $00, $00, $00, $F0, $0F, $00, $00, $00, $00
        .byte   $CC, $00, $CC, $CC, $4C, $00, $F0, $CF, $0C, $00, $C0, $CC, $00, $00, $FF, $00, $00, $00, $00, $CC
        .byte   $00, $CC, $CC, $4C, $00, $00, $C0, $CC, $00, $00, $CC, $00, $00, $CC, $FC, $0F, $00, $C0, $CC, $00
        .byte   $FF, $00, $FF, $FF, $FF, $0D, $F0, $FF, $0F, $80, $FF, $FF, $4F, $00, $FF, $00, $00, $00, $00, $FF
        .byte   $00, $FF, $FF, $FF, $0D, $80, $FF, $FF, $CF, $00, $FF, $00, $F8, $FF, $FF, $0F, $80, $FF, $FF, $4F
        .byte   $FF, $00, $FF, $00, $F2, $0F, $F0, $0F, $00, $F0, $1F, $20, $FF, $00, $FF, $00, $00, $00, $00, $FF
        .byte   $00, $FF, $00, $F2, $0F, $F0, $1F, $20, $FB, $00, $FF, $00, $FF, $01, $F0, $0F, $F0, $1F, $20, $FF
BannerData2:
        .byte   $FF, $00, $FF, $00, $F0, $0F, $F0, $0F, $00, $F0, $0F, $00, $FF, $00, $FF, $00, $00, $00, $00, $FF
        .byte   $00, $FF, $00, $F0, $0F, $F0, $4F, $00, $00, $00, $FF, $00, $FF, $00, $F0, $0F, $F0, $0F, $00, $FF
        .byte   $FF, $00, $FF, $00, $F0, $0F, $F0, $0F, $00, $F0, $FF, $FF, $FF, $00, $FF, $00, $00, $00, $00, $FF
        .byte   $00, $FF, $00, $F0, $0F, $20, $FF, $FF, $0C, $00, $FF, $00, $FF, $00, $F0, $0F, $F0, $FF, $FF, $FF
        .byte   $FF, $00, $FF, $00, $F0, $0F, $F0, $0F, $00, $F0, $3F, $33, $33, $00, $FF, $00, $00, $00, $00, $FF
        .byte   $00, $FF, $00, $F0, $0F, $00, $30, $B3, $DF, $00, $FF, $00, $FF, $00, $F0, $0F, $F0, $3F, $33, $33
        .byte   $FF, $00, $FF, $00, $F0, $0F, $F0, $0F, $00, $F0, $0F, $00, $CC, $00, $FF, $00, $00, $00, $00, $FF
        .byte   $00, $FF, $00, $F0, $0F, $C0, $04, $00, $FF, $00, $FF, $00, $FF, $00, $F0, $0F, $F0, $0F, $00, $CC
        .byte   $FF, $00, $FF, $00, $F0, $0F, $B0, $DF, $0C, $B0, $DF, $EC, $7F, $00, $FF, $00, $00, $00, $00, $FF
        .byte   $00, $FF, $00, $F0, $0F, $F0, $DF, $EC, $7F, $00, $FF, $00, $FB, $CD, $FC, $0F, $B0, $DF, $EC, $7F
        .byte   $FF, $00, $FF, $00, $F0, $0F, $00, $F3, $0F, $00, $F3, $FF, $03, $00, $FF, $00, $00, $00, $00, $FF
        .byte   $00, $FF, $00, $F0, $0F, $00, $F3, $FF, $03, $00, $FF, $00, $30, $FF, $FF, $0F, $00, $F3, $FF, $03
    
BannerCodes:
        ; 0 ..
        ;   ..
        .byte $20
        ; 1 X.
        ;   ..
        .byte $7E
        ; 2 .X
        ;   ..
        .byte $7C
        ; 3 XX
        ;   ..
        .byte $E2
        ; 4 ..
        ;   X.
        .byte $7B
        ; 5 X.
        ;   X.
        .byte $61
        ; 6 .X
        ;   X.
        .byte $FF
        ; 7 XX
        ;   X.
        .byte $EC
        ; 8 ..
        ;   .X
        .byte $6C
        ; 9 X.
        ;   .X
        .byte $7F
        ; A .X
        ;   .X
        .byte $E1
        ; B XX
        ;   .X
        .byte $FB
        ; C ..
        ;   XX
        .byte $62
        ; D X.
        ;   XX
        .byte $FC
        ; E .X
        ;   XX
        .byte $FE
        ; F XX
        ;   XX
        .byte $A0
