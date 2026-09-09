# PROJECT: LAST STAND — STEP 5 HUD + CommonUI

## STEP 5 최종 결과

| 항목 | 현재 기록 |
|---|---|
| Agent | Codex |
| Unreal Engine | 5.8.2 |
| Start Time | 2026-09-07 21:06:57 +09:00 |
| End Time | 2026-09-09 10:44:33 +09:00 — 최종 기록 계측 마감 |
| Elapsed Time | 37h 37m 36s / 2257.60분 — 중단·재개 포함 wall time, active time N/A |
| Requirements | 52 — STEP 5의 명시 완료조건 기준 |
| Completed / Failed | 50 / 2 |
| Completion Rate | 96.2% — 정확한 1920×1080 실기 화면과 raw 물리 ESC → Pause 미검증을 각각 미완료로 계산 |
| 결과 | CommonUI 기반 Main Menu → Gameplay → Pause / GameOver / Victory → Restart / Main Menu 흐름 구현 및 PIE 검증 완료 |
| 대상 Level | `/Game/AgentGameTest/Codex/Levels/L_LastStand_Arena_Codex` |
| User Intervention Count | 0 |
| Manual Action Count | 0 — 사용자의 Editor 수동 조작 요청 없음 |
| Token / Context | N/A — 현재 실행에서 신뢰 가능한 누적 telemetry를 제공받지 못함 |
| Usage Limit Data | N/A |

STEP 5 범위의 실제 Gameplay UI와 화면 흐름을 구현했다. 최종 C++ Build와 Widget Blueprint 7/7 Compile,
7회의 PIE 실행, 이벤트 기반 HUD, Menu Focus, 반복 Restart / Main Menu 복귀, GameOver / Victory를 검증했다.
정확한 1920×1080 실기 창과 raw 물리 ESC → Pause, 물리 Gamepad Accept / Back은 현재 환경에서 검증하지 못했으므로 100%로 기록하지 않는다.
STEP 6 VFX / Polish는 시작하지 않았다.

## 범위와 보존

- UI Asset은 `/Game/AgentGameTest/Codex/UI/` 아래에만 생성했다.
- 기존 STEP 1~4의 Player, GAS, Enemy, Wave, Score, Arena 구조를 유지하고 UI 연결에 필요한 최소 변경만 했다.
- 최종 Niagara, Audio, Save, Inventory, Quest, Online, 산업시설 Arena 재작업은 하지 않았다.
- 전체 화면을 한 장의 이미지로 굽지 않았다. 모든 화면은 편집 가능한 UMG/CommonUI Widget Tree로 구성했다.
- 다른 Agent의 Asset과 코드는 열거나 수정하지 않았다.

## UI Architecture

```text
ACodexLSPlayerController
└─ UCodexLSUIManagerComponent
   └─ W_PrimaryGameLayout_Codex (UCodexLSPrimaryGameLayout)
      ├─ UI.Layer.Game  → W_GameHUD_Codex (persistent)
      ├─ UI.Layer.Menu  → Main / Pause / GameOver / Victory
      └─ UI.Layer.Modal → 이후 확장용 예약 Layer

ACodexLSGameState events ───────┐
Player AbilitySystemComponent ──┼→ W_GameHUD_Codex
GamePhase event ────────────────┴→ UIManager result routing
```

- Root는 `UCommonUserWidget`, 각 Layer는 실제 `UCommonActivatableWidgetStack`이다.
- HUD와 Menu/Result를 서로 다른 Stack으로 분리했다. HUD는 한 번 Push한 뒤 Visibility만 전환한다.
- Main Menu, Pause, GameOver, Victory는 `UCommonActivatableWidget` 계열이다.
- `UI.Layer.Game`, `UI.Layer.Menu`, `UI.Layer.Modal` GameplayTag로 Layer를 식별한다.
- UI 정책은 local `PlayerController`가 소유한 `UCodexLSUIManagerComponent`에 두고, Gameplay 데이터는 `GameState`와 GAS가 소유한다.
- `CommonGameViewportClient`, CommonUI plugin, CommonInput data를 프로젝트 설정에 연결했다.

