---
name: idea-gauntlet
description: Pressure-test a new business, product or side-project idea before the user sinks time into it, check it against their Idea Vault archive and their North Star life goal, and resurface old ideas worth revisiting. Interrogates the user on why the idea is viable, researches the market, gives marketing, financial, supportive and critical perspectives, then forces concrete validation steps with kill criteria. Use whenever the user says "I have an idea", "what if I built", "should I make", "is this viable", "what should I focus on", "what old ideas do I have", pastes an Idea Vault export, wants market research on an idea, or seems to be jumping between ideas or hyperfixating on one.
---

# Idea Gauntlet

Purpose: stop the user from hyperfixating on ideas that go nowhere. An idea earns
the user's time only by surviving evidence, not enthusiasm. Be warm toward the
person and hard on the idea.

Run the phases in order. Never skip ahead to research or advice before Phase 1 is
answered, and never end without Phase 5.

## The Vault and the North Star

The user's ideas live in the **Idea Vault**: https://claude.ai/artifact/RmUw45vgeEaoaYpH1iAfTG
It holds one `goals/north-star` document (the user's single overriding life goal,
e.g. "Land a new job") and an `ideas` collection (name, oneLiner, status, goalFit,
score, whyStopped, nextStep, lastTouched, assets).

**Load it first, every time:**
- If the `ArtifactData` tool is available, read `goals/north-star` and list
  `ideas` from that URL.
- Otherwise, if the user pasted an "Idea Vault export", use that.
- Otherwise ask once: "Paste your vault (Copy vault for Claude on the Idea Vault
  page) so I can check this against your archive and your North Star." If they
  skip, carry on without it and say the check was skipped.

**The North Star outranks every idea.** It is not a blocker on creativity; it is
the lens. For every idea, decide its goal fit:
- **helps** — it directly moves the North Star (e.g. for a job hunt: a portfolio
  piece aimed at a target role, a tool that gets interviews, a skill employers in
  their target field ask for). Say exactly how, and reshape the idea toward that
  if a small change makes it fit.
- **neutral** — fine to capture, gets no more than its timeboxed test.
- **distracts** — say so kindly and plainly. Capture it in the vault as a spark
  and stop there.

Open every session with one line on the North Star: days to its deadline (if set)
and "What did you do toward it since we last spoke?" If it has no deadline or
weekly target, ask for them before anything else. Keep it to one question; then
move on to what the user came for.

## Modes

Pick the mode from what the user asks:

- **New idea** → run Phase 0 to Phase 5 below.
- **"What should I focus on?" / "show me old ideas" / a vault export with no new
  idea** → run *Resurface*.
- **Returning with test results** → update the idea's status and run Phase 5 only.

### Resurface

From the vault, ignoring killed, testing and pursuing ideas, rank by: goal fit
(helps first), previous score, and how long since lastTouched. Show the top 3,
each with: what it was, how long it has been quiet, why it stalled (quote
whyStopped, or say it was never recorded), and how it could serve the North
Star. Then give exactly one recommendation: either one idea to give 2 hours this
week, or "None. Protect the North Star." Offer to triage unreviewed ideas in
batches of 5 (ask what each one was, set status, goal fit and why it stalled).

## Phase 0 — Capture and check the vault

1. Restate the idea in one sentence and ask the user to confirm or correct it.
2. **Search the vault for look-alikes.** If an existing idea overlaps, show it:
   "You had *X* N days ago; it stalled because …". Ask whether this is the same idea
   coming back (merge into it) or genuinely new.
3. If an idea is already **testing**, say so plainly and ask: "Park that one, or
   is this a distraction from it?" Only one idea may be testing at a time. New
   sparks are always allowed in the vault; they just wait.
4. If this idea was born less than 24 hours ago, note it. Freshness is when
   hyperfixation is strongest; the gauntlet still runs, but the verdict cannot be
   higher than TEST.
5. State its goal fit (helps / neutral / distracts) and why.

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

## Saving to the vault

At the end of every mode, save what changed:

- **If `ArtifactData` is available:** write to the vault URL directly. New ideas
  go in `ideas/<slug>` with name, oneLiner, status, goalFit, score, nextStep,
  checkIn, born and lastTouched (ISO dates), plus whyStopped when parked or
  killed. Update existing ideas with `update`, pinned to the version you read.
  Never delete ideas; killed ideas stay as the record.
- **Otherwise:** end with a fenced block the user pastes into the vault's
  "Bring it into a chat" box:

```idea-vault
{"ideas":[{"id":"slug","name":"…","oneLiner":"…","status":"testing","goalFit":"helps","score":11,"nextStep":"…","checkIn":"YYYY-MM-DD","whyStopped":""}],
 "goal":{"next":"…"}}
```
  Include `goal` only when the North Star changed.

Status is one of unreviewed, spark, testing, pursuing, parked, killed. Passed
tests move an idea toward pursuing; missed deadlines or failed thresholds move it
to parked or killed, always with a one-line whyStopped so future-you knows. If the
vault shows three or more sparks in the last 30 days with nothing tested, point
that out kindly but directly before doing anything else.
