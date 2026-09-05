# PROJECT: LAST STAND — STEP 3 Wave + Spawn + Score + Victory / Game Over

## STEP 상태

| 항목 | 값 |
|---|---|
| Agent | Codex |
| Start Time | 2026-08-29 12:43:35 +09:00 |
| End Time | 2026-09-01 21:38:32 +09:00 |
| Elapsed Time | 3d 8h 54m 57s |
| Completion Rate | 64 / 64 (100%) |
| 결과 | SUCCESS |

이번 STEP에서는 STEP 1의 Player/GAS와 STEP 2의 Enemy/AI/GAS 전투를 연결해
`Preparing → Wave 1 → WaveClear → Wave 2 → WaveClear → Wave 3 → Victory`
및 `Player Death → GameOver → Restart` 흐름을 실제 PIE에서 검증했다.
STEP 4의 산업시설 Arena Art, STEP 5의 최종 UI/CommonUI, STEP 6의 최종 VFX/Polish는 만들지 않았다.

## Game Loop Architecture

작은 싱글플레이 게임에 맞춰 책임을 세 클래스로 분리했다.

- `ACodexLSGameMode`: Wave 정의와 진행, 타이머, Spawn 요청, Enemy/Player Death 처리, Score, Victory/GameOver, Restart 규칙을 권위 있게 관리한다.
- `ACodexLSGameState`: UI가 읽을 단일 Runtime State와 변경 Delegate를 보관한다. GameMode만 값을 변경하고 향후 HUD는 GameState를 구독한다.
- `ACodexLSEnemySpawner`: Spawn Point 수집, Player 거리/Navigation/충돌/위치 분리 검증, Enemy Class 선택과 실제 Spawn만 담당한다.
- `ACodexLSEnemySpawnPoint`: Level Art와 분리된 이동 가능한 Spawn 위치를 제공한다.

GameMode 하나에 위치 탐색과 UI 상태까지 넣지 않았고, World 전체 Actor 검색이나 Enemy Health Tick polling 없이 Event와 Timer 기반으로 동작한다.

## GamePhase 구조

`ECodexLSGamePhase`는 다음 상태를 사용한다.

| Phase | 의미 |
|---|---|
| `None` | 초기화 전 |
| `Preparing` | 최초 Wave 준비 시간 |
| `WaveInProgress` | Spawn 및 전투 진행 |
| `WaveClear` | 다음 Wave 전 휴식 |
| `Victory` | Wave 3의 Spawn과 생존 Enemy가 모두 0 |
| `GameOver` | Player Health가 0 |

모든 전환은 현재 Phase를 확인해 중복 호출을 막는다. 마지막 Enemy와 Player가 거의 동시에 죽는 경우에는 **GameOver 우선** 규칙을 적용했다. Enemy가 먼저 죽어 잠시 Victory 후보가 되더라도 같은 프레임의 Player Death가 GameOver를 확정하며, Player가 먼저 죽으면 이후 Enemy Death는 Score를 올리지 않는다.

## Wave 데이터 관리

`FCodexLSWaveData` 배열 하나에 `GruntCount`, `RunnerCount`, `SpawnInterval`을 저장한다. 함수 여러 곳에 Wave별 숫자를 흩어놓지 않았다.

| Wave | Grunt | Runner | 총 Enemy | Spawn Interval |
|---:|---:|---:|---:|---:|
| 1 | 5 | 0 | 5 | 0.45초 |
| 2 | 7 | 3 | 10 | 0.45초 |
| 3 | 10 | 6 | 16 | 0.45초 |

- 최초 준비 시간: 2.5초
- Wave 간 휴식: 4.0초
- MaxWave: 3
- Original과 Changed가 동일하며 난이도 수치는 변경하지 않았다.

Runtime State의 기준값은 `CurrentWave`, `MaxWave`, `AliveEnemyCount`,
`TotalSpawnedEnemyCount`, `RemainingSpawnCount`, `Score`, `GamePhase`이다.
내부와 외부 모두 CurrentWave 0은 시작 전, 1~3은 표시 가능한 실제 Wave 번호로 사용한다.

Wave Clear 조건은 반드시 다음 두 조건을 함께 만족해야 한다.

```text
RemainingSpawnCount == 0
AND
AliveEnemyCount == 0
```

## Enemy Spawn 구조

