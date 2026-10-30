# Contributing to Wye

Bug reports, ideas and pull requests are all welcome.

## Before you open a pull request

1. Build and run the tests: `./build.c tests`.
2. If you changed `firmware/`, `proto/` or `devcore/`, also build the firmware
   with `./build.c firmware` and try it on real Picos if you can.
3. Keep changes focused, and match the style of the code around them.

## How the code fits together

`proto/` is the wire protocol and `devcore/` the device logic. Both are shared
by the app, the firmware and the tests, so most device behavior can be tested
on a computer with the simulator in `tests/`.

## License

By contributing, you agree that your work is licensed under the
[GNU GPL, version 3 or later](LICENSE), like the rest of the project.
