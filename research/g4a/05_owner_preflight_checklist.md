# 05 — Owner Preflight Checklist

Do not flash until every mandatory line is done. This phase does not ask you to flash yet; G4B is the test session.

## Mandatory

1. Unit is a Polyend **Play+** already running FreePlay (Menu shows LFO and Matrix; Samples shows Slice).
2. Copy the **entire SD card** to the computer. Projects are in `Projects`.
3. Save the official file `PlayPlus_v1.3.0.ppf`. Check SHA-256  
   `535a903dffea93b9e624c7d0f613823812f7f7b1df74c91e93e3e93e8de94876`  
   Size 4098017.
4. Copy the FreePlay file that is on the card **now** to the computer under a name you will not confuse with the new build (the device name is always `PlayPlus_FreePlay_01.ppf`).
5. Candidate file SHA-256  
   `5dc172f7262d7bda29d9b250ddc09bd70265dd96321e2c35cc8eef34856c4f26`  
   Size 4236752. On the card it must be named `PlayPlus_FreePlay_01.ppf`. Do not use the public FreePlay installer for this file.
6. Leave `PlayPlus_v1.3.0.ppf` in the card `Firmware` folder as well.
7. Make a **new empty project** for the first Genesis test. Do not open a project you care about.

## Install (only when you start G4B)

Documented path (upstream FreePlay README; Polyend Play+ downloads page):

1. Eject the card safely, insert it, power on.
2. Settings → Firmware → Update Firmware (Polyend page: Main Menu → System Settings → Firmware → Update Firmware).
3. Select `PlayPlus_FreePlay_01.ppf`, confirm Yes.
4. Wait for the update screen (~30 seconds). Do not power off or remove the card. The device restarts itself.
5. Open Menu. **Genesis** should be listed with LFO and Matrix. The version line may still say FreePlay 0.1b.

## If it will not boot

Card on the computer. In `Firmware`, leave **only** `PlayPlus_v1.3.0.ppf`. Eject, reinsert, press the rear reset button. Then you can put your saved previous FreePlay file back and update from the menu if you want the old FreePlay.

## Report back

Boot yes/no, Genesis visible yes/no, any error text, and whether you had to use reset.
