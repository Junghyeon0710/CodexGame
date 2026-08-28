# PROJECT: LAST STAND — STEP 1 Player + GAS Foundation

## STEP 상태

| 항목 | 값 |
|---|---|
| Agent | Codex |
| Unreal Engine | 5.8.2 (CL 56702186) |
| Start Time | 2026-08-27 21:30:49 +09:00 |
| End Time | 2026-08-28 14:21:47 +09:00 |
| Elapsed Time | 16시간 50분 58초 (1010.98분) |
| 결과 | STEP 1 완료, STEP 2 미진행 |
| Test Level | `/Game/AgentGameTest/Codex/Levels/L_LastStand_PlayerTest_Codex` |

이번 STEP에서는 Player, Enhanced Input, GAS Attribute/Ability/Effect/Tag, 기본 공격, Dash, 정지형 GAS Test Target, 전용 Test Level만 구현했다. Enemy, AI, Wave, Score, UI, 최종 Arena, Niagara, Audio 등 후속 STEP 범위는 만들지 않았다.

## Architecture

- ASC 위치: `ACodexLSPlayerState`
- Owner Actor: `ACodexLSPlayerState`
- Avatar Actor: `ACodexLSPlayerCharacter`
- 이유: Pawn 재생성 시 GAS 상태를 PlayerState에 유지할 수 있고, 향후 Death/Restart 확장에 자연스러우며 현재 미니게임 규모에서 별도 대형 Framework가 필요하지 않다.
- Ability Grant: Server Authority에서 1회 수행하며 기존 Ability Class를 검색해 중복 Grant를 막는다.
- Attribute 초기화: Instant `UCodexLSGE_DefaultAttributes`를 PlayerState당 1회만 적용한다. `Health=100`, `MaxHealth=100`이다.
- 실제 런타임 로직은 native C++ Class가 담당한다. `/Game`의 GA/GE/BP는 편집·검사용 data-only 파생 Blueprint Asset이다.

## 입력과 GAS 흐름

```text
IA_PrimaryAttack / IA_Dash
        ↓ Enhanced Input
InputTag.Ability.PrimaryAttack / InputTag.Ability.Dash
        ↓
UCodexLSAbilitySystemComponent::AbilityInputTagPressed
        ↓ DynamicSpecSourceTags 검색
UCodexLSGA_PrimaryAttack / UCodexLSGA_Dash
```

이동은 `IA_Move` Axis2D를 Camera Yaw 기준 World Direction으로 변환하고 입력 Vector를 최대 길이 1로 Clamp한다. Mouse Aim은 Screen Position을 Deproject한 Ray와 Player 높이의 Ground Plane 교차점으로 계산하며 Character 회전과 공격 방향이 같은 Aim Vector를 사용한다.

## 전투 구현

### Primary Attack

- 방식: Mouse Aim 기반 1,800 cm Hitscan
- Trace: `ECC_Visibility`, Player 자신 제외
- Feedback: 0.35초 Debug Line과 Hit Sphere
- Fire Rate: GAS Cooldown GameplayEffect 0.3초
- Ability는 공격 직후 정상 종료한다.

```text
Left Mouse
→ InputTag.Ability.PrimaryAttack
→ UCodexLSGA_PrimaryAttack
→ Mouse Aim Hitscan
→ Target ASC
→ UCodexLSGE_Damage Spec
→ SetByCaller(Data.Damage=20)
→ UCodexLSAttributeSet.IncomingDamage
→ Health = Clamp(Health - Damage, 0, MaxHealth)
```

AttributeSet 외부에서 `Health -= Damage` 형태로 HP를 우회 감소시키지 않는다.

### Dash

