# CLAUDE.md

Wonderland repo: home of the wonderland plugin by simi soprano, built with iPlug2 in the "out of source" layout.

- @iPlug2 - Framework as git submodule (do not modify)
- Wonderland - The plugin project (see @Wonderland/CLAUDE.md)

## Notes

- `common-mac.xcconfig` sets `MACOSX_DEPLOYMENT_TARGET = 12.0`; current Xcode rejects older targets.
- Local standalone APP builds need ad-hoc signing overrides:
  `CODE_SIGN_IDENTITY="-" CODE_SIGN_STYLE=Manual DEVELOPMENT_TEAM="" PROVISIONING_PROFILE_SPECIFIER="" CODE_SIGN_ENTITLEMENTS=""`
- The standalone app can render the UI to a PNG: `~/Applications/Wonderland.app/Contents/MacOS/Wonderland --screenshot out.png`
- CI: `.github/workflows/build-mac.yml`, `build-win.yml` (build + pluginval/clap-validator/auval), `release.yml` (on `v*` tags).

## Additional Context

- iPlug2 framework: @iPlug2/CLAUDE.md
- Project structure: @iPlug2/Documentation/structure.md

## Design rule

Every user-facing string (UI text, param names, units, preset names, metadata) is lowercase.
