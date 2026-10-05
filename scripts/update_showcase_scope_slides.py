"""Add scope cards to edited decks while preserving all other package parts.

Only slide 2 and its existing speaker-note XML are replaced. Images, other
slides, relationships, themes and embedded content are copied unchanged.
"""
from io import BytesIO
from pathlib import Path
import pymupdf
import json
import zipfile

from pptx import Presentation
from pptx.dml.color import RGBColor
from pptx.util import Inches, Pt

ROOT = Path(__file__).resolve().parents[1]
CARDS = [
    ('PROPOSED FEATURES', 'Earth → interstellar mission\nSpacecraft • path • space environment\nTRS + hierarchy • spin • cameras\nSun • Flat / Gouraud / Phong • input'),
    ('IMPLEMENTED + ADDITIONS', '26 bodies • 18 hardware targets\nMission restart • manual flight\nExtra lights/shaders • surface maps\nShadows • BVH ray tracing'),
]
NOTES = '''PROPOSED SCOPE AND FINAL IMPLEMENTATION
The submitted two-page proposal is titled Voyager 2: Journey Beyond the Solar System. It planned an Earth-to-interstellar journey through Jupiter, Saturn, Uranus and Neptune, a primitive spacecraft, a visible path, starfield and heliopause. It also planned translation, rotation, scaling, composite and hierarchical transforms, planet spin, Sun lighting with ambient/diffuse/specular terms, Flat/Gouraud/Phong shading, and camera and keyboard interaction.

Point to the mission, inspection and comparison screenshots above the cards while describing the implemented feature groups. Launch bookmark 1 provides a mission restart; P pauses, =/- changes speed, C changes camera rig, T toggles the path and F3 selects the named shading technique. Explain that user input changes application state, update computes motion, and render consumes the current transforms and materials.

Additional features include 26 textured bodies, four ring systems, 76 spacecraft assemblies and 18 inspection targets, 8,500 instanced rocks, a dynamic comet and 7,380 stars. The final project adds manual six-degree flight, component inspection, a moving camera spotlight and directional fill, Blinn-Phong and Toon shading, derived surface maps, shadows and hybrid BVH ray tracing. These extend the implemented feature groups from the proposal.

Give the overview first; the following ten technical slides derive geometry, transforms, motion, lighting, shading, shadows, tracing and textures. The report methodology contains the detailed implementation and equations, and its typography follows the supplied CSE-4000 report template. Play the final project video at the existing cue, narrating each visible feature as it changes.
'''
PREFIX = 'Showcase scope '
LIGHTING_PREFIX = 'Showcase lighting equation '
LIGHTING_NOTE_MARKER = 'The lower-left panel shows ambient, diffuse and Phong-specular equations'

def add_cards(slide):
    for shape in list(slide.shapes):
        if shape.name.startswith(PREFIX):
            shape._element.getparent().remove(shape._element)
    for index, (heading, body) in enumerate(CARDS):
        x = 0.44 + index * 4.25
        background = slide.shapes.add_shape(1, Inches(x), Inches(5.18), Inches(3.97), Inches(1.62))
        background.name = PREFIX + heading + ' background'
        background.fill.solid()
        background.fill.fore_color.rgb = RGBColor.from_string('EDF3F6')
        background.line.fill.background()
        for label, text, y, h, size, bold, colour in [
            ('heading', heading, 5.32, 0.32, 15, True, '167F82'),
            ('body', body, 5.70, 1.00, 14, False, '14283B'),
        ]:
            box = slide.shapes.add_textbox(Inches(x + 0.16), Inches(y), Inches(3.65), Inches(h))
            box.name = PREFIX + heading + ' ' + label
            frame = box.text_frame
            frame.margin_top = frame.margin_bottom = frame.margin_left = frame.margin_right = 0
            frame.word_wrap = False
            for i, line in enumerate(text.splitlines()):
                p = frame.paragraphs[0] if i == 0 else frame.add_paragraph()
                p.text = line
                p.font.name = 'Arial'
                p.font.size = Pt(size)
                p.font.bold = bold
                p.font.color.rgb = RGBColor.from_string(colour)
                p.line_spacing = Pt(18)
                p.space_after = Pt(0)

def add_lighting_equations(slide):
    for shape in list(slide.shapes):
        if shape.name.startswith(LIGHTING_PREFIX):
            shape._element.getparent().remove(shape._element)
    background = slide.shapes.add_shape(1, Inches(0.44), Inches(5.83), Inches(7.78), Inches(0.67))
    background.name = LIGHTING_PREFIX + 'background'
    background.fill.solid()
    background.fill.fore_color.rgb = RGBColor.from_string('EDF3F6')
    background.line.fill.background()
    box = slide.shapes.add_textbox(Inches(0.59), Inches(5.87), Inches(7.52), Inches(0.59))
    box.name = LIGHTING_PREFIX + 'terms'
    frame = box.text_frame
    frame.margin_top = frame.margin_bottom = frame.margin_left = frame.margin_right = 0
    frame.word_wrap = False
    for i,line in enumerate([
        'AMBIENT  kₐ = 0.07   •   DIFFUSE  D = max(n·l, 0)',
        'SPECULAR  Sₚ = kₛ max(r·v, 0)ᵖ   •   I = a(kₐ+VₛDₛ+Dₒ)+VₛSₛ+Sₒ',
    ]):
        p = frame.paragraphs[0] if i == 0 else frame.add_paragraph()
        p.text = line
        p.font.name = 'Arial'
        p.font.size = Pt(13)
        p.font.bold = True
        p.font.color.rgb = RGBColor.from_string('14283B')
        p.line_spacing = Pt(16)
        p.space_after = Pt(0)

