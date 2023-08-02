zcc +msx -create-app -subtype=rom  -pragma-define:CRT_ORG_BSS=0xe010  --list -lmsxbios  main.c -o test.rom

rem  bank1.c bank2.c bank3.c bank4.c -o test.rom
