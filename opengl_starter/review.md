### ⚔️ CS200(OpenGL) vs. GBA(Bare-metal) 비교 분석

#### 1. 모델 변환 (Model Transformation)

- **CS200 (`week3.txt`): GPU가 처리**
  
  - **코드:** `vertex_glsl` 쉐이더에 `uniform mat3 uModel`이 선언되어 있습니다.
  
  - **동작:** CPU(`main_loop`)에서 `cos`, `sin`으로 행렬을 만들고 `glUniformMatrix3fv`로 GPU에 던지면, **GPU의 버텍스 쉐이더가 모든 정점에 대해 병렬로 곱셈을 수행**합니다.
  
  - **특징:** 정점(Vertex) 데이터 자체는 변하지 않고 그대로 유지됩니다.

- **GBA 구현: CPU가 처리 (Software Transform)**
  
  - **현실:** GBA에는 버텍스 쉐이더도, FPU(부동소수점 유닛)도 없습니다.
  
  - **구현:**
    
    1. **행렬 라이브러리:** `fixed` 기반의 3x3 행렬 구조체(`Mat3`)와 곱셈 함수(`mat3_mul_vec2`)를 직접 짜야 합니다.
    
    2. **CPU 연산:** 매 프레임 CPU가 `for` 문을 돌면서 정점 하나하나에 행렬을 직접 곱해서 새로운 좌표를 계산해야 합니다.
  
  - **결과:** "변환된 좌표"를 `draw_line` 함수에 넘겨줘야 비로소 화면에 그려집니다.

#### 2. 스태틱 vs. 다이내믹 버퍼 (Static vs. Dynamic Buffers)

- **CS200 (`week3.txt`): 메모리 전송 (Transfer)**
  
  - **Static:** `gVertexBuffer`(얼굴 모양)는 `GL_STATIC_DRAW`로 생성되어 한 번만 전송됩니다.
  
  - **Dynamic:** `gBackground`(물결 배경)는 `GL_DYNAMIC_DRAW`로 생성됩니다. 매 프레임 CPU에서 `sin/cos`으로 값을 바꾸고, **`glBufferSubData`**를 호출하여 CPU 메모리에서 GPU 메모리로 데이터를 **복사(Upload)**합니다.

- **GBA 구현: 메모리 직접 수정 (Direct Access)**
  
  - **현실:** GBA는 **통합 메모리 구조**입니다. VRAM, WRAM(CPU 메모리)이 같은 주소 공간에 있습니다. "전송(Upload)"이라는 개념이 (DMA를 쓰지 않는 한) 없습니다.
  
  - **구현:**
    
    - **Static:** `const Vertex face[] = { ... };` (ROM에 저장됨)
    
    - **Dynamic:** `Vertex wave[]` 배열을 RAM에 선언하고, 그냥 `wave[i].x = ...` 처럼 **값을 직접 바꿉니다.**
  
  - **차이점:** `glBufferSubData` 같은 함수 호출이 필요 없습니다. 그냥 변수 값을 바꾸고 `draw` 함수를 부르면 끝입니다.

---

### 🛠️ GBA Week 3 구현 전략 수정 (Bridge the Gap)

CS200의 "변환"과 "동적 버퍼"를 GBA에서 제대로 느끼려면, 단순히 선만 그리는 것으로는 부족합니다. 태경 님의 의견을 반영하여 **Week 3 구현 목표를 업그레이드**하겠습니다.

1. **Rasterizer (기반):**
   
   - 변환된 점들을 눈으로 확인하려면 **`draw_line` (브레즌햄 알고리즘)**이 필수입니다. (이게 없으면 계산만 하고 화면은 까맣습니다.)

2. **Transform (핵심):**
   
   - `fixed` 기반 **행렬 연산 함수** 구현 (`rotate`, `scale`, `translate`).
   
   - 정점 배열을 돌며 행렬을 곱하는 로직 구현.

3. **Dynamic Buffer (응용):**
   
   - 삼각형의 정점을 매 프레임 회전(Rotation) 시키면서 모양을 바꾸는 데모 구현.
