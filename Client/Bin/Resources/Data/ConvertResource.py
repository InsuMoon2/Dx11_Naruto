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

# 출력 파일 (Data/json/ResourceTable.json)
OUTPUT_JSON = os.path.join(BASE_DIR, 'json', 'ResourceTable.json')

def convert_all_csv_to_json():
    if not os.path.exists(CSV_DIR):
        print(f"Error: CSV directory not found: {CSV_DIR}")
        return

    # 모든 리소스를 담을 딕셔너리 (Type -> List of items)
    all_data = collections.defaultdict(list)
    
    # csv 폴더 내의 모든 .csv 파일 찾기
    csv_files = glob.glob(os.path.join(CSV_DIR, '*.csv'))
    
    if not csv_files:
        print(f"Warning: No CSV files found in {CSV_DIR}")
        return

    print(f"🔍 Found {len(csv_files)} CSV files.")

    for csv_file in csv_files:
        try:
            print(f"Processing: {os.path.basename(csv_file)}...")
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
                        'key': row.get('Key', '').strip(),
                        'path': row.get('Path', '').strip(),
                        'level': row.get('Level', '').strip()
                    }
                    
                    # Count 처리 (빈칸이면 1)
                    count_str = row.get('Count', '1').strip()
                    item['count'] = int(count_str) if count_str else 1

                    # 추가 속성 (Model 타입 등)
                    if res_type == 'Model':
                        item['type'] = row.get('Extra', 'NonAnim').strip()

                    all_data[res_type].append(item)
                    
        except Exception as e:
            print(f"❌ Error processing {os.path.basename(csv_file)}: {e}")

    # JSON 저장
    try:
        # 출력 폴더가 없으면 생성
        output_dir = os.path.dirname(OUTPUT_JSON)
        if output_dir and not os.path.exists(output_dir):
            os.makedirs(output_dir)
            print(f"📁 Created directory: {output_dir}")

        with open(OUTPUT_JSON, 'w', encoding='utf-8') as f:
            json.dump(all_data, f, indent=4, ensure_ascii=False)
        
        print(f"✅ Conversion Complete! All data merged into -> {OUTPUT_JSON}")

    except Exception as e:
        print(f"❌ Error writing JSON: {e}")

if __name__ == "__main__":
    convert_all_csv_to_json()