이 규모에서는 별도 전역 UI Subsystem보다 PlayerController 소유 Component가 수명과 Restart 경계를 명확히 하면서도
하나의 거대한 GameMode를 피할 수 있어 적합하다고 판단했다.

## CommonUI / Input / Focus

- Gameplay 화면은 `ECommonInputMode::Game`, Menu 화면은 `ECommonInputMode::Menu`를 반환한다.
- Menu 진입 시 `FInputModeUIOnly`, Gameplay 복귀 시 `FInputModeGameAndUI`를 적용한다.
- Gameplay 중 Cursor는 Crosshair, Menu 중 Cursor와 Click / Hover를 활성화한다.
- 기본 Focus는 Main Menu `PLAY`, Pause `RESUME`, Result `RESTART`다.
- `NativeGetDesiredFocusTarget()`과 `RequestRefreshFocus()`를 사용하고, 다음 Tick에 Input Mode와 Focus를 한 번 더 적용해 생성 직후 Focus 경쟁을 해결했다.
- 공용 `UCodexLSCommonButton`은 Focus 가능하며 Hover / Pressed / Focus에 따라 Label 색상이 바뀐다.
- Pause Widget의 CommonUI Back은 `RESUME`으로 연결되고 Main Menu Back은 안전하게 소비한다.
- Mouse Click은 핵심 메뉴 흐름에서 확인했다. Keyboard Arrow / Enter는 이전 PIE에서 Slate `PressKey → PLAY_ACCEPTED / RESTART`가 확인됐지만, 최종 post-build PIE #7에서는 동일 raw 입력 활성화가 재현되지 않아 packaged 물리 키보드 경로는 N/A로 보존한다.
- Synthetic Gamepad D-Pad로 Focus 이동과 CommonInput의 Gamepad 전환은 확인했다. 물리 Gamepad Confirm / Back은 N/A다.

## 생성한 UI Asset

| 구분 | Asset | 역할 |
|---|---|---|
| Style | `Styles/BP_UI_LastStandButtonStyle_Codex` | Normal / Hovered / Pressed / Disabled 공용 Button Style |
| Common | `Common/W_CommonButton_Codex` | Focus 가능한 재사용 Button + `CommonTextBlock` Label |
| Layout | `Layout/W_PrimaryGameLayout_Codex` | Game / Menu / Modal Stack Root |
| HUD | `HUD/W_GameHUD_Codex` | HP, Wave, Enemy Count, Score, Dash, Phase / Announcement |
| Menu | `Menu/W_MainMenu_Codex` | PLAY / EXIT |
| Menu | `Menu/W_PauseMenu_Codex` | RESUME / RESTART / MAIN MENU |
| Result | `Result/W_GameOver_Codex` | Final Score / RESTART / MAIN MENU |
| Result | `Result/W_Victory_Codex` | Final Score / RESTART / MAIN MENU |

- Widget Blueprint: 7개.
- CommonButtonStyle Blueprint: 1개.
- Unreal Content Asset 신규 생성: 총 8개.
- UI Texture / Icon: 0개. 이번 화면은 직접 만든 UMG Border, Text, ProgressBar와 Style로 충분해 외부 이미지 Asset을 추가하지 않았다.
- 기존 `BP_GameMode_Arena_Codex`는 Main Menu boot 설정을 위해 수정했으며 신규 Asset 수에는 포함하지 않았다.

## HUD Data Flow

