.info LDA, LDX, LDY, STA, STX, STY (31 opcodes)
.device ../mos6502.so
.pindef clk1.pin clk2.pin ab.pin db.pin rw.pin sync.pin pc.reg p.reg a.reg x.reg y.reg i.reg

.memset $0020   ! Bytes to be copied to $0040
    $88 $99 $AA

.memset $0080   ! X and Y offsets
    $03 $04

.memset $0200   ! Bytes to be copied to $0400
    $11 $22 $33 $44 $55

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

    ! Copy $44 from $0203 to $0403
    $A4 $80     ! LDY $80
    $BE $00 $02 ! LDX $0200,Y
    $8E $03 $04 ! STX $0403

    ! Copy $55 from $0204 to $0404
    $A6 $81     ! LDX $81
    $BC $00 $02 ! LDY $0200,X
    $8C $04 $04 ! STY $0404

    ! Copy $88 from $0020 to $0040
    $A5 $20     ! LDA $20
    $85 $40     ! STA $40

    ! Copy $99 from $0021 to $0041 + $0042
    $A0 $01     ! LDY #$01
    $B6 $20     ! LDX $20,Y
    $86 $41     ! STX $41
    $96 $41     ! STX $41,Y

    ! Copy $AA from $0022 to $0043 + $0044
    $A2 $01     ! LDX #$01
    $B4 $21     ! LDY $21,X
    $84 $43     ! STY $43
    $94 $43     ! STY $43,X

    ! TODO: Test indexed addressing that crosses page boundaries

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

.info Copy $44 from $0203 to $0403
.info LDY $80
.step .pintest 1 0 $801C $00 1 1 $801C %0XXX010X $33 $02 $02 $9D
.step .pintest 0 1 $801C $A4 1 1 $801C %0XXX010X $33 $02 $02 $9D

.step .pintest 1 0 $801D $A4 1 0 $801D %0XXX010X $33 $02 $02 $A4
.step .pintest 0 1 $801D $80 1 0 $801D %0XXX010X $33 $02 $02 $A4

.step .pintest 1 0 $0080 $80 1 0 $801E %0XXX010X $33 $02 $02 $A4
.step .pintest 0 1 $0080 $03 1 0 $801E %0XXX010X $33 $02 $02 $A4

.info LDX $0200,Y
.step .pintest 1 0 $801E $03 1 1 $801E %0XXX010X $33 $02 $03 $A4
.step .pintest 0 1 $801E $BE 1 1 $801E %0XXX010X $33 $02 $03 $A4

.step .pintest 1 0 $801F $BE 1 0 $801F %0XXX010X $33 $02 $03 $BE
.step .pintest 0 1 $801F $00 1 0 $801F %0XXX010X $33 $02 $03 $BE

.step .pintest 1 0 $8020 $00 1 0 $8020 %0XXX010X $33 $02 $03 $BE
.step .pintest 0 1 $8020 $02 1 0 $8020 %0XXX010X $33 $02 $03 $BE

.step .pintest 1 0 $0203 $02 1 0 $8021 %0XXX010X $33 $02 $03 $BE
.step .pintest 0 1 $0203 $44 1 0 $8021 %0XXX010X $33 $02 $03 $BE

.info STX $0403
.step .pintest 1 0 $8021 $44 1 1 $8021 %0XXX010X $33 $44 $03 $BE
.step .pintest 0 1 $8021 $8E 1 1 $8021 %0XXX010X $33 $44 $03 $BE

.step .pintest 1 0 $8022 $8E 1 0 $8022 %0XXX010X $33 $44 $03 $8E
.step .pintest 0 1 $8022 $03 1 0 $8022 %0XXX010X $33 $44 $03 $8E

.step .pintest 1 0 $8023 $03 1 0 $8023 %0XXX010X $33 $44 $03 $8E
.step .pintest 0 1 $8023 $04 1 0 $8023 %0XXX010X $33 $44 $03 $8E

.step .pintest 1 0 $0403 $04 0 0 $8024 %0XXX010X $33 $44 $03 $8E
.step .pintest 0 1 $0403 $44 0 0 $8024 %0XXX010X $33 $44 $03 $8E

.info Copy $55 from $0204 to $0404
.info LDX $81
.step .pintest 1 0 $8024 $04 1 1 $8024 %0XXX010X $33 $44 $03 $8E
.step .pintest 0 1 $8024 $A6 1 1 $8024 %0XXX010X $33 $44 $03 $8E

.step .pintest 1 0 $8025 $A6 1 0 $8025 %0XXX010X $33 $44 $03 $A6
.step .pintest 0 1 $8025 $81 1 0 $8025 %0XXX010X $33 $44 $03 $A6

.step .pintest 1 0 $0081 $81 1 0 $8026 %0XXX010X $33 $44 $03 $A6
.step .pintest 0 1 $0081 $04 1 0 $8026 %0XXX010X $33 $44 $03 $A6

