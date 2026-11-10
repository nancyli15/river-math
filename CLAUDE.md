@~/nancyli15_vault/Teaching.md
# CLAUDE.md: working rules for RiverMath

Read this before writing or planning any code in this repo.

## Scope: smallest version that makes the claims true
- Build the **smallest working version** of each feature first. Expansions come later, only when asked.
- v1 is defined in `PLAN.md` ("v1: minimum to make the resume true"). Don't add features outside it unless asked.
- If a feature can be built two ways, choose the simpler one and mention the other as a possible later expansion.
- Resume and README numbers must come from real measurements (`rivermath bench`), never from estimates.

## Readability
- Names should explain themselves to someone new to C and to poker:
  - `cards`, `board`, `hole_cards`, `best_rank`, `num_players`. Not `h`, `b`, `hc`, `br`, `n`.
  - Single letters are fine only for tiny loop indices (`i`, `j`) or well-known math (`n choose k`).
  - Functions are verbs: `rm_parse_cards`, `rm_evaluate_bitwise`, `rm_count_cards`.
- Public names start with `rm_` (C has no namespaces, so the prefix prevents name clashes).
- Comments explain **why** or give the poker rule. Don't restate what the code says.
- Keep functions short. If a function needs a comment for each section, split it into named helpers.
- Use named constants or enums instead of magic numbers (`RM_NUM_CARDS`, not `52`).

## No repeated logic
- Each rule lives in one place. Examples:
  - Straight detection: one helper, used by every evaluator and table builder.
  - Hand-value encoding (category + kickers → number): one helper.
  - Dealing random cards: one helper, shared by Monte Carlo and anything else that samples.
- Before writing a new helper, check whether one already exists in `include/rivermath/`.
- Generate data from existing code when possible. For example, the lookup tables are filled by calling the
  bitwise evaluator, not by re-implementing hand ranking.
- The naive evaluator is the one intentional exception. It's the independent reference used to check the others,
  so it doesn't reuse their logic (it may share card/encoding helpers).

## Simplicity
- No abstraction until there's a second real user of it.
- Prefer plain arrays, structs and functions. No macros beyond the test harness.
- No new dependencies or tools without asking the user first.

## Tests
- Write tests alongside each feature, not at the end.
- Every bug fix gets a test that would have caught it.
- `make test` must pass (with sanitizers) before calling anything done.

## Workflow
- Tell the user about anything they need to install or set up; don't install it yourself.
- Don't commit or push unless the user asks.
- Build: `make`. Tests: `make test`. Run: `make run ARGS="..."`.
