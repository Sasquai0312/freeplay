# Owner installation

Play+ only. The file the device lists is `PlayPlus_FreePlay_01.ppf`. The download name `PlayPlus_FreePlay_Genesis_v0_2_Alpha1.ppf` stays on the computer. Do not run it through the public FreePlay installer. That installer requires an upstream FreePlay image.

The unit should already be on FreePlay (Menu shows LFO, Matrix, and Slice). Genesis v0.1 has been installed on this unit before. Alpha 1 replaces that application image.

Take a **new** copy of the card first. The backup from the v0.1 session does not contain projects saved after that day.

Commands use `E:` for the card. Change the letter if Windows assigns another one.

## 1. Check the candidate

```powershell
Get-FileHash -Algorithm SHA256 .\PlayPlus_FreePlay_Genesis_v0_2_Alpha1.ppf
(Get-Item .\PlayPlus_FreePlay_Genesis_v0_2_Alpha1.ppf).Length
```

Required hash:

`39A061780B16832DE063D30A248233405BE40378E2DFFE9833AC15B8BF193228`

Required length: `4271087`.

PowerShell prints the hash without a specific case. Compare the hex digits.

## 2. Check the v0.1 rollback file

```powershell
Get-FileHash -Algorithm SHA256 .\PlayPlus_FreePlay_Genesis_v0_1.ppf
(Get-Item .\PlayPlus_FreePlay_Genesis_v0_1.ppf).Length
```

Hash `5DC172F7262D7BDA29D9B250DDC09BD70265DD96321E2C35CC8EEF34856C4F26`, length `4236752`.

Keep this file off the card, under its long name, until a rollback.

## 3. Back up the card and the current projects

```powershell
$stamp = Get-Date -Format 'yyyyMMdd-HHmm'
$dest = "D:\PlayPlus-backup-$stamp"
New-Item -ItemType Directory -Path $dest | Out-Null
Copy-Item -Path 'E:\*' -Destination $dest -Recurse -Force
Copy-Item 'E:\Firmware\PlayPlus_FreePlay_01.ppf' "$dest\PlayPlus_FreePlay_01-before-alpha1.ppf"
```

`D:` is an example disk on the computer. Use a disk that is not the card. Confirm `Projects` is inside `$dest` before going on.

## 4 and 5. Installation copy on the card

```powershell
Copy-Item .\PlayPlus_FreePlay_Genesis_v0_2_Alpha1.ppf 'E:\Firmware\PlayPlus_FreePlay_01.ppf' -Force
```

Leave the official `E:\Firmware\PlayPlus_v1.3.0.ppf` where it is.

## 6. Check the copy

```powershell
Get-FileHash -Algorithm SHA256 'E:\Firmware\PlayPlus_FreePlay_01.ppf'
(Get-Item 'E:\Firmware\PlayPlus_FreePlay_01.ppf').Length
```

Same hash and length as step 1.

## 7. Eject

```powershell
$shell = New-Object -ComObject Shell.Application
$shell.Namespace(17).ParseName('E:\').InvokeVerb('Eject')
```

If the letter is still there afterward, use the taskbar **Safely Remove Hardware and Eject Media** and wait until Windows says the card can be removed.

## 8. Install

1. Insert the card and power on.
2. Settings → Firmware → Update Firmware. On some Play+ menus the same screen is Main Menu → System Settings → Firmware → Update Firmware.
3. Pick `PlayPlus_FreePlay_01.ppf` and confirm.
4. The screen says the firmware is updating and not to power off. That is about 30 seconds. Leave the power and the card alone until the unit restarts itself.

## 9. Confirm

Settings → Firmware should show `1.3.0-FreePlay0.2d`.

Menu should list Genesis together with LFO and Matrix. Samples should still list Slice.

Open a **new empty project** for the checklist in `08_owner_hardware_checklist.md`. Do not open a project you mean to keep.

## 10. Rollback

If the install does not finish, the unit does not boot, or a stop condition in the checklist hits, follow `09_rollback_procedure.md`. Returning to v0.1 is the menu update with the preserved v0.1 file renamed to `PlayPlus_FreePlay_01.ppf`. Stock recovery is the single-file reset procedure. Neither one restores pattern edits, and neither one is a guarantee if the bootloader itself is damaged.
