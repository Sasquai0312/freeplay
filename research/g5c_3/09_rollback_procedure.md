# Rollback

A firmware update replaces the application image. It does not put back pattern data Genesis already wrote. Take a fresh copy of the card before installing Alpha 1. An older backup does not contain work done after that backup.

## Back to hardware-validated Genesis v0.1

Use this when the device still boots and the firmware menu opens.

The preserved file is `PlayPlus_FreePlay_Genesis_v0_1.ppf`, 4,236,752 bytes, SHA-256 `5dc172f7262d7bda29d9b250ddc09bd70265dd96321e2c35cc8eef34856c4f26`.

1. Card in the computer.
2. Check that hash with `Get-FileHash` (see `07_owner_installation.md`).
3. Copy it onto the card as `Firmware\PlayPlus_FreePlay_01.ppf`, replacing the Alpha 1 file of that name.
4. Leave `PlayPlus_v1.3.0.ppf` in `Firmware` as well.
5. Eject, insert, power on.
6. Settings → Firmware → Update Firmware. Pick `PlayPlus_FreePlay_01.ppf`. Confirm. Wait for the restart. Do not pull power.
7. Menu should still show Genesis, LFO, Matrix, and Slice. The version line returns to the v0.1 string (`1.3.0-FreePlay0.1b`).

Then reload the project backup if the disposable project looks wrong.

## Official stock, if the device will not boot

This is the procedure in the repository README, “Going back to the official firmware.” It is not a guarantee. It depends on the bootloader, which this candidate does not modify. A power loss during the update screen can still leave the unit unable to start.

1. Card on the computer.
2. In `Firmware`, leave only `PlayPlus_v1.3.0.ppf`. Move every other `.ppf` off the card.
3. Confirm that file’s SHA-256 is `535a903dffea93b9e624c7d0f613823812f7f7b1df74c91e93e3e93e8de94876` and its size is 4,098,017 bytes.
4. Eject, put the card back, press the small reset button on the back of the Play+. The bootloader installs the single file it finds.
5. When the unit is up, the saved v0.1 file can be copied back as `PlayPlus_FreePlay_01.ppf` and installed from the menu.

Do not leave several `.ppf` files in `Firmware` and then press reset. The Play+ manual text says a reset may install an arbitrary `.ppf` when more than one is present.

The original Play USB tool is not part of this Play+ procedure.