Test Level에는 6개의 `BP_EnemySpawnPoint_Codex`를 여러 방향에 배치하고 Enemy는 미리 배치하지 않았다.
`BP_EnemySpawner_Codex`는 설정 가능한 `TSubclassOf<ACodexLSEnemyCharacter>`로 Grunt와 Runner Blueprint Class를 받으므로 코드에 Blueprint 경로 문자열을 하드코딩하지 않는다.

Spawn 후보는 다음 순서로 검증한다.

1. 6개 Spawn Point를 한 번 수집하고 직전 Point를 피하며 순서를 섞는다.
2. Player로부터 최소 1,000cm 떨어진 Point만 사용한다.
3. Point 주변 180cm Offset을 적용해 같은 위치 중첩을 줄인다.
4. `ProjectPointToNavigation`으로 250 × 250 × 300cm 범위의 NavMesh 위치를 얻는다.
5. 충돌과 직전 Spawn 위치 160cm 최소 분리를 확인한다.
6. 내부 최대 12회 시도 후 실패를 GameMode에 반환한다.
7. GameMode는 최대 3회 제한 재시도하고 오류를 기록해 무한 대기하지 않는다.

Enemy Spawn이 실제 성공한 뒤에만 `AliveEnemyCount`와
`TotalSpawnedEnemyCount`를 증가시킨다. 검증에서 6개 Point가 모두 사용됐고
관측된 Player 최소 거리는 2,179cm였다.

## Enemy Death → Wave 및 Score 연결

Spawn 성공 직후 GameMode가 각 Enemy의 기존 Death Delegate에 등록한다.
Enemy Death 시 다음 순서로 처리한다.

1. 해당 Enemy가 `ActiveEnemies`에 있는지 제거하며 최초 처리인지 확인한다.
2. 최초 Death인 경우에만 `AliveEnemyCount`를 1 감소시키고 0 미만은 허용하지 않는다.
3. Phase가 `WaveInProgress`일 때만 Enemy의 `ScoreValue`를 Score에 더한다.
4. `RemainingSpawnCount == 0 && AliveEnemyCount == 0`이면 지연된 Wave 완료 판정을 큐에 넣는다.
5. Destroy/EndPlay 시 Delegate를 해제해 수명 종료 후 호출을 막는다.

Score는 Enemy가 전역 상태에 직접 접근하지 않고 GameMode가 보상값을 읽어 관리한다.

| Enemy | Score |
|---|---:|
| Grunt | 100 |
| Runner | 150 |

검증 중 Grunt 2 + Runner 1 처치에서 Score가 500 → 700 → 850으로 정확히 350 증가했다.
전체 Victory Score는 `5×100 + 7×100 + 3×150 + 10×100 + 6×150 = 3,550`이었다.
31개 Death Event 모두 최초 처리 1회였고 중복 Score나 음수 Alive Count는 없었다.

## Victory / GameOver / Restart

### Victory

Wave 3에서 남은 Spawn과 생존 Enemy가 모두 0일 때만 `Victory`로 진입한다.
Phase/Spawn/다음 Wave Timer를 모두 정리하고 Player 입력 및 전투 Ability 진행을 막는다.
Wave 4는 시작되지 않았으며 Victory snapshot에서
`Wave=3, Alive=0, Remaining=0, Score=3550`을 확인했다.

### GameOver

Player의 기존 GAS Health 변경/Death Event를 구독하며 Tick으로 Health를 검사하지 않는다.
Health 0 이벤트를 받으면 즉시:

- Phase를 `GameOver`로 전환
- 준비/Wave 전환/Spawn/Wave 완료/QA Timer 취소
- Spawn Queue의 남은 수량 제거
- 살아 있는 Enemy AI의 이동·공격 중지
- Player 입력, Primary Attack, Dash 차단

을 수행한다. Spawn 도중 Player가 죽은 테스트에서는 성공 Spawn 2마리 이후
`PendingCancelled=3`이 기록됐고 8.4초 뒤에도 `TotalSpawned=2`로 추가 Spawn이 없었다.
WaveClear 대기 중 Player Death 테스트도 Next Wave Timer가 취소되어 Wave 2가 시작되지 않았다.

### Restart