.info LDY $0200,X
.step .pintest 1 0 $8026 $04 1 1 $8026 %0XXX010X $33 $04 $03 $A6
.step .pintest 0 1 $8026 $BC 1 1 $8026 %0XXX010X $33 $04 $03 $A6

.step .pintest 1 0 $8027 $BC 1 0 $8027 %0XXX010X $33 $04 $03 $BC
.step .pintest 0 1 $8027 $00 1 0 $8027 %0XXX010X $33 $04 $03 $BC

.step .pintest 1 0 $8028 $00 1 0 $8028 %0XXX010X $33 $04 $03 $BC
.step .pintest 0 1 $8028 $02 1 0 $8028 %0XXX010X $33 $04 $03 $BC

.step .pintest 1 0 $0204 $02 1 0 $8029 %0XXX010X $33 $04 $03 $BC
.step .pintest 0 1 $0204 $55 1 0 $8029 %0XXX010X $33 $04 $03 $BC

.info STY $0404
.step .pintest 1 0 $8029 $55 1 1 $8029 %0XXX010X $33 $04 $55 $BC
.step .pintest 0 1 $8029 $8C 1 1 $8029 %0XXX010X $33 $04 $55 $BC

.step .pintest 1 0 $802A $8C 1 0 $802A %0XXX010X $33 $04 $55 $8C
.step .pintest 0 1 $802A $04 1 0 $802A %0XXX010X $33 $04 $55 $8C

.step .pintest 1 0 $802B $04 1 0 $802B %0XXX010X $33 $04 $55 $8C
.step .pintest 0 1 $802B $04 1 0 $802B %0XXX010X $33 $04 $55 $8C

.step .pintest 1 0 $0404 $04 0 0 $802C %0XXX010X $33 $04 $55 $8C
.step .pintest 0 1 $0404 $55 0 0 $802C %0XXX010X $33 $04 $55 $8C

.info Copy $88 from $0020 to $0040
.info LDA $20
.step .pintest 1 0 $802C $04 1 1 $802C %0XXX010X $33 $04 $55 $8C
.step .pintest 0 1 $802C $A5 1 1 $802C %0XXX010X $33 $04 $55 $8C

.step .pintest 1 0 $802D $A5 1 0 $802D %0XXX010X $33 $04 $55 $A5
.step .pintest 0 1 $802D $20 1 0 $802D %0XXX010X $33 $04 $55 $A5

.step .pintest 1 0 $0020 $20 1 0 $802E %0XXX010X $33 $04 $55 $A5
.step .pintest 0 1 $0020 $88 1 0 $802E %0XXX010X $33 $04 $55 $A5

.info STA $40
.step .pintest 1 0 $802E $88 1 1 $802E %1XXX010X $88 $04 $55 $A5
.step .pintest 0 1 $802E $85 1 1 $802E %1XXX010X $88 $04 $55 $A5

.step .pintest 1 0 $802F $85 1 0 $802F %1XXX010X $88 $04 $55 $85
.step .pintest 0 1 $802F $40 1 0 $802F %1XXX010X $88 $04 $55 $85

.step .pintest 1 0 $0040 $40 0 0 $8030 %1XXX010X $88 $04 $55 $85
.step .pintest 0 1 $0040 $88 0 0 $8030 %1XXX010X $88 $04 $55 $85

.info Copy $99 from $0021 to $0041 + $0042
.info LDY #$01

.step .pintest 1 0 $8030 $40 1 1 $8030 %1XXX010X $88 $04 $55 $85
.step .pintest 0 1 $8030 $A0 1 1 $8030 %1XXX010X $88 $04 $55 $85

.step .pintest 1 0 $8031 $A0 1 0 $8031 %1XXX010X $88 $04 $55 $A0
.step .pintest 0 1 $8031 $01 1 0 $8031 %1XXX010X $88 $04 $55 $A0

.info LDX $20,Y
.step .pintest 1 0 $8032 $01 1 1 $8032 %0XXX010X $88 $04 $01 $A0
.step .pintest 0 1 $8032 $B6 1 1 $8032 %0XXX010X $88 $04 $01 $A0

.step .pintest 1 0 $8033 $B6 1 0 $8033 %0XXX010X $88 $04 $01 $B6
.step .pintest 0 1 $8033 $20 1 0 $8033 %0XXX010X $88 $04 $01 $B6

.step .pintest 1 0 $0020 $20 1 0 $8034 %0XXX010X $88 $04 $01 $B6
.step .pintest 0 1 $0020 $88 1 0 $8034 %0XXX010X $88 $04 $01 $B6

.step .pintest 1 0 $0021 $88 1 0 $8034 %0XXX010X $88 $04 $01 $B6
.step .pintest 0 1 $0021 $99 1 0 $8034 %0XXX010X $88 $04 $01 $B6

