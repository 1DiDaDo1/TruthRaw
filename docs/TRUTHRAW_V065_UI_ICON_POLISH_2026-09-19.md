# TruthRaw v0.65 — compact-phone UI + clean TR lens icon

Date: 2026-09-19

Status: **IMPLEMENTED / GCC + CLANG + ANDROID CI GREEN / APK VERIFIED**

Active branch:

`integration/truthraw-suite-v0-65-ui-icon-polish`

App version:

`0.30-v0.65-ui-icon-polish`

## Scope

v0.65 is intentionally a presentation/navigation correction on top of v0.64.

It does not change:

- sealed-source admission;
- Scientific Master pixels;
- TruthRange/Zero-Line;
- scene-scale;
- the frozen 180-byte Technical Backplane;
- PURE Float32 writer contract `TRUTHRAW_PURE_SELF_BINDING_V0_63`;
- the v0.63 CRC correction;
- v0.64 Advanced derivative mathematics or authority boundaries;
- physicalFrameCount=1 / independentEvidenceCount=1.

## Compact-phone fixes

Real-device screenshots of v0.64 showed three UI defects:

1. scroll content could move underneath the Android status bar;
2. TRUTHRAW PURE and TRUTHRAW ADVANCED titles wrapped/clipped poorly on compact widths;
3. the lower output region visually collided with the Android navigation area.

v0.65 moves system-bar insets from the scrolling child content to the ScrollView viewport itself. `clipToPadding=true` keeps scrolled content inside the safe visual region. The same pattern is applied to the launcher, Advanced settings and research/settings screens.

The two TruthRaw output titles are now intentionally rendered as exactly two lines:

`TRUTHRAW\nPURE`

`TRUTHRAW\nADVANCED`

with compact title sizing and slightly taller TruthRaw cards. JPG/JPG XL remain shorter.

## Icon correction

The earlier generated icon was recognizable but visually noisy/glitch-like on device.

v0.65 replaces it with a clean dark TR monogram:

- white-to-blue TR body;
- subtle blue border/glow;
- compact camera-lens/open-world reflection located in the upper R area;
- no large digital-glitch overlay.

Packaged asset:

`suite_android/app/src/main/res/drawable-nodpi/truthraw_icon.webp`

Asset SHA-256:

`398874fcf8052761fce9451e70088e20985210ef49b869e7c6f14cf7cb4fe607`

## CI / APK

Workflow run `35473170260`:

- host GCC: **SUCCESS**
- host Clang: **SUCCESS**
- Android arm64: **SUCCESS**

Artifact:

- ID: `10593941511`
- name: `truthraw-suite-v0-65-ui-icon-polish-debug-arm64`
- archive SHA-256: `10004e3b4ac318c8a021a876e81963c56f752e462364ed0e447679844950d7ed`

Extracted APK:

- bytes: `5962141`
- SHA-256: `9557aaba0222ebed23be961b3865fe5004eaa0af5589de654439eacb766bf21d`

APK inspection confirms the new icon is packaged and the v0.64 Advanced/PURE native markers remain present.

## Remaining gate

Install v0.65 on the compact real device and visually confirm:

- status-bar overlap is gone during scrolling;
- PURE and ADVANCED titles remain legible and contained;
- bottom content stays clear of Android navigation;
- the clean TR/lens icon appears in both app header and launcher.
