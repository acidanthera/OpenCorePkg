EFI_STATUS
[[Prefix]]ConfigurationInit (
  OUT [[PREFIX]]_GLOBAL_CONFIG   *Config,
  IN  VOID               *Buffer,
  IN  UINT32             Size
  )
{
  BOOLEAN  Success;

  [[PREFIX]]_GLOBAL_CONFIG_CONSTRUCT (Config, sizeof (*Config));
  Success = ParseSerialized (Config, &mRootConfigurationInfo, Buffer, Size);

  if (!Success) {
    [[PREFIX]]_GLOBAL_CONFIG_DESTRUCT (Config, sizeof (*Config));
    return EFI_UNSUPPORTED;
  }

  return EFI_SUCCESS;
}

VOID
[[Prefix]]ConfigurationFree (
  IN OUT [[PREFIX]]_GLOBAL_CONFIG   *Config
  )
{
  [[PREFIX]]_GLOBAL_CONFIG_DESTRUCT (Config, sizeof (*Config));
}
