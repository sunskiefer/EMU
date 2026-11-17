## What and why
Adds five Force/MPC OS ports of lemus-dev's emulated machines, all from https://github.com/sunskiefer/EMU (AGPL-3.0, core by lemus-dev, ported by L'Cronx): 303MPC, SHMPC, 808MPC, 909MPC and QVRBGT MPC. No ROMs are shipped; each plugin finds the user's own ROM by SHA-1 in its `roms` folder.

## Testing status
Offline only (2026-11-17). Nothing has been run on a Force or any MPC. 808MPC and 909MPC are circuit solvers whose CPU cost on ARMv7 is unmeasured and may be too heavy. QVRBGT MPC uses placeholder panel art. All five entries are `"tier": "experimental"`.

## Catalog check
`catalog_check.py --catalog` on each release zip: OK, 2 warnings each (install.sh/uninstall.sh differ from the template: the L'Cronx apostrophe is shell-escaped by release.py, same as merged PR #520). Zip SHA-256 values: see SHA256SUMS.txt. `catalog_build.py --check-registry`: to run on the PR branch. `sh -n` passes on every install.sh and uninstall.sh.

## Listing status
Listed (experimental cap). No tested.json yet, so not Verified.

## Checks
- [x] catalog_check.py --catalog on each zip
- [x] sh -n on every install.sh / uninstall.sh
- [ ] catalog_build.py --check-registry
- [ ] Tested on a device