| 표시 | 실제 Source | 업데이트 방식 |
|---|---|---|
| HP / Max HP | Player `AbilitySystemComponent`의 `UCodexLSAttributeSet` | Health / MaxHealth attribute change delegate |
| Wave | `ACodexLSGameState` | `OnWaveChanged(CurrentWave, MaxWave)` |
| Enemy Count | `ACodexLSGameState` | `OnAliveEnemyCountChanged` |
| Score | `ACodexLSGameState` | `OnScoreChanged` |
| Game Phase | `ACodexLSGameState` | `OnGamePhaseChanged` |
| Dash Cooldown | Player ASC의 실제 `Cooldown.Player.Dash` GameplayTag와 활성 GameplayEffect | Tag New/Removed event, 활성 중에만 0.05초 Timer로 remaining/duration 조회 |

HUD 자체 Tick은 사용하지 않는다. Wave / Enemy / Score / Phase와 Health는 모두 Event 기반이다.
Dash만 Cooldown Tag가 활성인 짧은 시간 동안 실제 GameplayEffect의 remaining / duration을 읽어 ProgressBar와 `DASH 3.0 → READY`를 갱신한다.
Widget 비활성화 / 파괴 시 GameState Dynamic Delegate, ASC Attribute Delegate, GameplayTag Event와 Timer를 모두 해제한다.

## 화면별 동작

### Main Menu

- Level 진입 시 GameMode가 Wave Timer를 시작하지 않고 `GamePhase=None`에서 대기한다.
- `PLAY`는 Menu Stack을 Pop하고 HUD를 보인 뒤 준비시간과 Wave 1을 시작한다.
- `EXIT`는 PIE에서 Editor를 닫지 않는 안전한 no-op이며, 비 Editor 실행에서는 `QuitGame`을 호출한다.
- Mouse Click과 CommonUI desired focus를 확인했다. Keyboard Enter 실행은 이전 PIE에서 확인했지만 최종 post-build PIE #7 자동화에서는 재현되지 않았다.

### Pause

- PlayerController의 Escape binding은 Pause 상태에서도 실행되도록 설정했다. Controller-level simulated Escape 경로는 검증했으나 raw 물리 Escape는 검증하지 못했다.
- Pause 진입 시 Gameplay 입력을 막고 `SetGamePaused(true)`, Menu Stack Push, UI Input Mode, `RESUME` Focus를 적용한다.
- 실제 Pause 상태에서 HP와 Dash 표시가 2초 이상 변하지 않아 World 정지를 확인했다.
- `RESUME`은 동일 HUD를 유지한 채 World와 Gameplay Input을 복구한다.
- `RESTART`와 `MAIN MENU`, CommonUI Back 경로를 검증했다.

### GameOver / Victory

- UI Manager가 `OnGamePhaseChanged`를 구독해 GameOver / Victory를 Result Widget으로 라우팅한다.
- HUD를 숨기고 현재 `GameState.Score`를 `FinalScore`에 전달한다.
- GameOver Score 0과 Victory Score 3,550을 실제 화면 / 로그에서 확인했다.
- 두 Result 화면 모두 `RESTART`와 `MAIN MENU`를 제공하고 기본 Focus는 `RESTART`다.

### Restart / Main Menu

- 작은 싱글플레이 게임에 맞춰 Level Reload 방식을 유지했다.
- Restart는 같은 Level을 `?AutoPlay` 옵션으로 열어 Main Menu를 건너뛰고 Preparing → Wave 1로 재진입한다.
- Main Menu는 옵션 없이 같은 Level을 열어 Score / Wave / HP / UI Stack을 새 세션으로 초기화한다.
- Victory와 GameOver 양쪽 Restart, Pause Restart, 각 화면의 Main Menu 복귀를 확인했다.

## C++ / Config 변경 통계

- C++ 신규: 10개 — UI class header / cpp 5쌍.
- C++ 수정: 8개 — GameMode, GameplayTags, GameState, PlayerController header / cpp.
- Metrics의 `CppFiles=18`은 신규 10 + 수정 8의 고유 파일 합계다.
- Project / Config: `CodexGame.uproject`, `DefaultEngine.ini`, `DefaultGame.ini`, `CodexGame.Build.cs` 수정.
- Blueprint 신규: 8개, 기존 Blueprint 수정: 1개.
- Asset 생성 Script: `Scripts/AgentGameTest/Codex/BuildStep05UI.py`.

