.info LDA, LDX, LDY, STA, STX, STY
.device ../mos6502.so

.pindef clk1.pin clk2.pin ab.pin db.pin rw.pin sync.pin pc.reg p.reg a.reg x.reg y.reg i.reg

.memset $0000

.memset $0200   ! Bytes to be copied to $0400
    $11 $22 $33

.memset $0300   ! X and Y offsets
    $01 $02

.memset $8000
    ! Copy $11 from $0200 to $0400
    $AD $00 $02 ! LDA $0200
    $8D $00 $04 ! STA $0400

    ! Copy $22 from $0201 to $0401
    $AE $00 $03 ! LDX $0300
    $BD $00 $02 ! LDA $0200,X
    $A0 $01     ! LDY #$01
    $99 $00 $04 ! STA $0400,Y

    ! Copy $33 from $0202 to $0402
    $AC $01 $03 ! LDY $0301
    $B9 $00 $02 ! LDA $0200,Y
    $A2 $02     ! LDX #$02
    $9D $00 $04 ! STA $0400,X

.memset $FFFC
    $00 $80     ! RESET => $8000

.reset
.run 8

.info Copy $11 from $0200 to $0400
.info LDA $0200
.step .pintest 1 0 $8000 $80 1 1 $8000 %XXXX01XX $XX $XX $XX $00
.step .pintest 0 1 $8000 $AD 1 1 $8000 %XXXX01XX $XX $XX $XX $00

.step .pintest 1 0 $8001 $AD 1 0 $8001 %XXXX01XX $XX $XX $XX $AD
.step .pintest 0 1 $8001 $00 1 0 $8001 %XXXX01XX $XX $XX $XX $AD

.step .pintest 1 0 $8002 $00 1 0 $8002 %XXXX01XX $XX $XX $XX $AD
.step .pintest 0 1 $8002 $02 1 0 $8002 %XXXX01XX $XX $XX $XX $AD

.step .pintest 1 0 $0200 $02 1 0 $8003 %XXXX01XX $XX $XX $XX $AD
.step .pintest 0 1 $0200 $11 1 0 $8003 %XXXX01XX $XX $XX $XX $AD

.info STA $0400
.step .pintest 1 0 $8003 $11 1 1 $8003 %0XXX010X $11 $XX $XX $AD
.step .pintest 0 1 $8003 $8D 1 1 $8003 %0XXX010X $11 $XX $XX $AD

.step .pintest 1 0 $8004 $8D 1 0 $8004 %0XXX010X $11 $XX $XX $8D
.step .pintest 0 1 $8004 $00 1 0 $8004 %0XXX010X $11 $XX $XX $8D

.step .pintest 1 0 $8005 $00 1 0 $8005 %0XXX010X $11 $XX $XX $8D
.step .pintest 0 1 $8005 $04 1 0 $8005 %0XXX010X $11 $XX $XX $8D

.step .pintest 1 0 $0400 $04 0 0 $8006 %0XXX010X $11 $XX $XX $8D
.step .pintest 0 1 $0400 $11 0 0 $8006 %0XXX010X $11 $XX $XX $8D

.info Copy $22 from $0201 to $0401
.info LDX $0300
.step .pintest 1 0 $8006 $04 1 1 $8006 %0XXX010X $11 $XX $XX $8D
.step .pintest 0 1 $8006 $AE 1 1 $8006 %0XXX010X $11 $XX $XX $8D

.step .pintest 1 0 $8007 $AE 1 0 $8007 %0XXX010X $11 $XX $XX $AE
.step .pintest 0 1 $8007 $00 1 0 $8007 %0XXX010X $11 $XX $XX $AE

.step .pintest 1 0 $8008 $00 1 0 $8008 %0XXX010X $11 $XX $XX $AE
.step .pintest 0 1 $8008 $03 1 0 $8008 %0XXX010X $11 $XX $XX $AE

.step .pintest 1 0 $0300 $03 1 0 $8009 %0XXX010X $11 $XX $XX $AE
.step .pintest 0 1 $0300 $01 1 0 $8009 %0XXX010X $11 $XX $XX $AE

.info LDA $0200,X
.step .pintest 1 0 $8009 $01 1 1 $8009 %0XXX010X $11 $01 $XX $AE
.step .pintest 0 1 $8009 $BD 1 1 $8009 %0XXX010X $11 $01 $XX $AE

.step .pintest 1 0 $800A $BD 1 0 $800A %0XXX010X $11 $01 $XX $BD
.step .pintest 0 1 $800A $00 1 0 $800A %0XXX010X $11 $01 $XX $BD

.step .pintest 1 0 $800B $00 1 0 $800B %0XXX010X $11 $01 $XX $BD
.step .pintest 0 1 $800B $02 1 0 $800B %0XXX010X $11 $01 $XX $BD

.step .pintest 1 0 $0201 $02 1 0 $800C %0XXX010X $11 $01 $XX $BD
.step .pintest 0 1 $0201 $22 1 0 $800C %0XXX010X $11 $01 $XX $BD