- 방식: `LaunchCharacter`
- 속도/Ability Lifetime: 1,400 cm/s, 0.18초
- 방향: 현재 이동 입력 우선, 입력이 없으면 Mouse Aim 방향
- 대각선: 정규화된 World Direction 사용
- State: `State.Player.Dashing`은 Ability 활성 동안 존재하고 정상 종료·Cancel·PIE 종료 시 제거된다.
- Cooldown: GAS GameplayEffect 3초, `Cooldown.Player.Dash`
- 종료 정리: 0.18초 종료 시 Dash 수평 속도를 즉시 정리하고, 계속 누른 일반 이동 입력은 다음 Tick부터 정상 처리한다.
- Cost/Stamina: 없음. STEP 1 범위에 없는 Resource Attribute를 추가하지 않았다.

## 생성한 C++ Class

| 영역 | Class |
|---|---|
| GAS | `UCodexLSAbilitySystemComponent` |
| Attribute | `UCodexLSAttributeSet` |
| Ability Base | `UCodexLSGameplayAbility` |
| Ability | `UCodexLSGA_PrimaryAttack`, `UCodexLSGA_Dash` |
| GameplayEffect | `UCodexLSGE_DefaultAttributes`, `UCodexLSGE_Damage`, `UCodexLSGE_PrimaryAttackCooldown`, `UCodexLSGE_DashCooldown` |
| Player | `ACodexLSPlayerCharacter`, `ACodexLSPlayerController`, `ACodexLSPlayerState` |
| Game | `ACodexLSGameMode` |
| Test | `ACodexLSGASTestTarget` |

- C++ Class: 14
- C++ Files Created: 21
- Existing C++ Files Modified: 1 (`CodexGame.Build.cs`)

`ACodexLSPlayerController`에는 실제 PIE 자동 검증을 위한 `CodexDebugInputChord`와 `CodexDebugSetMouse` Exec 명령을 두었다. 두 명령 모두 Enhanced Input/PlayerController 입력 경로를 통과시키는 QA 보조 기능이며 게임 시스템을 대체하지 않는다.

## 생성한 Unreal Asset

| 종류 | Asset |
|---|---|
| Player Blueprint | `/Game/AgentGameTest/Codex/Blueprints/BP_LastStand_Player_Codex` |
| GameMode Blueprint | `/Game/AgentGameTest/Codex/Blueprints/BP_LastStand_GameMode_Codex` |
| Test Target Blueprint | `/Game/AgentGameTest/Codex/Blueprints/BP_GAS_TestTarget_Codex` |
| GameplayAbility | `/Game/AgentGameTest/Codex/Abilities/GA_Player_PrimaryAttack` |
| GameplayAbility | `/Game/AgentGameTest/Codex/Abilities/GA_Player_Dash` |
| GameplayEffect | `/Game/AgentGameTest/Codex/Effects/GE_Player_DefaultAttributes` |
| GameplayEffect | `/Game/AgentGameTest/Codex/Effects/GE_Damage` |
| GameplayEffect | `/Game/AgentGameTest/Codex/Effects/GE_Cooldown_PrimaryAttack` |
| GameplayEffect | `/Game/AgentGameTest/Codex/Effects/GE_Cooldown_Dash` |
| InputAction | `/Game/AgentGameTest/Codex/Input/IA_Move` |
| InputAction | `/Game/AgentGameTest/Codex/Input/IA_PrimaryAttack` |
| InputAction | `/Game/AgentGameTest/Codex/Input/IA_Dash` |
| InputMappingContext | `/Game/AgentGameTest/Codex/Input/IMC_Player` |
| Level | `/Game/AgentGameTest/Codex/Levels/L_LastStand_PlayerTest_Codex` |

총 Unreal Asset은 14개다. Test Level에는 Floor, PlayerStart, DirectionalLight, SkyLight, 서로 다른 세 방향의 GAS Test Target 3개가 있으며 GameMode가 Player를 Spawn한다. 외부 Asset은 사용하지 않았고 Engine 기본 Cube/Cone만 사용했다.

## GameplayTag

