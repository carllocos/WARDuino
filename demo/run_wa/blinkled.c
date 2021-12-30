unsigned char wasm[] = {
  0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x0d, 0x03, 0x60,
  0x02, 0x7f, 0x7f, 0x00, 0x60, 0x01, 0x7f, 0x00, 0x60, 0x00, 0x00, 0x02,
  0x3f, 0x03, 0x03, 0x65, 0x6e, 0x76, 0x0d, 0x63, 0x68, 0x69, 0x70, 0x5f,
  0x70, 0x69, 0x6e, 0x5f, 0x6d, 0x6f, 0x64, 0x65, 0x00, 0x00, 0x03, 0x65,
  0x6e, 0x76, 0x12, 0x63, 0x68, 0x69, 0x70, 0x5f, 0x64, 0x69, 0x67, 0x69,
  0x74, 0x61, 0x6c, 0x5f, 0x77, 0x72, 0x69, 0x74, 0x65, 0x00, 0x00, 0x03,
  0x65, 0x6e, 0x76, 0x0a, 0x63, 0x68, 0x69, 0x70, 0x5f, 0x64, 0x65, 0x6c,
  0x61, 0x79, 0x00, 0x01, 0x03, 0x05, 0x04, 0x02, 0x02, 0x02, 0x02, 0x07,
  0x08, 0x01, 0x04, 0x6d, 0x61, 0x69, 0x6e, 0x00, 0x06, 0x0a, 0x31, 0x04,
  0x07, 0x00, 0x41, 0xf4, 0x03, 0x10, 0x02, 0x0b, 0x08, 0x00, 0x41, 0x1a,
  0x41, 0x01, 0x10, 0x01, 0x0b, 0x08, 0x00, 0x41, 0x1a, 0x41, 0x00, 0x10,
  0x01, 0x0b, 0x15, 0x00, 0x41, 0x1a, 0x41, 0x02, 0x10, 0x00, 0x03, 0x40,
  0x10, 0x04, 0x10, 0x03, 0x10, 0x05, 0x10, 0x03, 0x0c, 0x00, 0x0b, 0x0b
};
unsigned int wasm_len = 156;
/*
0000000: 0061 736d                                 ; WASM_BINARY_MAGIC
0000004: 0100 0000                                 ; WASM_BINARY_VERSION
; section "Type" (1)
0000008: 01                                        ; section code
0000009: 00                                        ; section size (guess)
000000a: 03                                        ; num types
; type 0
000000b: 60                                        ; func
000000c: 02                                        ; num params
000000d: 7f                                        ; i32
000000e: 7f                                        ; i32
000000f: 00                                        ; num results
; type 1
0000010: 60                                        ; func
0000011: 01                                        ; num params
0000012: 7f                                        ; i32
0000013: 00                                        ; num results
; type 2
0000014: 60                                        ; func
0000015: 00                                        ; num params
0000016: 00                                        ; num results
0000009: 0d                                        ; FIXUP section size
; section "Import" (2)
0000017: 02                                        ; section code
0000018: 00                                        ; section size (guess)
0000019: 03                                        ; num imports
; import header 0
000001a: 03                                        ; string length
000001b: 656e 76                                  env  ; import module name
000001e: 0d                                        ; string length
000001f: 6368 6970 5f70 696e 5f6d 6f64 65         chip_pin_mode  ; import field name
000002c: 00                                        ; import kind
000002d: 00                                        ; import signature index
; import header 1
000002e: 03                                        ; string length
000002f: 656e 76                                  env  ; import module name
0000032: 12                                        ; string length
0000033: 6368 6970 5f64 6967 6974 616c 5f77 7269  chip_digital_wri
0000043: 7465                                     te  ; import field name
0000045: 00                                        ; import kind
0000046: 00                                        ; import signature index
; import header 2
0000047: 03                                        ; string length
0000048: 656e 76                                  env  ; import module name
000004b: 0a                                        ; string length
000004c: 6368 6970 5f64 656c 6179                 chip_delay  ; import field name
0000056: 00                                        ; import kind
0000057: 01                                        ; import signature index
0000018: 3f                                        ; FIXUP section size
; section "Function" (3)
0000058: 03                                        ; section code
0000059: 00                                        ; section size (guess)
000005a: 04                                        ; num functions
000005b: 02                                        ; function 0 signature index
000005c: 02                                        ; function 1 signature index
000005d: 02                                        ; function 2 signature index
000005e: 02                                        ; function 3 signature index
0000059: 05                                        ; FIXUP section size
; section "Export" (7)
000005f: 07                                        ; section code
0000060: 00                                        ; section size (guess)
0000061: 01                                        ; num exports
0000062: 04                                        ; string length
0000063: 6d61 696e                                main  ; export name
0000067: 00                                        ; export kind
0000068: 06                                        ; export func index
0000060: 08                                        ; FIXUP section size
; section "Code" (10)
0000069: 0a                                        ; section code
000006a: 00                                        ; section size (guess)
000006b: 04                                        ; num functions
; function body 0
000006c: 00                                        ; func body size (guess)
000006d: 00                                        ; local decl count
000006e: 41                                        ; i32.const
000006f: f403                                      ; i32 literal
0000071: 10                                        ; call
0000072: 02                                        ; function index
0000073: 0b                                        ; end
000006c: 07                                        ; FIXUP func body size
; function body 1
0000074: 00                                        ; func body size (guess)
0000075: 00                                        ; local decl count
0000076: 41                                        ; i32.const
0000077: 1a                                        ; i32 literal
0000078: 41                                        ; i32.const
0000079: 01                                        ; i32 literal
000007a: 10                                        ; call
000007b: 01                                        ; function index
000007c: 0b                                        ; end
0000074: 08                                        ; FIXUP func body size
; function body 2
000007d: 00                                        ; func body size (guess)
000007e: 00                                        ; local decl count
000007f: 41                                        ; i32.const
0000080: 1a                                        ; i32 literal
0000081: 41                                        ; i32.const
0000082: 00                                        ; i32 literal
0000083: 10                                        ; call
0000084: 01                                        ; function index
0000085: 0b                                        ; end
000007d: 08                                        ; FIXUP func body size
; function body 3
0000086: 00                                        ; func body size (guess)
0000087: 00                                        ; local decl count
0000088: 41                                        ; i32.const
0000089: 1a                                        ; i32 literal
000008a: 41                                        ; i32.const
000008b: 02                                        ; i32 literal
000008c: 10                                        ; call
000008d: 00                                        ; function index
000008e: 03                                        ; loop
000008f: 40                                        ; void
0000090: 10                                        ; call
0000091: 04                                        ; function index
0000092: 10                                        ; call
0000093: 03                                        ; function index
0000094: 10                                        ; call
0000095: 05                                        ; function index
0000096: 10                                        ; call
0000097: 03                                        ; function index
0000098: 0c                                        ; br
0000099: 00                                        ; break depth
000009a: 0b                                        ; end
000009b: 0b                                        ; end
0000086: 15                                        ; FIXUP func body size
000006a: 31                                        ; FIXUP section size
*/
