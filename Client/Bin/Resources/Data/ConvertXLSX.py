import pandas as pd
import json
import os
import glob
from collections import defaultdict

# ----------------- 설정 -----------------
BASE_DIR = os.path.dirname(os.path.abspath(__file__))
# 기획자가 액셀 원본을 넣을 폴더. 기존 'csv' 대신 'xlsx' 라는 폴더를 하나 새로 파시는걸 추천!
XLSX_DIR = os.path.join(BASE_DIR, 'xlsx') 
JSON_DIR = os.path.join(BASE_DIR, 'json')
# ----------------------------------------

def convert_all_xlsx_to_json():
    if not os.path.exists(XLSX_DIR):
        print(f"Error: 엑셀(xlsx) 폴더를 찾을 수 없습니다: {XLSX_DIR}")
        return

    os.makedirs(JSON_DIR, exist_ok=True)
    
    # xlsx 폴더 내의 모든 .xlsx 엑셀 파일 찾기
    xlsx_files = glob.glob(os.path.join(XLSX_DIR, '*.xlsx'))
    
    if not xlsx_files:
        print(f"Warning: {XLSX_DIR} 에 엑셀 파일이 없습니다.")
        return

    print(f"총 {len(xlsx_files)} 개의 엑셀 파일을 찾았습니다.")

    for xlsx_file in xlsx_files:
        filename = os.path.basename(xlsx_file)
        
        # 엑셀 열려있을 때 생기는 임시파일(~$...)은 씹고 넘어감
        if filename.startswith('~$'): 
            continue 

        print(f"변환 시작: {filename}...")
        
        try:
            # 이 한 줄이 핵심! 엑셀 안의 '모든 시트'를 딕셔너리로 다 가져옴
            # all_sheets = { 'ModelTable': DataFrame, 'ModelTable2': DataFrame, ... }
            all_sheets = pd.read_excel(xlsx_file, sheet_name=None, engine='openpyxl')
            
            # JSON으로 저장할 최상위 딕셔너리
            file_data = defaultdict(list)
            
            # 각 엑셀 파일 안의 탭(시트명)을 돌면서 데이터 파싱!
            for sheet_name, df in all_sheets.items():
                # 빈 시트면 터지니까 스킵
                if df.empty: continue
                
                # 빈 칸(NaN)은 빈 문자열('')로 채우고, 파이썬 리스트/딕셔너리로 1차 가공
                sheet_list = df.dropna(how='all').fillna('').to_dict(orient='records')
                
                # C++ 로더가 기대하는(ResourceLoader.cpp) json key 값 구조로 맞춤
                for row in sheet_list:
                    res_type = str(row.get('Type', '')).strip()
                    
                    # Id 없으면 빈 줄이거나 쓰레기값이니 스킵
                    if not str(row.get('Id', '')).strip(): 
                        continue 
                        
                    item = {
                        'id': str(row.get('Id', '')).strip(),
                        'level': str(row.get('Level', '')).strip(),
                        'path': str(row.get('Path', '')).strip()
                    }
                    
                    # 빈 값이 아니면 값을 넣고, 빈 값이면 기본값으로 처리
                    count_val = str(row.get('Count', '')).strip()
                    item['count'] = int(count_val) if count_val else 1
                    
                    model_type = str(row.get('ModelType', '')).strip()
                    if model_type: item['modelType'] = model_type
                        
                    extra_type = str(row.get('Extra', '')).strip()
                    if extra_type: item['type'] = extra_type
                    
                    # -----------------------------------------------------
                    # TODO: Skill 데이터가 시트 안에 섞여있다면 추가 파싱 작성
                    # -----------------------------------------------------
                    
                    # 🚀 대망의 저장! [시트명] 이라는 방에 아이템들을 쌓습니다.
                    file_data[sheet_name].append(item)
            
            # .xlsx 확장자 떼고 .json 으로 바꿔서 저장!
            json_filename = os.path.splitext(filename)[0] + '.json'
            output_json_path = os.path.join(JSON_DIR, json_filename)
            
            with open(output_json_path, 'w', encoding='utf-8') as f:
                json.dump(file_data, f, indent=4, ensure_ascii=False)
                
            print(f"  -> 성공: {json_filename} 안에 여러 시트 정보 저장 완료!")
                    
        except Exception as e:
            print(f"에러 발생 [{filename}]: {e}")

    print(f"모든 변환 작업 완료!")

if __name__ == "__main__":
    convert_all_xlsx_to_json()
