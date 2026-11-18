# cAdvent 2026

A first-person 3D advent calendar for retro gamers. Walk through a solemn,
candle-lit festive hall with **24 portals**. From **1 to 24 December 2026** one
more portal opens each day, and behind each one waits a ~20-minute original game
that celebrates a classic PC/console game from about 25 years ago, re-imagined in
first person with low-res retro 3D, dynamic lighting and shadows.
Collect the **Shining Star** hidden in each of the first 23 portals and set them
into the Star Arch to open the 24th on Christmas Eve, and watch out for the
marks the crew left everywhere: *"Dr.Doom did this"*, *"Bloodmaker was here"*,
*"Beware of Messerjocke"*…

* **Tech:** C17 · SDL3 · OpenGL 3.3 / WebGL 2 · Emscripten · WebXR
* **Targets:** Windows, Linux, Web (touch controls on mobile), WebXR
* **Process:** [OpenSpec](openspec/) spec-driven development, ≥ 95 % unit-test coverage, automated game tests and completion bots
* **Multiplayer (optional):** split screen and online via WebRTC

## Concept

Start with **[docs/concept/README.md](docs/concept/README.md)**: vision & hub,
technology, multiplayer, quality, production plan and all 24 portal designs.

Current OpenSpec change: [`bootstrap-advent-calendar`](openspec/changes/bootstrap-advent-calendar/proposal.md)

```bash
openspec validate --strict
```

## Licence

* Code: [MIT](LICENSE)
* Art, audio, maps & content in `assets/`: [CC BY 4.0](LICENSE-ASSETS.md)

All portal games are original works inspired by genres and mechanics. They
contain no names, characters, art, levels, sounds or music from the games that
inspired them. Those titles belong to their respective owners.