| Tag | 용도 |
|---|---|
| `InputTag.Ability.PrimaryAttack` | 기본 공격 입력 |
| `InputTag.Ability.Dash` | Dash 입력 |
| `Ability.Player.PrimaryAttack` | 기본 공격 Ability 식별 |
| `Ability.Player.Dash` | Dash Ability 식별 |
| `State.Player.Dashing` | Dash 활성 상태 |
| `Cooldown.Player.PrimaryAttack` | 공격 0.3초 Cooldown |
| `Cooldown.Player.Dash` | Dash 3초 Cooldown |
| `Data.Damage` | SetByCaller Damage Magnitude |

Unreal GameplayTags Toolset에서 위 8개 Tag 등록을 실제 조회했다.

## Build 기록

| Build | 결과 | 시간 | 내용 / 실패 원인 |
|---|---:|---:|---|
| #1 | SUCCESS | 89.51초 | 초기 C++ Editor Build, 16 actions |
| #2 | SUCCESS | 31.17초 | Cooldown GE CDO 생성 방식 수정 후 Build |
| #3 | FAIL | 20.89초 | Sandbox가 UBT AppData/Trace 쓰기를 차단. 코드 Compile 전 환경 실패 |
| #4 | SUCCESS | 3.06초 | 동일 Build를 허용된 환경에서 재실행, up to date |
| #5 | SUCCESS | 21.63초 | Dash 속도 정리와 Attribute 1회 적용 가드 Build |
| #6 | FAIL | 26.99초 | QA 명령의 지역 변수 `Character`가 `AController::Character`를 가린 C4458 1건 |
| #7 | SUCCESS | 6.16초 | 변수명 수정 후 Build |
| #8 | SUCCESS | 15.44초 | 최종 Mouse Aim QA 명령 포함 Build |

- Build Attempts: 8
- Successful Builds: 6
- Failed Builds: 2
- Compile Errors: 1, 해결 완료
- Compile Warnings: 0
- 최종 Build: SUCCESS

Data-only Blueprint/GA/GE 9개는 `warnings_as_errors=true`로 두 차례, 총 18회 Compile하여 전부 성공했다.

## Asset 생성 Commandlet 기록

총 6회 실행했다.

1. Sandbox의 AppData/UBT/DDC 접근 거부
2. GameplayEffect Component를 Constructor에서 빈 이름으로 `NewObject`한 CDO Fatal
3. Unreal Python `unreal.Key(key_name=...)` API 오류
4. DirectionalLight Component Python Property 접근 오류
5. Asset 생성 성공 Marker 출력, 기존 `AllToolsets/GameFeatures` 설정 오류로 Process Exit 1
6. IMC Modifier 저장 수정 후 Asset 재생성 성공 Marker 출력, 같은 기존 설정 오류로 Process Exit 1

최종 Marker:

```text
CODEX_STEP1_ASSET_BUILD_SUCCESS input_assets=4 blueprint_assets=9 level=/Game/AgentGameTest/Codex/Levels/L_LastStand_PlayerTest_Codex
```

## PIE 테스트

| PIE | 결과 | 검증 내용 |
|---|---|---|
| #1 | FAIL 후 수정 | 공격/HP/Dash는 동작했지만 Python 생성 IMC의 Modifier가 저장되지 않아 W/A/S/D가 같은 축으로 입력됨 |
| #2 | SUCCESS | 수정된 W/A/S/D, 공격 HP 연속 감소, 공격/Dash Cooldown, State Tag, 기본 캡처 확인 |
| #3 | PARTIAL | Dash 정리 수정 후 실행. 외부 Win32 chord 입력이 PIE 키 포커스에 도달하지 않아 QA 입력 방법을 내부 `InputKey` 방식으로 교체 |
| #4 | SUCCESS | W/A/S/D/WA/WD/SA/SD, 방향별 Dash, 연타 차단, 3초 후 재사용, State/Cooldown 분리 확인 |
| #5 | SUCCESS | Mouse Aim 상/하/좌/우/대각선, Hit/Miss, 공격 Cooldown, 과잉 Damage Clamp, Dash Cancel/PIE 종료 정리 확인 |

