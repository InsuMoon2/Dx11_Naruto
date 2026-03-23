import pandas as pd
import json
import os
import glob
from collections import defaultdict

# ----------------- 설정 -----------------
BASE_DIR = os.path.dirname(os.path.abspath(__file__))
XLSX_DIR = os.path.join(BASE_DIR, 'xlsx') 
JSON_DIR = os.path.join(BASE_DIR, 'json')
# ----------------------------------------

def convert_all_xlsx_to_json():
    if not os.path.exists(XLSX_DIR):
        print(f"Error: 엑셀(xlsx) 폴더를 찾을 수 없습니다: {XLSX_DIR}")
        return

    os.makedirs(JSON_DIR, exist_ok=True)
    
    xlsx_files = glob.glob(os.path.join(XLSX_DIR, '*.xlsx'))
    
    if not xlsx_files:
        print(f"Warning: {XLSX_DIR} 에 엑셀 파일이 없습니다.")
        return

    print(f"총 {len(xlsx_files)} 개의 엑셀 파일을 찾았습니다.")

    for xlsx_file in xlsx_files:
        filename = os.path.basename(xlsx_file)
        
        if filename.startswith('~$'): 
            continue 

        print(f"변환 시작: {filename}...")
        
        try:
            all_sheets = pd.read_excel(xlsx_file, sheet_name=None, engine='openpyxl')
            
            group_name = "Unknown"
            if "Texture" in filename: group_name = "Texture"
            elif "Shader" in filename: group_name = "Shader"
            elif "Model" in filename: group_name = "Model"
            elif "Terrain" in filename: group_name = "Terrain"
            elif "Skill" in filename: group_name = "Skill"
            else: group_name = os.path.splitext(filename)[0]

            file_data = {group_name: []}
            
            for sheet_name, df in all_sheets.items():
                if df.empty: continue
                
                # 기획자용 메모나 주석 컬럼은 JSON 변환에서 제외 (용량 최적화)
                if 'Memo' in df.columns:
                    df = df.drop(columns=['Memo'])
                if '주석' in df.columns:
                    df = df.drop(columns=['주석'])
                
                sheet_list = df.dropna(how='all').fillna('').to_dict(orient='records')
                
                for row in sheet_list:
                    if 'Id' in row and not str(row.get('Id', '')).strip():
                        continue
                    if 'SkillID' in row and not str(row.get('SkillID', '')).strip():
                        continue
                        
                    item = {}
                    for col_name, val in row.items():
                        if val != '' and not str(col_name).startswith('Unnamed'):
                            
                            # [추가된 안전장치] Pandas/Numpy 특수 숫자를 파이썬 기본 숫자로 변환!
                            if hasattr(val, 'item'):
                                val = val.item()
                                
                            item[col_name] = val
                    
                    if item:
                        file_data[group_name].append(item)
            
            json_filename = os.path.splitext(filename)[0] + '.json'
            output_json_path = os.path.join(JSON_DIR, json_filename)
            
            with open(output_json_path, 'w', encoding='utf-8') as f:
                json.dump(file_data, f, indent=4, ensure_ascii=False)
                
            print(f"  -> 성공: 여러 시트의 정보를 '{group_name}' 키 하나로 병합하여 {json_filename} 에 저장 완료!")
                    
        except Exception as e:
            print(f"에러 발생 [{filename}]: {e}")

    print(f"모든 변환 작업 완료!")

if __name__ == "__main__":
    convert_all_xlsx_to_json()