.info LDY #$01
.step .pintest 1 0 $800C $22 1 1 $800C %0XXX010X $22 $01 $XX $BD
.step .pintest 0 1 $800C $A0 1 1 $800C %0XXX010X $22 $01 $XX $BD

.step .pintest 1 0 $800D $A0 1 0 $800D %0XXX010X $22 $01 $XX $A0
.step .pintest 0 1 $800D $01 1 0 $800D %0XXX010X $22 $01 $XX $A0

.info STA $0400,Y
.step .pintest 1 0 $800E $01 1 1 $800E %0XXX010X $22 $01 $01 $A0
.step .pintest 0 1 $800E $99 1 1 $800E %0XXX010X $22 $01 $01 $A0

.step .pintest 1 0 $800F $99 1 0 $800F %0XXX010X $22 $01 $01 $99
.step .pintest 0 1 $800F $00 1 0 $800F %0XXX010X $22 $01 $01 $99

.step .pintest 1 0 $8010 $00 1 0 $8010 %0XXX010X $22 $01 $01 $99
.step .pintest 0 1 $8010 $04 1 0 $8010 %0XXX010X $22 $01 $01 $99

.step .pintest 1 0 $0401 $04 1 0 $8011 %0XXX010X $22 $01 $01 $99
.step .pintest 0 1 $0401 $00 1 0 $8011 %0XXX010X $22 $01 $01 $99

.step .pintest 1 0 $0401 $00 0 0 $8011 %0XXX010X $22 $01 $01 $99
.step .pintest 0 1 $0401 $22 0 0 $8011 %0XXX010X $22 $01 $01 $99

.info Copy $33 from $0202 to $0402
.info LDY $0301
.step .pintest 1 0 $8011 $00 1 1 $8011 %0XXX010X $22 $01 $01 $99
.step .pintest 0 1 $8011 $AC 1 1 $8011 %0XXX010X $22 $01 $01 $99

.step .pintest 1 0 $8012 $AC 1 0 $8012 %0XXX010X $22 $01 $01 $AC
.step .pintest 0 1 $8012 $01 1 0 $8012 %0XXX010X $22 $01 $01 $AC

.step .pintest 1 0 $8013 $01 1 0 $8013 %0XXX010X $22 $01 $01 $AC
.step .pintest 0 1 $8013 $03 1 0 $8013 %0XXX010X $22 $01 $01 $AC

.step .pintest 1 0 $0301 $03 1 0 $8014 %0XXX010X $22 $01 $01 $AC
.step .pintest 0 1 $0301 $02 1 0 $8014 %0XXX010X $22 $01 $01 $AC

.info LDA $0200,Y
.step .pintest 1 0 $8014 $02 1 1 $8014 %0XXX010X $22 $01 $02 $AC
.step .pintest 0 1 $8014 $B9 1 1 $8014 %0XXX010X $22 $01 $02 $AC

.step .pintest 1 0 $8015 $B9 1 0 $8015 %0XXX010X $22 $01 $02 $B9
.step .pintest 0 1 $8015 $00 1 0 $8015 %0XXX010X $22 $01 $02 $B9

.step .pintest 1 0 $8016 $00 1 0 $8016 %0XXX010X $22 $01 $02 $B9
.step .pintest 0 1 $8016 $02 1 0 $8016 %0XXX010X $22 $01 $02 $B9

.step .pintest 1 0 $0202 $02 1 0 $8017 %0XXX010X $22 $01 $02 $B9
.step .pintest 0 1 $0202 $33 1 0 $8017 %0XXX010X $22 $01 $02 $B9

.info LDX #$02
.step .pintest 1 0 $8017 $33 1 1 $8017 %0XXX010X $33 $01 $02 $B9
.step .pintest 0 1 $8017 $A2 1 1 $8017 %0XXX010X $33 $01 $02 $B9

.step .pintest 1 0 $8018 $A2 1 0 $8018 %0XXX010X $33 $01 $02 $A2
.step .pintest 0 1 $8018 $02 1 0 $8018 %0XXX010X $33 $01 $02 $A2

.info STA $0400,X
.step .pintest 1 0 $8019 $02 1 1 $8019 %0XXX010X $33 $02 $02 $A2
.step .pintest 0 1 $8019 $9D 1 1 $8019 %0XXX010X $33 $02 $02 $A2

.step .pintest 1 0 $801A $9D 1 0 $801A %0XXX010X $33 $02 $02 $9D
.step .pintest 0 1 $801A $00 1 0 $801A %0XXX010X $33 $02 $02 $9D

.step .pintest 1 0 $801B $00 1 0 $801B %0XXX010X $33 $02 $02 $9D
.step .pintest 0 1 $801B $04 1 0 $801B %0XXX010X $33 $02 $02 $9D

.step .pintest 1 0 $0402 $04 1 0 $801C %0XXX010X $33 $02 $02 $9D
.step .pintest 0 1 $0402 $00 1 0 $801C %0XXX010X $33 $02 $02 $9D

.step .pintest 1 0 $0402 $00 0 0 $801C %0XXX010X $33 $02 $02 $9D
.step .pintest 0 1 $0402 $33 0 0 $801C %0XXX010X $33 $02 $02 $9D

.memtest $0400
    $11 $22 $33
