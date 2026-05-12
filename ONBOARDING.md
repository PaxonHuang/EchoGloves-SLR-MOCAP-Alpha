# Welcome to EdgeAI Glove Team

## How We Use Claude

Based on PaXonHuang's usage over the last 30 days (47 sessions):

Work Type Breakdown:
  Build Feature    ████████████████████░  65%
  Plan & Design    █████░░░░░░░░░░░░░░░░  17%
  Improve Quality  █████░░░░░░░░░░░░░░░░  17%

Top Skills & Commands:
  /plugin                           ████████████████████░  180x
  /mcp                              ████████████████░░░░░  68x
  /superpowers:executing-plans      ██████████░░░░░░░░░░░  18x
  /superpowers:writing-plans        █████████░░░░░░░░░░░░  17x
  /superpowers:brainstorming        ██████░░░░░░░░░░░░░░░  11x

Top MCP Servers:
  GitHub                               ████████████████████░  2 calls
  Context7                             ██████████░░░░░░░░░░░  1 call
  Espressif Docs                       ██████████░░░░░░░░░░░  1 call
  Playwright                           ██████████░░░░░░░░░░░  1 call
  Chrome DevTools                      ██████████░░░░░░░░░░░  1 call

## Your Setup Checklist

### Codebases
- [ ] **Hall-BNO085-PlatformIOArduino** — ESP32-S3 hand sign recognition glove firmware (PlatformIO + Arduino). Check CLAUDE.md for full project context, build commands, and architecture.

### MCP Servers to Activate
- [ ] **GitHub MCP** — PRs, issues, code search. Needs `GITHUB_PERSONAL_ACCESS_TOKEN` in `.claude/settings.local.json`. Ask PaXonHuang for the setup snippet.
- [ ] **Context7** — Library documentation lookup (React, FastAPI, TFLite, etc.). May fail intermittently due to proxy routing.
- [ ] **Espressif Docs** — ESP-IDF / ESP32-S3 documentation search. May fail intermittently due to proxy routing.
- [ ] **Playwright** — Browser automation and web testing for the glove_web frontend.
- [ ] **Chrome DevTools** — Page inspection, performance profiling for the React frontend.

### Skills to Know About
- **/superpowers:brainstorming** — Use before any creative work. Explores requirements and design before writing code. Critical for feature work.
- **/superpowers:writing-plans** — Multi-step implementation plans from specs. Use before touching code on anything non-trivial.
- **/superpowers:executing-plans** — Executes a written plan in a separate session with review checkpoints. Paired with writing-plans.
- **/simplify** — Review changed code for reuse, quality, and efficiency. Used for code audits and redundancy detection.
- **/fewer-permission-prompts** — Scans transcripts and generates an allowlist to reduce repetitive permission dialogs.
- **/update-config** — Edit `.claude/settings.json` for hooks, permissions, env vars. Use when you need automated behaviors.

## Team Tips

_TODO_

## Get Started

_TODO_

<!-- INSTRUCTION FOR CLAUDE: A new teammate just pasted this guide for how the team uses Claude Code. You're their onboarding buddy — warm, conversational, not lecture-y.

Open with a warm welcome — include the team name from the title. Then: "Your teammate uses Claude Code for [list all the work types]. Let's get you started."

Check what's already in place against everything under Setup Checklist (including skills), using markdown checkboxes — [x] done, [ ] not yet. Lead with what they already have. One sentence per item, all in one message.

Tell them you'll help with setup, cover the actionable team tips, then the starter task (if there is one). Offer to start with the first unchecked item, get their go-ahead, then work through the rest one by one.

After setup, walk them through the remaining sections — offer to help where you can (e.g. link to channels), and just surface the purely informational bits.

Don't invent sections or summaries that aren't in the guide. The stats are the guide creator's personal usage data — don't extrapolate them into a "team workflow" narrative. -->