# Slot Math Simulation

The project has two halves:

1. **The math model (Excel).** I calculated the game's RTP exactly, by counting every possible reel combination, and tuned the reel strips until the game hit its target RTP of about 95%.
2. **The simulator (C++20).** A Monte Carlo engine plays the same game a billion times across all CPU cores and reports RTP, hit rates, feature frequencies, per-symbol contribution and the win distribution.

The two were built independently, and they agree to within a few hundredths of a percent. That's the check that gives confidence the game is right.

## The game

The game is a classic 20-line slot:

- 5 reels × 3 rows, 20 fixed paylines, paying left to right
- 7 paying symbols (3 high, 4 low), a Wild and a Scatter
- 20 credit bet (1 credit per line)
- 3+ scatters pay anywhere and award 10 free spins, which can retrigger
- Free spins use their own reel strips
- Total win per base spin is capped at 1,100× the bet

The one rule that took some care is how wilds pay. If a line starts with wilds, the wilds can either pay on their own or substitute for the next symbol. The simulator works out both and pays whichever is higher, the way it's handled in real games. Getting this wrong is an easy way to be off by a percent or two of RTP.

## Designing the math

The workbook (`Math-ExcelSheet.xlsx`) holds the full math model:

- **Game Description:** rules, paytable and payline definitions
- **BaseGameReel / FreeGameReel:** the reel strips plus the analytical calculations for each
- **Buy free spins:** reel strips and calculations for a 50× buy feature
- **Parsheet:** the summary a game studio or test lab would review

**Exact RTP by counting combinations.** Each base game reel has 50 stops, so there are 50⁵ = 312.5 million possible outcomes. Instead of sampling them, the sheet counts symbols (and wilds) on every reel and works out exactly how many of those outcomes give 3, 4 or 5 of a kind for each symbol. The tricky part is wilds. Combinations are counted so nothing is paid twice, and a wild-only combination is only paid as wilds when that's worth more than the symbol it could complete. Scatter combinations are counted separately, since they pay anywhere in the window.

**Free spins as a feature.** From the scatter counts I get the trigger probability, 1 in 69.8 spins. Retriggers during free spins (1 in 54.5 free spins, +10 spins each) make the expected feature length a geometric series:

> average spins = 10 / (1 − 10/54.55) = 12.24

Multiply that by the win per free spin (2.13× bet) and you get the average feature win of 26.14× bet, which works out to 37.43% of RTP from free spins.

**Tuning the reels.** With the model in place, I adjusted symbol counts and positions on each strip until base game and free spins together landed on the target: 57.73% + 37.43% = **95.15%**. The result is a game where the base game pays often (about 1 in 3 spins) and free spins carry a large share of the return.

**Buy feature.** Buying free spins costs 50× the bet and uses its own 24-stop reels. The model gives an average return of 47.49× per buy, about 95% RTP, in line with the base game. This part isn't in the simulator yet.

## Analytical vs. simulated (1 billion spins)

| | Excel (exact) | Simulator |
|---|---|---|
| Base game RTP | 57.726% | 57.728% |
| Free spins RTP | 37.428% | 37.421% |
| **Total RTP** | **95.154%** | **95.148%** |
| Free spins trigger | 1 in 69.84 | 1 in 69.85 |
| Average free spins per trigger | 12.245 | 12.244 |
| Average free spins win | 26.139× bet | 26.139× bet |
| Retrigger (per free spin) | 1 in 54.55 | 1 in 54.56 |

Other results from the simulation:

| | |
|---|---|
| Hit rate | 1 in 3.07 spins |
| Max win (1,100× bet) reached | 4 times |

Win distribution (share of all spins):

| Win | 0× | 0–1× | 1–2× | 2–3× | 3–5× | 5–10× | 10–20× | 20–50× | 50–100× | 100×+ |
|---|---|---|---|---|---|---|---|---|---|---|
| % | 67.39 | 17.78 | 7.21 | 2.88 | 1.10 | 1.74 | 0.82 | 0.73 | 0.29 | 0.06 |

Inside the 100×+ group, 500–600× wins are about ten times more common than 300–500× wins. That's the five-scatter pay, which is worth 500× the bet on its own.

## How it's put together

```
src/
├── main.cpp              game flow: spin, evaluate, trigger free spins, report
├── constants.hpp         paytable, paylines, symbols, bet, max win
├── reelsFunction.hpp     loads reel strips from Excel, builds the 5×3 window
├── WinningFunctions.hpp  payline and scatter evaluation
├── Stats/                statistics, one class per game feature
├── multiThread/          parallel simulation runner
└── Utility/              small helpers
```

**Game definition.** Everything that defines the math (paytable, paylines, bet, free spin count, max win) lives in `constants.hpp`. Changing the game doesn't mean touching the simulation code.

**Reels.** Reel strips are read from an Excel workbook, the same format mathematicians already work in, using OpenXLSX. Each spin picks a random stop per reel and takes three consecutive symbols, wrapping around the end of the strip, just like a real reel.

**Wins.** `WinningFunctions` checks all 20 lines and counts scatters. Every win is recorded against the symbol and the match length (3, 4 or 5 of a kind), so per-symbol stats come for free.

**Statistics.** I split stats by feature so any number can be pulled out on its own:

- `SimResult` holds whole-game totals, the max-win tracking and the overall win distribution.
- `BaseMatrixData` and `FreeMatrixData` hold stats for the base game and free spins. The free spins class also tracks triggers, retriggers, average spins and average win per trigger. A buy feature class is started but not finished yet.
- Each of these keeps its own per-symbol data (wins, RTP, 3/4/5-of-a-kind hit rates) and its own win distribution.

**Running it fast.** A billion spins needs to be quick, so:

- The work is split across about 80% of the CPU threads. Each thread keeps its own results object, so there are no locks while spinning. When all threads finish, the results are added together with `operator+=`.
- Each thread creates its random number generator (`std::mt19937`) once and reuses it for every spin.
- The spin matrix is allocated once per thread and reused, so the inner loop doesn't allocate memory.
- Progress is reported through a single atomic counter.

The parallel runner doesn't know anything about this particular game. It just needs a results type that can be added together and a function that plays N spins, so it will work unchanged for other games too.

## Building

You'll need GCC 11+ (or Clang 14+), CMake 3.16+ and Git. OpenXLSX is downloaded automatically during the build.

```bash
git clone https://github.com/TRavi107/Slot-Math-Simulation
cd Slot-Math-Simulation
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/myapp
```

Build in Release. Debug builds are many times slower at this spin count. For a quick check, lower `spinCount` in `main.cpp`. The reel file and sheet names are set in `constants.hpp`.

## What's next

My plan is to grow this into a general slot engine where a new game is a config file rather than new code:

- Load paytable, lines, reels and features from JSON
- A common interface for features: free spins, buy feature, multipliers, respins, hold & win
- Support for ways (243/1024), cluster pays and Megaways, not just paylines
- Volatility index, standard deviation and confidence intervals on RTP
- Seedable RNG so any run can be reproduced exactly
- CSV/JSON export and automatic PAR sheet generation
- Unit tests for the win evaluation, and CI on GitHub Actions