## Build / Widget Compile

### C++ Build

Target: `CodexGameEditor Win64 Development`, UE 5.8.2.

| Attempt | 결과 | 기록 |
|---:|---|---|
| 1 | FAIL | Sandbox가 UnrealBuildTool의 AppData 기록을 거부한 환경 실패 |
| 2 | FAIL | 조건식으로 `UE_LOG` verbosity를 선택한 4개 call site에서 15개 compiler diagnostic |
| 3 | SUCCESS | 위 C++ 오류 수정 후 전체 Link 성공 |
| 4 | SUCCESS | Announcement Plate 연결 후 성공 |
| 5 | SUCCESS | `BindWidget` 정리 후 성공 |
| 6 | SUCCESS | 1차 Focus target 수정 후 성공 |
| 7 | SUCCESS | next-tick Focus 적용 후 성공 |
| 8 | SUCCESS | 최종 Focus log 정리 후 `Result: Succeeded`, 2026-09-08 21:38 KST |

- Build Attempts / Failures: 8 / 2.
- 관측 C++ Compile diagnostics: 15, 최종 잔여 0.
- 최종 산출물: `UnrealEditor-CodexGame.dll` Link와 target metadata 성공.
- Build #8 산출물을 다시 실행한 post-build PIE #7에서 Main Menu → Mouse PLAY → HUD → 자연 GameOver 흐름과 STEP 5 오류 0건을 확인했다.

### Widget Blueprint

- 1차 Commandlet 생성은 C++ Property와 Widget 변수 이름 충돌로 Blueprint diagnostics 118개가 발생했다.
- Widget 변수를 `BindWidget` 계약으로 정리하고 다시 생성 / 저장했다.
- 최종 Unreal MCP `CompileWidgetBlueprint`: 7/7 `true`.
- 최종 Widget Blueprint Error / Warning: 0 / 0.
- `Step05UIBuild.log`는 1차 실패 이력이며 최종 성공 증거로 사용하지 않는다.

## PIE Runtime 검증

총 `StartPIE` 7회다. 중첩 호출 실패처럼 실제로 시작되지 않은 요청은 횟수에서 제외했다.

| 검증 | 결과 |
|---|---|
| Main Menu boot | PASS — Wave Timer 시작 전 `GamePhase=None`, PLAY / EXIT 표시 |
| Main Menu → Gameplay | PASS — Mouse, HUD 표시, Preparing → Wave 1. Keyboard Enter는 이전 PIE에서 통과했으나 post-build PIE #7 raw 자동화에서는 재현되지 않아 물리 키보드는 N/A |
| HP | PASS — GAS Health 100 → 82 → 64 … → 0과 고체력 QA 변경이 HUD에 즉시 반영 |
| Wave / Enemy / Score | PASS — W1 5 / 500, W2 10 / 누적 1,650, W3 16 / 최종 3,550 |
| Dash | PASS — 실제 Dash input 경로로 3.0초 Cooldown 표시 후 READY 복귀 |
| Pause / Resume | PASS — WorldPaused=true, HUD 보존, HP / Dash 정지 후 정상 복귀 |
| Pause Restart | PASS — Preparing, Wave 0→1, Score 0, HP 100으로 재시작 |
| Pause Main Menu | PASS — Main Menu로 Level Reload, Wave 미시작 |
| GameOver | PASS — HP 0 → GameOver, Final Score 표시, RESTART Focus |
| GameOver Restart / Main Menu | PASS |
| Victory | PASS — W3 종료 → Victory, Final Score 3,550, Wave 4 없음 |
| Victory Restart / Main Menu | PASS |
| 반복 Cycle | PASS — Main→Play→Pause→Restart→Main→Play→Victory→Restart→GameOver→Main |
| Delegate / Push | PASS — HUD bind마다 delegates 7, 중복 Result Push / fatal callback 없음 |
| STEP 5 Gameplay Runtime 오류 패턴 | PASS — PIE #7 이후 Accessed None / Invalid ASC / Invalid Delegate / Blueprint Runtime Error / STEP5 failure 0 |

