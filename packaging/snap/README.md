# PlotJuggler 4 classic snap

## Scope and build

This recipe builds **PlotJuggler 4, amd64, core26, classic** from source in
Ubuntu 26.04 LXD. Core26 was explicitly selected during packaging to match the
development host's glibc/graphics stack. Use a Snapcraft build bundling
**patchelf 0.19.3 or later** so Qt 6 plugin metadata survives ELF patching
([snapcraft#6410](https://github.com/canonical/snapcraft/issues/6410)).
Snapcraft **9.2.1.post51**, revision **19569**, on `latest/edge` contains the fix;
the maintainers also plan to include it in Snapcraft 9.3.

Prerequisites on a snapd-enabled Linux host:

```sh
sudo snap install snapcraft --classic --channel=latest/edge
# For an existing installation: sudo snap refresh snapcraft --channel=latest/edge
/snap/snapcraft/current/bin/patchelf --version
sudo snap install lxd
lxd init
# Ensure your user has access to LXD; re-login after group membership changes.
export SNAPCRAFT_BUILD_INFO=1
packaging/snap/build.sh
```

The host wrapper needs Bash, Git, rsync and standard Linux coreutils. It runs
`snapcraft pack --use-lxd`. The verified build target is amd64; no `--build-for`
argument is needed for a native amd64 build. Snapcraft
creates the artifact inside the generated context; the wrapper then moves it to
`packaging/snap/dist/`, which is ignored locally.

### Wrapper CLI and source semantics

```sh
packaging/snap/build.sh                     # current working tree + Git metadata
packaging/snap/build.sh --clean             # clean the cached part, then build
packaging/snap/build.sh --context /path/outside/checkout
```

The wrapper copies the **current working tree**, including `.git`, staged and
unstaged changes, and untracked files. The root `build/`, `packaging/snap/dist/`
and `__pycache__/` directories are excluded. Other ignored files are copied too;
rsync does not apply `.gitignore`. Packaging files also come from the current
working copy. App and Qt versions come from the copied `versions.env`; callers'
`PJ_VERSION` overrides are not used by this wrapper. The wrapper does not resolve
or record a source commit. Git metadata is available in the context for future
versioning changes; the current `adopt-info` still uses `PJ_APP_VERSION`.

The default stable context is
`${HOME}/.cache/plotjuggler-snap`.
It is a build copy of the working tree and its Git metadata. `--context DIR` overrides
this path. Contexts inside the repository are refused. The wrapper creates the
context if needed and warns when it is non-empty, then copies source and packaging
directly into it. Matching files are updated; other existing files, including
files deleted from the working tree, remain. No temporary export
or context-ownership marker is used. `--clean` cleans the cached Snapcraft part
before building; it does not clear the context.
The wrapper prints its context path for inspection.

The part's `override-pull` initializes the docking and doomgeneric submodules after
`craftctl default` copies the local source, including `.git`, into
`CRAFT_PART_SRC`. It uses `git submodule update --init --recursive` for
`3rdparty/Qt-Advanced-Docking` and `3rdparty/doomgeneric`, reading the revisions from the copied Git index
and the URL from `.gitmodules`. Host submodule initialization is not required.
The update does not use `--force`: local edits at the recorded revision remain,
and edits that conflict with checking out that revision cause the pull to fail.
A root `snap/` directory is materialized **only inside the generated context**, never in this
checkout. The old ignored PJ3 `build/` is neither used nor removed.

Build helpers are pinned to Conan **2.34.0**, aqtinstall **3.3.0** and CMake
**3.31.10**. Ubuntu 26.04's `gcc-13`/`g++-13` compile both the application and
Conan dependencies. `conan profile detect --exist-ok` creates the default profile
in the isolated Conan home when it is missing, detecting the architecture and
compiler after `CC`/`CXX` are set. Upstream passes C++20 and `RelWithDebInfo` for
the application dependency graph. `CC`/`CXX` select the compiler executables;
Conan uses its default dependency-build parallelism and source-download settings.
Conan recipes can select their own locked build tools, including CMake 4.4.2.
The apt build list supplies native compiler/build drivers, Qt's imported
OpenGL/EGL/XKB/Vulkan development targets, and FFmpeg's VAAPI/DRM system headers.
Qt runtime-only XCB/Wayland/fontconfig/D-Bus/GLib dependencies are stage packages;
their part-install paths are exposed during the build for Qt tools and ELF
collection. There is no second apt CMake/pip or redundant download/archive CLI
toolchain; aqt and Conan use their Python download/extraction implementations.
The kit is exactly **Qt 6.12.0 linux_gcc_64**, including matching private headers
and platform plugins. The snap invokes the build-tree copy of
`scripts/install_qt6.sh`, which installs into `${CRAFT_PART_BUILD}/.qt`, reuses an
existing installation and retries transient download failures. It uses the
pinned aqtinstall from the tools virtual environment. System Qt is not a substitute.

The snap invokes the build-tree copy of upstream
`build.sh --no-tests --no-demos --debug-info none` to install Conan dependencies,
configure CMake and build all enabled targets, including `pj_app`,
`pj-plugin-check` and `pj-raster-helper`.
It sets `PJ_INSTALLATION=snap`, the application version from `versions.env`, four
build jobs, `PJ_BUILD_RASTER_HELPER=ON` and the Ninja generator. The options
disable tests, demos, the standalone marketplace app and debug information.
Outputs are under `${CRAFT_PART_BUILD}/build`.

Upstream resolves the locked Conan graph through the anonymous
`plotjuggler-conan` and ConanCenter remotes, building missing binaries from source.
After the build, the snap queries the resolved Conan graph for dependency license
collection; that graph stays in the build directory. The snap does not include
the custom build-info directory, Conan profile or build-environment inventories.
The lock pins recipe revisions, not a fully immutable Ubuntu package archive
or every transitive pip version.

`cmake --install` does not install this application completely. `local/build.sh`
manually installs the application, sibling `pj-plugin-check`, Conan's matching
CPython standard library/extensions, the Qt kit's entire runtime plugin tree, the desktop PNG
and dependency license notices. The `snap-assets` dump part installs the launcher,
and desktop entry from `snap/local` using `organize` mappings and a stage
allowlist. `deploy.py` walks the runtime ELF dependencies. Snapcraft's
`enable-patchelf` on the payload-producing part handles
ELF interpreter and library-search-path patching for classic confinement.
Qt plugins are included in the same part and patched by Snapcraft as well.
The Python standard-library directory is selected from Conan's installed runtime
under `lib/python*/`, rather than naming a Python minor version in the packaging.

After changing packaging scripts, refresh the part's copied source:

```sh
packaging/snap/build.sh --clean
```

## Install and use

Install the locally built artifact with classic confinement:

```sh
sudo snap install --dangerous --classic packaging/snap/dist/plotjuggler_4.1.0_amd64.snap
plotjuggler --help
plotjuggler --version
plotjuggler --selftest-python
plotjuggler /path/to/data
plotjuggler --plugin-dir /path/to/compatible/extensions
```

There are **no plugs to connect, snap options, hooks, layouts or daemons**.
Devmode/strict installation and the confinement-validator workflow do not apply
to this explicitly classic package. Classic cannot run on Ubuntu Core. Store
distribution requires approval for classic confinement.

The package is app-only. Obtain compatible loaders/streamers through
**File → Marketplace** or supply external PJ4 extensions. PJ3/Qt5 plugin binaries
are incompatible. The admission helper and SDK/version gates remain mandatory;
classic access does not make arbitrary C++/Qt/glibc ABIs compatible. No unpinned
marketplace snapshot is silently bundled.

The standalone GPLv2 raster helper is installed under
`usr/bin/3rdparty/retro/`, where the application looks for it. `base.wad` comes
from Ubuntu multiverse's `doom-wad-shareware` build package. APT verifies that
package; the snap uses its installed data without a separate content-hash pin.
The helper, data, GPL text, source offer, shareware license and upstream retro
README are shipped together. PlotJuggler launches the helper as a separate
process and does not link it. See `3rdparty/retro/README.md` in the source tree.

For host ROS, source the installed host distribution and custom workspace before
launching, for example on Ubuntu 26.04 with ROS Lyrical:

```sh
source /opt/ros/lyrical/setup.bash
source ~/my_ros_workspace/install/setup.bash
plotjuggler
```

The launcher preserves `ROS_DISTRO`, `AMENT_PREFIX_PATH`, `CMAKE_PREFIX_PATH`,
other ROS state, and nonempty inherited `LD_LIBRARY_PATH` entries. It does not
force a distribution or source a bundled underlay. It restricts Qt plugin paths
to the matching bundled kit, clears `QT_IM_MODULE` to avoid loading older host
Qt input-method plugins, sets relocated `PYTHONHOME`, and preserves arguments and
`QT_QPA_PLATFORM`. The launcher puts bundled libraries and core26's runtime
before sourced library paths, followed by explicit host graphics directories.
These paths also serve the host dependencies excluded by Snapcraft's classic
`NODEFLIB` setting. Compatibility with sourced environments
is covered by the runtime checks below.
Arbitrary processes spawned by extensions still need integration testing because
they inherit environment variables.

Extensions normally persist under
`${XDG_DATA_HOME:-~/.local/share}/PlotJuggler/PlotJuggler4/extensions`, with sibling
staging/backup transaction directories. The application creates these writable
locations; no read-only plugin directory is forced on the command line.

## Graphics and reference evaluation

The [RViz Jazzy recipe](https://github.com/canonical/rviz2_snap/tree/jazzy)
informed classic confinement, `enable-patchelf`, explicit `libxcb-cursor0`, and
deliberate host GL resolution. Its colcon pipeline, Qt5 paths, forced Jazzy
variables and replacement ROS underlays are unsuitable for PJ4's Qt6 application
and host-selected ROS extensions.

The `gpu-2604` part uses Canonical's `gpu-snap` repository pinned to commit
`7c4091ddcea8f969b98a752c48841af9b0a659d8`. After the application is primed, it runs
`gpu-2604-cleanup mesa-2604`. The maintained lists remove graphics libraries,
drivers and supporting X11/XCB, Wayland, DRM and VAAPI files from the final snap.
`deploy.py` preserves distro libraries' multiarch paths so those lists apply;
Qt and Conan libraries remain in the private `usr/lib` directory. Its only
library exclusion is glibc/the ELF loader, supplied by core26.

This is a cleanup-only part: its helper files are excluded from the payload.
It does not install `mesa-2604`, add a GPU content interface or use a GPU
command-chain wrapper. Removed graphics libraries and vendor drivers resolve
from the host, whose multiarch directories are appended after preserved ROS
paths. Remaining Qt/application libraries are bundled. Core26 is intentional:
the development host's Mesa Gallium
library requires **GLIBC_2.43**, which core24's glibc 2.39 cannot satisfy. Core26
removes that known floor mismatch; actual X11/Wayland/hardware rendering still
needs testing, and future or foreign host graphics stacks can have other ABI
requirements. `LIBGL_ALWAYS_SOFTWARE=1` is useful diagnostically but still uses
the host Mesa software driver.

## Acceptance checks

### Validation snapshot (2026-11-18)

The core26/classic/amd64 artifact `plotjuggler_4.1.0_amd64.snap` was built through
`packaging/snap/build.sh`, using Snapcraft's ELF patching for the app, libraries
and Qt plugins, plus the pinned `gpu-2604` cleanup part. Snapcraft
`9.2.1.post51` (revision `19569`) and its bundled patchelf `0.19.3` were verified
inside the LXD builder. The artifact was installed with `--dangerous --classic`
in an Ubuntu 26.04 LXD container. Its SHA256 is
`bc389b9ca41d76dc995116ea1c4fc62fe2e477e01418b166c9564858c3ccf526`.

Passed runtime checks:

- `--version` and `--selftest-python`.
- External CSV Loader 1.0.7 loading and admission through `pj-plugin-check`.
- External ROS 2 Topic Subscriber 1.1.3 loading with ROS Lyrical sourced.
- All 165 shipped ELF files resolve their dependencies against the snap, core26
  and the host with the launcher's library paths. The app and admission helper
  use the core26 ELF interpreter; no build/cache paths remain in ELF RPATHs.
- All 25 shipped Qt plugins were patched by Snapcraft. Their Qt metadata bytes
  match the original kit and remain covered by `PT_NOTE` segments.
- Bundled Python and Qt take precedence over conflicting inherited library paths.
- No shipped paths match the pinned `mesa-2604` cleanup list. Qt, the admission
  helper and private application libraries remain; GPU helper sources are absent.
- The upstream rendering fixture: both plots, the 2D viewer and the 3D view
  rendered through OpenGL on X11/Xvfb and native Wayland/Weston, using Mesa
  llvmpipe. A sourced Lyrical/custom-message workspace was present in the X11 run.

Direct regular-file redirection through `snap run` produced empty logs in this
container, including for a plain shell `echo`. Capture through a pipe when running
the rendering checks; a Bash command wrapper can preserve failure status with
`set -o pipefail; snap run plotjuggler "$@" 2>&1 | cat`.

Snapcraft's library linter reports the graphics/X11/XCB/Wayland/VAAPI dependencies
removed by `gpu-2604`; those resolve from the host in the installed-snap audit.
Unused-library and metadata warnings remain visible. The version-adoption hook
skips setting an already-matching version because local-source refreshes can
rerun `override-pull` within the same lifecycle.

Live ROS custom-message reception, marketplace download/restart persistence,
hardware GPU rendering, arm64 builds and Jazzy integration remain unverified.

### Release acceptance

On a clean Ubuntu 26.04 host/container with snapd and core26:

1. Install the actual `.snap`; run `--help`, `--version`, and
   `QT_QPA_PLATFORM=offscreen plotjuggler --selftest-python`.
2. Audit all shipped ELFs with `readelf -l/-d`, including `pj-plugin-check`, Qt
   plugins and Python `lib-dynload`. Check the core26 interpreter, no build/cache
   RPATHs, and no unresolved dependencies except deliberate host graphics.
3. Use a checksum-pinned compatible CSV plugin, exercise admission and load data.
   Install through the marketplace, restart, and verify persistence.
4. Source host ROS plus a workspace defining custom messages; test the ROS
   extension against an actual publisher using those messages.
5. Test `QT_QPA_PLATFORM=xcb` and `QT_QPA_PLATFORM=wayland` with visible plots and
   QRhi rendering. See upstream `packaging/smoke/render_check.sh` and
   `packaging/smoke/wayland_check.sh`. Offscreen startup alone is not a graphics
   acceptance test. Check hardware decode/software fallback as appropriate.

For diagnosis, use `snap run --shell plotjuggler`, `QT_DEBUG_PLUGINS=1`, and
`LD_DEBUG=libs`. Check `journalctl -xe` for host/session or driver errors;
AppArmor denials are not the primary diagnostic for this classic package.
HTTPS uses host certificate roots, and optional Mosaico receives a valid
`GRPC_DEFAULT_SSL_ROOTS_FILE_PATH` unless the caller already set it.

Keep Qt, FFmpeg, CPython and other third-party license notices with distributed
packages. Before a Store release, complete runtime checks, review licenses and
source-availability obligations, obtain classic approval, and then choose an
appropriate release channel and grade.