- PIE Test Runs: 5
- 최종 결과: SUCCESS
- Blueprint Runtime Errors: 0
- Accessed None: 0
- Runtime Errors: 0
- GAS Warning / Invalid GameplayTag / Invalid ASC: 0

### 이동 결과

| 입력 | 확인된 World Direction |
|---|---|
| W | `(1.00, 0.00)` |
| A | `(0.00, -1.00)` |
| S | `(-1.00, 0.00)` |
| D | `(0.00, 1.00)` |
| WA | `(0.71, -0.71)` |
| WD | `(0.71, 0.71)` |
| SA | `(-0.71, -0.71)` |
| SD | `(-0.71, 0.71)` |

대각선은 길이 1로 정규화됐고, 입력 해제 후 0.5초 간격으로 두 번 조회한 위치가 동일해 정지를 확인했다.

### Mouse Aim / Hit / Miss 결과

| Mouse 위치 | Aim Direction | 결과 |
|---|---|---|
| 화면 우측 | `(-0.04, 1.00)` | Target Hit |
| 화면 좌측 | `(-0.04, -1.00)` | Miss |
| 화면 상단 | `(1.00, 0.00)` | Target Hit |
| 화면 하단 | `(-1.00, 0.00)` | Target Hit |
| 우상단 대각선 | `(0.68, 0.73)` | Miss, Character Yaw 약 46.84도 |

Miss에서 Null Pointer, Invalid ASC, Accessed None은 발생하지 않았다.

### Primary Attack / Damage 결과

- 실제 입력 연결 로그: `InputTag.Ability.PrimaryAttack | Matched=true Activated=true`
- 0.3초 Cooldown 중 두 번째 입력: `Matched=true Activated=false`
- 활성 Effect: `Default__CodexLSGE_PrimaryAttackCooldown`, Duration 약 0.3초
- 활성 Tag: `Cooldown.Player.PrimaryAttack`
- Target HP: `100 → 80 → 60 → 40 → 20 → 0`
- 0 HP에서 추가 Damage 20: `0 → 0`
- MCP 최종 Attribute: `Health=0`, `MaxHealth=100`, `IncomingDamage=0`

### Dash 결과

- W: `(1.00, 0.00)`
- S: `(-1.00, 0.00)`
- A: `(0.00, -1.00)`
- D: `(0.00, 1.00)`
- WA: `(0.71, -0.71)`
- WD: `(0.71, 0.71)`
- Dash 시작 시 Tag: `Cooldown.Player.Dash`, `State.Player.Dashing`
- 0.18초 종료 후: `State.Player.Dashing` 제거, Cooldown Tag/Effect만 유지
- Cooldown Effect: `Default__CodexLSGE_DashCooldown`, Duration 3초
- 첫 Dash 직후 재입력: `Activated=false`
- 3초 후 다음 방향 Dash: `Activated=true`
- Dash 종료 후 위치를 0.5초 간격으로 재조회해 잔류 속도 없이 정지함을 확인
- Dash 중 PIE 종료: `Dash Ended | Cancelled=true | StateTag=removed`

## GAS 초기화 결과

- ASC Owner: PlayerState 정상
- ASC Avatar: Character 정상
- Granted Abilities: `CodexLSGA_Dash` Level 1, `CodexLSGA_PrimaryAttack` Level 1
- Ability 총수: 매 PIE에서 2, 중복 없음
- Player Attribute: Health 100, MaxHealth 100, IncomingDamage 0
- Default Attribute Effect: 정상 적용, PlayerState당 중복 적용 방지
- PIE 정상 종료 시 Ability Cancel 및 ActorInfo 정리

## 요구사항 완료율

| 그룹 | 요구사항 | 완료 | 실패 |
|---|---:|---:|---:|
| Player | 5 | 5 | 0 |
| GAS | 8 | 8 | 0 |
| Attack | 8 | 8 | 0 |
| Dash | 7 | 7 | 0 |
| Validation | 5 | 5 | 0 |
| 합계 | 33 | 33 | 0 |

Completion Rate: **100%**

## 오류와 Tool 기록

