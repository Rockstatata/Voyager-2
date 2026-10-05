# Validation evidence and limits

Audit date: 5 October 2026. Working directory: repository root. Runtime: Windows, OpenGL 3.3 core, Radeon RX 590 Series; window/framebuffer 1440×900. Debug/Release x64 compiled with the installed Visual Studio 18 Community MSBuild. No new C++ source/header was needed, so project/filter registrations are unchanged.

| Check | Result and evidence |
| --- | --- |
| Debug x64 build | Passed; [build-debug.txt](build-debug.txt) |
| Release x64 build | Passed; [build-release.txt](build-release.txt) |
| Scene layout | Passed; [scene-layout.txt](scene-layout.txt) |
| Navigation and motion contracts | Passed; [navigation-motion.txt](navigation-motion.txt) |
| Brief keyboard taps | Original build missed all six test taps; fixed by GLFW sticky polling; [input-taps.txt](input-taps.txt), [original runtime](input-taps-before.txt); run scripts/verify_input_taps.py |
| Runtime keyboard smoke | Pause, free movement, Focus/Inspect, thrust/yaw/pitch/roll, flight-mode return, five shaders, lighting switches, F9/F10 and safe quit exercised; [input-runtime.txt](input-runtime.txt) |
| Runtime shaders and scene setup | Loaded successfully; [benchmark-runtime.txt](benchmark-runtime.txt), [demo-runtime.txt](demo-runtime.txt) |
| Runtime captures | 13 standard, 20 shading and 16 raster/traced frames, plus 18 demo chapter images; JPEG selections in ../figures/ |
| PDF/report layout | Nineteen XeLaTeX pages; all rendered for visual review; embedded Times New Roman, resolved references and no overfull boxes |
| Presentation | Thirteen PDF pages / editable PPTX slides; 2 introductions + 10 technical + thanks; all have substantial notes; overlap guard passes |
| Existing demo media | Earlier optional recording: 120 seconds, 3,600 frames at 30 fps, 1440×900 H.264; the author will attach the final recording |
| Release performance | Eight views, vsync disabled; [benchmark.txt](benchmark.txt); mean 1.71 ms, worst raster view 1.15 ms, traced Voyager 6.60 ms |

The vendored GLFW binary emits LNK4099 warnings because its debug-symbol PDB is absent. Release also emits LNK4098 for a default runtime-library conflict. These are existing dependency/configuration warnings, not hidden compile failures. The audit does not claim warning-free builds or resolve/rebuild the instructor's vendored library.

The layout verifier exercises numerical/data checks including encounter geometry. The navigation verifier checks source contracts; it does not inject all input combinations. Scripted captures exercise camera framing, scene data, shader loading and switches through controller APIs. The moving spotlight is shown during a scripted camera orbit. Human checks for picking, mouse boundaries, every manual flight direction and the target lab hardware remain a presentation rehearsal task.

Benchmark frame time includes the full frame interval, while GPU timing covers Application::render(), including HUD. Renderer draw/triangle counters exclude shadow depth, fullscreen tracing and text; instancing multiplies counted triangles by instance count. GL_QUERY_RESULT can wait for unfinished samples. This is one fresh current-build run; earlier before/after results in guide chapter 11 were not newly reproduced. Video recording and benchmark were run separately.

The original BMP captures and PDF-review renders are under captures/assessment/, ignored by git. The rebuild adds a 25-frame `--capture-assessment` tour and a fresh 26-body tour, with [technical-capture-runtime.txt](technical-capture-runtime.txt) and [bodies-capture-runtime.txt](bodies-capture-runtime.txt). Clean screenshots temporarily suppress HUD/labels within the scripted run. Wireframes use polygon-line mode on actual meshes; no demonstration geometry is synthesized.

[document-check.json](document-check.json) records page/slide counts, fonts, chapter structure, removed sections, seven external references and all eleven full-width images; [latex-build.txt](latex-build.txt) retains three XeLaTeX passes. [slide-layout.json](slide-layout.json) records deck content bounds. The acknowledgment and original Societal/Complex Engineering chapters are removed; the expanded methodology spans eleven pages. Compact contents/lists and a shared results/conclusion page retain the page limit.

Earlier JPEG source hashes are in [provenance.json](../figures/provenance.json); the new [visual manifest](../figures/technical/visual_manifest.json) records code/capture sources and SHA-256 hashes. Source maps, scientific data and approximation caveats are in [the viva guide](../Assessment_and_Viva.md) and [visual fundamentals](../Visual_Fundamentals.md). Existing media verification applies to the earlier recording, not the author's pending final video.

[Document review](Document_Review.md) maps every requirement to slides/report sections and records claim evidence, review findings and the remaining manual rehearsal checks. All visual source hashes were checked against the final generated manifests.
