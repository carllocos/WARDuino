unsigned char wasm[] = {
  0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x12, 0x04, 0x60,
  0x02, 0x7f, 0x7f, 0x00, 0x60, 0x02, 0x7f, 0x7f, 0x00, 0x60, 0x01, 0x7f,
  0x00, 0x60, 0x00, 0x00, 0x02, 0x3f, 0x03, 0x03, 0x65, 0x6e, 0x76, 0x0d,
  0x63, 0x68, 0x69, 0x70, 0x5f, 0x70, 0x69, 0x6e, 0x5f, 0x6d, 0x6f, 0x64,
  0x65, 0x00, 0x00, 0x03, 0x65, 0x6e, 0x76, 0x12, 0x63, 0x68, 0x69, 0x70,
  0x5f, 0x64, 0x69, 0x67, 0x69, 0x74, 0x61, 0x6c, 0x5f, 0x77, 0x72, 0x69,
  0x74, 0x65, 0x00, 0x01, 0x03, 0x65, 0x6e, 0x76, 0x0a, 0x63, 0x68, 0x69,
  0x70, 0x5f, 0x64, 0x65, 0x6c, 0x61, 0x79, 0x00, 0x02, 0x03, 0x03, 0x02,
  0x03, 0x03, 0x07, 0x08, 0x01, 0x04, 0x6d, 0x61, 0x69, 0x6e, 0x00, 0x04,
  0x0a, 0x27, 0x02, 0x07, 0x00, 0x41, 0xf4, 0x03, 0x10, 0x02, 0x0b, 0x1d,
  0x00, 0x41, 0x1a, 0x41, 0x02, 0x10, 0x00, 0x03, 0x40, 0x41, 0x1a, 0x41,
  0x01, 0x10, 0x01, 0x10, 0x03, 0x41, 0x1a, 0x41, 0x00, 0x10, 0x01, 0x10,
  0x03, 0x0c, 0x00, 0x0b, 0x0b
};
unsigned int wasm_len = 149;
/*
0000000: 0061 736d                                 ; WASM_BINARY_MAGIC
0000004: 0100 0000                                 ; WASM_BINARY_VERSION
; section "Type" (1)
0000008: 01                                        ; section code
0000009: 00                                        ; section size (guess)
000000a: 04                                        ; num types
; type 0
000000b: 60                                        ; func
000000c: 02                                        ; num params
000000d: 7f                                        ; i32
000000e: 7f                                        ; i32
000000f: 00                                        ; num results
; type 1
0000010: 60                                        ; func
0000011: 02                                        ; num params
0000012: 7f                                        ; i32
0000013: 7f                                        ; i32
0000014: 00                                        ; num results
; type 2
0000015: 60                                        ; func
0000016: 01                                        ; num params
0000017: 7f                                        ; i32
0000018: 00                                        ; num results
; type 3
0000019: 60                                        ; func
000001a: 00                                        ; num params
000001b: 00                                        ; num results
0000009: 12                                        ; FIXUP section size
; section "Import" (2)
000001c: 02                                        ; section code
000001d: 00                                        ; section size (guess)
000001e: 03                                        ; num imports
; import header 0
000001f: 03                                        ; string length
0000020: 656e 76                                  env  ; import module name
0000023: 0d                                        ; string length
0000024: 6368 6970 5f70 696e 5f6d 6f64 65         chip_pin_mode  ; import field name
0000031: 00                                        ; import kind
0000032: 00                                        ; import signature index
; import header 1
0000033: 03                                        ; string length
0000034: 656e 76                                  env  ; import module name
0000037: 12                                        ; string length
0000038: 6368 6970 5f64 6967 6974 616c 5f77 7269  chip_digital_wri
0000048: 7465                                     te  ; import field name
000004a: 00                                        ; import kind
000004b: 01                                        ; import signature index
; import header 2
000004c: 03                                        ; string length
000004d: 656e 76                                  env  ; import module name
0000050: 0a                                        ; string length
0000051: 6368 6970 5f64 656c 6179                 chip_delay  ; import field name
000005b: 00                                        ; import kind
000005c: 02                                        ; import signature index
000001d: 3f                                        ; FIXUP section size
; section "Function" (3)
000005d: 03                                        ; section code
000005e: 00                                        ; section size (guess)
000005f: 02                                        ; num functions
0000060: 03                                        ; function 0 signature index
0000061: 03                                        ; function 1 signature index
000005e: 03                                        ; FIXUP section size
; section "Export" (7)
0000062: 07                                        ; section code
0000063: 00                                        ; section size (guess)
0000064: 01                                        ; num exports
0000065: 04                                        ; string length
0000066: 6d61 696e                                main  ; export name
000006a: 00                                        ; export kind
000006b: 04                                        ; export func index
0000063: 08                                        ; FIXUP section size
; section "Code" (10)
000006c: 0a                                        ; section code
000006d: 00                                        ; section size (guess)
000006e: 02                                        ; num functions
; function body 0
000006f: 00                                        ; func body size (guess)
0000070: 00                                        ; local decl count
0000071: 41                                        ; i32.const
0000072: f403                                      ; i32 literal
0000074: 10                                        ; call
0000075: 02                                        ; function index
0000076: 0b                                        ; end
000006f: 07                                        ; FIXUP func body size
; function body 1
0000077: 00                                        ; func body size (guess)
0000078: 00                                        ; local decl count
0000079: 41                                        ; i32.const
000007a: 1a                                        ; i32 literal
000007b: 41                                        ; i32.const
000007c: 02                                        ; i32 literal
000007d: 10                                        ; call
000007e: 00                                        ; function index
000007f: 03                                        ; loop
0000080: 40                                        ; void
0000081: 41                                        ; i32.const
0000082: 1a                                        ; i32 literal
0000083: 41                                        ; i32.const
0000084: 01                                        ; i32 literal
0000085: 10                                        ; call
0000086: 01                                        ; function index
0000087: 10                                        ; call
0000088: 03                                        ; function index
0000089: 41                                        ; i32.const
000008a: 1a                                        ; i32 literal
000008b: 41                                        ; i32.const
000008c: 00                                        ; i32 literal
000008d: 10                                        ; call
000008e: 01                                        ; function index
000008f: 10                                        ; call
0000090: 03                                        ; function index
0000091: 0c                                        ; br
0000092: 00                                        ; break depth
0000093: 0b                                        ; end
0000094: 0b                                        ; end
0000077: 1d                                        ; FIXUP func body size
000006d: 27                                        ; FIXUP section size
*/