| 항목 | 값 |
|---|---:|
| Debug Iterations | 8 |
| Unreal MCP Calls | 248 |
| Unreal MCP Failures | 11 |
| Blender MCP Calls / Failures | 0 / 0 |
| Computer-use Capture Attempts / Failures | 2 / 2 |
| Blueprint Compile Errors | 0 |
| Blueprint Runtime Errors | 0 |
| Accessed None | 0 |
| Gameplay Runtime Errors | 0 |

Unreal MCP 11회 실패는 잘못된 SceneTools 이름/필수 인자 5회, 오래된 PIE Actor Ref 1회, Slate `locator` 대신 `ref`가 필요한 호출 3회, 잘못된 Blueprint Toolset 이름 1회, 이미 종료된 PIE에 Stop 요청 1회다. 모두 원인을 확인하고 올바른 호출 또는 내부 QA 경로로 재검증했다. MCP 수치는 HTTP initialize/notification이 아닌 meta `tools/call` 기준이다.

Computer-use 화면 캡처는 장치의 `SetIsBorderRequired` 미지원으로 2회 실패하여 중단했고, 이후 Unreal MCP Slate/Inspector와 런타임 GAS Inspector를 사용했다.

## 생성 Asset 집계

| 항목 | 수량 |
|---|---:|
| C++ Files Created | 21 |
| C++ Files Modified | 1 |
| General Blueprints Created | 3 |
| GameplayAbilities Created | 2 |
| GameplayEffects Created | 4 |
| InputActions Created | 3 |
| InputMappingContexts Created | 1 |
| Levels Created | 1 |
| Materials / MaterialInstances / Textures | 0 / 0 / 0 |
| StaticMeshes Created | 0 |
| NiagaraSystems / Widgets | 0 / 0 |
| Total Unreal Assets Created | 14 |

External Assets: None. Engine 기본 `/Engine/BasicShapes/Cube`와 `Cone`만 사용했다.

## 변경량

| 항목 | 수량 |
|---|---:|
| Files Added | 43 |
| Files Modified | 3 |
| Files Deleted | 0 |
| 구현 Text Lines Added | 2,226 |
| 구현 Text Lines Deleted | 2 |

Line 수치는 문서와 binary Unreal Asset을 제외하고 C++/config/script 변경만 집계했다. File 수치는 문서와 PIE 캡처를 포함해 STEP 1 시작 Commit 대비 최종 전달 변경을 집계했다.

## 측정값

- Token Usage: N/A
- Context Usage: N/A
- Usage Limit Data: N/A
- User Intervention Count: 0
- Manual Action Requests: 0

## Known Issues

- 프로젝트 시작 시 기존 `AllToolsets/GameFeatures` 구성에서 `GameFeatureData` AssetManager Rule 누락 오류 1건이 남아 있다. STEP 1 Gameplay/GAS Runtime 오류가 아니며 Editor Build, Blueprint Compile, PIE와 GAS 검증은 정상 통과했다. 이 기존 오류 때문에 asset-generation commandlet는 성공 Marker를 출력한 뒤 Process Exit 1을 반환한다.
- Online/Multiplayer용 Aim TargetData와 서버 재판정은 명시적으로 범위 밖이다. 현재 요구 범위인 Standalone PIE에서 검증했다.
- Test Level은 STEP 1 검증용 Engine 기본 Primitive 구성이다. 최종 Arena/Material/VFX는 이후 STEP 범위다.

## 자체 평가

| 항목 | 점수 / 10 |
|---|---:|
| Code Quality | 9 |
| Architecture | 9 |
| Feature Completeness | 10 |
| Stability | 9 |
| Visual Quality | 6 |
| Tool Efficiency | 7 |
| Autonomy | 10 |

## 결론

STEP 1의 구현, C++ Build, data-only Blueprint Compile, 실제 PIE 입력/전투/Dash/Clamp/Cancel 검증과 기록을 완료했다. STEP 2를 진행할 기반은 준비됐지만 이번 작업에서는 시작하지 않는다.
