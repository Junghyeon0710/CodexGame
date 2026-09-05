# PROJECT: LAST STAND — STEP 4 Arena Level + Environment

## STEP 4 최종 결과

| 항목 | 현재 기록 |
|---|---|
| Agent | Codex |
| Start Time | 2026-09-02 21:40:11 +09:00 |
| End Time | 2026-09-04 21:06:50 +09:00 — 최종 기록 계측 마감 |
| Elapsed Time | 47h 26m 39s / 2846.65분 — 중단·재개 포함 wall time, active time N/A |
| Requirements | 55 — STEP 4의 명시 완료조건 기준 |
| Completed / Failed | 55 / 0 |
| Completion Rate | 100% — 명시된 55개 완료조건 기준; 아래 비치명 한계 별도 |
| User Intervention Count | 0 — 현재까지 구현 선택·오류 해결·Editor 수동 작업 요청 없음 |
| Manual Action Count | 0 |
| Token Usage / Context Usage | Root 누적 37,602,205 / 88,126 → 206,478 (보고 window 258,400) |
| Usage Limit Data | N/A |

Environment 제작, Build와 최종 Arena의 정상 입력 기반 Wave 1~3 / Victory, 별도 밀집·Collision fixture,
GameOver / Restart 및 Camera / Geometry 증거를 정리했다.
기존 STEP 3의 성공을 새 Arena의 Gameplay 성공으로 대체하지 않는다.
소요 시간 / Token과 `Metrics.csv` / `Summary.md`를 함께 마감했다. STEP 4 완료이며 STEP 5는 시작하지 않았다.

## 범위와 보존

- 새 Level: `/Game/AgentGameTest/Codex/Levels/L_LastStand_Arena_Codex`.
- Environment: `/Game/AgentGameTest/Codex/Environment/{Meshes,Materials,Textures}`.
- 원본: `ExternalAssets/LastStand/Codex/{Models,Textures}`.
- 생성·Import·검증 Script: `Scripts/AgentGameTest/Codex/`.
- 기존 STEP 1~3 Test Level 및 Blueprint는 유지한다. Arena 전용 상속 Blueprint로 시각 설정과 연결을 분리했다.
- 기존 STEP 3 미커밋 변경은 보존한다. `git diff` 전체를 STEP 4 코드 변경량으로 집계하지 않는다.
- 다른 Agent의 코드, Content, Level, Blender 파일, 외부 Asset 원본을 열어 재사용하거나 수정하지 않는다.
- 새 Wave, 무기, Enemy 종류, GAS Framework, UI/CommonUI, 최종 Niagara, Audio, Save/Inventory/Quest/Online은 추가하지 않는다.

## Arena Layout / Ground / Lighting

플레이 영역은 **5000 × 5000cm = 50 × 50m**, 중심 기준 X/Y 약 ±2500cm이다.
이는 배경 Mesh까지 포함한 Level 전체 bounds가 아니라 전투 가능한 Yard의 설계 크기다.

- 중앙: 지붕 없는 Pipe Skid와 Utility 설비. 좌우 우회 경로를 유지하고 높은 지붕으로 Player를 가리지 않도록 구성했다.
- Main Loop: 중앙 설비를 도는 넓은 주 동선. 6~8m를 설계 목표로 두었다.
- Side Routes: Container / Barrier 주변 보조 통로. 3.5~4.5m, 유효 최소 3m를 설계 목표로 두었다. 실제 정상 전투 / 16 Enemy 밀집 / Collision 시험에서 이동 가능한 동선을 확인했으며 모든 통로의 최소 폭을 실측했다고 주장하지 않는다.
- PlayerStart: 중앙 서측 열린 Yard `(-1300, 0, 100)`cm. 기존 Capsule R42/H88, Camera arm 1100cm / pitch -55도, Dash 약 252cm를 기준으로 배치했다.
- Container 네 동과 Barrier, Warehouse Bay, Pallet / Crate / Barrel 묶음으로 공간을 구획한다. Prop은 주 동선보다 구조물 가장자리에 모았다.
- Boundary: Curb / Fence와 Warehouse로 전투 구역을 제한한다. 최종 확장 Collision fixture에서 네 방향 Fence가 약 ±2452cm에서 Player를 막았고 이동 중 sample에도 외부 이탈 / 바닥 낙하는 없었다.
- Background: 별도 Apron, Warehouse와 Container Stack을 배치했다. 배경은 전투 공간과 구분하고 불필요한 Collision을 끈다.
- Ground: 10m Ground Tile 25개에 Concrete / Asphalt Material을 적용했다. Drain, 도색 선, 횡단 표시로 구역을 나눈다. 단순 무재질 Plane 하나로 끝내지 않는다.
- 반복 완화: 동일 Mesh 재사용 + Material Instance 색상 변화, 회전, Prop 묶음의 배치 변화. 별도 최종 Decal Asset은 만들지 않았다.

최종 Camera 수정은 Gameplay Camera / GAS를 바꾸지 않고 환경 배치로 해결했다.
전투 영역의 Container 4동은 Z scale **0.45**, 높이 **116.55cm**의 낮은 Cargo로 조정했다.
원본 / 배경 Shipping Container 높이 259cm는 유지한다. 접촉한 Player가 높은 Container 뒤에 완전히 가려지던 문제를
0.7배 중간 수정 후 다시 검토하고 0.45배로 낮췄다. Barrel 묶음 2 / 4의 각각 3개, 총 6개를 X축으로 -230 / -340cm 이동해
Container와의 겹침을 해소했으며 주요 전투 동선은 유지했다.

현재 생성 Script 기준 Lighting은 Directional Light 12000, SkyLight 1.25,
SkyAtmosphere, 약한 Fog density 0.003, 고정 노출 EV 11.4, Bloom 0.12,
Vignette 0.05, Chromatic Aberration 0이다. 산업 조명 Point Light는 2개,
intensity 750 / attenuation 450cm, Shadow 비활성으로 제한했다.
수치 설정과 별도로 실제 Top-Down Camera / Warehouse / Container 접촉 / 밀집 화면을 검토했다.

## Blender 제작과 Asset 품질

Codex 전용 `LS_Codex_Environment.blend`에서 Blender 5.2 Python(`bpy`)로 제작했다.
Render Mesh 13개와 UCX 단순 Collision Mesh 13개, 총 26개 Mesh Object를 만들고 13개 FBX로 Export했다.
직접 제작한 Environment Kit는 총 21,520 render triangles이며, 모델별 크기와 Triangle 정보는
`ExternalAssets/LastStand/Codex/Models/Step04BlenderKitManifest.json`에 기록했다.

