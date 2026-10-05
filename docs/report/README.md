# CSE 4102 assessment package

Prepared for **Sarwad Hasan Siddiqui, Roll 2107006**, Department of CSE, **KUET**, **October 2026**. Course: **Computer Graphics and Image Processing Laboratory**. Course teachers: **Md Tajmilur Rahman, Lecturer**, and **Md Mubtashim Abrar Nihal, Lecturer**.

The presentation follows the clarified structure: **two introduction slides, ten technical slides, one thank-you slide**. The report is **nineteen A4 pages**, including cover, front matter and references. The author will record and attach the final video after the introduction; the new document build does not record or embed a video.

| Deliverable | Contents |
| --- | --- |
| [Report PDF](Voyager_2_Graphics_Report.pdf) | Nineteen pages; five chapters; eleven-page methodology; numbered equations; full-width figures; verification; limitations; seven external references |
| [Editable LaTeX report](Voyager_2_Graphics_Report.tex) | XeLaTeX source with Times New Roman and centralized template formatting |
| [Editable PowerPoint](Voyager_2_Presentation.pptx) | Thirteen slides, editable text, high-resolution equations/screenshots and detailed speaker notes |
| [Presentation PDF](Voyager_2_Presentation.pdf) | Thirteen-slide companion exported from the edited main PowerPoint, including the scope comparison |
| [Editable speaker-note source](Presentation_Notes_Source.md) | Derivations, all eighteen hardware constructions, worked examples, source paths and caveats |
| [Exported speaker notes](Presentation_Notes.md) | The explanations embedded in the current PPTX |
| [Visual fundamentals catalogue](Visual_Fundamentals.md) | Full-resolution body/hardware catalogues, primitive rules, worked topology examples and links to every object |
| [Audit and viva guide](Assessment_and_Viva.md) | Requirement evidence, formulas, defense questions and rehearsal checks |
| [Presentation learning workbook](../guide/12-presentation-rehearsal.md) | Study route, worked problems, closed-book checks, live-demo sequence and mock viva |
| [Teacher showcase preparation](../guide/13-teacher-showcase.md) | Current requirements, original proposal comparison, narrated demo, implementation navigation and parameter-change drills |
| [Validation](validation/README.md) | Builds, scene checks, runtime capture logs, input regression and document checks |
| [Visual provenance](figures/technical/visual_manifest.json) | Actual source images/code and SHA-256 hashes for each new screenshot plate/diagram |

## Presentation sequence

| Slide | What it explains |
| ---: | --- |
| 1 | Project purpose, complete scope, representative scenes and student/course information |
| 2 | Mission exploration, inspection, controls and rendering comparisons; cue for the author's video |
| 3 | Vertex layout, buffers, sphere parameters, indexed cells, seams/poles, counts and all 26 bodies |
| 4 | Procedural Voyager; box/cylinder/frustum/rod/dish construction, normals, counts and curved wireframe |
| 5 | Rings, instancing, all three fields, points/lines, heliosphere, HUD and dynamic comet |
| 6 | TRS order, parent/child matrices, inverse-transpose normals, scale, floating origin and four cameras |
| 7 | Hermite historical flight, moon revolution, spin, manual quaternion/inertial flight and comet direction |
| 8 | Point, spot and directional light equations; matched lighting views; moving-headlamp sequence |
| 9 | All five shading cases on Earth and Voyager; evaluation stages and specular formulas |
| 10 | Off/hard/soft shadows, finite-Sun visibility, translucent bands and spacecraft depth-map/PCF |
| 11 | Primary/shadow/reflection rays, sphere/ring/triangle tests, BVH and bounded hybrid Whitted tracing |
| 12 | Body albedo, NASA atlas, Sobel normal maps, ocean specular mask and runtime maps off/on |
| 13 | Thanks, implementation evidence and discussion |

Speaker notes are substantive explanations, not a substitute for visible content: they expand the equations and connect each example to its implementation. Full-resolution catalogue plates are available separately because 26 body names and eighteen hardware close-ups cannot all be readable thumbnails on one projected slide.

Slide 2 shows **Proposed features** and **Implemented + additions**, matching the report's project overview. Slide 8 visibly shows the ambient, diffuse and specular terms and the combined illumination equation. Slide 9 already shows the Phong and Blinn--Phong specular equations below the matched views. The report derivation is Chapter III, Section 3.5, PDF pages 12–13. Both edited PPTX files retain thirteen slides and unrelated slide content/media. Expanded notes define every symbol and describe the equations. To reapply these cards and equations to closed PPTX files, use `python -X utf8 scripts/update_showcase_scope_slides.py`; close both files in PowerPoint first. This targeted updater preserves unrelated package parts and does not regenerate other slides. It also adds the slide 8 equation to the presentation PDF.

