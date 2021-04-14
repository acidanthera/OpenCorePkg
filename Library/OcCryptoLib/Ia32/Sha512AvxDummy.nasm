BITS 32

section .text

align 8
global ASM_PFX(TryEnableAvx)
ASM_PFX(TryEnableAvx):
  mov eax, 0
  ret

align 8
global ASM_PFX(Sha512TransformAvx)
ASM_PFX(Sha512TransformAvx):
  ret