def update_pdf_equation_page():
    report_pdf = ROOT/'docs/report/Voyager_2_Presentation.pdf'
    doc = pymupdf.open(report_pdf)
    if len(doc) != 13:
        raise ValueError('Expected the existing 13-page presentation PDF')
    page = doc[7]
    rect = pymupdf.Rect(31.68, 419.76, 591.84, 468.0)
    page.draw_rect(rect, color=None, fill=(237/255,243/255,246/255), overlay=True)
    font = pymupdf.Font(fontfile='C:/Windows/Fonts/arialbd.ttf')
    page.insert_font(fontname='scopeArial', fontbuffer=font.buffer)
    lines = [
        'AMBIENT  k_a = 0.07   |   DIFFUSE  D = max(n . l, 0)',
        'SPECULAR  S_P = k_s max(r . v, 0)^p',
        'I = a(k_a + V_S D_S + D_o) + V_S S_S + S_o',
    ]
    text_rect = pymupdf.Rect(42.48, 422.64, 582.0, 465.12)
    result = page.insert_textbox(text_rect, '\n'.join(lines), fontname='scopeArial',
                                 fontsize=10.3, lineheight=1.13,
                                 color=(20/255,40/255,59/255), overlay=True)
    if result < 0:
        doc.close()
        raise ValueError('Lighting equations overflow their PDF panel')
    temporary = report_pdf.with_name('Voyager_2_Presentation.equations.pdf')
    doc.save(temporary, garbage=3, deflate=True)
    doc.close()
    temporary.replace(report_pdf)

def update(path):
    original = path.read_bytes()
    ppt = Presentation(BytesIO(original))
    assert len(ppt.slides) == 13
    slide = ppt.slides[1]
    # Do not cover author-added material in the area reserved for scope cards.
    for shape in slide.shapes:
        if shape.name.startswith(PREFIX):
            continue
        if (shape.left < Inches(8.66) and shape.left + shape.width > Inches(0.44)
                and shape.top < Inches(6.80) and shape.top + shape.height > Inches(5.18)):
            raise ValueError(f'Author content occupies scope-card area: {path.name}: {shape.name}')
    add_cards(slide)
    notes = slide.notes_slide.notes_text_frame
    if NOTES.splitlines()[0] not in notes.text:
        notes.text = notes.text.rstrip() + '\n\n' + NOTES
    lighting = ppt.slides[7]
    add_lighting_equations(lighting)
    lighting_notes = lighting.notes_slide.notes_text_frame
    if LIGHTING_NOTE_MARKER not in lighting_notes.text:
        lighting_notes.text = lighting_notes.text.rstrip() + '\n\n' + LIGHTING_NOTE_MARKER + ' together with the implemented final light-composition equation. Slide 9 shows the Phong and Blinn-Phong specular equations side by side. In the report, see Chapter III, Section 3.5 (PDF pages 12–13).'
    target_parts = {str(slide.part.partname).lstrip('/'), str(slide.notes_slide.part.partname).lstrip('/')}
    target_parts.update({str(lighting.part.partname).lstrip('/'), str(lighting.notes_slide.part.partname).lstrip('/')})
    modified = BytesIO()
    ppt.save(modified)
    output = BytesIO()
    with zipfile.ZipFile(BytesIO(original)) as before, zipfile.ZipFile(modified) as after, zipfile.ZipFile(output, 'w') as dest:
        for info in before.infolist():
            dest.writestr(info, after.read(info.filename) if info.filename in target_parts else before.read(info.filename))
    with zipfile.ZipFile(BytesIO(original)) as before, zipfile.ZipFile(output) as after:
        changed = [name for name in before.namelist() if before.read(name) != after.read(name)]
        assert set(changed) <= target_parts
        assert set(before.namelist()) == set(after.namelist())
    path.write_bytes(output.getvalue())
    return {'file':str(path.relative_to(ROOT)), 'slides':13, 'changed_package_parts':changed,
            'all_other_parts_preserved':True, 'visible_cards':[card[0] for card in CARDS],
            'lighting_equations_visible':True}

def main():
    records = [update(ROOT/'docs/report'/name) for name in ['Voyager_2_Presentation.pptx', 'Final-Presentation.pptx']]
    (ROOT/'docs/report/validation/slide-scope-update.json').write_text(json.dumps(records, indent=2)+'\n', encoding='utf-8')
    update_pdf_equation_page()
    print('Updated scope cards and lighting equations; unrelated deck content is preserved.')

if __name__ == '__main__':
    main()
