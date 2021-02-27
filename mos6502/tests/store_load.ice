.info Basic store and load
.pinset clk1.pin clk2.pin ab.pin db.pin rw.pin sync.pin pc.reg p.reg a.reg i.reg
.memset $0200
    $77
.memset $8000
    $A9 $66     # LDA #$66
    $8D $00 $03 # STA $0300
    $AD $00 $02 # LDA $0200
    $8D $01 $03 # STA $0301
.memset $FFFC
    $00 $80     # RESET => $8000
.reset
.info Start cycle
.run 6
.step
.step
.step
.step
.info Program start
.run 15
.memtest $0300
    $66 $77