Wave 전개를 빠르게 재현한 QA helper는 기존 GAS Damage / Enemy Death / Score / GamePhase Event 경로를 통과하게 사용했다.
직접 Score나 UI Text를 수정해 Victory 화면을 만든 것이 아니다. STEP 3에서 검증한 전체 전투를 UI 검증으로 대체하지 않는다.

## Resolution / Visual QA

| 화면 | 검증 크기 | 결과 |
|---|---|---|
| Main Menu | 1280×720 PIE client, 캡처 1286×760(창 frame 포함) | PASS |
| Gameplay HUD | 1280×720 PIE client, 캡처 1286×760 | PASS — 잘못 저장된 GameOver 캡처를 실제 Wave 1 HUD로 교체 후 육안 재검토 |
| Pause / GameOver / Victory | 1280×720 PIE client, 캡처 1286×760 | PASS |
| Main Menu large window | 1916×1036 content / 1920×1040 outer | PASS WITH LIMIT — Anchor / Scale 동작 확인, 정확한 1920×1080은 N/A |

- 1920×1080 Design Size의 `ScaleBox`와 Anchor 기반 Widget Tree를 사용했다.
- 16:9가 아닌 큰 창에서는 Aspect 유지로 좌우에 배경이 일부 보일 수 있다.
- 기본 흰 Button이 아니라 어두운 산업 UI Panel, Amber 강조색, 상태별 Style을 적용했다.
- HUD는 화면 가장자리에 배치해 중앙 전투 시야를 과도하게 가리지 않는다.
- 최종 HUD 증거 이미지를 직접 열어 Wave, Enemy, Score, HP, Dash와 Arena 배경을 확인했다.

## STEP 5 완료조건 집계

| 분류 | 완료 / 요구 | 결과 |
|---|---:|---|
| Architecture | 4 / 4 | PASS |
| HUD | 7 / 7 | PASS |
| Main Menu | 4 / 4 | PASS |
| Pause | 5 / 6 | Controller-level simulated Escape는 PASS, raw 물리 ESC → Pause는 N/A |
| Game Over | 4 / 4 | PASS |
| Victory | 4 / 4 | PASS |
| Input | 4 / 4 | PASS WITH LIMIT — Mouse와 이전 PIE Keyboard 경로 검증; 최종 PIE #7 raw 입력 재현 및 Gamepad hardware는 N/A |
| Visual | 4 / 5 | 정확한 1920×1080 실기 창 미검증 |
| Data Flow | 6 / 6 | PASS |
| Stability | 8 / 8 | PASS |
| **합계** | **50 / 52** | **96.2%** |

## 비교 계측

| 항목 | 값 |
|---|---:|
| Build Attempts / Failures | 8 / 2 |
| PIE Test Runs | 7 |
| C++ Compile Diagnostics / Final Remaining | 15 / 0 |
| Intermediate Widget Diagnostics / Final Remaining | 118 / 0 |
| STEP 5 Gameplay Runtime Errors | 0 |
| Debug Iterations | 4 |
| C++ Created / Modified | 10 / 8 |
| Blueprint Created / Modified | 8 / 1 |
| Assets Created | 8 |
| Widget Blueprint Compile | 7 / 7 PASS |
| Unreal MCP Calls | N/A — 전체 호출 횟수를 신뢰성 있게 재구성하지 않음 |
| Blender MCP Calls | 0 |
| User Intervention Count | 0 |
| Manual Action Count | 0 |
| Token / Context | N/A |

## Known Issues / 제한 사항

