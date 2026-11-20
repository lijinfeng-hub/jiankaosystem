"""Generate software-copyright application DOCX materials for this project.

The generator intentionally keeps the code-material document tied to the current
source tree so the reported source-line count and the extracted first/last pages
remain reproducible after a source change.
"""

from __future__ import annotations

import re
from datetime import date
from pathlib import Path
from typing import Iterable, Sequence

from docx import Document
from docx.enum.section import WD_SECTION
from docx.enum.table import WD_CELL_VERTICAL_ALIGNMENT, WD_TABLE_ALIGNMENT
from docx.enum.text import WD_ALIGN_PARAGRAPH, WD_BREAK, WD_LINE_SPACING
from docx.oxml import OxmlElement
from docx.oxml.ns import qn
from docx.shared import Inches, Pt, RGBColor
from docx.enum.style import WD_STYLE_TYPE


ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "docs" / "softcopyright"
APP_NAME = "智能排监考管理系统"
VERSION = "V1.0"
TODAY = date.today().isoformat()

BLUE = "2E74B5"
NAVY = "1F4D78"
GRAY = "5B6573"
LIGHT_BLUE = "E8EEF5"
LIGHT_GRAY = "F2F4F7"
WHITE = "FFFFFF"
BLACK = "1F1F1F"
FONT = "Microsoft YaHei"
CODE_FONT = "Consolas"
PAGE_WIDTH_DXA = 9360


def set_run_font(run, name=FONT, size=None, color=BLACK, bold=None, italic=None):
    run.font.name = name
    run._element.get_or_add_rPr().rFonts.set(qn("w:ascii"), name)
    run._element.get_or_add_rPr().rFonts.set(qn("w:hAnsi"), name)
    run._element.get_or_add_rPr().rFonts.set(qn("w:eastAsia"), name)
    if size is not None:
        run.font.size = Pt(size)
    if color is not None:
        run.font.color.rgb = RGBColor.from_string(color)
    if bold is not None:
        run.bold = bold
    if italic is not None:
        run.italic = italic


def shade(cell, fill):
    tc_pr = cell._tc.get_or_add_tcPr()
    shd = tc_pr.find(qn("w:shd"))
    if shd is None:
        shd = OxmlElement("w:shd")
        tc_pr.append(shd)
    shd.set(qn("w:fill"), fill)


def set_cell_margins(cell, top=80, start=120, bottom=80, end=120):
    tc = cell._tc
    tc_pr = tc.get_or_add_tcPr()
    tc_mar = tc_pr.first_child_found_in("w:tcMar")
    if tc_mar is None:
        tc_mar = OxmlElement("w:tcMar")
        tc_pr.append(tc_mar)
    for m, value in (("top", top), ("start", start), ("bottom", bottom), ("end", end)):
        node = tc_mar.find(qn(f"w:{m}"))
        if node is None:
            node = OxmlElement(f"w:{m}")
            tc_mar.append(node)
        node.set(qn("w:w"), str(value))
        node.set(qn("w:type"), "dxa")


def set_table_geometry(table, widths: Sequence[int], header=False):
    """Apply explicit DXA geometry required by the document preset."""
    table.alignment = WD_TABLE_ALIGNMENT.LEFT
    table.autofit = False
    tbl = table._tbl
    tbl_pr = tbl.tblPr
    tbl_w = tbl_pr.first_child_found_in("w:tblW")
    if tbl_w is None:
        tbl_w = OxmlElement("w:tblW")
        tbl_pr.append(tbl_w)
    tbl_w.set(qn("w:w"), str(sum(widths)))
    tbl_w.set(qn("w:type"), "dxa")
    tbl_ind = tbl_pr.first_child_found_in("w:tblInd")
    if tbl_ind is None:
        tbl_ind = OxmlElement("w:tblInd")
        tbl_pr.append(tbl_ind)
    tbl_ind.set(qn("w:w"), "120")
    tbl_ind.set(qn("w:type"), "dxa")

    grid = tbl.tblGrid
    for child in list(grid):
        grid.remove(child)
    for width in widths:
        col = OxmlElement("w:gridCol")
        col.set(qn("w:w"), str(width))
        grid.append(col)

    for row_idx, row in enumerate(table.rows):
        tr_pr = row._tr.get_or_add_trPr()
        cant_split = OxmlElement("w:cantSplit")
        tr_pr.append(cant_split)
        if header and row_idx == 0:
            repeat = OxmlElement("w:tblHeader")
            repeat.set(qn("w:val"), "true")
            tr_pr.append(repeat)
        for idx, cell in enumerate(row.cells):
            width = widths[min(idx, len(widths) - 1)]
            cell.width = Inches(width / 1440)
            tc_pr = cell._tc.get_or_add_tcPr()
            tc_w = tc_pr.first_child_found_in("w:tcW")
            if tc_w is None:
                tc_w = OxmlElement("w:tcW")
                tc_pr.append(tc_w)
            tc_w.set(qn("w:w"), str(width))
            tc_w.set(qn("w:type"), "dxa")
            set_cell_margins(cell)
            cell.vertical_alignment = WD_CELL_VERTICAL_ALIGNMENT.CENTER


