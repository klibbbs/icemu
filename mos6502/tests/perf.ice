.info Measure performance
.memset $0200
    $77 $99 $BB $DD $FF
.memset $8000
    $A9 $66     ! LDA #$66
    $8D $00 $03 ! STA $0300
    $AD $00 $02 ! LDA $0200
    $8D $01 $03 ! STA $0301
    $A9 $88     ! LDA #$88
    $8D $02 $03 ! STA $0302
    $AD $01 $02 ! LDA $0201
    $8D $03 $03 ! STA $0303
    $A9 $AA     ! LDA #$AA
    $8D $04 $03 ! STA $0304
    $AD $02 $02 ! LDA $0202
    $8D $05 $03 ! STA $0305
    $A9 $CC     ! LDA #$CC
    $8D $06 $03 ! STA $0306
    $AD $03 $02 ! LDA $0203
    $8D $07 $03 ! STA $0307
    $A9 $EE     ! LDA #$EE
    $8D $08 $03 ! STA $0308
    $AD $04 $02 ! LDA $0204
    $8D $09 $03 ! STA $0309
.memset $FFFC
    $00 $80     ! RESET => $8000
.reset

.info RESET sequence
.run 8
.info Program start
.run 70
.memtest $0300 $66 $77 $88 $99 $AA $BB $CC $DD $EE $FF
