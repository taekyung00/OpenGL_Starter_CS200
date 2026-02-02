### 🏫 CS200 vs. 🎮 GBA: 스프라이트 렌더링 비교

`week4.txt`의 `demo.cpp`와 쉐이더 코드를 기반으로 분석했습니다.

#### 1. 이미지 로딩 (Image Loading)

- **CS200 (PC):** `stbi_load` 함수로 `Cat.png` 파일(PNG)을 런타임에 읽어서 RAM에 압축을 풀고, `glTexImage2D`로 GPU에 전송합니다.

- **GBA (Console):** 파일 시스템(OS)이 없습니다. PNG를 읽을 수 없습니다.
  
  - **해결책:** PC에서 미리 PNG를 **"타일 데이터(C 배열)"** 로 변환한 뒤, 코드에 박아넣고 컴파일합니다. (PC의 `stbi_load` 과정을 컴파일 전에 미리 하는 셈입니다.)

#### 2. UV 좌표 (TexCoord)

- **CS200 (PC):** `0.0 ~ 1.0` 사이의 실수(float) 좌표를 씁니다.
  
  - 고양이 얼굴의 특정 부위를 자르기 위해 `uTexCoordTransform` 행렬로 UV를 조작합니다.

- **GBA (Console):** **타일 인덱스(Integer)** 를 씁니다.
  
  - GBA는 모든 이미지를 8x8 조각(타일)으로 쪼개서 관리합니다.
  
  - "좌표 0.5에서 0.1만큼 잘라라" 대신 **"5번 타일(Tile ID 5)을 써라"** 라고 정수로 명령합니다.

#### 3. 투명도 처리 (Transparency)

- **CS200 (PC):** 프래그먼트 쉐이더에서 알파값이 0이면 `discard` 합니다.

- **GBA (Console):** **Color Keying (Index 0)** 방식을 씁니다.
  
  - 팔레트의 **0번 색상**은 무조건 투명으로 처리됩니다. (쉐이더 없이 하드웨어 레벨에서 구멍이 뚫림)

#### 4. 위치 이동 (Transformation)

- **CS200 (PC):** `uModel` 행렬을 유니폼으로 넘겨서 버텍스 쉐이더가 곱셈을 합니다.

- **GBA (Console):** **OAM(Object Attribute Memory)** 에 값을 씁니다.
  
  - 행렬 곱셈 대신, OAM 메모리의 특정 주소(`attr0`, `attr1`)에 $y, x$ 값을 적어주면 PPU(하드웨어)가 알아서 그 위치에 그림을 띄웁니다.