작은 싱글플레이 Level이므로 `OpenLevel` 기반 Level Reload를 선택했다.
부분 Reset보다 Actor, Timer, Delegate, ASC 초기화 경로를 다시 시작하는 방식이 단순하고 안전하다.

Victory와 GameOver 양쪽에서 Restart 후 다음을 확인했다.

- Score 0, CurrentWave 0, AliveEnemyCount 0
- Player Health 100/100
- Dead 상태 및 Dead GameplayTag 제거
- 입력 활성화
- Ability 2개 유지, 중복 Grant 없음
- 준비 시간 후 Wave 1 재시작

## STEP 5 UI 연결용 Event

`ACodexLSGameState`는 다음 `BlueprintAssignable` Delegate를 제공한다.

- `OnGamePhaseChanged(PreviousPhase, NewPhase)`
- `OnWaveChanged(CurrentWave, MaxWave)`
- `OnAliveEnemyCountChanged(AliveEnemyCount)`
- `OnScoreChanged(Score)`

따라서 STEP 5 HUD와 Victory/GameOver 화면은 Tick polling 없이 상태 변경을 구독할 수 있다.
현재는 최종 UI 대신 화면 Debug Text에 Wave, Enemies, Score, State를 표시한다.
Player Health는 기존 GAS Attribute 변경 Delegate를 그대로 활용할 수 있다.

## Test Level 및 생성 Asset

- `Content/AgentGameTest/Codex/Levels/L_LastStand_GameLoopTest_Codex.umap`
- `Content/AgentGameTest/Codex/Blueprints/BP_GameMode_STEP3_Codex.uasset`
- `Content/AgentGameTest/Codex/Blueprints/BP_EnemySpawner_Codex.uasset`
- `Content/AgentGameTest/Codex/Blueprints/BP_EnemySpawnPoint_Codex.uasset`

Level 구성:

- PlayerStart 1
- Enemy Spawner 1
- Enemy Spawn Point 6
- NavMeshBoundsVolume 1
- 단순 장애물 3
- 사전 배치 Enemy 0
- 기본 Lighting

Asset 생성 스크립트 결과:

```text
CODEX_STEP3_ASSET_BUILD_SUCCESS bp_assets=3 newly_created=3 levels=1
spawnpoints=6 enemies=0 navmesh_bounds=1 obstacles=3
```

## C++ 변경

### 생성 6개

- `Source/CodexGame/Public/AgentGameTest/Codex/Game/CodexLSGameState.h`
- `Source/CodexGame/Private/AgentGameTest/Codex/Game/CodexLSGameState.cpp`
- `Source/CodexGame/Public/AgentGameTest/Codex/Spawn/CodexLSEnemySpawner.h`
- `Source/CodexGame/Private/AgentGameTest/Codex/Spawn/CodexLSEnemySpawner.cpp`
- `Source/CodexGame/Public/AgentGameTest/Codex/Spawn/CodexLSEnemySpawnPoint.h`
- `Source/CodexGame/Private/AgentGameTest/Codex/Spawn/CodexLSEnemySpawnPoint.cpp`

### 수정 10개

- `CodexLSGameMode.h/.cpp`
- `CodexLSPlayerCharacter.h/.cpp`
- `CodexLSPlayerController.h/.cpp`
- `CodexLSEnemyCharacter.h/.cpp`
- `CodexLSEnemyAIController.h/.cpp`

기존 STEP 1~2 구조는 유지하고 Death Event, ScoreValue, terminal-state 입력/AI 중지와
개발 전용 QA 명령에 필요한 최소 변경만 했다.

## Build 및 Asset 검증

실제 최종 Build 명령:

```powershell
& 'D:\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat' CodexGameEditor Win64 Development 'D:\Unreal Projects\CodexGame\CodexGame.uproject' -WaitMutex -NoHotReloadFromIDE
```

| Attempt | 결과 | 설명 |
|---:|---|---|
| 1 | FAIL | Sandbox가 Unreal Trace의 AppData 접근을 막아 Compile 시작 전 종료 |
| 2 | SUCCESS | UHT/Compile/Link 성공 |
| 3 | SUCCESS | 후속 변경 Build 성공 |
| 4 | FAIL | Attempt 1과 같은 Sandbox 환경 제한, Compile 시작 전 종료 |
| 5 | SUCCESS | 최종 UHT/Compile/Link 성공 |

