# STEP 4 — Codex 전용 PBR Texture 원본

2026-09-02에 Codex가 Poly Haven에서 독립 검색하고 직접 다운로드한 원본이다. 다른 Agent의 다운로드 파일, Material 또는 Scene을 읽거나 복사하지 않았다.

## Source / License / 용도

모든 Texture Asset은 Poly Haven이 [CC0-1.0](https://polyhaven.com/license)로 제공한다. 상업 사용과 재배포가 허용되며 Asset 사용 시 attribution은 필수는 아니지만 출처 추적을 위해 기록한다. Poly Haven의 사이트 로고, 소개 이미지, 예제 Render 등은 이 Asset 라이선스와 별개이며 다운로드하지 않았다.

| 세트 | Source / 제작자 | 실제 Tile 폭 | 예정 용도 | 원본 파일 수 |
|---|---|---:|---|---:|
| Concrete Floor 02 | [Poly Haven](https://polyhaven.com/a/concrete_floor_02) / Rob Tuytel | 2 m | Concrete yard, 구조물 기반, Barrier | 4 |
| Asphalt Floor | [Poly Haven](https://polyhaven.com/a/asphalt_floor) / eye-candy.xyz | 2.3 m | Main loop, Asphalt ground patch | 4 |
| Rusty Metal Sheet | [Poly Haven](https://polyhaven.com/a/rusty_metal_sheet) / Amal Kumar | 2 m | 낡은 철판, Container, Utility box, Pipe 부분 | 5 |
| Wooden Planks | [Poly Haven](https://polyhaven.com/a/wooden_planks) / Charlotte Baglioni, Dario Barresi | 2 m | Pallet / Crate timber | 4 |

## 파일 및 검증

- 4개 세트, PNG 17장, 모두 2048 × 2048 (2K).
- 각 세트: BaseColor (`diff`), DirectX Normal (`nor_dx`), Roughness (`rough`), AO (`ao`).
- Rusty Metal Sheet는 추가 ARM (`arm`): R=AO, G=Roughness, B=Metallic.
- 합계: **223,953,047 bytes / 213.58 MiB**.
- 공식 API가 제공한 MD5와 byte size: 17/17 일치.
- PNG header 해상도 및 로컬 이미지 decoder: 17/17 성공.
- 다운로드: 2026-09-02 12:43:17Z ~ 12:44:01Z (한국 시간 21:43:17 ~ 21:44:01).
- 개별 URL, 실제 파일 크기, MD5, SHA-256, 다운로드 시간은 [manifest.json](manifest.json)에 기록했다.
- 원본 파일은 색상/비트 깊이/해상도를 가공하지 않았다. 첫 PNG의 호스트 미리보기는 포맷을 표시하지 못했지만 별도 decoder와 원본 hash 검증은 성공했다. Unreal Import/Material의 최종 시각 검증은 별도로 진행해야 한다.

## Unreal Import 연결 지침

| 용도 | sRGB | Compression / 연결 |
|---|---|---|
| BaseColor | true | 일반 Color, RGB → Base Color |
| Normal DX | false | Normalmap, green 반전 없이 Normal에 연결 |
| Roughness | false | Masks/Data, R → Roughness |
| AO | false | Masks/Data, R → Ambient Occlusion |
| ARM | false | Masks, R=AO / G=Roughness / B=Metallic |

큰 Mesh에 한 타일을 늘이지 말고 실제 Tile 폭을 기준으로 UV tiling을 설정한다. Rust/Paint는 dielectric이므로 녹슨 Texture 전체를 Metallic=1로 만들지 않는다. 단색 Painted Metal variation은 동일 Master Material의 Color Tint 등으로 제작하되, 불필요한 Texture 복제는 피한다.

## 사용 범위

이 폴더는 STEP 4의 외부 원본 공급 기록이다. Unreal `.uasset`, Material Instance, Level, Blender 파일은 이 다운로드 작업에서 생성/수정하지 않았다. 이 원본 폴더를 Git에 포함할지는 프로젝트의 대용량 Asset 정책을 따라 결정한다.