def set_table_borders(table, color="C8D2DF", size="6"):
    tbl_pr = table._tbl.tblPr
    borders = tbl_pr.first_child_found_in("w:tblBorders")
    if borders is None:
        borders = OxmlElement("w:tblBorders")
        tbl_pr.append(borders)
    for edge in ("top", "left", "bottom", "right", "insideH", "insideV"):
        node = borders.find(qn(f"w:{edge}"))
        if node is None:
            node = OxmlElement(f"w:{edge}")
            borders.append(node)
        node.set(qn("w:val"), "single")
        node.set(qn("w:sz"), size)
        node.set(qn("w:space"), "0")
        node.set(qn("w:color"), color)


def set_paragraph(paragraph, before=0, after=6, line=1.10, align=None, keep=False):
    fmt = paragraph.paragraph_format
    fmt.space_before = Pt(before)
    fmt.space_after = Pt(after)
    fmt.line_spacing = line
    if align is not None:
        paragraph.alignment = align
    if keep:
        fmt.keep_with_next = True


def add_field(run, instruction):
    begin = OxmlElement("w:fldChar")
    begin.set(qn("w:fldCharType"), "begin")
    instr = OxmlElement("w:instrText")
    instr.set(qn("xml:space"), "preserve")
    instr.text = instruction
    separate = OxmlElement("w:fldChar")
    separate.set(qn("w:fldCharType"), "separate")
    text = OxmlElement("w:t")
    text.text = "1"
    end = OxmlElement("w:fldChar")
    end.set(qn("w:fldCharType"), "end")
    run._r.extend([begin, instr, separate, text, end])


def add_header_footer(doc: Document, kind: str):
    section = doc.sections[0]
    header = section.header
    hp = header.paragraphs[0]
    hp.text = ""
    set_paragraph(hp, after=2, line=1.0)
    table = header.add_table(rows=1, cols=2, width=Inches(6.5))
    set_table_geometry(table, [6000, 3360])
    set_table_borders(table, color=WHITE, size="0")
    left, right = table.rows[0].cells
    left.paragraphs[0].text = ""
    r = left.paragraphs[0].add_run(f"{APP_NAME}  {VERSION}")
    set_run_font(r, size=8.5, color=GRAY, bold=True)
    right.paragraphs[0].alignment = WD_ALIGN_PARAGRAPH.RIGHT
    right.paragraphs[0].text = ""
    r = right.paragraphs[0].add_run(f"{kind}  |  第 ")
    set_run_font(r, size=8.5, color=GRAY)
    r = right.paragraphs[0].add_run()
    set_run_font(r, size=8.5, color=GRAY)
    add_field(r, "PAGE")
    r = right.paragraphs[0].add_run(" 页")
    set_run_font(r, size=8.5, color=GRAY)

    footer = section.footer
    fp = footer.paragraphs[0]
    fp.text = ""
    fp.alignment = WD_ALIGN_PARAGRAPH.CENTER
    set_paragraph(fp, before=2, after=0, line=1.0)
    r = fp.add_run("软件著作权登记辅助材料  ·  ")
    set_run_font(r, size=8, color=GRAY)
    r = fp.add_run("第 ")
    set_run_font(r, size=8, color=GRAY)
    r = fp.add_run()
    set_run_font(r, size=8, color=GRAY)
    add_field(r, "PAGE")
    r = fp.add_run(" 页")
    set_run_font(r, size=8, color=GRAY)


def configure_document(doc: Document, title: str, kind: str):
    section = doc.sections[0]
    section.page_width = Inches(8.5)
    section.page_height = Inches(11)
    section.top_margin = Inches(0.72)
    section.bottom_margin = Inches(0.70)
    section.left_margin = Inches(1.0)
    section.right_margin = Inches(1.0)
    section.header_distance = Inches(0.32)
    section.footer_distance = Inches(0.32)
    styles = doc.styles
    normal = styles["Normal"]
    normal.font.name = FONT
    normal._element.rPr.rFonts.set(qn("w:eastAsia"), FONT)
    normal.font.size = Pt(11)
    normal.font.color.rgb = RGBColor.from_string(BLACK)
    normal.paragraph_format.space_before = Pt(0)
    normal.paragraph_format.space_after = Pt(6)
    normal.paragraph_format.line_spacing = 1.10
    for name, size, color, before, after in (
        ("Heading 1", 16, BLUE, 16, 8),
        ("Heading 2", 13, BLUE, 12, 6),
        ("Heading 3", 12, NAVY, 8, 4),
    ):
        style = styles[name]
        style.font.name = FONT
        style._element.rPr.rFonts.set(qn("w:eastAsia"), FONT)
        style.font.size = Pt(size)
        style.font.bold = True
        style.font.color.rgb = RGBColor.from_string(color)
        style.paragraph_format.space_before = Pt(before)
        style.paragraph_format.space_after = Pt(after)
        style.paragraph_format.keep_with_next = True
    if "CodeLine" not in [s.name for s in styles]:
        code = styles.add_style("CodeLine", WD_STYLE_TYPE.PARAGRAPH)
    else:
        code = styles["CodeLine"]
    code.font.name = CODE_FONT
    code._element.rPr.rFonts.set(qn("w:ascii"), CODE_FONT)
    code._element.rPr.rFonts.set(qn("w:hAnsi"), CODE_FONT)
    code._element.rPr.rFonts.set(qn("w:eastAsia"), FONT)
    code.font.size = Pt(5.8)
    code.font.color.rgb = RGBColor.from_string("263238")
    code.paragraph_format.space_before = Pt(0)
    code.paragraph_format.space_after = Pt(0)
    code.paragraph_format.line_spacing = Pt(8.4)
    code.paragraph_format.keep_together = True
    doc.core_properties.title = title
    doc.core_properties.subject = kind
    doc.core_properties.author = ""
    doc.core_properties.comments = "根据当前项目源代码自动生成的软著申请辅助材料"
    add_header_footer(doc, kind)


