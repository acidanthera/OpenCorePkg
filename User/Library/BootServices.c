#include <BootServices.h>

EFI_BOOT_SERVICES mBootServices =
{
  .RaiseTPL = DummyRaiseTPL
};

EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL mConOut = 
{
  .OutputString = NullTextOutputString
};

EFI_SYSTEM_TABLE mSystemTable =
{
  .BootServices = &mBootServices,
  .ConOut       = &mConOut
};

EFI_SYSTEM_TABLE  *gST  = &mSystemTable;
EFI_BOOT_SERVICES *gBS  = &mBootServices;

EFI_HANDLE gImageHandle = (EFI_HANDLE)(UINTN) 0x12345;

STATIC
EFI_TPL
EFIAPI
DummyRaiseTPL (
  IN EFI_TPL      NewTpl
  )
{
  ASSERT (FALSE);
  return 0;
}

STATIC
EFI_STATUS
EFIAPI
NullTextOutputString (
  IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
  IN CHAR16                          *String
  )
{
  while (*String) {
    if (*String != '\r') {
      printf("%c", (char) *String);
    }

    ++String;
  }
  return EFI_SUCCESS;
}
