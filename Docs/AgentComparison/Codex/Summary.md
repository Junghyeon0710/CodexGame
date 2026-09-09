# PROJECT: LAST STAND — Agent Comparison Summary

| Step | Time | Completion | Build Fail | PIE | User Intervention | Token |
|---|---:|---:|---:|---|---:|---|
| STEP 1 | 16h 50m 58s | 33/33 (100%) | 2/8 | 5 runs, Final SUCCESS | 0 | N/A |
| STEP 2 | 10h 00m 18s | 37/37 (100%) | 1/6 | 15 runs, Final SUCCESS | 0 | 42,294,821 |
| STEP 3 | 80h 54m 57s | 64/64 (100%) | 2/5 | 6 runs, Final SUCCESS | 0 | 78,906,327 |
| STEP 4 | 47h 26m 39s (중단 포함) | 55/55 (100%) | 1/2 | 18 runs, Final SUCCESS | 0 | 37,602,205 (root 계측) |
| STEP 5 | 37h 37m 36s (중단 포함) | 50/52 (96.2%) | 2/8 | 7 runs, Final SUCCESS | 0 | N/A |
| STEP 6 | — | — | — | — | — | — |

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

STEP 5의 시간은 2026-09-07 21:06:57~2026-09-09 10:44:33 KST 계측 마감 구간이다. 중단·재개를 포함하며 실제 active time과 Token / Context는 N/A다. CommonUI와 이벤트 기반 HUD, Main / Pause / Result / Restart 흐름은 최종 Build와 PIE에서 통과했다. 정확한 1920×1080 실기 창과 raw 물리 ESC → Pause는 미검증으로 계산해 50/52로 기록했고, 물리 Gamepad Accept / Back도 N/A로 보존했다. 최종 post-build PIE #7은 Mouse PLAY → HUD → 자연 GameOver와 STEP 5 Gameplay 오류 0건을 확인했지만, SlateInspector raw Enter / Space 활성화는 재현되지 않았다. STEP 6는 미착수다.
