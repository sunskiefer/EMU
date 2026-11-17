Catalog prep for the five EMU plugins (nothing here is published).
- plugins/*.json: registry entries for open-mpc/mpc-vst-plugins catalog/plugins/
- release_apostrophe.patch: the release.py change that shell-escapes the L'Cronx apostrophe (sh -n failed without it)
- ../builds/*.zip: the packaged 0.1.0 release zips; SHA256SUMS.txt
- PR_TEXT.md: the PR body
Still to do, blocked in the Claude Code session: publish a GitHub release per plugin (tags 303mpc-v0.1.0, shmpc-v0.1.0, 808mpc-v0.1.0, 909mpc-v0.1.0, qvrbgtmpc-v0.1.0, pre-release) attaching ../builds zips, then open the PR.