- 정확한 1920×1080 화면은 현재 1920×1040 데스크톱 가용 영역 때문에 캡처하지 못했다. 1280×720과 1916×1036 content에서 Anchor / Scale을 확인했다.
- 물리 Gamepad가 없어 Confirm / Back은 N/A다. Synthetic D-Pad Focus와 CommonInput의 Gamepad mode 전환까지만 확인했다.
- Slate가 보낸 raw Escape는 Editor의 `Stop PIE` shortcut이 먼저 소비했다. 게임의 PlayerController Escape binding은 `InputKey(CreateSimulated)` 경로로 Pause / Resume를 검증했으며 packaged build의 물리 Escape는 N/A다.
- 최종 post-build PIE #7에서 CommonUI는 `PlayButton`을 desired focus로 잡았지만 SlateInspector의 raw Enter / Space 호출은 Widget activation까지 재현하지 못했다. 동일 화면의 Mouse Click은 정상적으로 `PLAY_ACCEPTED`로 이어졌고, 이전 PIE에서는 Keyboard Enter 경로가 통과했다.
- Editor 시작 전 로그에는 Unreal의 UnifiedErrorTest / Automation self-test와 기존 `GameFeatureData` 설정 오류가 있다. 이는 PIE 시작 이후 STEP 5 Gameplay 오류 0건 집계에서 제외해 보존했다.
- Restart / PIE 종료 중 CrowdFollowing의 RecastNavMesh cleanup warning이 드물게 남는다. 기존 STEP 4에도 기록된 비치명 world teardown 경고이며 최종 Gameplay 오류 패턴에는 해당하지 않는다.
- FPS / GPU time / Draw Call은 실측하지 않아 N/A다.
- 현재 UI는 요구된 최소 Transition과 Announcement만 사용한다. STEP 6에서 최종 VFX / Audio / Animation polish가 가능하다.

## 증거 파일

- `Docs/AgentComparison/Codex/Evidence/Step05/MainMenu_1280x720.png`
- `Docs/AgentComparison/Codex/Evidence/Step05/GameplayHUD_1280x720.png`
- `Docs/AgentComparison/Codex/Evidence/Step05/Pause_1280x720.png`
- `Docs/AgentComparison/Codex/Evidence/Step05/GameOver_1280x720.png`
- `Docs/AgentComparison/Codex/Evidence/Step05/Victory_1280x720.png`
- `Docs/AgentComparison/Codex/Evidence/Step05/MainMenu_1920x1040.png`
- `Saved/AgentGameTest/Codex/Step05UIBuildReport.json` — 구조 / 8개 Asset report
- `Saved/Logs/CodexGame-backup-2026.09.07-13.55.54.log` — 최종 Widget compile 7/7 clean dispatch
- `Saved/Logs/CodexGame-backup-2026.09.07-14.31.34.log` — 장기 UI flow / Focus / Victory / Restart 검증
- `Saved/Logs/CodexGame.log` — final build post-build PIE #7 Main Menu → Mouse PLAY → HUD → 자연 GameOver와 STEP 5 오류 pattern scan

## 계측 범위와 종료 확인

- Start 2026-09-07 21:06:57 KST, 기록 계측 마감 2026-09-09 10:44:33 +09:00. 37h 37m 36s는 대기와 재개를 포함한 wall time이며 실제 active time은 N/A다.
- Token / Context는 현재 task에서 신뢰 가능한 telemetry 값을 제공받지 못해 추측하지 않고 N/A로 기록했다.
- 최종 Build는 성공했고 PIE를 종료했다. 임시 PIE 해상도는 1280×720 검증 뒤 기존 `NewWindowWidth=1920`, `NewWindowHeight=1000`으로 복원했다.
- `git diff --check`를 수행하고 generated `Binaries`, `Intermediate`, `Saved`를 Git 대상에 추가하지 않는다.
- Commit / Push는 이 STEP 종료 시점에 수행하지 않았다.
- STEP 5는 50/52, 96.2%로 마감한다. 위 검증 제한을 인수할 수 있다면 STEP 6 진행이 가능하지만 별도 지시 전 시작하지 않는다.
