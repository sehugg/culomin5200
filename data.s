; Curse of the Lost Miner for Atari 5200 
; Data


; Character sets for caves
.segment "DLIST"
_CLM_DATA_CHSET1:
.incbin "clmfont1.fnt"
;.segment "CL_CHSET2"
_CLM_DATA_CHSET2:
.incbin "clmfont2.fnt"

; Display list for caves
;.segment "CL_CAV_DL"
_CLM_DATA_DL_CAVE:


.byte 112 ,112 ,112
.byte 068 ,0 ,24
.byte 004 ,004 ,004 ,004 ,004 ,004 ,004 ,004 ,004 ,004 ,004 ,004 ,004 ,004 ,004
.byte 004 ,004 ,004 ,004 ,004 ,004
.byte 240
.byte 066 ,112,27
.byte 128
.byte 065
.byte <_CLM_DATA_DL_CAVE,>_CLM_DATA_DL_CAVE

; Levels
;.segment "CL_CAVES"
_CLM_DATA_CAVES:
.incbin "levels.dat"

;Raster Music Tracker
;.segment "RMT_ROM"
_CLM_RMT_AUX1:
.incbin "rmt_aux1.bin"
_CLM_RMT_AUX2:
.incbin "rmt_aux2.bin"
_CLM_RMT_MAIN:
.incbin "rmt_main.bin"
_CLM_RMT_MUSIC:
.incbin "rmt_music.bin"
_CLM_RMT_MUSIC_END:

; Export symbols to make them visible in the C program
.export _CLM_DATA_CAVES
.export _CLM_DATA_DL_CAVE
.export _CLM_DATA_CHSET1
.export _CLM_DATA_CHSET2
.export _CLM_RMT_AUX1
.export _CLM_RMT_AUX2
.export _CLM_RMT_MAIN
.export _CLM_RMT_MUSIC
.export _CLM_RMT_MUSIC_END

