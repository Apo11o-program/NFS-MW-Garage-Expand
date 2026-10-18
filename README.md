# NFS MW Garage Expand

**A garage expansion mod for Need for Speed: Most Wanted (2005).** Raise the
career garage limit from the vanilla 25-car ceiling to a configurable maximum
of 200 cars with the `NFSMWExtraOptions.asi` plugin.

If you are looking for an **NFS Most Wanted 2005 garage limit mod**, an
**NFSMW 200-car garage**, or a way to **increase the number of cars you can
own in NFS MW**, this project provides a plug-and-play package and the
corresponding source code.

> **Experimental:** The 26th-car case has been confirmed. The full 200-car
> capacity has not yet been tested end-to-end. Back up your career save before
> using this mod.

## Download and install

1. Download the [plug-and-play ZIP from the experimental release](https://github.com/Apo11o-program/NFS-MW-Garage-Expand/releases/tag/v1.0.0).
2. Back up your career save and existing `Scripts\NFSMWExtraOptionsSettings.ini`.
3. Extract the ZIP into the game folder that contains `speed.exe`.
4. Allow the files in `Scripts` to be added or replaced, then start the game.

### Requirements

- Need for Speed: Most Wanted (2005), **version 1.3 English**.
- An ASI Loader that loads `.asi` plugins from the game's `Scripts` folder.
- The game and plugin must use the same compatible installation.

The package sets `PurchasedCarLimit = 200`. If you already use Extra Options
and want to preserve your existing settings, keep your INI and change only
`PurchasedCarLimit` to `200` in its `[Gameplay]` section.

## How it works

The vanilla game has a 25-car career limit and fixed-size customization and
career-record tables. This plugin expansion redirects the relevant lookups and
allocators to additional records, and stores those records in a separate
per-profile `.garage` sidecar beside `NFSMWExtraOptions.asi`. The original
game-save format is not intentionally rewritten for expanded records.

The configured maximum is **200 cars**. That is the size of the game's native
car table and is a practical ceiling for this implementation, not an unlimited
garage.

Do not share or copy another player's `.garage` file: it contains
profile-specific data. Keep the sidecar with the plugin for the same profile
and installation.

## Build from source

Open `NFSMWExtraOptions\NFSMWExtraOptions.sln` in Visual Studio and build
**Release | Win32**. The source tree includes the GPL-3.0 license and the
complete Extra Options source used to build the plugin.

The main expansion implementation is
[`GarageExpansion.h`](./NFSMWExtraOptions/GarageExpansion.h); its integration
and the default 200-car setting are in `ExtraOptionsStuff.h` and
`NFSMWExtraOptionsSettings.ini`.

## Compatibility and safety

- Designed for the English 1.3 `speed.exe`; other executable versions are not
  supported by the underlying Extra Options plugin.
- Back up your save before testing. Never remove the `.garage` sidecar while
  the profile has cars using expanded slots.
- If the sidecar cannot be read or written, the plugin reports an error. Do not
  continue using a profile with expanded cars until its sidecar is restored.
- This mod is an independent community project and is not affiliated with or
  endorsed by Electronic Arts or the Need for Speed team.

## Search terms

NFS Most Wanted garage limit mod · NFSMW 200 car garage · NFS MW 2005 garage
expansion · increase garage size · remove 25 car limit · Need for Speed Most
Wanted car capacity mod

## License and credits

This project modifies the GPL-3.0-licensed
[NFSMW Extra Options](https://github.com/ExOptsTeam/NFSMWExOpts) source by the
ExOpts Team. The license and corresponding source are included in this
repository. See [`LICENSE`](./LICENSE). This community modification is not an
official ExOpts Team release.
