# 📝 GBA Porting Dev Log: Week 2 - Engine Foundation

## 1. 개요 (Overview)

CS200 Week 2의 주제는 엔진의 기초인 **Window(창), Input(입력), Rendering Pipeline(렌더링)**입니다.

이 문서는 PC의 고수준 API(OpenGL/SDL)가 GBA의 저수준 하드웨어(Bare-metal)에서 어떻게 매핑되는지 분석하고 구현 전략을 수립한 기록입니다.

---

## 2. 초기화와 윈도우 (Initialization)

**"도화지를 준비하는 과정"**

PC는 운영체제(OS)에게 요청해야 하지만, GBA는 하드웨어를 점유합니다.

| **Feature**     | **PC (OpenGL/SDL)**         | **GBA (Bare-metal)**     | **핵심 차이 (Key Difference)**   |
| --------------- | --------------------------- | ------------------------ | ---------------------------- |
| **System Init** | `SDL_Init(SDL_INIT_VIDEO)`  | **없음** (전원 ON = Init 완료) | OS 허가 vs 하드웨어 직접 제어          |
| **Window**      | `SDL_CreateWindow(...)`     | `REG_DISPCNT = Mode3`    | 가상 윈도우 생성 vs 물리 스크린 설정       |
| **Context**     | `SDL_GL_CreateContext(...)` | **없음** (MMIO)            | 상태 저장소 생성 vs 레지스터가 곧 상태      |
| **Drivers**     | `glewInit()`                | `#define REG_XXX 0x...`  | 런타임 함수 주소 찾기 vs 컴파일 타임 주소 고정 |

> **💡 Insight:**
> 
> - PC의 `CreateWindow`는 "공장"처럼 창을 계속 찍어낼 수 있지만, GBA의 화면은 물리적으로 하나뿐인 "액자"와 같습니다.
> 
> - GBA는 드라이버가 없으므로 `glewInit` 같은 로딩 과정 없이, 메모리 주소(`0x0400...`)에 값을 쓰는 즉시 하드웨어가 반응합니다.

---

## 3. 데이터 정의와 전송 (Data & Buffers)

**"데이터를 GPU로 옮기는 과정"**

GBA는 FPU(부동소수점 유닛)가 없고, 비디오 메모리(VRAM) 접근 방식이 다릅니다.

### A. 자료형 (Data Type)

- **PC:** `float` (32bit 부동소수점) - 정밀함, FPU 가속.

- **GBA:** `fixed` (고정 소수점) - 정수 연산으로 소수 흉내 (예: 256 = 1.0). **필수 최적화.**

### B. 버퍼링 (Buffering)

C++

```
// [PC] 복잡한 이사 과정
glGenBuffers(1, &VBO);          // 1. 박스 이름표 받기
glBindBuffer(..., VBO);         // 2. 박스 열기
glBufferData(..., vertices, ...); // 3. RAM -> VRAM 이사 (버스 타고 이동)
```

C

```
// [GBA] 단순한 접근
// 데이터는 이미 ROM(카트리지)에 있음.
const Vertex* v = &vertices[i]; // 포인터로 즉시 접근 (이사 불필요)
```

> **💡 Insight:**
> 
> - OpenGL의 `Binding`은 드라이버에게 "작업 대상"을 알려주는 추상화된 개념입니다.
> 
> - GBA는 이런 추상화 없이 CPU가 데이터(ROM)를 읽어 화면(VRAM)에 바로 씁니다.

---

## 4. 데이터 해석 (Vertex Interpretation)

**"이 데이터가 점인지 색깔인지 설명하는 과정"**

### A. VAO vs Struct

- **PC (`VAO`):** GPU는 데이터 덩어리만 받으므로, `glVertexAttribPointer`를 통해 "앞에 2개는 좌표고, 8바이트 건너뛰면 색깔이야"라고 일일이 설명하고 활성화(`Enable`)해야 합니다.

- **GBA (`Struct`):** C언어 구조체(`struct Vertex { fixed x, y; u16 color; }`)를 선언하는 순간 컴파일러가 오프셋을 다 알고 있습니다. 런타임 설정이 필요 없습니다.

| **개념**         | **PC (OpenGL)**                   | **GBA (C Code)**                   |
| -------------- | --------------------------------- | ---------------------------------- |
| **해석기**        | `glVertexAttribPointer` (Runtime) | `struct` Definition (Compile-time) |
| **간격(Stride)** | `sizeof(vertex)` 파라미터 전달          | `vertices[i]` (자동 계산)              |
| **위치(Offset)** | `(void*)offset` 포인터 연산            | `v.color` (자동 계산)                  |

---

## 5. 렌더링 파이프라인 (The Pipeline)

**"실제로 그림을 그리는 과정"**

GBA에는 쉐이더(Shader)가 없습니다. 모든 것을 CPU가 하거나, 고정된 하드웨어 기능을 씁니다.

### A. Vertex Shader (위치 결정)

- **PC:** `gl_Position = matrix * vec4(pos, ...)` (쉐이더 코드 실행).

- **GBA:** 하드웨어 **OAM(스프라이트)** 좌표 설정 또는 **Affine 레지스터**(`REG_PA` 등) 조작.

### B. Fragment Shader (색상 결정)

- **PC:** `FragColor = texture(...) * color` (픽셀마다 연산).

- **GBA:** **Palette RAM** 참조 (인덱스 컬러) 및 **SFX 레지스터**(`Mosaic`, `Blend`) 설정.

### C. Draw Call

C++

```
// [PC] GPU에게 명령 위임
glDrawElements(GL_TRIANGLES, count, ...);
```

C

```
// [GBA] CPU가 직접 노동 (Software Rasterizer)
for (int i=0; i<count; i++) {
    draw_line(v1.x, v1.y, v2.x, v2.y, color); // Bresenham Algo
}
```

---

## 6. 게임 루프 (Game Loop)

**"심장 박동: 입력-연산-출력"**

| **단계**    | **PC (SDL Loop)**           | **GBA (Infinite Loop)**             |
| --------- | --------------------------- | ----------------------------------- |
| **Input** | `SDL_PollEvent` (큐에서 꺼냄)    | `REG_KEYINPUT` (실시간 폴링)             |
| **Clear** | `glClear` (GPU가 지움)         | `Dirty Rectangle` or `DMA` (최적화 필수) |
| **Draw**  | `glDrawElements`            | `draw_line` / `OAM Update`          |
| **Sync**  | `SwapWindow` (OS/Driver 위임) | `sync_vblank` (수동 타이밍 대기)           |

> **💡 Insight:**
> 
> - PC는 멀티태스킹 환경이라 이벤트 큐와 OS의 허락(Swap)이 중요합니다.
> 
> - GBA는 독점 환경이라 무한 루프 내에서 직접 하드웨어 타이밍(VBlank)을 제어해야 합니다.
