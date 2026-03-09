import csv
import json
import collections
import os
import glob

# 설정
# 스크립트 위치: Client/Bin/Resources/Data/ConvertResource.py
BASE_DIR = os.path.dirname(os.path.abspath(__file__))

# 입력 폴더 (Data/csv)
CSV_DIR = os.path.join(BASE_DIR, 'csv')

# 출력 폴더 (Data/json)
JSON_DIR = os.path.join(BASE_DIR, 'json')

def convert_all_csv_to_json():
    if not os.path.exists(CSV_DIR):
        print(f"Error: CSV directory not found: {CSV_DIR}")
        return

    # 출력 폴더 생성
    if not os.path.exists(JSON_DIR):
        os.makedirs(JSON_DIR)
        print(f"Created directory: {JSON_DIR}")
    
    # csv 폴더 내의 모든 .csv 파일 찾기
    csv_files = glob.glob(os.path.join(CSV_DIR, '*.csv'))
    
    if not csv_files:
        print(f"Warning: No CSV files found in {CSV_DIR}")
        return

    print(f"Found {len(csv_files)} CSV files.")

    for csv_file in csv_files:
        try:
            filename = os.path.basename(csv_file)
            
            # JSON 파일명 생성 (csv 확장자 -> json)
            json_filename = os.path.splitext(filename)[0] + '.json'
            output_json_path = os.path.join(JSON_DIR, json_filename)
            
            # 변경점 확인 (수정 시간 비교)
            if os.path.exists(output_json_path):
                csv_mtime = os.path.getmtime(csv_file)
                json_mtime = os.path.getmtime(output_json_path)
                if csv_mtime <= json_mtime:
                    print(f"Skipping: {filename} (Up to date)")
                    continue

            print(f"Processing: {filename}...")
            
            # 파일별 데이터 저장소 (Type -> List of items)
            file_data = collections.defaultdict(list)
            
            with open(csv_file, 'r', encoding='utf-8-sig') as f:
                reader = csv.DictReader(f)
                
                # 빈 파일이거나 헤더가 없는 경우 처리
                if not reader.fieldnames:
                    print(f"  -> Skipped (Empty or No Header)")
                    continue

                for row in reader:
                    # Type이 없으면 스킵
                    res_type = row.get('Type', '').strip()
                    if not res_type: continue 

                    item = {
                        'id': row.get('Id', '').strip(),
                        'level': row.get('Level', '').strip()
                    }
                    
                    # Path가 있으면 추가 (리소스용)
                    if 'Path' in row and row['Path'].strip():
                        item['path'] = row['Path'].strip()
                    
                    # Count 처리 (빈칸이면 1)
                    count_str = row.get('Count', '1').strip()
                    item['count'] = int(count_str) if count_str else 1

                    # ModelType 처리 (Skeletal / Static)
                    model_type = row.get('ModelType', '').strip()
                    if model_type:
                        item['modelType'] = model_type

                    # Extra 필드 처리 (Model 타입 등)
                    extra = row.get('Extra', '').strip()
                    if extra:
                        item['type'] = extra

                    file_data[res_type].append(item)
            
            # JSON 파일명 생성 (csv 확장자 -> json)
            json_filename = os.path.splitext(filename)[0] + '.json'
            output_json_path = os.path.join(JSON_DIR, json_filename)
            
            # 각 CSV별로 JSON 저장
            with open(output_json_path, 'w', encoding='utf-8') as f:
                json.dump(file_data, f, indent=4, ensure_ascii=False)
                
            print(f"  -> Saved to {json_filename}")
                    
        except Exception as e:
            print(f"Error processing {os.path.basename(csv_file)}: {e}")

    print(f"Conversion Complete!")

if __name__ == "__main__":
    convert_all_csv_to_json()