def add_cover(doc: Document, title: str, subtitle: str, kind: str):
    p = doc.add_paragraph()
    set_paragraph(p, before=78, after=14, line=1.0, align=WD_ALIGN_PARAGRAPH.CENTER)
    r = p.add_run("软件著作权登记申请辅助材料")
    set_run_font(r, size=11, color=BLUE, bold=True)
    p = doc.add_paragraph()
    set_paragraph(p, after=8, line=1.0, align=WD_ALIGN_PARAGRAPH.CENTER)
    r = p.add_run(title)
    set_run_font(r, size=26, color=NAVY, bold=True)
    p = doc.add_paragraph()
    set_paragraph(p, after=26, line=1.0, align=WD_ALIGN_PARAGRAPH.CENTER)
    r = p.add_run(subtitle)
    set_run_font(r, size=14, color=GRAY)
    rows = [
        ("软件名称", APP_NAME),
        ("软件版本", VERSION),
        ("材料类型", kind),
        ("生成日期", TODAY),
        ("著作权人", "（请按实际申请主体填写）"),
    ]
    add_label_table(doc, rows, widths=[2600, 6760], header_fill=LIGHT_BLUE)
    add_callout(doc, "使用说明：本文档根据项目当前实现整理，登记时请结合实际开发完成日期、著作权人、权利取得方式及申请表信息进行核对。", fill="F8FAFC")
    doc.add_page_break()


def add_label_table(doc, rows: Iterable[tuple[str, str]], widths=(2700, 6660), header_fill=LIGHT_BLUE):
    rows = list(rows)
    table = doc.add_table(rows=len(rows), cols=2)
    set_table_geometry(table, widths)
    set_table_borders(table)
    for row, (label, value) in zip(table.rows, rows):
        shade(row.cells[0], header_fill)
        for idx, text in enumerate((label, value)):
            p = row.cells[idx].paragraphs[0]
            p.text = ""
            set_paragraph(p, after=0, line=1.05)
            r = p.add_run(text)
            set_run_font(r, size=10.5, color=BLACK, bold=(idx == 0))
    doc.add_paragraph().paragraph_format.space_after = Pt(2)
    return table


def add_matrix_table(doc, headers: Sequence[str], rows: Sequence[Sequence[str]], widths: Sequence[int]):
    table = doc.add_table(rows=1, cols=len(headers))
    set_table_geometry(table, widths, header=True)
    set_table_borders(table)
    for cell, text in zip(table.rows[0].cells, headers):
        shade(cell, LIGHT_BLUE)
        p = cell.paragraphs[0]
        p.text = ""
        set_paragraph(p, after=0, line=1.05, align=WD_ALIGN_PARAGRAPH.CENTER)
        r = p.add_run(text)
        set_run_font(r, size=10, color=NAVY, bold=True)
    for values in rows:
        cells = table.add_row().cells
        for cell, text in zip(cells, values):
            p = cell.paragraphs[0]
            p.text = ""
            set_paragraph(p, after=0, line=1.05)
            r = p.add_run(str(text))
            set_run_font(r, size=9.8, color=BLACK)
    set_table_geometry(table, widths, header=True)
    doc.add_paragraph().paragraph_format.space_after = Pt(2)
    return table


def add_callout(doc, text: str, fill=LIGHT_BLUE):
    table = doc.add_table(rows=1, cols=1)
    set_table_geometry(table, [PAGE_WIDTH_DXA])
    set_table_borders(table, color=fill, size="4")
    cell = table.cell(0, 0)
    shade(cell, fill)
    p = cell.paragraphs[0]
    p.text = ""
    set_paragraph(p, after=0, line=1.10)
    r = p.add_run(text)
    set_run_font(r, size=10.5, color=NAVY)
    doc.add_paragraph().paragraph_format.space_after = Pt(2)


def add_heading(doc, text, level=1):
    p = doc.add_paragraph(style=f"Heading {level}")
    p.add_run(text)
    return p


def add_body(doc, text, bold_lead=None):
    p = doc.add_paragraph()
    set_paragraph(p, after=6, line=1.10)
    if bold_lead and text.startswith(bold_lead):
        r = p.add_run(bold_lead)
        set_run_font(r, size=11, color=BLACK, bold=True)
        r = p.add_run(text[len(bold_lead):])
        set_run_font(r, size=11, color=BLACK)
    else:
        r = p.add_run(text)
        set_run_font(r, size=11, color=BLACK)
    return p


