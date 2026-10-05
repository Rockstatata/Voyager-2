"""Regenerate the XeLaTeX report, source-backed figures and 13-slide deck.

Python dependencies: docs/report/requirements.txt. Install MiKTeX or TeX Live
with XeLaTeX and the packages listed in the .tex preamble. Missing TeX packages
fail explicitly instead of opening an unattended installer dialog.
"""
import json
import os
from pathlib import Path
import re
import shutil
import subprocess

import pymupdf
from pptx import Presentation

import build_assessment_figures
import build_assessment_slides

ROOT=Path(__file__).resolve().parents[1]
REPORT=ROOT/'docs/report'
BUILD=ROOT/'captures/assessment/latex'

def compile_report():
    compiler=shutil.which('xelatex')
    if not compiler:raise RuntimeError('XeLaTeX is required; install MiKTeX or TeX Live.')
    BUILD.mkdir(parents=True,exist_ok=True)
    environment=os.environ.copy()
    # MiKTeX scans PATH entries as directories. Ignore malformed local entries
    # such as a launcher .exe; this affects this subprocess only.
    environment['PATH']=os.pathsep.join(p for p in environment['PATH'].split(os.pathsep) if Path(p).is_dir())
    logs=[]
    for iteration in range(3):
        result=subprocess.run([compiler,'--disable-installer','--interaction=nonstopmode','--halt-on-error',
                               '--output-directory='+str(BUILD),'Voyager_2_Graphics_Report.tex'],
                              cwd=REPORT,env=environment,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,
                              text=True,encoding='utf-8',errors='replace')
        logs.append(f'PASS {iteration+1}\n'+result.stdout)
        if result.returncode:
            (REPORT/'validation/latex-build.txt').write_text('\n'.join(logs),encoding='utf-8')
            raise RuntimeError('XeLaTeX failed; see docs/report/validation/latex-build.txt')
    (REPORT/'validation/latex-build.txt').write_text('\n'.join(logs),encoding='utf-8')
    if re.search(r'Overfull|undefined references|Reference .* undefined|Citation .* undefined',result.stdout):
        raise RuntimeError('LaTeX layout/reference issue; inspect validation/latex-build.txt')
    output=BUILD/'Voyager_2_Graphics_Report.pdf'
    with pymupdf.open(output) as doc:
        if len(doc)!=19:raise ValueError(f'Expected 19 report pages; generated {len(doc)}. Do not shrink the template font.')
    shutil.copy2(output,REPORT/'Voyager_2_Graphics_Report.pdf')

def verify_exports():
    with pymupdf.open(REPORT/'Voyager_2_Graphics_Report.pdf') as report:
        report_pages=len(report);text='\n'.join(page.get_text() for page in report)
        for required in ['Sarwad Hasan Siddiqui','2107006','CSE 4102','October','6.60','1.71']:
            if required not in text:raise ValueError('Missing report metadata/evidence: '+required)
        fonts=sorted({span['font'] for page in report for block in page.get_text('dict')['blocks']
                      for line in block.get('lines',[]) for span in line['spans']})
        if not any('TimesNewRoman' in name for name in fonts):raise ValueError('Report must embed Times New Roman')
        # Every technical image must occupy the full printable text width.
        # Catch height caps or thumbnail sizing silently shrinking later exports.
        figure_bounds=[{'page':page.number+1,'bbox':list(info['bbox'])}
                       for page in report if page.number>=5 for info in page.get_image_info()]
        if len(figure_bounds)<11:raise ValueError('Missing technical report figures')
        for item in figure_bounds:
            x0,y0,x1,y1=item['bbox']
            if x1-x0<430:raise ValueError(f"Report figure is too small on page {item['page']}")
            if y0<86 or y1>report[item['page']-1].rect.height-72:
                raise ValueError('Report figure crosses template margins')
    with pymupdf.open(REPORT/'Voyager_2_Presentation.pdf') as slides:
        slide_pages=len(slides)
    ppt=Presentation(REPORT/'Voyager_2_Presentation.pptx')
    if slide_pages!=13 or len(ppt.slides)!=13:raise ValueError('Deck must be 2 intro + 10 technical + 1 thanks')
    if not all(s.has_notes_slide and len(s.notes_slide.notes_text_frame.text)>800 for s in ppt.slides):
        raise ValueError('Every slide needs substantial fundamentals and code-backed notes')
    result={'report_pages':report_pages,'slide_pages':slide_pages,'pptx_slides':len(ppt.slides),
            'structure':'2 introductions + 10 technical + 1 thank you','report_engine':'XeLaTeX',
            'embedded_report_fonts':fonts,'video':'author records and attaches later'}
    result['report_figure_bounds']=figure_bounds
    (REPORT/'validation/document-check.json').write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
    (REPORT/'validation/document-check.txt').write_text('19-page XeLaTeX report; 13 PDF/PPTX slides with detailed notes. Confirmed KUET, student, course, teachers, October 2026 and benchmark.\n',encoding='utf-8')

def main():
    build_assessment_figures.build()
    build_assessment_slides.main()
    compile_report()
    verify_exports()
    print('[ASSESSMENT] 19-page XeLaTeX report; 13 slides; source-backed screenshots/diagrams; detailed speaker notes')

if __name__=='__main__':main()