| Mesh | 역할 |
|---|---|
| `SM_LS_Container` | 골판·프레임 디테일이 있는 6m급 Container, 색상 Variation |
| `SM_LS_ConcreteBarrier` | Cover / 통로 구획; Arena 배치에서는 높이를 1.25배 조정 |
| `SM_LS_PipeSkid` | 중앙 Pipe / Flange 설비 |
| `SM_LS_UtilityBox` | Utility / Maintenance 설비 |
| `SM_LS_Barrel` | 산업용 Barrel Prop |
| `SM_LS_Pallet` | 목재 Pallet |
| `SM_LS_FenceSection` | 경계 Fence 모듈 |
| `SM_LS_Warehouse` | Warehouse / Bay 및 배경 구조물 |
| `SM_LS_Crate` | 목재 Crate |
| `SM_LS_IndustrialLamp` | 산업용 조명 실루엣 |
| `SM_LS_GroundTile` | 10m Ground 모듈 |
| `SM_LS_Drain` | Drain / Grate |
| `SM_LS_Curb` | Concrete Curb / 경계 기단 |

- Origin은 바닥 중앙, GroundTile은 윗면 Z=0 기준. 모델 Transform scale은 1로 정리했다.
- UV0는 실제 미터 기준 타일링이며 기본 2m당 한 반복을 사용했다. 실제 Import된 UV channel은 **1개(UV0)**로 확인했다. 이전 기록의 UV1 생성 주장은 정정한다. 별도 lightmap UV1은 생성되지 않았다.
- FBX 단위/축 설정과 Unreal import scale 1을 명시해 100배 크기 오류를 방지했다.
- UCX Collision을 Import하고 Blocking Mesh에 단순 Collision이 존재하는지 검사했다.
- 기본 UE Cube를 환경 Asset으로 복제한 결과가 아니라 bevel, corrugation, flange, grate 등의 모델 형상을 직접 작성했다.
- 자동 검사와 Floating / Stretch / Occlusion의 실제 화면 검토를 별도 증거로 관리했다. 최종 검사에서는 심각한 Floating / Texture Stretch / Z-Fighting을 발견하지 못했으며 부분 Occlusion 한계는 Known Issues에 명시한다.

## Texture Source / License / Material

외부 원본은 Poly Haven에서 독립적으로 확보한 **CC0 Texture 4세트, 2K PNG 17장**이다.
다운로드 총량은 223,953,047 bytes (213.58 MiB). 공식 MD5 / byte size와 PNG 해상도·decode 검사는 17/17 일치했다.
이는 디스크 원본 크기이며 GPU Texture Memory 측정값이 아니다.