def add_bullet(doc, text, level=0):
    style = "List Bullet" if level == 0 else "List Bullet 2"
    p = doc.add_paragraph(style=style)
    set_paragraph(p, after=4, line=1.167)
    r = p.add_run(text)
    set_run_font(r, size=11, color=BLACK)
    return p


def add_number(doc, text):
    p = doc.add_paragraph(style="List Number")
    set_paragraph(p, after=5, line=1.10)
    r = p.add_run(text)
    set_run_font(r, size=11, color=BLACK)
    return p


def source_files() -> list[Path]:
    files = [p for p in (ROOT / "src").rglob("*") if p.is_file() and p.suffix.lower() in {".h", ".cpp", ".qrc"}]
    return sorted(files, key=lambda p: p.relative_to(ROOT).as_posix())


def source_lines() -> tuple[list[str], int, int]:
    lines: list[str] = []
    physical = 0
    nonblank = 0
    for path in source_files():
        raw = path.read_text(encoding="utf-8-sig").splitlines()
        physical += len(raw)
        nonblank += sum(1 for line in raw if line.strip())
        lines.append(f"// ===== source file: {path.relative_to(ROOT).as_posix()} =====")
        lines.extend(raw)
    return lines, physical, nonblank


def write_source_doc():
    lines, physical, nonblank = source_lines()
    page_size = 50
    selected = lines if len(lines) <= page_size * 60 else lines[: page_size * 30] + lines[-page_size * 30 :]
    while len(selected) < page_size * 60:
        selected.append("// [source listing ends]")
    selected = selected[: page_size * 60]
    doc = Document()
    configure_document(doc, f"{APP_NAME} 源程序鉴别材料", "源程序鉴别材料")
    for page in range(60):
        page_lines = selected[page * page_size : (page + 1) * page_size]
        for line in page_lines:
            p = doc.add_paragraph(style="CodeLine")
            p.paragraph_format.keep_together = True
            p.paragraph_format.widow_control = False
            r = p.add_run(line.expandtabs(4) if line else " ")
            set_run_font(r, name=CODE_FONT, size=5.8, color="263238")
        if page < 59:
            doc.add_page_break()
    path = OUT / f"{APP_NAME}_源程序鉴别材料_{VERSION}.docx"
    doc.save(path)
    return path, physical, nonblank, len(lines), len(selected)


