# PROJECT: LAST STAND — Agent Comparison Summary

| Step | Time | Completion | Build Fail | PIE | User Intervention | Token |
|---|---:|---:|---:|---|---:|---|
| STEP 1 | 16h 50m 58s | 33/33 (100%) | 2/8 | 5 runs, Final SUCCESS | 0 | N/A |
| STEP 2 | 10h 00m 18s | 37/37 (100%) | 1/6 | 15 runs, Final SUCCESS | 0 | 42,294,821 |
| STEP 3 | 80h 54m 57s | 64/64 (100%) | 2/5 | 6 runs, Final SUCCESS | 0 | 78,906,327 |
| STEP 4 | 47h 26m 39s (중단 포함) | 55/55 (100%) | 1/2 | 18 runs, Final SUCCESS | 0 | 37,602,205 (root 계측) |
| STEP 5 | 37h 37m 36s (중단 포함) | 50/52 (96.2%) | 2/8 | 7 runs, Final SUCCESS | 0 | N/A |
| STEP 6 | 97h 41m 53s (중단 포함) | 45/45 (100%) | 2/7 | 7 runs, Final SUCCESS | 0 | N/A |

STEP 1 상세 기록: [`Step01_PlayerGAS.md`](Step01_PlayerGAS.md)

STEP 1 PIE 캡처: [`STEP01_PIE.png`](STEP01_PIE.png)

STEP 2 상세 기록: [`Step02_EnemyCombat.md`](Step02_EnemyCombat.md)

STEP 2 증거: [`Navigation`](Evidence/Step02_Navigation.png), [`Multi Combat`](Evidence/Step02_MultiCombat.png), [`Runner Solo`](Evidence/Step02_RunnerSolo.png)

STEP 3 상세 기록: [`Step03_WaveGameLoop.md`](Step03_WaveGameLoop.md)

STEP 3 증거: [`Level Setup`](Evidence/Step03_LevelSetup.png), [`Wave 1`](Evidence/Step03_Wave1.png), [`Wave 2`](Evidence/Step03_Wave2.png), [`Wave 3`](Evidence/Step03_Wave3.png), [`Victory`](Evidence/Step03_Victory.png)

STEP 4 상세 기록: [`Step04_ArenaLevel.md`](Step04_ArenaLevel.md)

STEP 4 증거: [`최종 밀집 전투`](Evidence/Step04_Density.png), [`Victory`](Evidence/Step04_Victory.png), [`가림 개선`](Evidence/Step04_OcclusionFinal.png), [`Warehouse`](Evidence/Step04_WarehouseCamera.png)

STEP 4의 시간은 2026-09-02 21:40:11~2026-09-04 21:06:50 KST 계측 마감 구간이다. 중단·재개 대기를 포함하며 실제 active time은 N/A다. 토큰은 root task의 캐시 입력 포함 누적 telemetry 차이이며 최종 기록 저장·응답 이후 사용량은 포함하지 않는다. 비치명 cleanup 경고와 근접 부분 가림은 상세 기록에 보존했다.

STEP 5 상세 기록: [`Step05_UI.md`](Step05_UI.md)

STEP 5 증거: [`Main Menu 1280`](Evidence/Step05/MainMenu_1280x720.png), [`Gameplay HUD`](Evidence/Step05/GameplayHUD_1280x720.png), [`Pause`](Evidence/Step05/Pause_1280x720.png), [`Game Over`](Evidence/Step05/GameOver_1280x720.png), [`Victory`](Evidence/Step05/Victory_1280x720.png), [`Main Menu Large`](Evidence/Step05/MainMenu_1920x1040.png)

STEP 5 retains its two unverified physical-display/input items. STEP 6 is complete with final VFX, Audio, QA, and documentation.

STEP 6 상세 기록: [Step06_FinalQA.md](Step06_FinalQA.md)

STEP 6 증거: [Main Menu](Evidence/Step06/01_MainMenu.png), [Attack Hit](Evidence/Step06/02_AttackHit.png), [Dash](Evidence/Step06/04_Dash.png), [Enemy Death](Evidence/Step06/05_EnemyDeath.png), [Pause](Evidence/Step06/06_Pause.png), [Runner Hit](Evidence/Step06/07_RunnerHit_Fixed.png), [Wave 3](Evidence/Step06/08_Wave3_16Enemies_Full.png), [Victory](Evidence/Step06/10_Victory.png), [Natural GameOver](Evidence/Step06/11_GameOver_AfterVictoryRestart.png), [Runtime markers](Evidence/Step06/RuntimeFinalEvidence.txt), [Build evidence](Evidence/Step06/BuildFinalEvidence.txt).

STEP 6은 기존 GAS/Wave/CommonUI/Arena 구조를 유지하면서 Niagara 5개, 자체 생성 SoundWave 10개, Hit/Death/Damage/Dash feedback과 point-blank Runner 공격 수정을 추가했다. 최종 Editor Development Build와 Blueprint 31/31, Niagara 5/5 검증을 통과했다. 실제 PIE 7회 중 최종 검증 흐름에서 Main Menu PLAY accepted → QA Restart → Wave 1 → Wave 2 → Wave 3 → Victory, Final Score 3,550을 확인했고, 별도로 자연 GameOver, Victory/GameOver Restart, Pause/Resume, Main Menu 복귀를 검증했다. Full Victory는 일부 적 처치를 QA GAS damage helper로 가속한 QA-assisted run이며 무보조 human 3~5분 balance benchmark는 N/A다.

## 전체 프로젝트 최종 집계

| 항목 | STEP 1~6 누적 |
|---|---:|
| Requirements / Completed / Failed | 286 / 284 / 2 |
| PROJECT Completion Rate | 99.3% |
| Total Elapsed Time | 290h 32m 21s |
| Build Attempts / Failures / Successes | 36 / 10 / 26 |
| PIE Runs | 58 |
| User Interventions / Manual Actions | 0 / 0 |
| Total Token Usage | N/A |
| Known Token Subtotal | 158,803,353 — 값이 있는 STEP 2~4만 합산 |
| Total Debug Iterations | N/A — known subtotal 24 |
| LAST STAND C++ Files | 41 |
| Blueprint Asset Packages | 31 |
| GameplayAbilities / GameplayEffects | 3 / 6 |
| Static Meshes | 13 |
| Materials / Textures | 17 / 17 |
| Niagara Systems | 5 |
| Widgets | 7 — 별도 CommonButtonStyle Blueprint 1 |
| Audio SoundWaves | 10 |
| Levels | 4 |
| Total Unreal Content Assets | 101 |
| Arena Actor Count | 235 — STEP 4 최신 audit, STEP 6 미재계측 |

전체 완료율은 STEP 6의 45/45를 포함한다. STEP 5에서 미완료로 보존한 정확한 1920x1080 실기 창과 raw physical ESC → Pause 2건을 소급 변경하지 않아 프로젝트 전체는 284/286, 99.3%다. 정량 FPS/GameThread/GPU/FrameTime, physical Gamepad, Standalone/packaged physical ESC, 무보조 human playthrough 시간은 N/A다.