Report figures occupy the full printable width (about 6.1 inches), with captions immediately below. Methodology flows across pages instead of anchoring small images at the bottom of fixed pages. The five shading examples use a two-row, three-column close-up grid with larger labels; Moon relief examples use matching detail crops. The overview removes telemetry bands. Figure sources retain full-resolution provenance. Introductory/closing prose is condensed to preserve the nineteen-page limit while retaining the technical equations and template body font.

## Template formatting

The supplied [CSE-4000 template](<CSE-4000-Final (Template).pdf>) and [KUET logo](KUET-LOGO.png) are preserved. The report uses XeLaTeX, A4, **Times New Roman 12-point body text with 1.5-line spacing**, a **1.2-inch left/top margin and 1-inch right/bottom margin**, template-sized centered chapter titles, 14-point sections, 11-point captions, Roman front-matter pagination, Arabic main pagination, chapter-numbered equations, figure captions below, table captions above and IEEE-style references. The cover places the project-number field in the header, uses the template's title/author hierarchy and centers the logo within its specified 1.14×1.0-inch box while preserving its aspect ratio.

Course teachers replace the thesis supervisor field. At the author's request, the acknowledgment and the original Societal/Professional and Complex Engineering chapters are removed. Conclusions is renumbered Chapter V and shares a page with results. Methodology now spans eleven pages, adding worked indices/transforms, all eighteen hardware constructions, picking, Hermite reasoning, tangent-frame normal mapping, BVH details and rendering optimizations. The bibliography contains seven external sources and no project self-reference. The contents/lists are combined to retain the **less-than-twenty-page** constraint. The template typography and full-width figures remain; its example names, topic, signature placeholder and annotation boxes are not copied.

## Rebuild the documents

**Preserving edited slides:** the full builder below regenerates the PPTX from its source and overwrites manual PowerPoint edits. The author's current `Voyager_2_Presentation.pptx` and `Final-Presentation.pptx` were preserved during the showcase audit. For report-only changes, use the following command, which does not rebuild either presentation:

```powershell
python -X utf8 -c "import sys; sys.path.insert(0,'scripts'); import build_assessment_report as r; r.compile_report()"
```

Requires Windows fonts, Python dependencies and **XeLaTeX** (MiKTeX or TeX Live), with the packages in the report's preamble. The document builder disables unattended TeX package installation and reports missing packages explicitly.

```powershell
python -m pip install -r docs/report/requirements.txt
python -X utf8 scripts/build_assessment_report.py
```

This generates source-backed diagrams/comparison plates, the PDF/PPTX deck and speaker notes, then compiles LaTeX three times to resolve references. Checks require **nineteen report pages**, **thirteen slides**, substantive notes, Times New Roman, confirmed metadata, resolved citations and no overfull LaTeX boxes. The deck builder checks text/image/equation overlap. PDF and PPTX share one coordinate layout; text stays editable in PowerPoint. Intermediate TeX files and visual-review renders are in ignored `captures/assessment/`.

For fresh runtime evidence, run the application tours from the repository root. The technical tour uses actual scene objects and temporarily suppresses the HUD/labels for clean comparison images; its changes are confined to that scripted run.

```powershell
.\scripts\build.ps1
.\scripts\build.ps1 -Configuration Release
.\scripts\verify_scene_layout.ps1
.\scripts\verify_navigation_and_motion.ps1
python scripts/verify_input_taps.py
x64\Debug\Voyager-2.exe --capture captures/assessment/tour
x64\Debug\Voyager-2.exe --capture-bodies captures/assessment/bodies
x64\Debug\Voyager-2.exe --capture-assessment captures/assessment/technical
x64\Debug\Voyager-2.exe --capture-shading captures/assessment/shading
x64\Release\Voyager-2.exe --capture-raytrace captures/assessment/raytrace
```

The hardware catalogue also uses the existing component images in `docs/objects/images/voyager/`, captured from the unchanged procedural model. `--capture-voyager` regenerates them. The retained benchmark table is the documented eight-view Release run; it is separate from video recording. If a new benchmark is taken, update the report's numeric results and regenerate before submission.

The existing MP4 and recording helper remain available from the earlier package. They are not the final video requested here and are neither regenerated nor embedded by this rebuild. Attach your own final recording at slide 2's cue. Rehearse mouse picking, camera transitions and manual flight on the actual lab machine.