.info STX $41
.step .pintest 1 0 $8034 $99 1 1 $8034 %1XXX010X $88 $99 $01 $B6
.step .pintest 0 1 $8034 $86 1 1 $8034 %1XXX010X $88 $99 $01 $B6

.step .pintest 1 0 $8035 $86 1 0 $8035 %1XXX010X $88 $99 $01 $86
.step .pintest 0 1 $8035 $41 1 0 $8035 %1XXX010X $88 $99 $01 $86

.step .pintest 1 0 $0041 $41 0 0 $8036 %1XXX010X $88 $99 $01 $86
.step .pintest 0 1 $0041 $99 0 0 $8036 %1XXX010X $88 $99 $01 $86

.info STX $41,Y
.step .pintest 1 0 $8036 $41 1 1 $8036 %1XXX010X $88 $99 $01 $86
.step .pintest 0 1 $8036 $96 1 1 $8036 %1XXX010X $88 $99 $01 $86

.step .pintest 1 0 $8037 $96 1 0 $8037 %1XXX010X $88 $99 $01 $96
.step .pintest 0 1 $8037 $41 1 0 $8037 %1XXX010X $88 $99 $01 $96

.step .pintest 1 0 $0041 $41 1 0 $8038 %1XXX010X $88 $99 $01 $96
.step .pintest 0 1 $0041 $99 1 0 $8038 %1XXX010X $88 $99 $01 $96

.step .pintest 1 0 $0042 $99 0 0 $8038 %1XXX010X $88 $99 $01 $96
.step .pintest 0 1 $0042 $99 0 0 $8038 %1XXX010X $88 $99 $01 $96

.info Copy $AA from $0022 to $0043 + $0044
.info LDX #$01

.step .pintest 1 0 $8038 $99 1 1 $8038 %1XXX010X $88 $99 $01 $96
.step .pintest 0 1 $8038 $A2 1 1 $8038 %1XXX010X $88 $99 $01 $96

.step .pintest 1 0 $8039 $A2 1 0 $8039 %1XXX010X $88 $99 $01 $A2
.step .pintest 0 1 $8039 $01 1 0 $8039 %1XXX010X $88 $99 $01 $A2

.info LDY $21,X
.step .pintest 1 0 $803A $01 1 1 $803A %0XXX010X $88 $01 $01 $A2
.step .pintest 0 1 $803A $B4 1 1 $803A %0XXX010X $88 $01 $01 $A2

.step .pintest 1 0 $803B $B4 1 0 $803B %0XXX010X $88 $01 $01 $B4
.step .pintest 0 1 $803B $21 1 0 $803B %0XXX010X $88 $01 $01 $B4

.step .pintest 1 0 $0021 $21 1 0 $803C %0XXX010X $88 $01 $01 $B4
.step .pintest 0 1 $0021 $99 1 0 $803C %0XXX010X $88 $01 $01 $B4

.step .pintest 1 0 $0022 $99 1 0 $803C %0XXX010X $88 $01 $01 $B4
.step .pintest 0 1 $0022 $AA 1 0 $803C %0XXX010X $88 $01 $01 $B4

.info STY $43
.step .pintest 1 0 $803C $AA 1 1 $803C %1XXX010X $88 $01 $AA $B4
.step .pintest 0 1 $803C $84 1 1 $803C %1XXX010X $88 $01 $AA $B4

.step .pintest 1 0 $803D $84 1 0 $803D %1XXX010X $88 $01 $AA $84
.step .pintest 0 1 $803D $43 1 0 $803D %1XXX010X $88 $01 $AA $84

.step .pintest 1 0 $0043 $43 0 0 $803E %1XXX010X $88 $01 $AA $84
.step .pintest 0 1 $0043 $AA 0 0 $803E %1XXX010X $88 $01 $AA $84

.info STY $43,X
.step .pintest 1 0 $803E $43 1 1 $803E %1XXX010X $88 $01 $AA $84
.step .pintest 0 1 $803E $94 1 1 $803E %1XXX010X $88 $01 $AA $84

.step .pintest 1 0 $803F $94 1 0 $803F %1XXX010X $88 $01 $AA $94
.step .pintest 0 1 $803F $43 1 0 $803F %1XXX010X $88 $01 $AA $94

.step .pintest 1 0 $0043 $43 1 0 $8040 %1XXX010X $88 $01 $AA $94
.step .pintest 0 1 $0043 $AA 1 0 $8040 %1XXX010X $88 $01 $AA $94

.step .pintest 1 0 $0044 $AA 0 0 $8040 %1XXX010X $88 $01 $AA $94
.step .pintest 0 1 $0044 $AA 0 0 $8040 %1XXX010X $88 $01 $AA $94

.memtest $0040
    $88 $99 $99 $AA $AA

.memtest $0400
    $11 $22 $33 $44 $55