- Build Attempts: 5
- Build Failures: 2
- Compile Errors: 0
- 최종 Build: SUCCESS
- Blueprint 5개 Compile: warnings-as-errors 기준 SUCCESS
- Blueprint Build Paths: SUCCESS
- Map Check: 0 errors / 0 warnings

Asset commandlet는 요청 Asset 생성을 완료했지만 프로젝트에 기존부터 있던
`GameFeatureData`의 AssetManager 설정 때문에 전체 Exit Code 1을 반환했다.
STEP 3 Asset/Blueprint/Map 검증 결과에는 실패가 없으며 이 항목을 Known Issue에 별도 기록한다.

## PIE Runtime 검증

### Wave 1

- 2.5초 준비 후 Grunt 5마리 Spawn
- 여러 Spawn Point와 NavMesh 사용
- 추적/공격/Player → Enemy GAS Damage/Death 정상
- 마지막 Death 후 Alive 0, Remaining 0, WaveClear
- 누적 Score 500
- 결과: SUCCESS

### Wave 2

- Grunt 7 + Runner 3, 총 10마리 정확히 Spawn
- Grunt/Runner AI와 GAS 전투 정상
- Enemy Count 및 Score 갱신 정상
- 마지막 Death 후 WaveClear
- 누적 Score 1,650
- 결과: SUCCESS

### Wave 3

- Grunt 10 + Runner 6, 총 16마리 정확히 Spawn
- 전체 Death 후 Alive 0, Remaining 0
- Victory 진입, Wave 4 미생성
- Final Score 3,550
- 결과: SUCCESS

전체 Victory Flow는 QA 명령이 Enemy에게 실제 GAS Damage를 적용하는 방식으로 가속했다.
명령이 Phase, Alive Count, Score를 직접 변경하지 않았고 Enemy Health/Death Delegate,
Wave Timer와 Spawn 경로는 정상 Gameplay와 동일하게 실행됐다.

### GameOver 및 Edge Case

| 테스트 | 관측 결과 | 결과 |
|---|---|---|
| 일반 전투 중 Player Health 0 | GameOver, 입력/AI/모든 Timer 중지 | PASS |
| Spawn 중 GameOver | 2마리 성공 후 남은 3 Spawn 취소, 추가 Spawn 없음 | PASS |
| WaveClear 대기 중 GameOver | Next Wave Timer 취소, Wave 2 미시작 | PASS |
| 제한된 Spawn 실패 | 내부 12회 실패와 GameMode 1회 재시도 후 W1의 5마리 전부 정상 Spawn | PASS |
| Enemy 먼저 죽는 terminal race | 마지막 Enemy Death 뒤 Player Death, 최종 Phase GameOver | PASS |
| Player 먼저 죽는 terminal race | GameOver 뒤 마지막 Enemy Death의 Reward 0, 최종 Phase GameOver | PASS |
| 중복 Death | 31마리 모두 최초 Event 1회, 중복 Score 없음 | PASS |
| Alive Count 하한 | 모든 세션에서 0 이상 | PASS |

### Restart

| 경로 | 결과 |
|---|---|
| Victory → Restart → Wave 1 | SUCCESS |
| GameOver → Restart → HP 100 → Wave 1 | SUCCESS |
| Score/Wave/Dead Tag/ASC Ability 초기화 | SUCCESS |

### 실행 횟수

- PIE Test Runs: 6
- PIE 안에서 Level Reload로 생성된 Runtime Sessions: 19
- Wave Flow Test Attempts: 1
- Victory Test Attempts: 1
- GameOver terminal 관측: 18
- 성공 Restart: 13
- Debug Iterations: 4

Attempts는 로그 marker 기준이다. Restart 요청 marker 14개 중 과거 세션의 중복 marker 1개를 제외해 성공 Reload 13회로 집계했다.

## Runtime Error 검사

최종 로그에서 다음 치명 오류 패턴은 모두 0건이었다.

- Accessed None
- Invalid ASC
- Invalid Delegate
- GameplayTag Warning
- Navigation Warning
- Timer Warning
- Array Out of Bounds
- Assertion / Ensure
- Fatal / Unhandled

QA가 의도적으로 발생시킨 Spawn 후보 실패 12회, 최종 Spawn 실패 1회,
GameMode 재시도 1회는 이후 정상 회복을 검증하기 위한 예상 Warning이며 Runtime Error로 세지 않았다.