def write_feature_doc(physical: int, nonblank: int):
    doc = Document()
    configure_document(doc, f"{APP_NAME} 功能与技术特点说明", "功能与技术特点说明")
    add_cover(doc, "功能与技术特点说明", "Software Function & Technical Characteristics", "文件材料文档")

    add_heading(doc, "一、软件基本信息", 1)
    add_body(doc, "本说明用于概括软件的开发目的、主要功能、运行环境、技术特点和质量控制情况，可作为软件著作权登记的文件材料。带有“请按实际填写”的信息，提交前应由申请人核对。")
    add_label_table(doc, [
        ("软件全称", APP_NAME),
        ("软件简称", "智能排监考"),
        ("软件版本", VERSION),
        ("软件类型", "教务考试管理类桌面应用软件"),
        ("开发目的", "将教师资源、考试场次与考场安排集中管理，依据约束条件自动生成可执行的监考方案，减少人工排班冲突并便于留痕追溯。"),
        ("面向领域", "教育信息化、学校教务管理、考试组织与监考安排"),
        ("编程语言", "C++17；Qt Widgets/Core/Gui；CMake；JSON/CSV 数据格式"),
        ("源程序规模", f"当前 src 目录源程序物理行数 {physical} 行，非空行 {nonblank} 行（由脚本按当前文件自动统计）"),
    ])

    add_heading(doc, "二、硬件与软件环境", 1)
    add_matrix_table(doc, ["项目", "开发环境建议", "运行环境建议"], [
        ("硬件环境", "64 位四核处理器；8GB 及以上内存；2GB 以上可用磁盘空间。", "64 位双核处理器；4GB 及以上内存；1GB 以上可用磁盘空间。"),
        ("软件环境", "Windows 10/11 64 位；Qt 6.8.x；CMake 3.16+；支持 C++17 的编译器或 Qt Creator。", "Windows 10/11 64 位；无需部署独立服务器或数据库；具备程序目录读写权限。"),
        ("网络条件", "开发阶段可使用本地构建环境；不依赖在线接口。", "单机离线运行即可；导出文件可通过常规文件方式传递。"),
    ], [1800, 3780, 3780])
    add_callout(doc, "环境说明中的硬件参数是申请材料的建议配置，不是对用户设备的强制限制；软件当前采用本地文件存储，不包含服务端部署要求。")

    add_heading(doc, "三、开发目的与业务对象", 1)
    add_body(doc, "传统考试排监考工作通常需要在教师名单、课程关系、考试时间、教室容量和教师特殊要求之间反复核对。人工排班容易出现同一教师时间冲突、教师在不可监考时段被安排、任课教师未被纳入相应课程以及监考负荷差异过大等问题。")
    add_body(doc, "本软件以教师、考试场次、排班方案和操作日志为核心业务对象，通过图形化界面完成数据维护，以可解释的约束检查和评分式排班算法生成方案，并允许管理员查看、应用、导出和留存历史方案。")

    add_heading(doc, "四、主要功能", 1)
    add_matrix_table(doc, ["功能模块", "主要内容", "输出或结果"], [
        ("登录与权限", "读取本地用户数据，校验密码哈希并识别管理员身份。", "进入主窗口；写入登录及操作日志。"),
        ("教师管理", "维护工号、姓名、职称、课程、不可监考日期/时间段及原因；记录监考次数。", "可用教师资源及约束数据。"),
        ("考试场次管理", "维护日期、时间段、考场、大小教室、考试类别、课程、任课教师和需求人数。", "结构化考试场次。"),
        ("自动排班", "先安排任课教师硬性任务，再按可用性、冲突和负荷评分补齐，最后尝试工作量平衡。", "排班方案及失败原因。"),
        ("方案管理", "保存方案、应用方案、更新备注、删除方案并保留审计信息。", "历史方案和当前生效安排。"),
        ("查询与导出", "按教师姓名查看其监考场次，导出 UTF-8 CSV 监考表。", "查询结果和可交换的 CSV 文件。"),
        ("日志审计", "记录操作时间、操作用户、动作和详情。", "operations.log 文本日志。"),
    ], [1800, 4560, 3000])

    add_heading(doc, "五、系统结构与模块组成", 1)
    add_body(doc, "系统采用 Qt Widgets 单机分层结构：界面层负责交互与展示，业务服务层负责数据持久化、认证和排班算法，模型层负责领域对象和格式转换，应用上下文负责统一组织运行时依赖。")
    add_matrix_table(doc, ["层次/模块", "职责", "对应目录或文件"], [
        ("应用入口", "创建 QApplication，设置软件名称、版本和界面风格。", "src/main.cpp；src/app/AppInfo.h"),
        ("应用上下文", "组织认证、数据、日志服务和运行目录。", "src/app/AppContext.*"),
        ("数据模型", "定义教师、考试、用户、方案、忙碌时段等对象及 JSON 转换。", "src/model/Models.*"),
        ("认证服务", "用户读取、密码哈希、登录校验和安全写入。", "src/services/AuthManager.*"),
        ("数据服务", "JSON 读写、字段校验、增删改查、引用关系检查。", "src/services/DataManager.*"),
        ("排班服务", "硬约束检查、贪心评分、冲突避免和平衡修正。", "src/services/Scheduler.*"),
        ("界面模块", "登录、教师、考试、排班、查询和日志界面。", "src/ui/**"),
    ], [2200, 4100, 3060])

    add_heading(doc, "六、技术特点", 1)
    for text in [
        "本地化桌面架构：不依赖独立后端服务和数据库，适合学校教务人员在单机环境快速部署。",
        "结构化数据持久化：使用 JSON 保存业务对象，模型类提供统一的 toJson/fromJson 转换。",
        "安全写入策略：关键数据采用 QSaveFile 临时文件提交方式，避免直接截断原文件造成半文件。",
        "严格输入校验：日期、时间段、考场、课程、教师引用和忙碌时段均在数据服务层再次检查。",
        "可解释排班：将任课教师安排、教师不可用时段、同场重复、时间冲突和工作量差异作为明确规则处理。",
        "方案可追溯：每个方案具有唯一编号、创建时间、创建人、备注和 assignments 映射，可应用或删除。",
        "导出兼容性：CSV 字段统一进行逗号、双引号和换行转义，降低 Excel 等工具打开时的列错位风险。",
        "审计留痕：增删改、登录、排班和方案操作均通过日志服务记录时间、操作人和详情。",
    ]:
        add_bullet(doc, text)

    add_heading(doc, "七、核心业务流程", 1)
    add_number(doc, "管理员启动软件并登录，应用上下文初始化数据目录、JSON 文件路径和日志服务。")
    add_number(doc, "在教师管理中录入教师基础信息、职称、授课课程和不可监考时段。")
    add_number(doc, "在考试场次管理中录入日期、时间段、教室、考试类别、课程、任课教师和监考人数。")
    add_number(doc, "点击自动排班，系统按时间顺序处理场次，先满足任课教师安排，再补充非教授监考员并进行平衡修正。")
    add_number(doc, "排班成功后保存方案，管理员可以应用方案更新当前考试安排和教师监考统计。")
    add_number(doc, "根据需要按教师查询结果或导出 CSV，最后通过日志与数据文件完成归档和备份。")

    add_heading(doc, "八、排班算法与约束说明", 1)
    add_body(doc, "排班器对考试场次按日期、开始时间和编号排序。第一轮处理普通考试中的任课教师硬性安排，同一教师同一课程在多班场景下尽量只安排一次；第二轮为每场补齐剩余监考员，过滤教授补充、同场重复、忙碌时段和时间冲突候选人，再按负荷、同日安排和平均目标计算分数；第三轮尝试在保持约束的前提下交换高负荷与低负荷教师，使非教授教师的工作量差值不超过 1。")
    add_matrix_table(doc, ["约束类型", "规则", "失败处理"], [
        ("硬约束", "教师不能在不可监考时段被安排；同一时间不能同时监考多场；同一场不重复。", "停止生成并返回可读的失败原因。"),
        ("任课教师", "普通考试至少按课程关系安排一次任课教师；教师不存在时禁止保存。", "提示缺少教师或无法安排的场次。"),
        ("资源约束", "每场达到 requiredInvigilators；补充监考员优先从非教授教师中选择。", "提示人手不足或限制过多。"),
        ("平衡约束", "尝试让非教授教师监考次数的最大值与最小值之差不超过 1。", "无法交换时返回平衡失败原因。"),
    ], [1800, 4860, 2700])

    add_heading(doc, "九、数据文件与信息安全", 1)
    add_matrix_table(doc, ["文件", "内容", "处理方式"], [
        ("users.json", "用户名、密码哈希、管理员标识。", "密码不以明文保存；读取和写入经过认证服务。"),
        ("teachers.json", "教师资料、忙碌时段、已分配次数。", "数据服务统一校验；保存采用原子提交。"),
        ("exams.json", "考试场次与当前已应用安排。", "校验日期、时间、人数、教师引用。"),
        ("schedules.json", "方案编号、创建信息、备注、分配关系。", "支持方案保存、应用、备注更新和删除。"),
        ("operations.log", "操作时间、操作人、动作和详情。", "追加写入，便于审计与故障排查。"),
    ], [1900, 4100, 3360])
    add_callout(doc, "正式部署建议：将 data 与 logs 纳入定期备份范围，不要在公共电脑上继续使用默认 admin/admin 账号；软件登记材料中的申请主体和权利信息需与实际情况一致。", fill="FFF8E8")

    add_heading(doc, "十、质量控制与软著材料适配", 1)
    add_matrix_table(doc, ["检查项", "本项目处理方式", "材料体现"], [
        ("名称与版本", "集中定义 appinfo::name/version/title，界面标题和 QApplication 元数据统一使用。", "全部 Word 文档统一使用 APP_NAME 与 V1.0。"),
        ("源程序完整性", "按 src 目录的 .h/.cpp/.qrc 文件排序提取，物理行数由生成脚本实时统计。", "源程序鉴别材料固定为 60 个代码页，每页 50 行。"),
        ("文档一致性", "功能说明、技术特点和用户操作步骤均依据当前 UI、服务和数据文件命名撰写。", "功能与技术特点说明、用户操作手册。"),
        ("可验证性", "CMake 集成 models_contract_test，覆盖模型、数据校验和引用删除保护。", "开发验证记录与测试说明。"),
    ], [1900, 4460, 3000])

    add_heading(doc, "十一、开发与验证记录", 1)
    add_body(doc, "项目使用 CMake 管理构建。已加入 CTest 测试目标 models_contract_test，覆盖软件名称和版本、日期/时间段格式、CSV 转义、教师忙碌时段校验、考试场次合法性、教师引用检查及被引用教师删除保护。登记前建议在目标电脑重新配置并执行 ctest，以确认材料与提交版本一致。")
    add_body(doc, "本说明不替代软件著作权登记申请表。申请人仍需按登记机关要求准备身份证明、权属证明或其他必要文件，并对软件实际版本、完成日期、开发方式和权利归属负责。")

    path = OUT / f"{APP_NAME}_功能与技术特点说明_{VERSION}.docx"
    doc.save(path)
    return path


