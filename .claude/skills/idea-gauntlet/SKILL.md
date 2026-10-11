---
name: idea-gauntlet
description: Pressure-test a new business, product or side-project idea before the user sinks time into it. Interrogates the user on why the idea is viable, researches the market from their answers, gives marketing, financial, supportive and critical perspectives, then forces concrete validation steps with kill criteria. Use whenever the user says "I have an idea", "what if I built", "should I make", "is this viable", wants market research on an idea, or seems to be jumping between ideas or hyperfixating on one.
---

# Idea Gauntlet

Purpose: stop the user from hyperfixating on ideas that go nowhere. An idea earns
the user's time only by surviving evidence, not enthusiasm. Be warm toward the
person and hard on the idea.

Run the phases in order. Never skip ahead to research or advice before Phase 1 is
answered, and never end without Phase 5.

## Phase 0 — Capture and check the ledger

1. Restate the idea in one sentence and ask the user to confirm or correct it.
2. Check the idea ledger (see "Ledger" below). If the user already has an idea in
   **TESTING**, say so plainly and ask: "Do you want to park that one, or is this a
   distraction from it?" Only one idea may be in TESTING at a time.
3. If this idea was born less than 24 hours ago, note it. Freshness is when
   hyperfixation is strongest; the gauntlet still runs, but the verdict cannot be
   higher than TEST.

## Phase 1 — The Why interrogation

Ask these **one or two at a time**, wait for answers, and push back on vague ones
("lots of people", "everyone needs this", "it'd be cool") with a follow-up. Do not
accept a feeling as evidence.

1. **Problem** — What painful problem does this solve? Describe the last time
   someone (not you) had it.
2. **Customer** — Who exactly has it? Name a job title, age band, community or
   place where 100 of them gather.
3. **Evidence** — What makes you believe they want it? Have you seen them pay,
   complain, hack a workaround or search for it?
4. **Current alternative** — What do they do today instead, and why is that not
   good enough?
5. **Why you** — What unfair advantage, skill, access or audience do you have?
6. **Money** — Who pays, how much, how often? What would you charge on day one?
7. **Cost of pursuing** — What will you stop doing to work on this? How many
   hours a week can you honestly give it?
8. **Kill criteria** — What result in the next 14 days would make you drop it?

Score each answer 0–2 (0 = no answer/feeling, 1 = plausible but unproven,
2 = backed by something observable). Keep the scores; show them in Phase 5.

## Phase 2 — Research

Use web search (and any connected research tools) driven by the user's answers,
not generic queries. Cover:

- **Competitors** — at least 5 direct or adjacent ones, with pricing and the
  complaints people leave about them (reviews, Reddit, app-store, forums).
- **Demand signals** — search interest, communities discussing the problem,
  existing paid workarounds, recent funding or shutdowns in the space.
- **Market size** — a rough bottom-up estimate (reachable customers × price ×
  purchase frequency). Show the arithmetic. Do not quote a top-down TAM alone.
- **Graveyard** — products that tried this and died, and why.

Cite sources with links. Label every number as *sourced* or *estimate*. If search
is unavailable, say so and mark the whole section as unverified.

## Phase 3 — The four perspectives

Write each as a short, direct section (4–6 sentences), grounded in Phase 1 and 2:

- **📣 Marketing** — Who is the first 100 customers and how do you reach them for
  under $500? What is the one-line pitch? Which channel is cheapest?
- **💰 Financial** — Unit economics: price, cost to deliver, cost to acquire a
  customer, months to break even at the user's stated hours. What does it cost to
  test vs. to build?
- **🤝 Champion** — Completely supportive. The strongest honest case for the idea:
  what is genuinely good here, what the user is uniquely placed to do, and the best
  realistic outcome.
- **🔪 Critic** — Assumes it fails. The single most likely cause of death, the
  weakest Phase 1 answer, and what the user is avoiding looking at.

The Champion and Critic must disagree about something specific; name it.

## Phase 4 — Forced validation steps

Give exactly **three** tests the user must run before building anything. Each must
be cheap (under ~$100 and ~10 hours total), completed within 14 days, and
produce a number. Pick from or adapt:

- Talk to 5–10 people from the named customer group (problem interviews, no
  pitching). Target: ≥3 describe the pain unprompted.
- Landing page or post with a sign-up/pre-order. Target: a set conversion rate
  or number of sign-ups.
- Pre-sell or take a deposit from 1–3 people.
- Concierge/manual version for one customer.
- Competitor teardown: use the top 2 competitors and list what you'd do differently.

For each test give: the action, the success threshold, and a deadline date.
Then restate the user's own kill criteria from Phase 1 alongside these.

**Rule: no building, logos, domain names or code until at least two of the three
tests hit their threshold.** Say this explicitly.

## Phase 5 — Verdict

Give one verdict:

| Verdict | Meaning |
|---|---|
| **KILL** | Fatal flaw found. Write down why and move on. |
| **PARK** | Interesting but not now. Set a revisit date ≥30 days out. |
| **TEST** | Run the Phase 4 tests. Nothing else. |
| **PURSUE** | Only if prior tests already passed — next milestone and a check-in date. |

Include the Phase 1 score total (out of 16) and the one question the user must
answer better next time. End with: "Come back with the numbers from your three
tests by <date>."

## Ledger

Keep a ledger so ideas are tracked and the user can see the pattern over time.

- **If you can write files** (e.g. Claude Code): maintain `ideas/LEDGER.md` in the
  current project, creating it if missing, plus one file per idea in
  `ideas/<slug>.md` holding the Phase 1 answers, research, tests and verdict.
- **Otherwise** (e.g. the Claude app): end your reply with a fenced `ledger`
  block the user can save, and ask them to paste their ledger at the start of the
  next session.

Ledger row format:

```
| Date | Idea | Status | Score /16 | Next check-in | Outcome |
```

Status is one of KILLED, PARKED, TESTING, PURSUING. When the user returns with
test results, update the row: passed tests move it toward PURSUING; missed
deadlines or failed thresholds move it to PARKED or KILLED. If the user has
three or more ideas started in the last 30 days with no tests completed, point
that pattern out kindly but directly before doing anything else.
