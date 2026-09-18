# Contributing to WoWx

Thanks for wanting to help! There's still a lot left to do, and useful bug
reports, hardware tests and documentation fixes all help move this along.

## Before starting

Have a look at the [README](README.md), [roadmap](docs/ROADMAP.md) and
[feature checklist](docs/PARITY.md). They explain the goal and what's already
being worked on. For a big feature or a change to how something works, open an
issue first so we can talk it through before you spend time on it.

Please follow the [code of conduct](CODE_OF_CONDUCT.md) in issues, discussions
and pull requests.

## Reporting a bug or sharing a test

Use the forms in [New issue](https://github.com/TommySanzCode/WoWx/issues/new/choose).
Include the build or commit, what you tried, what you expected and what actually
happened. Say whether it happened on an Xbox, in xemu or in the PC tools.

For hardware tests, include the console revision if known, RAM size, dashboard,
video setup and controller. Screenshots and short videos help, especially if
they show the memory or timing display. Emulator results and real Xbox results
need to be clearly identified.

Before posting logs or screenshots, remove passwords, tokens, personal file
paths and anything else you don't want public. For a security problem, use
the [private reporting process](SECURITY.md) instead of a public issue.

Setup questions and general ideas can go in
[Discussions](https://github.com/TommySanzCode/WoWx/discussions).

## Working on the code

1. Fork the repo and create a branch for your change.
2. Follow the [build notes](docs/BUILDING.md). You'll need your own legit copy
   of Vanilla WoW 1.12.1, build 5875, plus the required development tools.
3. Keep machine-specific paths in environment variables or the ignored
   `config/local.paths.json`. The example config shows the available settings.
4. Make a focused change and follow the style of the surrounding code.
5. Check the affected behavior and explain the results in your pull request.

The target is a stock 64 MB Xbox. Watch memory use, loading time and failure
handling when changing asset loading, rendering or networking. Keep the
existing dependency pins unless changing a dependency is part of the proposal.

Please avoid mixing unrelated cleanup or large formatting changes into a fix.
If a file format or setup step changes, update the relevant notes too.

## Testing

Run the checks that cover your change. After building the host tools, CTest can
run individual checks, for example:

```sh
ctest --test-dir build/host -R character_controller_ui --output-on-failure
```

The tests in `tests/` and the checks in `tools/` cover different parts of the
client. Some need prepared game assets or your own disposable local server.
Use the [playtest notes](docs/PLAYTEST.md) for emulator and gameplay tests.

In the PR, include the commands you ran, what passed and what you couldn't test.
For documentation-only changes, checking the instructions and links is enough.
For visible changes, include a screenshot if you can. Don't describe an emulator
or isolated test as a completed hardware test.

## Files and licensing

Only submit material you have the right to contribute. Don't commit original
game archives, extracted assets, firmware, emulator disks, credentials, server
databases or XBE/ISO packages. Don't upload those files to issues or PRs either.

Original contributions use the [project license](LICENSE): MIT with the same
additional restriction on commercial game use as WoWee. By submitting a
contribution, you agree to offer your original changes under those terms.
Existing third-party code keeps its own license. Keep its notices and credit
intact, and document any added dependency in [NOTICE.md](NOTICE.md).

## Sending a pull request

Target `main` and fill out the PR template. Explain the problem and what your
change does, link any related issue, and include the testing information.
Small, understandable changes are much easier to review. If it's still in
progress, a draft PR is fine.
