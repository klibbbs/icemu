.info Basic store and load
.pins clk1.pin clk2.pin ab.pin db.pin rw.pin sync.pin pc.reg p.reg a.reg i.reg
.memset $0200
    $77
.memset $8000
    $A9 $66     ! LDA #$66
    $8D $00 $03 ! STA $0300
    $AD $00 $02 ! LDA $0200
    $8D $01 $03 ! STA $0301
.memset $FFFC
    $00 $80     ! RESET => $8000
.reset
.info RESET sequence
.run 6
.step .pintest 1 0 $FFFC $00 1 0 $XXXX %XXXX01XX $XX $00
.step .pintest 0 1 $FFFC $00 1 0 $XXXX %XXXX01XX $XX $00

.step .pintest 1 0 $FFFD $00 1 0 $XXXX %XXXX01XX $XX $00
.step .pintest 0 1 $FFFD $80 1 0 $XXXX %XXXX01XX $XX $00

.info LDA #$66
.step .pintest 1 0 $8000 $80 1 1 $8000 %XXXX01XX $XX $00
.step .pintest 0 1 $8000 $A9 1 1 $8000 %XXXX01XX $XX $00

.step .pintest 1 0 $8001 $A9 1 0 $8001 %XXXX01XX $XX $A9
.step .pintest 0 1 $8001 $66 1 0 $8001 %XXXX01XX $XX $A9

.info STA $0300
.step .pintest 1 0 $8002 $66 1 1 $8002 %0XXX010X $66 $A9
.step .pintest 0 1 $8002 $8D 1 1 $8002 %0XXX010X $66 $A9

.step .pintest 1 0 $8003 $8D 1 0 $8003 %0XXX010X $66 $8D
.step .pintest 0 1 $8003 $00 1 0 $8003 %0XXX010X $66 $8D

.step .pintest 1 0 $8004 $00 1 0 $8004 %0XXX010X $66 $8D
.step .pintest 0 1 $8004 $03 1 0 $8004 %0XXX010X $66 $8D

.step .pintest 1 0 $0300 $03 0 0 $8005 %0XXX010X $66 $8D
.step .pintest 0 1 $0300 $66 0 0 $8005 %0XXX010X $66 $8D
.memtest $0300 $66

.info LDA $0200
.step .pintest 1 0 $8005 $03 1 1 $8005 %0XXX010X $66 $8D
.step .pintest 0 1 $8005 $AD 1 1 $8005 %0XXX010X $66 $8D

.step .pintest 1 0 $8006 $AD 1 0 $8006 %0XXX010X $66 $AD
.step .pintest 0 1 $8006 $00 1 0 $8006 %0XXX010X $66 $AD

.step .pintest 1 0 $8007 $00 1 0 $8007 %0XXX010X $66 $AD
.step .pintest 0 1 $8007 $02 1 0 $8007 %0XXX010X $66 $AD

.step .pintest 1 0 $0200 $02 1 0 $8008 %0XXX010X $66 $AD
.step .pintest 0 1 $0200 $77 1 0 $8008 %0XXX010X $66 $AD

.info STA $0301
.step .pintest 1 0 $8008 $77 1 1 $8008 %0XXX010X $77 $AD
.step .pintest 0 1 $8008 $8D 1 1 $8008 %0XXX010X $77 $AD

.step .pintest 1 0 $8009 $8D 1 0 $8009 %0XXX010X $77 $8D
.step .pintest 0 1 $8009 $01 1 0 $8009 %0XXX010X $77 $8D

.step .pintest 1 0 $800A $01 1 0 $800A %0XXX010X $77 $8D
.step .pintest 0 1 $800A $03 1 0 $800A %0XXX010X $77 $8D

.step .pintest 1 0 $0301 $03 0 0 $800B %0XXX010X $77 $8D
.step .pintest 0 1 $0301 $77 0 0 $800B %0XXX010X $77 $8D

.memtest $0300
    $66 $77
