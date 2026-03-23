
import zipfile, xml.etree.ElementTree as ET, os
base_path = r'c:\Users\moon\Desktop\Jusin\GitDesktop\Dx11_Naruto\Client\Bin\Resources\Data\xlsx'
for f in ['DT_Texture.xlsx', 'DT_Shader.xlsx']:
    try:
        with zipfile.ZipFile(os.path.join(base_path, f), 'r') as z:
            print('\n--- ' + f + ' ---')
            wks = ET.fromstring(z.read('xl/workbook.xml'))
            sheets = [s.attrib['name'] for s in wks.findall('.//{http://schemas.openxmlformats.org/spreadsheetml/2006/main}sheet')]
            print('Sheets:', sheets)
            strings = []
            try:
                ss = ET.fromstring(z.read('xl/sharedStrings.xml'))
                strings = [t.text for t in ss.iter('{http://schemas.openxmlformats.org/spreadsheetml/2006/main}t')]
            except: pass
            for i, s in enumerate(sheets):
                try:
                    ws = ET.fromstring(z.read('xl/worksheets/sheet'+str(i+1)+'.xml'))
                    row = ws.find('.//{http://schemas.openxmlformats.org/spreadsheetml/2006/main}row')
                    if row is not None:
                        cols = [strings[int(c.find('{http://schemas.openxmlformats.org/spreadsheetml/2006/main}v').text)] if c.attrib.get('t') == 's' else c.find('{http://schemas.openxmlformats.org/spreadsheetml/2006/main}v').text for c in row.findall('{http://schemas.openxmlformats.org/spreadsheetml/2006/main}c') if c.find('{http://schemas.openxmlformats.org/spreadsheetml/2006/main}v') is not None]
                        print('Sheet:', s, 'Columns:', cols)
                except Exception as ex: print('err', ex)
    except Exception as e: print(e)