def write_manual():
    doc = Document()
    configure_document(doc, f"{APP_NAME} 用户操作手册", "用户操作手册")
    add_cover(doc, "用户操作手册", "User Operation Manual", "用户手册")

    add_heading(doc, "一、手册说明", 1)
    add_body(doc, f"本手册适用于 {APP_NAME} {VERSION}，面向教务管理员、考试组织人员和负责维护排班数据的工作人员。软件为 Windows 桌面应用，主要在本地完成数据维护、自动排班、方案应用、查询和导出。")
    add_label_table(doc, [
        ("适用版本", VERSION),
        ("运行方式", "双击应用程序或从 Qt Creator 运行；单机离线使用"),
        ("默认账号", "admin"),
        ("默认密码", "admin（首次启动自动创建，仅建议用于开发/演示）"),
        ("数据目录", "程序当前工作目录下的 data/"),
        ("日志目录", "程序当前工作目录下的 logs/"),
    ])
    add_callout(doc, "重要提示：第一次登录后请避免把默认账号用于正式环境。操作前请备份 data/ 与 logs/，尤其是在批量录入考试场次或应用新排班方案之前。", fill="FFF8E8")

    add_heading(doc, "二、运行准备", 1)
    add_body(doc, "建议运行环境为 Windows 10/11 64 位、4GB 及以上内存和 1GB 以上可用磁盘空间。程序不要求独立服务器或数据库，但需要对程序当前工作目录具有读写权限。")
    add_matrix_table(doc, ["准备项", "检查内容", "处理建议"], [
        ("程序文件", "确认可执行文件、Qt 运行库和 assets 目录完整。", "建议整体复制发布目录，不要单独移动 exe。"),
        ("目录权限", "程序能创建 data/ 和 logs/。", "将程序放在用户有写权限的目录，避免只读系统目录。"),
        ("备份", "已有 JSON 或日志是否需要保留。", "启动前复制 data/ 与 logs/ 到备份目录。"),
        ("基础数据", "教师、课程、考试和考场信息是否齐全。", "先录入教师，再录入考试场次。"),
    ], [1900, 3900, 3560])

    add_heading(doc, "三、启动与登录", 1)
    add_number(doc, "启动程序。首次运行时，系统初始化 data/ 和 logs/ 目录，并在不存在用户文件时创建管理员账号。")
    add_number(doc, "在登录窗口输入用户名和密码，点击登录。密码由认证服务进行哈希校验，错误时会提示登录失败。")
    add_number(doc, "登录成功后进入主窗口。标题栏显示“智能排监考管理系统 V1.0”，主界面提供教师、考试场次、排班、查询和日志等功能入口。")
    add_body(doc, "如果登录失败，请确认账号拼写、密码大小写和 users.json 是否存在有效内容。不要直接编辑密码字段；若数据文件被手工改坏，应先恢复备份。")

    add_heading(doc, "四、教师信息管理", 1)
    add_heading(doc, "4.1 新增教师", 2)
    for text in [
        "打开“教师管理”，点击新增或相应操作按钮。",
        "填写工号、姓名、职称和授课课程；工号和姓名不能为空。",
        "如教师存在不能监考的时段，添加日期和时间段，并填写原因。日期格式为 YYYY-MM-DD，时间段格式为 HH:MM-HH:MM。",
        "保存。系统会检查忙碌时段是否合法、是否重复，并记录操作日志。",
    ]:
        add_number(doc, text)
    add_heading(doc, "4.2 编辑与删除教师", 2)
    add_body(doc, "选中教师后可编辑资料。若教师已经被考试场次作为任课教师引用，或已出现在排班方案分配关系中，系统会阻止删除，以避免产生悬空引用；应先调整相关考试或方案。")
    add_matrix_table(doc, ["字段", "填写要求", "示例"], [
        ("工号", "唯一、非空，建议使用学校统一工号。", "T001"),
        ("姓名", "非空，用于查询和界面显示。", "张老师"),
        ("职称", "选择教授、副教授、讲师或助教。", "讲师"),
        ("课程", "可录入多门课程，用于识别任课关系。", "高等数学；线性代数"),
        ("不可监考时段", "日期与时间段组合，避免重复。", "2026-09-01|09:00-11:00"),
    ], [1900, 4380, 3080])

    add_heading(doc, "五、考试场次与考场管理", 1)
    add_number(doc, "打开“考试场次”管理界面，点击新增。")
    add_number(doc, "输入日期、时间段和教室名称，选择大教室或小教室。")
    add_number(doc, "选择考试类别。普通考试需要填写课程和任课教师；巡考类场次按界面要求填写。")
    add_number(doc, "填写每场所需监考人数，确认人数大于 0 后保存。系统会自动生成或校验考场编号，并检查教师引用是否存在。")
    add_body(doc, "时间段必须满足开始时间早于结束时间，例如 09:00-11:00。系统允许对常见全角标点和波浪号做规范化，但建议在录入时直接使用半角格式。")
    add_callout(doc, "排班前请先检查同一时间是否存在多个场次、是否有足够非教授教师、任课教师是否已经录入，以及忙碌时段是否与考试时间冲突。", fill="E8F3EE")

    add_heading(doc, "六、自动排班", 1)
    add_body(doc, "排班页面提供“开始自动排班并保存方案”按钮。系统按考试场次排序，先满足普通考试的任课教师安排，再从可用教师中选择补充监考员，最后进行工作量平衡修正。")
    add_heading(doc, "6.1 生成方案", 2)
    for text in [
        "完成教师和考试场次录入后，进入“排班”页面。",
        "可在备注框输入本次方案说明，例如“期末考试第一版”。",
        "点击“开始自动排班并保存方案”。成功时会生成唯一方案编号并自动应用到运行数据。",
        "失败时阅读页面给出的原因，针对人手不足、忙碌时段、时间冲突或教师引用问题调整数据后重试。",
    ]:
        add_number(doc, text)
    add_heading(doc, "6.2 排班规则速查", 2)
    add_matrix_table(doc, ["规则", "系统行为"], [
        ("任课教师", "普通考试按课程关系安排任课教师；同一教师同一课程多班场景尽量只安排一次。"),
        ("不可用时段", "教师在忙碌时段不会被选入对应考试。"),
        ("时间冲突", "同一教师在同一时间不能监考两场，也不能在同一场重复出现。"),
        ("教授安排", "教授承担任课教师的硬性安排，不参与普通补充监考。"),
        ("工作量", "补充监考采用负荷评分，之后尝试把非教授教师工作量差值控制在 1 以内。"),
    ], [2100, 7260])

    add_heading(doc, "七、方案管理", 1)
    add_number(doc, "在方案下拉框中选择要查看的历史方案，表格会显示每个场次及已安排教师工号。")
    add_number(doc, "点击“应用选中方案”，系统会更新考试场次中的安排，并重新计算教师监考次数与查询结果。")
    add_number(doc, "点击“更新备注”可以补充或修改方案说明。")
    add_number(doc, "删除方案前输入删除原因。删除只移除历史方案记录，不建议在未确认当前安排前随意操作。")
    add_body(doc, "应用方案失败时，先查看红色错误提示并检查 data/ 目录权限或文件状态。应用成功后建议立即导出 CSV 并备份 JSON 文件。")

    add_heading(doc, "八、查询与导出", 1)
    add_heading(doc, "8.1 按教师查询", 2)
    add_body(doc, "在查询页面输入教师姓名，系统读取当前已应用安排，显示该教师的全部监考场次。若结果为空，请确认是否已应用方案、姓名是否完全匹配以及考试场次是否已经保存。")
    add_heading(doc, "8.2 导出监考表", 2)
    add_number(doc, "在排班页面选择方案，点击“导出 CSV”。")
    add_number(doc, "选择保存位置，建议使用包含日期和方案编号的文件名。")
    add_number(doc, "使用 Excel 或 WPS 打开时选择 UTF-8 编码；系统已对逗号、双引号和换行做 CSV 转义。")
    add_matrix_table(doc, ["CSV 字段", "含义"], [
        ("planId / createdTime", "方案编号和方案创建时间。"),
        ("examId / date / timeSlot", "考场编号、考试日期和时间段。"),
        ("room / course / category", "教室、课程和考试类别。"),
        ("required / teacherIds", "所需人数及已安排教师工号列表。"),
    ], [2600, 6760])

    add_heading(doc, "九、日志、数据备份与恢复", 1)
    add_body(doc, "日志页面可查看系统记录的操作事件。日志内容包含时间、操作账号、动作和详情，适合在排班失败、数据误改或方案切换后进行追踪。")
    add_body(doc, "备份时关闭程序，复制 data/ 与 logs/ 到安全位置。恢复时先关闭程序，再用备份文件覆盖对应目录，重新启动后检查教师、考试和方案数量。不要在程序运行时直接覆盖 JSON 文件。")
    add_matrix_table(doc, ["现象", "建议处理"], [
        ("登录失败", "确认账号密码；检查 users.json 是否为空或格式损坏；必要时恢复备份。"),
        ("排班失败", "增加可用教师，减少冲突和忙碌时段，修正课程任课教师引用，或降低单场监考人数。"),
        ("无法删除教师", "检查其是否被考试场次或历史方案引用，先解除引用或删除无效方案。"),
        ("导出文件列错位", "用 UTF-8 编码打开；检查接收工具是否识别 CSV 引号转义。"),
        ("启动后数据为空", "确认当前工作目录是否正确，检查 data 文件是否存在并恢复有效备份。"),
    ], [2400, 6960])

    add_heading(doc, "十、登记材料信息速查", 1)
    add_body(doc, "下表对应常见软件著作权材料中的环境、语言、源程序数量和功能技术特点字段，便于从本手册快速核对。")
    add_label_table(doc, [
        ("硬件环境", "开发建议：64 位四核处理器、8GB 内存、2GB 可用磁盘；运行建议：64 位双核处理器、4GB 内存、1GB 可用磁盘。"),
        ("软件环境", "Windows 10/11 64 位；Qt 6.8.x；CMake 3.16+；C++17 编译器；运行时无需独立服务器和数据库。"),
        ("编程语言", "C++17、Qt Widgets/Core/Gui，使用 JSON 保存数据、CSV 导出结果。"),
        ("主要功能", "用户登录、教师管理、考试场次管理、自动排班、方案应用与查询、CSV 导出、操作日志。"),
        ("技术特点", "本地化部署、JSON 数据模型、原子写入、严格校验、约束排班、方案追溯、CSV 转义和日志审计。"),
    ])

    add_heading(doc, "十一、日常操作检查表", 1)
    for text in [
        "□ 操作前已备份 data/ 与 logs/。",
        "□ 教师工号、姓名、职称和授课课程已核对。",
        "□ 不可监考时段使用正确的日期和时间格式。",
        "□ 考试场次的日期、时间、教室、课程、任课教师和需求人数已核对。",
        "□ 自动排班成功后已查看方案明细和失败/成功提示。",
        "□ 应用方案后已按教师查询并抽查时间冲突。",
        "□ 已导出 CSV，并将方案编号、导出文件和日志一并归档。",
    ]:
        add_bullet(doc, text)
    add_callout(doc, "如需提交软件著作权登记，请将本手册与源程序鉴别材料、功能与技术特点说明以及实际申请表信息一并核对；申请主体、完成日期和权利归属不能由程序自动推定。", fill="F8FAFC")

    path = OUT / f"{APP_NAME}_用户操作手册_{VERSION}.docx"
    doc.save(path)
    return path


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    source_path, physical, nonblank, listing_count, selected_count = write_source_doc()
    feature_path = write_feature_doc(physical, nonblank)
    manual_path = write_manual()
    print(f"source={source_path}")
    print(f"feature={feature_path}")
    print(f"manual={manual_path}")
    print(f"source_files={len(source_files())} physical_lines={physical} nonblank_lines={nonblank} listing_lines={listing_count} selected_lines={selected_count}")


if __name__ == "__main__":
    main()
