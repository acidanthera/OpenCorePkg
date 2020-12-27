/**
  Initialize configuration with plist data.

  @param[out]  Config   Configuration structure.
  @param[in]   Buffer   Configuration buffer in plist format.
  @param[in]   Size     Configuration buffer size.

  @retval  EFI_SUCCESS on success
**/
EFI_STATUS
[[Prefix]]ConfigurationInit (
  OUT [[PREFIX]]_GLOBAL_CONFIG   *Config,
  IN  VOID               *Buffer,
  IN  UINT32             Size
  );

/**
  Free configuration structure.

  @param[in,out]  Config   Configuration structure.
**/
VOID
[[Prefix]]ConfigurationFree (
  IN OUT [[PREFIX]]_GLOBAL_CONFIG   *Config
  );

#endif // [[PREFIX]]_CONFIGURATION_LIB_H