| Asset | Source | License | 사용 |
|---|---|---|---|
| Concrete Floor 02 | [Poly Haven](https://polyhaven.com/a/concrete_floor_02) / Rob Tuytel | [CC0](https://polyhaven.com/license) | Concrete Yard, Barrier, 구조물 기반 |
| Asphalt Floor | [Poly Haven](https://polyhaven.com/a/asphalt_floor) / eye-candy.xyz | CC0 | Ground / Main Loop / Background Apron |
| Rusty Metal Sheet | [Poly Haven](https://polyhaven.com/a/rusty_metal_sheet) / Amal Kumar | CC0 | Container, Pipe, Utility 및 낡은 철판 |
| Wooden Planks | [Poly Haven](https://polyhaven.com/a/wooden_planks) / Charlotte Baglioni, Dario Barresi | CC0 | Pallet / Crate |

정확한 다운로드 URL, 해시, 크기, 시간은 `ExternalAssets/LastStand/Codex/Textures/manifest.json`,
설명은 같은 폴더의 `README.md`에 보존했다. 라이선스 불명 Asset은 사용하지 않았다.

Master는 `M_LastStand_Surface_Codex` **1개**, Environment MI 14개와 가독성 MI 2개를 합쳐 **16개**다.
BaseColor / Normal / Roughness / AO / Metallic Map, UV Tiling, ColorTint,
NormalStrength, RoughnessMultiplier, Emissive 설정을 공유한다.
BaseColor만 sRGB를 켜고 DirectX Normal은 normalmap compression과 sRGB off,
Roughness/AO/ARM은 data texture로 Import했다. Rust Metal ARM의 B를 Metallic 정보로 사용하며
녹슨 표면 전체를 무조건 Metallic=1로 만들지 않는다.
2m 기본 UV와 2.3m Asphalt 원본 크기를 Material tiling에 반영했다.
시각 검토 후 Concrete MI는 tint `(0.36, 0.40, 0.43)`, NormalStrength `0.35`,
BaseColorTexWeight `0.45`로 완화했다. 최종 화면에서 Concrete / Asphalt 구획과 Character 대비를 확인했으며 PBR 디테일의 시각 품질 자체 평가는 6/10이다.

## 기존 Gameplay 연결과 최소 C++ 수정

Wave / GAS / Damage / Score / Restart 구조는 기존 STEP 3을 유지한다.
Wave 구성은 **5 Grunt / 7 Grunt+3 Runner / 10 Grunt+6 Runner**, Score는 100 / 150,
전체 처치 기준 3550이며 이번 STEP에서 숫자를 조정하지 않았다.

Arena 전용 Blueprint 네 개는 기존 Codex Blueprint를 상속한다.

- `BP_Player_Arena_Codex`: Player 가독성 Material 연결.
- `BP_Grunt_Arena_Codex`, `BP_Runner_Arena_Codex`: 동일 gameplay를 유지하고 Enemy 색상 / Material 연결.
- `BP_GameMode_Arena_Codex`: 기존 STEP 3 GameMode를 상속하고 Arena Player class 연결.

STEP 4에 귀속되는 C++ 변경은 **기존 파일 5개, 새 파일 0개**다.

| 파일 | 필요한 변경과 이유 |
|---|---|
| `Private/AgentGameTest/Codex/AI/CodexLSEnemyAIController.cpp` | 근접거리라도 환경 LOS가 막히면 공격 대신 추적. `FAIMoveRequest`에서 agent/goal radius 가산을 모두 끄고, 차단 시 acceptance 25cm / 평상시 기존 AttackRange×0.8을 사용해 장애물 반대편에서 AlreadyAtGoal로 멈추는 문제를 방지. 이전 요청이 끝나면 Idle에서 좁은 요청으로 재시도. |
| `Public/AgentGameTest/Codex/Enemy/CodexLSEnemyCharacter.h` | 최소 LOS 함수와 설정 가능한 `VisualMaterial` 추가. |
| `Private/AgentGameTest/Codex/Enemy/CodexLSEnemyCharacter.cpp` | Actor root 높이의 Visibility LOS를 실제 Melee 실행에서도 검사. Pawn-only Damage Sweep가 Barrier 너머 Player를 때리는 것을 차단. Material 지정 후 기존 Enemy Color 적용. |
| `Public/AgentGameTest/Codex/Player/CodexLSPlayerCharacter.h` | Arena Blueprint에서 지정할 `VisualMaterial` 추가. |
| `Private/AgentGameTest/Codex/Player/CodexLSPlayerCharacter.cpp` | BeginPlay에서 지정한 시각 Material 적용. Player Ability / Attribute / Input을 재작성하지 않음. |

Enemy 수정은 Arena 장애물과 연결되는 명확한 전투/추적 문제에 한정한다.
Build 이후 Barrier 양면 fixture에서 관통 Melee 차단과 이동 재개 후 우회 공격을 확인했고,
SpawnPoint별 AI 도달도 개별 성공 증거를 확보했다. 상세 결과와 실패 이력은 아래 Runtime 기록에 분리한다.

## Spawn / Navigation

기존 `BP_EnemySpawner_Codex`와 SpawnPoint 기반을 재사용하고 Arena 전용 Grunt / Runner class를 연결했다.
최소 Player 거리 1100cm, Spawn 내부 시도 12회, 기존 Interval / 실패 처리 / Death Tracking은 유지한다.

| SpawnPoint | 설정 위치 cm |
|---|---|
| NW | (-2050, 1700, 100) |
| N | (0, 2150, 100) |
| NE | (2050, 1850, 100) |
| SE | (2100, -1850, 100) |
| S | (-200, -2150, 100) |
| SW | (-2050, -1750, 100) |

NavMeshBounds를 Arena에 맞게 두고 Recast를 Dynamic / 실제 agent radius **35cm** / height 176cm로 확인했다.
Player / Enemy Capsule radius는 별도로 **42cm**다. Python에서 지원되지 않은 radius 42 설정은 제거했으며,
실제 Recast 기본값 35를 Capsule 크기 또는 성공한 설정값 42로 잘못 기록하지 않는다.
지붕이나 Container 윗면에 작은 Navigation Island가 생기지 않도록 해당 구조물의 NavArea_Null을 설정했다.
회전 설정 수정 후 재생성·재검사한 Editor Audit에서는 6개 Point 모두 Navigation projection 및 PlayerStart까지 완전한 path query를 통과했다.
이는 **경로 조회 성공**이며 **실제 Spawn 또는 AI 도달 성공**과 동일하지 않다.

`TestStep04SpawnArrival.py`는 별도 TEST fixture다. 기존 Wave 추적 대상 Enemy 한 마리를 각 Point에 순서대로
teleport하고, 정상 AI Walking으로 170cm 이내 실제 도달하는지 최대 20초씩 검사하도록 작성했다.
Player Health/MaxHealth 9999와 위치 변경을 명시하며 Wave 카운터, Enemy 생성/삭제, Damage를 직접 조작하지 않는다.
실제 Point별 Spawn은 최종 정상 Wave Spawn 로그로 증명했다. Session `798B3020`의 31회 성공을 집계하면
NW 8 / N 6 / NE 2 / SE 5 / S 4 / SW 6회이며, 로그의 Player 거리 범위는 1405~3825cm다.
도달 fixture는 두 번의 전체 실행이 각각 5/6 성공했다. 첫 실행의 NW timeout과 두 번째 실행의 SE timeout은 보존하며,
두 실행을 합쳐 **6개 위치 각각의 실제 도달 성공**을 확보했다. 단일 실행이 6/6 성공했다고 기록하지 않는다.

## Tool 경로 / 실패와 복구

| 단계 | 관측된 결과 | 복구 / 구분 |
|---|---|---|
| Blender interactive MCP | localhost:9876 연결 실패 | 기존 프로젝트/다른 Agent Scene을 건드리지 않고 독립 경로 확인 |
| Blender background MCP | executable `blender` / `BLENDER_PATH`를 찾지 못함 | 설치된 Blender 5.2 절대 경로를 사용한 CLI + `bpy`로 제작 / Export |
| Blender 실제 제작 | 전용 `.blend`, 13 Render Mesh + 13 UCX, 13 FBX 생성 | MCP 성공으로 표기하지 않음. 실제 제작 주체는 Blender Python |
| Unreal Import 1 | Sandbox의 AppData / DDC 쓰기 권한 문제로 Script 실행 전 실패 | 필요한 권한으로 Commandlet 재시도 |
| Unreal Import 2 | Material Clamp 입력 연결 오류 | 실제 UE Material pin API에 맞게 Script 수정 후 재시도 |
| Unreal Import 후속 실행 | `Step04_ImportReport.json` status=success | 13 Mesh / 17 Texture / 1 Master / 14 Environment MI 존재 확인. 재실행의 created=false는 이미 생성된 Asset 재사용을 뜻함 |
| Unreal MCP | Scene / EditorApp / Slate toolset 조회와 Editor 상태 / 명령 / Capture 경로 사용 | Asset 제작은 Unreal Python, 상태·화면 검증은 MCP와 로그를 조합 |
| 네이티브 화면 Capture | Windows Capture API 오류 관측 | Unreal Editor / Slate Capture로 검토 경로 전환 |
| Editor startup Script | `-ExecutePythonScript` 정상 완료 후 자동 `QUIT_EDITOR`가 발생 | 처음에는 MCP 연결 종료 문제로 오인했으나 startup Script 종료 동작을 확인. 일반 map launch로 복구한 Editor에서 후속 PIE 수행 |
| Arena 회전 | `unreal.Rotator(0.0, yaw, 0.0)`가 의도한 Yaw가 아니라 Pitch 회전으로 적용 | 중앙 Barrier quaternion / bounds로 기울어 묻힌 형상을 확인. 모든 배치 및 Sunlight 회전을 `pitch=..., yaw=..., roll=...` 키워드 인수로 수정하고 Level 재생성 / Navigation 재검증 |
| 재생성 직후 Navigation | 2026-09-03 11:45:34.744~11:46:33.644 UTC에 `NavigationProjectionFailed` Spawn 경고 98회 | 새 Navigation Bounds 등록 / rebuild 시점 문제. 후속 Editor rebuild와 6개 경로 검사를 통과했고 정상 Victory 세션의 31회 Spawn에는 해당 실패 없음. 생성 Script의 후속 frame rebuild 처리를 분리 |
| Runtime Navigation 진단 | NavigationSystem CDO 호출에서 handled ensure 1회 발생 | 같은 ensure가 두 로그 위치에 출력됨. Gameplay 오류와 구분하고 Runtime AutoPlay / 도달 fixture에서 해당 static query를 제거. Editor 경로 조회와 Runtime 실제 이동을 분리 |
| 초기 Input AutoPlay | Editor background throttle로 약 3 FPS까지 낮아져 0.06 / 0.12초 입력 Press / Release가 같은 frame에서 처리됨 | 로드된 `EditorPerformanceSettings` CDO의 `bThrottleCPUWhenNotForeground=false`를 Runtime에서만 설정. `SaveConfig`하지 않음. 약 60 FPS 회복 후 입력 / Hit 정상. STEP 종료 시 원래 값 복원 필요 |
| Background throttle 진단 API | `unreal.EditorPerformanceSettings` Python module attribute 조회에서 AttributeError 1건 | `/Script/UnrealEd.EditorPerformanceSettings` class를 load한 뒤 CDO에 접근. 정상 Victory 중의 진단 오류이며 Gameplay 실패는 아님 |
| Spawn 도달 fixture 초기화 | `active_enemies` 비공개 property 조회 실패 1회, 유효한 live game을 얻지 못한 후속 fixture 실패 2회 | 비공개 TSet 조회를 제거하고 실제 Wave 생성 / AI possession / 수동 배치 Enemy 0개 / GameState count 증거를 분리. 이 3회는 실제 도달 시험을 시작하지 못한 실패로 보존 |

Blender MCP 호출은 현재 확인된 2회 모두 실패했으며 성공한 MCP 제작 호출은 0이다.
Blender CLI 제작 성공을 이 실패 기록에서 제외하거나 MCP 성공으로 바꾸지 않는다.
Unreal MCP의 정확한 전체 Calls / Successful / Failed는 **N/A — 최종 로그 집계 전 미확정**이다.
Editor가 정상 종료된 뒤 연결할 수 없는 상황과 MCP 자체 기능 실패는 원인을 구분해 기록한다.

## Build 기록

Target: `CodexGameEditor Win64 Development`, UE 5.8.2.

| Build | 결과 | 소요 시간 | 원인 / 근거 |
|---|---|---:|---|
| #1 | FAIL | 0.19초 | Sandbox AppData 쓰기 권한 문제. C++ Compile 시작 전 실패. 실패 Build 수에는 포함하지만 Compile Error로 계산하지 않음. |
| #2 | PASS | 37.15초 | 필요한 권한으로 재실행. `Saved/AgentComparison/Codex/Step04_Build02.log`의 `Result: Succeeded`, `Total execution time: 37.15 seconds`. |

- Build Attempts: **2**.
- Successful Builds: **1**.
- Build Failures: **1**.
- Compile Errors: **0**.
- Compile Warnings: N/A — 최종 경고 분류 미확정.
- Blueprint Compile Errors / Accessed None: 확인한 최종 실행 로그에서 0. Runtime의 치명적 Gameplay 오류는 최종 정상 실행에서 0이며, 과거 handled ensure 1회와 진단 / Navigation 실패는 위 이력에 별도 보존한다.
- Debug Iterations: N/A — STEP 전체 수정 회차의 정확한 집계 근거 없음.
- Import / Editor 시작 실패는 C++ Build와 별도 기록이며 Build 성공으로 PIE 성공을 대신하지 않는다.

## 생성 통계 — 현재 확인된 산출물

| 항목 | 수 |
|---|---:|
| C++ Files Created / Modified | 0 / 5 |
| Blueprints Created | 4 |
| StaticMeshes Created / Environment Mesh Count | 13 / 13 |
| Materials Created / Unique Master Material Count | 1 / 1 |
| MaterialInstances Created | 16 (Environment 14 + Readability 2) |
| Textures Imported | 17 |
| Levels Created | 1 |
| Decals Created | 0 |
| GameplayAbilities / GameplayEffects / InputActions Created | 0 / 0 / 0 |
| NiagaraSystems / Widgets Created | 0 / 0 |
| Unreal Asset 합계 | 52 (4 BP + 13 Mesh + 1 Master + 16 MI + 17 Texture + 1 Level) |
| Blender Render Mesh Objects / UCX Collision Objects | 13 / 13 |
| Blender Objects Created | 26 |
| Blender Meshes Exported / FBX files | 13 / 13 |
| Blender source `.blend` | 1 |
| Spawn Point Count | 6 |
| Script-created Level Actors | 228 |
| Editor Audit Actor Count | 최신 235 / 이전 236 — Engine helper Actor 포함 여부에 따른 snapshot 차이 |
| Files / Lines Added, Modified, Deleted | N/A — STEP 3의 기존 dirty work와 분리한 최종 집계 전 |

52개는 Unreal Content 산출물 집계다. Blender 원본, FBX, PNG 원본, Python Script, JSON 증거를 중복 합산하지 않는다.
228은 Level 생성 Script가 기록한 Actor 수, 최신 235(이전 236)는 Editor World Audit의 전체 Actor 수이므로 의미가 다르다.
현재 STEP 4 환경 산출물의 실제 Import/Readability/Level/Audit 보고서를 기준으로 했으며 재실행별 신규 개수를 더하지 않았다.

## 최종 검사 증거

Editor Audit의 현재 snapshot은 `passed`이며 다음을 확인했다.

- SpawnPoint 6개, PlayerStart 1개, Spawner 1개, 수동 배치 Enemy 0개.
- 6개 SpawnPoint projection / 완전한 PlayerStart 경로.
- Missing Mesh / Missing Material / Default fallback Material 없음.
- Blocking Mesh의 단순 Collision 존재, 양수·유효 Scale, Environment UV0 존재.

이 결과는 `Saved/AgentComparison/Codex/Step04_EditorAudit.json`의 정적 World/Asset 검사다.
초기 Runtime Audit / Barrier 재현 Capture는 오류 복구 이력으로 보존한다.
Container 높이 / Barrel 위치 최종 조정 뒤 정상 Gameplay, 16 Enemy 밀집, Geometry 및 Collision을 재검증했다.
정상 플레이와 TEST fixture 증거를 구분하며 중간 수정본의 성공을 최종 수정본의 증거로 대체하지 않는다.

| 최종 게이트 | 현재 상태 / 필요한 증거 |
|---|---|
| PIE Test Runs | 18 — 09-02 시작 2회 + 09-03 9회 + 09-04 7회. Level Reload는 새 PIE 시작 횟수에 중복 합산하지 않음 |
| Wave 1 | PASS — 최종 정상 입력 세션 `798B3020`, Grunt 5 Spawn / Death / Clear, 누적 Score 500 |
| Wave 2 | PASS — 같은 세션, Grunt 7 + Runner 3 Spawn / Death / Clear, 누적 Score 1650 |
| Wave 3 | PASS — 같은 세션, Grunt 10 + Runner 6 Spawn / Death / Clear; 별도 16 Enemy 밀집 fixture도 PASS |
| Full Victory Flow | PASS — 최종 Arena에서 정상 입력으로 Wave 1→2→3→Victory, 31마리 / Score 3550 / 최종 HP 64 |
| GameOver | PASS — 2026-09-04 세션 `C5AED231`, 실제 Enemy melee로 HP 0. 입력 / Enemy 5개 / Spawn·Wave Timer 정지 |
| Restart | PASS — 최종 Victory→`1760360C`, GameOver→`2437BD7B` Level Reload. HP 100 / Abilities 2 / Score 0 / Preparing→Wave 1 확인 |
| Spawn Point별 실제 Spawn | PASS — 최종 정상 Victory 로그에서 NW 8 / N 6 / NE 2 / SE 5 / S 4 / SW 6회 |
| Spawn Point별 실제 AI 도달 | PASS — 별도 두 전체 fixture의 성공을 합쳐 6개 위치 각각 확인. 두 실행 모두 단독 결과는 5/6이며 timeout 이력 보존 |
| Player 이동 / Dash / Attack | PASS — 정상 입력 전투로 31 Enemy 처치. 확장 Collision fixture 8/8 PASS, 실제 Dash activation / 최고 1400cm/s sample 확인 |
| Enemy Melee Barrier / 접근 반경 수정 | PASS — 세션 `76CC594C`에서 Barrier 양면 정지 시 HP 100 유지, 이동 재개 후 우회하여 정상 melee HP 82 확인 |
| Camera / Player / Enemy Visibility | PASS — 실제 Top-Down / Container 접촉 / Warehouse / 밀집 화면 확인. 완전 가림 수정, 상부 cyan Player와 Enemy 식별 가능. 접촉 시 하부 부분 가림은 비치명 한계 |
| Floating / Z-Fighting / Stretch / Material | PASS — Geometry 9개 검사와 최종 화면 검토에서 심각한 문제 없음. Barrel 6개 겹침 위치 수정 |
| Boundary / Container / Fence Collision | PASS — 최종 수정본에서 위치·속도 sample을 포함한 8개 fixture 통과. Fence 네 방향 / Container / CentralPipe / Barrier에서 관통·낙하 없음, OpenRoute 이동 성공 |
| 치명적 Runtime Error 없음 | PASS — 최종 정상 실행의 Fatal / Assertion / Ensure / SpawnFailure / Accessed None / Invalid ASC·Delegate / Navigation·Timer 핵심 오류 검색 0. 과거 오류와 cleanup warning은 별도 보존 |

`PlayStep04Arena.py`는 기존 Input Exec를 통해 WASD / Dash / Primary Attack만 요청하는 별도 검증 Script다.
Input 요청 횟수는 Hit 또는 Ability activation 성공 횟수가 아니다. 강제 Kill / Health 변경 / Counter 변경을 하는
fixture의 결과와 일반 Gameplay Flow 결과를 혼합하지 않는다.

### 정상 Wave / Victory 실행

`Saved/AgentComparison/Codex/Step04_AutoPlay_20260904T115215_703622Z.json`:
2026-09-04 **20:52:15.704619~20:53:44.202995 KST**, wall **88.500초** / game **88.502초**.
`gameplay_bypasses_used=[]`, `observed_full_victory_flow=true`, Script errors / warnings 모두 0이다.
Player Health의 Python 필드는 `null`이므로 HP는 Native Snapshot으로 확인한다.

Native 근거는 2026-09-04 `Saved/Logs/CodexGame.log`의 Session `798B3020`이다.
로그 시각은 UTC이며 Wave 1 Clear **11:52:34.456**, Wave 2 Clear **11:53:03.366**,
Wave 3 Clear / Victory **11:53:44.142**를 확인했다. **11:53:45.638** Snapshot은
`Victory / Wave 3/3 / Alive 0 / Tracked 0 / Remaining 0 / TotalSpawned 31 / Score 3550 / HP 64/100 / Abilities 2`
이며 Player 입력과 Prepare / Spawn / Completion Timer가 모두 정지했다.
정상 실행에서 Wave 3의 최대 동시 생존 관측은 14마리다. 생성 도중 2마리가 죽었으므로
이 실행을 16마리 동시 밀집 검증으로 대신하지 않고 다음 별도 fixture를 수행했다.
Wave별 구성은 Native / JSON 모두 5G / 7G+3R / 10G+6R이며 최대 생존은 5 / 10 / 14다.
공격 입력 요청 166회 / Dash 요청 6회는 activation 성공 수와 구분하며, 강제 Kill / HP 변경 / Counter 변경 없이 완료했다.
이 자동 입력 전투의 88.500초를 인간 플레이의 목표 3~5분 달성 증거로 간주하지 않는다. 인간 난이도 / 소요 시간은 N/A다.

이전 정상 성공 `Step04_AutoPlay_20260903T114927_447553Z.json`의 Session `6A499087`
(145.282 wall초 / HP 100 / Score 3550)은 별도 회귀 이력으로 유지한다.
2026-09-04 20:51의 중간 실행은 Wave 2에서 29.937초 `ManualStop`했으므로 완료된 Victory 횟수에 포함하지 않는다.

### 16 Enemy 동시 밀집 fixture

`Saved/AgentComparison/Codex/Step04_Density.json`은 `Completed / passed=true / errors=[]`다.
Wave 1 / 2를 실제 Spawn 이후 개발용 GAS 처치로 넘기고 Player HP / MaxHealth를 9999로 설정한 **TEST 전용** 실행이다.
밀집 중 공격은 끄고 실제 이동 / Dash 입력만 사용했다. 최종 수정본에서 **20.51086 game초** 동안 30개 sample 모두
`Alive=16 / Grunt=10 / Runner=6 / Remaining=0 / outside_bounds=0`이었다.
Enemy 최소 중심 간 거리는 **84.0598cm**까지 관측했다. 이 결과는 시험 구간의 밀집 추적 / 경계 유지 증거이지
HP 100 난이도나 전체 FPS 성능 측정값은 아니다.

### Spawn 위치별 AI 도달 fixture

거리 기준은 Player에서 170cm 이내이며, 동일한 Wave Enemy 한 마리를 각 위치로 옮긴 뒤 실제 Walking을 관측했다.

| 실행 / 파일 | 결과 | 개별 도달 거리 cm / 실패 |
|---|---|---|
| 2026-09-03 20:54:37~20:56:11 KST / `Step04_SpawnArrivalFixture_20260903T115437_351892Z.json` | 5/6 | N 168.98, NE 161.09, S 149.77, SE 138.98, SW 151.17 성공. NW는 최소 203.46에서 20.075 game초 timeout |
| 2026-09-03 20:56:59~20:58:21 KST / `Step04_SpawnArrivalFixture_20260903T115659_818499Z.json` | 5/6 | NW 132.80, N 159.80, NE 143.68, S 147.80, SW 141.90 성공. SE는 최소 208.74에서 20.047 game초 timeout |

다른 Wave Enemy가 Player 주변에 남아 있어 접근을 물리적으로 막을 수 있는 fixture다.
NW / SE도 다른 실행에서 도달했으므로 해당 Spawn의 경로 단절로 판정하지 않는다.
두 실패 실행을 삭제하거나 단독 6/6 결과로 바꾸지 않는다.

### 확장 이동 / Dash / Collision

2026-09-04 **20:56:36 KST**에 저장된 최종 수정본의 `Saved/AgentComparison/Codex/Step04_Traversal.json`은
**36.156 wall초**, `Completed / passed=true / errors=[]`, **8/8 성공**이다.
이 fixture도 Player teleport와 HP / MaxHealth 9999를 setup에만 사용하는 TEST이며,
각 이동은 기존 Input Exec → Enhanced Input → GAS Dash 경로로 요청했다.
중간 sample의 위치·속도 / 바닥 높이를 함께 검사해 끝점만 보고 통과시키지 않았다.

| Case | 이동 거리 cm | 결과 |
|---|---:|---|
| Fence West / East / North / South | 각각 약 201.90 | 네 방향 경계에서 막힘, 외부 이탈 없음 |
| Container | 215.90 | Container 앞에서 막힘 |
| CentralPipe | 332.90 | 중앙 설비 앞에서 막힘 |
| Barrier | 180.40 | Barrier 앞에서 막힘 |
| OpenRoute | 604.68 | 열린 통로 이동 성공 |

최종 Sample 최대속도는 약 **1400cm/s**이며 모든 Case의 `movement_observed / ground_height_valid`가 true다.
앞선 20:42 재검증에서도 8/8이었고 Native 로그 **11:41:35.133~11:42:00.293 UTC**에
8회 `Dash Activated` 및 `Dash Ended Cancelled=false`를 확인했다. 최종 시험은 동일한 입력 경로에서 배치 수정 회귀를 수행했다.
이 결과는 시험한 Collision 경로에 대한 검증이고 Camera / Geometry의 전체 시각 품질을 대체하지 않는다.

### Geometry / Camera / 시각 품질

`Saved/AgentComparison/Codex/Step04_GeometryAudit.json`의 2026-09-04 **20:52:12 KST** 검사:
205개 Mesh Actor / 13종 Mesh를 대상으로 **9개 검사 모두 true, issues=[]**다.
Yaw-only 회전, 양수 Scale, Pivot / bounds / 원본 치수, 의도된 지면·Pallet support,
Simple 또는 Convex Collision을 확인했다. Camera probe는 13개 중 플레이 가능한 12개에서
Player 상부 Z=110.15cm 및 몸통 Z=80cm 지점으로 Simple / Complex Visibility Ray가 모두 clear다.
Warehouse 바깥의 이동 불가능한 probe 1개는 제외했다. 이를 전체 공간의 모든 위치 가림을 수학적으로 보장하는 검사로 표현하지 않는다.

실제 Top-Down 이미지 `Step04_OcclusionFinal.png`에서 Container에 바짝 접촉하면 Player 하부가 일부 가려지지만
cyan 상부와 위치는 식별 가능하다. `Step04_WarehouseCamera.png`에서는 Warehouse 옆 Player가 선명하다.
Final Victory / Density 화면에서도 전투 대상과 지면 구획을 확인했다.
Material / 배치 / Shadow를 반복 검토하고 Concrete 대비, 잘못된 회전, Container 높이, Barrel 겹침을 수정했다.
최종 확인 범위에서 심각한 Floating / Z-Fighting / Texture Stretch / 지속적 완전 가림은 발견하지 못했다.
Full-silhouette fade / outline은 만들지 않았으므로 부분 가림과 단순한 Art 표현은 잔여 품질 한계다.

### Melee Barrier / GameOver / Restart

- Melee Barrier: 2026-09-03 Session `76CC594C`에서 Player와 Enemy를 Barrier 양면에 놓고 Enemy 이동을 1초 정지했다.
  11:47:09.646 / 11:47:10.647 UTC Snapshot의 HP는 모두 100이다. 이동 재개 후 우회하여 공격했고
  11:47:12.640 Snapshot의 HP는 82다. 이는 Enemy melee의 환경 차단 / 우회 증거이며 Player Primary Attack의 Barrier 차단 시험으로 간주하지 않는다.
- 실제 GameOver: 2026-09-04 Session `C5AED231`, **11:42:33.441 UTC**에 Enemy melee `Health: 10 -> 0`,
  Player Death Event, GameOver 전환, Enemy 5개 Suspended / halted를 확인했다.
  **11:42:50.469** Snapshot은 HP 0/100, DeadTag true, Input false, Abilities 2, 모든 Game Loop Timer false다.
- GameOver Restart: **11:43:15.199 UTC**에 `CodexDebugRestartGameLoop`, 이후 `2437BD7B`로 같은 Arena를 다시 로드했다.
  **11:43:15.311** HP 100 / MaxHealth 100, Abilities New 2 / Total 2;
  **11:43:15.350** Preparing / Wave 0 / Alive 0 / Remaining 0 / Score 0,
  **11:43:17.846** Wave 1 시작을 확인했다. 무입력 상태를 계속 유지한 뒤 다시 Enemy에게 죽은 것은 Restart 실패가 아니다.
- Victory Restart: 2026-09-03 **11:53:00.493 UTC**, Session `6A499087`의 Victory에서 Restart 후 `A5E590CE`로 로드했다.
  **11:53:00.617** HP 100 / MaxHealth 100 / Abilities 2, 이후 Preparing / Score 0 / Wave 1을 확인했다.
- 최종 Victory Restart: 2026-09-04 **11:54:18.889 UTC**, `798B3020` Victory에서 Restart 후 `1760360C`로 로드했다.
  **11:54:19.014** HP 100 / MaxHealth 100 / Abilities New 2 / Total 2, 이후 Preparing / Score 0,
  **11:54:21.745** Wave 1 재시작을 확인했다. 이 뒤 밀집 시험을 위한 HP 변경은 Restart 이후 별도 TEST 설정이다.
- Screenshot 이력: 09-03의 `Step04_Victory.png`는 Restart와 같은 명령에서 Capture를 요청해 재시작 World를 찍은 잘못된 증거였다.
  최종 파일은 09-04 **20:54:08 KST**, 안정된 Victory에서 다시 촬영했다. 최종 Capture와 Restart를 분리했다.

최종 정상 세션 `798B3020`은 Fatal / Assertion / Ensure / Python Error / SpawnFailure /
Accessed None / Invalid ASC·Delegate / Navigation·Timer 핵심 오류 검색 0이다.
09-03의 이전 정상 Victory 중 Editor 설정 진단의 Python AttributeError 1건과 과거 handled ensure / Spawn 경고는 별도 이력에 보존했다.
따라서 "STEP 전체의 모든 도구 / 진단 오류 0"이라고 기록하지 않는다.
09-04의 `LogCrowdFollowing` RecastNavMesh 경고는 확인된 각 행이 `BeginTearingDown` 직후이며
새 World의 실제 Spawn 실패로 이어지지 않은 **비치명 cleanup 경고**로 분류했다.
과거에는 Editor Map Rebuild / Map Load / Engine 종료의 CleanupWorld 중에도 같은 경고가 있었으므로
STEP 전체를 모두 PIE teardown-only였다고 주장하지 않는다. Engine cleanup 경고를 숨기기 위한 C++ 수정은 하지 않았다.

## 완료조건 확인

| 구분 | 요구 수 | 최종 Completed / Failed |
|---|---:|---|
| Level | 6 | 6 / 0 |
| Environment | 7 | 7 / 0 |
| Asset | 6 | 6 / 0 |
| Texture / Material | 7 | 7 / 0 |
| Lighting | 4 | 4 / 0 |
| Gameplay | 6 | 6 / 0 |
| Spawn | 4 | 4 / 0 |
| Wave | 3 | 3 / 0 |
| Game Flow | 3 | 3 / 0 |
| Quality | 6 | 6 / 0 |
| Validation | 3 | 3 / 0 |
| 합계 / 최종 Completion Rate | **55** | **55 / 0 (100%)** |

아래 55개는 STEP 4의 명시 완료조건과 일대일 대응한다. Quality의 기준은 심각한 문제 유무이며
모든 미세한 품질 한계가 없다는 뜻은 아니다. 비교 파일 / 소요 시간 / Token의 최종 마감은 별도 완료 게이트다.

### Level — 6

- [x] 새 Arena Level 생성 — Codex 전용 신규 Level.
- [x] 약 50m × 50m 규모 — 플레이 영역 5000cm 정사각형.
- [x] Gameplay 가능한 Layout — 최종 정상 Victory와 밀집 시험.
- [x] Central Structure 존재 — Pipe Skid / Utility.
- [x] 여러 이동 Route 존재 — 중앙 Loop / 외곽 / 보조 통로.
- [x] Arena Boundary 자연스러움 — Fence / Curb / Warehouse.

### Environment — 7

- [x] 산업시설 컨셉 구현 — 물류 Yard / 낡은 Metal / Concrete.
- [x] Warehouse / Structure 존재.
- [x] Container 존재 — 전투용 낮은 Cargo / 배경 Shipping Container.
- [x] Barrier 존재.
- [x] Industrial Props 존재 — Barrel / Pallet / Utility / Lamp.
- [x] Ground 완성 — Concrete / Asphalt / Paint / Drain.
- [x] Background 비어있지 않음 — Warehouse / Container Stack.

### Asset — 6

- [x] Environment Mesh 직접 제작 또는 적절히 구성 — 자체 제작 13종.
- [x] Agent 독립 Asset 사용 — Codex 전용 원본 / Export / Content.
- [x] Scale 정상 — Manifest / Geometry 검사 및 의도된 낮은 Cargo 조정.
- [x] UV 정상 — UV0 / Tileable PBR, 심각한 Stretch 없음.
- [x] Pivot 정상 — 하단 중심 / Ground 윗면 기준.
- [x] Collision 정상 — UCX 및 최종 이동·Dash fixture.

### Texture / Material — 7

- [x] Gray Material 상태 아님.
- [x] Concrete Material.
- [x] Metal Material — Painted / Worn / Rust.
- [x] Ground Material — Concrete / Asphalt.
- [x] PBR Texture 사용 — CC0 2K 4세트 / 17장.
- [x] Texture Stretch 심각한 문제 없음 — 최종 화면 검토.
- [x] Material Instance 적절히 사용 — Master 1 / MI 16.

### Lighting — 4

- [x] 게임 화면으로 충분한 Lighting.
- [x] Player 식별 가능 — cyan 상부 / 위치 식별.
- [x] Enemy 식별 가능 — Grunt / Runner 대비와 밀집 화면.
- [x] Shadow 너무 어둡지 않음 — SkyLight / 노출 및 실제 화면 확인.

### Gameplay — 6

- [x] Player 이동 정상 — 입력 전투 / 확장 이동 fixture.
- [x] Dash 정상 — GAS activation / 실제 속도 및 Collision.
- [x] Attack 정상 — 정상 입력으로 31 Enemy 처치.
- [x] Enemy AI 정상 — 추적 / 우회 / 실제 melee.
- [x] Navigation 정상 — 6개 완전 경로 / 실제 이동.
- [x] Spawn 정상 — 최종 정상 31회 성공.

### Spawn — 4

- [x] SpawnPoint 4개 이상 — 6개.
- [x] 여러 방향 Spawn — 최종 모든 Point 사용.
- [x] Player 바로 옆 Spawn 방지 — 정책 1100cm / 최종 관측 최소 1405cm.
- [x] 모든 Spawn 위치 Navigation 정상 — 6개 경로 및 개별 AI 도달 증거.

### Wave — 3

- [x] Wave 1 Arena 테스트 성공 — 5 Grunt.
- [x] Wave 2 Arena 테스트 성공 — 7 Grunt / 3 Runner.
- [x] Wave 3 밀집 테스트 성공 — 10 Grunt / 6 Runner 동시 유지.

### Game Flow — 3

- [x] Victory 정상 — 최종 정상 88.500초 / 3550점.
- [x] Game Over 정상 — 실제 Enemy damage로 HP 0.
- [x] Restart 정상 — GameOver / Victory 양쪽 Level Reload.

### Quality — 6

- [x] Floating Mesh 심각한 문제 없음 — support / bounds / 화면 검사.
- [x] Z-Fighting 심각한 문제 없음 — 지면 / 도색 높이 분리 및 화면 검사.
- [x] Collision 심각한 문제 없음 — 최종 8/8 fixture.
- [x] Texture Stretch 심각한 문제 없음 — UV / 최종 Material 화면 검사.
- [x] Camera Occlusion 심각한 문제 없음 — 12개 이동 가능 probe / 실제 화면; 접촉 시 하부 부분 가림은 남음.
- [x] Level 밖 이동 불가 — Fence 네 방향 Dash와 밀집 bounds 확인.

### Validation — 3

- [x] Build 정상 — 2회 중 최종 성공, 실패 1회 보존.
- [x] PIE 테스트 완료 — 정상 전투와 TEST fixture 분리.
- [x] 치명적 Runtime Error 없음 — 최종 정상 실행 핵심 오류 0, 과거 오류 / cleanup 경고 분리.

## Performance / Known Issues / 자체 평가

- 13 Mesh 재사용, Master 1개와 MI Variation, 2K Texture 기본, 제한된 Point Light와 228개 생성 Actor로 규모를 제한했다.
- 매우 단순한 Prop까지 Nanite를 강제하지 않는다. FPS / GPU 시간 / Draw Call / Texture Memory 수치는 실측 전 **N/A**다.
- Known Issues — 비치명: Container와 접촉하면 Player 하부 일부가 가려진다. cyan 상부 / 위치는 식별 가능하지만 Full-silhouette fade / outline 시스템은 만들지 않았다.
- Known Issues — 비치명: PIE 종료 / Restart cleanup의 CrowdFollowing RecastNavMesh 경고가 남아 있다. 과거 Editor Map Rebuild / Map Load / 종료 중 경고도 기록했고 최종 Gameplay의 Spawn 실패와 구분했다.
- 인간 플레이의 3~5분 목표 시간 / 난이도 실측은 N/A다. 입력 자동화의 빠른 정상 Victory는 게임루프 증거이지 사람의 체감 난이도 평가가 아니다.
- Editor startup 자동 종료 원인은 확인했고 일반 map launch로 복구하여 후속 PIE를 수행했다. 테스트용 Editor 설정을 복원했으며 아래 종료 기록에 근거를 남겼다.
- 위 한계를 숨겨 `Known Issues: None`으로 기록하지 않는다. 55개 명시 완료조건 및 비교 기록을 마감하여 STEP 5 진행이 가능하다.
- 자체 평가(각 10점, 타 Agent 결과를 열람하지 않은 자체 평가): **Code Quality 8 / Architecture 8 / Feature Completeness 10 / Stability 8 / Visual Quality 6 / Tool Efficiency 4 / Autonomy 9**.
- 평가 근거: 기존 구조 보존과 이벤트 기반 Gameplay는 양호하다. 기능 검증은 충족했으나 단순한 Art 표현, 부분 가림, 여러 도구 / API / 배치 수정 시행착오 때문에 Visual / Tool Efficiency 점수를 낮췄다.
- `Metrics.csv` / `Summary.md`: 최종 계측값과 STEP 4 결과를 반영했다.

## 증거 파일

- `ExternalAssets/LastStand/Codex/Models/Step04BlenderKitManifest.json`
- `ExternalAssets/LastStand/Codex/Models/LS_Codex_Environment.blend`
- `ExternalAssets/LastStand/Codex/Textures/manifest.json`, `README.md`
- `Saved/AgentComparison/Codex/Step04_Import.log`, `Step04_Import_02.log`, `Step04_Import_03.log`
- `Saved/AgentComparison/Codex/Step04_ImportReport.json`
- `Saved/AgentComparison/Codex/Step04_Readability.json`
- `Saved/AgentComparison/Codex/Step04_LevelReport.json`
- `Saved/AgentComparison/Codex/Step04_EditorAudit.json`, `Step04_RuntimeAudit.json`
- `Saved/AgentComparison/Codex/Step04_Build02.log`, `Step04_Build02.json`
- `Saved/AgentComparison/Codex/Step04_WallRepro.png`
- `Docs/AgentComparison/Codex/Evidence/Step04_BlenderKit.png`
- `Saved/AgentComparison/Codex/Step04_AutoPlay_20260903T114927_447553Z.json`
- `Saved/AgentComparison/Codex/Step04_AutoPlay_20260904T115215_703622Z.json`
- `Saved/AgentComparison/Codex/Step04_Density.json`, `Step04_Traversal.json`
- `Saved/AgentComparison/Codex/Step04_GeometryAudit.json`
- `Saved/AgentComparison/Codex/Step04_SpawnArrivalFixture_20260903T115437_351892Z.json`
- `Saved/AgentComparison/Codex/Step04_SpawnArrivalFixture_20260903T115659_818499Z.json`
- `Saved/Logs/CodexGame-backup-2026.09.03-12.01.34.log`, `Saved/Logs/CodexGame.log` (2026-09-04 GameOver / Restart)
- `Docs/AgentComparison/Codex/Evidence/Step04_Victory.png`, `Step04_Density.png`
- `Docs/AgentComparison/Codex/Evidence/Step04_OcclusionFinal.png`, `Step04_WarehouseCamera.png`
- `Docs/AgentComparison/Codex/Evidence/Step04_OcclusionBefore.png`, `Step04_OcclusionAfter.png` (수정 전 / 중간 실패 이력)

## 계측 범위와 종료 확인

- Start 2026-09-02 21:40:11 KST, 기록 계측 마감 2026-09-04 21:06:50 KST. 47h 26m 39s는 야간 중단과 재개 대기를 포함한다. 실제 active time은 N/A이며 wall time을 순수 구현 시간으로 표현하지 않는다. 이 마감 이후 파일 저장·검증·최종 응답은 계측에 포함하지 않는다.
- Root telemetry 경계: 직전 2026-09-02T12:39:47.611Z, STEP 첫 이벤트 12:40:11.988Z, 마지막 계측 2026-09-04T12:06:21.290Z. Input 37,505,582 (그중 cached 36,412,544), Output 96,623, Total 37,602,205. Reasoning 27,486은 Output의 부분집합이므로 더하지 않는다. 반복된 context 입력의 누적값이며 고유 텍스트 크기/요금/전체 agent 합계가 아니다. Child 사용량을 별도 합산하지 않았다.
- Context Start 88,126/258,400, End 206,478/258,400은 해당 telemetry의 last input 값이다. 사용자에게 보이는 account usage 비율과는 다르다.
- Metrics RuntimeErrors=1은 과거 QA NavCDO handled ensure 발생을 보존한 값이다. Spawn 경고 98회와 Python/MCP/API 실패는 별도 이력에 기록했고 최종 gameplay 치명 오류는 관측되지 않았다. 정확한 STEP 전체 Debug Iterations/Unreal MCP calls는 N/A다.
- 최종 배치 GameOver: Session D51F3084, 2026-09-04T11:57:12.840Z. 적 실제 melee → HP0, AI5개 정지, 모든 game loop Timer false. 11:57:42.763Z Restart → 915EB15F; 11:57:48.303Z Wave1 / HP100 / Score0 / DeadTag false / Abilities2 / Input true 확인.
- 최종 Traversal 8/8, 밀집 fixture 30 samples/20.51086s PASS. Geometry/Editor audit PASS. 최종 Wave/Victory 정상 입력 run은 798B3020이며 별도 fixture와 혼합하지 않는다.
- NavMesh Debug flag와 캡처를 시도했으나 해당 Editor 캡처에서 녹색 polygon overlay가 선명하지 않아 `Step04_Navigation.png`를 경로 도달 성공 증거로 사용하지 않는다. 6개 projection/full path, 6개 위치별 실제 AI 도달, 최종 전체 Wave/밀집 전투를 Navigation 검증 근거로 삼는다.
- PIE를 종료하고 테스트 callback을 정리했다. Nav draw offset 10 / polygon edges false로 복원, Navigation overlay 해제, `bThrottleCPUWhenNotForeground=True` 원복을 2026-09-04T12:03:13.585Z 로그로 확인했다. Arena를 저장해 Editor에 남겼다.
- `Metrics.csv`는 Spreadsheets artifact-tool로 기존 20열/이전 3행을 보존하여 STEP4 한 행을 검증·추가했다. `Summary.md` STEP4 행을 갱신했다. STEP5 진행 가능하나 별도 지시 전 착수하지 않는다. Commit/Push는 수행하지 않았다.
