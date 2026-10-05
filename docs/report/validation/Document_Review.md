# Assessment document review

Reviewed against the handwritten graphics criteria and the author's clarified **2 introduction + 10 technical + 1 thanks** structure. This is a laboratory project report, not a claim of a new rendering algorithm. The complete PDF remains below twenty pages; derivations and component-by-component explanations also live in the editable notes and visual catalogue.

## Coverage and evidence

| Requirement | Visible presentation content | Report and supporting explanation |
| --- | --- | --- |
| Complete project introduction and student information | Slides 1–2: spacecraft, overview, mission, inspection, comparison, author/course and video cue | Cover, title page, abstract and Chapter I; confirmed KUET/course teachers/October 2026 |
| Vertices, indices, triangles and unique objects | Slides 3–5: indexed sphere cell, all 26 bodies, primitive profiles, curved dish wireframe, rings and fields | Sections 3.1–3.2; worked index cell and all 18 hardware construction recipes in notes/catalogue |
| Translation, rotation, scaling and camera hierarchy | Slide 6: TRS, normal transform and parent/body/surface diagram | Section 3.3; world doubles, floating origin, nonlinear presentation scales and four camera modes |
| Complex motion and dynamic objects | Slide 7: matched moon revolution and axial-spin sequences, Hermite interpolation and quaternion flight; Slide 5 comet | Section 3.4; clock ownership, relative frames, post-data approximations and comet direction |
| Three lights and moving spotlight | Slide 8: Sun-only, camera spotlight and directional-fill comparison; moving headlamp frames | Section 3.5; attenuation, cosine cone, 12°/18° cutoffs and diffuse/specular composition |
| Five shading techniques | Slide 9: matched Earth and Voyager views for Flat, Gouraud, Phong, Blinn–Phong and Toon | Section 3.6; evaluation stage, normals, specular vectors and explicit Toon thresholds |
| Shadows and ray tracing | Slides 10–11: off/hard/soft, traced/raster comparisons, ray diagram and BVH diagram | Section 3.8; sphere/annulus/triangle intersection, visibility, depth map/PCF, BVH traversal and one reflection |
| Textures and relief/specular maps | Slide 12: Earth albedo/normal/mask, Moon maps off/on and spacecraft atlas | Section 3.7; UV, Sobel kernels, normal perturbation, ocean thresholds, atlas and asset credits |
| Performance, verification and defense | Notes and closing slide | Chapter IV measured eight-view table; Chapters V–VII, limitations, references, viva guide and validation logs |

## Claim boundaries

| Claim | Source/evidence | Qualification retained |
| --- | --- | --- |
| 26 bodies, four ring systems and procedural Voyager | Catalogues, scene/model construction, runtime captures | Ring systems contain multiple bands; 76 assemblies and 18 inspection targets are different counts |
| Shared sphere has 2,145 vertices and 3,968 triangles | `UvSphereGenerator.cpp`, indexed-cell figure and object documentation | Seam vertices duplicate; degenerate pole triangles are skipped; lower sphere LODs use different counts |
| Curved dish has 590 vertices and 1,152 triangles | `ParabolicDishGenerator.cpp`, builder parameters and real wireframe | Counts include both surfaces and rim; the former eight-ring handbook description was corrected |
| Historical mission follows one date | `MissionEphemeris`, `SimulationClock` and Hermite implementation | Compressed visual scale, accelerated moon/spin motion and post-2030 prediction are explicitly distinguished |
| All five shading modes are implemented | Runtime comparisons and scene/lighting shaders | Flat/Gouraud use different evaluation stages; the F9 traced view uses its own Blinn–Phong path |
| Ray tracing is implemented | Runtime F9/F10 captures and ray-tracing shader/BVH code | Hybrid renderer, one optional reflection, four ring layers; secondary reflections omit Voyager; no refraction or diffuse global illumination |
| Current measured performance is interactive | Retained eight-view Release benchmark on RX 590, 1440×900, vsync off | One hardware/run sample; GPU timing and frame interval differ; historical before/after figures were not newly reproduced |
| Brief input taps are preserved | Regression logs before/after GLFW sticky input fix | Source verifiers and scripted tours do not replace every hands-on mouse/flight/driver test |
| Visual examples represent the project | Runtime capture logs, figure provenance and matching SHA-256 checks | Diagrams explain construction; Earth channel plate reproduces the derivation and is not an exported GPU map |

## Five review dimensions

1. **Contribution:** the educational contribution is an integrated, inspectable mission explorer and explanation of established graphics techniques. No algorithmic novelty or superiority over research renderers is asserted.
2. **Writing clarity:** notation is defined alongside equations; worked examples and source pointers support reconstruction. The report condenses the technical account to nineteen pages; the notes/catalogue retain per-component detail rather than shrinking the template body font.
3. **Empirical strength:** real matched captures demonstrate lights, shaders, shadows, motion and map changes. Performance is reported with its exact hardware and protocol; no statistical or universal speed claim is made.
4. **Evaluation completeness:** Debug/Release builds, both repository verifiers, brief-input regression, runtime tours and document guards are recorded. Manual picking, every flight direction and the actual lab machine remain rehearsal checks, not falsely reported completed tests.
5. **Method soundness:** floating origin, indexed geometry, instancing/LOD, analytic shadow tests and triangle-BVH tracing match the code. Educational scales, simplified orbits/spin, derived relief maps and bounded reflection behavior are disclosed.

## Export and visual review

All nineteen report pages and thirteen presentation pages were rendered for review. A heading overlap in the first introduction was corrected. The final slide builder rejects overlapping text/equations/images. The report embeds Times New Roman, resolves citations and references and passes the no-overfull-box guard. The supplied logo and template are retained; the explicitly documented adaptations are course-teacher fields, combined contents/lists and shared Chapters V?VII. The author's final video is reserved after Slide 2 and is outside this rebuild.

See [document checks](document-check.json), [layout bounds](slide-layout.json), [LaTeX log](latex-build.txt), [visual provenance](../figures/technical/visual_manifest.json) and [validation evidence](README.md).

## Print-visibility revision

All eleven technical report images now occupy the approximately 6.1-inch printable width. Height caps and fixed bottom placement were removed. Figures/captions remain together at their source location while methodology flows naturally across pages. The shader comparison uses a three-column, two-row grid of cropped Earth views with larger labels; Moon map comparisons use matching detail crops. The overview excludes telemetry bands. Technical derivations remain; introductory/closing prose was condensed and Chapters V–VII share a page to retain nineteen pages. The figure list now references figure labels rather than assuming the section and its image share a page. Export checks reject images narrower than 430 PDF points or images crossing the template margins. The final nineteen pages were rendered again for visual review.

## Learning-resource consistency correction

Preparing the [rehearsal workbook](../../guide/12-presentation-rehearsal.md) exposed an incorrect assessment star count. `EnvironmentBuilder::buildStarLayers` creates 5,200 + 1,800 + 380 = **7,380 background points**; the original starfield object reference already documented this correctly. Report, slide text/notes, visual fundamentals and explanatory figures now use that count. The 4,000 asteroid instances are unchanged. The mission-learning chapter's opening now distinguishes historical planets/Voyager from visual moons/spin/comet animation. PDF/PPTX and LaTeX guards passed after regeneration; workbook resource/image links resolve.