## 완료 조건

| 그룹 | Requirements | Completed | Failed |
|---|---:|---:|---:|
| Wave | 8 | 8 | 0 |
| Spawn | 10 | 10 | 0 |
| Death Tracking | 6 | 6 | 0 |
| Score | 5 | 5 | 0 |
| Game Phase | 6 | 6 | 0 |
| Victory / GameOver | 9 | 9 | 0 |
| Restart | 7 | 7 | 0 |
| Events | 5 | 5 | 0 |
| Validation | 8 | 8 | 0 |
| **합계** | **64** | **64** | **0** |

Completion Rate: **100%**

## 비교 측정값

| 항목 | 값 |
|---|---:|
| Token / Context | 78,906,327 total tokens |
| Input Tokens | 78,637,918 |
| Cached Input Tokens | 76,252,416 |
| Output Tokens | 268,409 |
| Reasoning Tokens | 119,539 |
| Context Start | 141572 / 258400 |
| Context End | 73618 / 258400 |
| Token Cutoff | 2026-09-01 21:37:36 +09:00, root JSONL ordinal 7669 |
| User Intervention Count | 0 |
| Manual Action Count | 0 |
| Unreal MCP Calls | N/A |
| Blender MCP Calls | 0 |
| Build Attempts / Failures | 5 / 2 |
| PIE Test Runs | 6 |
| Compile Errors | 0 |
| Runtime Errors | 0 |
| Debug Iterations | 4 |
| C++ Created / Modified | 6 / 10 |
| Blueprint Created | 3 |
| Assets Created | 4 |

`이어서 진행` 메시지는 중단된 자동 작업의 재개 요청이며 구현 판단을 바꾼 교정이나
사용자 수동 Editor 조작이 아니므로 User Intervention과 Manual Action에 포함하지 않았다.
Token Total은 Input + Output이며 Reasoning은 Output의 부분집합이다.

| Token 구분 | Input | Cached Input | Output | Reasoning | Total |
|---|---:|---:|---:|---:|---:|
| Root | 56,958,046 | 55,651,072 | 123,487 | 40,851 | 57,081,533 |
| STEP 3 Subagents 8개 | 21,679,872 | 20,601,344 | 144,922 | 78,688 | 21,824,794 |
| **Combined** | **78,637,918** | **76,252,416** | **268,409** | **119,539** | **78,906,327** |

집계는 STEP 3 시작 root JSONL ordinal 4836부터 cutoff ordinal 7669까지와
명시적으로 생성한 STEP 3 Subagent 8개의 마지막 누적값을 사용했다.
중간에 별도 요청으로 처리한 플스방 추천 turn의 1,066,575 tokens는 제외했다.

## 증거

- [STEP 3 Level Setup](Evidence/Step03_LevelSetup.png)
- [Wave 1](Evidence/Step03_Wave1.png)
- [Wave 2](Evidence/Step03_Wave2.png)
- [Wave 3](Evidence/Step03_Wave3.png)
- [Victory](Evidence/Step03_Victory.png)

## Known Issues

- Asset commandlet 전체 Exit Code 1: STEP 3 외부의 기존 `GameFeatureData` AssetManager 설정 경고가 원인이다. 생성 Asset, Blueprint Compile, Build Paths와 Map Check는 성공했다.
- Level Reload 중 이전 World의 Recast/NavMesh teardown warning이 한 번 관측됐으나 새 World의 Navigation, Spawn, AI 이동과 전체 Flow에는 영향이 없었다.
- 최종 Arena Art, CommonUI, Niagara, Audio는 계획대로 후속 STEP 범위이며 현재 증거 화면은 기능 검증용 primitive Test Level이다.
- 전체 Victory Flow는 긴 반복 시간을 줄이기 위해 실제 GAS Damage를 보내는 Development 전용 QA 명령으로 진행했다. Shipping 실행에서는 QA 동작 본문이 비활성화된다.

## 결론

Wave 1~3, Navigation-aware Spawn, Enemy Death 집계, Score, Victory, GameOver,
Restart, UI Event 구조를 Build와 PIE에서 검증했다. STEP 3은 64/64로 완료됐으며
현재 구조를 유지한 채 STEP 4에서 Spawn Point와 NavMesh를 완성 Arena로 옮길 수 있다.
