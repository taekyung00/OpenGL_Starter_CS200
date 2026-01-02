from gitingest import ingest
import sys

def main():
    # 1. 사용자로부터 파일명 Prefix 입력 받기
    print("==========================================")
    print("   GitIngest 3-Split Auto Generator")
    print("==========================================")
    prefix = input("👉 생성할 파일의 이름을 입력하세요 (예: week7): ").strip()

    if not prefix:
        print("❌ 이름이 입력되지 않았습니다. 프로그램을 종료합니다.")
        return

    print(f"\n🚀 '{prefix}' 프로젝트 분석 시작...\n")

    # ---------------------------------------------------------
    # PART 1: Structure Only (*-structure.txt)
    # ---------------------------------------------------------
    print(f"1️⃣  [Structure] 구조 파일 생성 중...")
    try:
        # 전체를 스캔하되 content는 버리고 summary(s)와 tree(t)만 저장
        s, t, _ = ingest(".") 
        filename = f"{prefix}-structure.txt"
        with open(filename, "w", encoding="utf-8") as f:
            f.write(s + "\n" + t)
        print(f"   ✅ {filename} 완료")
    except Exception as e:
        print(f"   ❌ 오류 발생: {e}")

    # ---------------------------------------------------------
    # PART 2: Settings & Resources (*-setting.txt)
    # ---------------------------------------------------------
    print(f"2️⃣  [Settings] 설정 및 리소스 파일 생성 중...")
    try:
        # 설정 파일 및 쉐이더, 문서 관련 패턴 정의
        setting_patterns = [
            "CMake*", "*.json", "*.md", ".clang-format",
            "*.cmake", "cmake/*",
            "*.vert", "*.frag", "*.html"
        ]
        s, t, c = ingest(".", include_patterns=setting_patterns)
        filename = f"{prefix}-setting.txt"
        with open(filename, "w", encoding="utf-8") as f:
            f.write(f"{s}\n\n{t}\n\n{c}")
        print(f"   ✅ {filename} 완료")
    except Exception as e:
        print(f"   ❌ 오류 발생: {e}")

    # ---------------------------------------------------------
    # PART 3: Source Code (*-source.txt)
    # ---------------------------------------------------------
    print(f"3️⃣  [Source] 핵심 소스 코드 생성 중...")
    try:
        # 소스 코드 관련 패턴 정의 (필요시 *.h 추가 가능)
        source_patterns = [
            "source/*.cpp", "source/*.hpp", "source/*.h",
            "*.cpp", "*.hpp", "*.h" # 혹시 루트에 있는 소스도 포함하려면 유지
        ]
        # 설정 파일들은 소스에서 제외 (중복 방지)
        exclude_settings = ["CMake*", "cmake/*", "*.json", "*.md"]
        
        s, t, c = ingest(".", include_patterns=source_patterns, exclude_patterns=exclude_settings)
        filename = f"{prefix}-source.txt"
        with open(filename, "w", encoding="utf-8") as f:
            f.write(f"{s}\n\n{t}\n\n{c}")
        print(f"   ✅ {filename} 완료")
    except Exception as e:
        print(f"   ❌ 오류 발생: {e}")

    print("\n✨ 모든 작업이 완료되었습니다!")

if __name__ == "__main__":
    main()