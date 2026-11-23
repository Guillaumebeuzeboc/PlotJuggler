# PlotJuggler 4 snap

Classic snap based on **core26**. The build scripts currently target **amd64**;
the manifest's arm64 entry still needs architecture-specific Qt and library paths.
Classic distribution requires Snap Store approval and does not support Ubuntu Core.

## Build

Requires snapd, initialized LXD, Bash, Git and rsync. Use Snapcraft with bundled
**patchelf 0.19.3 or newer** to avoid corrupting Qt plugin metadata
([snapcraft#6410](https://github.com/canonical/snapcraft/issues/6410)).
Snapcraft `9.2.1.post51` on `latest/edge` includes the fix.

```bash
sudo snap install snapcraft --classic --channel=latest/edge
sudo snap install lxd
lxd init
/snap/snapcraft/current/bin/patchelf --version

packaging/snap/build.sh
```

For an existing Snapcraft installation, use `sudo snap refresh snapcraft --edge`.
Run the wrapper from the repository root. Artifacts go to `packaging/snap/dist/`.

```bash
packaging/snap/build.sh --clean
packaging/snap/build.sh --context /path/outside/checkout
```

The default context is `~/.cache/plotjuggler-snap`. The wrapper copies the working
tree, including `.git` and local edits, excluding `build/`, `packaging/snap/dist/`
and `__pycache__/`. Existing context files are retained, with a warning.
`--clean` cleans the Snapcraft part, not the context. Use a new context for a
fresh-source check, and do not build concurrently in the same context.

## Packaging

- `versions.env` supplies app/Qt versions; `conan.lock` pins dependency recipes.
- Upstream `scripts/install_qt6.sh` installs Qt. Upstream `build.sh` builds with
  tests/demos/debug info disabled and `PJ_BUILD_RASTER_HELPER=ON`.
- `local/build.sh` installs the app, admission helper, Python standard library,
  Qt shared libraries/plugins, icon and raster payload.
- `local/runtime_deploy.py` deploys Conan shared libraries and available license
  notices. Dependency executables and development files are not deployed.
- `extensions` downloads the 20 upstream-curated extensions from registry commit
  `a2243ee41aee5a4bc5e7c6d9a042077a21ed8f20`, verifies SHA256 checksums, manifest
  identities and app/SDK version floors, and bundles them under
  `usr/lib/plotjuggler/plugins/` without ELF
  patching. Update the part's `source-commit` to refresh versions; the selected
  IDs are in `local/install_extensions.py`. All selected IDs have amd64/arm64
  artifacts in this snapshot.
- `snap-assets` installs the launcher and desktop entry. `enable-patchelf`
  handles ELF relocation; `gpu-2604` removes host-provided graphics libraries.
- `stage-packages` supplies desktop/XCB dependencies, GTK3, CUPS and available Qt
  SQL clients. APT resolves their dependencies; core26 supplies the base runtime.

The complete Qt plugin tree is included. Its optional Oracle, Mimer and MySQL
drivers require `libclntsh.so.23.1`, `libmimerapi.so` and `libmysqlclient.so.21`,
which are not supplied by Resolute's repositories. Those backends need compatible
host libraries; shipping their plugin files does not make them usable by default.

The raster helper and APT-provided shareware WAD are installed together under
`usr/bin/3rdparty/retro/`, with the GPL text, source offer and data license.

**Release blocker:** the downloaded Qt kit has no `licenses/` directory. Qt's
applicable license texts and third-party notices still need to be sourced and
included. Conan license copying does not cover Qt or establish full compliance.

## Install and use

```bash
sudo snap install --dangerous --classic packaging/snap/dist/plotjuggler_*_amd64.snap
plotjuggler --version
plotjuggler --selftest-python
plotjuggler /path/to/data
```

On startup, PJ4 seeds bundled extensions into the current user's writable store.
Compatible newer marketplace versions are preserved. No install hook is needed.
The bundle includes file loaders, streamers, message parsers and tools, including
the ROS 2 subscriber. Install more through **File → Marketplace** or use
`plotjuggler --plugin-dir /path/to/extensions`. PJ3 plugins are incompatible.
For ROS, source the host installation and workspace before launching:

```bash
source /opt/ros/lyrical/setup.bash
source ~/my_workspace/install/setup.bash
plotjuggler
```

No interface connections are required. The launcher preserves sourced ROS paths
while preferring bundled libraries. Graphics drivers come from the host.

## Verify before release

- Build from a fresh context to expose stale libraries or deleted files.
- Run `--help`, `--version`, `--selftest-python` and the raster helper.
- Check first-launch extension seeding, restart persistence and preservation of
  a newer marketplace version; load a file with a bundled loader.
- Exercise ROS with a real publisher and custom-message workspace.
- Check X11/Wayland rendering; fixtures are under `packaging/smoke/`.
- Review ELF dependencies and lint findings. GPU cleanup deliberately removes
  some dependencies for host resolution; other missing libraries need fixing.
- Validate hardware graphics and complete license/source-distribution checks.

The stage-package audit covered Qt/Conan ELF dependencies and Resolute package
metadata. The revised list still needs a fresh snap build and runtime testing.
For diagnosis, use `QT_DEBUG_PLUGINS=1`, `LD_DEBUG=libs` or
`snap run --shell plotjuggler`.
